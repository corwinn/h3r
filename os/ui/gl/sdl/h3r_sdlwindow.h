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
#include <SDL.h>
#include <SDL_opengl.h>
#include "h3r_game.h"
#include "h3r_resdecoder.h"

H3R_NAMESPACE

// SDL is a multi-OS bridge itself, so there is no need for OS-specific classes
// yet.
// Its this deep in the directory tree on purpose.
//LATER plug-in; an SDL window shouldn't concern itself with h3r at all.
class SDLWindow : public OSWindow
{
#define public public:
#define private private:
#define protected protected:

    private SDL_Event _e;
    private SDL_Window * _window {};
    private SDL_GLContext _gContext {};
    private SDL_Surface * _screenSurface {};
    private SDL_Surface * _gHelloWorld {};
    private Pcx _main_window_background;
    private bool _q {false};
    private bool _visible {false};

    protected inline virtual void Hide() override {}

    protected inline virtual void Show() override
    {
        _window = SDL_CreateWindow ("h3r",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600,
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        if (! _window) {
             H3R_NS::Log::Info (H3R_NS::String::Format (
                "SDL_CreateWindow error: %s" EOL, SDL_GetError ()));
             return;
        }

        _screenSurface = SDL_GetWindowSurface (_window);

        var byte_arr_ptr = _main_window_background.RGB ();
        H3R_NS::Log::Info (H3R_NS::String::Format (
                "SDL_CreateRGBSurfaceFrom (w: %d, h: %d)" EOL,
                _main_window_background.Width (),
                _main_window_background.Height ()));
        _gHelloWorld = SDL_CreateRGBSurfaceFrom (
            byte_arr_ptr->operator byte * (),
            _main_window_background.Width (),
            _main_window_background.Height (),
            24, 3 * _main_window_background.Width (), 255, 255, 255, 0);
        if (! _gHelloWorld) {
            H3R_NS::Log::Info (H3R_NS::String::Format (
                "SDL_CreateRGBSurfaceFrom error: %s" EOL, SDL_GetError ()));
            return;
        }
        Render ();//TODO SDL_WINDOWEVENT_EXPOSED gets lost sometimes?
    }// Show

    protected inline virtual void Close() override {}

    private void Render();

    public void ProcessMessages() override;

    public SDLWindow(int, char **);

    public ~SDLWindow();
};

#undef public
#undef private
#undef protected

NAMESPACE_H3R

#endif