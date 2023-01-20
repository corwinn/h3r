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

#ifndef _H3R_VFS_H_
#define _H3R_VFS_H_

// virtual file system - abstract away the game files

#define VERY_IMPORTANT_AND_VERY_USELESS_MESSAGE "Resource system initialized()"

#include "h3r.h"
#include "h3r_string.h"
#include "h3r_stream.h"

H3R_NAMESPACE

//TODO unicode support,
// Consider ASCII to be the safe choice for your filenames.
class VFS
{
    public: enum class FileType {nvm,
        bik, def, fnt, h3c, h3m, h3r, mp3, msk, pal, pcx, smk, txt, wav};

    public: VFS(const String & path);
    public: virtual ~VFS();

    // You request by name - you get a stream. You request the same name
    // again - you get another stream. What you do with the returned stream
    // object is up to you.
    // For a VFS file (.lod, .snd, .vid) - you get a VFS stream. Releasing it
    // won't release the underlying file handler, unless this is the last open
    // VFS stream referring that particular file.
    // For an FS file - you get a file stream. Releasing it will release
    // the file handle.
    // use-case 1: on_game_start (cache all needed resources into RAM).
    // use-case 2: on_map_load (cache all needed resources into RAM).
    // use-case 3: on_map_load (invalidate, and do use-case #2)
    public: virtual Stream Get(const String & name);
};

NAMESPACE_H3R

#endif