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
#include "h3r_string.h"
#include "h3r_dll.h"
#include "h3r_stack.h"
#include <GL/gl.h>

H3R_NAMESPACE

// (1024 for buttons + 1024 for a few animations) * 2
#define H3R_DEFAULT_UI_MAX_SPRITES (1<<12)
// See render_engine.dia
#define H3R_DEFAULT_MAP_MAX_SPRITES (1<<17)

// This shall match the Open GL API expectations.
using H3Rfloat = GLfloat;

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
//
// Right now the z-order equals the order of UploadFrame(), because that's what
// glMultiDrawArrays passes to the pipeline.
// The z-order, if it becomes a requirement, shall be implemented via a z
// coordinate: {x,y,u,v} shall become {x,y,z,u,v}; with the ability to modify
// said z coordinate per key. The map window: the map control shall be defined
// 1st by the map window, so it will be rendered prior everything else.
class RenderEngine final
{
    H3R_CANT_COPY(RenderEngine)
    H3R_CANT_MOVE(RenderEngine)

    // Bind texture id to glMultiDrawArrays. Because frame0 could be at
    // tex-atlas0 and frame1 could be at tex-atlas1 e.g. Offset=0 shall be at
    // TexList.Texture == t0, and Offset=4 shall be at another TexList where
    // Texture == t1.
    struct TexList final
    {
        GLuint Texture {};
        List<GLint> _index {};
        List<GLsizei> _count {}; // 4, 4, ...
    };
    // .Texture distinct; one entry per tex-atlas.
    private List<TexList> _tex2_list {};
    private RenderEngine::TexList & ListByTexId(GLuint tex_id);

    private struct CheckPointEntry final
    {
        Array<int> TexListCounts {};
        int EntryCount {};
    };
    private Stack<CheckPointEntry> _cp_stack {};
    // Add a check-point to the animated sprite VBO. The next call to Rollback()
    // shall restore it to the check-point - all modifications after the check-
    // point shall be lost. Ideal for things that show for a wile and hide, then
    // show again after some time, and hide, etc.: like Dialog windows.
    public inline void CheckPoint()
    {
        CheckPointEntry e {};
        e.TexListCounts.Resize (_tex2_list.Count ());
        for (int i = 0; i < _tex2_list.Count (); i++) {
            H3R_ENSURE (
                _tex2_list[i]._index.Count () == _tex2_list[i]._count.Count (),
                "Bug: out of sync. TexList")
            e.TexListCounts[i] = _tex2_list[i]._index.Count ();
        }
        e.EntryCount = _entries.Count ();
        _cp_stack.Push (e);
    }
    public inline void Rollback()
    {
        H3R_ENSURE(! _cp_stack.Empty (), "Bug: Rollback w/o a CheckPoint")
        auto e = _cp_stack.Pop ();
        _tex2_list.Resize (e.TexListCounts.Length ());
        for (int i = 0; i < _tex2_list.Count (); i++) {
            _tex2_list[i]._index.Resize (e.TexListCounts[i]);
            _tex2_list[i]._count.Resize (e.TexListCounts[i]);
        }
        _entries.Resize (e.EntryCount);
    }

    private GLuint _vbo;
    private GLsizeiptr _vbo_max_elements;
    private GLfloat _znear = .0f, _zfar = 1.f, _zt = -.2f; // [-0.79;0.2]
    private GLfloat Depht2z(h3rDepthOrder); // UI Depth to here:z

