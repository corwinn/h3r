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

// Now everything texture-related happens here too.

#include "h3r_renderengine.h"
#include "h3r_texcache.h"
#include "h3r_log.h"
#include "h3r_string.h"
#include "h3r_textrenderingengine.h"
#include "h3r_math.h"
#include "h3r_font.h"

#define H3RGL_Debug \
{ \
    for (GLenum _a_; GL_NO_ERROR != (_a_ = glGetError ());) \
        printf ("%s:%d glGetError() error: %d" EOL, __FILE__, __LINE__, _a_); \
}

H3R_NAMESPACE

static const GLsizeiptr H3R_MAX_SPRITE_NUM {1<<17};
static const GLsizeiptr H3R_VERTEX_COMPONENTS {5}; // x,y,z,u,v
static const GLsizeiptr H3R_VERTEX_COORDS {3}; // x,y,z
static const GLsizeiptr H3R_VERTEX_UVCOORDS {2}; // x,y,z
static const GLsizeiptr H3R_SPRITE_VERTICES {4}; // {x,y,u,v}[4]
static const GLsizeiptr H3R_SPRITE_FLOATS
    {H3R_SPRITE_VERTICES * H3R_VERTEX_COMPONENTS};
static const GLsizeiptr H3R_SPRITE_COMPONENT_SIZE {sizeof(H3Rfloat)};

// Prevent pointless debug sessions where Open GL "doesn't work" just because
// there is no context yet.
static bool global_render_gl_init {false};

RenderEngine::RenderEngine() : RenderEngine {H3R_MAX_SPRITE_NUM} {}
RenderEngine::RenderEngine(GLsizeiptr max_sprite_frames)
{
    //LATER RenderEngine::UI ().ChangeZOrder ()

    H3R_ENSURE(global_render_gl_init, "You forgot to call RenderEngine::Init()")

    glGenBuffers (1, &_vbo);
    glBindBuffer (GL_ARRAY_BUFFER, _vbo);
    // The 1st quad is the invisible one (where index points to, to remain not
    // visible). This shouldn't slow down Open GL even a bit.
     // 4=H3R_SPRITE_VERTICES - elements per sprite frame
    _vbo_max_elements = (1 + max_sprite_frames) * H3R_SPRITE_VERTICES;
    glBufferData (GL_ARRAY_BUFFER,
        _vbo_max_elements * H3R_VERTEX_COMPONENTS * H3R_SPRITE_COMPONENT_SIZE,
        nullptr, GL_STATIC_DRAW);
}

RenderEngine::~RenderEngine()
{
    glDeleteBuffers (1, &_vbo);
    while (_texts.Prev ())
        _texts.Prev ()->Delete ();
}

static inline void VBOClientState()
{
    glVertexPointer (H3R_VERTEX_COORDS, GL_FLOAT,
        H3R_VERTEX_COMPONENTS*sizeof(GLfloat), (void *)(0));
    glTexCoordPointer (
        H3R_VERTEX_UVCOORDS, GL_FLOAT,
        H3R_VERTEX_COMPONENTS*sizeof(GLfloat),
        (void *)(H3R_VERTEX_COORDS*sizeof(GLfloat)));
}

// Requires distinct handling because of a bonus color component.
//TODO glColorPointer them all, if it won't slow things down.
static inline void WinVBOClientState()
{
    const int H3R_COLOR_COMPONENTS {4};
    const int H3R_UV_COMPONENTS {2};
    auto stride = (H3R_VERTEX_COMPONENTS+H3R_COLOR_COMPONENTS)*sizeof(GLfloat);
    glVertexPointer (H3R_VERTEX_COORDS, GL_FLOAT, stride, (void *)(0));
    glTexCoordPointer (H3R_VERTEX_UVCOORDS, GL_FLOAT, stride,
        (void *)(H3R_VERTEX_COORDS*sizeof(GLfloat)));
    glColorPointer (H3R_COLOR_COMPONENTS, GL_FLOAT, stride,
        (void *)((H3R_VERTEX_COORDS+H3R_UV_COMPONENTS)*sizeof(GLfloat)));
}

