//
//               "The Golden Crane flies for Tarmon Gai'Don."
//


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

#include "h3r_os_error.h"
H3R_ERR_DEFINE_UNHANDLED
H3R_ERR_DEFINE_HANDLER(Memory,H3R_ERR_HANDLER_UNHANDLED)
H3R_ERR_DEFINE_HANDLER(File,H3R_ERR_HANDLER_UNHANDLED)
H3R_ERR_DEFINE_HANDLER(Log,H3R_ERR_HANDLER_UNHANDLED)

#include "h3r_game.h"
#include "h3r_filestream.h"

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
n - up
s - down
d - deactivated
h - hover
   "new game"    : Data_H3sprite_lod/MMENUNG.def
   "load game"   : Data_H3sprite_lod/MMENULG.def ditto
   "high score"  : Data_H3sprite_lod/MMENUHS.def ditto
   "credits"     : Data_H3sprite_lod/MMENUCR.def ditto
   "quit"        : Data_H3sprite_lod/MMENUQT.def ditto
main menu music  : MP3/MAINMENU.MP3
button click     : Data_Heroes3_snd/BUTTON.wav
*/

#include "h3r_window.h"
#include "h3r_oswindow.h"

//LATER How hard is to replace this with a plug-in?
#ifdef SDL2_GL
#include <SDL.h>
#include <SDL2/SDL_mixer.h>
#include "h3r_sdlwindow.h"
#endif

#include <new>

int main(int argc, char ** argv)
{
    H3R_ENSURE(argc > 0, "Can't handle your OS - argc can't be < 1")
    H3R_NS::OS::Log_stdout ("Process: %s" EOL, argv[0]);
    char * p = argv[0]; // the future base dir or work dir
    int len = H3R_NS::OS::Strlen (argv[0]);
    for (int i = len-1; i >= 0; i--)
        if ('/' == p[i] || '\\' == p[i] ) {
            p[i] = '\0';
            break;
        }
    H3R_NS::OS::Log_stdout ("WorkDir: %s" EOL, p);

    H3R_NS::Game game {p};

#ifdef SDL2_GL
    if (SDL_Init (SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
        return H3R_NS::Log::Info (H3R_NS::String::Format (
            "SDL_Init error: %s" EOL, SDL_GetError ())), 101;

    //TODO audio init; requres OSFS
    int audio_result = 0;
    int audio_flags = MIX_INIT_MP3;
    if (audio_flags != (audio_result = Mix_Init (audio_flags)))
        return H3R_NS::Log::Info (H3R_NS::String::Format (
            "Mix_Init error: %s" EOL, Mix_GetError ())), 9;
    if (-1 == Mix_OpenAudio (44100, AUDIO_S16SYS, 2, 1024))
        return H3R_NS::Log::Info (H3R_NS::String::Format (
            "Mix_OpenAudio error: %s" EOL, Mix_GetError ())), 9;
    Mix_Music * music = Mix_LoadMUS ("MP3/MAINMENU.MP3");
    Mix_PlayMusic (music, -1);

    H3R_NS::SDLWindow * sdl_gl;
    H3R_CREATE_OBJECT(sdl_gl, H3R_NS::SDLWindow) {};
#endif
    H3R_NS::Window * main_window;
    H3R_CREATE_OBJECT(main_window, H3R_NS::Window)
#ifdef SDL2_GL
    {sdl_gl}
#endif
    ;

    H3R_NS::Game::MainWindow = main_window;
    main_window->Show ();

#ifdef SDL2_GL
    Mix_FreeMusic (music); // sounds nice :)
    SDL_Quit ();
#endif
    return 0;
}
