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

#ifndef _H3R_FILESTREAM_H_
#define _H3R_FILESTREAM_H_

#include "h3r_stream.h"
#include "h3r_os_stdio_wrappers.h"
#include "h3r_string.h"

H3R_NAMESPACE
namespace OS {

// Caution: Mode::WriteOnly streams should be verified right after construction
// because the user might choose to not overwrite the file in question. You can
// always use FileStream::Exists of course and ask the user prior construction,
// in which case handling FileError::Op::Replace becomes very simple.
// Not checking when the user have said "no" will get you an exit() from the
// 1st stdio wrapper.
class FileStream final: public Stream
{
    private: FILE * _f {};
    private: off_t _size_on_open {};
    private: String _name;
    private: inline off_t online_size() const
    {
        return FileSize (_name);
    }
    private: off_t tell() const;
    private: Stream & seek(off_t offset);

    public: enum class Mode
    {
        ReadOnly,  // resources, save games, hi-scores
        WriteOnly, // save games, hi-scores ; the stream will be unusable should
                   // the user have chosen to not replace the file
        Append,    // file log
        ReadWrite  // unused so far
    } _mode;

    // https://pubs.opengroup.org/onlinepubs/9699919799/functions/fopen.html
    private: enum class Op {None, Read, Write} _last_op {Op::None};

    public: FileStream(const String & name, Mode mode);
    public: ~FileStream() override;

    public: inline operator bool() override { return nullptr != _f; }

    public: inline Stream & Seek(off_t pos) override { return seek (pos); }
    public: inline off_t Tell() const override { return tell (); }
    public: off_t Size() const override;
    public: Stream & Read(void * b, size_t bytes = 1) override;
    public: Stream & Write(const void * b, size_t bytes = 1) override;
    public: Stream & Reset() override;

    public: inline const String & Name() const { return _name; }

    public: static bool Exists(String & name);
};

} // namespace OS
NAMESPACE_H3R

#endif