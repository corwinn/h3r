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

#ifndef _H3R_GAMEWINDOW_H_
#define _H3R_GAMEWINDOW_H_

#include "h3r.h"
#include "h3r_dialogwindow.h"
#include "h3r_event.h"

H3R_NAMESPACE

// Primary window: this where you play.
//
// A dialog:
//  - when you go to the main menu, there is no "back"
//
// Transition? - showing kingdom overview seems to be using cross-fading
//TODO option: fade in/out window vfx
//
// Resources:
//   - background: AdvMap.pcx
//   - status bar: AResBar.pcx, HEROQVBK.PCX, TResBar.pcx
//   - "Help.txt" - hints
//   - mouse cursors: CRADVNTR.def (adv map), CRCOMBAT.def (battle),
//                  crdeflt.def (mouse ptr, wait ptr)
//   - map frame: EDG.def
//   - Game::CurrentPlayerColor
//     - next hero: iam000.def
//     - end turn: iam001.def
//     - kingdom overview: iam002.def
//     - surface view: iam003.def
//     - quest log: iam004.def
//     - sleep: iam005.def
//     - move: iam006.def
//     - spell: iam007.def
//     - adv. options: iam008.def
//     - sys. options: iam009.def
//     - underworld view: iam010.def
//     - wake: iam011.def
//     - hero list: up iam012.def
//     - hero list: down iam013.def
//     - town list: up iam014.def
//     - town list: down iam015.def
//     - status bar resources: TSRESOUR.DEF? - see above
//   - map scroll: RADAR.def
//   - fog of war: Tshrc.def
//   - fog of war edges: Tshre.def
//Is there any reason for these 2 to be not gnerated on the fly? - there isn't.
//TODO shader
//   - mana bar L of hero portrait: imana.def
//   - movement points R of hero portrait: imobil.def
//   - towns: ITPA.def
//     - hint: town: itpt.def
//  - hero details view
//     - background: AdStatHr.pcx, HEROQVBK.PCX
//     - morale: IMRL22.def
//     - luck: ILCK22.def
//  - map
//    - terrain tiles: Lavatl.def, Watrtl.def, Snowtl.def, Swmptl.def,
//      sandtl.def, GRASTL.def, Subbtl.def, ROUGTL.def, rocktl.def, DIRTTL.def
//
#undef public
class GameWindow final : public DialogWindow, public IHandleEvents
#define public public:
{
    public GameWindow(Window * base_window) {}
    public ~GameWindow() override {}
};// GameWindow

NAMESPACE_H3R

#endif