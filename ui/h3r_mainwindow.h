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

#ifndef _H3R_MAINWINDOW_H_
#define _H3R_MAINWINDOW_H_

#include "h3r.h"
#include "h3r_window.h"
#include "GL/gl.h"
#include "h3r_label.h"

H3R_NAMESPACE

// MainWindowGL
class MainWindow : public Window
{
#define public public:
#define private private:
#define protected protected:

    private int _w {800}, _h{600};
    private List<Label *> _time_labels {};

    public MainWindow(OSWindow *, Point &&);
    public ~MainWindow() override;

    protected virtual void OnKeyUp(const EventArgs &) override;
    protected virtual void OnShow() override;
    protected virtual void OnRender() override;
    protected virtual void OnResize(int w, int h) override;
};

#undef public
#undef private
#undef protected

NAMESPACE_H3R

#endif