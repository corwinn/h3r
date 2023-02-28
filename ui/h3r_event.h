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

// Custom events.

#ifndef _H3R_EVENT_H_
#define _H3R_EVENT_H_

#include "h3r.h"
#include "h3r_list.h"
#include "h3r_log.h"

H3R_NAMESPACE

struct EventArgs;

// You implement this at the class that shall handle events.
struct IHandleEvents {};

namespace {

using EventHandler = void (IHandleEvents::*)(EventArgs * args);
// Binder. UI only, so thread-unsafe.
template <typename Obj> class McastD final
{
    public using EventHandler = void (Obj::*)(EventArgs * args);
    private struct Delegate final
    {
        Delegate(EventHandler eh, Obj * object)
            : EH{eh}, Object{object} {}
        ~Delegate() { Object = {}; EH = {}; }
        EventHandler EH;
        Obj * Object;
        bool operator==(const Delegate & b) const
        {
            return EH == b.EH && Object == b.Object;
        }
    };
    private List<Delegate> _subscribers {};
    public McastD() {}
    public ~McastD() { for (auto & d : _subscribers) d.~Delegate (); }
    public template <typename T> McastD & Subscribe(Obj * obj, T eh)
    {
        Delegate d {static_cast<EventHandler>(eh), obj};
        if (! _subscribers.Contains (d))
            _subscribers.Add (d);
        else
            Log::Info ("Warning: subscribing twice isn't supported" EOL);
        return *this;
    }
    public void operator()(EventArgs * args)
    {
        for (auto & d : _subscribers)
            if (d.Object && d.EH) (d.Object->*d.EH) (args);
    }
};// Multi-cast Delegate
}

// Use this at the event providers: Event OnFoo; notify: OnFoo ();
using Event = McastD<IHandleEvents>;

//LATER merge this, above.
//
// class ShowProgress : public Event
// {
//     void Do(class Control * c) override
//     {
//         if (Handled ()) ShowPorgress (c); else Event::Do (c);
//     }
//     bool Handled() override { return _dialog_window != nullptr; }
// }
// To be a Chain of Responsibility or not to be? Was that a question?
// Confusing a little bit. Its event for the one invoking it a.k.a. the Subject;
// its event handler for the one receiving it a.k.a the Observer.
// Control is not the "C" at the "MVC" - its just a basic "sender" type a.k.a
// the one who sent the event. For non-UI events that shall be null obviously.
// I hope this won't become a showstopper at some point, since I'm combining
// one too many responsibilities in this single class.
/*class Event
{
    // Do you know how hard is to turn this into a multi-cast delegate?
    // Not at all.
    //LATER
    // public: void Attach(Event *); // aka foo.Changed += bar
    // public: void Detach(Event *); // aka foo.Changed -= bar
    private Event * _next;
    public Event(Event * next_eh = nullptr)
        : _next{next_eh} {}
    public inline virtual void Do(
        class Control & sender, struct EventArgs & args)
    {
        if (_next) _next->Do (sender, args);
    }
    // Implement this when your object has "doubts" about handling something
    public inline virtual bool Handled() { return true; }
    // Why is this "virtual" eludes me still. Perhaps on the next re-read.
    public inline virtual void SetNext(Event * next_eh) { _next = next_eh; }
};*/

// The book responsible for code like the above one:
// "Design Patterns: Elements of Reusable Object-Oriented Software"

NAMESPACE_H3R

#endif