    // Maps your frames (different uv, same 4 vertexes) to the _index above,
    // via a texture, because different frames could land in different atlases.
    // This entry represents one quad (a triangle-strip of four 4-component
    // vertices), where each frame has the same vertices, but different uv:
    //   frame0: {l,t,l0,t0}, {l,b,l0,b0}, {r,t,r0,t0}, {r,b,r0,b0},
    //   frame1: {l,t,l1,t1}, {l,b,l1,b1}, {r,t,r1,t1}, {r,b,r1,b1}, ...
    // All frames are placed in one giant VBO, but the UV could refer different
    // texture per frame. Now, the Base offset refers the position of frame0 at
    // the VBO. Offset=0 refers frame0 at Base, Offset=4 refers frame1, etc.
    private struct Entry final
    {
        // Base offset at the VBO (the sprite starts here).
        // "element" is one vertex that includes all the components needed for
        // its processing by the Open GL pipeline.
        GLint Base {};   // [element] (element:{x,y,u,v})
        // Specifies the currently being rendered sprite frame.
        // Base+Offset is put at _index above.
        GLint Offset {}; // [element] {x,y,u,v}
        // When false,
        // ListByTexId(_entries[key].Texture())._index[_entries[key].Key] = 0 (
        // a.k.a. the invisible quad)
        bool Visible {true};
        int Key; // TexList one

        // [0;Offset), [Offset0;Offset1), ...
        // "Offset" is the upper exclusive limit of the current range.
        struct OffsetRange2TexId final { GLint Offset {}; GLuint Texture {}; };
        // Maps Offset range to texture. Example: range[0;8) t0, range[8;12) t1.
        // Groups Frames by Texture - e.g. the name:
        //   frames.GroupBy (x=>x.Texture);
        List<OffsetRange2TexId> TexFrame {};
        int Frames {}; // Number of frames managed by this Entry
        inline GLuint Texture() // Returns the texture for current entry.Offset.
        {//LATER optimize
            if (TexFrame.Count () == 1) return TexFrame[0].Texture;
            // should be no more than Count () = 2, but
            for (int i = 1; i < TexFrame.Count (); i++)
                if (Offset >= TexFrame[i-1].Offset
                    && Offset < TexFrame[i].Offset) return TexFrame[i].Texture;
            H3R_ENSURE(false, "You have a bug: unknown texture")
        }
        //TODO using H3R_SPRITE_VERTICES <- move it here, very carefully
        inline void SetTexture(GLuint tex_id) // Update TexFrame
        {//LATER optimize
            for (int i = 0; i < TexFrame.Count (); i++)
                if (TexFrame[i].Texture == tex_id) {
                    // printf (" update TexFrame: [%d;%d)",
                    //   0 == i ? 0 : TexFrame[i-1].Offset, TexFrame[i].Offset);
                    TexFrame[i].Offset = Frames * 4; // up the range
                    // printf (" to: [%d;%d)" EOL,
                    //   0 == i ? 0 : TexFrame[i-1].Offset, TexFrame[i].Offset);
                    return;
                }
            OffsetRange2TexId itm {};
            itm.Offset = Frames * 4; // 4 - number of elements in a frame
            // printf (" new TexFrame: [0;%d)" EOL, itm.Offset);
            itm.Texture = tex_id;
            TexFrame.Add (itm);
        }
        // Return the next available position at the VBO. [elements]
        // 4 - number of elements at a frame.
        inline GLint NextBufPos() { return Base + Frames * 4; }
    };// Entry

    // Everything that could possibly be rendered. Accessed by the key returned
    // by GenKey().
    private List<RenderEngine::Entry> _entries {};

    private RenderEngine();
    public ~RenderEngine();
    // The UI doesn't require that many sprites.
    public RenderEngine(GLsizeiptr max_sprites);

    public void Render();
    public void Resize(int, int); // The 2D output has been resized.

    // Returns a key/handle to identify your sprite with the renderer.
    // Just-uploaded things, are visible => .Visible is true by default.
    public int GenKey();

    // Returns the offset you want applied when calling ChangeOffset()
    // "x" - left, and "y" - top, are absolute coordinates.
    // "fmt" - whatever TexCache::Cache() supports; currently 24(GL_RGB) and
    // 32(GL_RGBA) - that's your "data" format mind you; bytes per pixel.
    // The "texkey" is forwarded to the texcache for the purposes of uniquely
    // identifying a bitmap being cached there.
    //TODO this many parameters means composition is required
    //
    // "render_order": if you want "a" drawn over "b", "a.order" shall be >
    // "b.order". There shall be no more than 256 distinct things one over
    // another, so a byte should be enough; change it to short otherwise.
    public int UploadFrame(
        int key, GLint x, GLint y, GLint w, GLint h,
        h3rBitmapCallback data, h3rBitmapFormat fmt,
        const String & texkey, h3rDepthOrder render_order);

