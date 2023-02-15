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

#ifndef _H3R_MEMORYSTREAM_H_
#define _H3R_MEMORYSTREAM_H_

#include "h3r_stream.h"
#include "h3r_array.h"

H3R_NAMESPACE

// A stream wrapped buffer.
class MemoryStream final : public Stream
{
#define public public:
#define private private:

    private Array<byte> _buf {};
    private bool _ok {false};
    private off_t _pos {}; // rw pointer
    public MemoryStream(Stream * s, int size)
        : Stream {s}, _buf {size}
    {
        if (s && *s)
            _ok = s->Read (_buf.operator byte * (), _buf.Length ());
        _ok &= true;
    }
    public ~MemoryStream() override {}
    public inline operator bool() override { return _ok; }
    public inline Stream & Seek(off_t ofs) override
    {
        H3R_ARG_EXC_IF(! _ok, "The stream is useless")
        off_t npos = _pos + ofs;
        H3R_ARG_EXC_IF(npos < 0, "Can't seek prior stream start")
        H3R_ARG_EXC_IF(npos >= _buf.Length (), "Can't seek after stream end")
        _pos = npos;
        return *this;
    }
    public inline off_t Tell() const override { return _pos; }
    public inline off_t Size() const override { return _buf.Length (); }
    public inline Stream & Read(void * buf, size_t bytes) override
    {
        H3R_ARG_EXC_IF(bytes <= 0, "Can't read that")
        H3R_ARG_EXC_IF(_pos + (int)bytes > _buf.Length (), "Can't read that")
        OS::Memcpy (buf, _buf.operator byte * () + _pos, bytes);
        _pos += bytes;
        return *this;
    }
    public inline Stream & Write(const void * buf, size_t bytes) override
    {
        H3R_ARG_EXC_IF(bytes <= 0, "Can't write that")
        H3R_ARG_EXC_IF(_pos + (int)bytes > _buf.Length (), "Can't write that")
        OS::Memcpy (_buf.operator byte * () + _pos, buf, bytes);
        _pos += bytes;
        return *this;
    }
    public inline Stream & Reset() override { _pos = 0; return *this; }

    // Direct buffer access.
    public const byte * Buffer() const { return _buf.operator byte * (); }

#undef public
#undef private
};

NAMESPACE_H3R

#endif