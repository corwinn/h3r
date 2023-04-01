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

NAMESPACE_H3R
