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

#include "h3r_refreadstream.h"

H3R_NAMESPACE

RefReadStream::RefReadStream(Stream * s, off_t start, off_t size)
    : Stream {s}, _start{start}, _size{size}
{
    Reset ();
}

Stream & RefReadStream::Seek(off_t ofs)
{
    H3R_ENSURE(_pos + ofs <= _size, "Bug: RefReadStream: seek overflow")
    _pos += ofs;
    return *this;
}

Stream & RefReadStream::Read(void * buf, size_t bytes)
{
    var p = _start + _pos;
    if (Stream::Tell () != p) Stream::Seek (p - Stream::Tell ());
    H3R_ENSURE(Stream::Tell () == p, "Bug: RefReadStream: can't sync to base")
    H3R_ENSURE(_pos + static_cast<off_t>(bytes) <= _size,
        "Bug: RefReadStream: read overflow")
    return _pos += bytes, Stream::Read (buf, bytes), *this;
}

Stream & RefReadStream::Write(const void *, size_t)
{
    H3R_NOT_SUPPORTED_EXC("Write is not supported.")
}

Stream & RefReadStream::ResetTo(off_t start, off_t size)
{
    _start = {start};
    _size = {size};
    Reset ();
    return *this;
}

Stream & RefReadStream::Reset()
{
    _ok = false;
    _pos = 0;
    Stream::Seek (_start - Stream::Tell ());
    H3R_ENSURE(
        Stream::Tell () == _start, "Bug: RefReadStream: can't sync to base")
    _ok = true;//TODO should this be used for all ops?
    return *this;
}

NAMESPACE_H3R