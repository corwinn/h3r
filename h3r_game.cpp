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
H3R_LOG_STATIC_INIT

#include "h3r_string.h"
#include "h3r_lodfs.h"
#include "h3r_sndfs.h"
#include "h3r_vidfs.h"

H3R_NAMESPACE

TaskThread Game::IOThread {};

ResManager * Game::RM {};

IWindow * Game::MainWindow {};

Game::Game(const char * process_path)
    : _3rd {"main.log"}
{
    _4th.Subscribe (&_2nd);
    _4th.Subscribe (&_3rd);

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
}

Game::~Game()
{
    H3R_DESTROY_OBJECT(Game::RM, ResManager)
    H3R_DESTROY_OBJECT(MainWindow, IWindow)
}

void Game::SilentLog(bool v)
{
    _4th.Silent (v);
}

void Game::ProcessThings()
{
    if (Game::MainWindow) Game::MainWindow->ProcessMessages ();
}

NAMESPACE_H3R