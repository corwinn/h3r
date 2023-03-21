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

#ifndef _H3R_CRITICALSECTION_H_
#define _H3R_CRITICALSECTION_H_

// Critical Section Object.
//
// Used by base building blocks: Log_stderr, Log_stdout, MM

#include "h3r.h"
#include <pthread.h>

H3R_NAMESPACE
namespace OS {

//  POSIX,
// how do you handle sync object API error codes in a thread-safe manner? TIA

// Represents a critical section as in "Critical Section Objects".
// This class is written in the hopes that "printf" is thread-safe - e.g. I
// can not predict what exactly will happen when error reporting has to be
// done - expect anything ("undefined behaviour").
class CriticalSection final
{
    H3R_CANT_COPY(CriticalSection)
    H3R_CANT_MOVE(CriticalSection)

    // All these API functions return error codes. Bonus: C++ destructors are
    // denying their right to return said codes.

    private pthread_mutex_t _m {};
    // required by the WaitObj
    public inline pthread_mutex_t & Mutex_T() { return _m; }

    public CriticalSection();

    // EDEADLK - you already locked this; no it won't detect the "big fun"
    // deadlocks (when thread A waits for thread B and thread B waits for
    // thread A simultaneously); yes threading is complexity; and finesse.
    public CriticalSection & Acquire();

    // EPERM - this ain't yours to unlock
    public void Release();

    // On error: it will try to report the error, then call exit().
    //DONE Careful! main-thread :Acquire->Destroy ain't causing an error:
    //   CriticalSection::Log_stderr.Acquire (); return 0;
    // POSIX states:
    // "Attempting to destroy a locked mutex results in undefined behaviour.",
    // "pthread_mutex_destroy() function may fail".
    // So there "may" be an error notifying you of bad threading on your side.
    public ~CriticalSection();
}; // class CriticalSection

} // namespace OS
NAMESPACE_H3R

//TODO avoid repetition: Just copy this for your platform
namespace __pointless_verbosity
{
    struct CriticalSection_Acquire_finally_release final
    {
        H3R_NS::OS::CriticalSection & _s;
        CriticalSection_Acquire_finally_release(H3R_NS::OS::CriticalSection & s)
            : _s{s.Acquire ()} {}
        ~CriticalSection_Acquire_finally_release() { _s.Release (); }
    };
}

#endif
