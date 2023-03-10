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
#include "h3r_ffdparser.h"
#include "h3r_stream.h"
#include "h3r_dbg.h"

H3R_NAMESPACE

class FFDNode;

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
    public FFD(const String & d);
    public ~FFD();

    // TODO FFD::Load ("description").Parse ("foo");

    // The type of text at the description.
    //
    // Attribute are distinct nodes for now; when processing, they're the nth
    // previous nodes. Its possible that there will be global attributes.
    //
    public enum class SType {Comment, MachType, TxtList, TxtTable, Unhandled,
        Struct, Field, Enum, Const, Format, Attribute};
    public enum class SConstType {None, Int, Text};
    public class EnumItem final
    {
        public String Name {};
        public int Value {};
        public List<FFDParser::ExprToken> Expr {};
        public bool Enabled {};
    };
    public class ArrDimItem final
    {
        public String Name {}; // Empty when Value is used
        public int Value {}; // 0 when Name is used
        public inline bool None() const { return Name.Empty () && Value <= 0; }
        public inline void DbgPrint()
        {
            if (None ()) return;
            if (Value > 0) Dbg << "intlit: " << Value;
                      else Dbg << "symbol: " << Name;
        }
    };
    // Syntax node - these are created as a result of parsing the description.
    // Its concatenation of SType. It could become class hierarchy.
    public class SNode final
    {
        // Allow copying for now
        public SNode() {}
        public ~SNode();

        // a temporary not OO LL <=> until the h3r_dll situation gets resolved
        public SNode * Prev {};
        public SNode * Next {};
        private template <typename F> void WalkBackwards(F on_node)
        {
            if (! on_node (this)) return;
            if (nullptr == Prev) return;
            Prev->WalkBackwards (on_node);
        }

        // Returns Usable() only! Means the SNode is enabled, and doesn't need
        // evaluation.
        public SNode * NodeByName(const String &);
        // Returns all that match the name, regardless of flags.
        public List<SNode *> NodesByName(const String &);

        // Call after all nodes are parsed. Allows for dependency-independent
        // order of things at the description.
        public void ResolveTypes();
        public template <typename F> void WalkForward(F on_node)
        {
            if (! on_node (this)) return;
            if (nullptr == Next) return;
            Next->WalkForward (on_node);
        }

        public String Attribute {}; // [.*] prior it
        public SType Type {SType::Unhandled};
        public SNode * Base {};   // Base->Type == SType::Struct
        public String Name {};
        public SNode * DType {};  // Data/dynamic type (Expr: resolved on parse)
        public String DTypeName {}; // Prior resolve
        public List<SNode *> Fields {};
        public List<FFDParser::ExprToken> Expr {};
        public String Comment {};

        public bool HashKey {};
        public String HashType {};

        public bool Array {};     // Is it an array
        public ArrDimItem Arr[FFD_MAX_ARR_DIMS] {};
        public inline int ArrDims() const
        {
            for (int i = 0; i < FFD_MAX_ARR_DIMS; i++)
                if (Arr[i].None ()) return i;
            return FFD_MAX_ARR_DIMS;
        }

        public bool Variadic {};  // "..." Type == SType::Field
        public bool VListItem {}; // Struct foo:value-list ; "foo" is at "Name"
        // public String ValueList {};   // Variadic: value list :n,m,p-q,...
        // Variadic: value list :n,m,p-q,...
        public List<FFDParser::VLItem> ValueList {};
        public inline void PrintValueList() const
        {
            for (auto & itm : ValueList)
                Dbg << " itm[" << itm.A << ";" << itm.B << "]";
            Dbg << EOL;
        }
        public inline bool InValueList(int value) const
        {
            for (auto & itm : ValueList) {
                // Dbg << "InValueList( " << value << ", a: " << itm.A
                //     << ", b: " << itm.B << EOL;
                if (itm.Contains (value)) return true;
            }
            return false;
        }
        public inline SNode * FindVListItem(const String & dname, int value)
        {//TODO this is the same as NodeByName, NodesByName, etc. unify them
         //TODO more than one is an error: report it
            FFD::SNode * result = {};
            WalkBackwards([&](FFD::SNode * node) {
                // Dbg << "FindVListItem(" << dname << " vs " << node->Name
                //     << EOL;
                if (node->VListItem && node->Usable () && node->Name == dname
                    && node->InValueList (value)) {
                    result = node;
                    return false;
                }
                return true;
            });
            if (nullptr == result && Next)
                Next->WalkForward([&](FFD::SNode * node) {
                    // Dbg << "FindVListItem(" << dname << " vs " << node->Name
                    //    << EOL;
                    if (node->VListItem && node->Usable ()
                        && node->Name == dname && node->InValueList (value)) {
                        result = node;
                        return false;
                    }
                    return true;
                });
            return result;
        }

        public bool Composite {}; // replace it with FindDType (Name)

        public SConstType Const {};
        public String StringLiteral {};
        public int IntLiteral {};

        public bool Enabled {}; // true when Expr has evaluated to it
        public bool Resolved {}; // true when Expr has been evaluated
        public bool inline Usable() const
        {//TODO report ! Resolved
            return Expr.Count () <= 0 || (Resolved && Enabled);
        }

        // Type == SType::MachType
        public bool Signed {};
        public int Size {};
        // public SNode * Alias {};  // This could become useful later

        public List<EnumItem> EnumItems {};
        public EnumItem * FindEnumItem(const String &);

        public bool Parse(FFDParser &);
        public bool ParseMachType(FFDParser &);
        public bool ParseLater(FFDParser &);
        public bool ParseAttribute(FFDParser &);
        public bool ParseStruct(FFDParser &);
        private bool ParseField(FFDParser &);
        public bool ParseConst(FFDParser &);
        public bool ParseEnum(FFDParser &);

        private bool ParseCompositeField(FFDParser &, int);

        public inline bool IsRoot() const { return FFD::SType::Format == Type; }
        // DType not possible: a.k.a. some nodes do not require/have dtype.
        public inline bool NoDType()
        {
            switch (Type) {
                case (SType::Field): case (SType::Enum): return false;
                default: return true;
            };
        }
        public inline bool IsAttribute() const
        {
            return SType::Attribute == Type;
        }
        public inline bool IsStruct() const
        {
            return SType::Struct == Type || SType::Format == Type;
        }
        public inline bool IsMachType() const
        {
            return SType::MachType == Type;
        }
        public inline bool IsEnum() const { return SType::Enum == Type; }
        public inline bool IsField() const { return SType::Field == Type; }
        public inline bool IsIntConst() const
        {
            return SType::Const == Type && SConstType::Int == Const;
        }
        public inline bool IsConst() const { return SType::Const == Type; }
        public inline bool IsValidArrDim() const
        {
            return IsMachType () || IsEnum ();
        }
        // 32 bit int, mind you; also include the ! Fp, when floating-point
        // types come around
        public inline bool IsIntType() const
        {
            return (IsMachType () || IsEnum () || IsIntConst ())
                && (Size >= 1 && Size <= 4);
        }
        // By value. Allow many attributes for custom extensions.
        public SNode * GetAttr(const String & query)
        {
            auto n = Prev;
            while (n && n->IsAttribute ()) {
                if (query == n->Attribute) return n;
                n = n->Prev;
            }
            return nullptr;
        }
        public inline String TypeToString() const
        {
            switch (Type) {
                case FFD::SType::MachType: return "MachType";
                case FFD::SType::Struct: return "Struct";
                case FFD::SType::Field: return "Field";
                case FFD::SType::Enum: return "Enum";
                case FFD::SType::Const: return "Const";
                case FFD::SType::Format: return "Format";
                case FFD::SType::Attribute: return "Attribute";
                default: return "Unhandled";
            };
        }
        public inline bool HasExpr() const { return Expr.Count () > 0; }
        public void DbgPrint();
        // where there are no dynamic arrays and expressions
        public int PrecomputeSize()
        {
            int result {};
            for (auto f : Fields) {
                if (! f->DType) return 0;
                if (! f->Expr.Empty ()) return 0;
                if (f->DType->IsStruct ()) return 0;
                if (f->Array) {
                    int arr_result = 1, i {};
                    for (; i < 3 && ! Arr[i].None (); i++) {
                        if (! f->Arr[i].Name.Empty ()) {
                            H3R_ENSURE(nullptr != Base, "array node w/o Base?")
                            auto n = Base->NodeByName (f->Arr[i].Name);
                            if (n && ! n->IsIntConst ()) return 0;
                            arr_result *= n->IntLiteral;
                        }
                        else
                            arr_result *= f->Arr[i].Value;
                    }
                    H3R_ENSURE(i > 0, "array node w/o dimensions?")
                    result += arr_result;
                }
                else
                    result += f->DType->Size;
            }
            return result;
        }// PrecomputeSize()
    };// SNode

    private SNode * _root {};
    // An LL is preferable to a list, because each node should be able to look
    // at its neighbors w/o accessing third party objects.
    private FFD::SNode * _tail {}, * _head {}; // DLL<FFD::SNode>

    // public FFDNode * File2Tree(const String & d, const String & f);
    public FFDNode * File2Tree(const String & f);
};// FFD

NAMESPACE_H3R

#endif