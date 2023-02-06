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

// Q'N'D code.

//c clang++ -std=c++11 -fsanitize=address,undefined,integer -fvisibility=hidden -I. -Ios -Ios/posix -Iutils -Istream -O0 -g -DH3R_DEBUG -fno-exceptions -fno-threadsafe-statics main.a unpack_snd.cpp -o unpack_snd

#include "h3r_os_error.h"
H3R_ERR_DEFINE_UNHANDLED
H3R_ERR_DEFINE_HANDLER(Memory,H3R_ERR_HANDLER_UNHANDLED)

#include "h3r_filestream.h"

H3R_NAMESPACE
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

int main(int argc, char ** argv)
{
    if (2 != argc)
        return printf ("usage: unpack_snd sndfile\n");

    H3R_NS::OS::FileStream s {argv[1], H3R_NS::OS::FileStream::Mode::ReadOnly};

    int cnt;
    s.Read (&cnt, 4);
    H3R_ENSURE(cnt > 0 && cnt < 8192, "that's way too many sounds")

    H3R_NS::Array<int> ofs {cnt};
    H3R_NS::Array<int> sz {cnt};
    H3R_NS::Array<H3R_NS::String> name {cnt};
    for (int i = 0; i < cnt; i++) {
        char fname[40] {};
        s.Read (fname, 40);
        // just for display reasons; not sure if the extension is needed
        // by the game
        for (int j = 0; j < 40; j++)
            if ('\0' == fname[j]) {
                fname[j] = '.';
                    // the "gog" version adds more code here
                    if (j+4 < 40) fname[j+4] = '\0' ; else
                fname[39] = '\0'; break;
        }
        name[i] = fname;
        s.Read (&ofs[i], 4);
        s.Read (&sz[i], 4);
#ifdef LIST_ONLY
        printf (
            "Entry #%2d: name: \"%s\", offset: %8d, size: %8d" EOL,
            i, name[i].AsZStr (), ofs[i], sz[i]);
#endif
    }

#ifdef LIST_ONLY
    return 0;
#endif

    const size_t BUF_SIZE {1<<16};
    H3R_NS::Array<H3R_NS::byte> buf {BUF_SIZE};
    for (int i = 0; i < cnt; i++) {
        s.Seek (ofs[i] - s.Tell ());
        off_t size = sz[i];
        H3R_ENSURE(size > 0, "snd size can't be <= 0")

        H3R_NS::OS::FileStream n {name[i],
            H3R_NS::OS::FileStream::Mode::WriteOnly};
        // H3R_NS::Log::Info (H3R_NS::String::Format (
        //    "Entry #%2d: name: \"%s\", offset: %8d, size: %8d" EOL,
        //    i, name[i].AsZStr ().Data (), ofs[i], size));
        while (size > 0) {
            size_t read = size < BUF_SIZE ? size : BUF_SIZE;
            s.Read ((H3R_NS::byte *)buf, read);
            n.Write ((H3R_NS::byte *)buf, read);
            size -= read;
        }
    }

    return 0;
} // main()