void RenderEngine::Render()
{
    // glBindBuffer (GL_ARRAY_BUFFER, _vbo);
    // glDrawArrays (GL_TRIANGLE_STRIP, 4, 4);
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity ();

    // stage1: big sprite VBO
    glDisable (GL_COLOR_ARRAY); // mandatory on "windows"
        glBindBuffer (GL_ARRAY_BUFFER, _vbo);
        VBOClientState ();
        for (int i = 0; i < _tex2_list.Count (); i++ ) {
            glBindTexture (GL_TEXTURE_2D, _tex2_list[i].Texture);
            /*for (int j = 0; j < _tex2_list[i]._index.Count (); j++) {
                printf ("count [%d][%d]: %d\n", i, j, _tex2_list[i]._count[j]);
                printf ("index [%d][%d]: %d\n", i, j, _tex2_list[i]._index[j]);
            }
            for (int j = 0; j < _tex2_list[i]._index.Count (); j++)
                glDrawArrays (GL_TRIANGLE_STRIP, _tex2_list[i]._index[j],
                    _tex2_list[i]._count[j]);*/
            glMultiDrawArrays (GL_TRIANGLE_STRIP,
                _tex2_list[i]._index.begin (), _tex2_list[i]._count.begin (),
                _tex2_list[i]._index.Count ());
        }
    glEnable (GL_COLOR_ARRAY);

    // stage2: window
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    for (auto w : _win_entries) {
        glBindTexture (GL_TEXTURE_2D, w.Texture);
        glBindBuffer (GL_ARRAY_BUFFER, w.Vbo);
        WinVBOClientState ();
        glDrawArrays (GL_TRIANGLE_STRIP, 0, 12);
    }
    // glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    // stage3: text
    for (auto * n = _texts.Prev (); n != nullptr; n = n->Prev ()) {
        if (! n->Data.Visible) continue;
        glBindTexture (GL_TEXTURE_2D, n->Data.Texture);
        glBindBuffer (GL_ARRAY_BUFFER, n->Data.Vbo);
        WinVBOClientState ();
        if (n->Data.HasTransform) {
            glLoadIdentity ();
            glTranslatef (n->Data.Tx, n->Data.Ty, .0f);
            glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
            glLoadIdentity ();
        }
        else
            glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
    }
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}// RenderEngine::Render()

void RenderEngine::Resize(int w, int h)
{
    if (w <= 0 || h <= 0) return;
    glViewport (0, 0, w, h);
    glMatrixMode (GL_PROJECTION), glLoadIdentity ();
    // Its a 2D game.
    glOrtho (.0f, w, h, .0f, _znear, _zfar); // or 1,2 ?!
    // www.opengl.org/archives/resources/faq/technical/transformations.htm
    glTranslatef (.375f, .375f, _zt);
    glMatrixMode (GL_MODELVIEW), glLoadIdentity ();
}

GLfloat RenderEngine::Depht2z(h3rDepthOrder b)
{
    return b * (-(_zfar - _znear)/(H3R_LAST_DEPTH+1)) - _zt;
}

int RenderEngine::GenKey()
{
    RenderEngine::Entry e {};

    // [elements] the 1st quad is special ( {x,y,z,u,v}[4] )
    e.Base = H3R_SPRITE_VERTICES;

    if (_entries.Count () > 0)
        e.Base = _entries[_entries.Count ()-1].NextBufPos ();
    H3R_ENSURE(e.Base+H3R_SPRITE_VERTICES <= _vbo_max_elements,
        "VBO overflow: either increase its size or implement a multi-VBO "
        "solution")
    _entries.Add (e);
    return _entries.Count () - 1;
}

RenderEngine::TexList & RenderEngine::ListByTexId(GLuint tex_id)
{//LATER optimize
    for (int i = 0; i < _tex2_list.Count (); i++)
        if (_tex2_list[i].Texture == tex_id) return _tex2_list[i];
    // printf ("new TexList" EOL);
    RenderEngine::TexList new_tex {};
    new_tex.Texture = tex_id;
    return _tex2_list.Add (new_tex);
}

