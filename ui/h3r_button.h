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

#ifndef _H3R_BUTTON_H_
#define _H3R_BUTTON_H_

#include "h3r.h"
#include "h3r_string.h"
#include "h3r_point.h"
#include "h3r_gc.h"
#include "h3r_control.h"
#include "h3r_eventargs.h"
#include "h3r_renderengine.h"

H3R_NAMESPACE

// Its size is stored at the resource it is created from.
class Button: public Control
{
    private: RenderEngine * _re {};
    private: int _rkey {};
    private: int _on {}; // up
    private: int _oh {}; // hover
    private: int _os {}; // down
    private: String _sprite_name {};

    public: Button(const String &, Control * = nullptr);
    public: virtual ~Button() override
    {
        if (! _re)
            RenderEngine::UI ().ChangeVisibility (_rkey, false);
    }

    public: virtual Control * SetPos(int, int) override;
    public: void UploadFrames(RenderEngine * = nullptr) override;

    // public: virtual void OnRender(GC &) override;
    public: virtual void OnMouseMove(const EventArgs &) override;
    public: virtual void OnMouseDown(const EventArgs &) override;
    public: virtual void OnMouseUp(const EventArgs &) override;

    private: bool _mouse_over {};
    private: bool _mouse_down {};

    // Experiment: you attach here a capturing lambda via a template function
    // pointer: EventDelegate<decltype(cl)>
    public: void (*OnClick)(Control * sender) {};
};

NAMESPACE_H3R

#endif