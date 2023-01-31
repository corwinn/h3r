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
#include "h3r_taskthread.h"
#include "h3r_resmanager.h"
#include "h3r_gamearchives.h"
#include "h3r_asyncfsenum.h"
#include "h3r_iwindow.h"

H3R_NAMESPACE

// Singleton - implicit contract by Log. A second instance will reward you with
// exit(EXIT_ASSERTION_FAILED).
class Game final
{
    H3R_CANT_COPY(Game)
    H3R_CANT_MOVE(Game)

                              // These names matter not - their order does.
    private: Log_Stdout _2nd; // You're using OS::Alloc (), OS::Free (), and
#if LOG_FILE
    private: Log_File   _3rd; //
#endif
    private: Log        _4th; // Log::Info ()

    public: static IWindow * MainWindow;

    public: Game(const char * process_path);
    public: ~Game();
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

    // The IO thread. You want IO done: Game::IOThread.Task = your IAsyncTask.
    // Now what happens if a task is already in progress? Your thread shall be
    // blocked on a sync. object waiting for said task to complete.
    //
    // Let me re-summarise the threading model of this application; 4 shall ride
    // forward:
    //  * main  - handles rendering & interactive events; issues "sound", "IO"
    //            and "log" requests;
    //  * log   - handles "log" requests; does its own IO in its own thread
    //            through its own IO adapters, if any
    //  * sound - handles "sound" requests; does its own IO in its own thread
    //  * IO    - handles IO requests from the "main" thread only
    // The "main" one shall render progress or display messages (the rare
    // "please wait" coming to mind :) ), etc. while waiting for an IO task to
    // complete, and not wait for another one prior the 1st one completes
    // because (see TaskThread). One task at a time!
    // There are a few independent threads:
    //  * AsyncFsEnum - handles FS file enumeration in its own thread
    //  * AsyncAdapter - use any Stream object in a separate thread;
    //                   shall be used for NetworkStream for example
    //TODO sequence diagram
    // There is no point to further complicate this complex program with task
    // queues, thread pools, and all the bonus deadlocks and hundreds of hours
    // on the whiteboard, coming with all that.
    //
    // This is not and it will never be a server, or handle-it-all framework,
    // or R&D on parallelism, or whatever; because: short and simple.
    public: static TaskThread IOThread;

    public: static ResManager * RM;//PERHAPS this doesn't need to be public

    public: static Stream * GetResource(const String & name);

    private: class ResManagerInit final
    {
        private: H3R_NS::GameArchives GA {};
        private: H3R_NS::ResManager & RM;
        private: H3R_NS::AsyncFsEnum<ResManagerInit> _subject;
        private: int _files {}, _dirs {};
        private: bool HandleItem(
            const H3R_NS::AsyncFsEnum<ResManagerInit>::EnumItem & itm)
        {
            if (! itm.IsDirectory) {
                _files++;
                H3R_NS::String name_lc = itm.FileName.ToLower ();
                if (GA.Has (name_lc)) {
                    H3R_NS::OS::Log_stdout ("Resource Manager: "
                        "Registering Game Archive: \"%s\"" EOL,
                        (const char *)itm.Name);

                    //TODO there is no need for this thread (! main) to wait
                    //     another one (! main);
                    // Async load the Game Archive
                    RM.Load (itm.Name);
                    while (! RM.TaskComplete ()) {
                        //LATER (messages from the IOThread)
                        // H3R_NS::Log::Info (H3R_NS::String::Format ("%s" EOL,
                        //    task_info.GetInfo ().Message ().AsZStr ()));
                        H3R_NS::OS::Thread::SleepForAWhile ();
                    }
                }
            } else _dirs++;
            return true;
        }
        public: ResManagerInit(H3R_NS::String path, H3R_NS::ResManager& rm)
            : GA {}, RM{rm}, _subject{
//np base_path: path, observer: this, handle_on_item: &ResManagerInit::HandleItem
                path, this, &ResManagerInit::HandleItem} {}
        public: bool Complete() const { return _subject.Complete (); }
        public: int Files() const { return _files; }
        public: int Directories() const { return _dirs; }
    }; // ResManagerInit

    public: int Run(int, char **);
};// Game

NAMESPACE_H3R

#endif