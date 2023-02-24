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

#ifndef _H3R_FFDNODE_H_
#define _H3R_FFDNODE_H_

#include "h3r.h"
#include "h3r_string.h"
#include "h3r_list.h"
#include "h3r_stream.h"
#include "h3r_dbg.h"
#include "h3r_ffd.h"

H3R_NAMESPACE

// File Format Description.
// This is the tree that your data gets transformed to, by the description.
// FFDNode = f (SNode, Stream)
class FFDNode
{
#define public public:
#define private private:

    private Array<byte> _data {}; // empty for _array == true; _fields has them
    private Stream * _s {}; // reference
    private FFD::SNode * _n {}; // reference ; node
    private FFD::SNode * _f {}; // reference ; field node (Foo _f[])
    private bool _enabled {true}; // set by bool expr.
    private bool _signed {}; // signed machine types
    private bool _array {}; // array of struct
    private bool _hk {}; // hash key
    private FFD::SNode * _arr_dim[3] {}; // references; store the dimensions
    private List<FFDNode *> _fields {}; //TODO DL-list-me
    private int _level {};
    private FFDNode * _base {};
    private FFDNode * _ht {}; // hash table - referred by a hash key node
    // node, stream, base_node, field_node (has DType and Array: responsible for
    // "node" processing)
    public FFDNode(FFD::SNode *, Stream *, FFDNode * base = nullptr,
        FFD::SNode * = nullptr);
    public void FromStruct(FFD::SNode * = nullptr);
    public void FromField();
    public ~FFDNode();

    //PERHAPS all of these As.* must handle the _hk flag
    public String AsString();
    public inline FFDNode * Hash(const FFDNode * key) const
    {
        // auto sn = key->FieldNode ();
        if (_data.Length () > 0) {
            //LATER either construct a new FFDNode to just use its AsInt() -
            //      doesn't sound too bright to me; or use distinct functions:
            //      IntHash for example
            /*auto ofs = sn->DType->Size * key->AsInt ();
            auto buf = _data.operator byte * () + ofs;
            switch (sn->DType->Size) {
                case 1: return static_cast<byte>(*buf);
                case 2: return static_cast<short>(*buf);
                case 4: return static_cast<int>(*buf);
                default: H3R_ENSURE(0, "Unknown hash key size")
            }*/
            H3R_ENSURE(0, "Implement me: int hash(key)")
        }
        else if (_fields.Count () > 0)
            return _fields[key->AsInt (key->_ht)];
        else
            H3R_ENSURE(0, "Empty HashTable")
    }
    public inline int AsInt(FFDNode * ht = nullptr) const
    {
        int result {};
        switch (_data.Length ())
        {
            case 1:
                result = static_cast<int>(*(_data.operator byte * ())); break;
            case 2: result = static_cast<int>(
                *(reinterpret_cast<short *>(_data.operator byte * ()))); break;
            case 4: result = static_cast<int>(
                *(reinterpret_cast<int *>(_data.operator byte * ()))); break;
            default: H3R_ENSURE(0, "Don't request that AsInt")
        }
        if ((_hk && ! ht) || (ht && ht != _ht)) {//TODO test-me
            H3R_ENSURE(nullptr != _ht, "HashKey without a HashTable")
            auto sn = _ht->FieldNode ();
            H3R_ENSURE(sn->DType->IsIntType (), "HashTable<not int>")
            result = _ht->Hash (this)->AsInt ();
        }
        return result;
    }
    public inline bool Enabled() const { return _enabled; }

