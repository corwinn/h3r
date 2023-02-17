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

#ifndef _H3R_FFD_H_
#define _H3R_FFD_H_

#include "h3r.h"
#include "h3r_string.h"
#include "h3r_list.h"

H3R_NAMESPACE

// File Format Description.
// Wraps a ffd (a simple text file written using a simple grammar) that can be
// used to parse different (I hope; its h3m for now) file formats.
// A.k.a. transforms binary files into trees.
//
// If this happens to work (is fast enough, is complicated not): Def, Pcx, Fnt,
// Pal, Lod, Vid, Snd, etc. will become a one-line function call:
//   data = FFD::File2Tree ("description", "file").
class FFD
{
#define public public:
#define private private:

    public FFD();
    public ~FFD();

    // The type of text at the description.
    public enum class SType {Comment, MachType, TxtList, TxtTable, Unhandled,
        Struct, Field, Enum, Const, Format};
    public enum class SConstType {None, Int, Text};
    // Syntax node - these are created as a result of parsing the description.
    public class SNode
    {
        public String Attribute {}; // [.*] prior it
        public SType Type {SType::Unhandled};
        public SNode * Base {};   // Base->Type == SType::Struct
        public String Name {};
        public SNode * DType {};  // Data/dynamic type (Expr: resolved on parse)
        public String DTypeName {}; // Prior resolve
        public bool Array {};     // Is it an array
        public bool Variadic {};  // "..." Type == SType::Field
        public bool VListItem {}; // Struct foo:value-list ; "foo" is at "Name"
        public bool Composite {}; // replace it with FindDType (Name)
        public String VList {};   // Variadic: value list :n,m,p-q,...
        public SConstType Const {};
          public String StringLiteral {};
          public int IntLiteral {};
        public List<SNode *> Fields {};
        public String Expr {};
        public String Comment {};
        // Type == SType::MachType
        public bool Signed {};
          public int Size {};
          public SNode * Alias {};  // Alias->Type == SType::MachType

        public bool Parse(const byte *, int, int &);
        public bool ParseMachType(const byte *, int, int &);
        public bool ParseLater(const byte *, int, int &);
        public bool ParseAttribute(const byte *, int, int &);
        public bool ParseStruct(const byte *, int, int &);
        private bool ParseField(const byte *, int, int &);
        public bool ParseConst(const byte *, int, int &);
        public bool ParseEnum(const byte *, int, int &);

        private bool ParseCompositeField(const byte *, int, int, int);
    };

    public class Node
    {
    };

    public static Node * File2Tree(const String & d, const String & f);

#undef public
#undef private
};// FFD

NAMESPACE_H3R

#endif