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

#include "h3r_button.h"
// #include "h3r_log.h"
// #include "h3r_string.h"

H3R_NAMESPACE

Button::Button(const String &, Control * base)
    : Control {base}
{
    //TODO h3r_defdecoder
}

void Button::OnMouseMove(const EventArgs & e)
{
    static Point p;
    p.X = e.X;
    p.Y = e.Y;
    _mouse_over = HitTest (p);
    /*Log::Info (String::Format (
        "Button::OnMouseMove (%d, %d), hover: %d, _bb (%d, %d, %d, %d)" EOL,
        p.X, p.Y, _mouse_over,
        ClientRectangle ().Pos.X, ClientRectangle ().Pos.Y,
        ClientRectangle ().Size.X, ClientRectangle ().Size.Y));*/
}

void Button::OnRender(GC & gc)
{
    glLoadIdentity ();
    glDisable (GL_TEXTURE_2D); // implict contract with MainWindow.Show()
                               //LATER to the render engine
    if (! _mouse_over) glColor3d (1.0, 1.0, 1.0);
    else glColor3d (0.0, 1.0, 0.0);
    gc.RenderBox (ClientRectangle ());
    glEnable (GL_TEXTURE_2D);
}

NAMESPACE_H3R
