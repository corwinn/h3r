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
#include "h3r_newgame.h"
#include "h3r_gamewindow.h"

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

    static byte * bmp_data {};
    auto bitmap_data = []() { return bmp_data; };
    auto key = Window::UI->GenKey ();
    bmp_data = byte_arr_ptr->operator byte * ();
    Window::UI->UploadFrame (key, 0, 0, main_window_background.Width (),
        main_window_background.Height (), bitmap_data,
        h3rBitmapFormat::RGB, "GamSelBk.pcx", Depth ());

    // -- Controls ---------------------------------------------------------

    // They're hcentered against a right-aligned: "MainMenu.pcx" (340x600)?
    // No.

    int y = 10, x = 525, spacing = 0; // measured, measured, guessed
    List<String> mm {};
    mm  << "MMENUNG.def" << "MMENULG.def" << "MMENUHS.def" << "MMENUCR.def"
        << "MMENUQT.def";
    _mm_a = 0, _mm_b = 4;

    // Centered at x=525
    // 1. Measure
    int ww = 0;
    Button * btn_quit {}, * btn_ng {};
    for (const auto & b : mm) {
        Button * btn;
        H3R_CREATE_OBJECT(btn, Button) {b, this};
        ww = btn->Size ().X > ww ? btn->Size ().X : ww;
        btn->SetPos (x, y);
        y += btn->Size ().Y + spacing;
    }
    btn_ng = static_cast<Button *>(Controls ()[_mm_a]);
    btn_ng->Click.Subscribe (this, &MainWindow::NewGame);
    btn_quit = static_cast<Button *>(Controls ()[_mm_b]);
    btn_quit->Click.Subscribe (this, &MainWindow::Quit);
    // 2. Layout
    for (int i = _mm_a; i <= _mm_b; i++) {
        auto btn = Controls ()[i];
        btn->SetPos (
            // no: (800-340)+((340-btn->Size ().X)/2)
            btn->Pos ().X + ((ww - btn->Size ().X) / 2), btn->Pos ().Y);
    }
    // 3. Upload
    for (int i = _mm_a; i <= _mm_b; i++) Controls()[i]->UploadFrames ();

    // New Game buttons
    List<String> ng {};
    ng  << "GTSINGL.def" << "GTMULTI.def" << "GTCAMPN.def" << "GTTUTOR.def"
        << "GTBACK.def";
    _ng_a = _mm_b+1, _ng_b = _ng_a+4;

    // Centered at x=545
    // 1. Measure
    ww = 0, y = 4, x = 545, spacing =-5; // measured, measured, measured
    Button * btn_single {}, * btn_back {};
    for (const auto & b : ng) {
        Button * btn;
        H3R_CREATE_OBJECT(btn, Button) {b, this};
        ww = btn->Size ().X > ww ? btn->Size ().X : ww;
        btn->SetPos (x, y);
        y += btn->Size ().Y + spacing;
    }
    btn_single = static_cast<Button *>(Controls ()[_ng_a]);
    btn_single->Click.Subscribe (this, &MainWindow::NewGameSignleScenario);
    btn_back = static_cast<Button *>(Controls ()[_ng_b]);
    btn_back->Click.Subscribe (this, &MainWindow::NewGameBack);
    // 2. Layout
    for (int i = _ng_a; i <= _ng_b; i++) {
        auto btn = Controls ()[i];
        btn->SetPos (
            btn->Pos ().X + ((ww - btn->Size ().X) / 2), btn->Pos ().Y);
    }
    // 3. Upload
    for (int i = _ng_a; i <= _ng_b; i++) {
        Controls()[i]->UploadFrames ();
        Controls()[i]->SetHidden (true);
    }

    // Preview all fonts as real-time clocks. These should be at depth: 1.
    // Here you go: partially visible text. Freeze on MessageBox, because
    // MainWindow::OnRender() stops being called - here is my block anim. on
    // dialog window, option.
    List<String> fnt {};
    fnt << "CALLI10R.FNT" << "CREDITS.FNT" << "HISCORE.FNT" << "MedFont.fnt"
        << "TIMES08R.FNT" << "bigfont.fnt" << "smalfont.fnt" << "tiny.fnt"
        << "verd10b.fnt";

    int tx = 365, ty = 285;
    for (const auto & f : fnt) {
        Point tpos {tx, ty};
        Label * lbl;
        H3R_CREATE_OBJECT(lbl, Label) {"00:00:00", f, tpos, this};
        _time_labels.Add (lbl);
        ty += 32;
    }

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
    Def sprite {Game::GetResource ("crdeflt.def")};
    H3R_ENSURE(sprite.Query ("CursrD00.pcx"), "Sprite not found")
    IWindow::MousePtrInfo mp {};
    mp.Bitmap = sprite.ToRGBA ()->operator byte * ();
    mp.Width = sprite.Width ();
    mp.Height = sprite.Height ();
    mp.BitmapFormat = h3rBitmapFormat::RGBA;
    SetMouseCursor (mp);
}

void MainWindow::OnRender()
{
    Window::OnRender ();

    // Misusing OnRender as a timing source
    static int h{}, m{}, s{};
    int nh{}, nm{}, ns{};
    OS::TimeNowHms (nh, nm, ns);
    if (nh != h || nm != m || ns != s) {
        h = nh, m = nm, s = ns;
        auto time_now = String::Format ("%002d:%002d:%002d - ", h, m, s);
        for (auto lbl : _time_labels)
            if (lbl) lbl->SetText (time_now + lbl->Font ());
    }
}

void MainWindow::OnResize(int w, int h)
{
    Window::OnResize (w, h); // Uncoditional

    if (! w || ! h) return;
    SetSize (Point {w, h});
}

void MainWindow::Quit(EventArgs *)
{
    // The mesage is located at GENRLTXT.TXT:81 (1-based)
    auto dr = MessageBox::Show ("Are you sure you want to quit?",
    //TODO check if all message boxes are using this font
        "MedFont.fnt", MessageBox::Buttons::OKCancel);
    if (DialogResult::OK == dr)
        Close ();
}

void MainWindow::NewGame(EventArgs *)
{
    for (int i = _mm_a; i <= _mm_b; i++) Controls ()[i]->SetHidden (true);
    for (int i = _ng_a; i <= _ng_b; i++) Controls ()[i]->SetHidden (false);
}

void MainWindow::NewGameBack(EventArgs *)
{
    for (int i = _mm_a; i <= _mm_b; i++) Controls ()[i]->SetHidden (false);
    for (int i = _ng_a; i <= _ng_b; i++) Controls ()[i]->SetHidden (true);
}

void MainWindow::NewGameSignleScenario(EventArgs *)
{
    String selected_map {};
    {
        NewGameDialog ngd {this};
        if (DialogResult::OK == ngd.ShowDialog ())
            selected_map = ngd.SelectedMap ()->FileName ();
        //TODO DialogWindow.Close - the ~ shouldn't be the only way
    }
    if (! selected_map.Empty ()) {
        //TODO progress
        GameWindow game {this, selected_map};
        game.ShowDialog ();
    }
}

NAMESPACE_H3R
