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

#include "h3r_wait.h"

#include "h3r_os.h"
#define H3R_ENSURE_W(C,M) \
    { if (! (C)) { \
        printf ("Fixme: %s:%d: %s" EOL, __FILE__, __LINE__, M); \
        H3R_NS::OS::Exit (H3R_NS::OS::EXIT_ASSERTION_FAILED); \
    } }

H3R_NAMESPACE
namespace OS {

WaitObj::Mutex::Mutex()
{
    _m = CreateMutex (nullptr, false, nullptr);
    H3R_ENSURE_W(NULL != _m, "CreateMutex")
}

WaitObj::Mutex::~Mutex()
{
    H3R_ENSURE_W(CloseHandle (_m), "Mutex: CloseHandle")
}

WaitObj::Mutex & WaitObj::Mutex::Acquire()
{
    auto r = WaitForSingleObject (_m, INFINITE);
    H3R_ENSURE_W(WAIT_OBJECT_0 == r, "Mute: WaitForSingleObject")
    return *this;
}

void WaitObj::Mutex::Release()
{
    H3R_ENSURE_W(ReleaseMutex (_m), "ReleaseMutex")
}

WaitObj::WaitObj()
{
    bool manual_reset, signalled;
    _e = CreateEvent (nullptr,
        // Follow the POSIX "pthread_cond_t" because that's all they can do.
        manual_reset=false,
        //
        signalled=false, nullptr);
    H3R_ENSURE_W(NULL != _e, "CreateEvent")
}

// Sequence (as far as I understood):
//  1. lock
//  2. unlock
//  3. sleep
//            <-- signal
//  4. lock
//  5. wake
//  6. unlock
// I.e. the lock is unlocked only while the thread is waiting.
void WaitObj::Wait()
{
    // Simple:
    // auto r = WaitForSingleObject (_e, INFINITE)
    // H3R_ENSURE_W(WAIT_OBJECT_0 == r, "WaitForSingleObject")
    // POSIX:
    auto r = SignalObjectAndWait (_gate.Handle (), _e, INFINITE, false);
    H3R_ENSURE_W(WAIT_OBJECT_0 == r, "SignalObjectAndWait")
    _gate.Acquire ();
}

void WaitObj::GoGoGo()
{
    ::__pointless_verbosity::Mutex_Acquire_finally_release
        ____ {_gate};
    H3R_ENSURE_W(SetEvent (_e), "SetEvent")
}

WaitObj::~WaitObj()
{
    H3R_ENSURE_W(CloseHandle (_e), "Event: CloseHandle")
}

} // namespace OS
NAMESPACE_H3R
