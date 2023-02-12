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

// Project globals.

#ifndef _H3R_H_
#define _H3R_H_

// there is no auto - there is var,
// #define var auto
// :D
// Something managed to conflict with my poor "var":
//   "mingw64/include/psdk_inc/intrin-impl.h" has a union named "var"
// there is no var - there is intrin-impl.h, obviously :)

#define H3R_NS h3r

#define H3R_TYPE(T) H3R_NS::T
#define H3R_PUBLISH_TYPE(T) using T = H3R_NS::T;

#define H3R_NAMESPACE namespace H3R_NS {
#define NAMESPACE_H3R }

#define H3R_CANT_COPY(T) \
    T(const T &) = delete; T & operator=(const T &) = delete;
#define H3R_CANT_MOVE(T) T(T &&) = delete; T & operator=(T &&) = delete;

//LATER inttypes.h
static_assert(sizeof(int) >= 4, "This program expects 32-bit int at least");

// #include < new >
#define H3R_CREATE_OBJECT(P,T) H3R_NS::OS::Alloc (P); new (P) T
#define H3R_DESTROY_OBJECT(P,T) \
    { if (nullptr != P) { P->~T (); H3R_NS::OS::Free (P); } }

H3R_NAMESPACE

using byte = unsigned char;

#define H3R_LAST_DEPTH 255
using h3rDepthOrder = byte;

using h3rPlayerColor = byte;
#define H3R_VALID_PLAYER_COLOR(PC) (PC >= 0 && PC < 8)
// For some unknown reason the main menu is using 0=Red. See "parse_pal.cpp" for
// more details.
#define H3R_DEFAULT_PLAYER_COLOR 0

// Used by the memory allocator.
// This remake is expected to use no more than:
static int const H3R_MEMORY_LIMIT {1<<29}; // [bytes] of RAM.

NAMESPACE_H3R

namespace __pointless_verbosity {
    // __pointless_verbosity::__try_finally_store<off_t> ___ {f.Ptr};
    // f.Ptr++; ...
    /*template <typename T> struct __try_finally_store
    {
        T & _state;
        T _sentinel;
        __try_finally_store(T & state) : _state {state}, _sentinel {state} {}
        ~__try_finally_store() { _state = _sentinel; }
    };*/
}

#endif
