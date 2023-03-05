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

#ifndef _H3R_NEWGAME_H_
#define _H3R_NEWGAME_H_

#include "h3r.h"
#include "h3r_renderengine.h"
#include "h3r_dialogwindow.h"
#include "h3r_string.h"
#include "h3r_event.h"
#include "h3r_scrollbar.h"

H3R_NAMESPACE

// This one doesn't get drop shadow?!
// And it isn't even centered?! How many thing one didn't usually notice :)
//
// New game dialog:
//
//  - tab: "Show Advanced Options" (left): ADVOPTBK.PCX
//    - item: ADOP(B|G|O|P|R|S(pink)|T|Y(tan))PNL.PCX
//    - (starting bonus)left arrow: ADOPLFA.DEF
//    -                             ADOPRTA.DEF - why?! this won't be loaded
//    - (starting bonus image): SCNRSTAR.def; whats this: CampStar.def ?
//    - flag buttons: AOFLGB(B|G|O|P|R|S(pink)|T|Y(tan)).DEF
//    - probably used at the tool tip ("Starting Town"): CPRSMALL.def
//    - castle icons (for the Starting Town column): ITPA.def
//    - Starting Hero: can't select: hpsrand6.pcx; other variations:
//      2 dies with "?": HPSRAND4.PCX ?; heroes with "?": hpsrand.pcx ?
//  - main: GSelPop1.pcx (370x585)
//    [] background: gamselb(0|1).pcx (w: 800, h: 600) - randomly chosen
//    - button "begin": GSPBGIN.DEF  (w: 166, h:  40)
//      - another one: ScnrBeg.def   (w: 166, h:  40) ?!
//    - button "back" : SCNRBACK.DEF (w: 166, h:  40) (n & s only)
// <> This mess speaks of a 3rd party that is ordering it <>
//    [] the odd long buttons (the tab switch ones): GSPBUT2.DEF ( has no < );
//       ( has < ) : GSPBUTT.DEF      (w: 200, h:  20)
//    - diff. buttons: GSPBUT[3;7].DEF - 0-4 (w:  30, h:  46)
//    - mini-static-flags: itgflags.def      (w:  15, h:  20)
//    [] quad arrow down button: ScnrBDn.def; correct and pointless: ScnrBUp.def
//       the other two pointless ones: ScnrBLf.def, ScnrBRt.def
//                                           (w:  16, h:  16)
//    [] scroll-bar middle button: ScnrBSl.def(w:  16, h:  16)
//    [] another scroll-bar set: SCXLBUT.def(w:  44, h:  33), SLIDBUV.def(w:  16,
//       h:  16), SLIDEBUH.def(w:  16, h:  16), SLIDEBUV.def(w:  16, h:  16) ?!
//    [] top-left mini-icon of the selected map size: ScnrMpSz.def
//                                           (w:  29, h:  23)
//    - victory condition mini-icon: SCNRVICT.def (w:  29, h:  21)
//    - loss condition mini-icon: SCNRLOSS.def    (w:  29, h:  21)
//  - tab: "Random Map": RanMapBk.pcx
//  - tab: "Show Available Scenarios" SCSelBck.pcx
//    - group by size buttons: RanSiz(L|M|S|X).def; "All": SCALBUT.DEF
//      - alt1: SC L GBUT.DEF ; alt2: SC M DBUT.DEF ?!
//      - alt3: SC S MBUT.DEF; alt4: SC X LBUT.DEF
//    - sort by Map Version column header button: SCButCp.DEF
//    - map version icon: ScSelC.def
//    - sort by Player Num column header button: SCBUTT1.DEF
//    - sort by Map Size column header button: SCBUTT2.DEF
//    - sort by Map Name column header button: SCBUTT3.DEF
//    - sort by Victory Con column header button: SCBUTT4.DEF
//    - sort by Loss Con column header button: SCBUTT5.DEF
//
//   I can't find the tool-tip windows yet.
//
//  - quad arrows on brown background: "ComSlide.def" ?!
//  - load progress bars: loadprog.def
//  - load background: loadbar.pcx
#undef public
class NewGameDialog : public DialogWindow, public IHandleEvents
#define public public:
{
    // private List<int> _re_keys {};
    private bool _has_dr {};
    private DialogResult _dr {};
    private int _t {}, _l {};

    public NewGameDialog(Window * base_window);
    public ~NewGameDialog() override;
    public DialogResult ShowDialog();//TODO virtual at dialogwindow

    protected void OnKeyUp(const EventArgs &) override;

    // private void HandleBeginClick(EventArgs *);
    // private void HandleBackClick(EventArgs *);
    private void ToggleAvailScen(EventArgs *);
    private void ToggleRndScen(EventArgs *);
};

NAMESPACE_H3R

#endif