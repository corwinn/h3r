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
class Object
{
    public: virtual void Visit(Object *) {}
}

NAMESPACE_H3R
#endif
