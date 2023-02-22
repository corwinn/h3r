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
#include "h3r_renderengine.h"

#include <new>

H3R_NAMESPACE

//TODO DLL<Window *> as Window::static && Node<Window *> * at each Window:
//     node = _node->Remove ();
//     H3R_DESTROY_OBJECT(node->Data, Window)
//     H3R_DESTROY_OBJECT(node, DLL<Window *>)
// ui_main:
//   while (! Window::List->Empty ()) Window::List->Data->ProcessMessages ();
static List<Window *> global_win_list {};//TODO de-static-ify me

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
    global_win_list.Clear ();
    return 0;
}

// GC Window::_gc {};
Window * Window::ActiveWindow {};
Window * Window::MainWindow {};
RenderEngine * Window::UI {};

// Code repeats, but I don't intend to re-create ICollection<T>.
void Window::AddControl(Control * c)
{
    H3R_ARG_EXC_IF(_controls.Contains (c), "duplicate pointer: fix your code")
    H3R_ARG_EXC_IF(nullptr == c, "c can't be null")
    _controls.Add (c);
    c->_depth = NextDepth (_wdepth); // redundant, but still
    UpdateTopMost (c);
}

h3rDepthOrder Window::NextDepth() { return Window::NextDepth (_topmost); }

void Window::UpdateTopMost(Control * c)
{
    if (c->Depth () > _topmost) _topmost = c->Depth ();
}

/*static*/ h3rDepthOrder Window::NextDepth(h3rDepthOrder depth)
{
    H3R_ENSURE(depth < H3R_LAST_DEPTH, "Depth overflow")
    return depth + 1;
}

Window::Window(Window * base_window, Point && size)
    : _win{base_window->_win}, _wdepth{base_window->NextDepth ()}, _size{size}
{
    H3R_ENSURE(nullptr != Window::MainWindow, "No MainWindow ?!")
    global_win_list.Add (this);
}

Window::Window(OSWindow * actual_window, Point && size)
    : _win{actual_window}, _wdepth{0}, _size{size}
{
    static bool once {};
    H3R_ENSURE(! once, "One MainWindow please")
    once = true;
    if (nullptr == Window::UI) { // just in case
        RenderEngine::Init ();
        H3R_CREATE_OBJECT(Window::UI, RenderEngine) {
            H3R_DEFAULT_UI_MAX_SPRITES};
    }
    global_win_list.Add (this);
}

Window::~Window()
{
    // It provides events, so stop it first.
    if (global_win_list.Count () <= 1)
        H3R_DESTROY_OBJECT(_win, OSWindow)
    for (Control * c : _controls)
        H3R_DESTROY_OBJECT(c, Control)
    global_win_list.Remove (this);
    if (global_win_list.Count () <= 0)
        H3R_DESTROY_OBJECT(Window::UI, RenderEngine)
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
// GALLIUM_HUD="VRAM-usage,GPU-load,cpu,fps" . _invoke
//
// Async. task status check granularity min.: TIGHT_LOOP_THRESHOLD [nsec]
// Removing it completely will cause 100% CPU load (tight loop) - which will
// actually slow down the async. task.
void Window::ProcessMessages()//TODO static
{
    Window::ActiveWindow = this;
    _win->ProcessMessages ();

    static OS::TimeSpec frame_a, frame_b;
    static OS::TimeSpec adj_a {}, adj_b;
    static long adjustment {};

    OS::GetCurrentTime (adj_b);
    long outside_time = OS::TimeSpecDiff (adj_a, adj_b);

    // This detection shall fail on heavy-loaded machine.
    long const TIGHT_LOOP_THRESHOLD {100000}; // [nsec]
    if (outside_time <= TIGHT_LOOP_THRESHOLD) {
        OS::Thread::NanoSleep (TIGHT_LOOP_THRESHOLD - outside_time);
        OS::GetCurrentTime (adj_b);
        outside_time = OS::TimeSpecDiff (adj_a, adj_b);
    }

    adjustment -= outside_time;

    if (adjustment <= 0) {
        OS::GetCurrentTime (frame_a);
        Render ();
        OS::GetCurrentTime (frame_b);

        auto const TARGET_FPS {32};
        long const F_ALLOWED {1000000000/TARGET_FPS}; // [nsec/frame]
        adjustment = F_ALLOWED - OS::TimeSpecDiff (frame_a, frame_b);
    }

    OS::GetCurrentTime (adj_a);
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
// Close by default; the base window has no idea how to ask, yet.
void Window::OnClose(IWindow * sender, bool &)
{
    _closed = true;
    if (sender == _win) { // CloseAll
        printf ("Handle OSMainWindow closing" EOL);
        for (auto w : global_win_list) w->_closed = true;
    }
}
void Window::OnRender() { Window::UI->Render (); }
void Window::OnResize(int w, int h) { Window::UI->Resize (w, h); }

NAMESPACE_H3R
