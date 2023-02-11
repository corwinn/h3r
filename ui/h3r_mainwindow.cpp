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
#include "h3r_label.h"
#include "h3r_timing.h"
#include "h3r_messagebox.h"

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

MainWindow::MainWindow(OSWindow * actual_window, Point && size)
    : Window{actual_window, static_cast<Point &&>(size)}
{
    Pcx main_window_background {Game::GetResource ("GamSelBk.pcx")};
    auto byte_arr_ptr = main_window_background.ToRGB ();
    if (! byte_arr_ptr || byte_arr_ptr->Empty ()) {
        H3R_NS::Log::Err ("Failed to load GamSelBk.pcx" EOL);
        return;
    }

    auto key = RenderEngine::UI ().GenKey ();
    RenderEngine::UI ().UploadFrame (key, 0, 0, main_window_background.Width (),
        main_window_background.Height (), byte_arr_ptr->operator byte * (), 3,
        "GamSelBk.pcx", 100);

    // -- Controls ---------------------------------------------------------

    int y = 10 /*measured*/, x = 524 /*measured*/, spacing = 0;
    List<String> mm {};
    mm  << "MMENUNG.def" << "MMENULG.def" << "MMENUHS.def" << "MMENUCR.def"
        << "MMENUQT.def";

    // Centered at x=525
    // 1. Measure
    int ww = 0;
    Button * btn_quit {};
    for (const auto & b : mm) {
        Button * btn;
        H3R_CREATE_OBJECT(btn, Button) {b};
        ww = btn->Size ().X > ww ? btn->Size ().X : ww;
        Add (btn->SetPos (x, y));
        y += btn->Size ().Y + spacing;
        btn_quit = btn;
    }
    btn_quit->OnClick = [](Control *) {
        // The mesage is located at GENRLTXT.TXT:81 (1-based)
        auto dr = MessageBox::Show ("Are you sure you want to quit?",
            "MedFont.fnt", MessageBox::Buttons::OKCancel);
        if (DialogResult::OK == dr)
            ;
        //TODO check if all message boxes are using this font
    };
    // 2. Layout
    for (Control * btn : Controls ())
        btn->SetPos (
            btn->Pos ().X + ((ww - btn->Size ().X) / 2), btn->Pos ().Y);
    // 3. Upload
    for (Control * btn : Controls ()) btn->UploadFrames ();

    // Preview all fonts as real-time clocks
    /*List<String> fnt {};
    fnt << "CALLI10R.FNT" << "CREDITS.FNT" << "HISCORE.FNT" << "MedFont.fnt"
        << "TIMES08R.FNT" << "bigfont.fnt" << "smalfont.fnt" << "tiny.fnt"
        << "verd10b.fnt";

    int tx = 365, ty = 285;
    for (const auto & f : fnt) {
        Point tpos {tx, ty};
        Label * lbl;
        H3R_CREATE_OBJECT(lbl, Label) {"00:00:00", f, tpos};
        Add (lbl);
        _time_labels.Add (lbl);
        ty += 32;
    }*/

    Window::MainWindow = this;
}

MainWindow::~MainWindow() { Window::MainWindow = nullptr; }

void MainWindow::OnKeyUp(const EventArgs & e)
{
    printf ("MainWindow::OnKeyUp" EOL);
    if (H3R_KEY_Q == e.Key)
        Close ();
}

void MainWindow::OnShow()
{
}

void MainWindow::OnRender()
{
    Window::OnRender ();

    // Misusing OnRender as a timing source
    /*static int h{}, m{}, s{};
    int nh{}, nm{}, ns{};
    OS::TimeNowHms (nh, nm, ns);
    if (nh != h || nm != m || ns != s) {
        h = nh, m = nm, s = ns;
        auto time_now = String::Format ("%002d:%002d:%002d - ", h, m, s);
        for (auto lbl : _time_labels)
            if (lbl) lbl->SetText (time_now + lbl->Font ());
    }*/
}

void MainWindow::OnResize(int w, int h)
{
    Window::OnResize (w, h); // Uncoditional

    if (! w || ! h) return;
    SetSize (Point {w, h});
}

NAMESPACE_H3R
