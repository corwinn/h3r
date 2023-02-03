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

#include "h3r_mainwindow.h"
#include "h3r_game.h"
#include "h3r_pcx.h"
#include "h3r_thread.h"
#include "h3r_button.h"
#include "h3r_list.h"
#include "h3r_string.h"
#include "h3r_renderengine.h"

#include "h3r_textrenderingengine.h"

#include <new>

H3R_NAMESPACE

/*
n - up
s - down
d - deactivated
h - hover
   "new game"    : Data_H3sprite_lod/MMENUNG.def
   "load game"   : Data_H3sprite_lod/MMENULG.def ditto
   "high score"  : Data_H3sprite_lod/MMENUHS.def ditto
   "credits"     : Data_H3sprite_lod/MMENUCR.def ditto
   "quit"        : Data_H3sprite_lod/MMENUQT.def ditto
 */

MainWindow::MainWindow(OSWindow * actual_window)
    : Window{actual_window}
{
    RenderEngine::Init ();

    Pcx main_window_background {Game::GetResource ("GamSelBk.pcx")};
    auto byte_arr_ptr = main_window_background.ToRGB ();
    if (! byte_arr_ptr || byte_arr_ptr->Empty ()) {
        H3R_NS::Log::Err ("Failed to load GamSelBk.pcx" EOL);
        return;
    }

    auto key = RenderEngine::UI ().GenKey ();
    RenderEngine::UI ().UploadFrame (key, 0, 0, main_window_background.Width (),
        main_window_background.Height (), byte_arr_ptr->operator byte * (), 3);

    // -- Controls ---------------------------------------------------------

    int y = 10 /*measured*/, x = 525 /*measured*/, spacing = 0;
    List<String> mm {};
    mm  << "MMENUNG.def" << "MMENULG.def" << "MMENUHS.def" << "MMENUCR.def"
        << "MMENUQT.def";

    // Centered at x=525
    // 1. Measure
    int ww = 0;
    for (const auto & b : mm) {
        Button * btn;
        H3R_CREATE_OBJECT(btn, Button) {b};
        ww = btn->Size ().X > ww ? btn->Size ().X : ww;
        Add (btn->SetPos (x, y));
        y += btn->Size ().Y + spacing;
    }
    // 2. Layout
    for (Control * btn : Controls ())
        btn->SetPos (
            btn->Pos ().X + ((ww - btn->Size ().X) / 2), btn->Pos ().Y);
    // 3. Upload
    for (Control * btn : Controls ()) btn->UploadFrames ();
}

MainWindow::~MainWindow() {}

void MainWindow::OnKeyUp(const EventArgs & e)
{
    if (H3R_KEY_Q == e.Key)
        Close ();
}

static inline int Log2i(int v) //TODO h3r_math
{
    int r = 1;
    while (r < v) r <<= 1;
    return r;
}

// just for the test; see if everything works up 'till now;
static GLuint global_text_tex {};
static int global_b {}, global_r {};
static GLfloat global_u {}, global_v {};

void MainWindow::OnShow()
{
    Font * font = TextRenderingEngine::One ().TryLoadFont ("tiny.fnt");
    if (! font) {
        printf ("font load failed" EOL);
        return;
    }
    int w, h;
    byte * tb = TextRenderingEngine::One ().RenderText (*font, "Hi", w, h);
    /*printf ("L" EOL);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++)
            printf ("%3d ", tb[y*2*w+2*x]);
        printf (EOL);
    }
    printf ("A" EOL);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++)
            printf ("%3d ", tb[y*2*w+2*x+1]);
        printf (EOL);
    }*/
    printf ("Measured: w:%d, h:%d" EOL, w, h);
    int tw = Log2i (w), th = Log2i (h);
    Array<byte> tex_buf {(size_t)tw*th*2};
    Font::CopyRectangle (tex_buf, tw, 0, 0, tb, w, h);
    printf ("tex L" EOL);
    for (int y = 0; y < th; y++) {
        for (int x = 0; x < tw; x++)
            printf ("%3d ", ((byte *)tex_buf)[y*2*tw+2*x]);
        printf (EOL);
    }
    printf ("Texture: %d, %d" EOL, tw, th);
    int t = 10, l = 10;
    // int b = t + h, r = l + w;
    global_b = t + h, global_r = l + w;
    global_u = 1.f * w / tw, global_v = 1.f * h / th;
    printf ("Texture: u:%00000.2f, v:%00000.2f" EOL, global_u, global_v);
    glGenTextures (1, &global_text_tex);
    glBindTexture (GL_TEXTURE_2D, global_text_tex);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, tw, th,
        0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, tex_buf.operator byte * ());
    /*glBegin (GL_TRIANGLE_STRIP);
        glVertex2d (l, t); glTexCoord2f (0, 0);
        glVertex2d (l, b); glTexCoord2f (0, v);
        glVertex2d (r, t); glTexCoord2f (u, 0);
        glVertex2d (r, b); glTexCoord2f (u, v);
    glEnd ();*/
}

void MainWindow::OnRender()
{
    glClear (GL_COLOR_BUFFER_BIT);
    glLoadIdentity ();

    RenderEngine::UI ().Render ();

    glBindTexture (GL_TEXTURE_2D, global_text_tex);
    int t = 10, l = 10, b = global_b, r = global_r;
    GLfloat u = global_u, v = global_v;
    // printf ("l:%d t:%d r:%d b:%d\n" EOL, l, t, global_r, global_b);
    glBegin (GL_TRIANGLE_STRIP);
        glTexCoord2f (0, 0); glVertex2d (l, t);
        glTexCoord2f (0, v); glVertex2d (l, b);
        glTexCoord2f (u, 0); glVertex2d (r, t);
        glTexCoord2f (u, v); glVertex2d (r, b);
    glEnd ();

    OS::Thread::Sleep (16);//TODO timing
}

void MainWindow::OnResize(int w, int h)
{
    if (! w || ! h) return;
    glViewport (0, 0, _w = w, _h = h);
    glMatrixMode (GL_PROJECTION), glLoadIdentity ();
    // Its a 2D game.
    glOrtho (.0f, _w, _h, .0f, .0f, 1.f);
    // www.opengl.org/archives/resources/faq/technical/transformations.htm
    glTranslatef (.375f, .375f, -.2f);
    glMatrixMode (GL_MODELVIEW), glLoadIdentity ();
}

NAMESPACE_H3R
