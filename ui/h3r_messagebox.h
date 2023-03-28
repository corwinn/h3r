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

#ifndef _H3R_MESSAGEBOX_H_
#define _H3R_MESSAGEBOX_H_

#include "h3r.h"
#include "h3r_renderengine.h"
#include "h3r_dialogwindow.h"
#include "h3r_string.h"
#include "h3r_list.h"
#include "h3r_event.h"

H3R_NAMESPACE

// Quit message box:
//
//  - background: tiling, but a mystery:
//    DIBOX128.PCX is 128x128 // I'll have to checkout on a screenshot basis
//    DiBoxBck.pcx is 256x256 // when to use what
//
//  - button cancel: iCANCEL.def (30 pixels height) || ICN6432.DEF (32 pixels)?
//    "iCANCELn.pcx"
//    "iCANCELs.pcx"
//    "iCANCELd.pcx"
//    "iCANCELh.pcx" - ain't using it?!
//    It is using the 30 pixels variant. Also, some box is drawn around the
//    button: a 66x32 pixels. There is such a box: "box66x32.pcx" - but its size
//    is 68x34; the one: "Box64x30.pcx". Why using a bitmap at all - instead of
//    4 lines?!
//
//  - button OK: "IOK6432.DEF", "iOKAY.def" (quit dialog), "IOKAY32.DEF"
//    Guess.
//
//  - the min. size is defined by its decorations at "dialgbox.def": 128x128 -
//    in order to put the 4 corner decors (64x64 each).
//
//TODO all these names must go to some central place.
//
#undef public
class MessageBox : public DialogWindow, public IHandleEvents
#define public public:
{
    private int _t {}, _l {};

    public enum class Buttons {OKCancel};

    public MessageBox(Window * base_window, Point && size,
        const String & msg, const String & fnt, MessageBox::Buttons btn);
    public ~MessageBox() override;

    public static DialogResult Show(
        const String & msg, const String & fnt, MessageBox::Buttons btn);

    protected void OnKeyUp(const EventArgs &) override;

    private void HandleOKClick(EventArgs *);
    private void HandleCancelClick(EventArgs *);
};

NAMESPACE_H3R

#endif