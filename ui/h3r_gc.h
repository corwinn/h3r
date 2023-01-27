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

#ifndef _H3R_GC_H_
#define _H3R_GC_H_

#include "h3r.h"
#include "h3r_box.h"
#include "GL/gl.h"

H3R_NAMESPACE

// Graphics Context.
// A collection of rendering functions. Simplifies all other rendering code.
// This is where the glyph and sprite cache will be.
// You can always replace these with shaders, or whatever.
//
// The game rendering includes:
//   * Sprite (animated; with an FPS, group parameter)
//   * Text (1 sprite per glyph, rendered as the font engine says so)
//   * Line (the cursor for the text edit box (which is text rendering again))
//   * Video
// There is no color state here. Everything is pre-encoded at the game data.
class GC
{
    public: static GC * Current;

    public: inline void RenderBox(const Box & b)
    {
        glBegin (GL_LINE_LOOP);
            glVertex2d (b.Pos.X         , b.Pos.Y         );
            glVertex2d (b.Pos.X         , b.Pos.Y+b.Size.Y);
            glVertex2d (b.Pos.X+b.Size.X, b.Pos.Y+b.Size.Y);
            glVertex2d (b.Pos.X+b.Size.X, b.Pos.Y         );
        glEnd ();
    }
};

NAMESPACE_H3R

#endif
