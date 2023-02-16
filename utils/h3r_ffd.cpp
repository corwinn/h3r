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

#include "h3r_ffd.h"
#include "h3r_filestream.h"
#include "h3r_memorystream.h"

H3R_NAMESPACE

static int global_line = 1;

static inline bool is_whitespace(byte b) { return b <= 32; }
static inline bool is_comment(const byte * buf, int len, int i)
{
    return '/' == buf[i] && i < len-1 && ('/' == buf[i+1] || '*' == buf[i+1]);
}
static inline bool is_eol(const byte * buf, int len, int i)
{
    if ('\r' == buf[i] && i < len - 1) return '\n' == buf[i+1];
    return '\n' == buf[i];
}

// read_while c()
template <typename F> inline static void read_while(const char * txt,
    const byte * buf, int len, int & i, F c)
{
    int j = i;
    while (i < len && c ()) {
        if (is_eol (buf, len, i)) global_line++;
        i++;
    }
    if (i != j)
        printf ("%3d: read_%s    : [%5d;%5d]" EOL, global_line, txt, j, i);
}

static void skip_whitespace(const byte * buf, int len, int & i)
{
    read_while ("whitespc", buf, len, i, [&](){return buf[i] <= 32;});
}

// ensure is_comment() returns true prior calling this one
static void skip_comment(const byte * buf, int len, int & i)
{
    if ('/' == buf[i+1])
        read_while ("comment1", buf, len, i, [&](){return buf[i] != '\n';});
    else if ('*' == buf[i+1])
        // Yes, that would be slow if I had to parse 60 files per second ...
        read_while ("commentn", buf, len, i,
              [&](){return !('*' == buf[i] && i < len-1 && '/' == buf[i+1]);});
}

static void skip_nonwhitespace(const byte * buf, int len, int & i)
{
    read_while("parse_me", buf, len, i,
        [&](){return buf[i] > 32 && buf[i] <= 126;});
}

/*static*/ FFD::Node * FFD::File2Tree(const String & d, const String &)
{
    OS::FileStream fh {d, H3R_NS::OS::FileStream::Mode::ReadOnly};
    MemoryStream br {&fh, static_cast<int>(fh.Size ())};
    const byte * buf = br.Buffer ();
    int len = static_cast<int>(br.Size ());

    // pre-validate - simplifies the parser
    for (int i = 0; i < len; i++) {
        H3R_ENSURE(
            '\n' == buf[i] || '\r' == buf[i] || (buf[i] >= 32 && buf[i] <= 126),
            "Wrong chars at description")
    }

    printf ("parsing %d bytes ffd" EOL, len);
    // 1. Read the nodes in TB LR manner
    for (int i = 0, chk = 0; i < len; chk++) {
        if (is_whitespace (buf[i])) skip_whitespace (buf, len, i);
        else if (is_comment (buf, len, i)) {
            skip_comment (buf, len, i);
        }
        else {
            skip_nonwhitespace (buf, len, i);
        }
        H3R_ENSURE(chk < len, "infinite loop")
    }// (int i = 0; i < len;)

    return nullptr;
}// FFD::File2Tree()

NAMESPACE_H3R
