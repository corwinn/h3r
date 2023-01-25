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
#include <SDL2/SDL_mixer.h>
#include "h3r_game.h"
#include "h3r_resdecoder.h"

H3R_NAMESPACE

// SDL is a multi-OS bridge itself, so there is no need for OS-specific classes
// yet.
// Its this deep in the directory tree on purpose.
//LATER plug-in; an SDL window shouldn't concern itself with h3r at all.
class SDLWindow : public OSWindow
{
    SDL_Event e;
    SDL_Window * _window {};
    SDL_Surface * _screenSurface {};
    SDL_Surface * _gHelloWorld {};
    private: Pcx _main_window_background;
    bool q {false};

    protected: inline virtual void Open () override
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

        while (! q) {
            while (SDL_PollEvent (&e) != 0) {
                if (! q) q = SDL_QUIT == e.type;
                if (SDL_WINDOWEVENT == e.type) {
                    H3R_NS::Log::Info ("SDL_WINDOWEVENT" EOL);
                    if (SDL_WINDOWEVENT_RESIZED == e.window.event)
                        if (e.window.data1 > 0 && e.window.data2 > 0)
                            _screenSurface = SDL_GetWindowSurface (_window);
                }
                if (SDL_KEYUP == e.type &&
                    SDL_SCANCODE_Q == e.key.keysym.scancode)
                    q = true;
            }
            // SDL_FillRect (screenSurface, nullptr,
            //    SDL_MapRGB (screenSurface->format, 0xaa, 0xaa, 0xaa));
            // screenSurface = SDL_GetWindowSurface (window);
            SDL_BlitSurface (_gHelloWorld, nullptr, _screenSurface, nullptr);
            SDL_UpdateWindowSurface (_window);
            SDL_Delay (16);
        }
    }// Open

    public: void ProcessMessages() override { SDL_PumpEvents (); }

    public: SDLWindow()
        : _main_window_background{Game::GetResource ("GamSelBk.pcx")}
    {
    }

    public: ~SDLWindow() override
    {
        if (_gHelloWorld) SDL_FreeSurface (_gHelloWorld);
        if (_window) SDL_DestroyWindow (_window);
    }
};

NAMESPACE_H3R

#endif