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

//TODO prove this wrong

#include "h3r_criticalsection.h"

#include "h3r_os.h"
#include <pthread.h>
#include <errno.h>

H3R_NAMESPACE
namespace OS {

CriticalSection CriticalSection::Log_stderr {true};
CriticalSection CriticalSection::Log_stdout;
CriticalSection CriticalSection::MM;

CriticalSection::CriticalSection(bool use_printf)
    : _printf {use_printf}
{
    pthread_mutexattr_t attr;
#ifdef H3R_DEBUG
    auto r = pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_ERRORCHECK);
#else
    auto r = pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_NORMAL);
#endif
    H3R_ENSURE(0 == r, "pthread_mutexattr_settype")
    r = pthread_mutexattr_setpshared (&attr, PTHREAD_PROCESS_PRIVATE);
    H3R_ENSURE(0 == r, "pthread_mutexattr_setpshared")
    r = pthread_mutex_init (&_m, &attr);
    H3R_ENSURE(0 == r, "pthread_mutex_init")
}

CriticalSection & CriticalSection::Acquire()
{
    auto r = pthread_mutex_lock (&_m);
    if (! _printf) {
        H3R_ENSURE(0 == r, "pthread_mutex_lock")
    }
    else switch (r)
    {
        case 0: break;
        default: LOG_ERR_CS("pthread_mutex_lock", r)
    }
    return *this;
}

void CriticalSection::Release()
{
    auto r = pthread_mutex_unlock (&_m);
    if (! _printf) {
        H3R_ENSURE(0 == r, "pthread_mutex_unlock")
    }
    else switch (r)
    {
        case 0: break;
        default: LOG_ERR_CS("pthread_mutex_unlock", r)
    }
}

CriticalSection::~CriticalSection()
{
    auto r = pthread_mutex_destroy (&_m);
    if (! _printf) {
        H3R_ENSURE(0 == r, "pthread_mutex_destroy")
    }
    else switch (r)
    {
        case 0: break;
        default: LOG_ERR_CS("pthread_mutex_destroy", r)
    }
}

} // namespace OS
NAMESPACE_H3R
