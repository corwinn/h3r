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

/*
  The storm engine is a really simple one: a few flying trees nicely
 interwoven into a priority-queued chaos graph.
*/

#ifndef _H3R_GAME_OBJECT_H_
#define _H3R_GAME_OBJECT_H_

#include "h3r.h"
#include "h3r_iserializable.h"
#include "h3r_map.h"

H3R_NAMESPACE

/*
 The game: an object can visit another object.

 The game table is a multilayered quad: L quads of size A.
 A game object is an NxM rectangular area put on said quad.
 Each tile of a game object defines its visit-ability.

 There are rules of course: movement, visitors per host, etc. a.k.a. game-play.
*/

/*
   There is a game board; on it there are objects. M no.

   I have a hero. I can move him/her around in order to explore the map to find
   resources, artifacts, experience, to become stronger, build my castle to
   build an army to win the game. Each turn, my hero has limited amount of
   movement points. The game is played in turns: its me, then you, then me, ...
   my kingdom can have many heroes, towns/castles, other structures. There are
   many ways to win/loose the game.
   A few kingdoms compete for the win. There can be one or more human-controlled
   kingdom(s) and one or more computer-controlled ones.
   There are objects on the map (including special patches of land) that can
   apply bonuses or penalties (including bonus army units, or destroying part of
   the existing army) - modifiers - to a visiting hero and or hers/his kingdom.
   Different objects have different attributes: there are objects that:
     - teach spells or skills: modify hero
     - teleport the hero around the map w/o movement point cost (this is
       probably one of the infinite turn reasons, at the original game)
     - block hero movement - impassable objects
     - give or take (modify): kingdom resources, hero artefacts, hero army
     - can be "picked-up" or destroyed by a visitor - transient map objects
     - can become part of the kingdom: modify kingdom

   I hope that's enough. A picture formed already.
*/

// Let:
//  Modifier { Object * Source (); ... }
//  Hero : Object
//    Hero::Visit(obj) => obj->Visit (this)
//  Windmill : Object, Modifier
//    Windmill::Visit(obj) => if (...) obj->Kingdom ()->Mod (this->Mod ())
//  MagicPlains : Object, Modifier
//    MagicPlains::Visit(obj) => Subscribe (obj)->Mod     ( this->Mod ())
//    MagicPlains::OnLeave(obj) => UnSubscribe (obj)->Mod (-this->Mod ())
//  Artifact : Object, Modifier
// Then: Use-case:
//  * Windmill o1; Hero o2; o2.Visit (o1);
//  * MagicPlains o1; Hero o2; o2.Visit (o1);
//  * QuietEyeOfTheDragon a1; Hero o1; o1.Visit (a1);
//
// Extend: Add new "Object"s, even by a plug-in; also: ScriptedObject : Object.
//
// I add a new attribute: alignment; how shall the windmill object handle this
// new attribute: lets say it shall provide resources to those with equal
// alignment only? Extending the Windmill object is nice, but - yes there is a
// "but" :) - what happens If I want 100 distinct objects to handle it? - there
// will be a lot of extending to do - not good for a single attribute; bad
// design for me.
// My new attribute will need an AppliesTo(o1, o2) that defines its application.
// The map loader shall be instructed to add new attributes to objects that do
// not have them; how will it know what objects to add it to?
// The Object will have to identify itself open to new attributes, that support
// certain operations, like Attributes that SupportsAppliedTo().
// But: the above attribute shouldn't be applied to mines at a "flag all mines"
// map - because it could make the map not win-able. Objects related the the
// win/loss condition will reject SupportsAppliedTo() new attributes.
// Due to recursive relation between future attribute and future objects, the
// matching between them shall be done on a predefined set of generalized
// capabilities.
//LATER reason why a Decorator won't handle the above use-case; read the book
//      reason about all impossible solutions;
// The attributes open a wide area for bugs; what complicates when I remove
// them? They become Object methods: IsTransient(), and adding new ones becomes
// impossible: Add Aligment(), now make 100 descendants use it. Or they become
// scripts: complexity++; speed--; but the recursive relation goes to /dev/null.
// The good news is there are just two extensible aspects: Object and
// ObjectAttribute. The risks ObjectAttribute introduces could be partially
// mitigated by a run-time approval test performed on plug-in load:
//   - Denies(AVisit) - that it shouldn't
//   - Deletes(Object) - that it shouldn't
//   - Modifies(Object) - that it shouldn't
//   - Modifies(Kingdom) - that it shouldn't
//   - ...
// The test then could suggest modifications for AppliesTo(), mostly based on
// win/loss conditions: for example the Alignment attribute could do:
//   "if (winloss->Refers (obj)) return true" and pass the Denies(AVisit) test.
// And the above statement does look like a script ...
//TODO scripting idea
//
// I do think all these objects can be scripted entirely.
// OK let me script them in C++ 1st - see what falls out.
class Object : public ISerializable
{
#define public public:
#define private private:
#define protected protected:

    // Game engine. See "game.dia".
    public virtual void Visit(Object *) {}

    // ISerializable
    // The file format is like this:
    //   sentinel = s.Tell ();
    //   Object obj {s};
    //   s.Seek (sentinel - s.Tell ())
    //   Sprites[obj.Sprite ()] defines whose Object descendant shall be
    //   created.
    // I'm doing something different, with extensibility in mind:
    //   Object * o1 = Object::Create (s, map_version, Object {s})
    public Object(Stream & s)
        : _location {s}
    {
        Stream::Read (s, &_sprite);
        s.Read (_unknown, 5);
    }
    private void Serialize(Stream & s) override
    {
        static_cast<ISerializable>(_location).Serialize (s);
        Stream::Write (s, &_sprite);
        s.Write (_unknown, 5);
    }
    // "continue reading constructor" - skip pos & sprite; the void* is for C++
    // it can be anything - just some way to distinct the constructor.
    protected Object(void *, Stream &) {}
    private Object() {} // reflection-only constructor
    // The odd name is a hint to use the "continue reading constructor".
    public virtual Object * ContinueCreating(Stream &) { return nullptr; }
    // Register type T to be instantiated for MapVersion+ObjType.
    public template <typename T> static void Register(
        h3rMapVersion, h3rObjType);
    //
    public static Object * Create(Stream &, h3rMapVersion, const Object &);
    // The complicated and unpleasant system above, speeds-up map loading,
    // and allows extend-able object management. No more giant switch
    // statements.

    public virtual ~Object(){}

#undef public
    // allow to change the type of X, Y, Z, should the need arise, w/o an issue
    public: struct Pos final : public ISerializable
    {
#define public public:
        h3rCoord X, Y, Z;
        public Pos() {}
        public Pos(Stream & s)
        {
            Stream::Read (s, &X).Read (s, &Y).Read (s, &Z);
        }
        private inline void Serialize(Stream & s) override
        {
            Stream::Write (s, &X).Write (s, &Y).Write (s, &Z);
        }
    };

    // Dictated by the h3m file format:
    protected Pos _location;
    protected h3rObjRef _sprite; // sprite pointer (map.Sprites[])
    protected byte _unknown[5];

    public Pos & Location() { return _location; }
    public h3rObjRef Sprite() const { return _sprite; }
};// Object

// Use this to make your class usable by the map loader.
// Basically all built-in objects shall "call" it.
//LATER perhaps it will be more simple to just register them at main();
//      the above odd, static, factory-like, risky, hard to debug, machinery
//      will go to /dev/null
#define H3R_REGISTER_OBJECT(O,V,T) \
static struct _h3r_r_object_##O { \
    _h3r_r_object_##O () { Object::Register<O> (V, T); } \
};

#undef public
#undef private
#undef protected

NAMESPACE_H3R
#endif
