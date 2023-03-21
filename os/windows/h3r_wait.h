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

#ifndef _H3R_WAIT_H_
#define _H3R_WAIT_H_

// Wait object - a.k.a. "cond" a.k.a. "event".

#include "h3r.h"
#undef public
#include "windows.h"
#define public public:

H3R_NAMESPACE
namespace OS {

// Thread::Idle
class WaitObj final
{
    H3R_CANT_COPY(WaitObj)
    H3R_CANT_MOVE(WaitObj)

    // learn.microsoft.com/en-us/windows/win32/sync/synchronization-objects
    public class Mutex final
    {
        private HANDLE _m {};
        public HANDLE & Handle() { return _m; }
        Mutex();
        ~Mutex();
        public Mutex & Acquire();
        public void Release();
    };

    // mutex - because SignalObjectAndWait can't signal CS
    private Mutex _gate {};
    private HANDLE _e {};

    public inline Mutex & Lock() { return _gate; }

    public WaitObj();

    // Thread A waits. Ensure Lock() above is Acquire()d by the Wait()er.
    public void Wait();

    // Thread B lets thread A go.
    public void GoGoGo();

    public ~WaitObj();
}; // class WaitObj

} // namespace OS
NAMESPACE_H3R

namespace __pointless_verbosity
{
    struct Mutex_Acquire_finally_release final
    {
        H3R_NS::OS::WaitObj::Mutex & _m;
        Mutex_Acquire_finally_release(H3R_NS::OS::WaitObj::Mutex & m)
            : _m{m.Acquire ()} {}
        ~Mutex_Acquire_finally_release() { _m.Release (); }
    };
}

#endif