int RenderEngine::UploadFrame(
    int key, GLint x, GLint y, GLint w, GLint h,
    h3rBitmapCallback data, h3rBitmapFormat fmt,
    const String & texkey, h3rDepthOrder order)
{
    H3R_ENSURE(key >= 0 && key < (int)_entries.Count (), "Bug: wrong key")
    auto uv = TexCache::One ()->Cache (w, h, data, fmt, texkey);
    H3RGL_Debug
    GLfloat l = x, t = y, b = t + h, r = l + w;
    GLfloat z = Depht2z (order);
    // printf ("Frame: Order: %3d, z: %.5f" EOL, order, z);
    GLfloat v[H3R_SPRITE_FLOATS] {
        l,t,z,uv.l,uv.t, l,b,z,uv.l,uv.b, r,t,z,uv.r,uv.t, r,b,z,uv.r,uv.b};

    auto & e = _entries[key];
    size_t ofs_bytes =
        (e.Base + e.Frames * H3R_SPRITE_VERTICES)
        * H3R_VERTEX_COMPONENTS * sizeof(H3Rfloat);
    size_t buf_bytes = H3R_SPRITE_FLOATS * sizeof(H3Rfloat);
    glBindBuffer (GL_ARRAY_BUFFER, _vbo);
    /*GLint size = 0;
    glGetBufferParameteriv (GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
    printf ("  UploadFrame: t: %d, ofs: %zu, size: %zu, avail: %d" EOL,
        uv.Texture, ofs_bytes, buf_bytes, size);
    for (int i = 0; i < H3R_SPRITE_FLOATS; i++) printf ("%.2f ", v[i]);
    printf (EOL);*/
    glBufferSubData (GL_ARRAY_BUFFER, ofs_bytes, buf_bytes, v);
    H3RGL_Debug
    e.Frames++;
    e.SetTexture (uv.Texture);
    auto & lists = ListByTexId (uv.Texture);
    if (1 == e.Frames) {
        lists._index.Add (e.Base);
        lists._count.Add (H3R_SPRITE_VERTICES);
        e.Key = lists._index.Count () - 1;
    }
    return (e.Frames - 1) * H3R_SPRITE_VERTICES;
}

void RenderEngine::ChangeVisibility(int key, bool value)
{
    H3R_ENSURE(key >= 0 && key < (int)_entries.Count (), "Bug: wrong key")
    RenderEngine::Entry & e = _entries[key];
    if (e.Visible == value || 0 == e.Frames) return;
    e.Visible = value;
    auto & lists = ListByTexId (e.Texture ());
    if (value) lists._index[e.Key] = e.Base + e.Offset;
    else lists._index[e.Key] = 0; // point to the invisible one
}

//TODO handle "missing texture" rendering
void RenderEngine::ChangeOffset(int key, GLint value)
{
    // printf ("RenderEngine::ChangeOffset: key: %d, value: %d" EOL,
    //    key, value);
    H3R_ENSURE(key >= 0 && key < (int)_entries.Count (), "Bug: wrong key")
    if (_entries[key].Offset == value || 0 == _entries[key].Frames) return;
    _entries[key].Offset = value;
    auto & lists = ListByTexId (_entries[key].Texture ());
    // printf ("RenderEngine::ChangeOffset: t: %d" EOL,
    //    _entries[key].Texture ());
    lists._index[_entries[key].Key] = _entries[key].Base + _entries[key].Offset;
}

