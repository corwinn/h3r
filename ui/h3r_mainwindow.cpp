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

#include "h3r_mainwindow.h"
#include "h3r_game.h"
#include "h3r_resdecoder.h"
#include "h3r_thread.h"
#include "h3r_button.h"

#include <new>

H3R_NAMESPACE

MainWindow::MainWindow(OSWindow * actual_window)
    : Window{actual_window}
{
    Button * hellow_world_button;
    H3R_CREATE_OBJECT(hellow_world_button, Button) {"", nullptr};
    Add (hellow_world_button);
}

MainWindow::~MainWindow()
{
    glDeleteBuffers (1, &_vbo), glDeleteTextures (1, &_tex);
}

void MainWindow::OnKeyUp(const EventArgs & e)
{
    if (H3R_KEY_Q == e.Key)
        Close ();
}

void MainWindow::OnShow()
{
    glDisable (GL_COLOR_MATERIAL);
    glEnable (GL_TEXTURE_2D);
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glEnable (GL_CULL_FACE), glCullFace (GL_BACK);
    glEnable (GL_VERTEX_ARRAY); glEnable (GL_TEXTURE_COORD_ARRAY);
    glClearColor (.0f, .0f, .0f, 1.f);
    glDisable (GL_DEPTH_TEST);
    glDisable (GL_DITHER);
    glDisable (GL_BLEND);
    glDisable (GL_LIGHTING);
    glDisable (GL_FOG);
    glDisable (GL_MULTISAMPLE);
    glShadeModel (GL_FLAT);

    glGenTextures (1, &_tex);
    Pcx main_window_background {Game::GetResource ("GamSelBk.pcx")};
    var byte_arr_ptr = main_window_background.RGB ();
    if (! byte_arr_ptr || byte_arr_ptr->Empty ()) {
        H3R_NS::Log::Err ("Failed to load GamSelBk.pcx");
        return;
    }
    glBindTexture (GL_TEXTURE_2D, _tex);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexImage2D (GL_TEXTURE_2D, 0, /*GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,*/
        GL_RGBA,
        main_window_background.Width (),
        main_window_background.Height (),
        0, GL_RGB, GL_UNSIGNED_BYTE, byte_arr_ptr->operator byte * ());

    GLfloat v[16] {0,0,0,0, 0,1,0,1, 1,0,1,0, 1,1,1,1};
    glGenBuffers (1, &_vbo);
    glBindBuffer (GL_ARRAY_BUFFER, _vbo),
    glBufferData (GL_ARRAY_BUFFER, 16*sizeof(GLfloat), v, GL_STATIC_DRAW);
}

void MainWindow::OnRender()
{
    glClear (GL_COLOR_BUFFER_BIT);
    glLoadIdentity ();
    glScalef (_w, _h, 1);

    glVertexPointer (2, GL_FLOAT, 4*sizeof(GLfloat), (void *)(0));
    glTexCoordPointer (
        2, GL_FLOAT, 4*sizeof(GLfloat), (void *)(2*sizeof(GLfloat)));
    glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);

    // Render the controls, if any
    Window::OnRender ();

    OS::Thread::Sleep (16);//TODO timing
}

void MainWindow::OnResize(int w, int h)
{
    if (! w || ! h) return;
    glViewport (0, 0, _w = w, _h = h);
    glMatrixMode (GL_PROJECTION), glLoadIdentity ();
    // Its a 2D game.
    glOrtho (.0f, _w, _h, .0f, .0f, 1.f);
    // www.opengl.org/archives/resources/faq/technical/transformations.htm
    glTranslatef (.375f, .375f, -.2f);
    glMatrixMode (GL_MODELVIEW), glLoadIdentity ();
}

NAMESPACE_H3R