    // Use when a window is shown/hidden. Hiding the just the window won't do
    // what you think; the window shall notify all its controls to hide.
    //TODO this could be simplified by depth testing...
    public void ChangeVisibility(int key, bool state);

    // A.k.a. Render another frame.
    // When displaying animation this will be called often.
    // At any given time there are n animated sprites visible; so I think
    // this shall do: lets say 10000 animations (100x100 map) * 16 FPS = 160000
    // calls per second.
    // LATER Perhaps I can do something about it. This game does nothing in real
    //       time but render sprites and music, so it could be thought about.
    public void ChangeOffset(int key, GLint offset);

    // Do not use very often.
    // Reason:
    //  A button needs to get its texture generated so it can auto-size to it,
    //  and that size is needed to perform some limited layout, like centering
    //  a few buttons in a column defined by the widest one.
    //  Either it can't Upload() here, until its final position, or it needs
    //  a way to update its data. This method is the 2nd option.
    //  The 1st option involves someone else (the one doing the layout) to
    //  Upload() button data here, after it finishes moving it around.
    // Reason:
    //  You displayed Window2 over Window1, but you need to display Window1
    //  over Window2 - requires updating the z coordinates of both windows.
    public void UpdateRenderOrder(int key, h3rDepthOrder order);

    // Move (all frames)
    public void UpdateLocation(int key, GLint dx, GLint dy);

    public static void Init();

    // Text

    // Each entry has its own texture and its own VBO; that should be fast
    // enough for the simplicity.
    private struct TextEntry final
    {
        GLuint Texture {};//LATER Glyph cache; there are no more than 128 text
        GLuint Vbo {};    //      objs visible at once
        bool Visible {true};
        bool InUse {false};
        bool HasTransform {false};
        GLfloat Tx {}, Ty {};
    };
    private LList<RenderEngine::TextEntry> _texts {};
    public class TextKey final
    {
        private LList<RenderEngine::TextEntry> * _node {};
        public TextKey(LList<RenderEngine::TextEntry> &);
        public TextKey() {} // List<T>
        public void Delete();
        public inline void ChangeTextVisibility(bool state)
        {
            _node->Data.Visible = state;
        }
        public inline void SetTranslateTransform(bool state,
            GLfloat tx, GLfloat ty)
        {
            _node->Data.HasTransform = state;
            if (state) {
                _node->Data.Tx = tx;
                _node->Data.Ty = ty;
            }
        }
        public inline RenderEngine::TextEntry & Entry() { return _node->Data; }
    };
    public TextKey GenTextKey();
    // Layout it prior rendering. Everything TextRenderingEngine::RenderText()
    // supports, is supported.
    public void UploadText(TextKey & key,
        const String & font_name, const String & txt, int left, int top,
        unsigned int color, h3rDepthOrder order);
    public void UpdateText(TextKey & key,
        const String & font_name, const String & txt, int left, int top,
        unsigned int color, h3rDepthOrder order);
    public void ChangeTextVisibility(TextKey & key, bool state);
    public void TextSetTranslateTransform(TextKey & key, bool state,
        GLfloat = 0.f, GLfloat = 0.f);
    public void DeleteText(TextKey & key);

    // Window

    // These have alpha-blended drop-shadow, and tiling background.
    // I see no point storing their state, yet. So.
    private struct WinEntry final
    {
        GLuint Texture {};
        GLuint Vbo {};
    };
    private Stack<WinEntry> _win_entries {};
    // bpp - bytes per pixel
    public void ShadowRectangle(
        GLint x, GLint y, GLint w, GLint h,
        const byte * tile, h3rBitmapFormat tile_fmt, int tile_w, int tile_h,
        h3rDepthOrder order);
    // LIFO: Close the last opened one.
    public void DeleteShadowRectangle();

};// RenderEngine

NAMESPACE_H3R

#endif

// How many scene graphs and hash functions do you need to create a renderer?
