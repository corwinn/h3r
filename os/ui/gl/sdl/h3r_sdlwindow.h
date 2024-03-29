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

#ifndef _H3R_SDLWINDOW_H_
#define _H3R_SDLWINDOW_H_

#include "h3r.h"
#include "h3r_log.h"
#include "h3r_oswindow.h"

#undef public
#undef private
#undef protected
#  include <SDL.h>
#  include <SDL_opengl.h>
#  include <SDL2/SDL_mixer.h>
#define public public:
#define private private:
#define protected protected:

H3R_NAMESPACE

// SDL is a multi-OS bridge itself, so there is no need for OS-specific classes
// yet.
// Its this deep in the directory tree on purpose.
//LATER plug-in; an SDL window shouldn't concern itself with h3r at all.
#undef public
class SDLWindow : public OSWindow
#define public public:
{
    private SDL_Event _e;
    private SDL_Window * _window {};
    private SDL_GLContext _gc {};
    private Mix_Music * _music {};
    private int _w {800}, _h{600};
    private bool _q {false};
    private bool _visible {false};
    private bool _initialized {false};
    protected virtual void Show() override;
    protected inline virtual void Hide() override { _visible = false; }
    protected inline virtual void Close() override { _q = true; }
    protected void Render() override;
    protected bool Idle() override;
    private void Resized();
    private void HandleWindowEvent();
    private void HandleKeyboardEvent(EventArgs &);
    private void HandleKeyboardEventDown(EventArgs &);
    private void HandleMouseMotionEvent(EventArgs &);
    private void HandleMouseButtonEvent(EventArgs &);
    public void ProcessMessages() override;
    public SDLWindow(int, char **, Point &&);
    public ~SDLWindow();

    private SDL_Cursor * _mouse_cursor {};
    private void ClearMouseCursor();
    private void SetMouseCursor(IWindow::MousePtrInfo &) override;
};// SDLWindow

NAMESPACE_H3R

#endif