void RenderEngine::UpdateRenderOrder(int key, h3rDepthOrder order)
{
    glBindBuffer (GL_ARRAY_BUFFER, _vbo);
    H3R_ENSURE(key >= 0 && key < (int)_entries.Count (), "Bug: wrong key")
    RenderEngine::Entry & e = _entries[key];
    size_t ofs_in_bytes = e.Base * H3R_VERTEX_COMPONENTS * sizeof(H3Rfloat);
    size_t buf_in_bytes = e.Frames * H3R_SPRITE_FLOATS * sizeof(H3Rfloat);
    Array<H3Rfloat> buf_data {static_cast<int>(H3R_SPRITE_FLOATS) * e.Frames};
    H3Rfloat * buf = buf_data;
    glGetBufferSubData (GL_ARRAY_BUFFER, ofs_in_bytes, buf_in_bytes, buf);
    H3RGL_Debug
    // z is each 3rd: {x,y,z,u,v}
    for (int i = 0; i < buf_data.Length (); i += H3R_VERTEX_COMPONENTS)
        buf_data[i+2] = Depht2z (order);
    glBufferSubData (GL_ARRAY_BUFFER, ofs_in_bytes, buf_in_bytes, buf);
    H3RGL_Debug
}
// The code below, and the one above, doesn't look the same; they're the same.
void RenderEngine::UpdateLocation(int key, GLint dx, GLint dy)
{
    glBindBuffer (GL_ARRAY_BUFFER, _vbo);
    H3R_ENSURE(key >= 0 && key < (int)_entries.Count (), "Bug: wrong key")
    RenderEngine::Entry & e = _entries[key];
    // all sprite frames
    size_t ofs_in_bytes = e.Base * H3R_VERTEX_COMPONENTS * sizeof(H3Rfloat);
        /*(e.Base + e.Frames * H3R_SPRITE_VERTICES)
        * H3R_VERTEX_COMPONENTS * sizeof(H3Rfloat);*/
    size_t buf_in_bytes = e.Frames * H3R_SPRITE_FLOATS * sizeof(H3Rfloat);
    Array<H3Rfloat> buf_data {static_cast<int>(H3R_SPRITE_FLOATS) * e.Frames};
    H3Rfloat * buf = buf_data;
    /*printf ("key: %d, ofs_in_bytes: %ld, buf_in_bytes: %ld\n", key,
        ofs_in_bytes, buf_in_bytes);*/
    glGetBufferSubData (GL_ARRAY_BUFFER, ofs_in_bytes, buf_in_bytes, buf);
    H3RGL_Debug
    // {x,y,z,u,v}
    for (int i = 0; i < buf_data.Length (); i += H3R_VERTEX_COMPONENTS)
        buf_data[i] += dx, buf_data[i+1] += dy;
    glBufferSubData (GL_ARRAY_BUFFER, ofs_in_bytes, buf_in_bytes, buf);
    H3RGL_Debug
}
void RenderEngine::SetLocation(int key, GLint x, GLint y)
{
    glBindBuffer (GL_ARRAY_BUFFER, _vbo);
    H3R_ENSURE(key >= 0 && key < (int)_entries.Count (), "Bug: wrong key")
    RenderEngine::Entry & e = _entries[key];
    // all sprite frames
    size_t ofs_in_bytes = e.Base * H3R_VERTEX_COMPONENTS * sizeof(H3Rfloat);
        /*(e.Base + e.Frames * H3R_SPRITE_VERTICES)
        * H3R_VERTEX_COMPONENTS * sizeof(H3Rfloat);*/
    size_t buf_in_bytes = e.Frames * H3R_SPRITE_FLOATS * sizeof(H3Rfloat);
    Array<H3Rfloat> buf_data {static_cast<int>(H3R_SPRITE_FLOATS) * e.Frames};
    H3Rfloat * buf = buf_data;
    /*printf ("key: %d, ofs_in_bytes: %ld, buf_in_bytes: %ld\n", key,
        ofs_in_bytes, buf_in_bytes);*/
    glGetBufferSubData (GL_ARRAY_BUFFER, ofs_in_bytes, buf_in_bytes, buf);
    H3RGL_Debug
    // {x,y,z,u,v}
    for (int i = 0; i < buf_data.Length ();
        i += H3R_VERTEX_COMPONENTS*H3R_SPRITE_VERTICES) {
        H3Rfloat dx = x-buf_data[i], dy = y-buf_data[i+1]; // [0]{x,y}
        for (int v = 0; v < H3R_SPRITE_VERTICES; v++)
            buf[v*H3R_VERTEX_COMPONENTS
                +i*H3R_VERTEX_COMPONENTS*H3R_SPRITE_VERTICES] += dx,
            buf[v*H3R_VERTEX_COMPONENTS
                +i*H3R_VERTEX_COMPONENTS*H3R_SPRITE_VERTICES+1] += dy;
    }
    glBufferSubData (GL_ARRAY_BUFFER, ofs_in_bytes, buf_in_bytes, buf);
    H3RGL_Debug
}

