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

#include "h3r_zipinflatestream.h"

H3R_NAMESPACE

Stream & ZipInflateStream::Seek(off_t)
{
    H3R_ENSURE(false, "ZipInflateStream: Seek is not supported.")
}

off_t ZipInflateStream::Tell() { return Stream::Tell () - _zs.avail_in; }

off_t ZipInflateStream::Size() { return _usize; }

Stream & ZipInflateStream::Read(void * buf, size_t bytes)
{
    /*OS::Log_stdout ("ZipInflateStream::Read %zu bytes" EOL, bytes);*/
    H3R_ARG_EXC_IF(bytes <= 0, "bytes can't be <= 0")
    H3R_ARG_EXC_IF(nullptr == buf, "buf can't be null")
    // zlib: uInt
    // The sheer elegance of z_stream is impressive. Well NiceMountainView done.
    _zs.avail_out = static_cast<uInt>(bytes);
    _zs.next_out = static_cast<z_const Bytef *>(buf);

    while (_zs.avail_out > 0) {
        if (_zs.avail_in <= 0) {
            _zs.avail_in =
                static_cast<uInt>(_size - (Stream::Tell () - _pos_sentinel));
            H3R_ENSURE(_zs.avail_in > 0, "ZipInflateStream::Read no more input")
            if (_zs.avail_in > _IN_BUF) _zs.avail_in = _IN_BUF;
            Stream::Read (_buf, _zs.avail_in);
            _zs.next_in = static_cast<z_const Bytef *>(_buf);
        }
        /*OS::Log_stdout (
            "ZipInflateStream::Read prior inflate in:%zu, out:%zu" EOL,
            _zs.avail_in, _zs.avail_out);*/
        var sentinel1 = _zs.avail_in;
        var sentinel2 = _zs.avail_out;
        _zr = inflate (&_zs, Z_SYNC_FLUSH);
        /*OS::Log_stdout (
            "ZipInflateStream::Read after inflate in:%zu, out:%zu" EOL,
            _zs.avail_in, _zs.avail_out);*/
        if (Z_STREAM_END == _zr) _zr = Z_OK;
        H3R_ENSURE(Z_OK == _zr, "ZipInflateStream::Read error")
        H3R_ENSURE(sentinel1 != _zs.avail_in || sentinel2 != _zs.avail_out,
            "ZipInflateStream::Read infinite loop case")
    }
    return  *this;
}// Read()

Stream & ZipInflateStream::Write(const void *, size_t)
{
    H3R_ENSURE(false, "ZipInflateStream: Write is not supported.")
}

// Reset to state
Stream & ZipInflateStream::ResetTo(int size, int usize)
{
    inflateReset (&_zs); _zr = Z_OK;
    _size = {size}, _usize = {usize};
    /*OS::Log_stdout ("ZipInflateStream::ResetTo size:%zu, usize:%zu" EOL,
        _size, _usize);*/
    Stream::Seek (_pos_sentinel - Stream::Tell ());
    return *this;
}

NAMESPACE_H3R