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
#include "h3r_game.h"
#include "h3r_def.h"
#include "h3r_log.h"
#include "h3r_string.h"
#include "h3r_sdlrwops.h"
#include "h3r_renderengine.h"

H3R_NAMESPACE

// To function it is.
//TODO widely accessible for other controls.
static byte * LoadSpriteFrame(Def & def,
    const String & def_name, const String & frame_name)
{
    auto frame = def.Query (frame_name);
    if (! frame) frame = def.QueryCI (frame_name);
    if (! frame) {
        H3R_NS::Log::Err (String::Format (
            "Can't find %s[%s]" EOL, def_name.AsZStr (), frame_name.AsZStr ()));
        return nullptr;
    }
    auto byte_arr = frame->ToRGBA ();
    if (! byte_arr || byte_arr->Empty ()) {
        H3R_NS::Log::Err (String::Format (
            "Can't load %s[%s]" EOL, def_name.AsZStr (), frame_name.AsZStr ()));
        return nullptr;
    }
    // printf ("SpriteFrame: %s: %d x %d" EOL,
    //     frame_name.AsZStr (), frame->Width (), frame->Height ());
    return byte_arr->operator byte * ();
}

Button::Button(const String & res_name, Control * base)
    : Control {base}
{
    // Its needed here just to initialize *this size
    //TODO
    Def sprite {Game::GetResource (res_name)};
    Resize (sprite.Width (), sprite.Height ());
    _sprite_name = res_name;
    /*printf ("Sprite: %s: %d x %d" EOL,
         _sprite_name.AsZStr (), sprite.Width (), sprite.Height ());*/
}

Button::~Button()
{
    // This is base Window job now:
    // RenderEngine::UI ().ChangeVisibility (_rkey, false);
}

Button::Button(const String & res_name, Window * base)
    : Control {base}
{
    // Its needed here just to initialize *this size
    //TODO
    Def sprite {Game::GetResource (res_name)};
    Resize (sprite.Width (), sprite.Height ());
    _sprite_name = res_name;
    /*printf ("Sprite: %s: %d x %d" EOL,
         _sprite_name.AsZStr (), sprite.Width (), sprite.Height ());*/
}

void Button::UploadFrames()
{
    auto & RE = RenderEngine::UI ();
    _rkey = RE.GenKey ();
    auto & size = Size ();
    auto & pos = Pos ();
    Def sprite {Game::GetResource (_sprite_name)};

    printf ("[%2d] frame: %s %d %d %d %d, ",
        _rkey, _sprite_name.AsZStr (), pos.X, pos.Y, size.X, size.Y);

    byte * frame = LoadSpriteFrame (sprite, _sprite_name,
        _sprite_name.ToLower ().Replace (".def", "n.pcx"));
    if (frame)
        _on = RE.UploadFrame (_rkey, pos.X, pos.Y, size.X, size.Y, frame, 4,
            sprite.GetUniqueKey (_sprite_name), Depth ());

    frame = LoadSpriteFrame (sprite, _sprite_name,
        _sprite_name.ToLower ().Replace (".def", "h.pcx"));
    if (frame)
        _oh = RE.UploadFrame (_rkey, pos.X, pos.Y, size.X, size.Y, frame, 4,
            sprite.GetUniqueKey (_sprite_name), Depth ());

    frame = LoadSpriteFrame (sprite, _sprite_name,
        _sprite_name.ToLower ().Replace (".def", "s.pcx"));
    if (frame)
        _os = RE.UploadFrame (_rkey, pos.X, pos.Y, size.X, size.Y, frame, 4,
            sprite.GetUniqueKey (_sprite_name), Depth ());
    // printf ("ofs: %d %d %d" EOL, _on, _oh, _os);
}

Control * Button::SetPos(int x, int y)
{
    Control::SetPos (x, y);
    return this;
}

//TODO there are way too many software events happening in response to the HW
//     ones; create the interactive event filter
void Button::OnMouseMove(const EventArgs & e)
{
    static Point p;
    p.X = e.X;
    p.Y = e.Y;
    _mouse_over = HitTest (p);
    RenderEngine & re = RenderEngine::UI ();
    re.ChangeOffset (_rkey,
        _mouse_down ? _os : _mouse_over ? _oh : _on);
    /*Log::Info (String::Format (
        "Button::OnMouseMove (%d, %d), hover: %d, _bb (%d, %d, %d, %d)" EOL,
        p.X, p.Y, _mouse_over,
        ClientRectangle ().Pos.X, ClientRectangle ().Pos.Y,
        ClientRectangle ().Size.X, ClientRectangle ().Size.Y));*/
}

static Mix_Chunk * global_fx1[MIX_CHANNELS] {};

void Button::OnMouseDown(const EventArgs &)
{
    if (! _mouse_over) return;
    _mouse_down = true;
    RenderEngine & re = RenderEngine::UI ();
    re.ChangeOffset (_rkey, _os);
    Log::Info ("MouseDown" EOL);
    auto stream = Game::GetResource ("BUTTON.wav");
    if (! stream) {
        Log::Err ("Can't load BUTTON.wav" EOL);
        return;
    }
    SdlRWopsStream wav_ops {stream};
    int freesrc = true;
    Mix_Chunk * wav_chunk = Mix_LoadWAV_RW (wav_ops, freesrc);
    if (! wav_chunk) {
        Log::Err ("Mix_LoadWAV_RW: can't load BUTTON.wav" EOL);
        return;
    }
    //TODO This complicates things a little bit
    Mix_ChannelFinished ([](int c) {
        if (global_fx1[c]) {
            printf ("Mix_FreeChunk at global_fx1" EOL);
            Mix_FreeChunk (global_fx1[c]);
            global_fx1[c] = nullptr;
        }
    });
    global_fx1[Mix_PlayChannel (-1, wav_chunk, 0)] = wav_chunk;
}

void Button::OnMouseUp(const EventArgs &)
{
    //TODO Chain of Responsibility can really come in handy here. You clicked on
    //     one control - that's it: the others need not get notified. I don't
    //     need the bubbling/tunneling madness of the "WPF" here.
    if (! _mouse_down) return; // It was not me
    _mouse_down = false;
    RenderEngine & re = RenderEngine::UI ();
    re.ChangeOffset (_rkey, _mouse_over ? _oh : _on);
    // If you release the button outside of the of the thing you clicked on
    // there shall be no mouse click event.
    if (! _mouse_over) return;
    // Log::Info ("MouseUp" EOL);
    Click (nullptr);
}

NAMESPACE_H3R
