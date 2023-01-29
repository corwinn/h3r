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

#include "h3r_button.h"
#include "h3r_game.h"
#include "h3r_def.h"
#include "h3r_log.h"
#include "h3r_string.h"

H3R_NAMESPACE

Button::Button(const String & res_name, Control * base)
    : Control {base}
{
    Def btn_def {Game::GetResource (res_name)};
    auto n = res_name.ToLower ().Replace (".def", "n.pcx");
    auto btn_def_n = btn_def.Query (n);
    if (! btn_def_n) {
        H3R_NS::Log::Err (String::Format (
            "Can't find %s[%s]", res_name.AsZStr (), n.AsZStr ()));
        return;
    }
    auto byte_arr_ptr = btn_def_n->ToRGBA ();
    if (! byte_arr_ptr || byte_arr_ptr->Empty ()) {
        H3R_NS::Log::Err (String::Format (
            "Can't load %s[%s]", res_name.AsZStr (), n.AsZStr ()));
        return;
    }

    Resize (btn_def_n->Width (), btn_def_n->Height ());

    glDisable (GL_COLOR_MATERIAL);
    glEnable (GL_TEXTURE_2D);
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glEnable (GL_CULL_FACE), glCullFace (GL_BACK);
    glEnable (GL_VERTEX_ARRAY); glEnable (GL_TEXTURE_COORD_ARRAY);
    glClearColor (.0f, .0f, .0f, 1.f);
    glDisable (GL_DEPTH_TEST);
    glDisable (GL_DITHER);
    glDisable (GL_BLEND);
    glDisable (GL_LIGHTING);
    glDisable (GL_FOG);
    glDisable (GL_MULTISAMPLE);
    glShadeModel (GL_FLAT);

    glGenTextures (2, _tex);

    glBindTexture (GL_TEXTURE_2D, _tex[0]);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexImage2D (GL_TEXTURE_2D, 0, /*GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,*/
        GL_RGBA,
        btn_def_n->Width (), // TODO power of 2 UI atlas
        btn_def_n->Height (),
        0, GL_RGBA, GL_UNSIGNED_BYTE, byte_arr_ptr->operator byte * ());

    glBindTexture (GL_TEXTURE_2D, _tex[1]);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    n = res_name.ToLower ().Replace (".def", "h.pcx");
    btn_def_n = btn_def.Query (n);
    byte_arr_ptr = btn_def_n->ToRGBA ();
    glTexImage2D (GL_TEXTURE_2D, 0, /*GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,*/
        GL_RGBA,
        btn_def_n->Width (), // TODO power of 2 UI atlas
        btn_def_n->Height (),
        0, GL_RGBA, GL_UNSIGNED_BYTE, byte_arr_ptr->operator byte * ());

    float fw = static_cast<float>(btn_def_n->Width ()),
        fh = static_cast<float>(btn_def_n->Height ());
    GLfloat v[16] {0,0,0,0, 0,fh,0,1, fw,0,1,0, fw,fh,1,1};
    glGenBuffers (1, &_vbo);
    glBindBuffer (GL_ARRAY_BUFFER, _vbo),
    glBufferData (GL_ARRAY_BUFFER, 16*sizeof(GLfloat), v, GL_STATIC_DRAW);
}

void Button::OnMouseMove(const EventArgs & e)
{
    static Point p;
    p.X = e.X;
    p.Y = e.Y;
    _mouse_over = HitTest (p);
    /*Log::Info (String::Format (
        "Button::OnMouseMove (%d, %d), hover: %d, _bb (%d, %d, %d, %d)" EOL,
        p.X, p.Y, _mouse_over,
        ClientRectangle ().Pos.X, ClientRectangle ().Pos.Y,
        ClientRectangle ().Size.X, ClientRectangle ().Size.Y));*/
}

void Button::OnRender(GC &)
{
    glLoadIdentity ();

    glEnable (GL_ALPHA_TEST);
    glAlphaFunc (GL_GEQUAL, 1.0f);
    // glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    if (! _mouse_over) glBindTexture (GL_TEXTURE_2D, _tex[0]);
    else glBindTexture (GL_TEXTURE_2D, _tex[1]);
    glBindBuffer (GL_ARRAY_BUFFER, _vbo);

    glTranslatef (Pos ().X, Pos ().Y, 0);
    glVertexPointer (2, GL_FLOAT, 4*sizeof(GLfloat), (void *)(0));
    glTexCoordPointer (
        2, GL_FLOAT, 4*sizeof(GLfloat), (void *)(2*sizeof(GLfloat)));
    glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
    glTranslatef (-Pos ().X, -Pos ().Y, 0);

    /*glDisable (GL_TEXTURE_2D); // implict contract with MainWindow.Show()
                               //LATER to the render engine

    if (! _mouse_over) glColor3d (1.0, 1.0, 1.0);
    else glColor3d (0.0, 1.0, 0.0);
    gc.RenderBox (ClientRectangle ());
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEnable (GL_TEXTURE_2D);*/
    glDisable (GL_ALPHA_TEST);
}

NAMESPACE_H3R
