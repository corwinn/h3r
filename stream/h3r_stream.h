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

#ifndef _H3R_STREAM_H_
#define _H3R_STREAM_H_

#include "h3r_os.h"

H3R_NAMESPACE

class Stream
{
    // The book states the decorator shall differ the component, but it fails
    // to give reason; so lets find out:
    private Stream * _f{};

    public Stream(Stream * s) : _f{nullptr == s ? NoStream () : s} {}
    protected Stream() {} // no decorator constructor
    public virtual ~Stream() {}
    protected static Stream * NoStream(); // no "if (_f)"
    protected inline const Stream * BaseStream() const { return _f; }

    // true when the stream is ok
    public virtual inline operator bool() { return _f->operator bool(); }

    // Tell()-based: Seek (x - Tell ())
    public virtual inline Stream & Seek(off_t pos)
    {
        return _f->Seek (pos), *this;
    }
    public inline Stream & Begin() { return this->Seek (-Tell ()); }
    public inline Stream & End() { return this->Seek (Size () -Tell ()); }

    public virtual inline off_t Tell() const { return _f->Tell (); }
    public virtual inline off_t Size() const { return _f->Size (); }
    public virtual inline Stream & Read(void * b, size_t bytes = 1)
    {
        return _f->Read (b, bytes), *this;
    }
    public virtual inline Stream & Write(const void * b, size_t bytes = 1)
    {
        return _f->Write (b, bytes), *this;
    }

    // Reuse the stream object. Reset it to its just-constructed state.
    public inline virtual Stream & Reset() { return _f->Reset (); }

    // Avoid manual size computation. "virtual template <typename T>":
    public template <typename T> static Stream & Read(
        Stream & s, T * d, size_t num = 1)
    {
        return s.Read (d, num * sizeof(T));//TODO overflow math
    }
    public template <typename T> static Stream & Write(
        Stream & s, const T * d, size_t num = 1)
    {
        return s.Write (d, num * sizeof(T));//TODO overflow math
    }

    public static int constexpr STREAM_WRITE_BUF_SIZE {1<<12};
    // Buffered Write; buffer size: STREAM_WRITE_BUF_SIZE.
    // If "length" is 0, the entire stream shall be written.
    // You get ArgumentException for bad "src" or "length".
    // Writing starts at whatever position src is - its Seek() won't be called.
    public void Write(Stream & src, off_t length = 0);

    //LATER Stream.Read (Stream &)
    //LATER bool Readable(); bool Writeable(); bool Seekable(); bool Sizeless();
};

NAMESPACE_H3R

#endif