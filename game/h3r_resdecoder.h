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

// A means to abstract away game-specific bitmap from render-specific one.
// E.g. instead of pcx_2_sdl(), etc.: pcx_2_RGBA_buf().

#ifndef _H3R_RESDECODER_H_
#define _H3R_RESDECODER_H_

#include "h3r.h"
#include "h3r_array.h"
#include "h3r_log.h"
#include "h3r_stream.h"
#include "h3r_string.h"

H3R_NAMESPACE

// If the returned buffer is empty, see the log for errors.
class ResDecoder
{
    public ResDecoder() {}
    public virtual ~ResDecoder() {}
    // This is my slow convert function, where I need to set alpha=f(rgb).
    public virtual Array<byte> * ToRGBA() { return nullptr; }
    // "wingdi.h" didn't like my "RGB" symbol.
    // This is the game native format, better left for Open GL to convert to.
    public virtual Array<byte> * ToRGB() { return nullptr; }
    public static int const MAX_SIZE {16384};

    public virtual int Width () { return 0; }
    public virtual int Height () { return 0; }
};

NAMESPACE_H3R

#endif