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

#ifndef _H3R_MM_H_
#define _H3R_MM_H_

// Memory management.

H3R_NAMESPACE
// class Game;
namespace OS {

// Singleton.
// Takes care of bad allocation attempts (H3R_MEMORY_LIMIT), memory leaks,
// and alloc/free statistics.
// Long-term allocate and forget - it ain't a GC.
// Out of memory policy: OS::Malloc.
// H3R_MEMORY_LIMIT policy: H3R_ENSURE
// Slow, simple.
//TODONT optimize for speed; usage: rare alloc/free
//TODONT anonymous namespace - it could de-singleton-ize it; c++OOP != OOP
class MM final
{
    H3R_CANT_COPY(MM)
    H3R_CANT_MOVE(MM)

    private OS::CriticalSection _thread_gate {};

    private struct Entry { void * p {}; size_t n {}; };
    private Entry * _e {};
    private int _n {}, _c {}; // num, capacity
    // While allocating this much at once isn't happening, cumulative numbers
    // can easily overflow an "int".
    private size_t _user_bytes {}, _a {}, _f {}; // stats
    private size_t _current_bytes {}; // currently allocated bytes
    private MM();
    // friend class H3R_NS::Game;

    private template <typename T> void Add(T * &, size_t n = 1);
    private template <typename T> void Remove(T * &);
    private ~MM();

    // private: static MM * _one;
    private static MM & One();
    public template <typename T> static void Alloc(T * & p, size_t n = 1)
    {
        MM::One ().Add (p, n);
    }
    public template <typename T> static void Free(T * & p)
    {
        MM::One ().Remove (p);
    }
};

} // namespace OS
NAMESPACE_H3R

// #define H3R_MM_STATIC_INIT \
//    H3R_NS::OS::MM * H3R_NS::OS::MM::_one {nullptr};

#endif
