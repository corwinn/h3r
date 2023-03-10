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

// Not for the faint of heart. This is quick'n'dirty code for FF analysis.

//c clang++ -std=c++11 -DH3R_MM -fsanitize=address,undefined,integer,leak -fvisibility=hidden -I. -Ios -Ios/posix -Iutils -Istream -O0 -g -DH3R_DEBUG -fno-exceptions -fno-threadsafe-statics main.a parse_def.cpp -o parse_def

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

// a shortcut for H3R_NS::Stream::Read
template <typename T> static H3R_NS::Stream & Read(
    H3R_NS::Stream & s, T * d, size_t num = 1)
{
    return H3R_NS::Stream::Read (s, d, num);
}

#include "store_as_bmp.cpp"

static const int GH {9};
static const int GW {8};
static struct { // takes care of the render to atlas part
    const int _w {1024};
    int _x {0}, _y {-1}, _h {0};
    int _row_height, _column_width, _columns;
    // sprite-related state
    bool _bonus_sprite {};
    int _block {};
    int _sprite {};
    H3R_NS::Array<H3R_NS::byte> * _b {};
    H3R_NS::byte _digits[10*GH] = { // my font
     0x38, 0x44, 0x82, 0x82, 0x92, 0x82, 0x82, 0x44, 0x38,
     0x10, 0x30, 0x50, 0x90, 0x10, 0x10, 0x10, 0x10, 0xFE,
     0x38, 0x44, 0x82, 0x02, 0x04, 0x18, 0x20, 0x40, 0xFE,
     0x38, 0x44, 0x02, 0x04, 0x18, 0x04, 0x82, 0x44, 0x38,
     0x82, 0x82, 0x82, 0x82, 0xFE, 0x02, 0x02, 0x02, 0x02,
     0xFE, 0x80, 0x80, 0xB8, 0xC4, 0x02, 0x02, 0xC4, 0x38,
     0x3C, 0x42, 0x80, 0xB8, 0xC4, 0x82, 0x82, 0xC4, 0x38,
     0xFE, 0x42, 0x04, 0x04, 0x3C, 0x10, 0x10, 0x20, 0x40,
     0x38, 0x44, 0x44, 0x28, 0x38, 0x44, 0x82, 0x44, 0x38,
     0x38, 0x44, 0x82, 0x46, 0x3A, 0x02, 0x02, 0xC4, 0x38
   /*0011 1000
     0100 0100
     1000 0010
     1000 0010
     1001 0010
     1000 0010
     1000 0010
     0100 0100
     0011 1000*/
    };
    H3R_NS::byte _dash[GH] = {0, 0, 0, 0, 0x7C, 0, 0, 0, 0}; // my font
    H3R_NS::byte _semicolon[GH] = {0, 0, 16, 0, 0, 0, 16, 0, 0}; // my font
    inline void put_pixel(int x, int y, int cidx)
    {
        if (y >= 0 && y < _h && x >= 0 && x < _w) // clip when bitmap is smaller
            *((H3R_NS::byte *)*_b + y * _w + x) = cidx;
    }
    inline void RenderGlyph(const H3R_NS::byte * g, int x, int y, int cidx)
    {//TODONT optimize :)
        for (int i = 0; i < GH; i++)
            for (int b = 1, j = 0; b < 256; b <<= 1, j++)
                if (b & g[i]) put_pixel (x + (6 - j), y + i, cidx);
    }
    inline int RenderNum(int num, int x, int y, int cidx)
    {
        int r {0};
        if (num < 0)
            RenderGlyph (_dash, x, y, cidx), x += GW, r += GW, num = -num;
        if (num > -1 && num < 10)
            return RenderGlyph (&_digits[num*GH], x, y, cidx), r + GW;
        else
        for (int i = 1000000000; i > 0; i /= 10)
            if (num/i > 0)
                for (int j = i, k = 0; j > 0; j /= 10, k++)
                    RenderGlyph (&_digits[(num/j)*GH], x + GW * k, y, cidx),
                    num -= num/j*j, r += GW;
        return r;
    }
    void Init(int sprite_w, int sprite_h)
    {
        _column_width = sprite_w;
        _x = _columns = _w / sprite_w;
        _row_height = sprite_h;
    }
    void Add(const H3R_NS::byte * b, int w, int h, int l, int t)
    {
        if (_x >= _columns) {
            //DONE "Not allocated here" related to Array.Resize ()
            // perhaps the array gets released after the MM due to "bmp" being
            // static. Ensuring it gets released prior main() ends, resolves the
            // issue.
            // printf ("resize to: %lu" EOL, _b->Length () + _w * _row_height);
            _b->Resize (_b->Length () + _w * _row_height);
            _x = 0, _y++, _h += _row_height;
        }
        int left = _x * _column_width, top = _y * _row_height;
        // printf ("Add: _x: %d, _y: %d, _w: %d, _h: %d"
        //    ", left: %d, top: %d, w: %d, h: %d" EOL,
        //    _x, _y, _w, _h, left, top, w, h);
        for (int i = 0; i < h; i++)
            H3R_NS::OS::Memmove (
                (H3R_NS::byte *)*_b + ((top + i + t) * _w) + left + l,
                b + i * w, w);
        int tw = RenderNum (_block, left + 5, top + 5, 255);
        RenderGlyph (_semicolon, left + 5 + tw, top + 5, 255), tw += GW;
        RenderNum (_sprite, left + 5 + tw, top + 5, 255);
        if (_bonus_sprite)
            for (int i = 0; i < _column_width; i++)
                *((H3R_NS::byte *)*_b + (top * _w) + i + _x * _column_width)
                = 255;
        _x++;
    }
    void Write(char * fn, const H3R_NS::byte * p)
    {
        store_as_bmp (fn, _b->Data (), _w, _h, 8, p);
    }
} bmp; // yes - it is this simple

