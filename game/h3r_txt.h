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

// this is where it becomes scary; guess the number of these
// New_Game->Available_Scenarios
#define H3R_GENRLTXT_SCENARIO_NAME 587
#define H3R_GENRLTXT_SCENARIO_DESCR 588
#define H3R_GENRLTXT_SHOW_AVAIL_SCEN 592
#define H3R_GENRLTXT_RANDOM_MAP 862
#define H3R_GENRLTXT_VCON 589
#define H3R_GENRLTXT_LCON 590
#define H3R_GENRLTXT_MAP_DIFFICULTY 586
#define H3R_GENRLTXT_SHOW_ADV_OPT 593

// lcdesc.txt
#define H3R_LCON_DEFAULT 0
// vcdesc.txt
#define H3R_VCON_DEFAULT 0

H3R_NAMESPACE

class Txt final
{
    private List<String> _lines {};
    private String _res_name;

    //TODO to StreamReader
    private inline bool ReadLine(Stream * stream, String & result)
    {
        static const int BUF_SIZE {128};
        static byte buf[BUF_SIZE] {};
        static byte * n {}, * e {}, * m {};
        m = n ? n : buf;
        bool cr {}, lf {};
        for (;;) {
            if (n >= e) {
                auto av = stream->Size () - stream->Tell (); // available bytes
                auto br = av > BUF_SIZE ? BUF_SIZE : av;          // read bytes
                if (0 == br) return false;
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
            /*printf ("ld: %d, result: %s, n:%ld\n", l, result.AsZStr (),
                n-m);*/
            n++;
            if (l > 0) return true; // EOL
            if (n >= e && stream->Tell () < stream->Size ()) continue;
            return true; // EOF
        }
    }// ReadLine()
    public Txt(Stream * stream, const String & res_name)
        : _res_name{res_name}
    {
        H3R_ENSURE(nullptr != stream, "TXT: no stream")
        int p = (1<<16);
        String line {};
        while (p-- && ReadLine (stream, line))
            _lines.Put (static_cast<String &&>(line));
        H3R_ENSURE(p > 0, "Too many lines of text")
    }
    public ~Txt() {}
    public inline int LineCount() const { return _lines.Count (); }
    // If you see that text: either your data is corrupted, or my program has a
    // bug.
    public inline const String & operator[](int i) const
    {
        static String bug {};
        return i >= 0 && i < _lines.Count () ? _lines[i]
            : bug = String::Format ("bug[]:%s:%d", _res_name.AsZStr (), i);
    }
};// Txt

NAMESPACE_H3R

#endif