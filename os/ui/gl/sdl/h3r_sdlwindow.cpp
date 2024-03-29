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

#include "h3r_sdlwindow.h"
#if defined(_WIN32) && !defined(H3R_GL_DUMMY_CONTEXT)
#include "h3r_gl.h"
#endif

namespace __pointless_verbosity
{
    struct _bool_finally_invert final
    {
        bool & _b;
        _bool_finally_invert(bool & b) : _b{b} {}
        ~_bool_finally_invert() { _b = ! _b; }
    };
}

#define THE_WAY_IS_SHUT \
    static bool _you_shall_not_pass {}; \
    H3R_ENSURE(! _you_shall_not_pass, "fix: your event-driven mess") \
    _you_shall_not_pass = true; \
    __pointless_verbosity::_bool_finally_invert ____ {_you_shall_not_pass};

// Something like this shall be done at the plug-in code.
static bool global_sdl_init {};
static bool global_sdl_mix_init {};
static void Init_SDL()
{
    if (SDL_Init (SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        H3R_NS::Log::Err (H3R_NS::String::Format (
            "SDL_Init error: %s. Stop." EOL, SDL_GetError ()));
        H3R_NS::OS::Exit (1);
    }
    global_sdl_init = true;

    //TODO audio init; requires OS VFS
    int audio_result = 0;
    int audio_flags = MIX_INIT_MP3;
    if (audio_flags != (audio_result = Mix_Init (audio_flags))) {
        H3R_NS::Log::Err (H3R_NS::String::Format (
            "Mix_Init error: %s. The rest is silence." EOL, Mix_GetError ()));
        return;
    }
    if (-1 == Mix_OpenAudio (44100, AUDIO_S16SYS, 2, 1024)) {
        H3R_NS::Log::Info (H3R_NS::String::Format (
            "Mix_OpenAudio error: %s. The rest is silence." EOL,
            Mix_GetError ()));
        return;
    }
    // Ok, the docs state that SDL_mixer allocates MIX_CHANNELS (currently 8)
    // channels when you open an audio device. That should be enough for this
    // game:
    //   * I can hardly imagine use case where more than 2 are being used
    // The SDL provides this nice struct:
    // SDL_RWops
    // It has function pointers, so I can create SDL_RWops_Stream.
    // The SDL_RWops is a parameter to Mix_Chunk * Mix_LoadWAV_RW().
    // The chunk is parameter to:
    // "int Mix_PlayChannel(int channel, Mix_Chunk *chunk, int loops);" and
    // "Mix_PlayChannel (-1"... "play on the first free channel".
    // The picture is quite clear. It can play directly from the res manager
    // via a RefReadStream, in its own thread; IPC: {res_name; command; channel}
    // This one requires a Queue.
    // With auto cross-fade based on channel - requires predefined channel
    // numbers: battle_fx:2, env:4 (the odd being using for the cross-fade);
    // could work - one way to find out.
    // Audio Manager is coming to mind - as the Tex Cache.
    // Continued at Done at SDLWindow::SDLWindow()
}// Init_SDL

H3R_NAMESPACE

SDLWindow::SDLWindow(int, char **, Point && size)
    : OSWindow (0, nullptr), _w{size.X}, _h{size.Y}
{
    if (! global_sdl_init) Init_SDL ();
    Mix_Music * music = Mix_LoadMUS ("MP3/MAINMENU.MP3");
    if (music) Mix_PlayMusic (music, -1);
    global_sdl_mix_init = true;

    // 2.0 should be enough for a proof of concept
    //TODO request this from the abstraction
    //TODO all SDL_GL_SetAttribute and SDL_GL_GetAttribute need error checking.
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // These two are casuing new memory leak:
    //   glXChooseFBConfigSGIX @ libGL.so - thats "mesa 3d"
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK,
        SDL_GL_CONTEXT_PROFILE_CORE);
    //    SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

    SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute (SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute (SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute (SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute (SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute (SDL_GL_ALPHA_SIZE, 8);

    _window = SDL_CreateWindow ("h3r v0.2-alpha",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, _w, _h,
        SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN /*| SDL_WINDOW_RESIZABLE*/);
    if (! _window) {
        H3R_NS::Log::Err (H3R_NS::String::Format (
            "SDL_CreateWindow error: %s" EOL, SDL_GetError ()));
        return;
    }

    // Set mouse cursor - something has to provide the bitmap data:
    //   SDL_CreateRGBSurfaceFrom
    //   SDL_CreateColorCursor

    _gc = SDL_GL_CreateContext (_window);
    if (! _gc) {
        H3R_NS::Log::Err (H3R_NS::String::Format (
            "SDL_GL_CreateContext error: %s" EOL, SDL_GetError ()));
        return;
    }

    if (-1 == SDL_GL_SetSwapInterval (1)) // VSYNC
        H3R_NS::Log::Err (H3R_NS::String::Format (
            "Failed to set vsync:on : %s" EOL, SDL_GetError ()));

    int attr;
    printf ("SDL2: Open GL response: " EOL);
    SDL_GL_GetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, &attr);
    printf ("  OpenGL %d.", attr);
    SDL_GL_GetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, &attr);
    printf ("%d" EOL, attr);
    SDL_GL_GetAttribute (SDL_GL_DOUBLEBUFFER, &attr);
    printf ("  Double-buffered: %s" EOL, (attr ? "true" : "false"));
    SDL_GL_GetAttribute (SDL_GL_ACCELERATED_VISUAL, &attr);
    printf ("  HW Accelration: %s" EOL, (attr ? "true" : "false"));
    SDL_GL_GetAttribute (SDL_GL_RED_SIZE, &attr);
    printf ("  RGBA: %d", attr);
    SDL_GL_GetAttribute (SDL_GL_GREEN_SIZE, &attr);
    printf (",%d", attr);
    SDL_GL_GetAttribute (SDL_GL_BLUE_SIZE, &attr);
    printf (",%d", attr);
    SDL_GL_GetAttribute (SDL_GL_ALPHA_SIZE, &attr);
    printf (",%d" EOL, attr);
    printf ("  V-sync: %d" EOL, SDL_GL_GetSwapInterval ());
    SDL_GL_GetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, &attr);
    printf ("  Profile: ");
    switch (attr) {
        case SDL_GL_CONTEXT_PROFILE_CORE: printf ("Core"); break;
        case SDL_GL_CONTEXT_PROFILE_COMPATIBILITY: printf ("Compat"); break;
        case SDL_GL_CONTEXT_PROFILE_ES: printf ("ES"); break;
        default: printf ("a mystery"); break;
    }
    printf (EOL);

#if defined(_WIN32) && !defined(H3R_GL_DUMMY_CONTEXT)
    //TODO SDL_GL_GetProcAddress
    H3R_ENSURE(H3rGL_Init (), "OpenGL init failed")
#endif
}// SDLWindow::SDLWindow()

SDLWindow::~SDLWindow()
{
    if (_music) Mix_FreeMusic (_music);
    ClearMouseCursor ();
    SDL_Quit ();
}

void SDLWindow::Show()
{
    THE_WAY_IS_SHUT

    _initialized = false;
    if (_visible || ! _window || ! _gc) return;
    SDL_ShowWindow (_window);
    _visible = true;

    OnShow (); //TODO OnShow called Render() bellow and OnRender () tried using
               //     uninitialized state;
    Resized ();//     there are a few things to think about here
    _initialized = true; //TODO shall this block Resized() too?

    Render ();//TODO SDL_WINDOWEVENT_EXPOSED gets lost sometimes?

}// Show

void SDLWindow::Render()
{
    if (! _initialized) return;
    THE_WAY_IS_SHUT // If you're here: don't async-load resources at OnRender :)
    // Loading resources at OnRender() means you did wrong; resources shall be
    // loaded elsewhere. OnRender() has extremely heavy task, so ...

    if (! _gc || ! _visible) return;
    OnRender ();
    SDL_GL_SwapWindow (_window);
}

void SDLWindow::Resized()
{
    THE_WAY_IS_SHUT

    OnResize (_w, _h);
}

void SDLWindow::ProcessMessages()
{
    static EventArgs e {};
    // SDL_PumpEvents ();
    if (_q) return;
    while (SDL_PollEvent (&_e) != 0) {
        if (_q) continue;
        if (SDL_QUIT == _e.type) {
            OnClose (this, _q);
            if (_q) Close ();//TODO redundant
        }
        if (_q) continue; //TODO should I?

        switch (_e.type) {
            case SDL_WINDOWEVENT: HandleWindowEvent (); break;
            case SDL_KEYUP: HandleKeyboardEvent (e); break;
            case SDL_KEYDOWN: HandleKeyboardEventDown (e); break;
            case SDL_MOUSEMOTION: HandleMouseMotionEvent (e); break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: HandleMouseButtonEvent (e); break;
        }
    }
}// ProcessMessages

bool SDLWindow::Idle() { return ! SDL_PollEvent (nullptr); }

void SDLWindow::HandleWindowEvent()
{
    THE_WAY_IS_SHUT

    // H3R_NS::Log::Info ("SDL_WINDOWEVENT" EOL);
    switch (_e.window.event) {
        case SDL_WINDOWEVENT_RESIZED: {
            if (_e.window.data1 > 0 && _e.window.data2 > 0)
                _w = _e.window.data1, _h = _e.window.data2, Resized ();
        } break;
        // The game is rendering using target FPS due to its animations, so this
        // is redundant and will cause disturbance in measurements.
        // case SDL_WINDOWEVENT_EXPOSED: Render (); break;
    }
}

void SDLWindow::HandleKeyboardEvent(EventArgs & e)
{
    THE_WAY_IS_SHUT

    switch (_e.key.keysym.scancode) {
        case SDL_SCANCODE_Q:
            { e.Key = H3R_KEY_Q; OnKeyUp (e); e.Key = H3R_NO_KEY; } break;
        case SDL_SCANCODE_ESCAPE:
            { e.Key = H3R_KEY_ESC; OnKeyUp (e); e.Key = H3R_NO_KEY; } break;
        case SDL_SCANCODE_UP:
            { e.Key = H3R_KEY_ARROW_UP; OnKeyUp (e); e.Key = H3R_NO_KEY; }
        break;
        case SDL_SCANCODE_DOWN:
            { e.Key = H3R_KEY_ARROW_DN; OnKeyUp (e); e.Key = H3R_NO_KEY; }
        break;
        case SDL_SCANCODE_KP_9:
        case SDL_SCANCODE_PAGEUP:
            { e.Key = H3R_KEY_PGUP; OnKeyUp (e); e.Key = H3R_NO_KEY; } break;
        case SDL_SCANCODE_KP_3:
        case SDL_SCANCODE_PAGEDOWN:
            { e.Key = H3R_KEY_PGDN; OnKeyUp (e); e.Key = H3R_NO_KEY; } break;
        default: break;
    }
}

void SDLWindow::HandleKeyboardEventDown(EventArgs & e)
{
    THE_WAY_IS_SHUT

    switch (_e.key.keysym.scancode) {
        case SDL_SCANCODE_Q:
            { e.Key = H3R_KEY_Q; OnKeyDown (e); e.Key = H3R_NO_KEY; } break;
        case SDL_SCANCODE_ESCAPE:
            { e.Key = H3R_KEY_ESC; OnKeyDown (e); e.Key = H3R_NO_KEY; } break;
        case SDL_SCANCODE_UP:
            { e.Key = H3R_KEY_ARROW_UP; OnKeyDown (e); e.Key = H3R_NO_KEY; }
        break;
        case SDL_SCANCODE_DOWN:
            { e.Key = H3R_KEY_ARROW_DN; OnKeyDown (e); e.Key = H3R_NO_KEY; }
        break;
        case SDL_SCANCODE_KP_9:
        case SDL_SCANCODE_PAGEUP:
            { e.Key = H3R_KEY_PGUP; OnKeyDown (e); e.Key = H3R_NO_KEY; } break;
        case SDL_SCANCODE_KP_3:
        case SDL_SCANCODE_PAGEDOWN:
            { e.Key = H3R_KEY_PGDN; OnKeyDown (e); e.Key = H3R_NO_KEY; } break;
        default: break;
    }
}

void SDLWindow::HandleMouseMotionEvent(EventArgs & e)
{
    THE_WAY_IS_SHUT

    //TODO _e.motion.which; the mouse instance id
    //     Please don't play with more than one mouse simultaneously :)

    e.X = _e.motion.x;
    e.Y = _e.motion.y;
    OnMouseMove (e);
    e.X = e.Y = 0;
}

void SDLWindow::HandleMouseButtonEvent(EventArgs & e)
{
    // Handling a dialog in OnMouse event requires re-entry.
    // I'm opening this gate. Stay tuned for stack overflows.
    // THE_WAY_IS_SHUT

    //TODO _e.motion.which; the mouse instance id
    //     Please don't play with more than one mouse simultaneously :)
    switch (_e.button.button) {
        case SDL_BUTTON_LEFT: e.Button = H3R_MKEY_LEFT; break;
        default: break; // TODO handle the others
    }
    e.X = _e.button.x;
    e.Y = _e.button.y;
    switch (_e.button.state) {
        case SDL_PRESSED: OnMouseDown (e); break;
        case SDL_RELEASED: OnMouseUp (e); break;
        default: H3R_NS::Log::Info ("Unhandled mouse button state" EOL); break;
    }
    e.X = e.Y = 0;
}

namespace __pointless_verbosity {
struct try_finally_sdl_surface
{
    SDL_Surface *& state;
    try_finally_sdl_surface(SDL_Surface *& s) : state{s} {}
    ~try_finally_sdl_surface()
    {
        if (nullptr != state) {
            SDL_FreeSurface (state);
            state = nullptr;
        }
    }
};}

void SDLWindow::ClearMouseCursor()
{
    if (nullptr != _mouse_cursor) {
        SDL_FreeCursor (_mouse_cursor);// will the program break at this point?
        _mouse_cursor = nullptr;
    }
}

void SDLWindow::SetMouseCursor(IWindow::MousePtrInfo & info)
{
    int bpp = h3rBitmapFormat::RGB == info.BitmapFormat ? 24 : 32;
    auto s = SDL_CreateRGBSurfaceFrom (info.Bitmap, info.Width, info.Height,
        bpp, (bpp>>3)*info.Width, 0x000000ffu, 0x0000ff00u, 0x00ff0000u,
        0xff000000u);
    if (nullptr == s) {
        H3R_NS::Log::Err (H3R_NS::String::Format (
            "SDL_CreateRGBSurfaceFrom error: %s" EOL, SDL_GetError ()));
        return;
    }
    __pointless_verbosity::try_finally_sdl_surface ____ {s};
    ClearMouseCursor ();
    //TODO parametrize mouse hotspot
    auto cursor = SDL_CreateColorCursor (s, 0, 0);
    if (nullptr == cursor) {
        H3R_NS::Log::Err (H3R_NS::String::Format (
            "SDL_CreateColorCursor error: %s" EOL, SDL_GetError ()));
        return;
    }
    SDL_SetCursor (cursor);
}

NAMESPACE_H3R