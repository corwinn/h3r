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

#include "h3r_game.h"
// H3R_MM_STATIC_INIT
#include "h3r_log.h"
H3R_LOG_STATIC_INIT

#include "h3r_string.h"
#include "h3r_lodfs.h"
#include "h3r_sndfs.h"
#include "h3r_vidfs.h"

// No plug-in interface yet, so
#include "h3r_sdlwindow.h"

#include "h3r_mainwindow.h"

H3R_NAMESPACE

TaskThread Game::IOThread {};

ResManager * Game::RM {};

IWindow * Game::MainWindow {};

h3rPlayerColor Game::CurrentPlayerColor {H3R_DEFAULT_PLAYER_COLOR};

Txt * Game::GENRLTXT {};
Txt * Game::lcdesc {};
Txt * Game::vcdesc {};

Game::Game(const char * process_path)
#if LOG_FILE
    : _3rd {"main.log"}
#endif
{
    _4th.Subscribe (&_2nd);
#if LOG_FILE
    _4th.Subscribe (&_3rd); // disable the file log for pre-releases
#endif

    H3R_CREATE_OBJECT(Game::RM, ResManager) {};

    LodFS * lod_handler {};
    H3R_CREATE_OBJECT(lod_handler, LodFS) {};
    Game::RM->Register (lod_handler);

    SndFS * snd_handler {};
    H3R_CREATE_OBJECT(snd_handler, SndFS) {};
    Game::RM->Register (snd_handler);

    VidFS * vid_handler {};
    H3R_CREATE_OBJECT(vid_handler, VidFS) {};
    Game::RM->Register (vid_handler);

    ResManagerInit res_manager_init {process_path, *Game::RM};
    while (! res_manager_init.Complete ())
        ProcessThings ();
        // OS::Thread::Sleep (1);
    OS::Log_stdout ("Scaned: %d files, and %d folders" EOL,
        res_manager_init.Files (), res_manager_init.Directories ());
}// Game::Game()

Game::~Game()
{
    H3R_DESTROY_OBJECT(Game::RM, ResManager)
}

void Game::SilentLog(bool v)
{
    _4th.Silent (v);
}

/*static*/ void Game::ProcessThings()
{
    if (Game::MainWindow) Game::MainWindow->ProcessMessages ();
    // A tight loop makes ResManagerInit above infinite ?!
    // The pointless sync at the BoolProperty at the TaskThread resolves it,
    // but I'm adding this here just in case, until I understand whats going on.
    else OS::Thread::Sleep (1);//TODO understand the odd deadlock
}

/*static*/ Stream * Game::GetResource(const String & name)
{
    // Danger! Returns a Copy?!
    //LATER RTFM
    //var task_info = Game::RM->GetResource (name);
    // This should take a nanosecond now; why is it slowing things down?
    // Because: bad timing, and missing sequence diagrams. TODO Resolve at
    // "async-ui-issue.dia".
    const auto & task_info = Game::RM->GetResource (name);
    while (! Game::RM->TaskComplete ())
        Game::ProcessThings (); // <- causing partially rendered UI!
    H3R_ENSUREF(nullptr != task_info.Resource, "Resource not found: %s",
        name.AsZStr ())
    return task_info.Resource;
}

int Game::Run(int argc, char ** argv)
{
    Txt genrltxt {GetResource ("GENRLTXT.TXT"), "GENRLTXT.TXT"};
    Game::GENRLTXT = &genrltxt;
    Txt lcdesctxt {GetResource ("lcdesc.txt"), "lcdesc.txt"};
    Game::lcdesc = &lcdesctxt;
    Txt vcdesctxt {GetResource ("vcdesc.txt"), "vcdesc.txt"};
    Game::vcdesc = &vcdesctxt;

    // create the main window
    // Again, no plug-in interface yet, so
    auto main_window =
        IWindow::Create<H3R_NS::MainWindow, H3R_NS::SDLWindow>(
            argc, argv, Point {800, 600});

    Game::MainWindow = main_window;
    main_window->Show (); // make it visible
    return ui_main (argc, argv);
}

NAMESPACE_H3R