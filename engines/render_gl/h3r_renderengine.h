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

// "fast" Open GL:
//  * minimize the gl calls
//  * let it compute

#ifndef _H3R_RENDERENGINE_H_
#define _H3R_RENDERENGINE_H_

#include "h3r.h"
#include "h3r_list.h"
#include "h3r_array.h"
#include <GL/gl.h>

H3R_NAMESPACE

// (1024 for buttons + 1024 for a few animations) * 2
#define H3R_DEFAULT_UI_MAX_SPRITES (1<<12)
// See render_engine.dia
#define H3R_DEFAULT_MAP_MAX_SPRITES (1<<17)

// This shall match the Open GL API expectations.
using H3Rfloat = GLfloat;

#define public public:
#define private private:

// Use case:
//  1. UI - those are things that don't move; the scrollbars just modify the 1st
//          visible cell (model) - the cells themselves do not move;
//  2. Map - these things move together; a.k.a. glTranslatef() is required to
//           handle map scrolling since Open GL is orders of magnitudes faster (
//           and that's an understatement) in updating multiple vertexes
//  3 & 4. Battle - 1 moving unit at a time; the rest are stationary;
// Should it happen that more instances are needed, this becomes class VBO,
// and RenderEngine shall handle VBOs.
//
// No this is not a general-use render engine. Its specific to this project.
// There are already a few of the generic ones that are pretty capable - there
// is no need for this project to re-invent the wheel. Perhaps one day this
// project shall be able to use some of the best renderers out there - I know
// you want to play multi-dimensional "Heroes III"; patience.
class RenderEngine final
{
    struct TexList final // bind texture id to glMultiDrawArrays
    {
        GLuint Texture {};
        List<GLint> _index {};
        List<GLsizei> _count {}; // 4, 4, ...
    };
    private List<TexList> _tex2_list {};
    private RenderEngine::TexList & ListByTexId(GLuint tex_id);

    private GLuint _vbo;

    private struct Entry final
    {
        // Base offset at the VBO (the sprite starts here).
        GLint Base {};   // [element] {x,y,u,v}
        // Specifies the currently being rendered sprite frame.
        // Base+Offset is put at _index above.
        GLint Offset {}; // [element] {x,y,u,v}
        bool Visible {true};
        int Key; // TexList one

        // [0;Offset), [Offset0;Offset1), ...
        struct OffsetRange2TexId final { GLint Offset {}; GLuint Texture {}; };
        List<OffsetRange2TexId> TexFrame {};
        int Frames {};
        inline GLuint Texture()
        {
            if (TexFrame.Count () == 1) return TexFrame[0].Texture;
            // should be no more than 2, but
            for (size_t i = 1; i < TexFrame.Count (); i++)
                if (Offset >= TexFrame[i-1].Offset
                    && Offset < TexFrame[i].Offset) return TexFrame[i].Texture;
            H3R_ENSURE(false, "You have a bug: unknown texture")
        }
        inline void SetTexture(GLuint tex_id)
        {
            for (size_t i = 0; i < TexFrame.Count (); i++)
                if (TexFrame[i].Texture == tex_id) {
                    // printf (" update TexFrame: [%d;%d)",
                    //   0 == i ? 0 : TexFrame[i-1].Offset, TexFrame[i].Offset);
                    TexFrame[i].Offset = Frames * 4; // up the range
                    // printf (" to: [%d;%d)" EOL,
                    //   0 == i ? 0 : TexFrame[i-1].Offset, TexFrame[i].Offset);
                    return;
                }
            OffsetRange2TexId itm {};
            itm.Offset = Frames * 4; // 4 - {x,y,u,v}
            // printf (" new TexFrame: [0;%d)" EOL, itm.Offset);
            itm.Texture = tex_id;
            TexFrame.Add (itm);
        }

        inline GLint NextBufPos() { return Base + Frames * 4; }
    };// Entry

    // everything that could possibly be rendered
    private List<RenderEngine::Entry> _entries {};

    public RenderEngine();
    public ~RenderEngine();
    // The UI doesn't require that many sprites.
    public RenderEngine(GLsizeiptr max_sprites);

    public void Render();

    // "n" - number of H3Rfloat in "buf"
    // Just-uploaded things are visible.
    //public int Upload(const H3Rfloat * buf, size_t n);
    public int GenKey();
    // Returns the offset you want applied when calling ChangeOffset()
    // "l" - left, and "t" - top, are absolute coordinates
    public int UploadFrame(
        int key, GLint x, GLint y, GLint w, GLint h, byte * data, int bpp);

    // When a window is shown/hidden
    public void ChangeVisibility(int key, bool state);
    // A.k.a. Render another frame
    public void ChangeOffset(int key, GLint offset);
    // Do not use very often.
    // Reason:
    //  A button needs to get its texture generated so it can auto-size to it,
    //  and that size is needed to perform some limited layout, like centering
    //  a few buttons in a column defined by the widest one.
    //  Either it can't Upload() here until its final position, or it needs
    //  a way to update its data here. This method is the 2nd option.
    //  The 1st option involves someone else (the one doing the layout) to
    //  Upload() button data here, after it finishes moving it around.
    //public void UpdateGeometry(int key, const H3Rfloat * buf);

    public static RenderEngine & UI();

    public static void Init();
};

#undef public
#undef private

NAMESPACE_H3R

#endif

// How many scene graphs and hash functions do you need to create a renderer?
