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
//c clang++ -std=c++11 -I. -Iasync -Iui -Ios -Ios/posix -Iutils -Istream -Igame -DH3R_MM -O0 -g -DH3R_DEBUG -fsanitize=address,undefined,integer,leak -fvisibility=hidden -fno-exceptions -fno-threadsafe-statics unpack_lod_vfs.cpp -o unpack_lod_vfs main.a h3r_game.o -lz

#include "h3r_os_error.h"
H3R_ERR_DEFINE_UNHANDLED
H3R_ERR_DEFINE_HANDLER(Memory,H3R_ERR_HANDLER_UNHANDLED)
H3R_ERR_DEFINE_HANDLER(Log,H3R_ERR_HANDLER_UNHANDLED)

#include "h3r_filestream.h"
#include "h3r_game.h"
#include "h3r_lodfs.h"
#include "h3r_resmanager.h"
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

static int const QN {4};
static int qring {0}, qtimeout {0};
static int const QNEXT {2000}; // [ms]
static const char * const Q[QN] =
{
    "\"Nature does not hurry, yet everything is accomplished.\"",
    "\"He strains to hear a whisper who refuses to hear a shout.\"",
    "\"Dance with her, and she will forgive much; dance well, and she will forgive anything.\"",
    "\"Surprising what you can dig out of books if you read long enough, isn't it?\""
};

// Display entry names as they're being enumerated
H3R_NAMESPACE
class VFSProgressHandler final : public VFS::VFSEvent
{
    private: OS::CriticalSection _msg_lock {};
    // IOThread
    public: void Do(VFS::VFSInfo * info) override
    {
        if (info->Changed ()) {//TODO resolve .AsZStr ()
            __pointless_verbosity::CriticalSection_Acquire_finally_release
                ____ {_msg_lock};
            _msg = H3R_NS::String::Format (
                "IOThread: Progress: %003d %% %s         \r",
                info->Progress (), info->Message ().AsZStr ().Data ());
        }
    }
    private: String _msg;
    public: String Message()
    {
        __pointless_verbosity::CriticalSection_Acquire_finally_release
            ____ {_msg_lock};
        return _msg;
    }
};
NAMESPACE_H3R

int main(int c, char ** v)
{//DONE async IO, with the main thread displaying progress; and wise quotes
    if (2 != c)
        return printf ("usage: unpack_lod lodfile\n");

    // init the log service
    H3R_NS::Game game;

    H3R_NS::OS::Log_stdout ("%s " EOL, Q[qring]);

    H3R_NS::ResManager RM;

    // the res manager handles all VFS, so don't do this:
    //   H3R_NS::LodFS lod_handler {};
    // do this:
    H3R_NS::LodFS * lod_handler {};
    H3R_CREATE_OBJECT(lod_handler, H3R_NS::LodFS) {};
    RM.Register (lod_handler);

    var task_info = RM.Load (v[1]);
    while (! RM.TaskComplete ()) {
        var msg = H3R_NS::String {task_info.GetInfo ().Message ()} + EOL;
        H3R_NS::Log::Info (msg);
        H3R_NS::OS::Thread::SleepForAWhile ();
    }

    H3R_NS::VFSProgressHandler _on_lod_entry {};
    RM.OnProgress.SetNext (&_on_lod_entry);
    var task_info_enum = RM.Enumerate (
        [](H3R_NS::Stream & stream, const H3R_NS::VFS::Entry & e)
        {
            H3R_NS::OS::FileStream {
                e.Name, H3R_NS::OS::FileStream::Mode::WriteOnly}
                .H3R_NS::Stream::Write (stream);
            return true;
        }
    );

    while (! RM.TaskComplete ()) {
        var info = task_info_enum.GetInfo ();
        if (info.Changed ()) {
            var msg = H3R_NS::String {info.Message ()} + "\r";
            H3R_NS::Log::Info (msg);
        }
        // TODO VFS[n/m] complete
        // H3R_NS::OS::Log_stdout ("%003d %% complete\r", info.Progress ());
        H3R_NS::Log::Info (_on_lod_entry.Message ());
        H3R_NS::OS::Thread::SleepForAWhile ();

        qtimeout += 2; if (qtimeout > QNEXT) {
            qring = (qring + 1) % QN; qtimeout = 0;
            H3R_NS::OS::Log_stdout ("%s " EOL, Q[qring]);
        }
    }
    // Yes you can miss a status message from the thread because they aren't
    // queued; e.g. the UI shall take care of "completed"
    H3R_NS::OS::Log_stdout (EOL "100 %% complete" EOL);

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
