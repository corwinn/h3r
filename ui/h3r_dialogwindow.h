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
//      - solid black with ~x% alpha: 7 pixels
//      - solid black border with ~x% alpha: 1 pixel
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
// After a few screen-shots: the game seems to be using an external palette - a
// per-player color one - for the UI, with red being the 0 (default, 1st player
// color); how exactly it does that color mixing is a TODO.
//
// The quit dialog box is 320 x 192 w/o the drop shadow; located at 240,204 -
// that's centered w/o the drop-shadow.
// The tiling can be done by Open GL itself but only if using Open GL directly
// w/o the RenderEngine, unless I parameterize it to support tiling.
//
//LATER The drop-shadow shall be done by a shader; unless I find a game res.
// that represents it, but I doubt it; its probably GDI ROP they're doing.
// For now it will be a procedurally-generated RGBA.
//
// These Window could use the big UI VBO, because of its pass-es, but I'll have
// to think about storing their keys for re-use. The other option is to enable
// inserting RenderEngine as a "pass" at the UI one: prior the text pass.
//
// The bad news: it has a dialog over a dialog (a map displaying a message;
// Alt+F4 displays the quit dialog over the 1st one), where text is partially
// visible; means the z-ordering becomes mandatory: the RenderEngine - not that
// simple.
#undef public
class DialogWindow : public Window
#define public public:
{
    private Window * _w;
    private Point _size;
    // These are not resizeable.
    public DialogWindow(Window * base_window, Point && size)
        : Window {base_window, static_cast<Point &&>(size)},
            _w {base_window}, _size{size}
    {
        Window::GetOSWindow ()->SetEventHandler (this);
        Window::UI->CheckPoint ();
    }
    public virtual ~DialogWindow() override
    {
        Window::UI->Rollback ();
    }
    protected inline virtual void OnClose(IWindow * s, bool & allow) override
    {
        Window::GetOSWindow ()->SetEventHandler (_w);
        Window::OnClose (s, allow);
    }
    // No need to repeat this everywhere.
    //LATER Property<DialogResult>
    protected bool _has_dr {};
    protected DialogResult _dr {};
    public inline virtual DialogResult ShowDialog()
    {
        _dr = DialogResult::Cancel;
        while (! _has_dr && ! Closed ())
            Window::ActiveWindow->ProcessMessages ();
        bool allow_close = true;
        OnClose (this, allow_close);
        return _dr;
    }
};// DialogWindow

NAMESPACE_H3R

#endif