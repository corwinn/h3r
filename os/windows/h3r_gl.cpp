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

//LATER clarify __MINGW32__ vs __MINGW64__
#if __MINGW32__
#include <windows.h>
#include <GL/gl.h>
// #undef GL_GLEXT_PROTOTYPES
#include <GL/glext.h>
#include <stdio.h>
#include <stdlib.h>

extern "C" {
// Use their declarations:
//  - there are calling conventions hidden under the hood
//  - you get compiler-notified on signature changes
//TODO codegen starts here
PFNGLGENBUFFERSPROC pglGenBuffers {};
void glGenBuffers(GLsizei a, GLuint * b) { pglGenBuffers (a, b); }
PFNGLBINDBUFFERPROC pglBindBuffer {};
void glBindBuffer(GLenum a, GLuint b) { pglBindBuffer (a, b); }
PFNGLBUFFERDATAPROC pglBufferData {};
void glBufferData(GLenum a, GLsizeiptr b, const void * c, GLenum d)
{
    pglBufferData (a, b, c, d);
}
PFNGLDELETEBUFFERSPROC pglDeleteBuffers {};
void glDeleteBuffers(GLsizei a, const GLuint * b)
{
    pglDeleteBuffers (a, b);
}
PFNGLMULTIDRAWARRAYSPROC pglMultiDrawArrays {};
void glMultiDrawArrays(GLenum mode, const GLint * first,
    const GLsizei * count, GLsizei drawcount)
{
    pglMultiDrawArrays (mode, first, count, drawcount);
}
PFNGLBUFFERSUBDATAPROC pglBufferSubData {};
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size,
    const void * data)
{
    pglBufferSubData (target, offset, size, data);
}
PFNGLGETBUFFERSUBDATAPROC pglGetBufferSubData {};
void glGetBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size,
    void * data)
{
    pglGetBufferSubData (target, offset, size, data);
}
PFNGLGETBUFFERPARAMETERIVPROC pglGetBufferParameteriv {};
void glGetBufferParameteriv(GLenum target, GLenum value, GLint * data)
{
    pglGetBufferParameteriv (target, value, data);
}
/*PFNGLENABLEVERTEXATTRIBARRAYPROC pglEnableVertexAttribArray {};
void glEnableVertexAttribArray(GLuint index)
{
    pglEnableVertexAttribArray (index);
}*/
/*GLuint (*pglCreateShader)(GLenum){};
 GLuint glCreateShader(GLenum a) {return pglCreateShader (a);}
void (*pglDeleteShader)(GLuint){};
 void glDeleteShader(GLuint a) {pglDeleteShader (a);}
void (*pglShaderSource)(GLuint, GLsizei, const GLchar * const *,
    const GLint *){};
 void glShaderSource(GLuint a, GLsizei b, const GLchar * const * c,
    const GLint * d) { pglShaderSource (a, b, c, d);}
void (*pglCompileShader)(GLuint){};
 void glCompileShader(GLuint a) {pglCompileShader (a);}
void (*pglGetShaderiv)(GLuint, GLenum, GLint *){};
 void glGetShaderiv(GLuint a, GLenum b, GLint * c)
    {pglGetShaderiv (a, b, c);}
void (*pglGetShaderInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *){};
 void glGetShaderInfoLog(GLuint a, GLsizei b, GLsizei * c, GLchar * d)
    {pglGetShaderInfoLog (a, b, c, d);}
GLuint (*pglCreateProgram)(){};
 GLuint glCreateProgram() {return pglCreateProgram ();}
void (*pglAttachShader)(GLuint, GLuint){};
 void glAttachShader(GLuint a, GLuint b) {pglAttachShader (a, b);}
void (*pglLinkProgram)(GLuint){};
 void glLinkProgram(GLuint a) {pglLinkProgram (a);}
void (*pglGetProgramiv)(GLuint, GLenum, GLint *){};
 void glGetProgramiv(GLuint a, GLenum b, GLint * c)
    {pglGetProgramiv (a, b, c);}
void (*pglGetProgramInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *){};
 void glGetProgramInfoLog(GLuint a, GLsizei b, GLsizei * c, GLchar * d)
    {pglGetProgramInfoLog (a, b, c, d);}
void (*pglUseProgram)(GLuint){};
 void glUseProgram(GLuint a) {pglUseProgram (a);}
GLint (*pglGetUniformLocation)(GLuint, const GLchar *){};
 GLint glGetUniformLocation(GLuint a, const GLchar * b)
    {return pglGetUniformLocation (a, b);}
void (*pglUniform1i)(GLint, GLint){};
 void glUniform1i(GLint a, GLint b) {pglUniform1i (a, b);}
void (*pglActiveTexture)(GLenum){};
 void glActiveTexture(GLenum a) {pglActiveTexture (a);}
void (*pglClientActiveTexture)(GLenum){};
 void glClientActiveTexture(GLenum a) {pglClientActiveTexture (a);}
void (*pglCompressedTexImage2D)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint,
    GLsizei, const void *){};
 void glCompressedTexImage2D(GLenum a, GLint b, GLenum c, GLsizei d,
    GLsizei e, GLint f, GLsizei g, const void * h)
    {pglCompressedTexImage2D (a, b, c, d, e, f, g, h);}*/
}

