/**** BEGIN LICENSE BLOCK ****

BSD 3-Clause License

Copyright (c) 2021-2023, the wind.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**** END LICENCE BLOCK ****/

// Now everything texture-related happens here.

//LATER verify how this maps with "Direct X".

#include "h3r_texcache.h"
#include <GL/glext.h>
#include "h3r_game.h"
#include "h3r_stream.h"
#include "h3r_vfs.h"
#include "h3r_log.h"
#include "h3r_def.h"
#include "h3r_pcx.h"
#include "h3r_resnamehash.h"

H3R_NAMESPACE

GLuint TexCache::_bound {}; // currently bound texture

//TODO figure out a better way; requirements:
//       * The TexCache gets initialized *after* the required context
//       * The TexCache gets released *prior* the required context
//       * The TexCache is accessible to all Window ancestors
//       * The TexCache is accessible to all Control ancestors
//       * The TexCache shall notify for its progress (resource enumeration)
//         and that notification should get rendered somewhere.
/*static*/ TexCache * TexCache::One()
{
    static TexCache TC {}; return &TC;
}

#undef TC_STATS
#ifdef TC_STATS
static int const MAX_SIZE {2048};
static int wa[MAX_SIZE] {};
static int ha[MAX_SIZE] {};
static int most_frequent_w {}, mf_w_id {};
static int most_frequent_h {}, mf_h_id {};
static int total {}, distinct_w {}, distinct_h {};
static int max_w {}, max_h {};
static int min_w {MAX_SIZE}, min_h {MAX_SIZE};
#endif

static GLint H3R_MAX_TEX_SIZE {1<<12};

static GLint TexSize()
{
    GLint i;
    glGetIntegerv (GL_MAX_TEXTURE_SIZE, &i);
    return i < H3R_MAX_TEX_SIZE ? i : H3R_MAX_TEX_SIZE;
}

TexCache::TexCache()
    : _tsize{TexSize ()}
{
#ifdef TC_STATS
    distinct_h = distinct_w = total = most_frequent_w = most_frequent_h = 0;
    for (auto & v : wa) v = 0;
    for (auto & v : ha) v = 0;
    Log::Info ("TexCache: Preliminary" EOL);
    //TODO what follows is way too slow; waaay too slow; it shouldn't be this
    //     slow
    //TODO Buffer the FileStream - see what happens;
    Game::RM->Enumerate (
        [](Stream & s, const VFS::Entry & e) -> bool
        {
            bool const go_on {true};
            int lw {}, lh {};
            if (e.Name.ToLower ().EndsWith (".def")) {
                Def dec {&s};
                lw = dec.Width ();
                lh = dec.Height ();
                total += dec.Num ();
            }
            else if (e.Name.ToLower ().EndsWith (".pcx")) {
                Pcx dec {&s};
                lw = dec.Width ();
                lh = dec.Height ();
                total++;
            }
            else
                return go_on;
            if (lw <= 0 || lw > MAX_SIZE || lh <= 0 || lh > MAX_SIZE) {
                Log::Info (String::Format ("Odd res: (%d x %d), %s",
                    e.Name.AsZStr (), lw, lh));
                return go_on;
            }
            wa[lw]++; if (wa[lw] > most_frequent_w)
                { most_frequent_w = wa[lw]; mf_w_id = lw; }
            if (lw > max_w) max_w = lw;
            if (lw < min_w) min_w = lw;
            ha[lh]++; if (ha[lh] > most_frequent_h)
                { most_frequent_h = ha[lh]; mf_h_id = lh; }
            if (lh > max_h) max_h = lh;
            if (lh < min_h) min_h = lh;
            return go_on;
        }
    );
    // Hm, Perhaps another TaskThread is needed; this blocks the UI startup
    while (! Game::RM->TaskComplete ())
        Game::ProcessThings ();
    for (int i = 0; i < MAX_SIZE; i++) {
        if (wa[i]) distinct_w++;
        if (ha[i]) distinct_h++;
        if (wa[i]/100) printf ("w:%4d: %4d" EOL, i, wa[i]);
        if (ha[i]/100) printf ("h:%4d: %4d" EOL, i, ha[i]);
    }
    Log::Info ("TexCache: Initialized" EOL);
    Log::Info (String::Format (" most freq w    : %d" EOL, mf_w_id));
    Log::Info (String::Format (" most freq w num: %d" EOL, wa[mf_w_id]));
    Log::Info (String::Format (" most freq h    : %d" EOL, mf_h_id));
    Log::Info (String::Format (" most freq h num: %d" EOL, ha[mf_h_id]));
    Log::Info (String::Format (" max w          : %d" EOL, max_w));
    Log::Info (String::Format (" max h          : %d" EOL, max_w));
    Log::Info (String::Format (" min w          : %d" EOL, min_w));
    Log::Info (String::Format (" min h          : %d" EOL, min_h));
    Log::Info (String::Format (" distinct w     : %d" EOL, distinct_w));
    Log::Info (String::Format (" distinct h     : %d" EOL, distinct_h));
    Log::Info (String::Format (" resources      : %d" EOL, total));
#endif

    Log::Info (String::Format ("Open GL  : %s" EOL, glGetString (GL_VENDOR)));
    Log::Info (String::Format (" Renderer: %s" EOL, glGetString (GL_RENDERER)));
    Log::Info (String::Format (" Version : %s" EOL, glGetString (GL_VERSION)));
    Log::Info (String::Format (" GLSL    : %s" EOL,
        glGetString (GL_SHADING_LANGUAGE_VERSION)));

    GLint i;
    glGetIntegerv (GL_MAX_TEXTURE_SIZE, &i);
    Log::Info (String::Format (" HW TSize  : %d" EOL, i));
    // As it happens my accelerator supports 16384x16384 textures - that's
    // 1GB RGBA; way too much bytes for this remake.
    // I'll start with a 4k one, see what happens
    Log::Info (String::Format (" H3R TSize : %d" EOL, _tsize));

}// TexCache::TexCache()

