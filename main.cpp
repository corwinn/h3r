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

#include <SDL.h>
#include <SDL2/SDL_mixer.h>

// TODO: string, os::path
/*char const * const gDataRoot = "~/.wine/drive_c/games/h3";
char const * const gData1 = "Data";
char const * const gData2 = "Heroes3/Data";
char const * const gMusic = "MP3";
char const * const gMaps = "Maps";
char const * const gSave = "games";

Heroes3.snd
Heroes3.vid

H3ab_ahd.snd
H3ab_bmp.lod
H3bitmap.lod
Heroes3.snd
VIDEO.VID
H3ab_ahd.vid
H3ab_spr.lod
H3sprite.lod

 H3BITMAP.LOD
 H3SPRITE.LOD
H3psprit.lod
 Heroes3.snd
 Video.vid
 h3ab_ahd.snd
 h3ab_ahd.vid
 h3ab_bmp.lod
 h3ab_spr.lod
h3abp_bm.lod
h3abp_sp.lod*/

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

#include "h3r_gamearchives.h"
#include "h3r_asyncfsenum.h"

#include "h3r_asyncadapter.h"
#include "h3r_taskthread.h"
class foo
{
    H3R_NS::AsyncAdapter<foo> _adapter;
    void h1(H3R_NS::AsyncAdapter<foo>::Result r)
    {
        if (H3R_NS::AsyncAdapter<foo>::StreamOp::Size == r.Op)
            size = r.Size;
        complete = true;
    }
    void h2(H3R_NS::AsyncAdapter<foo>::Result){}
    public: foo(H3R_NS::Stream & s, H3R_NS::TaskThread & a)
//np {thread: a, observer: this, on_complete: &foo::h1, on_canceled: &foo::h2}
        : _adapter {a, this, &foo::h1, &foo::h2}
    {
        _adapter.Stream = s;
    }
    public: off_t size {};
    public: bool complete {};
    public: void Size() { _adapter.Size (); }
};

class FileEnumTest final
{
    private: H3R_NS::GameArchives GA {};
    private: H3R_NS::AsyncFsEnum<FileEnumTest> _subject;
    private: int _files {}, _dirs {};
    private: bool HandleItem(
        const H3R_NS::AsyncFsEnum<FileEnumTest>::EnumItem & itm)
    {
        if (! itm.IsDirectory)
        {
            _files++;
            if (GA.BaseArchives ().Contains (
                H3R_NS::String {itm.FileName.ToLower ()}))
            H3R_NS::OS::Log_stdout ("Found Game Archive: \"%s\"" EOL,
                itm.Name.AsZStr ().Data ());
        } else _dirs++;
        return true;
    }
    public: FileEnumTest(H3R_NS::String path)
        : GA {}, _subject{path, this, &FileEnumTest::HandleItem} {}
    public: bool Complete() const { return _subject.Complete (); }
    public: int Files() const { return _files; }
    public: int Directories() const { return _dirs; }
};

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

    H3R_NS::Game game;
    H3R_NS::Log::Info ("hello nurse" EOL);

    H3R_NS::OS::FileStream t {
        H3R_NS::String {p} + H3R_PATH_SEPARATOR + H3R_NS::String {"main"},
        H3R_NS::OS::FileStream::Mode::ReadOnly};
    H3R_NS::Log::Info (H3R_NS::String::Format (
        "File: \"%s\" opened in read-only mode. Size: %d" EOL,
        t.Name ().AsZStr ().Data (), t.Size ()));

    H3R_NS::TaskThread file_io_thread {};
    foo test_adapter {t, file_io_thread};
    test_adapter.Size ();
    while (! test_adapter.complete)
        H3R_NS::OS::Thread::Sleep (1);
    H3R_NS::Log::Info (H3R_NS::String::Format (
        "File: \"%s\" opened in read-only mode. Async Size: %d" EOL,
        t.Name ().AsZStr ().Data (), test_adapter.size));

    FileEnumTest test_file_enum {p};
    while (! test_file_enum.Complete ())
        H3R_NS::OS::Thread::Sleep (1);
    H3R_NS::OS::Log_stdout ("Scaned: %d files, and %d folders" EOL,
        test_file_enum.Files (), test_file_enum.Directories ());

    /*if (SDL_Init (SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
        return H3R_NS::Log::Info (H3R_NS::String::Format (
            "SDL_Init error: %s" EOL, SDL_GetError ())), 9;

    int audio_result = 0;
    int audio_flags = MIX_INIT_MP3;
    if (audio_flags != (audio_result = Mix_Init (audio_flags)))
        return H3R_NS::Log::Info (H3R_NS::String::Format (
            "Mix_Init error: %s" EOL, Mix_GetError ())), 9;
    if (-1 == Mix_OpenAudio (44100, AUDIO_S16SYS, 2, 1024))
        return H3R_NS::Log::Info (H3R_NS::String::Format (
            "Mix_OpenAudio error: %s" EOL, Mix_GetError ())), 9;
    Mix_Music * music = Mix_LoadMUS (
        "/mnt/workspace/rain/drive64_c/games/h3/MP3/MAINMENU.MP3");
    Mix_PlayMusic (music, -1);

    bool q {false};
    SDL_Event e;
    SDL_Window * window = nullptr;
    SDL_Surface * screenSurface = nullptr;
    SDL_Surface * gHelloWorld = nullptr;

    window = SDL_CreateWindow ("h3r",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (! window)
        return H3R_NS::Log::Info (H3R_NS::String::Format (
            "SDL_CreateWindow error: %s" EOL, SDL_GetError ())), 9;
    screenSurface = SDL_GetWindowSurface (window);
    gHelloWorld = SDL_LoadBMP (
        "../h3r_unpacked/h3/Data_H3bitmap_lod/GamSelBk.bmp");
    if (! gHelloWorld)
        return H3R_NS::Log::Info (H3R_NS::String::Format (
            "SDL_LoadBMP error: %s" EOL, SDL_GetError ())), 9;

    while( !q ) {
        while (SDL_PollEvent (&e) != 0) {
            if (! q) q = SDL_QUIT == e.type;
            if (SDL_WINDOWEVENT == e.type) {
                H3R_NS::Log::Info ("SDL_WINDOWEVENT" EOL);
                if (SDL_WINDOWEVENT_RESIZED == e.window.event)
                    if (e.window.data1 > 0 && e.window.data2 > 0)
                        screenSurface = SDL_GetWindowSurface (window);
            }
        }
        //SDL_FillRect (screenSurface, nullptr,
        //    SDL_MapRGB (screenSurface->format, 0xaa, 0xaa, 0xaa));
        //screenSurface = SDL_GetWindowSurface (window);
        SDL_BlitSurface (gHelloWorld, nullptr, screenSurface, nullptr);
        SDL_UpdateWindowSurface (window);
        SDL_Delay (16);
    }

    Mix_FreeMusic (music); // sounds nice :)
    SDL_FreeSurface (gHelloWorld);
    SDL_DestroyWindow (window);
    SDL_Quit ();*/
    return 0;
}
