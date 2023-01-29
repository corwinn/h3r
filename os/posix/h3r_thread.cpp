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

static int const THREAD_MAX {5}; // Log, Files, MusicRT, Music, SoundFX
Thread * Thread::Threads[THREAD_MAX];

//TODO I'm not sure this is a good idea. Threads should start and stop in
// certain order, for example: the logger thread should stop last - prior main
Thread::Thread(Proc & p)
    : _p {p}
{
    static int ti {0};
    H3R_ENSURE(ti >= 0 && ti < THREAD_MAX, "thread index out of range")
    Thread::Threads[ti++] = this;
    auto r = pthread_create (&_thr, nullptr, [](void * q) -> void *
        {
            return ((Proc *)q)->Run ();
        }, &p);
    H3R_ENSURE(0 == r, "pthread_create failed")
}

Thread::~Thread()
{
    for (size_t i = 0; i < THREAD_MAX; i++)
        if (this == Thread::Threads[i]) {
            Thread::Threads[i] = nullptr;
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

/*static*/ void Thread::SleepForAWhile() { Thread::Sleep (1); }

void Thread::Join()
{
    auto r = pthread_join (_thr, nullptr);
    H3R_ENSURE(0 == r, "pthread_join failed")
}

void Thread::Stop()
{
    H3R_ENSURE(false == _p.stop, "Thread already stopped")
    _p.stop = true;
    Join ();
}

/*static*/ void Thread::StopAll()
{
    // foreach (auto t in Thread.Threads.Where (x => x != null)) t.Stop ();
    for (auto t : Thread::Threads) if (t) t->Stop ();
}

} // namespace OS
NAMESPACE_H3R