// Be careful when adding keys.
static int const H3R_TEX_KEYS_NUM {6};
// later
//                                                0   1    2    3    4     5
// static int const H3R_TEX_KEYS[H3R_TEX_KEYS_NUM]{32, 64, 128, 256, 512, 1024};
static inline int GetKeyIndex(int key) // transforms a key into an index
{
    //         0  1  2     4           8
    int m[] = {1, 2, 3, 0, 4, 0, 0, 0, 5};
    if (key <= 32) return 0;
    else return m[key>>(H3R_TEX_KEYS_NUM+1)];
}
// That is still way too much memory, a texturing one mind you, but lets
// see what will happen.
// Using GL_COMPRESSED_RGBA_S3TC_DXT3_EXT shall lower the memory usage, although
// I'm not sure about the graphics quality impact.
static TexCache::Entry global_h[H3R_TEX_KEYS_NUM] {}; // group by height

TexCache::~TexCache()
{
    printf ("TexCache::~TexCache()" EOL);
    for (int i = 0; i < H3R_TEX_KEYS_NUM; i++)
        glDeleteTextures (1, &(global_h[i].Texture));
}

static inline int NextP2(int value)//TODO outperform the compiler :)
{
    return value <= 32 ? 32 :
        value <= 64 ? 64 :
        value <= 128 ? 128 :
        value <= 256 ? 256 :
        value <= 512 ? 512 : 1024;
}

TexCache::Entry TexCache::Cache(GLint w, GLint h,
    h3rBitmapCallback data, h3rBitmapFormat fmt,
    const String & key)
{
    static ResNameHash<TexCache::Entry> cache {};
    TexCache::Entry cached_entry {};
    if (cache.TryGetValue (
        key.operator const Array<byte> &(), cached_entry))
        return cached_entry;

    int clasify_h = NextP2 (h);
    int clasify_w = NextP2 (w);
    int id = GetKeyIndex (clasify_h);
    TexCache::Entry & e = global_h[id];
    /*printf (
        "TexCache::Cache: w:%3d, h:%3d, cw:%d, ch:%d, id:%d" EOL,
        w, h, clasify_w, clasify_h, id);*/

    if (0 == e.x && 0 == e.y) {
        glGenTextures (1, &(e.Texture));
        glBindTexture (GL_TEXTURE_2D, _bound = e.Texture);
        // No need for mipmaps, yet
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexImage2D (GL_TEXTURE_2D, 0, /*GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,*/
            GL_RGBA,
            H3R_MAX_TEX_SIZE,
            H3R_MAX_TEX_SIZE,
            0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    }
    Entry result;
    // if there is space left (LR TD logic)
    //LATER much much later; better fit, but keep it fast
    int sx = H3R_MAX_TEX_SIZE - e.x;
    if (e.x > 0 && sx <= clasify_w) { e.x = 0; e.y += clasify_h; } // next row
    int sy = H3R_MAX_TEX_SIZE - e.y;
    sx = H3R_MAX_TEX_SIZE - e.x;
    /*printf (
        "TexCache::Cache: space x:%4d, space y:%4d" EOL,
        sx, sy);*/
    if (sy >= clasify_h) {
        glBindTexture (GL_TEXTURE_2D, _bound = e.Texture);
        glTexSubImage2D (GL_TEXTURE_2D, 0, e.x, e.y, w, h,
            h3rBitmapFormat::RGB == fmt ? GL_RGB : GL_RGBA,
            GL_UNSIGNED_BYTE, data ());
        result.Texture = e.Texture;
        result.l = 1.f * e.x / H3R_MAX_TEX_SIZE; // left
        result.t = 1.f * e.y / H3R_MAX_TEX_SIZE; // top
        result.r = 1.f * (e.x + w) / H3R_MAX_TEX_SIZE; // right
        result.b = 1.f * (e.y + h) / H3R_MAX_TEX_SIZE; // bottom
        /*printf (
            "TexCache::Cache: tex: %12d, top: %00000.4f, bottom: %00000.4f "
            "left: %00000.4f, right: :%00000.4f" EOL,
            result.Texture, result.t, result.b, result.l, result.r);*/
        e.x += w;
    }
    else {
        printf ("One texture is not enough" EOL);
        OS::Exit (11);
        //TODO add another texture
    }
    cache.Add (key.operator const Array<byte> &(), result);
    return result;
}

void TexCache::Invalidate()
{
}

NAMESPACE_H3R