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

//c clang++ -std=c++11 -DH3R_MM -fsanitize=address,undefined,integer,leak -fvisibility=hidden -I. -Ios -Ios/posix -Iutils -Istream -O0 -g -DH3R_DEBUG -fno-exceptions -fno-threadsafe-statics main.a parse_fnt.cpp -o parse_fnt

#define int int
static_assert(4 == sizeof(int), "define 32-bit int type");

#include "h3r_os_error.h"
H3R_ERR_DEFINE_UNHANDLED
H3R_ERR_DEFINE_HANDLER(Memory,H3R_ERR_HANDLER_UNHANDLED)

#include "h3r_filestream.h"
#include "h3r_array.h"

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

#include "store_as_bmp.cpp"

// H3bitmap_lod
// CALLI10R.FNT
// CREDITS.FNT
// HISCORE.FNT
// MedFont.fnt
// TIMES08R.FNT
// bigfont.fnt
// smalfont.fnt
// tiny.fnt
// verd10b.fnt
//
// A bitmap font of 256 glyphs; each glyph: 8-bit bitmap, with implicit palette;
// Some have pre-smoothing encoded as 1. bearingY is pre-computed.
// Format:
// glyph_entry
//    4 offset1 // [pixels] - no idea what those are but the game is using them
//    4 width   // [bytes]
//    4 offset2 // [pixels] - no idea what those are but the game is using them
//
// fnt
//     5 unknown
//     1 glyph_height
//    26 unknown
//    12 glyph_entry[256]
//     4 glyph_bitmap_offset[256]
//   256 glyph_bitmaps[glyph_entry[].width*fnt.glyph_height]
int main(int argc, char ** argv)
{
    if (2 != argc)
        return printf ("usage: parse_fnt fntfile\n");

    H3R_NS::OS::FileStream s {argv[1], H3R_NS::OS::FileStream::Mode::ReadOnly};
    printf (EOL "%s 1st offset: %000000008X" EOL, argv[1], (uint)s.Tell ());

    s.Seek (5); // unknown 5 bytes
    H3R_NS::byte h;
    H3R_NS::Stream::Read (s, &h);
    printf ("Font height: %d" EOL, h);
    s.Seek (26); // unknown 26 bytes
    int bearing_x_buf, width_buf, offset2_buf, bitmap_ofs, total_width {};
    // A char[065]:  0,  6,  1, 00000625
    // V char[086]: -1,  6,  0, 00000B00
    // AV and VA have both negative kerning; how does it render these two
    // combos: the game has no kerning.
    // 0 char[048]:  0,  4,  1, 00000323
    // 1 char[049]:  1,  3,  1, 0000034F
    // 2 char[050]:  0,  4,  1, 00000370
    // 4 char[052]:  0,  5,  0, 000003C8
    // Observations from the game (tiny.fnt is being used under the army stacks
    // at the map view it seems):
    //    10 - has 1 empty (game)
    //   101 - has 1 2 empty
    //   411 - has 1 2 empty
    //    40 - no empty
    //   104 - has 1 1
    //    12 - has 1 empty (game)
    //    44 - no empty
    //   100 - 1 1
    //   410 - 1 1
    // The game doesn't do shadow or smoothing of "tiny.fnt".
    H3R_NS::Array<int> bearing_x {256}; // not a bearingx per se but some offset
    H3R_NS::Array<int> width {256}; // the glyph width (width*height bytes)
    H3R_NS::Array<int> offset2 {256}; // just another offset used by the game
    for (int i = 0; i < 256; i++) {
        H3R_NS::Stream::Read (s, &bearing_x_buf).Read (s, &width_buf)
            .Read (s, &offset2_buf);
        bearing_x[i] = bearing_x_buf;
        width[i] = width_buf;
        offset2[i] = offset2_buf;
        //DONE check with FreeType.
        total_width += width_buf + bearing_x_buf + offset2_buf;
    }

    H3R_NS::Array<H3R_NS::byte> atlas {total_width*3*h};
    H3R_NS::byte * atlas_ptr = atlas;
    int atlas_ptr_w {};
    for (int i = 0; i < 256; i++) {
        H3R_NS::Stream::Read (s, &bitmap_ofs);
        printf ("char[%0003d]: %2d, %2d, %2d, %000000008X" EOL,
            i, bearing_x[i], width[i], offset2[i], bitmap_ofs);
        // printf ("char[%0003d]: offset: %000000008X" EOL,
        //   i, bitmap_ofs); // (32+256*4*4) - based; (header + entry_table)
        if (width[i] <= 0) continue;
        auto sentinel = s.Tell ();
        s.Seek ((bitmap_ofs + (32+256*4*4)) - s.Tell ());

        H3R_NS::Array<H3R_NS::byte> glyph {width[i]*h};
        H3R_NS::byte * glyph_buf = glyph;
        s.Read (glyph_buf, width[i]*h);
        int advance = bearing_x[i] + width[i] + offset2[i];
        atlas_ptr += bearing_x[i]*3;
        // printf ("Read %d %d glyph" EOL, advance[i], h);
        for (int r = 0 ; r < h; r++) {
            // printf ("row%002d: ", r);
            for (int c = 0 ; c < width[i]; c++) {
                auto color = glyph_buf[r*width[i]+c];
                // printf ("%3d ", color);
                if (color != 1 && color != 0 && color != 255)
                    printf ("Unahdled color: %3d " EOL, color);
                color = 1 == color ? 128 : 255 == color ? 255 : 0;
                atlas_ptr[0] = atlas_ptr[1] = atlas_ptr[2] = color;
                atlas_ptr += 3;
            }
            // printf (EOL);
            atlas_ptr += (total_width-width[i])*3;
        }
        atlas_ptr_w += advance;
        atlas_ptr = atlas;
        atlas_ptr += atlas_ptr_w * 3;
        if (i != 255) s.Seek (sentinel - s.Tell ());
    }
    printf (EOL "%s last offset: %000000008X" EOL, argv[1], (uint)s.Tell ());

    store_as_bmp (argv[1], atlas, total_width, h, 24, nullptr);

    printf ("unparsed %000000008X/%000000008X" EOL,
            (uint)(s.Size () - s.Tell ()), (uint)s.Size ());

    return 0;
} // main()
