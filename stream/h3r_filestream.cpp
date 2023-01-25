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

#include "h3r_filestream.h"

H3R_NAMESPACE
namespace OS {

off_t FileStream::tell() const
{
    var r = Ftell (_f);
    H3R_ENSURE(H3R_FILE_OP_NOT_SUPPORTED != r, "tell() is not supported")
    return r;
}

Stream & FileStream::seek(off_t offset)
{
    var r = Fseek (_f, offset, SEEK_CUR);
    H3R_ENSURE(H3R_FILE_OP_NOT_SUPPORTED != r, "seek() is not supported")
    return *this;
}

FileStream::FileStream(const String & name, Mode mode)
    : Stream{nullptr}, _name {name}, _mode {mode}
{
    switch (mode) {
        case Mode::ReadOnly : { // should exist
            _f = Fopen (_name, "rb");
            _size_on_open = online_size();
        } break;
        case Mode::Append : // created if ! exists
            _f = Fopen ( _name, "ab");
            break;
        case Mode::ReadWrite : // should exist
            _f = Fopen ( _name, "rb+");
            break;
        case Mode::WriteOnly : {
            if (! FileExists (_name)) {
                _f = Fopen (_name, "wb");
                break;
            }
            FileError t {FileError::Op::Replace, FileError::Code::Loop};
            // Log_stdout ("Asking for file replace" EOL);
            bool handled = Error::File.Handled (&t);
            H3R_ENSURE(handled, "Handling file replacement is mandatory.")
            if (t.Replace) t.Replace = false, _f = Fopen (_name, "wb");
        } break;
        default: H3R_ENSURE(false, "Unknown file mode")
    };
}

FileStream::~FileStream() { if (_f) Fclose (_f), _f = 0; }

off_t FileStream::Size() const
{
    return Mode::ReadOnly == _mode ? _size_on_open : online_size ();
}

Stream & FileStream::Read(void * b, size_t bytes)
{
    if (Mode::ReadWrite == _mode) {
        if (_last_op == Op::Write) online_size ();
        _last_op = Op::Read;
    }
    return Fread (b, 1, bytes, _f), *this;
}

Stream & FileStream::Write(const void * b, size_t bytes)
{
    if (Mode::ReadWrite == _mode) {
        if (_last_op == Op::Read) online_size ();
        _last_op = Op::Write;
    }
    return Fwrite (b, 1, bytes, _f), *this;
}

Stream & FileStream::Reset()
{
    if (Mode::ReadOnly == _mode || Mode::ReadWrite == _mode) {
        seek (-tell ());
        return *this;
    }
    H3R_ENSURE(false, "ImplementMe: FileStream::Reset() mode != Read")
    return *this;
}

/*static*/ bool FileStream::Exists(String & name)
{
    return OS::FileExists (name);
}

} // namespace OS
NAMESPACE_H3R
