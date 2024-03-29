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

#ifndef _H3R_TEXCACHE_H_
#define _H3R_TEXCACHE_H_

#include "h3r.h"
#ifdef _WIN32
#undef public
#endif
#include <GL/gl.h>
#ifdef _WIN32
#define public public:
#endif
#include "h3r_string.h"

H3R_NAMESPACE

// Use-case:
//   var e = tc->Cache (w, h, bug, 3)
//   vertices.uv = e.uv;
//   ...
//
//   TexCache::Bind (e)
//LATER is there a point creating a dynamic tex. atlas (to save resources)?
//      Dynamic tex. atlas: resize the tex. atlas on an as needed basis.
class TexCache final
{
    // The object is managed by a static hash at Cache()
    public struct Entry final
    {
        GLuint Texture {};
        GLfloat l, t, r, b;
        // internal state; don't modify please
        int x {};
        int y {};
    };
    private GLint const _tsize;
    public TexCache();
    public ~TexCache();
    public void Invalidate();

    // The "key" part binds TexCache::Entry e.g. you don't have to worry about
    // duplicate bitmaps as long as they're uniquely identified by a "key".
    // Make sure your "key" uniquely identifies your bitmap, or you shall get
    // the wrong TexCache::Entry. The bitmap "data" shall be requested as
    // needed.
    public Entry Cache(GLint w, GLint h,
        h3rBitmapCallback data, h3rBitmapFormat fmt,
        const String & key);

    private static GLuint _bound; // currently bound texture
    // Either all are using it, or none. Since there are entities that are'n
    // uisng the TexCache - like text rendering, this is off.
    /*public static inline void Bind(TexCache::Entry & e)
    {
        if (e.Texture != _bound)
            glBindTexture (GL_TEXTURE_2D, _bound = e.Texture);
    }*/

    public static TexCache * One ();
};

NAMESPACE_H3R

#endif