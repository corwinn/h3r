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

#ifndef _H3R_SPRITECONTROL_H_
#define _H3R_SPRITECONTROL_H_

#include "h3r.h"
#include "h3r_point.h"
#include "h3r_control.h"
#include "h3r_def.h"
#include "h3r_string.h"

// Maps sprite frames to values, since the game is using sprites for its
// UI and frames are often used as a mapping to some object property; like
// an icon, representing victory condition at a .h3m:
//   SpriteControl ("SCNRVICT.def") scon;
//   scon.Map ("ScnrVLaa.pcx", 1); // map.VCon is using "vcdesc.txt" mapping,
//                                 // where "Acquire Artifact" is at line 1
//   scon.Show (map.VCon);
//TODO there are a few thing to think about here, regarding a description form

H3R_NAMESPACE

// Its size is stored at the resource it is created from.
// Map keys: 0-255. Re-mapping and multi-map aren't supported, because they are
// not needed, yet.
#undef public
class SpriteControl: public Control
#define public public:
{
    private int _rkey {};
    private Def _sprite;
    private String _sprite_name;
    private bool _has_frames {};

    // This should do for now. If it happens to be not enough, then I'll
    // complicate the code here.
    private static int const MAP_MAX {256};
    private int _map[MAP_MAX] {};

    // Map sprite frame to an int key.
    public void Map(const String &, int);

    // Make "key" the currently visible sprite.
    public void Show(int);

    public SpriteControl(const String &, Control *, Point);
    public SpriteControl(const String &, Window *, Point);
    public ~SpriteControl() override;

    private void OnVisibilityChanged() override;
};// SpriteControl

NAMESPACE_H3R

#endif