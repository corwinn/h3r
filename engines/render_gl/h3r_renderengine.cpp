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

H3R_NAMESPACE

static const GLsizeiptr H3R_MAX_SPRITE_NUM {1<<17};
static const GLsizeiptr H3R_VERTEX_COMPONENTS {4}; // x,y,u,v
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
    //TODO then figure out little z-offsets (relative) for each control
    //     (f(z-order) so the entire UI shall be rendered in one gl call
    //     GL_DEPTH_TEST; and compare to the above; choose the less-code one
    //
    //TODO texture atlas(es) for the animated sprites (encode frame number at
    //     some pixel; PixelDB - NoSQL :) )

    H3R_ENSURE(global_render_gl_init, "You forgot to call RenderEngine::Init()")

    glGenBuffers (1, &_vbo);
    glBindBuffer (GL_ARRAY_BUFFER, _vbo);
    // The 1st quad is the invisible one (where index points to to remain not
    // visible). This shouldn't slow down Open GL even a bit.
     // 4=H3R_SPRITE_VERTICES - elements per sprite frame
    _vbo_max_elements = (1 + max_sprite_frames) * H3R_SPRITE_VERTICES;
    glBufferData (GL_ARRAY_BUFFER,
        _vbo_max_elements * H3R_VERTEX_COMPONENTS * H3R_SPRITE_COMPONENT_SIZE,
        nullptr, GL_STATIC_DRAW);
    glVertexPointer (2, GL_FLOAT, 4*sizeof(GLfloat), (void *)(0));
    glTexCoordPointer (
        2, GL_FLOAT, 4*sizeof(GLfloat), (void *)(2*sizeof(GLfloat)));
}

RenderEngine::~RenderEngine()
{
    glDeleteBuffers (1, &_vbo);
}

void RenderEngine::Render()
{
    // glBindBuffer (GL_ARRAY_BUFFER, _vbo);
    // glDrawArrays (GL_TRIANGLE_STRIP, 4, 4);
    for (size_t i = 0; i < _tex2_list.Count (); i++ ) {
        glBindTexture (GL_TEXTURE_2D, _tex2_list[i].Texture);
        glMultiDrawArrays (GL_TRIANGLE_STRIP,
            _tex2_list[i]._index.begin (), _tex2_list[i]._count.begin (),
            _tex2_list[i]._index.Count ());
    }
}

int RenderEngine::GenKey()
{
    RenderEngine::Entry e {};
    e.Base = 4; // [elements] the 1st quad is special ( {x,y,u,v}[4] )
    if (_entries.Count () > 0)
        e.Base = _entries[_entries.Count ()-1].NextBufPos ();
    H3R_ENSURE(e.Base <= _vbo_max_elements, "VBO overflow: either increase its"
        " size or implement a multi-VBO solution")
    _entries.Add (e);
    return _entries.Count () - 1;
}

RenderEngine::TexList & RenderEngine::ListByTexId(GLuint tex_id)
{//LATER optimize
    for (size_t i = 0; i < _tex2_list.Count (); i++)
        if (_tex2_list[i].Texture == tex_id) return _tex2_list[i];
    // printf ("new TexList" EOL);
    RenderEngine::TexList new_tex {};
    new_tex.Texture = tex_id;
    return _tex2_list.Add (new_tex);
}

int RenderEngine::UploadFrame(
    int key, GLint x, GLint y, GLint w, GLint h, byte * data, int bpp)
{
    H3R_ENSURE(key >= 0 && key < (int)_entries.Count (), "Bug: wrong key")
    auto uv = TexCache::One ()->Cache (w, h, data, bpp);
    GLfloat l = x, t = y, b = t + h, r = l + w;
    GLfloat v[H3R_SPRITE_FLOATS] {
        l,t,uv.l,uv.t, l,b,uv.l,uv.b, r,t,uv.r,uv.t, r,b,uv.r,uv.b};

    auto & e = _entries[key];
    size_t ofs_bytes =
        (e.Base + e.Frames * 4) * H3R_VERTEX_COMPONENTS * sizeof(H3Rfloat);
    size_t buf_bytes = H3R_SPRITE_FLOATS * sizeof(H3Rfloat);
    // printf ("  UploadFrame: t: %d, ofs: %zu, size: %zu" EOL,
    //    uv.Texture, ofs_bytes, buf_bytes);
    glBufferSubData (GL_ARRAY_BUFFER, ofs_bytes, buf_bytes, v);
    GLenum err = glGetError ();
    if (GL_NO_ERROR != err) Log::Err ("glBufferSubData error" EOL);
    e.Frames++;
    e.SetTexture (uv.Texture);
    auto & lists = ListByTexId (uv.Texture);
    if (1 == e.Frames) {
        lists._index.Add (e.Base);
        lists._count.Add (H3R_SPRITE_VERTICES);
        e.Key = lists._index.Count () - 1;
    }
    return (e.Frames - 1) * H3R_VERTEX_COMPONENTS;
}

void RenderEngine::ChangeVisibility(int key, bool value)
{
    H3R_ENSURE(key >= 0 && key < (int)_entries.Count (), "Bug: wrong key")
    RenderEngine::Entry & e = _entries[key];
    if (e.Visible == value) return;
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
    if (_entries[key].Offset == value) return;
    _entries[key].Offset = value;
    auto & lists = ListByTexId (_entries[key].Texture ());
    // printf ("RenderEngine::ChangeOffset: t: %d" EOL,
    //    _entries[key].Texture ());
    lists._index[_entries[key].Key] = _entries[key].Base + _entries[key].Offset;
}

/* do not delete me, yet
 * void RenderEngine::UpdateGeometry(int key, const H3Rfloat * buf)
{
    H3R_ENSURE(key >= 0 && key < (int)_entries.Count (), "Bug: wrong key")
    RenderEngine::Entry & e = _entries[key];
    glBufferSubData (GL_ARRAY_BUFFER,
        e.Base * H3R_SPRITE_COMPONENTS * sizeof(H3Rfloat),
        e.Num * sizeof(H3Rfloat), buf);
    GLenum err = glGetError ();
    if (GL_NO_ERROR != err) Log::Err ("glBufferSubData error" EOL);
}*/

/*static*/ RenderEngine & RenderEngine::UI()
{
    static RenderEngine ui {H3R_DEFAULT_UI_MAX_SPRITES};
    return ui;
}

/*static*/ void RenderEngine::Init()
{
    glDisable (GL_COLOR_MATERIAL);
    glEnable (GL_TEXTURE_2D);
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glEnable (GL_CULL_FACE), glCullFace (GL_BACK);

    glClearColor (.0f, .0f, .0f, 1.f);
    glDisable (GL_DEPTH_TEST);
    glDisable (GL_DITHER);
    glDisable (GL_BLEND);
    glDisable (GL_LIGHTING);
    glDisable (GL_FOG);
    glDisable (GL_MULTISAMPLE);
    glShadeModel (GL_FLAT);

    glEnable (GL_ALPHA_TEST);
    glAlphaFunc (GL_GEQUAL, 1.0f);

    glEnable (GL_VERTEX_ARRAY);
    glEnable (GL_TEXTURE_COORD_ARRAY);

    global_render_gl_init = true;
}

NAMESPACE_H3R
