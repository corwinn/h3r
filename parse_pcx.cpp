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

// Q'N'D code.

//c clang++ -std=c++11 -fsanitize=address,undefined,integer -fvisibility=hidden -I. -Ios -Ios/posix -Iutils -Istream -O0 -g -DH3R_DEBUG -fno-exceptions -fno-threadsafe-statics main.a h3r_game.o parse_pcx.cpp -o parse_pcx

#define int int
static_assert(4 == sizeof(int), "define 32-bit int type");

#include "h3r_os_error.h"
H3R_ERR_DEFINE_UNHANDLED
H3R_ERR_DEFINE_HANDLER(Memory,H3R_ERR_HANDLER_UNHANDLED)
H3R_ERR_DEFINE_HANDLER(Log,H3R_ERR_HANDLER_UNHANDLED)

#include "h3r_filestream.h"
#include "h3r_game.h"

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

static void store_as_bmp(char * fn,
    const H3R_NS::byte * b, int w, int h, int bpp,
    const H3R_NS::byte * p = nullptr);

// looks odd: Data_H3bitmap_lod/CAMPNOSC.PCX
int main(int argc, char ** argv)
{
    if (2 != argc)
        return printf ("usage: parse_pcx deffile\n");

    H3R_NS::Game game;
    game.SilentLog (true);

    H3R_NS::OS::FileStream s {argv[1], H3R_NS::OS::FileStream::Mode::ReadOnly};
    H3R_NS::OS::Log_stdout (EOL "%s 1st offset: %8X" EOL, argv[1], s.Tell ());

    // these are 4809 (H3bitmap.lod) + 2406 (H3ab_bmp.lod)
    // 1077 distinct combos of w, h, and bpp
    // select distinct order by count:
    // 52         25 :   48
    // 27         19 :   51
    //  8 126x122x 8 :   97
    //  7 150x164x 8 :  102
    //  6 276x186x 8 :  102
    //  5  28x 60x 8 :  102
    //  4  78x 94x 8 :  102
    //  3 400x232x24 :  158
    //  2  48x 32x 8 :  342
    //  1  58x 64x 8 : 1251
    int size, w, h, fmt;
    Read (s, &size).Read (s, &w).Read (s, &h);
    H3R_ENSURE(0 != w, "width of 0 is a no no")
    H3R_ENSURE(0 != h, "height of 0 is a no no")
    fmt = size / (w * h);
    H3R_NS::OS::Log_stdout (
        "Size  : %5d bytes" EOL "Dim : %dx%dx%d" EOL, size, w, h, fmt << 3);
    if (1 == fmt) {
        H3R_NS::Array<H3R_NS::byte> b {(size_t)(w * h)}, p {(size_t)(3 * 256)};
        // s.Seek (w * h); // bitmap
        s.Read ((H3R_NS::byte *)b, b.Length ());
        // s.Seek (3*256); // palette
        s.Read ((H3R_NS::byte *)p, p.Length ());
        store_as_bmp (argv[1], b, w, h, 8, p);
    }
    else if (3 == fmt) {
        H3R_NS::Array<H3R_NS::byte> b {(size_t)(w * h * 3)};
        // s.Seek (w * h * 3); // bitmap
        s.Read ((H3R_NS::byte *)b, b.Length ());
        store_as_bmp (argv[1], b, w, h, 24);
    } else H3R_ENSURE (false, "unknown PCX format")

    if (s.Tell () != s.Size ())
        H3R_NS::OS::Log_stdout ("unparsed %8X/%8X" EOL, s.Tell (), s.Size ());

    return 0;
} // main()

#include <ctype.h>

// b - bitmap, w - width, h - height, bpp - bits per pixel, p - RGB palette
void store_as_bmp(char * fn,
    const H3R_NS::byte * b, int w, int h, int bpp, const H3R_NS::byte * p)
{
    int bppl {(4 - (w % 4)) & 3}; // bonus pixels per line
                                  // "BMP" format requirement
    int actual_w {w + bppl}, bytes_per_pixel {bpp >> 3};

    H3R_NS::byte h8[] = {
        0x42, 0x4d,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0x28, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        1, 0,
        0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0x13, 0x0b, 0, 0,
        0x13, 0x0b, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    };
    const int FSIZE {2}, BMPOFS {10}, W {18}, H {22}, BPP {28}, BMPSIZE {34},
        COLORS {46}, HSIZE {sizeof(h8)};
    H3R_NS::OS::Memmove (h8 + W, &w, 4); //*(int *)(h8 + W) = w;
    H3R_NS::OS::Memmove (h8 + H, &h, 4); //*(int *)(h8 + H) = h;
    *(h8 + BPP) = bpp;
    int bmp_size = actual_w * h * bytes_per_pixel;
    H3R_NS::OS::Memmove (h8 + BMPSIZE, &bmp_size, 4);
    // *(int *)(h8 + BMPSIZE) = w * h * (bpp >> 8);
    H3R_NS::Array<H3R_NS::byte> pal;
    if (8 == bpp) {
        for (int i = 0; i < 256; i++) {
            // H3R_NS::byte rgba[] = {p[3*i], p[3*i+1], p[3*i+2], 0};
            // the source should be RGB; e.g. the BMP requires BGR
            H3R_NS::byte rgba[] = {p[3*i+2], p[3*i+1], p[3*i], 0};
            pal.Append (rgba, 4);
        }
        *(h8 + COLORS + 1) = *(h8 + COLORS + 5) = 1;
    }
    else if (24 == bpp)
        // for (int i = 0; i < (bmp_size - 3); i += 3) // RGB -> BGR or BGR -> RGB
        //    { var t = *(b+i); *(b+i) = *(b+i+2), *(b+i+2) = t; }
        ;
    else H3R_ENSURE(false, "unknown bpp")
    int bmpofs = HSIZE + pal.Length (), fsize = bmpofs + bmp_size;
    H3R_NS::OS::Memmove (h8 + FSIZE, &fsize, 4);
    H3R_NS::OS::Memmove (h8 + BMPOFS, &bmpofs, 4);
    // *(int *)(h8 + FSIZE) = 54 + pal.Length () + bmp_size;
    // *(int *)(h8 + BMP) = 54 + pal.Length ();

    H3R_NS::OS::Log_stdout ("old fname: %s" EOL, fn);
    var len = H3R_NS::OS::Strlen (fn);
    if (len > 4) fn[len-3] = isupper (fn[len-3]) ? 'B' : 'b',
                 fn[len-2] = isupper (fn[len-2]) ? 'M' : 'm',
                 fn[len-1] = isupper (fn[len-1]) ? 'P' : 'p',
                 fn[len-4] = '.';
    else H3R_ENSURE (false, "unsuported filename")
    H3R_NS::OS::Log_stdout ("new fname: %s" EOL, fn);
    H3R_NS::OS::FileStream s {fn,
        H3R_NS::OS::FileStream::Mode::WriteOnly};
    s.Write (h8, HSIZE);
    if (pal.Length () > 0) s.Write (pal.Data (), pal.Length ());
    // no :) s.Write (b, bmp_size); nothing is simple with these people
    H3R_NS::Array<H3R_NS::byte> zeroes {(size_t)(bppl * bytes_per_pixel)};
    for (int i = h-1; i >= 0; i--) {
        s.Write (b + i * w * bytes_per_pixel, w * bytes_per_pixel);
        if (bppl > 0) s.Write (zeroes.Data (), zeroes.Length ());
    }
}