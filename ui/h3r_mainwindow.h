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

#ifndef _H3R_MAINWINDOW_H_
#define _H3R_MAINWINDOW_H_

#include "h3r.h"
#include "h3r_window.h"
#include "h3r_label.h"
#include "h3r_event.h"

H3R_NAMESPACE

// MainWindow
//TODO mouse pointers: adventure map: CRADVNTR.def; combat: CRCOMBAT.def;
//     defaults?: crdeflt.def
#undef public
class MainWindow final : public Window, public IHandleEvents
#define public public:
{
    private List<Label *> _time_labels {};

    public MainWindow(OSWindow *, Point &&);
    public ~MainWindow() override;

    private void OnKeyUp(const EventArgs &) override;
    private void OnShow() override;
    private void OnRender() override;
    private void OnResize(int w, int h) override;

    private void Quit(EventArgs *);
    private void NewGame(EventArgs *);
    private void NewGameBack(EventArgs *);
    private void NewGameSignleScenario(EventArgs *);

    private int _mm_a, _mm_b; // main menu buttons [a;b] (at Controls ())
    private int _ng_a, _ng_b; // new game buttons [a;b] (at Controls ())

    //TODO load and map mouse cursor bitmaps:
    //     - CRADVNTR.def (adv map), CRCOMBAT.def (battle),
    //       crdeflt.def (mouse ptr, wait ptr)
    //     - where to define the mapping?
};

NAMESPACE_H3R

#endif