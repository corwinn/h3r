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

#ifndef _H3R_SDLRWOPS_H_
#define _H3R_SDLRWOPS_H_

#include "h3r.h"
// #include "h3r_criticalsection.h"
#include "h3r_log.h"
#include "h3r_string.h"
#include <SDL2/SDL_mixer.h>

H3R_NAMESPACE

/*static inline void TheWayIsShut()
{
    static OS::CriticalSection one_by_one_please {};
}*/

// Translate Stream to SDL_RWops:
//   SDL_RWops * SDL_RWFromStream(Stream & s)
// The SDL_RWops shall be closed by Mix_LoadWAV_RW(), so no need to do
// funny things just yet.
// Supports the zipinflatestream.
//LATER cache; The people at SDL have really thought this through. I can only
//      observe and learn how to design things.
//TODO be careful; this shall not work if the input stream does not support
//     seek
class SdlRWopsStream final : public Stream
{
#define public public:
#define private private:
    public struct RWops final
    {
        // Sint64 (*)(SDL_RWops *) size - Size()
        // Sint64 (*)(SDL_RWops *, Sint64, int) seek
        // size_t (*)(SDL_RWops *, void *, size_t, size_t) read
        // size_t (*)(SDL_RWops *, const void *, size_t, size_t) write
        // int (*)(SDL_RWops *) close
        struct SDL_RWops SdlStream;
        Stream & Data;
        off_t DataSize;
        RWops(Stream & data) : Data{data}, DataSize{data.Size ()}
        {
            SdlStream.type = SDL_RWOPS_UNKNOWN;
            SdlStream.size = [](SDL_RWops * h) -> Sint64
                {
                    return reinterpret_cast<RWops*>(h)->Data.Size ();
                };
            SdlStream.seek = [](SDL_RWops * h, Sint64 o, int whence) -> Sint64
                {
                    Stream & s = reinterpret_cast<RWops*>(h)->Data;
                    switch (whence) {
                        case RW_SEEK_CUR: if (o < 0) o += s.Tell (); break;
                        case RW_SEEK_END: o = s.Size () - o; break;
                        default: break;
                    }
                    s.Reset (); s.Seek (o); return s.Tell ();
                };
            SdlStream.read =
                [](SDL_RWops * h, void * buf, size_t s, size_t n) -> size_t
                {
                    SdlRWopsStream::RWops * ops = reinterpret_cast<RWops*>(h);
                    if (ops->Data.Tell () == ops->DataSize)
                        return (-1); // wrong design; EOF is platform specific
                    ops->Data.Read (buf, s*n);
                    return n;
                };
            SdlStream.write = nullptr;
            SdlStream.close = [](SDL_RWops *) -> int { return 0; };
        }
        ~RWops()
        {
        }
    };
    private RWops _ops;
    public SdlRWopsStream(Stream * s) : Stream {s}, _ops{*this} {}
    public ~SdlRWopsStream() override {}
    public inline operator SDL_RWops *()
    {
        return reinterpret_cast<SDL_RWops *>(&_ops);
    }
};

#undef public
#undef private

NAMESPACE_H3R

#endif