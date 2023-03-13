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

// .txt game resource service

#ifndef _H3R_TXT_H_
#define _H3R_TXT_H_

#include "h3r.h"
#include "h3r_list.h"
#include "h3r_stream.h"
#include "h3r_string.h"

H3R_NAMESPACE

class Txt final
{
    private List<String> _lines {};
    private inline String ReadLine(Stream * stream) //TODO to StreamReader
    {
        static const int BUF_SIZE {128};
        static byte buf[BUF_SIZE] {};
        static byte * n {}, * e {}, * m {};
        m = n ? n : buf;
        String result {};
        bool cr {}, lf {};
        for (;;) {
            if (n >= e) {
                auto av = stream->Size () - stream->Tell (); // available bytes
                auto br = av > BUF_SIZE ? BUF_SIZE : av;          // read bytes
                if (0 == br) return static_cast<String &&>(result);
                stream->Read (buf, br);
                m = n = buf, e = buf + br;
            }
            while (n < e) {
                if ('\n' == *n) { lf = true; break; }
                else if ('\r' == *n) cr = true;
                n++;
            }
            int l = cr && lf ? 2 : lf ? 1 : 0;
            if (n-(l ? l-1 : 0) > m)
                result += String {m, static_cast<int>((n-(l ? l-1 : 0))-m)};
            /*printf ("l: %d, result: %s, n:%d\n", l, result.AsZStr (),
                n-m);*/
            n++;
            if (l > 0) return result; // EOL
            if (n >= e && stream->Tell () < stream->Size ()) continue;
            return result; // EOF
        }
    }// ReadLine()
    public Txt(Stream * stream)
    {
        H3R_ENSURE(nullptr != stream, "TXT: no stream")
        int p = (1<<16);
        do {
            _lines.Put (static_cast<String &&>(ReadLine (stream)));
        } while (p-- && stream->Tell () < stream->Size ());
        H3R_ENSURE(p > 0, "Too many lines of text")
    }
    public ~Txt() {}
    public inline int LineCount() const { return _lines.Count (); }
    public inline const String & operator[](int i) const { return _lines[i]; }
};// Txt

NAMESPACE_H3R

#endif