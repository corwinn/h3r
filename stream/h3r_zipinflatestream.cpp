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

// It can actually "seek" forwards.
//TODO StreamCaps: CanSeekFwd, CanSeekBack, CanTell, etc.
Stream & ZipInflateStream::Seek(off_t offset)
{
    if (0 == offset) return * this;
    if (offset < 0) H3R_NOT_SUPPORTED_EXC("Backwards seek is not supported.")
    //LATER Skip() perhaps this shall become Stream method?
    byte b;
    while (offset--) Read (&b, 1);
    return * this;
}

//TODO this is unreliable: reading 12 bytes, makes it return 128
//See h3r_pcx.h for an example
off_t ZipInflateStream::Tell() const { return Stream::Tell () - _zs.avail_in; }

off_t ZipInflateStream::Size() const { return _usize; }

Stream & ZipInflateStream::Read(void * buf, size_t bytes)
{
    /*OS::Log_stdout ("%pZipInflateStream::Read %zu bytes" EOL, this, bytes);*/
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
        auto sentinel1 = _zs.avail_in;
        auto sentinel2 = _zs.avail_out;
        _zr = inflate (&_zs, Z_SYNC_FLUSH);
        /*OS::Log_stdout (
            "ZipInflateStream::Read after inflate in:%zu, out:%zu; zr:%d" EOL,
            _zs.avail_in, _zs.avail_out, _zr);*/
        if (Z_STREAM_END == _zr) _zr = Z_OK;
        H3R_ENSURE(Z_OK == _zr, "ZipInflateStream::Read error")
        H3R_ENSURE(sentinel1 != _zs.avail_in || sentinel2 != _zs.avail_out,
            "ZipInflateStream::Read infinite loop case")
    }
    return  *this;
}// Read()

Stream & ZipInflateStream::Write(const void *, size_t)
{
    H3R_NOT_SUPPORTED_EXC("Write is not supported.")
}

// Reset to state
Stream & ZipInflateStream::ResetTo(int size, int usize)
{
    _zs.avail_in = _zs.avail_out = 0;     // caught me by surprise
    _zs.next_in = _zs.next_out = nullptr; // caught me by surprise
    _zr = inflateReset (&_zs);
    H3R_ENSURE(Z_OK == _zr, "inflateReset() error")
    _zr = Z_OK;
    _size = {size}, _usize = {usize};
    /*OS::Log_stdout ("%pZipInflateStream::ResetTo size:%zu, usize:%zu" EOL,
        this, _size, _usize);*/
    Stream::Seek (_pos_sentinel - Stream::Tell ());
    return *this;
}

NAMESPACE_H3R