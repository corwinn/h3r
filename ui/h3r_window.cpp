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

/*main menu
   background    : Data_H3bitmap_lod/GamSelBk.pcx
   right overlay : Data_H3bitmap_lod/MainMenu.pcx
                   Looks the same as the rightmost portion of GamSelBk. Redraw
                   reasons? Layout?
   "new game"    : Data_H3sprite_lod/MMENUNG.def 1 blk 4 sprites; 2 bonus
                   Type: 71
                   Block[0]
                    name[0]: "mmenungn.pcx" up
                    name[1]: "mmenungs.pcx" down
                    name[2]: "mmenungh.pcx" highlighted
                    name[3]: "mmenungh.pcx"
                  The 2 bonus ones do look like replicating 0:0 and 0:1.
                  0:2 and 0:3 do look the same, and do have equivalent names.
SOMAIN.DEF has type 71 as well
Block [0]
  name[0]: "SOMainS.pcx" down
  name[1]: "SOMainH.pcx" up
The game has no reference to "mmenungh" - how does it know what to use on mouse
over? No reference to "SOMainS" and "mmenungn" - SOMAIN.DEF and MMENUNG.def have
"up" and "down" reversed - index-wise - how does it "know" whats what?
it counts on the last char prior the . it seems:
<> mainwindow <>
main menu music  : MP3/MAINMENU.MP3
button click     : Data_Heroes3_snd/BUTTON.wav
*/

#include "h3r_window.h"
#include "h3r_list.h"
#include "h3r_thread.h"
#include "h3r_timing.h"

H3R_NAMESPACE

//TODO DLL<Window *> as Window::static && Node<Window *> * at each Window:
//     node = _node->Remove ();
//     H3R_DESTROY_OBJECT(node->Data, Window)
//     H3R_DESTROY_OBJECT(node, DLL<Window *>)
// ui_main:
//   while (! Window::List->Empty ()) Window::List->Data->ProcessMessages ();
static List<Window *> global_win_list {};

int ui_main(int, char **)
{
    for (int i = 0; ; i %= global_win_list.Count ()) {
        auto w = global_win_list[i];
        if (w->Closed ()) {
            H3R_DESTROY_OBJECT(w, Window)
            if (global_win_list.Empty ()) break;
        }
        else {
            if (w->Idle ()) OS::Thread::SleepForAWhile ();
            w->ProcessMessages ();
            i++;
        }
    }
    return 0;
}

GC Window::_gc {};

// Code repeats, but I don't intend to re-create ICollection<T>.
void Window::Add(Control * c)
{
    H3R_ARG_EXC_IF(_controls.Contains (c), "duplicate pointer: fix your code")
    H3R_ARG_EXC_IF(nullptr == c, "c can't be null")
    _controls.Add (c);
}

Window::Window(OSWindow * actual_window)
    : _win{actual_window}
{
    global_win_list.Add (this);
}

Window::~Window()
{
    H3R_DESTROY_OBJECT(_win, OSWindow)
    for (Control * c : _controls)
        H3R_DESTROY_OBJECT(c, Control)
    global_win_list.Remove (this);
}

// Bridge
void Window::Show()
{
    _win->Show ();
    _visible = true;
}
void Window::Hide()
{
    _visible = false;
    _win->Hide ();
}
void Window::Close()
{
    _visible = false;
    _win->Close ();
    _closed = true;
}

// WndProc. Do not block it.
void Window::ProcessMessages()
{
    _win->ProcessMessages ();

    // A good a place as any. For timing that is. It can easily be moved when/if
    // OnIdle is implemented.
    // GALLIUM_HUD="VRAM-usage,GPU-load,cpu,fps" . _invoke
    static OS::TimeSpec frame_a, frame_b;
    OS::GetCurrentTime (frame_a);
    Render ();//TODO OnIdle
    OS::GetCurrentTime (frame_b);
    auto frame_time = OS::TimeSpecDiff (frame_a, frame_b); // [nsec]
    if (frame_time > 0) {
        auto ftus = frame_time / 1000.0; // [usec]
        auto const TARGET_FPS {32};//TODO to Game - link with Options
        GLdouble adj = 1000000.0 / TARGET_FPS - ftus; // [usec]
        //TODO high-resolution OS::Thread::Sleep
        if (adj > 1000.0 && adj < 1000000.0)
            OS::Thread::Sleep (static_cast<int>(adj/1000));
    }
}

// Observer
void Window::OnKeyDown(const EventArgs & e)
{
    for (Control * c : _controls) c->OnKeyDown (e);
}
void Window::OnKeyUp(const EventArgs & e)
{
    for (Control * c : _controls) c->OnKeyUp (e);
}
void Window::OnMouseMove(const EventArgs & e)
{
    for (Control * c : _controls) c->OnMouseMove (e);
}
void Window::OnMouseDown(const EventArgs & e)
{
    for (Control * c : _controls) c->OnMouseDown (e);
}
void Window::OnMouseUp(const EventArgs &e)
{
    for (Control * c : _controls) c->OnMouseUp (e);
}
void Window::OnShow() { _visible = true; }  //TODO forward these?
void Window::OnHide() { _visible = false; } //
// Close by default; the base window has no idea how to ask.
void Window::OnClose(bool &) { _closed = true; }
void Window::OnRender()
{
    if (_visible) //LATER this shall cause the continues-from-where-it-stopped
                  //      effect; is that ok?
        for (Control * c : _controls) c->OnRender (_gc);
}
// No anchoring, or auto-sizing, so no point forwarding it.
void Window::OnResize(int, int) {}

NAMESPACE_H3R
