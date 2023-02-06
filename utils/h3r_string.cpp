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

#include "h3r_string.h"
#include "h3r_criticalsection.h"

H3R_NAMESPACE
using OS::Log_stdout;
template <typename T> String & String::append(const T * buf, int num)
{
    if (num <= 0) return *this;
    int new_size = num + Length () + _NZ;
    _b.Resize (new_size);
    byte * ptr = _b;
    OS::Memmove (ptr + new_size - _NZ - num, buf, num * sizeof(T));
    return *this;
}

String::String(const char * cstr) { append (cstr, OS::Strlen (cstr)); }
String::String(const byte * cstr, int len) { append (cstr, len); }

String::String(Array<byte> && b) { b.MoveTo (_b); }

String::String(const String & s) { append (s.AsZStr (), s.Length ()); }
String & String::operator=(const String & s) { return _b = s._b, *this; }

String::String(String && s)
{
    /* Log_stdout ("Move" EOL); */
    s._b.MoveTo (_b);
    // "s" can't be left in an invalid state; move shall do alloc();
    //s._b.Resize (String::_NZ);
}
String & String::operator=(String && s)
{
    s._b.MoveTo (_b);
    //s._b.Resize (String::_NZ);
    return *this;
}

String String::Format(const char * fmt, ...)
{
    static OS::CriticalSection lock;
    __pointless_verbosity::CriticalSection_Acquire_finally_release ____ {lock};

    // dLog_stdout ("String::Format: fmt: \"%s\"" EOL, fmt);
    static const int MAX_BUF_SIZE {1<<11};
    static const int BUF_SIZE {1};
    // Stay out of any static memory managers.
    static Array<byte, OS::Malloc, OS::Mfree> buf {BUF_SIZE};
    int r, len = buf.Length ();
    va_list ap;
    unsigned short iloop = 1;
    do {
        H3R_ENSURE(iloop = iloop << 1, "What is it that you're formatting?")
        va_start (ap, fmt);
        // as it happens, it doesn't print when len == r
        r = vsnprintf (
            reinterpret_cast<char *>(buf.operator byte * ()), len, fmt, ap);
        va_end (ap);
        H3R_ENSURE(r >= 0, "vsnprintf() error")
        // dLog_stdout ("String::Format: r: %d" EOL, r);
        bool done = r < len;
        len = r + 1;
        H3R_ENSURE(len <= MAX_BUF_SIZE, "MAX_BUF_SIZE overflow")
        if (done) break; else buf.Resize (len); // grow
    }
    while (1);
    return String (
        buf, OS::Strlen (reinterpret_cast<const char *>(buf.Data ())));
}

String & String::operator+=(const String & s)
{
    return this->append (s.AsZStr (), s.Length ());
}

String & String::operator+=(const char * s)
{
    return this->append (s, OS::Strlen (s));
}

String String::operator+(const String & s)
{
    return (String (*this) += s);
}

String String::operator+(const char * s)
{
    return (String (*this) += s);
}

bool String::operator==(const String & s) const
{
    if (this == &s) return true;
    if (this->Length () != s.Length ()) return false;
    return ! OS::Strncmp (*this, s, this->Length ());
}

String operator+(const char * l, const String & r)
{
    return String (l) + r;
}

String String::ToLower() const //TODO uncode (iconv)
{
    Array<byte> result = _b;
    for (int i = 0; i < Length (); i++)
        result[i] = static_cast<byte>(OS::ToLower (result[i]));
    return String {static_cast<Array<byte> &&>(result)};
}

String String::Replace(const char * what, const char * with)
{
    H3R_ARG_EXC_IF(nullptr == what, "\"what\" can't be null")
    H3R_ARG_EXC_IF(nullptr == with, "\"with\" can't be null")
    int wlen = OS::Strlen (what);
    if (wlen <= 0) return *this;
    int slen = OS::Strlen (what);
    if (slen <= 0) return *this;
    if (_b.Empty ()) return *this;
    byte * b = _b;
    char * c = reinterpret_cast<char *>(b);
    Array<byte> result {};
    // a.txt ".d" ".e"
    char * p = c;
    for (char * i = c; i <= c + Length () - slen;)
        if (! OS::Strncmp (i, what, slen)) {
            if (i > p) result.Append (p, i-p);
            result.Append (with, OS::Strlen (with));
            p = (i = i + slen);
        }
        else i++;
    if (p < c + Length ()) result.Append (p, (c + Length ()) - p);
    if (! result.Empty ()) return String {result, result.Length ()};
    else return *this;
}

NAMESPACE_H3R