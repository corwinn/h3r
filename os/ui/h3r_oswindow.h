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

#ifndef _H3R_OSWINDOW_H_
#define _H3R_OSWINDOW_H_

#include "h3r.h"
#include "h3r_iwindow.h"

H3R_NAMESPACE

// Bridge: Implementation
#undef public
class OSWindow : public IWindow
#define public public:
{
    public virtual void Show () override {}
    public virtual void Hide () override {}
    public virtual void Close () override {}
    public virtual void ProcessMessages() override {}
    public virtual bool Idle() override { return true; }
    public virtual void Render() override {}
    public virtual void SetMouseCursor(IWindow::MousePtrInfo &) override {}

    public OSWindow(int, char **) {}
    public virtual ~OSWindow() {}

    // handle event forwarding
    public struct NoWindow final : IWindow // avoid "if"
    {
        inline void OnKeyDown(const EventArgs &) override {}
        inline void OnKeyUp(const EventArgs &) override {}
        inline void OnMouseMove(const EventArgs &) override {}
        inline void OnMouseDown(const EventArgs &) override {}
        inline void OnMouseUp(const EventArgs &) override {}
        inline void OnShow() override {}
        inline void OnHide() override {}
        inline void OnClose(IWindow *, bool &) override {}
        inline void OnRender() override {}
        inline void OnResize(int, int) override {}
    };
    private IWindow * _eh {};
    public inline virtual void SetEventHandler(IWindow * w)
    {
        static NoWindow n;
        _eh = nullptr == w ? &n : w;
    }

    // Why are these virtual? Because you might want to create a monitoring
    // window where you get notified with these, and forward them afterwards;
    // or a "passive" window that handles no events.
    // If this proves to be a wrong design, a multicast delegate shall be used
    // instead of these. Until then:
    // Ensure these get called, should you choose to override them, or the
    // observer shall not get notified.
    //LATER codegen: base = h3r_iwindow.h; targets: h3r_window.h, h3r_oswindow.h
    protected inline virtual void OnKeyDown(const EventArgs & e) override
    {
        _eh->OnKeyDown (e);
    }
    protected inline virtual void OnKeyUp(const EventArgs & e) override
    {
        _eh->OnKeyUp (e);
    }
    protected inline virtual void OnMouseMove(const EventArgs & e) override
    {
        _eh->OnMouseMove (e);
    }
    protected inline virtual void OnMouseDown(const EventArgs & e) override
    {
        _eh->OnMouseDown (e);
    }
    protected inline virtual void OnMouseUp(const EventArgs & e) override
    {
        _eh->OnMouseUp (e);
    }
    protected inline virtual void OnShow() override
    {
        _eh->OnShow ();
    }
    protected inline virtual void OnHide() override
    {
        _eh->OnHide ();
    }
    protected inline virtual void OnClose(IWindow * s, bool & cancel) override
    {
        _eh->OnClose (s, cancel);
    }
    protected inline virtual void OnRender() override
    {
        _eh->OnRender ();
    }
    protected inline virtual void OnResize(int w, int h) override
    {
        _eh->OnResize (w, h);
    }
};// OSWindow

NAMESPACE_H3R

#endif