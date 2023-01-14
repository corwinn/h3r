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
#include "h3r_gc.h"
#include "h3r_mkstate.h"
#include "h3r_event.h"

H3R_NAMESPACE

// Graphics control. "owns" all controls on it <-> manages their resources.
// Coordinate system:
//  right: +x; left: -x; top: -y; bottom: +y; 0,0 as top left screen corner;
//  w,h [pixels]

class Control
{
    private: List<Control *> _z;   // z-order; render: from 0 to count-1
    private: List<Control *> _n;   // non-visible ones
    private: void Add(Control *);
    private: static Control * Active;     // receives input
    private: Control() {}
    private: static Control * Root();     // refers all controls
    public: Control(Control *);

    private: Box _bb; // bounding box
    protected: virtual bool HitTest(Point & p); // [input function]

    private: bool _visible;
    public: static void NotifyDraw(GC &); // Entry point for Draw
    protected: void DoDraw(GC &);         // TreeWalk(OnDraw)

    // do not check _visible here; there is visible list and a non-visible one -
    // controlled by event-driven logic; no need to check visibility each time
    // draw is issued
    protected: virtual void Draw(GC &) {}
    protected: virtual void MouseEvent(MKState &) {}
    protected: virtual void KeyboardEvent(MKState &) {}
    protected: virtual void OnEvent(Event &); // when the above 2 are not enough
};

NAMESPACE_H3R

#endif