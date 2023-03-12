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

#include "h3r_gc.h"
#include "h3r_window.h"
#include "h3r_renderengine.h"

H3R_NAMESPACE

int UploadFrame(int rkey, int l, int t, Def & sprite,
    const String & sprite_name, const String & frame_name, h3rDepthOrder depth)
{
    static Array<byte> * bitmap {};
    auto s1 = sprite.Query (frame_name);
    H3R_ENSUREF(nullptr != s1, "Sprite not found: %s", frame_name.AsZStr ())
    bitmap = s1->ToRGBA ();
    H3R_ENSUREF(nullptr != bitmap && ! bitmap->Empty (),
        "Sprite->ToRGBA() failed: %s", frame_name.AsZStr ())
    auto bitmap_data = []() { return bitmap->operator byte * (); };
    return Window::UI->UploadFrame (rkey, l - sprite.FLeft (),
        t - sprite.FTop (), sprite.Width (), sprite.Height (), bitmap_data,
        h3rBitmapFormat::RGBA, sprite.GetUniqueKey (sprite_name), depth);
}

void UploadFrame(int rkey, int l, int t, Pcx & image,
    const String & image_name, h3rDepthOrder depth)
{
    static Array<byte> * bitmap {};
    bitmap = image.ToRGBA ();
    H3R_ENSUREF(nullptr != bitmap && ! bitmap->Empty (),
                "Failed to load %s", image_name.AsZStr ())
    auto bitmap_data = []() { return bitmap->operator byte * (); };
    Window::UI->UploadFrame (rkey, l, t, image.Width (), image.Height (),
        bitmap_data, h3rBitmapFormat::RGBA, image_name, depth);
}

NAMESPACE_H3R
