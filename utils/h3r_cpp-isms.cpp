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

#include "h3r_os.h"

#ifdef H3R_TEST
#define IES noexcept
#else
#define IES
#endif

// "delete" was requested by C++ virtual ~base(), just because they can.
// If you have a duplicate, just comment this one out.
void operator delete(void *) IES { H3R_ENSURE (false, "It was used!") }

// Why is all of this being done? The above one, no abstract methods,
// no exceptions, no STL, etc. ?
// Because the final executable won't have unwanted runtime dependencies.
// Its only dependency shall be libc.so.
// It is being coded on a musl-libc linux system, so it should run on any POSIX
// one, no matter whats your c++ runtime, unwinder, c++abi, and libc.

// The "no STL" reason is way broader of course. Short: Use Delphi - its
// code library, C# - the ".NET Framework", Java - its framework. Compare.
//
// I'm not writing this to initiate pointless discussion(s). You should use the
// STL, and the other one, even for the sole reason to acquire some objective
// reasoning should you continue using them or not.
// This is for people who want to know my humble POV behind my decision for my
// project.
