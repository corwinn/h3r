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

#ifndef _H3R_DBG_H_
#define _H3R_DBG_H_

#include "h3r.h"
#include "h3r_os.h"
#include "h3r_string.h"

H3R_NAMESPACE

// print h rows, w size each; cell: buf[x,y][n-1]
void PrintBuf2D(const char * cell_fmt, const byte * buf, int w, int h, int n);

// Convenience.
struct UnqueuedThreadSafeDebugLog final
{
    using L = UnqueuedThreadSafeDebugLog;
    template <typename T> inline L & Fmt(const char * f, T & v)
    {
        if (Enabled) OS::Log_stdout (f, v);
        return *this;
    }
    template <typename T> inline L & Fmt(const char * f, T && v)
    {
        if (Enabled) OS::Log_stdout (f, v);
        return *this;
    }
    inline L & operator<<(const char * v) { return Fmt ("%s", v); }
    inline L & operator<<(long v) { return Fmt ("%ld", v); }
    inline L & operator<<(unsigned long v) { return Fmt ("%lu", v); }
    inline L & operator<<(int v) { return Fmt ("%d", v); }
    inline L & operator<<(short v) { return Fmt ("%d", v); }
    inline L & operator<<(byte v) { return Fmt ("%d", v); }
    inline L & operator<<(const String & v) { return Fmt ("%s", v.AsZStr ()); }
    // Dbg << Dbg.Fmt ("%foo", bar) << "p" << q << EOL;
    inline L & operator<<(L & l) { return l; }

    bool Enabled {true};
    static UnqueuedThreadSafeDebugLog & D();
};

#define Dbg (H3R_NS::UnqueuedThreadSafeDebugLog::D ())

NAMESPACE_H3R

#endif