static void read_sprite(H3R_NS::Stream & s);

/* some are coupled with .msk file of size 14; they look like:
  1 w
  1 h
  6 bit mask - pass-ability mask (max size 8x6)
  6 bit mask - trigger(s) mask (activators)
  example: 03 02, 00 00 00 00 E0 E0, 00 00 00 00 00 E0
   has meaning when (the origin is bottom left):
    block:
     0000 0000
     0000 0000
     0000 0000
     0000 0000
     1110 0000
     1110 0000
    visit:
     0000 0000
     0000 0000
     0000 0000
     0000 0000
     0000 0000
     1110 0000

  See map_grammar.MapObj
*/
// Interesting one: Data_H3sprite_lod/SGTWMTB.def:
//                  (also, Data_H3sprite_lod/SGTWMTA.def)
//  Width: 320
//  Height: 320
//  but:name[0]: "SgTwMt01.pcx"
//      offset[0]:      3DB
//      size: 2992: type: 1, aw: 320, ah: 320, w: 1280, h: 1284, l: 1288, t: 1292
//  Thats invalid w,h,l,t for a sub-sprite?!
//  db 03 00 00 9b 0f 00 00
//  Difference between offsets: 16 bytes: ofs_n+1 - ofs_n
//  sub-sprite0: size: 0b b0 00 00, ofs: 3db
//  sub-sprite1: size: aa 11 00 00, ofs: f9b  | diff 16 bytes
//  sub-sprite2: size:            , ofs: 2155 | diff 16 bytes
//  Shift with -16 bytes at the header while w,h,l,t = aw,ah,0,0: ok.
// So why are 16 bytes of the header (each nth header (11 at SGTWMTB.def and 1
// at SGTWMTA.def)) missing? I could have some very specific bug at the lod
// unpacker, but still? TODO make a packer, then unpacked-packed-unpacked shall
// equal unpacked; then manually fix these two - packed-unpacked shall equal
// them.
int main(int argc, char ** argv)
{
    if (2 != argc)
        return printf ("usage: parse_def deffile\n");

    H3R_NS::Array<H3R_NS::byte> bmp_b {};
    bmp._b = &bmp_b;

    H3R_NS::OS::FileStream s {argv[1], H3R_NS::OS::FileStream::Mode::ReadOnly};
    printf (EOL "%s 1st offset: %8X" EOL, argv[1], (uint)s.Tell ());

    // named "Entry"?
    //TODO having two code-bases for the same thing is not ok.
    int type, w, h, bcnt; // bcnt - "group" count if you ask me
    Read (s, &type).Read (s, &w).Read (s, &h).Read (s, &bcnt);

    printf (
        "Type: %d" EOL "Width: %3d" EOL "Height: %3d" EOL "Block count: %d" EOL,
        type, w, h, bcnt);
    bmp.Init (w, h);

    H3R_NS::Array<H3R_NS::byte> palette {3*256}; // RGB
    Read (s, (H3R_NS::byte *)palette, palette.Length ());

    auto last_ofs = s.Tell ();
    int scnt = 0;
    for (int i = 0, ecnt = 0; i < bcnt; i++) { // entry count
        // i is group index?
        // named "EntryBlock" ?
        uint unk;
        Read (s, &unk); // this is some auto-inc number: a primary key perhaps?
        // it correlates to the unreferenced sprites (deleted perhaps? - the
        // eventual "primary key" has holes in it); its observable at sprites
        // with bcnt > 1
        printf ("Block[%2d] unk1: %000000008X" EOL, i, unk);
        int cnt2; // some other count?
        Read (s, &cnt2);
        printf ("Block[%2d] sprite count: %d" EOL, i, cnt2);

        // Not floating-point; not date or time (that I know of). Doesn't look
        // like a checksum.
        // Wizards, help.
        Read (s, &unk); // unknown2; its always bigger than unknown3
        printf ("Block[%2d] unk2: %000000008X" EOL, i, unk);
        Read (s, &unk); // unknown3; half of its bits == the same unknown2 ones,
                        //           but are distinct across all sprites
        printf ("Block[%2d] unk3: %000000008X" EOL, i, unk);

        // EntryBlock.Data
        for (int j = 0; j < cnt2; j++) {
            H3R_NS::Array<H3R_NS::byte> name {14}; // pcx
            Read (s, (H3R_NS::byte *)name, 13);
            printf (
                "  name[%2d]: \"%s\"" EOL, j, (char *)(H3R_NS::byte *)name);
        }
        int prev_ofs {};
        for (int j = 0; j < cnt2; j++) {
            int offset; // 0-based
            Read (s, &offset);
            printf ("  offset[%d]: %8X, ofs-prev.ofs (ordering): %d" EOL,
                j, offset, offset-prev_ofs); prev_ofs = offset;
            auto sentinel = s.Tell ();
            // so its not an entry - its a sprite
            bmp._block = i, bmp._sprite = j;
            read_sprite (s.Seek (offset - s.Tell ())); scnt++;
            last_ofs = s.Tell ();
            s.Seek (sentinel - s.Tell ());
        }
        //TODO the above is an odd way to store data - figure out why
    } // for (int i = 0, ecnt = 0; i < bcnt; i++)
    printf (" offset after all blocks: %8X" EOL, (uint)last_ofs);
#if LIST_ONLY
    return 0;
#endif
    // There are a lot of .def files containing series of name-less sprites
    // after the "main" data:
    //   Data_H3ab_spr_lod/AH17_.DEF   - 32 of them; 45 named in 10 blocks
    //   Data_H3ab_spr_lod/RanNum3.def -  2 of them;  4 named in  1 block
    //   95 more from at "H3ab_spr.lod"
    //   582 at "H3sprite.lod"
    // Their size looks consistent with the .def header.
    // There is no more extra data after these unreferenced? sprites.
    int k = 0;
    if (last_ofs != s.Size ()) {
        bmp._bonus_sprite = true;
        bmp._block = bmp._sprite = -1;
        // printf ("unparsed" EOL);
        auto sz = s.Size (); // just in case
        s.Seek (last_ofs - s.Tell ());
        while (s.Tell () < sz) {
            printf ("Bonus sprite #%d, ofs: %8X" EOL, ++k, (uint)s.Tell ());
            ++bmp._sprite, read_sprite (s);
            last_ofs = s.Tell ();
        }
    }
    if (last_ofs != s.Size ())
        printf (
            "unparsed2: pos: %8X, size: %8X, bonus sprites: %d" EOL,
            (uint)last_ofs, (uint)s.Size (), k);
    else printf ("sprites: %d, bonus sprites: %d" EOL,
        scnt, k);

    bmp.Write (argv[1], palette.Data ());
    return 0;
} // main()