/*static*/ void RenderEngine::Init()
{
    glDisable (GL_COLOR_MATERIAL);
    glEnable (GL_TEXTURE_2D);
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glEnable (GL_CULL_FACE), glCullFace (GL_BACK);

    glClearColor (.0f, .0f, .0f, 1.f);

    glEnable (GL_DEPTH_TEST);
    glDepthFunc (GL_GEQUAL);
    glClearDepth (0.0f);

    glDisable (GL_DITHER);
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable (GL_LIGHTING);
    glDisable (GL_FOG);
    glDisable (GL_MULTISAMPLE);
    glShadeModel (GL_FLAT);

    glEnable (GL_ALPHA_TEST);
    // Only a>0 shall pass - stop a=0 fragments from discarding underlying
    // (their.z>my.z) fragments, by the depth test.
    // Implicit contract with ResDecoder.ToRGBA()
    glAlphaFunc (GL_GREATER, 0.0f);

    glEnable (GL_VERTEX_ARRAY);
    glEnable (GL_TEXTURE_COORD_ARRAY);
    glEnable (GL_COLOR_ARRAY);

    global_render_gl_init = true;
}

// -- Text -------------------------------------------------------------------

RenderEngine::TextKey::TextKey(LList<RenderEngine::TextEntry> & tail)
{
    H3R_CREATE_OBJECT(_node, LList<RenderEngine::TextEntry>) {};
    tail.Insert (_node);
}

void RenderEngine::TextKey::Delete()
{
    auto * node = _node->Delete ();
    if (node->Data.InUse) {
        glDeleteBuffers (1, &(node->Data.Vbo));
        glDeleteTextures (1, &(node->Data.Texture));
    }
    H3R_DESTROY_OBJECT (node, LList<RenderEngine::TextEntry>)
}

RenderEngine::TextKey RenderEngine::GenTextKey()
{
    return RenderEngine::TextKey {_texts};
}

// Slow and simple. Let me establish the most use-able protocol first. The game
// haven't even been started yet; no need to fine tune anything just yet. This
// is proof of concept code - wasting too much time with it is pointless.
void RenderEngine::UploadText(TextKey & key,
    const String & font_name, const String & txt, int left, int top,
    unsigned int color, h3rDepthOrder order)
{
    int w, h;
    byte * tb = TextRenderingEngine::One ().RenderText (font_name, txt, w, h);
    // This is wrong when the actual w is greater than w
    /*for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++)
            printf ("%2X ", tb[y*2*w+2*x]);
        printf ("\n");
    }*/
    int tw = Log2i (w), th = Log2i (h);
    Array<byte> tex_buf {tw*th*2};
    Font::CopyRectangle (tex_buf, tw, 0, 0, tb, w, h,
        TextRenderingEngine::One ().TexBufferPitch ());
    // int b = t + h, r = l + w;
    GLfloat font_size = 1.f;
    GLfloat t = top, l = left, b = t + font_size*h, r = l + font_size*w;
    GLfloat u = 1.f * w / tw, v = 1.f * h / th;
    RenderEngine::TextEntry & e = key.Entry ();
    if (e.InUse) {
        glDeleteBuffers (1, &(e.Vbo));
        glDeleteTextures (1, &(e.Texture));
    }
    else
        e.InUse = true;
    glGenTextures (1, &(e.Texture));
    glBindTexture (GL_TEXTURE_2D, e.Texture);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, tw, th,
        0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, tex_buf.operator byte * ());
    // same ordering and components as the big VBO
    GLfloat z = Depht2z (order);
    // printf ("Text: Order: %3d, z: %.5f" EOL, order, z);
    union {unsigned int c; byte rgba[4]; };
    c = color; GLfloat i = rgba[0]/255.f, g = rgba[1]/255.f, j = rgba[2]/255.f,
        a = rgba[3]/255.f;
    GLfloat verts[36] {l,t,z,0,0,i,g,j,a, l,b,z,0,v,i,g,j,a,
        r,t,z,u,0,i,g,j,a, r,b,z,u,v,i,g,j,a};
    glGenBuffers (1, &(e.Vbo));
    glBindBuffer (GL_ARRAY_BUFFER, e.Vbo);
    glBufferData (GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
}

