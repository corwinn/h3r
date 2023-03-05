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

#ifndef _H3R_SCROLLBAR_H_
#define _H3R_SCROLLBAR_H_

#include "h3r.h"
#include "h3r_control.h"
#include "h3r_event.h"
#include "h3r_button.h"

H3R_NAMESPACE

// Preview: deciding whether its pros outweigh its cons. Temporary here.
template <typename T> class Property final
{
    private T _v {};
    public Property() {}
    public Property(const T & v) : _v{v} {}
    public Property(T && v) : _v{v} {}
    public T & operator=(const T & v)
    {
        if (_v != v) {
            _v = v;
            Changed (nullptr);
        }
        return _v;
    }
    public T & operator=(T && v)
    {
        if (_v != v) {
            _v = static_cast<T &&>(v);
            Changed (nullptr);
        }
        return _v;
    }
    public operator T() { return _v; }
    public Event Changed {};
};

// Scrollbars are intricate mechanisms. This one can't do a few things the big
// UI framework ones can:
//   - no repeating events: holding a mouse down doesn't cause repeating action
//   - its middle button has constant size
//   - it scrolls lines, not pixels
//   - it is vertical only
//   - it handles one event (from the virtual view) only: scroll 1 line up/down
//   - does not react to the mouse wheel
//   - view: no animated transitions
//   - it doesn't handle keyboard events AFAIK
//
// VScrollBar
//
// There is a peculiar behaviour I won't reproduce: you can start dragging the
// middle button by holding mouse down over the arrow buttons. I can't imagine
// how they did this bug.
#undef public
class ScrollBar final : public Control, public IHandleEvents
#define public public:
{
    // This is the width and the height of the up, dn, and mid buttons, and the
    // width of the scroll-bar. Being it this many things actually simplifies
    // the whole thing.
    private int _a {}; // [pixels]
    private int _t {}; // [pixels] - top of the mid button
    private int _key_b {}; // background
    private int _key_m {};
    private Button * btn_dn {};
    private Button * btn_up {};

    // parameters:
    public Property<int> Min {1};
    public Property<int> Max {100};
    public Property<int> SmallStep {1};  // up/down
    public Property<int> LargeStep {10}; // page up/down
    // This is how you update the scroll-bar position
    public Property<int> Pos {1};

    public ScrollBar(Control * c, Point p, int h) : Control {c} { Init (p, h); }
    public ScrollBar(Window * w, Point p, int h) : Control {w} { Init (p, h); }
    public virtual ~ScrollBar() override;

    // Modifying the parameters while handling this, will reward you with an
    // exception.
    // Changing Pos won't cause a Scroll event. The Scroll event is caused by
    // ScrollBar only in response to interactive (mouse in this case) events.
    // You shall get EventArgs with Delta holding the Position delta.
    public Event Scroll {};

    public void Init(Point, int);
    private void UpdateView(EventArgs *);
    private void Model2View();
    private void NotifyOnScroll(int);

    private void HandleScrollDown(EventArgs *);
    private void HandleScrollUp(EventArgs *);

    public inline int Width() const { return _a; }
    private void OnVisibilityChanged() override;
    private void OnMoved(int, int) override;
};// ScrollBar

NAMESPACE_H3R

#endif