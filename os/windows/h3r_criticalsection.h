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
#undef public
#include "windows.h"
#define public public:

H3R_NAMESPACE
namespace OS {

// Represents a critical section as in "Critical Section Objects".
class CriticalSection final
{
    H3R_CANT_COPY(CriticalSection)
    H3R_CANT_MOVE(CriticalSection)

    private CRITICAL_SECTION _m; // No point using mutex or semaphore.

    //  POSIX,
    // there is something for you to learn here.
    // ~thank_you.
    public CriticalSection();

    public CriticalSection & Acquire();

    public void Release();

    public ~CriticalSection();
};

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
