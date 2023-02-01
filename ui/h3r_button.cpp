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
#include "h3r_sdlrwops.h"

H3R_NAMESPACE

Button::Button(const String & res_name, Control * base)
    : Control {base}
{
    Def btn_def {Game::GetResource (res_name)};
    auto n = res_name.ToLower ().Replace (".def", "n.pcx");
    auto btn_def_n = btn_def.Query (n);
    if (! btn_def_n) {
        H3R_NS::Log::Err (String::Format (
            "Can't find %s[%s]" EOL, res_name.AsZStr (), n.AsZStr ()));
        return;
    }
    auto byte_arr_ptr = btn_def_n->ToRGBA ();
    if (! byte_arr_ptr || byte_arr_ptr->Empty ()) {
        H3R_NS::Log::Err (String::Format (
            "Can't load %s[%s]" EOL, res_name.AsZStr (), n.AsZStr ()));
        return;
    }

    Resize (btn_def_n->Width (), btn_def_n->Height ());

    _en = TexCache::One ()->Cache (
        btn_def_n->Width (),
        btn_def_n->Height (),
        byte_arr_ptr->operator byte * (), 4);

    n = res_name.ToLower ().Replace (".def", "h.pcx");
    btn_def_n = btn_def.Query (n);
    byte_arr_ptr = btn_def_n->ToRGBA ();
    _eh = TexCache::One ()->Cache (
        btn_def_n->Width (),
        btn_def_n->Height (),
        byte_arr_ptr->operator byte * (), 4);

    //TODO to function; or description;
    n = res_name.ToLower ().Replace (".def", "s.pcx");
    btn_def_n = btn_def.Query (n);
    byte_arr_ptr = btn_def_n->ToRGBA ();
    _es = TexCache::One ()->Cache (
        btn_def_n->Width (),
        btn_def_n->Height (),
        byte_arr_ptr->operator byte * (), 4);

    float fw = static_cast<float>(btn_def_n->Width ()),
        fh = static_cast<float>(btn_def_n->Height ());
    GLfloat t = Pos ().Y, l = Pos ().X, b = t + fh, r = l + fw;
    GLfloat v[48] {
        l,t,_en.l,_en.t, l,b,_en.l,_en.b, r,t,_en.r,_en.t, r,b,_en.r,_en.b,
        l,t,_eh.l,_eh.t, l,b,_eh.l,_eh.b, r,t,_eh.r,_eh.t, r,b,_eh.r,_eh.b,
        l,t,_es.l,_es.t, l,b,_es.l,_es.b, r,t,_es.r,_es.t, r,b,_es.r,_es.b
    };
    glGenBuffers (1, &_vbo);
    glBindBuffer (GL_ARRAY_BUFFER, _vbo);
    glBufferData (GL_ARRAY_BUFFER, 48*sizeof(GLfloat), v, GL_STATIC_DRAW);
}

Control * Button::SetPos(int x, int y)
{
    Control::SetPos (x, y);

    // Engine::UpdateGeometry (_key, v);
    GLfloat t = Pos ().Y, l = Pos ().X, b = t + Size ().Y, r = l + Size ().X;
    GLfloat v[48] {
        l,t,_en.l,_en.t, l,b,_en.l,_en.b, r,t,_en.r,_en.t, r,b,_en.r,_en.b,
        l,t,_eh.l,_eh.t, l,b,_eh.l,_eh.b, r,t,_eh.r,_eh.t, r,b,_eh.r,_eh.b,
        l,t,_es.l,_es.t, l,b,_es.l,_es.b, r,t,_es.r,_es.t, r,b,_es.r,_es.b
    };
    glBindBuffer (GL_ARRAY_BUFFER, _vbo);
    glBufferData (GL_ARRAY_BUFFER, 48*sizeof(GLfloat), v, GL_STATIC_DRAW);
    return this;
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

static Mix_Chunk * global_fx1[MIX_CHANNELS] {};

void Button::OnMouseDown(const EventArgs &)
{
    if (! _mouse_over) return;
    _mouse_down = true;
    Log::Info ("MouseDown" EOL);
    auto stream = Game::GetResource ("BUTTON.wav");
    if (! stream) {
        Log::Err ("Can't load BUTTON.wav" EOL);
        return;
    }
    SdlRWopsStream wav_ops {stream};
    int freesrc = true;
    Mix_Chunk * wav_chunk = Mix_LoadWAV_RW (wav_ops, freesrc);
    if (! wav_chunk) {
        Log::Err ("Mix_LoadWAV_RW: can't load BUTTON.wav" EOL);
        return;
    }
    //TODO This complicates things a little bit
    Mix_ChannelFinished ([](int c) {
        if (global_fx1[c]) {
            printf ("Mix_FreeChunk at global_fx1" EOL);
            Mix_FreeChunk (global_fx1[c]);
            global_fx1[c] = nullptr;
        }
    });
    global_fx1[Mix_PlayChannel (-1, wav_chunk, 0)] = wav_chunk;
}

void Button::OnMouseUp(const EventArgs &)
{
    //TODO Chain of Responsibility can really come in handy here. You clicked on
    //     one control - that's it: the others need not get notified. I don't
    //     need the bubbling/tunneling madness of the "WPF" here.
    if (! _mouse_down) return; // It was not me
    _mouse_down = false;
    // If you release the button outside of the of the thing you clicked on
    // there shall be no mouse click event.
    if (! _mouse_over) return;
    Log::Info ("MouseUp" EOL);
}

void Button::OnRender(GC &)
{
    glLoadIdentity ();

    glEnable (GL_ALPHA_TEST);
    glAlphaFunc (GL_GEQUAL, 1.0f);
    // glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    GLint ofs = 0;
    TexCache::Bind (_en); // It looks like a tex. bind, but it could be not
    if (_mouse_over) {
        if (_mouse_down) {TexCache::Bind (_es); ofs = 8;}
        else { TexCache::Bind (_eh); ofs = 4; }
    }
    glBindBuffer (GL_ARRAY_BUFFER, _vbo);

    // glTranslatef (Pos ().X, Pos ().Y, 0);
    glVertexPointer (
        2, GL_FLOAT, 4*sizeof(GLfloat), (void *)(0));
    glTexCoordPointer (
        2, GL_FLOAT, 4*sizeof(GLfloat), (void *)(2*sizeof(GLfloat)));
    glDrawArrays (GL_TRIANGLE_STRIP, ofs, 4);
    // glTranslatef (-Pos ().X, -Pos ().Y, 0);

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
