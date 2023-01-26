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

#ifndef _H3R_SDLWINDOW_H_
#define _H3R_SDLWINDOW_H_

#include "h3r.h"
#include "h3r_log.h"
#include "h3r_oswindow.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include "h3r_game.h"
#include "h3r_resdecoder.h"

H3R_NAMESPACE

// SDL is a multi-OS bridge itself, so there is no need for OS-specific classes
// yet.
// Its this deep in the directory tree on purpose.
//LATER plug-in; an SDL window shouldn't concern itself with h3r at all.
class SDLWindow : public OSWindow
{
#define public public:
#define private private:
#define protected protected:

    private SDL_Event _e;
    private SDL_Window * _window {};
    private SDL_GLContext _gc {};
    private int _w {800}, _h{600};
    private bool _q {false};
    private bool _visible {false};

    // Open GL state
    GLuint _tex, _vbo;

    protected inline virtual void Hide() override {}

    protected inline virtual void Show() override
    {
        // 2.0 should be enough for a proof of concept
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, 0);

        _window = SDL_CreateWindow ("h3r",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, _w, _h,
            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        if (! _window) {
             H3R_NS::Log::Info (H3R_NS::String::Format (
                "SDL_CreateWindow error: %s" EOL, SDL_GetError ()));
             return;
        }

        _gc = SDL_GL_CreateContext (_window);
        if (! _gc) {
             H3R_NS::Log::Info (H3R_NS::String::Format (
                "SDL_GL_CreateContext error: %s" EOL, SDL_GetError ()));
             return;
        }

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
            H3R_NS::Log::Info ("Failed to load GamSelBk.pcx");
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

        Resized ();
        Render ();//TODO SDL_WINDOWEVENT_EXPOSED gets lost sometimes?
    }// Show

    protected inline virtual void Close() override {}

    private void Render();
    private void Resized();

    public void ProcessMessages() override;

    public SDLWindow(int, char **);

    public ~SDLWindow();
};

#undef public
#undef private
#undef protected

NAMESPACE_H3R

#endif