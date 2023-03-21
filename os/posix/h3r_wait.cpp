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

#include <errno.h>
#include "h3r_os.h"

#define H3R_ENSURE_W(C,M) \
    { if (! (C)) { \
        printf ("Fixme: %s:%d: %s" EOL, __FILE__, __LINE__, M); \
        H3R_NS::OS::Exit (H3R_NS::OS::EXIT_ASSERTION_FAILED); \
    } }

H3R_NAMESPACE
namespace OS {

WaitObj::WaitObj()
{
    auto r = pthread_cond_init (&_c, nullptr);
    H3R_ENSURE_W(0 == r, "pthread_cond_init")
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
    auto r = pthread_cond_wait (&_c, &(_gate.Mutex_T ()));
    H3R_ENSURE_W(0 == r, "pthread_cond_wait")
}

void WaitObj::GoGoGo()
{
    ::__pointless_verbosity::CriticalSection_Acquire_finally_release
        ____ {_gate};
    auto r = pthread_cond_signal (&_c);
    H3R_ENSURE_W(0 == r, "pthread_cond_signal")
}

WaitObj::~WaitObj()
{
    auto r = pthread_cond_destroy (&_c);
    H3R_ENSURE_W(0 == r, "pthread_cond_destroy")
}

} // namespace OS
NAMESPACE_H3R
