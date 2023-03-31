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

// Simplicity ends, with the definition of the Window class.

// Do not use specifics here and onwards, like GL.*, D3D.*, etc.
// Window shall not care about anything system-specific; it has enough other
// responsibilities as it is.

#ifndef _H3R_WINDOW_H_
#define _H3R_WINDOW_H_

#include "h3r.h"
#include "h3r_iwindow.h"
#include "h3r_oswindow.h"
#include "h3r_control.h"
#include "h3r_list.h"
// #include "h3r_gc.h"
#include "h3r_point.h"
#include "h3r_renderengine.h"

H3R_NAMESPACE

// Bridge: Abstraction. In case you're wondering: put nothing specific here.
//
// It shall allow you to create a text console version of the game if you dare:
// just implement OSWindow for it.
// Use-case:
//  * main window
//  * dialog window
//
// It shall process messages while hidden.
// It takes care of ~OSWindow().
#undef public
class Window : public IWindow
#define public public:
{
#define IW

    //TODO static
    private OSWindow * _win; // Shared. Until proved otherwise.
    // Bridge on demand
    protected inline OSWindow * GetOSWindow() { return _win; }

    // These simple booleans ... are anything but simple. They are 2 now.
    private bool _visible {false};
    private bool _closed {false};
    public bool Closed() const { return _closed; }

    // It looks like a list, but it isn't. Its a tree. Each Control has the
    // same list.
    private List<Control *> _controls {};
    private List<Control *> _shown {}; // used by OnShow / OnHide
    //LATER IContainer<T>
    public void AddControl(Control *);
    protected inline List<Control *> & Controls() { return _controls; }
    // private static GC _gc; // One GC should be enough.

    // Automatic depth. You define whats rendered over what, by placing:
    //  - Window over a Window
    //  - Control over a Window
    //  - Control over a Control
    private h3rDepthOrder _wdepth; // the window depth
    private h3rDepthOrder _topmost; // the topmost control depth
    // Used by Window::Window(Window * base_window, ...
    protected h3rDepthOrder NextDepth(); // the next available depth
    // Interface for Control::Control(Control *); Avoid walking a tree just
    // to find how deep it is.
    public inline h3rDepthOrder Depth() const { return _wdepth; }
    public void UpdateTopMost(Control *); // Callback for leaf Controls
    // Use this function to get next available depth.
    public static h3rDepthOrder NextDepth(h3rDepthOrder); // Sentinel

    IW public void ProcessMessages() override final;
    IW public inline virtual bool Idle() override { return _win->Idle (); }
    IW protected inline virtual void Render() override { _win->Render (); }

    // Provide Size for all Window. This is not bridged!
    private Point _size;
    public inline const Point & GetSize() const { return _size; }
    protected inline void SetSize(Point && p) { _size = p; }

    // Constructor for non-bridged "Window"s. They share the same OSWindow.
    public Window(Window * base_window, Point && size);
    // The one that ProcessMessages. Switched by ui_main().
    public static Window * ActiveWindow;
    public static Window * MainWindow; // Used by MessageBox::Show()
    public static RenderEngine * UI; // Managed by Window

    // Use IWindow::Create().
    // This shall be the MainWindow only! It has _wdepth of 0.
    public Window(OSWindow * actual_window, Point && size);

    //  Window  OSWindow  OSWindow
    //  | Show  |         |
    //  |------>|         |
    //  |       | Show    |
    //          |-------->|
    //                    | OnShow
    //  |OnShow           |
    //  |<................|
    IW public void Show() override;
    IW public void Hide() override;
    IW public void Close() override;

    public virtual ~Window() override;

    IW protected virtual void OnKeyDown(const EventArgs &) override;
    IW protected virtual void OnKeyUp(const EventArgs &) override;
    IW protected virtual void OnMouseMove(const EventArgs &) override;
    IW protected virtual void OnMouseDown(const EventArgs &) override;
    IW protected virtual void OnMouseUp(const EventArgs &) override;
    IW protected virtual void OnShow() override;
    IW protected virtual void OnHide() override;
    IW protected virtual void OnClose(IWindow *, bool &) override;
    IW protected virtual void OnRender() override;
    // Forget to call this one, and you shall see nothing or a mess.
    IW protected virtual void OnResize(int, int) override;
    IW protected virtual void SetMouseCursor(IWindow::MousePtrInfo &) override;
#undef IW

    //TODO map Window::MousePtr to the game resources
    // resource names:          CursrD00, CursrD01, CursrD02,
    public enum class MousePtr {Default , Default2, Wait     };
};// Window

NAMESPACE_H3R

#endif