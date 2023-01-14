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

// "main" class - required in order to put to order the init/deinit of things.
// All static init should avoid using services that are initialized here.

#ifndef _H3R_GAME_H_
#define _H3R_GAME_H_

#include "h3r.h"
#include "h3r_log.h"
#include "h3r_log_stdout.h"
#include "h3r_log_file.h"

H3R_NAMESPACE

// Singleton - implicit contract by Log. A second instance will reward you with
// exit(EXIT_ASSERTION_FAILED).
class Game final
{
    H3R_CANT_COPY(Game)
    H3R_CANT_MOVE(Game)

                              // These names matter not - their order does.
    private: Log_Stdout _2nd; // You're using OS::Alloc (), OS::Free (), and
    private: Log_File   _3rd; //
    private: Log        _4th; // Log::Info ()

    public: Game();
    public: void SilentLog(bool);
    // Main Thread.
    // This is the thing to call periodically while waiting for some async
    // task to complete. The thing to call from the async task.
    // There are 2 primary types of async tasks the main thread should sync
    // with: file system IO and network IO. The sound thread doesn't need to
    // sync with the main thread - it can feed music/fx data into the sound
    // device on its own while communicating with the main thread with simple
    // messages like start/stop/etc.; it can log w/o the main thread because
    // the log service is threaded.
    public: static void ProcessThings();
    // Another design if ProcessThings() code becomes too repetitious.
    // Example:
    //  _vfs {argv[1]};
    //  Wait (VFS_init_complete);
    //public: static Wait(bool (*AsyncTaskComplete)())
    // { while (! AsyncTaskComplete ()) ProcessThings (); }
    // I think you can safely "repeat" the above 1 line of code, but enforcing
    // named wait functions could be better maintenance-ability wise.
};

NAMESPACE_H3R

#endif