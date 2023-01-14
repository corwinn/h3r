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

#ifndef _H3R_MM_TEMPLATES_H_
#define _H3R_MM_TEMPLATES_H_

// Included at a specific place at h3r_os.h
//  - using OS::Malloc, OS::Memmove, OS::Mfree, OS::Exit, OS::CriticalSection

template <typename T> void MM::Add(T * & p, size_t n)
{
    {
        __pointless_verbosity::CriticalSection_Acquire_finally_release ____ {
            OS::CriticalSection::MM};

        //TODO h3r_math.h these computations should be overflow-safe - the
        //                idea is to prevent bad allocation attempts
        // Not ovious: SIZE_MAX and INT_MAX are provided by the OS
        H3R_ENSURE(n <= H3R_MEMORY_LIMIT, "RAM limit overflow")
        H3R_ENSURE(n <= SIZE_MAX / sizeof(T), "size_t overflow")
        size_t q = n * sizeof (T);
        const int entry_size = sizeof (Entry);
        H3R_ENSURE(_current_bytes <= INT_MAX - entry_size, "int overflow")
        int tmp = _current_bytes + entry_size;
        H3R_ENSURE(q <= SIZE_MAX - tmp, "size_t overflow")
        size_t nt = tmp + q;
        H3R_ENSURE(nt <= H3R_MEMORY_LIMIT, "RAM limit overflow")
        _current_bytes = (int)nt;

        if (_n == _c) {
            _c += 10;
            Entry * t;
            OS::Malloc (t, _c * sizeof (Entry));
            if (_e) OS::Memmove (t, _e, _n * sizeof (Entry)),
                OS::Mfree (_e);
            _e = t;
        }
        // dLog_stdout ("+Block $%3d: %d bytes" EOL, _n, q);
        Malloc (p, n);
        _e[_n].p = p;
        _e[_n++].n = q; _a++; _user_bytes += q;
    } // CriticalSection::MM.Acquire
} // public: template <typename T> void Add

template <typename T> void MM::Remove(T * & p)
{
    if (! p) return;
    {
        __pointless_verbosity::CriticalSection_Acquire_finally_release ____ {
            OS::CriticalSection::MM};

        for (int i = _n-1; i >= 0; i--)
            if (_e[i].p == p) {
                // dLog_stdout ("-Block $%3d: %d bytes" EOL, i, _e[i]->n);
                _current_bytes -= (_e[i].n + sizeof (Entry));
                OS::Mfree (p);
                var bytes = (_n - 1 - i) * sizeof (Entry);
                if (i < _n - 1) OS::Memmove (_e + i, _e + i + 1, bytes);
                _n--; _f++;
                return;
            }
    }
    H3R_LOG_STDERR("Not allocated here: %p", p);
    OS::Exit (OS::EXIT_WITH_ERROR);
}

#endif