namespace {
template <typename T> struct NoRef final {typedef T type;};
template <typename T> struct NoRef<T&> final {typedef T type;};

auto Init_GL_proc = [](const char * n, auto & p) -> bool
{
    /*p = reinterpret_cast<typename NoRef<decltype(p)>::type>(
        GetProcAddress (h, n));*/
    if (p) return printf ("Reinit: %s\r\n", n), true;
    auto t = wglGetProcAddress (n);
    if (t) printf ("Got *%s\r\n", n);
    else printf ("GetLastError(): %d\r\n", GetLastError ());
    p = reinterpret_cast<typename NoRef<decltype(p)>::type>(t);
    if (! p) printf ("GetProcAddress(%s) failed\r\n", n);
    return nullptr != p;
};
}

struct fun2 final
{
    HGLRC _s;
    HWND _w;
    HDC _d;
    fun2(HGLRC s, HWND w, HDC d) : _s{s}, _w{w}, _d{d} {}
    ~fun2()
    {
        if (_s) wglMakeCurrent (nullptr, nullptr),
            wglDeleteContext (_s);
        if (_w && _d) ReleaseDC (_w, _d);
        if (_w) DestroyWindow (_w);
    }
};

bool H3rGL_Init()
{
// define it so the procs will be initialized at main() line 1
#ifdef H3R_GL_DUMMY_CONTEXT
    // what follows is scary; and required so wglGetProcAddress works

    // nice shortcut being responded to with ERROR_INVALID_PIXEL_FORMAT
    // auto hdc = GetDC (nullptr);

    // create a window, because, well just create it because wglGetProcAddress()
    WNDCLASS wc {};
    wc.hInstance = GetModuleHandle (nullptr); // this shouldn't fail does it?
    wc.lpfnWndProc = DefWindowProc;
    wc.lpszClassName = "H3rGL";
    if (! RegisterClass (&wc))
        return printf ("RegisterClass failed\r\n"), false;
    auto w = CreateWindow ("H3rGL", "H3rGL", 0,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, nullptr, GetModuleHandle (nullptr), nullptr);
    if (! w ) return printf ("CreateWindow failed\r\n"), false;
    // no it doesn't end here
    auto hdc = GetDC (w);
    if (! hdc) return printf ("GetDC failed\r\n"), false;
    PIXELFORMATDESCRIPTOR pf {};
    pf.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pf.nVersion = 1;
    pf.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    auto pf4 = ChoosePixelFormat (hdc, &pf);
    if (! pf4) return printf ("ChoosePixelFormat failed\r\n"), false;
    if (! SetPixelFormat (hdc, pf4, &pf))
        return printf ("SetPixelFormat failed\r\n"), false;
    // here it comes ...
    auto gldc = wglCreateContext (hdc);
    if (! gldc) return printf ("wglCreateContext failed\r\n"), false;
    if (! wglMakeCurrent (hdc, gldc))
        return printf ("wglMakeCurrent failed\r\n"), false;
    fun2 __ {gldc, w, hdc};
#endif

    //TODO codegen continues here
    return
           Init_GL_proc ("glGenBuffers", pglGenBuffers)
        && Init_GL_proc ("glBindBuffer", pglBindBuffer)
        && Init_GL_proc ("glBufferData", pglBufferData)
        && Init_GL_proc ("glDeleteBuffers", pglDeleteBuffers)
        && Init_GL_proc ("glMultiDrawArrays", pglMultiDrawArrays)
        && Init_GL_proc ("glBufferSubData", pglBufferSubData)
        && Init_GL_proc ("glGetBufferSubData", pglGetBufferSubData)
        && Init_GL_proc ("glGetBufferParameteriv", pglGetBufferParameteriv)
        /*&& Init_GL_proc ("glEnableVertexAttribArray",
            pglEnableVertexAttribArray)
        && Init_GL_proc ("glCreateShader", pglCreateShader)
        && Init_GL_proc ("glDeleteShader", pglDeleteShader)
        && Init_GL_proc ("glShaderSource", pglShaderSource)
        && Init_GL_proc ("glCompileShader", pglCompileShader)
        && Init_GL_proc ("glGetShaderiv", pglGetShaderiv)
        && Init_GL_proc ("glGetShaderInfoLog", pglGetShaderInfoLog)
        && Init_GL_proc ("glCreateProgram", pglCreateProgram)
        && Init_GL_proc ("glAttachShader", pglAttachShader)
        && Init_GL_proc ("glLinkProgram", pglLinkProgram)
        && Init_GL_proc ("glGetProgramiv", pglGetProgramiv)
        && Init_GL_proc ("glGetProgramInfoLog", pglGetProgramInfoLog)
        && Init_GL_proc ("glUseProgram", pglUseProgram)
        && Init_GL_proc ("glGetUniformLocation", pglGetUniformLocation)
        && Init_GL_proc ("glUniform1i", pglUniform1i)
        && Init_GL_proc ("glActiveTexture", pglActiveTexture)
        && Init_GL_proc ("glClientActiveTexture", pglClientActiveTexture)
        && Init_GL_proc ("glCompressedTexImage2D", pglCompressedTexImage2D)*/
        ;
    // glGenBuffers = reinterpret_cast<decltype(glGenBuffers)>(
    //     GetProcAddress (r, "glGenBuffers"));
}

#else

bool H3rGL_Init() { return true; }

#endif