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

#include <ctype.h>
// w:173; h:111 bpp:24; bppl = 3; actual_w = 176; bytes_per_pixel = 3
// b - bitmap, w - width, h - height, bpp - bits per pixel, p - RGB palette
static void store_as_bmp(char * fn,
    const H3R_NS::byte * b, int w, int h, int bpp, const H3R_NS::byte * p)
{
    int bytes_per_pixel {bpp >> 3};
    int bpl = w *  bytes_per_pixel;
    int bbpl {(4 - (bpl % 4)) & 3}; // bonus bytes per line
                                    // "BMP" format requirement
    H3R_NS::byte h8[] = {
        0x42, 0x4d,      //  0
        0, 0, 0, 0,      //  2
        0, 0, 0, 0,      //  6
        0, 0, 0, 0,      // 10
        0x28, 0, 0, 0,   // 14
        0, 0, 0, 0,      // 18
        0, 0, 0, 0,      // 22
        1, 0,            // 26
        0, 0,            // 28
        0, 0, 0, 0,      // 30
        0, 0, 0, 0,      // 34
        0x13, 0x0b, 0, 0,// 38
        0x13, 0x0b, 0, 0,// 42
        0, 0, 0, 0,      // 46
        0, 0, 0, 0       // 50
    };
    const int FSIZE {2}, BMPOFS {10}, W {18}, H {22}, BPP {28}, BMPSIZE {34},
        COLORS {46}, HSIZE {sizeof(h8)};
    H3R_NS::OS::Memmove (h8 + W, &w, 4); // *(int *)(h8 + W) = w;
    H3R_NS::OS::Memmove (h8 + H, &h, 4); // *(int *)(h8 + H) = h;
    H3R_NS::OS::Memmove (h8 + BPP, &bpp, 2);
    int bmp_size = (w * bytes_per_pixel + bbpl) * h;
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
        //    { auto t = *(b+i); *(b+i) = *(b+i+2), *(b+i+2) = t; }
        ;
    else H3R_ENSURE(false, "unknown bpp")
    int bmpofs = HSIZE + pal.Length (), fsize = bmpofs + bmp_size;
    H3R_NS::OS::Memmove (h8 + FSIZE, &fsize, 4);
    H3R_NS::OS::Memmove (h8 + BMPOFS, &bmpofs, 4);
    // *(int *)(h8 + FSIZE) = 54 + pal.Length () + bmp_size;
    // *(int *)(h8 + BMP) = 54 + pal.Length ();

    H3R_NS::OS::Log_stdout ("old fname: %s" EOL, fn);
    auto len = H3R_NS::OS::Strlen (fn);
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
    H3R_NS::Array<H3R_NS::byte> zeroes {(size_t)(bbpl)};
    for (int i = h-1; i >= 0; i--) {
        s.Write (b + i * w * bytes_per_pixel, w * bytes_per_pixel);
        if (bbpl > 0) s.Write (zeroes.Data (), zeroes.Length ());
    }
}
