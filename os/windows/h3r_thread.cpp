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

#include "h3r_thread.h"
#include <time.h>

H3R_NAMESPACE
namespace OS {

// Log, Files, FileEnum, MusicRT, Music, SoundFX
// Because while "FileEnum" is in progress, "Files" is needed to load resources.
static int const THREAD_MAX {6};
Thread * Thread::Threads[THREAD_MAX];
HANDLE Thread::ThreadHandles[THREAD_MAX];

//TODO I'm not sure this is a good idea. Threads should start and stop in
// certain order, for example: the logger thread should stop last - prior main
Thread::Thread(Proc & p)
    : _p {p}
{
    int ti {0};
    for (int i = 0; i < THREAD_MAX; i++)
        if (nullptr != Thread::Threads[i]) ti++;
    H3R_ENSURE(ti >= 0 && ti < THREAD_MAX, "thread index out of range")
    for (int i = 0; i < THREAD_MAX; i++)
        if (nullptr == Thread::Threads[i]) { // put new first available
            Thread::Threads[i] = this;
            break;
        }
    const LPSECURITY_ATTRIBUTES THREAD_SA {nullptr};
    const SIZE_T  THREAD_DEFAULT_STACK_SIZE {0};
    const DWORD   THREAD_RUN_AT_ONCE {0};
    const LPDWORD THREAD_ID_UNUSED {nullptr};
    _thr = CreateThread (THREAD_SA, THREAD_DEFAULT_STACK_SIZE,
        [](void * q) -> DWORD
        {
            ((Proc *)q)->Run ();
            return 0;
        }, &p, THREAD_RUN_AT_ONCE, THREAD_ID_UNUSED);
    H3R_ENSURE(NULL != _thr, "CreateThread failed")
    Thread::ThreadHandles[ti-1] = _thr;
}

Thread::~Thread()
{
    for (size_t i = 0; i < THREAD_MAX; i++)
        if (this == Thread::Threads[i]) {
            Thread::Threads[i] = nullptr; // mark free
            if (! _p.stop) Stop ();
            return;
        }
}

/*static*/ /*Thread & Thread::Create(Proc &)
{
    Thread * new_thr;
    OS::Alloc (new_thr);
    //TODO return new_thr->Start (p);
    return *new_thr;
}*/

/*static*/ void Thread::Sleep(int msec)
{
    H3R_ENSURE(msec > 0 && msec < 1001, "Sleep under a second please")
    struct timespec foo {0, msec * 1000000};
    nanosleep (&foo, nullptr);
}

/*static*/ void Thread::SleepForAWhile()
{//TODO this doesn't seem to be working; I got ~100% CPU with it; test with
 //     CreateWaitableTimer()
    struct timespec foo {0, 100000}; // [nsec]
    nanosleep (&foo, nullptr);
}

/*static*/ void Thread::NanoSleep(long nsecs)
{
    struct timespec foo {0, nsecs}; // [nsec]
    nanosleep (&foo, nullptr);
}

void Thread::Join()
{
    auto r = WaitForSingleObject (_thr, INFINITE);
    H3R_ENSURE(WAIT_FAILED != r, "WaitForSingleObject failed")
    //TODO https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject
}

void Thread::Stop(bool signal_only)
{
    //TODO re-enact this check (canceled by ~TaskThread + WaitObj)
    // H3R_ENSURE(false == _p.stop, "Thread already stopped")
    _p.stop = true; if (signal_only) return;
    Join ();
}

/*static*/ void Thread::StopAll()
{
    // foreach (auto t in Thread.Threads.Where (x => x != null)) t.Stop ();
    bool const signal_only = true;
    for (Thread * t : Thread::Threads) if (t) t->Stop (signal_only);
    WaitForMultipleObjects (THREAD_MAX, Thread::ThreadHandles, TRUE, INFINITE);
}

} // namespace OS
NAMESPACE_H3R