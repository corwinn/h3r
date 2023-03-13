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

// This will hold common rendering code for now.

#ifndef _H3R_GC_H_
#define _H3R_GC_H_

//TODO rename me

#include "h3r.h"
#include "h3r_def.h"
#include "h3r_pcx.h"

H3R_NAMESPACE

// Stops the program if it fails.
// Uploads a sprite frame to the RE, using rkey, left, top, sprite, sprite_name,
// frame_name, and depth. Returns the frame offset at the RE.
// ft: Apply sprite.FLeft () and sprite.FTop () transform.
//TODO find out why some sprites require it (mm buttons) and some do not
// (scenario list item icons)
int UploadFrame(int rkey, int l, int t, Def & sprite,
    const String & sprite_name, const String & frame_name, h3rDepthOrder depth);

// Stops the program if it fails.
// Uploads a bitmap to the RE, using rkey, left, top, image, image_name, and
// depth.
void UploadFrame(int rkey, int l, int t, Pcx & image, const String & image_name,
    h3rDepthOrder depth);

NAMESPACE_H3R

#endif
