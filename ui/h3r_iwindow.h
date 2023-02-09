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

#ifndef _H3R_IWINDOW_H_
#define _H3R_IWINDOW_H_

#include "h3r.h"
#include "h3r_os.h"
#include "h3r_eventargs.h"
#include "h3r_point.h"

H3R_NAMESPACE

// Abstract window.
// A contract between the windowing bridge: abstraction and implementation:
// Helps avoiding syntax errors and missing overrides.
// Also, the "Game" communicates with the UI through this thing only.
struct IWindow
{
    // Show a hidden window.
    virtual void Show() {H3R_NOT_IMPLEMENTED_EXC}

    // Hide a shown window.
    virtual void Hide() {H3R_NOT_IMPLEMENTED_EXC}

    // Once closed, the window gets released.
    virtual void Close() {H3R_NOT_IMPLEMENTED_EXC}

    // The thing to periodically do while waiting for an async task to complete.
    virtual void ProcessMessages() {H3R_NOT_IMPLEMENTED_EXC}

    // Return true to indicate there are no messages to process.
    virtual bool Idle() {H3R_NOT_IMPLEMENTED_EXC}

    // A.k.a. Update(); a.k.a. Refresh(); etc.
    virtual void Render() {H3R_NOT_IMPLEMENTED_EXC}

    virtual ~IWindow() {}

    // Events
    virtual void OnKeyDown(const EventArgs &) {H3R_NOT_IMPLEMENTED_EXC}
    virtual void OnKeyUp(const EventArgs &) {H3R_NOT_IMPLEMENTED_EXC}
    virtual void OnMouseMove(const EventArgs &) {H3R_NOT_IMPLEMENTED_EXC}
    virtual void OnMouseDown(const EventArgs &) {H3R_NOT_IMPLEMENTED_EXC}
    virtual void OnMouseUp(const EventArgs &) {H3R_NOT_IMPLEMENTED_EXC}
    virtual void OnShow() {H3R_NOT_IMPLEMENTED_EXC}
    virtual void OnHide() {H3R_NOT_IMPLEMENTED_EXC}
    // "bool &" - allow the user to cancel the closing.
    virtual void OnClose(bool &) {H3R_NOT_IMPLEMENTED_EXC}
    virtual void OnRender() {H3R_NOT_IMPLEMENTED_EXC}
    virtual void OnResize(int, int) {H3R_NOT_IMPLEMENTED_EXC}

    // Don't forget to '#include < new >'.
    // Convenience method.
    // Implicit contract with a matching OSWindow::OSWindow.
    template <typename Abstraction, typename Implementation> static Abstraction *
    Create(int argc, char ** argv, Point && size)
    {
        Implementation * a; // The "Window" takes care
        H3R_CREATE_OBJECT(a, Implementation) {
            argc, argv, static_cast<Point &&>(size)};
        Abstraction * b;
        H3R_CREATE_OBJECT(b, Abstraction) {a, static_cast<Point &&>(size)};
        a->SetEventHandler (b);
        return b;
    }
};// IWindow

// Handles the UI portion of the program. Implemented by the chosen "window"
// implementation.
int ui_main(int argc, char ** argv);

NAMESPACE_H3R

#endif