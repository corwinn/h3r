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

// lod file format unpacker; it unpacks to the current directory!
//c clang++ -std=c++11 -I. -Ios -Ios/posix -Iutils -Istream -Igame -O0 -g -DH3R_DEBUG -fsanitize=address,undefined,integer,leak -fvisibility=hidden -fno-exceptions -fno-threadsafe-statics unpack_lod_vfs.cpp -o unpack_lod_vfs main.a h3r_game.o -lz

#include "h3r_os_error.h"
H3R_ERR_DEFINE_UNHANDLED
H3R_ERR_DEFINE_HANDLER(Memory,H3R_ERR_HANDLER_UNHANDLED)
H3R_ERR_DEFINE_HANDLER(Log,H3R_ERR_HANDLER_UNHANDLED)

#include "h3r_filestream.h"
#include "h3r_game.h"
#include "h3r_lodfs.h"
#include "h3r_array.h"

H3R_NAMESPACE

// allow file replacement
namespace OS {
class FileErrReplace final : public Error
{
    public: bool Handled(Error * e = nullptr) override
    {
        FileError * fe = (FileError *)e;
        if (FileError::Op::Replace == fe->Op)
            return fe->Replace = true;
        return false;
    }
};
} // namespace OS
NAMESPACE_H3R
static H3R_NS::OS::FileErrReplace my_file_err;
H3R_ERR_DEFINE_HANDLER(File,my_file_err)

int main(int c, char ** v)
{//TODO async IO, with the main thread displaying progress; and wise quotes
    if (2 != c)
        return printf ("usage: unpack_lod lodfile\n");

    // init the log service
    H3R_NS::Game game;

    H3R_NS::LodFS {v[1]}
        .Walk([](H3R_NS::Stream & stream, const char * name) -> bool
        {
            H3R_NS::Array<unsigned char> stream_data {};
            //TODO Buffer
            stream_data.Resize (stream.Size ());
            unsigned char * stream_data_buf = stream_data;
            H3R_NS::OS::FileStream stream_to_file {
                name, H3R_NS::OS::FileStream::Mode::WriteOnly};
            stream.Read (stream_data_buf, stream.Size ());
            stream_to_file.Write (stream_data_buf, stream.Size ());
            return true;
        });

    return 0;
}
