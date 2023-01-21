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

#ifndef _H3R_ZIPINFLATESTREAM_H_
#define _H3R_ZIPINFLATESTREAM_H_

#include "h3r_stream.h"
#include <zlib.h>

H3R_NAMESPACE

// A stream for reading zip-encoded data. You get NotSupportedException on
// Write() and Seek().
// It allocates _IN_BUF bytes buffer (4k) so be wary.
class ZipInflateStream : public Stream
{
    H3R_CANT_COPY(ZipInflateStream)
    H3R_CANT_MOVE(ZipInflateStream)

#define public public:
#define private private:
    private z_stream _zs {};
    private int _zr {~Z_OK}, _size, _usize;
    private const off_t _pos_sentinel; // for ResetTo()
    private static uInt constexpr _IN_BUF {1<<12}; // zlib: uInt
    private byte _buf[_IN_BUF] {};
    public ZipInflateStream(Stream * s, int size, int usize)
        : Stream {s}, _size{size}, _usize{usize}, _pos_sentinel{s->Tell ()}
    {
        inflateInit (&_zs);
    }
    public ~ZipInflateStream() override { inflateEnd (&_zs); }
    public inline operator bool() override { return Z_OK == _zr; }
    public Stream & Seek(off_t) override;
    // You can use this for progress: 1.0 * Tell() / Size() * 100
    public off_t Tell() const override; // compressed
    public off_t Size() const override; // uncompressed
    public Stream & Read(void *, size_t) override;
    public Stream & Write(const void *, size_t) override;

    // same meaning as constructor parameters
    public Stream & ResetTo(int size, int usize);
#undef public
#undef private
};

NAMESPACE_H3R

#endif