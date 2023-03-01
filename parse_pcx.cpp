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

//c clang++ -std=c++11 -DH3R_MM -fsanitize=address,undefined,integer,leak -fvisibility=hidden -I. -Ios -Ios/posix -Iutils -Istream -O0 -g -DH3R_DEBUG -fno-exceptions -fno-threadsafe-statics main.a parse_pcx.cpp -o parse_pcx

#define int int
static_assert(4 == sizeof(int), "define 32-bit int type");

#include "h3r_os_error.h"
H3R_ERR_DEFINE_UNHANDLED
H3R_ERR_DEFINE_HANDLER(Memory,H3R_ERR_HANDLER_UNHANDLED)

#include "h3r_filestream.h"

H3R_NAMESPACE
namespace OS {
#undef public
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

// Looks fine: Data_H3bitmap_lod/CAMPNOSC.PCX ; the isse was at the bmp store
// proc. Resolved a few days ago: I thought they aligned pixels to 4 per row;
// and it was bytes.
int main(int argc, char ** argv)
{
    if (2 != argc)
        return printf ("usage: parse_pcx pcxfile\n");

    H3R_NS::OS::FileStream s {argv[1], H3R_NS::OS::FileStream::Mode::ReadOnly};
    printf (EOL "%s 1st offset: %8X" EOL, argv[1], (uint)s.Tell ());

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
    printf (
        "Size  : %5d bytes" EOL "Dim : %dx%dx%d" EOL, size, w, h, fmt << 3);
    if (1 == fmt) {
        H3R_NS::Array<H3R_NS::byte> b {w * h}, p {3 * 256};
        // s.Seek (w * h); // bitmap
        s.Read ((H3R_NS::byte *)b, b.Length ());
        // s.Seek (3*256); // palette
        s.Read ((H3R_NS::byte *)p, p.Length ());
        store_as_bmp (argv[1], b, w, h, 8, p);
    }
    else if (3 == fmt) {
        H3R_NS::Array<H3R_NS::byte> b {w * h * 3};
        // s.Seek (w * h * 3); // bitmap
        s.Read ((H3R_NS::byte *)b, b.Length ());
        store_as_bmp (argv[1], b, w, h, 24, nullptr);
    } else H3R_ENSURE (false, "unknown PCX format")

    if (s.Tell () != s.Size ())
        printf ("unparsed %8X/%8X" EOL, (uint)s.Tell (), (uint)s.Size ());

    return 0;
} // main()
