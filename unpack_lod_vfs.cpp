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

// lod file format unpacker; it unpacks to the current directory!
// H3R_MM rule: when the engine is built -DH3R_MM, so does this
//c clang++ -std=c++11 -I. -Iasync -Ios -Ios/posix -Iutils -Istream -Igame -DH3R_MM -O0 -g -DH3R_DEBUG -fsanitize=address,undefined,integer,leak -fvisibility=hidden -fno-exceptions -fno-threadsafe-statics unpack_lod_vfs.cpp -o unpack_lod_vfs main.a -lz

#include <time.h>

#include "h3r_os_error.h"
H3R_ERR_DEFINE_UNHANDLED
H3R_ERR_DEFINE_HANDLER(Memory,H3R_ERR_HANDLER_UNHANDLED)

#include "h3r_log.h"
H3R_LOG_STATIC_INIT

#include "h3r_filestream.h"
#include "h3r_thread.h"
#include "h3r_lodfs.h"
#include "h3r_taskthread.h"
#include "h3r_taskstate.h"
#include "h3r_iasynctask.h"
#include "h3r_vfs.h"
#include "h3r_criticalsection.h"

// allow file replacement
H3R_NAMESPACE
namespace OS { class FileErrReplace final : public Error
{
    public: bool Handled(Error * e = nullptr) override
    {
        FileError * fe = static_cast<FileError *>(e);
        if (FileError::Op::Replace == fe->Op)
            return fe->Replace = true;
        return false;
    }
}; }
NAMESPACE_H3R
static H3R_NS::OS::FileErrReplace my_file_err;
H3R_ERR_DEFINE_HANDLER(File, my_file_err)

static int const QN {10};
static const char * const Q[QN] =
{
    "\"Countless words count less than the silent balance between yin and yang\"",
    "\"Nature does not hurry, yet everything is accomplished.\"",
    "\"He strains to hear a whisper who refuses to hear a shout.\"",
    "\"Dance with her, and she will forgive much; dance well, and she will forgive anything.\"",
    "\"Surprising what you can dig out of books if you read long enough, isn't it?\"",
    "\"Destiny has two ways of crushing us - by refusing our wishes and by fulfilling them.\"",
    "\"Cahn's Axiom: When all else fails, read the instructions.\"",
    "\"People get what they get. It has nothing to do with what they deserve.\"",
    "Unwise one: The sky is the limit? I hate limits."
};

static H3R_NS::OS::CriticalSection _task_info_gate {};

class MyWalkTask final : public H3R_NS::IAsyncTask
{
    public: H3R_NS::LodFS * _lodfs;
    public: MyWalkTask(H3R_NS::LodFS * fs) : _lodfs{fs} {}
    public: static H3R_NS::TaskState * State;
    public: inline void Do() override
    {
        _lodfs->Walk (
            [](H3R_NS::Stream & stream, const H3R_NS::VFS::Entry & e) -> bool
            {
                static int count {};
                {
                __pointless_verbosity::CriticalSection_Acquire_finally_release
                    ____ {_task_info_gate};
                    *State = H3R_NS::TaskState {count++,
                        H3R_NS::String::Format ("Enumerating ... %s",
                            e.Name.AsZStr ())};
                    State->SetChanged (true);
                }
                H3R_NS::OS::FileStream {
                    e.Name, H3R_NS::OS::FileStream::Mode::WriteOnly}
                    .H3R_NS::Stream::Write (stream);
                return true;
            });
    }
    public: H3R_NS::TaskState Whatsup() override
    {
        __pointless_verbosity::CriticalSection_Acquire_finally_release
            ____ {_task_info_gate};
        return *State;
    }
};
H3R_NS::TaskState * MyWalkTask::State {};

int main(int c, char ** v)
{
    if (2 != c)
        return printf ("usage: unpack_lod lodfile\n");

    printf ("main: %s " EOL, Q[time (nullptr) % QN]);

    H3R_NS::TaskThread MyThread {};
    H3R_NS::LodFS lodfs {v[1]};
    MyWalkTask task {&lodfs};
    H3R_NS::TaskState task_state {0, ""};
    MyWalkTask::State = &task_state;
    MyThread.Task = task;

    while (! MyThread.Done ()) {
        auto info = task.Whatsup ();
        if (info.Changed ())
            printf ("main: [%4d] %-32s\r",
                info.Progress (), info.Message ().AsZStr ());
        // TODO VFS[n/m] complete
        // printf ("main: %003d %% complete\r", info.Progress ());
        // printf ("main: %s\r", _on_lod_entry.Message ().AsZStr ());
        H3R_NS::OS::Thread::Sleep (5);

        static time_t t0 = time (nullptr);
        time_t hms = time (nullptr) / 60;
        if (hms-t0 > 2)
            t0 = hms,
            printf ("main: %s " EOL, Q[hms % QN]);
    }
    // Yes you can miss a status message from the thread because they aren't
    // queued; e.g. the UI shall take care of "completed"
    printf (EOL "main: [%4d] 100 %% complete" EOL,
        task.Whatsup ().Progress ());

    // No threads: short and simple.
    /*H3R_NS::LodFS {v[1]}
        .Walk([](H3R_NS::Stream & stream, const H3R_NS::VFS::Entry & e) -> bool
        {
            H3R_NS::OS::FileStream {
                e.Name, H3R_NS::OS::FileStream::Mode::WriteOnly}
                .H3R_NS::Stream::Write (stream);
            return true;
        });*/

    return 0;
}
