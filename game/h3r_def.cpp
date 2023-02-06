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

//TODONT optimize; these shall be called once per game; then cached by the
//       3D accelerator;

#include "h3r_def.h"
#include "h3r_filestream.h"

H3R_NAMESPACE

// #include "store_as_bmp.cpp"

static void read_sprite_224(Stream & s, int w, int h, H3R_NS::byte * p)
{
    for (int i = 0; i < h; i++) // for each line
        for (int llen = 0; llen < w; ) {
            byte b, len;
            Stream::Read (s, &b);
            len = (b & 31) + 1;
            if ((224 & b) == 224) s.Read (p + i * w + llen, len);
            else for (int j = 0; j < len; j++) *(p + i * w + llen + j) = b>>5;
            llen += len;
        }
}

static void read_sprite(Stream & s, Array<byte> & buf, SubSpriteHeader & sh)
{
    Stream::Read (s, &sh);
    buf.Resize (sh.Width*sh.Height);
    byte * ptr = buf;
    // The "if" operators are ordered by frequency.
    if (1 == sh.Type) {
        // an offset table based on its own beginning:
        // Array<unsigned int> off_tbl {sh.Height};
        // auto off_table_base = s.Tell ();
        s.Seek (sh.Height * sizeof (unsigned int));
        // Stream::Read (s, (unsigned int *)off_tbl, off_tbl.Length ());
        for (int i = 0; i < sh.Height; i++)
            // s.Seek (off_table_base - s.Tell ()).Seek (off_tbl[i]);
            for (int llen = 0; llen < sh.Width; ) {
                byte b, len;
                Stream::Read (s, &b).Read (s, &len);
                if (255 == b) s.Read (ptr + i * sh.Width + llen, len + 1);
                else for (int j = 0; j < (len + 1); j++)
                    *(ptr + i * sh.Width + llen + j) = b;
                llen += (len + 1);
            }
    }// 1 == sh.Type
    else if (3 == sh.Type) {
        // int ofs_num = sh.Width >> 5;
        // Array<unsigned short> off_tbl {ofs_num * sh.Height};
        read_sprite_224 (s, sh.Width, sh.Height, ptr);
    }
    else if (2 == sh.Type) {
        unsigned short unk_len;
        Stream::Read (s, &unk_len);
        H3R_ENSURE(unk_len >= 2, "SeekBack not supported")
        s.Seek (unk_len - 2); // unknown data
        read_sprite_224 (s, sh.Width, sh.Height, ptr);
    }
    else if (0 == sh.Type)
        for (int i = 0; i < sh.Height; i++)
            s.Read (ptr + i * sh.Width, sh.Width);
    // a bug here; or corrupted data
    //TODO Handle gracefully
    else H3R_NOT_SUPPORTED_EXC("read_sprite: unknown type");
}// read_sprite()

void Def::Init()
{
    if (nullptr == _s) return;
    if (! *_s) return;
    auto & s = _s->Reset ();

    int type, cnt; // type, width, height, count
    Stream::Read (s, &type).Read (s, &_w).Read (s, &_h).Read (s, &cnt);

    //TODO validate
    _palette.Resize (3*256); // RGB
    Stream::Read (s, static_cast<byte *>(_palette), _palette.Length ());

    _sprites.Resize (cnt);
    for (int i = 0; i < cnt; i++) {
        _sprites[i].Read (s);
        _n += _sprites[i].Entries.Length ();
    }

    //LATER There are sprites afterwards. Just have to parse the last
    //      SubSprite in order to get the startup offset.
    //      See parse_def.cpp for more info.
    //      Cleanup the game archives of these not-referenced sprites. Test
    //      how the original game shall react.
    // SubSprite & _last_sprite =
    //    _sprites[cnt-1].Entries[_sprites[cnt-1].Entries.Length ()-1];
}

Array<byte> * Def::Decode(Array<byte> & buf, int u8_num)
{
    static SubSpriteHeader sh {};
    H3R_ENSURE(3 == u8_num || 4 == u8_num, "Can't help you")
    if (nullptr == _s) return &buf;
    if (! *_s) return  &buf;

    // Unfortunately Open GL isn't happy with these random widths.
    // Trying (4 - (bpl % 4)) & 3 ...
    //TODO do the same for the Pcx decoder; or just create the tex. atlases
    int bpl = _w * u8_num;
    int pitch = bpl + ((4 - (bpl % 4)) & 3);

    if (_request) {
        // Unfortunately I can't do that with zipstreams yet.
        // _s->Seek(_request.Offset - _s->Tell ())
        // but I can do this:
        { _s->Reset (); _s->Seek (_request->Offset); }
        Array<byte> indexed_bitmap {};
        read_sprite (*_s, indexed_bitmap, sh);
        H3R_ENSURE(sh.AnimWidth == _w, "leaf.w != tree.w")
        H3R_ENSURE(sh.AnimHeight == _h, "leaf.h != tree.h")
        printf ("DEF: %s: W:%d, H:%d, S: " EOL,
            _request->Name.AsZStr (), _w, _h);
        printf (
            "DEF.sh: %s: Type:%d, AW:%d, AH:%d W:%d, H:%d, T:%d, L:%d" EOL,
            _request->Name.AsZStr (), sh.Type, sh.AnimWidth, sh.AnimHeight,
            sh.Width, sh.Height, sh.Top, sh.Left);
        /*store_as_bmp (const_cast<char *>(_request->Name.AsZStr ()),
            static_cast<const byte *>(indexed_bitmap),
            sh.Width, sh.Height, 8, static_cast<const byte *>(_palette));*/
        // convert
        // A, if present, is 0; also, pal[0]={255, 255, 0} = A = 0.
        if (buf.Empty ()) buf.Resize (pitch*_h);
        byte * buf_ptr = buf;
        for (int y = 0; y < sh.Height; y++) {
            for (int x = 0; x < sh.Width; x++) {
                int i = indexed_bitmap[y*sh.Width + x];
                if (0 == i) continue; // no point; 0 is good enough mask
                int dy = y + sh.Top;
                int dx = x + sh.Left;
                byte * p = buf_ptr + dy*pitch + dx*u8_num;
                *(p+0) = *((byte *)_palette+3*i+0); // 2 BMP wants these swapped
                *(p+1) = *((byte *)_palette+3*i+1); // |
                *(p+2) = *((byte *)_palette+3*i+2); // 0 Open GL does not?!
                if (4 == u8_num) *(p+3) = 255; // opaque
            }
        }
        /*store_as_bmp (const_cast<char *>(_request->Name.AsZStr ()),
            buf_ptr, _w, _h, 24, nullptr);*/
        // Looks kind of messy using pointers?
    }
    return &buf;
}// Decode()

NAMESPACE_H3R