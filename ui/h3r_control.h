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

// I never quite understood why are people writing millions of lines of
// code in order to render a few glyphs/sprites and rectangles.
//
// The UI is a game. There is a model (event-driven) and a view (renderer,
// persistence, etc.). This instance requires model and renderer only.

#ifndef _H3R_CONTROL_H_
#define _H3R_CONTROL_H_

#include "h3r.h"
#include "h3r_list.h"
#include "h3r_point.h"
#include "h3r_box.h"
// #include "h3r_gc.h"
#include "h3r_eventargs.h"
#include "h3r_event.h"
//#include "h3r_renderengine.h"

#define H3R_CONTROL_DEFAULT_SIZE 32

H3R_NAMESPACE

// Graphics control. "owns" all controls on it <-> manages their resources.
// Coordinate system:
//  right: +x; left: -x; top: -y; bottom: +y; 0,0 as top left screen corner;
//  w,h [pixels]
//
// Absolute coordinates, because there is no auto-layout being done, and because
// this isn't a generic UI framework. The local CS, should it becomes a
// necessity, is -Pos();
class Control
{
    protected using Window = class Window;
    friend Window;

    private Box _bb; // bounding box
    private bool _enabled {true}; // whether the user can interact with it
    private bool _hidden {false}; // whether the user can interact with it
    // The difference: _enabled represents whether a control is "grayed out";
    // _hidden controls whether a control is rendered at all.

    protected Window * _window;
    protected h3rDepthOrder _depth;
    // Render order. See Window: "Automatic depth".
    protected inline virtual h3rDepthOrder Depth() { return _depth; }
    private List<Control *> _n;   // Sub-controls
    private void Add(Control *);

    // Handle nullptr at con. init.
    protected static Window * GetWindow(Control *);

    // Controls placed directly on another control have the same depth
    // (control+1); their rendering order is the order of placement.
    public Control(Control *); // sets _depth
    // Controls placed directly on a window have the same depth (window+1);
    // their rendering order is the order of placement.
    public Control(Window *);  // sets _depth
    public virtual ~Control()
    {
        for (auto p : _n) H3R_DESTROY_OBJECT(p, Control)
    }

    public const Point & Pos() { return _bb.Pos; }
    public const Point & Size() { return _bb.Size; }
    public inline int Width() const { return _bb.Size.Width; }
    public inline int Height() const { return _bb.Size.Height; }
    // Let controls know, their position has been changed.
    public virtual Control * SetPos(int, int);

    // Upload the Frames to the RenderEngine.
    // This is not called on construction, on purpose: so that layout can be
    // done prior uploading to the render engine.
    public inline virtual void UploadFrames(/*RenderEngine * = nullptr*/) {}

    //TODO template <typename T, typename Changed> class Property.
    public inline void SetEnabled(bool value) { _enabled = value; }
    // Has a specific sprite for it.
    public inline bool Enabled() const { return _enabled; }
    public inline void SetHidden(bool value)
    {
        if (value != _hidden) {
            _hidden = value;
            OnVisibilityChanged ();
        }
    }
    // "hidden" do receive no events.
    public inline bool Hidden() const { return _hidden; }

    protected inline Box & ClientRectangle() { return _bb; }
    //LATER Allowed for now. Its bound to its sprite. Scaling shall be done by
    //      the render engine regardless of this function here.
    protected void Resize(int, int);
    protected virtual bool HitTest(Point & p);

    protected virtual void OnRender(/*GC &*/) {} //TODO useless?
    protected virtual void OnMouseMove(const EventArgs &e)
    {
        for (auto p : _n) p->OnMouseMove (e);
    }
    protected virtual void OnMouseUp(const EventArgs & e)
    {
        for (auto p : _n) p->OnMouseUp (e);
    }
    protected virtual void OnMouseDown(const EventArgs & e)
    {
        for (auto p : _n) p->OnMouseDown (e);
    }
    protected virtual void OnKeyDown(const EventArgs &) {}
    protected virtual void OnKeyUp(const EventArgs &) {}
    protected virtual void OnVisibilityChanged() {}

    //TODO hint system; MyHint: Event ... show_hint (myhint);
    //     Control.RMB += (Event & ShowHint) => Root().OnEvent (ShowHint);
    //     After merging CoR back into the mcast delegate.
    // public: virtual void OnEvent(Event &);
};

NAMESPACE_H3R

#endif