//TODO refactor me
void RenderEngine::UpdateText(TextKey & key,
    const String & font_name, const String & txt, int left, int top,
    unsigned int color, h3rDepthOrder order)
{
    UploadText (key, font_name, txt, left, top, color, order);
}

void RenderEngine::ChangeTextVisibility(TextKey & key, bool state)
{
    key.ChangeTextVisibility (state);
}

void RenderEngine::ChangeTextColor(TextKey & key, unsigned int color)
{
    RenderEngine::TextEntry & e = key.Entry ();
    if (! e.InUse) return; //LATER H3R_ENSURE?
    glBindBuffer (GL_ARRAY_BUFFER, e.Vbo);
    GLfloat verts[36] {}; // {x,y,z,u,v,r,g,b,a}[4]
    glGetBufferSubData (GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    H3RGL_Debug
    union {unsigned int c; byte rgba[4]; };
    c = color; GLfloat r = rgba[0]/255.f, g = rgba[1]/255.f, b = rgba[2]/255.f,
        a = rgba[3]/255.f;
    for (int i = 0; i < 36; i += 9)
        verts[i+5] = r, verts[i+6] = g, verts[i+7] = b, verts[i+8] = a;
    glBufferSubData (GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    H3RGL_Debug
}

void RenderEngine::TextSetTranslateTransform(TextKey & key, bool state,
    GLfloat tx, GLfloat ty)
{
    key.SetTranslateTransform (state, tx, ty);
}

void RenderEngine::DeleteText(TextKey & key)
{
    key.Delete ();
}

// -- Window -----------------------------------------------------------------

void RenderEngine::ShadowRectangle(GLint x, GLint y, GLint w, GLint h,
    const byte * tile, h3rBitmapFormat tile_fmt, int tile_w, int tile_h,
    h3rDepthOrder order)
{
    // 3 rectangles: light shadow; darker shadow, tiled one
    GLfloat u = 1.f * w / tile_w, v = 1.f * h / tile_h;
    GLfloat a = .5f; // looks close enough
    GLfloat const S {8.f}; // drop-shadow size [pixels]
    GLfloat l = x, t = y, r = l+w, b = t+h, z = Depht2z (order);
    GLfloat vx[3*4*9] { // 3 r., 4 v., 9 c. {x,y,z,u,v,r,g,b,a}
        l+S,t+S,z, 0,0, 0,0,0,a,
        l+S,b+S,z, 0,0, 0,0,0,a,
        r+S,t+S,z, 0,0, 0,0,0,a,
        r+S,b+S,z, 0,0, 0,0,0,a,     // light shadow
        l+S+1,t+S+1,z, 0,0, 0,0,0,a,
        l+S+1,b+S-1,z, 0,0, 0,0,0,a,
        r+S-1,t+S+1,z, 0,0, 0,0,0,a,
        r+S-1,b+S-1,z, 0,0, 0,0,0,a, // darker shadow
        l,t,z, 0,0, 1,1,1,1,
        l,b,z, 0,v, 1,1,1,1,
        r,t,z, u,0, 1,1,1,1,
        r,b,z, u,v, 1,1,1,1};        // tiled background
    RenderEngine::WinEntry e {};
    glGenTextures (1, &(e.Texture));
    glBindTexture (GL_TEXTURE_2D, e.Texture);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, tile_w, tile_h,
        0, h3rBitmapFormat::RGB == tile_fmt ? GL_RGB : GL_RGBA,
        GL_UNSIGNED_BYTE, tile);
    H3RGL_Debug
    glGenBuffers (1, &(e.Vbo));
    glBindBuffer (GL_ARRAY_BUFFER, e.Vbo);
    glBufferData (GL_ARRAY_BUFFER, 3*4*9*sizeof(GLfloat), vx, GL_STATIC_DRAW);
    H3RGL_Debug
    _win_entries.Push (e);
}// RenderEngine::ShadowRectangle()

void RenderEngine::DeleteShadowRectangle()
{
    auto e = _win_entries.Pop ();
    glDeleteBuffers (1, &(e.Vbo));
    glDeleteTextures (1, &(e.Texture));
}

NAMESPACE_H3R
