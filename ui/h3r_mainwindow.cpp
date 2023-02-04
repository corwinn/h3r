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

void MainWindow::OnShow()
{
    auto textkey = RenderEngine::UI ().GenTextKey ();
    RenderEngine::UI ().UploadText (textkey, "tiny.fnt",
        "Hi ,\n I'm a proof of concept edition", 10, 10);
}

void MainWindow::OnRender()
{
    glClear (GL_COLOR_BUFFER_BIT);
    glLoadIdentity ();

    RenderEngine::UI ().Render ();

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
