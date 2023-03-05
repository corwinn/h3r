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
#include "h3r_window.h"
#include "h3r_textrenderingengine.h"

H3R_NAMESPACE

static bool InitBitmap(Button::TempSpriteData *& tsd, Def & sprite,
    const String & sprite_name, String && frame_name)
{
    H3R_CREATE_OBJECT(tsd, Button::TempSpriteData);
    auto frame = sprite.Query (frame_name);
    if (! frame) {
        H3R_NS::Log::Err (String::Format ("Can't find %s[%s]" EOL,
            sprite_name.AsZStr (), frame_name.AsZStr ()));
        return false;
    }
    tsd->TopLeft.Left = sprite.Left ();
    tsd->TopLeft.Top = sprite.Top ();
    tsd->UniqueKey = sprite.GetUniqueKey (sprite_name);
    auto byte_arr = sprite.ToRGBA ();
    if (! byte_arr || byte_arr->Empty ()) {
        H3R_NS::Log::Err (String::Format ("Can't load %s[%s]" EOL,
            sprite_name.AsZStr (), frame_name.AsZStr ()));
        return false;
    }
    tsd->Bitmap = *byte_arr;
    return true;
}

void Button::Init(const String & res_name, int flags)
{
    Def sprite {Game::GetResource (res_name)};
    Resize (sprite.Width (), sprite.Height ());
    H3R_ENSURE(flags > 0 && flags < 16, "Odd flags")
    if (flags & H3R_UI_BTN_UP)
        H3R_ENSURE (InitBitmap (_tsdn, sprite, res_name,
            res_name.ToLower ().Replace (".def", "n.pcx")), "no n.pcx")
    if (flags & H3R_UI_BTN_DOWN)
        H3R_ENSURE (InitBitmap (_tsds, sprite, res_name,
            res_name.ToLower ().Replace (".def", "s.pcx")), "no s.pcx")
    if (flags & H3R_UI_BTN_HOVER)
        H3R_ENSURE (InitBitmap (_tsdh, sprite, res_name,
            res_name.ToLower ().Replace (".def", "h.pcx")), "no h.pcx")
    if (flags & H3R_UI_BTN_GRAYOUT)
        H3R_ENSURE (InitBitmap (_tsdd, sprite, res_name,
            res_name.ToLower ().Replace (".def", "d.pcx")), "no d.pcx")
}

Button::Button(const String & res_name, Control * base, int flags)
    : Control {base}
{
    Init (res_name, flags);
}

Button::~Button() {}

Button::Button(const String & res_name, Window * base,  int flags)
    : Control {base}
{
    Init (res_name, flags);
}

void Button::UploadFrames()
{
    auto RE = Window::UI;
    _rkey = RE->GenKey ();
    auto & size = Size ();
    auto & pos = Pos ();

    static Array<byte> * bitmap {};
    auto bitmap_data = []() { return bitmap->operator byte * (); };

    Button::TempSpriteData * f[4] = {_tsdn, _tsdh, _tsds, _tsdd};
    int *                    g[4] = { &_on,  &_oh,  &_os,  &_od};

    for (int i = 0; i < 4; i++)
        if (f[i]) {
            bitmap = &(f[i]->Bitmap);
            *(g[i]) = RE->UploadFrame (_rkey, pos.X - f[i]->TopLeft.Left,
                pos.Y - f[i]->TopLeft.Top, size.X, size.Y, bitmap_data,
                h3rBitmapFormat::RGBA, f[i]->UniqueKey, Depth ());
            H3R_DESTROY_NESTED_OBJECT(
                f[i], Button::TempSpriteData, TempSpriteData)
        }
}

//TODO there are way too many software events happening in response to the HW
//     ones; create the interactive event filter:
//     All events are Enqueued. At a certain time interval (this game doesn't
//     require very fast keyboard/mouse switching response), they're all
//     dequeued in a distinct manner, and sent around. Why "manner"? - because
//     if you managed to move the mouse 10 times for 5 ms (like moving 10 mice
//     at once), there is no point to notify 10 times, but just once - by
//     computing the resulting move vector.
//     If the above idea proves inadequate, just filter out equivalent events
//     that happen within a constant time interval: say 50 mouse events with
//     state not changed in < 1ms - notify just once.
void Button::OnMouseMove(const EventArgs & e)
{
    static Point p;
    p.X = e.X;
    p.Y = e.Y;
    _mouse_over = HitTest (p);
    auto re = Window::UI;
    // Why is there no _flags check? - see the explanation at OnMouseDown().
    re->ChangeOffset (_rkey,
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
    auto re = Window::UI;
    // By default all offsets are "_on", and H3R_ENSURE ensures it exists.
    // So even if "_os" wasn't uploaded, "_on" shall be rendered.
    re->ChangeOffset (_rkey, _os);
    if (_lbl) { _lbl->SetHidden (true); _lbl_dn->SetHidden (false); }
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
    auto re = Window::UI;
    re->ChangeOffset (_rkey, _mouse_over ? _oh : _on);
    if (_lbl) { _lbl->SetHidden (false); _lbl_dn->SetHidden (true); }
    // If you release the button outside of the of the thing you clicked on
    // there shall be no mouse click event.
    if (! _mouse_over) return;
    // Log::Info ("MouseUp" EOL);
    Click (nullptr);
}

void Button::OnVisibilityChanged()
{
    if (Hidden ()) {
        Window::UI->ChangeOffset (_rkey, _on);
        _mouse_over = _mouse_down = false;
    }
    Window::UI->ChangeVisibility (_rkey, ! Hidden ());
}

void Button::OnMoved(int dx, int dy)//TODO UploadFrames() is no longer needed
{
    if (! _rkey) return; // not initialized yet
    auto RE = Window::UI;
    RE->UpdateLocation (_rkey, dx, dy);
}

void Button::SetText(const String & text, const String & font, unsigned int c)
{
    if (_lbl) {
        _lbl->SetText (text);
        _lbl_dn->SetText (text);
        return;
    }
    Point ts = TextRenderingEngine::One ().MeasureText (font, text);
    // TODO I have yet to find out, how the game does this:
    //      right now my cx and cy are 440, 86; the game ones are 440, 85
    int cx = Pos ().X + (Width () - ts.Width) / 2,   // HCenter
        cy = Pos ().Y + (Height () - ts.Height) / 2; // VCenter
    H3R_CREATE_OBJECT(_lbl, Label) {text, font, Point {cx, cy-1}, this, c};
    H3R_CREATE_OBJECT(_lbl_dn, Label) {text, font, Point {cx+1, cy}, this, c};
    _lbl_dn->SetHidden (true);
}

NAMESPACE_H3R
