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

#include "h3r_sdlwindow.h"
#include <SDL2/SDL_mixer.h>

struct SDL_Release final
{
    Mix_Music * _music;
    SDL_Release(Mix_Music * m) : _music{m} {}
    ~SDL_Release() { if (_music) Mix_FreeMusic (_music); SDL_Quit (); }
};

// Something like this shall be done at the plug-in code.
static bool global_sdl_init {};
static bool global_sdl_mix_init {};
static void Init_SDL()
{
    if (SDL_Init (SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        H3R_NS::Log::Err (H3R_NS::String::Format (
            "SDL_Init error: %s. Stop." EOL, SDL_GetError ()));
        H3R_NS::OS::Exit (1);
    }
    global_sdl_init = true;

    //TODO audio init; requires OS VFS
    int audio_result = 0;
    int audio_flags = MIX_INIT_MP3;
    if (audio_flags != (audio_result = Mix_Init (audio_flags))) {
        H3R_NS::Log::Err (H3R_NS::String::Format (
            "Mix_Init error: %s. The rest is silence." EOL, Mix_GetError ()));
        return;
    }
    if (-1 == Mix_OpenAudio (44100, AUDIO_S16SYS, 2, 1024)) {
        H3R_NS::Log::Info (H3R_NS::String::Format (
            "Mix_OpenAudio error: %s. The rest is silence." EOL,
            Mix_GetError ()));
        return;
    }
    Mix_Music * music = Mix_LoadMUS ("MP3/MAINMENU.MP3");
    if (music) Mix_PlayMusic (music, -1);
    global_sdl_mix_init = true;

    static SDL_Release ____ {music};
}// Init_SDL

H3R_NAMESPACE

SDLWindow::SDLWindow(int, char **)
    : OSWindow (0, nullptr),
    _main_window_background{Game::GetResource ("GamSelBk.pcx")}
{
    if (! global_sdl_init) Init_SDL ();
}

SDLWindow::~SDLWindow()
{
    if (_gHelloWorld) SDL_FreeSurface (_gHelloWorld);
    if (_window) SDL_DestroyWindow (_window);
}

void SDLWindow::Render()
{
    // SDL_FillRect (screenSurface, nullptr,
    //    SDL_MapRGB (screenSurface->format, 0xaa, 0xaa, 0xaa));
    // screenSurface = SDL_GetWindowSurface (window);
    SDL_BlitSurface (_gHelloWorld, nullptr, _screenSurface, nullptr);
    SDL_UpdateWindowSurface (_window);
    SDL_Delay (16);//TODO timing
}

void SDLWindow::ProcessMessages()
{
    // SDL_PumpEvents ();
    if (_q) return;
    while (SDL_PollEvent (&_e) != 0) {
        if (_q) continue;
        if (SDL_QUIT == _e.type) {
            OnClose (_q);
            if (_q) Close ();
        }
        if (_q) continue; //TODO should I?

        if (SDL_WINDOWEVENT == _e.type) {
            H3R_NS::Log::Info ("SDL_WINDOWEVENT" EOL);
            if (SDL_WINDOWEVENT_RESIZED == _e.window.event)
                if (_e.window.data1 > 0 && _e.window.data2 > 0)
                    _screenSurface = SDL_GetWindowSurface (_window);
            if (SDL_WINDOWEVENT_EXPOSED == _e.window.event)
                Render ();
        }
        if (SDL_KEYUP == _e.type && SDL_SCANCODE_Q == _e.key.keysym.scancode) {
            _q = true;
            OnClose (_q);
            if (_q) Close ();
        }
    }
}// ProcessMessages

NAMESPACE_H3R