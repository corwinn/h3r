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

#include "h3r_spritecontrol.h"
#include "h3r_game.h"
#include "h3r_gc.h"
#include "h3r_window.h"

H3R_NAMESPACE

void SpriteControl::Map(const String & frame, int key)
{
    H3R_ENSURE(key >= 0 && key < SpriteControl::MAP_MAX, "Key out of range")
    _map[key] = UploadFrame (_rkey, Pos ().Left, Pos ().Top, _sprite,
        _sprite_name, frame, Depth ());
    _has_frames = true;
}

void SpriteControl::Show(int key)
{
    if (Hidden ()) return;
    H3R_ENSURE(key >= 0 && key < SpriteControl::MAP_MAX, "Key out of range")
    Window::UI->ChangeOffset (_rkey, _map[key]);
}

SpriteControl::SpriteControl(const String & sname, Control * base, Point p)
    : Control {base}, _sprite {Game::GetResource (sname)}, _sprite_name {sname}
{
    SetPos (p);
    _rkey = Window::UI->GenKey ();
}

SpriteControl::SpriteControl(const String & sname, Window * base, Point p)
    : Control {base}, _sprite {Game::GetResource (sname)}, _sprite_name {sname}
{
    SetPos (p);
    _rkey = Window::UI->GenKey ();
}

SpriteControl::~SpriteControl() {}

void SpriteControl::OnVisibilityChanged()
{
    if (_has_frames && _rkey > 0)
        Window::UI->ChangeVisibility (_rkey, ! Hidden ());
}

NAMESPACE_H3R
