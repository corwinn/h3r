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

#include "h3r_control.h"
#include "h3r_window.h"

H3R_NAMESPACE

void Control::Add(Control * c)
{
    // Adding one and the same thing twice speaks for an error somewhere;
    // Allowing it will slow things down. Ignoring it would leave wrong code.
    H3R_ARG_EXC_IF(_n.Contains (c), "duplicate pointer: fix your code")
    H3R_ARG_EXC_IF(nullptr == c, "c can't be null")
    _n.Add (c);
}

/*static*/ Window * Control::GetWindow(Control * c)
{
    H3R_ARG_EXC_IF(nullptr == c, "base shan't be null")
    return c->_window;
}

Control::Control(Control * base)
    : _window{GetWindow (base)}
{
    H3R_ARG_EXC_IF(this == base, "this can't be base")
    H3R_ARG_EXC_IF(nullptr == base, "base shan't be null")
    base->Add (this);
    _depth = Window::NextDepth (base->_depth);
    _window->UpdateTopMost (this);
    Resize (H3R_CONTROL_DEFAULT_SIZE, H3R_CONTROL_DEFAULT_SIZE);
}

Control::Control(Window * base)
    : _window{base}, _depth{Window::NextDepth (base->Depth ())}
{
    H3R_ARG_EXC_IF(nullptr == base, "base shan't be null")
    base->AddControl (this);
}

void Control::Resize(int w, int h)
{
    _bb.Size.X = w;
    _bb.Size.Y = h;
}
Control * Control::SetPos(int x, int y)
{
    // Yep, it begins to become scary, when it begins framework-ify-ing itself.
    int dx = x - _bb.Pos.X, dy = y - _bb.Pos.Y;
    _bb.Pos.X = x;
    _bb.Pos.Y = y;
    if (dx || dy) OnMoved (dx, dy);
    return this;
}
Control * Control::SetPosNoNotify(int x, int y)
{
    _bb.Pos.X = x;
    _bb.Pos.Y = y;
    return this;
}

bool Control::HitTest(Point & p) { return _bb.Contains (p); }

/*void Control::OnEvent(Event & e)
{
    EventArgs foo;
    e.Do (*this, foo);
}*/

NAMESPACE_H3R
