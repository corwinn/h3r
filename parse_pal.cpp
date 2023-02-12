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

//c clang++ -std=c++11 -DH3R_MM -fsanitize=address,undefined,integer,leak -fvisibility=hidden -I. -Ios -Ios/posix -Iutils -Istream -O0 -g -DH3R_DEBUG -fno-exceptions -fno-threadsafe-statics main.a parse_pal.cpp -o parse_pal

#define int int
static_assert(4 == sizeof(int), "define 32-bit int type");
static_assert(8 == sizeof(long long), "define 64-bit int type");

#include "h3r_os_error.h"
H3R_ERR_DEFINE_UNHANDLED
H3R_ERR_DEFINE_HANDLER(Memory,H3R_ERR_HANDLER_UNHANDLED)

#include "h3r_filestream.h"

H3R_NAMESPACE
namespace OS {

class FileErrReplace final : public Error
{
    public: bool Handled(Error * e = nullptr) override
    {
        FileError * fe = (FileError *)e;
        if (FileError::Op::Replace == fe->Op)
            return fe->Replace = true;
        return false;
    }
};

} // namespace OS
NAMESPACE_H3R
static H3R_NS::OS::FileErrReplace my_file_err;
H3R_ERR_DEFINE_HANDLER(File,my_file_err)

template <typename T> static H3R_NS::Stream & Read(
    H3R_NS::Stream & s, T * d, size_t num = 1)
{
    return H3R_NS::Stream::Read (s, d, num);
}

#include "store_as_bmp.cpp"

int main(int argc, char ** argv)
{
    if (2 != argc)
        return printf ("usage: parse_pal palfile\n");

    H3R_NS::OS::FileStream s {argv[1], H3R_NS::OS::FileStream::Mode::ReadOnly};
    printf (EOL "%s 1st offset: %000000008X, size: %lu" EOL, argv[1],
            static_cast<uint>(s.Tell ()), s.Size ());

    // these are 3:
    //  Data_H3bitmap_lod/font.pal - there are a lot of colors here?!
    //    the text from screen-shots has 255,243,222 as color - that's nowhere
    //    to be found?!
    //  Data_H3bitmap_lod/PLAYERS.PAL - 8x32 - player color 0-7
    //  Data_H3bitmap_lod/game.pal - some ordered sub blocks here
    char sign1[5] {};
    int size1, size2;
    char sign2[9] {};
    // uint unk1;// 01000300
    short pal_size_in_bytes, pal_entry_num; // resulting palette size, in bytes
    Read (s, sign1, 4).Read (s, &size1);
    if (size1 != s.Size () - s.Tell ()) return printf ("Wrong fmt" EOL), 1;
    Read (s, sign2, 8).Read (s, &size2);
    Read (s, &pal_size_in_bytes).Read (s, &pal_entry_num);
    printf ("Sign1: %s, Size1: %5d, Sign2: %s, Size2: %5d, Pal: %d x %d"
        EOL, sign1, size1, sign2, size2,
        pal_entry_num, pal_size_in_bytes/pal_entry_num);
    H3R_NS::Array<H3R_NS::byte> b {16 * 16}, p {4 * pal_entry_num}; // {r,g,b,x}
    printf (EOL "%s pal offset: %lu" EOL, argv[1], s.Tell ());
    s.Read (static_cast<H3R_NS::byte *>(p), p.Length ());
    // size2 = 1(size,num) + pal_entry_num + 1(unk2)
    for (int u = 1+pal_entry_num; u < size2/4; u++) {
        int unk_n;
        Read (s, &unk_n); printf ("unk%d: %000000008X" EOL, 257-u+1, unk_n);
    }
    // printf ("%000000008X" EOL, static_cast<uint>(s.Tell ()));
    if (size2 != s.Tell ()-20) return printf ("Wrong fmt" EOL), 1;
    for (int i = 0; i < 3; i++) // PLAYERS.PAL, game.pal
        if ((s.Tell () != s.Size ())) {
            char sign_n[5] {};
            int size_n;
            Read (s, sign_n, 4).Read (s, &size_n);
            printf ("Sign%d: %s, Size%d: %5d" EOL, i+3, sign_n, i+3, size_n);
            s.Seek (size_n);
        }

    if (s.Tell () != s.Size ())
        printf ("unparsed %000000008X/%000000008X" EOL,
            static_cast<uint>(s.Tell ()), static_cast<uint>(s.Size ()));
    // 4 -> 3
    H3R_NS::Array<H3R_NS::byte> p3 {3 * 256};
    for (int i=0; i < pal_entry_num; i++) {
        b[i] = i,
        p3[3*i] = p[4*i], p3[3*i+1] = p[4*i+1], p3[3*i+2] = p[4*i+2];
        printf ("%3d: r:%3d, g:%3d, b:%3d, u:%3d" EOL,
            i, p[4*i], p[4*i+1], p[4*i+2], p[4*i+3]);
    }
    store_as_bmp (argv[1], b, 16, 16, 8, p3);

    return 0;
} // main()
