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
template <typename T> String & String::append(const T * buf, size_t num)
{
    if (num > 0) _b.Append (buf, num);
    return *this;
}

String & String::append(const String & s)
{
    var & buf = s.AsByteArray ();
    return this->append (buf.Data (), buf.Length ());
}

String::String(const char * cstr) { append (cstr, OS::Strlen (cstr)); }
String::String(const byte * cstr) { append (cstr, OS::Strlen ((char *)cstr)); }

String::String(const Array<byte> & b) { append (b.Data (), b.Length ()); }
String::String(Array<byte> && b) { b.MoveTo (_b); }

String::String(const String & s) { /*Log_stdout ("Copy" EOL);*/ append (s); }
String & String::operator=(const String & s) { return _b = s._b, *this; }

String::String(String && s) { /*Log_stdout ("Move" EOL);*/ s._b.MoveTo (_b); }
String & String::operator=(String && s) { return s._b.MoveTo (_b), *this; }

String String::Format(const char * fmt, ...)
{
    static OS::CriticalSection lock;
    __pointless_verbosity::CriticalSection_Acquire_finally_release ____ {lock};

    // dLog_stdout ("String::Format: fmt: \"%s\"" EOL, fmt);
    static const size_t MAX_BUF_SIZE {1<<11};
    static const size_t BUF_SIZE {1};
    // Stay out of any static memory managers.
    static Array<byte, OS::Malloc, OS::Mfree> buf {BUF_SIZE};
    size_t r, len = buf.Length ();
    va_list ap;
    unsigned short iloop = 1;
    do {
        H3R_ENSURE(iloop = iloop << 1, "What is it that you're formatting?")
        va_start (ap, fmt);
        // as it happens, it doesn't print when len == r
        r = vsnprintf ((char *)(byte *)buf, len, fmt, ap);
        va_end (ap);
        H3R_ENSURE(r >= 0, "vsnprintf() error")
        // dLog_stdout ("String::Format: r: %d" EOL, r);
        bool done = r < len;
        len = r + 1;
        H3R_ENSURE(len <= MAX_BUF_SIZE, "MAX_BUF_SIZE overflow")
        if (done) break; else buf.Resize (len); // grow
    }
    while (1);
    return String (buf);
}

size_t String::Length() const { return _b.Length (); }

String & String::operator+=(const String & s)
{
    return this->append (s);
}

String & String::operator+=(const char * s)
{
    return this->append (s, OS::Strlen (s));
}

String String::operator+(const String & s)
{
    return String ().append (*this). append (s);
}

String String::operator+(const char * s)
{
    return String ().append (*this).append (s, OS::Strlen (s));
}

bool String::operator==(const String & s)
{
    if (this == &s) return true;
    var & a = _b;
    var & b = s._b;
    if (b.Length () != a.Length ()) return false;
    return ! OS::Strncmp ((const char *)a.Data (),
                          (const char *)b.Data (), a.Length ());
}

String operator+(const char * l, const String & r)
{
    return String (l) + r;
}

#include <ctype.h>

String String::ToLower()//TODO uncode (iconv)
{
    Array<byte> result {};
    int tmp {};
    for (auto c : AsByteArray ())
        result.Append (reinterpret_cast<byte *>(&(tmp = tolower (c))), 1);
    return String {result};
}

NAMESPACE_H3R