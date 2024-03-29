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

#include "h3r_gamewindow.h"
#include "h3r_renderengine.h"
#include "h3r_textrenderingengine.h"
#include "h3r_game.h"
#include "h3r_def.h"
#include "h3r_pcx.h"
#include "h3r_label.h"
#include "h3r_button.h"
#include "h3r_gc.h"
#include "h3r_dbg.h"

H3R_NAMESPACE

// _map {map_name, header_only = false}
GameWindow::GameWindow(Window * base_window, const String & map_name)
    : DialogWindow {base_window, Point {800, 600}},
    _map {map_name, false}
{
    auto RE = Window::UI;
    Pal pp {Game::GetResource ("PLAYERS.PAL")};
    Pcx dlg_main {Game::GetResource ("AdvMap.pcx")};
    dlg_main.SetPlayerColor (Game::CurrentPlayerColor, pp);
    UploadFrame (RE->GenKey (), 0, 0, dlg_main, "AdvMap.pcx", Depth ());

    Pcx sbar_back {Game::GetResource ("AResBar.pcx")};
    sbar_back.SetPlayerColor (Game::CurrentPlayerColor, pp);
    UploadFrame (RE->GenKey (), 3, 575, sbar_back, "AResBar.pcx", Depth ());

    Label * lbl {}; // TODO properties
    // int x1 = 40, dx = 84;
    // "RESTYPES.TXT": Wood, Mercury, Ore, Sulfur, Crystal, Gems, Gold
    H3R_CREATE_OBJECT(lbl, Label) {"29",
        "smalfont.fnt", Point {40, 578}, this, H3R_TEXT_COLOR_WHITE};
    H3R_CREATE_OBJECT(lbl, Label) {"9",
        "smalfont.fnt", Point {124, 578}, this, H3R_TEXT_COLOR_WHITE};
    H3R_CREATE_OBJECT(lbl, Label) {"3",
        "smalfont.fnt", Point {208, 578}, this, H3R_TEXT_COLOR_WHITE};
    H3R_CREATE_OBJECT(lbl, Label) {"12",
        "smalfont.fnt", Point {292, 578}, this, H3R_TEXT_COLOR_WHITE};
    H3R_CREATE_OBJECT(lbl, Label) {"17",
        "smalfont.fnt", Point {376, 578}, this, H3R_TEXT_COLOR_WHITE};
    H3R_CREATE_OBJECT(lbl, Label) {"9",
        "smalfont.fnt", Point {460, 578}, this, H3R_TEXT_COLOR_WHITE};
    H3R_CREATE_OBJECT(lbl, Label) {"15920",
        "smalfont.fnt", Point {544, 578}, this, H3R_TEXT_COLOR_WHITE};

    // Date
    H3R_CREATE_OBJECT(lbl, Label) {"Month: 1, Week: 1, Day: 4",
        "smalfont.fnt", Point {622, 578}, this, H3R_TEXT_COLOR_WHITE};

    H3R_CREATE_OBJECT(lbl, Label) {"Status Window",
        "smalfont.fnt", Point {7, 555}, this, H3R_TEXT_COLOR_MSGB,
        false, Point {601-7, 573-555}};

    // Ok, no combo of coastal tile frames makes sense.
    // Water tiles are using palette animation.
    Def sprite {Game::GetResource ("Watrtl.def")};
    int show_them_all = 0;
    _frame_count = 12;
    _frame_id = Window::UI->Offset0 ();
    for (int y = 20; y < 20+32*10; y+=32)
        for (int x = 20; x < 20+32*10; x+=32) {
            auto k = _keys.Add (RE->GenKey ());
            for (int i = 0; i < _frame_count; i++) { // upload some frames
                UploadFrame (k, x, y, sprite,
                    String::Format ("%dWatrtl.def", i),
                    sprite.Query (0, show_them_all)->FrameName (), Depth ());
                // Sea. So says the "gimp" (Colors->Map->Rearrange). 241 and 255
                // are w&b.
                // With the help of "kmag": 228 is stationary (not part of the
                // anim).
                // There is direction specified by the editor I suppose (via
                // frame id at the sprite). The direction shall be verified
                // later by comparing both renderings (original vs remake) under
                // "kmag".
                // Some very high-precision craftsmanship here. Well done.
                sprite.PaletteAnimationL (229, 12);
                // Sea-shore. Assume 242 is stationary too.
                sprite.PaletteAnimationR (243, 12);
            }
            show_them_all++;
            show_them_all %= 33;
        }
    Dbg << "Keys: " << _keys.Count () << EOL;
    Dbg << "Game: " << _map.Name () << EOL;
}

void GameWindow::OnKeyUp(const EventArgs & e)
{
    //TODO behaviour repeats; think about a way
    if (H3R_KEY_ESC == e.Key) {
        _dr = DialogResult::Cancel;
        _has_dr = true;
    }
}

GameWindow::~GameWindow() {}

void GameWindow::OnRender()
{
    static int slow_down {4}; // implicit (TARGET_FPS=32)/4
    Window::OnRender ();
    if (slow_down--) return; else slow_down = 4;
    for (auto sa : _keys)
        Window::UI->ChangeOffset (sa, _frame_id);
    _frame_id += Window::UI->OffsetDistance ();
    if (_frame_id / Window::UI->OffsetDistance () >= _frame_count)
        _frame_id = Window::UI->Offset0 ();
}

NAMESPACE_H3R