    struct ExprCtx final // required to evaluate enum elements in expression.
    {
        int v[2] {}; // l r
        int i {};    // 0 1
        bool n[2] {}; // negate for l r
        String LSymbol {}; // Version | RoE
        String RSymbol {}; // RoE     | Version
        FFDParser::ExprTokenType op {FFDParser::ExprTokenType::None}; // Binary
        public inline int Compute()
        {
            if (n[0]) v[0] = ! v[0];
            if (n[1]) v[1] = ! v[1];
            // Dbg << "Compute " << i << ", l: " << v[0] << ", r: " << v[1]
            //     << EOL;
            switch (op) {
                case FFDParser::ExprTokenType::None: return v[0];
                case FFDParser::ExprTokenType::opN: return ! v[0];
                case FFDParser::ExprTokenType::opNE: return v[0] != v[1];
                case FFDParser::ExprTokenType::opE: return v[0] == v[1];
                case FFDParser::ExprTokenType::opG: return v[0] > v[1];
                case FFDParser::ExprTokenType::opL: return v[0] < v[1];
                case FFDParser::ExprTokenType::opGE: return v[0] >= v[1];
                case FFDParser::ExprTokenType::opLE: return v[0] <= v[1];
                case FFDParser::ExprTokenType::opOr: return v[0] || v[1];
                case FFDParser::ExprTokenType::opAnd: return v[0] && v[1];
                default: H3R_ENSURE(0, "Unknown op")
            }
        }
        public inline void DbgPrint()
        {
            switch (op) {
                case FFDParser::ExprTokenType::None: Dbg << " "; break;
                case FFDParser::ExprTokenType::opN: Dbg << "! "; break;
                case FFDParser::ExprTokenType::opNE: Dbg << "!= "; break;
                case FFDParser::ExprTokenType::opE: Dbg << "== "; break;
                case FFDParser::ExprTokenType::opG: Dbg << "> "; break;
                case FFDParser::ExprTokenType::opL: Dbg << "< "; break;
                case FFDParser::ExprTokenType::opGE: Dbg << ">= "; break;
                case FFDParser::ExprTokenType::opLE: Dbg << "<= "; break;
                case FFDParser::ExprTokenType::opOr: Dbg << "|| "; break;
                case FFDParser::ExprTokenType::opAnd: Dbg << "&& "; break;
                default: Dbg << "?? "; break;
            }
        }
    };// ExprCtx
    // Evaluate machtype|enum Size, or const IntLiteral, based on their Expr.
    // Cache their Enabled state, based on the evaluated Expr.
    // Returns the SNode of the symbol that was found.
    // Use when machtype, enum, or const have Expression on them.
    // "resolve_only" is true when you don't need a value from the stream.
    // "resolve_only" is false for implicit "bool" for example: "(bool)", where
    // bool is a machine type, and true for some machine type whose size depends
    // on runtime bool evaluation.
    private FFD::SNode * ResolveSNode(const String &, int & value,
        FFD::SNode * sn, bool resolve_only = false);
    private void ResolveSymbols(ExprCtx &, FFD::SNode * sn, FFDNode * base);
    // sn - expression node, base - current struct node
    private bool EvalBoolExpr(FFD::SNode * sn, FFDNode * base);
    private void EvalArray();

    // [dbg]
    private inline void PrintByteSequence()
    {
        if (_data.Length () <= 0) return;
        Dbg << Dbg.Fmt ("[%002X", _data[0]);
        for (int i = 1; i < _data.Length (); i++)
            Dbg << Dbg.Fmt (" %002X", _data[i]);
        Dbg << "]" << EOL;
    }
    public inline FFD::SNode * FieldNode() const { return _f ? _f : _n; }
    public inline FFDNode * NodeByName(const String & name)
    {
        //LATER
        // This lookup is not quite ok. Duplicate symbol names might surprise
        // one. I better think of some way to explicitly mark "public" symbols.
        if (_array) { // no point looking in it
            // Dbg << " NodeByName: no _array lookup" << EOL;
            if (_base) return _base->NodeByName (name);
            return nullptr;
        }

        for (auto n : _fields) {
            auto sn = n->FieldNode ();
            // Dbg << "  NodeByName: q:" << name << ", vs:" << sn->Name
            //     << EOL;
            if (n->_enabled && sn->Name == name) return n;
        }

        if (_base) return _base->NodeByName (name);

        return nullptr;
    }
    public inline FFDNode * FindHashTable(const String & type_name)
    {
        if (_array) { // no point looking in it
            if (_base) return _base->FindHashTable (type_name);
            return nullptr;
        }
        for (auto n : _fields) {
            auto sn = n->FieldNode ();
            /*Dbg << " field: " << sn->Name << ", type: "
                << (sn->DType ? sn->DType->Name : "none")
                << ", arr. dims: " << sn->ArrDims () << EOL;*/
            if (n->_enabled && 1 == sn->ArrDims ()
                && sn->DType && sn->DType->Name == type_name) return n;
        }
        if (_base) return _base->FindHashTable (type_name);
        return nullptr;
    }

    public inline void PrintTree(int f_id = -1)
    {
        for (int i = 0; i < _level; i++) {
            if (_level-1 == i) {
                if (_base && _base->_fields.Count () - 1 == f_id) Dbg << "'-";
                else Dbg << "|-";
            }
            else if (! (i % 2)) Dbg << "| ";
            else Dbg << "  ";
        }
        Dbg << "Node";
        if (f_id >= 0) Dbg << Dbg.Fmt ("[%5d]", f_id);
        Dbg << " level " << _level << ": ";
        auto fn = FieldNode ();
        if (fn) {
            if (fn->Name.Empty ()) Dbg << "{empty name} ???";
            else Dbg << fn->Name;
        }
        Dbg << EOL;
        for (int i = 0; i < _fields.Count (); i++) {
            if (! _fields[i]) Dbg << "{null field} ???" << EOL;
            else _fields[i]->PrintTree (i);
        }
    }

#undef public
#undef private
};// FFDNode

NAMESPACE_H3R

#endif