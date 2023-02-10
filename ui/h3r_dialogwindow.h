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

#ifndef _H3R_DIALOGWINDOW_H_
#define _H3R_DIALOGWINDOW_H_

#include "h3r.h"
#include "h3r_window.h"

H3R_NAMESPACE

enum class DialogResult {OK, Cancel, Yes, No};

// The primary function of this window is to temporarily redirect messages to
// itself; since rendering is now handled by "Window" the "RenderEngine",
// animations won't freeze; TODO this however differs the original, so think
// about a way to freeze them while a dialog window is shown.
//
// All of these drop alpha-blended shadow (bottom and right):
//   * measurement 1 (GENRLTXT.TXT:Are you sure you want to quit?):
//      - solid width with ~75% alpha: 7 pixels
//      - solid border ~50% alpha: 1 pixel
//
// Decoration: "dialgbox.def":
//   "DiBoxTL.pcx"
//   "DiBoxTR.pcx"
//   "DiBoxBL.pcx"
//   "DiBoxBR.pcx"
//   "DiBoxL.pcx"
//   "DiBoxR.pcx"
//   "DiBoxT.pcx"
//   "DiBoxB.pcx"
// There are 3 more that have additional space (a status bar space):
//   "DiBoxRL.pcx"
//   "DiBoxRR.pcx"
//   "DiBoxRB.pcx"
//TODO all of the above are blue-ish, and the game ones are red-ish
// Some of them have their own decoration.
//
// The quit dialog box is 320 x 192 w/o the drop shadow; located at 240,204 -
// that's centered w/o the drop-shadow.
// The tiling can be done by Open GL itself; the trick is just to hint the tex.
// parameter.
//
//LATER The drop-shadow shall be done by a shader; unless I find a game res.
// that represents it, but I doubt it; its probably GDI ROP they're doing.
// For now it will be a procedurally-generated GL_LUMINANCE_ALPHA.
//
// These windows can't use the big UI VBO because it doesn't support recycling.
class DialogWindow : public Window
{
#define public public:
#define private private:
#define protected protected:

    private Window * _w;
    private Point _size;
    // These are not resizeable.
    public DialogWindow(Window * base_window, Point && size)
        : Window {base_window, static_cast<Point &&>(size)},
            _w {base_window}, _size{size}
    {
        Window::GetOSWindow ()->SetEventHandler (this);
    }
    public virtual ~DialogWindow() override {}
    protected inline virtual void OnClose(IWindow * s, bool & allow) override
    {
        Window::GetOSWindow ()->SetEventHandler (_w);
        Window::OnClose (s, allow);
    }
};

#undef public
#undef private
#undef protected

NAMESPACE_H3R

#endif