// 224 is a key of one of the original game encodings
static void read_sprite_224(H3R_NS::Stream & s, int w, int h, H3R_NS::byte * p)
{
    for (int i = 0; i < h; i++) { // for each line
        // printf ("Line #%2d" EOL, i);
        for (int llen = 0; llen < w; ) {
            H3R_NS::byte b, len;
            // printf ("  ofs:  %8X:", s.Tell ());
            Read (s, &b);
            len = (b & 31) + 1;
            // printf (" b: %2X, v: %3d, len: %3d, llen: %3d"
            //    EOL, b, b>>5, len, llen + len);
            if ((224 & b) == 224) s.Read (p + i * w + llen, len);
            else for (int j = 0; j < len; j++) *(p + i * w + llen + j) = b>>5;
            // if ((224 & b) == 224) s.Seek (len); // copy "len" bytes
            // else ; // "b>>5" repeats "len" times
            llen += len;
        }
    }
    // printf (
    //    " offset after 1st sprite: %8X" EOL, s.Tell ());
    // H3R_NS::OS::Exit (1);
}

// type 1: ../h3r_unpacked/h3/Data_H3ab_spr_lod/Cnomad.def
// type 3: ../h3r_unpacked/h3/Data_H3ab_spr_lod/AH17_.DEF
// type 0: ../h3r_unpacked/h3/Data_H3sprite_lod/Lavatl.def
// type 2: ../h3r_unpacked/h3/Data_H3sprite_lod/Clrrvr.def
void read_sprite(H3R_NS::Stream & s)
{
    // - left and top are offsets (translation transform) inside aw ah:
    //     aw = l + w; ah = t + h; why are they stored in each sprite?!
    // - aw and ah are the size of the animation
    // - size: sprite size w/o this "header"
    int size, type, aw, ah, w, h, l, t;

    Read (s, &size).Read (s, &type).Read (s, &aw).Read (s, &ah).
    Read (s, &w).Read (s, &h).Read (s, &l).Read (s, &t);

    // Data_H3sprite_lod/SGTWMTA.def && Data_H3sprite_lod/SGTWMTB.def
    // Their "size" correctly reflects the missing fields, but the sprites
    // aren't sorted: neither by name, nor by offset: there is no simple way
    // to detect this half-header.
    if (w > aw && h > ah && l > aw && t > ah)
        s.Seek (-16), w = aw, h = ah, l = 0, t = 0;

    printf (
        "   size: %4d: type: %1d, aw: %3d, ah: %3d, w: %3d, h: %3d, l: %2d,"
        " t: %2d" EOL, size, type, aw, ah, w, h, l, t);

#if LIST_ONLY
    return;
#endif
    // type1: 32524 sprites
    // type3: 15281 sprites
    // type0:   835 sprites
    // type2:   187 sprites
    H3R_NS::Array<H3R_NS::byte> bitmap {w * h};
    H3R_NS::byte * ptr = (H3R_NS::byte *)bitmap;

    if (1 == type) {
        // an offset table based on its own beginning:
        H3R_NS::Array<unsigned int> off_tbl {h};
        auto off_table_base = s.Tell ();
        Read (s, (unsigned int *)off_tbl, off_tbl.Length ());
        for (int i = 0; i < h; i++) {
            // printf ("Line #%2d" EOL, i);
            // s.Seek (off_table_base - s.Tell ()).Seek (off_tbl[i]);
            for (int llen = 0; llen < w; ) {
                // printf ("  ofs:  %8X:", s.Tell ());
                H3R_NS::byte b, len;
                Read (s, &b).Read (s, &len);
                // printf (" b: %2X, len: %3d, llen: %3d"
                //    EOL, b, len + 1, llen + len + 1);
                if (255 == b) s.Read (ptr + i * w + llen, len + 1);
                else for (int j = 0; j < (len + 1); j++)
                    *(ptr + i * w + llen + j) = b;
                // if (255 == b) s.Seek (len + 1); // copy "len + 1" bytes
                // else ; // "b" repeats "len + 1" times
                llen += (len + 1);
            }
        }
    } // if (1 == type)
    else if (3 == type) {
        // an offset table based on its own beginning:
        int ofs_num = w >> 5;
        H3R_NS::Array<unsigned short> off_tbl {ofs_num * h};
        auto off_table_base = s.Tell ();
        Read (s, (unsigned short *)off_tbl, off_tbl.Length ());
        H3R_ENSURE(s.Tell () == (off_table_base + off_tbl[0]),
            "sprite type 3: the 1st offset doesn't match table length")
        read_sprite_224 (s, w, h, ptr);
        /*for (int i = 0; i < h; i++) { // for each line
            // printf ("Line #%2d" EOL, i);
            // s.Seek (off_table_base - s.Tell ()).Seek (off_tbl[ofs_num*i]);
            // only the 1st offset is being used?!
            // off_tbl[3*i+0] - the 1st enc. byte of the current line
            // off_tbl[3*i+1] - the 1st visible pixel? nope; the next one s/-/+
            // off_tbl[3*i+2] - the last enc. byte of the current line, mostly
            //                  at many instance it refers to last-1; sometimes
            //                  to last-2, -3, -4
            // There doesn't seem to be any point to these offsets, other than
            // data integrity check?
            H3R_ENSURE(off_table_base + off_tbl[ofs_num*i] == s.Tell (),
                "sprite type 3: the offset table is useful after all")

            //TODO these shouldn't be decoded here; sprite encoding is being
            // done to save some RAM, and the sprites are being decoded right
            // prior drawing. Means all of these are not indexed in real-time,
            // but stored into RAM; that's 17 941 866 bytes (including headers)
            // for H3ab_spr - 347 entries, and 100 610 248 bytes (ditto) for
            // H3Sprite - 2565 entries; given that each map contains a distinct
            // sub-set of these: optimal RAM usage well done.

            // decode 1 line:
            //  0xe0 indicates byte sequence of length [1;32] (bits [0;4])
            //  everything else indicates repetition:
            //   bits [5;7] - what repeats
            //   bits [0;4] - length
            for (int llen = 0; llen < w; ) {
                H3R_NS::byte b, len;
                //int ofs_id = 0;
                //for (int k = 0; k < ofs_num; k++)
                //if (off_table_base + off_tbl[ofs_num*i + k] == s.Tell ())
                //   { ofs_id = k + 1; break; }
                //if (ofs_id)
                //    printf ("  ofs: %d%8X:", ofs_id, s.Tell ());
                //else printf ("  ofs:  %8X:", s.Tell ());
                Read (s, &b);
                len = (b & 31) + 1;
                //printf (" b: %2X, v: %3d, len: %3d, llen: %3d"
                //    EOL, b, b>>5, len, llen + len);
                if ((224 & b) == 224) s.Seek (len); // copy "len" bytes
                else ; // "b>>5" repeats "len" times
                llen += len;
            }
        } // for (int i = 0; i < h; i++)
            // printf (
            //    " offset after 1st sprite: %8X" EOL, s.Tell ());
            // H3R_NS::OS::Exit (1);*/
    } // if (3 == ftype)
    else if (2 == type) {
        unsigned short unk_len;
        Read (s, &unk_len);
        s.Seek (unk_len - 2); // unknown data
        read_sprite_224 (s, w, h, ptr);
    } // if (2 == type)
    else if (0 == type) {
        for (int i = 0; i < h; i++)
            s.Read (ptr + i * w, w);
            //s.Seek (w); // read "w" pixels
    } // if (0 == type)
    else {
        printf ("Unknown type%d " EOL, type);
        H3R_NS::OS::Exit (1);
    }

    bmp.Add (ptr, w, h, l, t);
} // read_sprite()