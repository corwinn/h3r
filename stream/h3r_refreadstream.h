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

#ifndef _H3R_REFREADSTREAM_H_
#define _H3R_REFREADSTREAM_H_

#include "h3r_stream.h"

H3R_NAMESPACE

// A stream used for reading a sector of the decorated stream.
// All of its state is local; the base stream state isn't published.
// You get NotSupportedException on Write().
// Thread-unsafe!
//LATER Should the need arise for multiple threads to read simultaneously,
//      a ThreadSafeReadStream shall be created:
//      RefReadStream {ThreadSafeReadStream {base_one}, ...}
// Why: because you might want to not cache everything in RAM.
//LATER If you want everything cached, use MemoryStream instead of this one.
class RefReadStream final : public Stream
{
    H3R_CANT_COPY(RefReadStream)
    H3R_CANT_MOVE(RefReadStream)

#define public public:
#define private private:
    private bool _ok {false};
    private off_t _start, _size, _pos {};
    public RefReadStream(Stream * s, off_t start, off_t size);
    public ~RefReadStream() override {}
    public inline operator bool() override { return _ok; }
    public Stream & Seek(off_t) override;
    public inline off_t Tell() const override { return _pos; }
    public inline off_t Size() const override { return _size; }
    public Stream & Read(void *, size_t) override;
    public Stream & Write(const void *, size_t) override;

    // same meaning as constructor parameters; can't modify the _f thought
    public Stream & ResetTo(off_t start, off_t size);
    private void Reset();
#undef public
#undef private
};

NAMESPACE_H3R

#endif