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

#include "h3r_ffdnode.h"

#include <new>

H3R_NAMESPACE

FFDNode::~FFDNode()
{
    for (int i = 0; i < _fields.Count (); i++)
        H3R_DESTROY_OBJECT(_fields[i], FFDNode)
}

FFDNode::FFDNode(FFD::SNode * n, Stream * br, FFDNode * base,
    FFD::SNode * field_node)
    : _s{br}, _n{n}, _f{field_node}, _base{base}
{
    if (base) _level = base->_level + 1;

    if (n->IsField ()) FromField ();
    else if (n->IsStruct ()) FromStruct ();
    else
        Dbg << "Can't handle " << n->TypeToString () << EOL;
}

// All expressions are encolsed in ().
// example 1: open, symbol, op, symbol, close
// example 2: open, symbol, op, open, symbol, op, symbol, close, close
// example 3: open, symbol, close
template <typename F> bool eval_expr(
    const List<FFDParser::ExprToken> & e, F resolve_symbols, int & id)
{
    H3R_ENSURE(id >= 0 && id < e.Count (), "Wrong expr.")
    FFDNode::ExprCtx ctx {};
    Dbg << "  Expr: ";
    for (; id < e.Count (); id++) { // find "l op r", or "op l"
        switch (e[id].Type) {
            case FFDParser::ExprTokenType::Open: {
                Dbg << "( ";
                H3R_ENSURE(ctx.i < 2, "Wrong number of arguments")
                ctx.v[ctx.i++] = eval_expr (e, resolve_symbols, ++id);
            } break;
            case FFDParser::ExprTokenType::Close: {
                Dbg << ") " << EOL;
                return ctx.Compute ();
            }
            case FFDParser::ExprTokenType::Symbol: {
                Dbg << "{" << e[id].Symbol << "} ";
                H3R_ENSURE(ctx.i < 2, "Wrong number of arguments")
                if (0 == ctx.i) ctx.LSymbol = e[id].Symbol;
                else if (1 == ctx.i) ctx.RSymbol = e[id].Symbol;
                ctx.i++;
                resolve_symbols (ctx);
            } break;
            case FFDParser::ExprTokenType::Number: {
                Dbg << e[id].Value << " ";
                H3R_ENSURE(ctx.i < 2, "Wrong number of arguments")
                ctx.v[ctx.i++] = e[id].Value;
            } break;
            default: {
                if (FFDParser::ExprTokenType::opN == e[id].Type)
                    ctx.n[ctx.i] = true;
                else
                    { ctx.op = e[id].Type; ctx.DbgPrint (); }
                break;
            }
        }
    }
    Dbg << EOL;
    H3R_ENSURE(1 == ctx.i, "Evaluation failed")
    return ctx.v[0];
}// eval_expr

FFD::SNode * FFDNode::ResolveSNode(const String & n, int & value,
    FFD::SNode * sn)
{//TODO cache me
    Dbg << "  ResolveSNode: requested symbol: " << n << EOL;
    static String sym_name {};
    for (auto sym : sn->Base->NodesByName (n)) {
        if (sym->IsIntConst () || sym->IsMachType () || sym->IsEnum ()) {
            H3R_ENSURE(sym_name != sym->Name, "Don't do that")
            if (! sym->Resolved) {
                sym->Resolved = true;
                if (sym->Expr.Count () > 0) {
                    sym_name = sym->Name;
                    int ptr {};
                    sym->Enabled = eval_expr (sym->Expr,
                        [&](ExprCtx & ctx) {
                            ResolveSymbols (ctx, sn, nullptr);
                        }, ptr);
                    sym_name = String {};
                }
                else
                    sym->Enabled = true;
            }
            if (sym->Enabled) {//LATER detect conflicts (more than 1 enabled)
                if (sym->IsIntConst ())
                    return value = sym->IntLiteral, sym;
                else
                    return value = sym->Size, sym;
            }
        }
    }
    Dbg << "  not found at _n->Base" << EOL;
    return nullptr;
}// FFDNode::ResolveSNode()

//LATER string literal const
void FFDNode::ResolveSymbols(ExprCtx & ctx, FFD::SNode * sn, FFDNode * base)
{
    Dbg << "   resolve symbol: ";
    if (1 == ctx.i) Dbg << ctx.LSymbol; else Dbg << ctx.RSymbol;
    Dbg << EOL;
    if (sn->Base) { // SNode
        int value {};
        bool found {};
        if (! ctx.LSymbol.Empty ())
            if (ResolveSNode (ctx.LSymbol, value, sn)) {
                ctx.v[0] = value;
                found = true;
            }
        if (! ctx.RSymbol.Empty ())
            if (ResolveSNode (ctx.RSymbol, value, sn)) {
                ctx.v[1] = value;
                found = true;
            }
        if (found) {
            Dbg << "   SNode found: L: " << ctx.v[0] << ", R: " << ctx.v[1]
                << EOL;
            return;
        }
       //TODO already set? (on duplicate symbol name for example)
    }
    if (base) { // FFDNode
        FFDNode * lsym {}, * rsym {};
        if (! ctx.LSymbol.Empty ())
            lsym = base->NodeByName (ctx.LSymbol);
        if (! ctx.RSymbol.Empty ())
            rsym = base->NodeByName (ctx.RSymbol);
        if (lsym && rsym) {
            H3R_ENSURE(lsym->_enabled, "NodeByName() contract failed")
            H3R_ENSURE(rsym->_enabled, "NodeByName() contract failed")
            ctx.v[0] = lsym->AsInt ();
            ctx.v[1] = rsym->AsInt ();
            return;
        }
        else if (! lsym && ! rsym) {
            H3R_ENSURE(0, "Symbols not found")
        }
        else if (2 == ctx.i) { // requested both
            if (lsym && ! rsym) {
                H3R_ENSURE(lsym->_enabled, "NodeByName() contract failed")
                Dbg << "    lsym && ! rsym "; lsym->_n->DbgPrint ();
                // check against the type set
                if (lsym->_n->DType->IsEnum ()) {
                    // Dbg << "   rsym.enum: find " << ctx.RSymbol << EOL;
                    auto enum_entry =
                        lsym->_n->DType->FindEnumItem (ctx.RSymbol);
                    if (enum_entry) {//TODO handle the not found part
                    /*if (enum_entry)
                        Dbg << "   rsym.enum: found" << enum_entry->Name << EOL;
                    else Dbg << "   rsym.enum: not found" << EOL;*/
                    ctx.v[0] = lsym->AsInt (); // already set at (1 == ctx.i)
                    ctx.v[1] = enum_entry->Value;
                    Dbg << "   L: " << ctx.v[0] << ", R: " << ctx.v[1] << EOL;
                    return;
                    }
                }
                H3R_ENSURE(0, "Symbol not found")
            }//TODO handle the reverse: if (! lsym && rsym)
        }
        else if (1 == ctx.i) {
            if (lsym) {
                H3R_ENSURE(lsym->_enabled, "NodeByName() contract failed")
                ctx.v[0] = lsym->AsInt ();
                return;
            }
        }
        Dbg << "  not found at _base" << EOL;
    }// if (_base)
}// FFDNode::ResolveSymbol

bool FFDNode::EvalBoolExpr(FFD::SNode * sn, FFDNode * base)
{
    int ptr {};
    auto result = eval_expr (sn->Expr, [&](ExprCtx & ctx) {
        ResolveSymbols (ctx, sn, base);
    }, ptr);
    Dbg << "   FFD::Node::EvalBoolExpr: " << result << EOL;
    return result;
}

String FFDNode::AsString()
{
    return static_cast<String &&>(String {_data, _data.Length ()});
}

// return 0 when its not machine and or can't be computed
/*static int GetSize(FFD::SNode * n)
{
    if (! n->DType) return 0;
    auto t = n->DType;
    if (t->IsMachType () || t->IsEnum ()) return t->Size;
    return 0;
}*/

void FFDNode::EvalArray()
{
    _array = true;
    auto n = nullptr != _f ? _f : _n;
    Dbg << " +field, array of " << n->DType->Name << EOL;
    // array size
    int arr_size {}, final_size {1};
    for (int i = 0; i < 3; i++) {
        if (n->Arr[i].None ()) break;
        Dbg << " ++dim type: "; n->Arr[i].DbgPrint (); Dbg << EOL;
        // Is it an implicit machine type?
        if (! n->Arr[i].Name.Empty ()) {
            Dbg << " arr: look for " << n->Arr[i].Name << " at "
                << n->Base->Name << EOL;
            auto m = n->Base->NodeByName (n->Arr[i].Name); // n is an LL !
            Dbg << "n->Prev: " << Dbg.Fmt ("%p, ", n->Prev)
                << ", n->Next: " << Dbg.Fmt ("%p", n->Next) << EOL;
            // m = n->NodeByName (n->Arr[i].Name); // look at the current node
            if (! m) {
                Dbg << " arr: not found?! " << n->Arr[i].Name << " at "
                    << n->Base->Name << EOL;
                int value {};
                // evaluate whatever might need evaluating
                m = ResolveSNode (n->Arr[i].Name, value, n);
            }
            if (m && m->IsIntConst ()) {
                _arr_dim[i] = m;
                Dbg << " ++dim value (intconst): " << m->IntLiteral << " items"
                    << EOL;
                arr_size = m->IntLiteral;
            }
            else if (m && ! m->IsMachType () && ! m->IsEnum ()) {
                Dbg << " implement me: FFDNode array dim" << EOL;
                return;
            }
            else if (m) {
                _arr_dim[i] = m;
                Dbg << " ++dim size (implicit): " << m->Size << " bytes" << EOL;
                H3R_ENSURE(m->Size >= 0 && m->Size <= 4, "array dim overflow")
                _s->Read (&arr_size, m->Size);
                Dbg << " ++dim value (implicit): " << arr_size << " bytes"
                    << EOL;
            }
        }// ! n->Arr[i].Name.Empty ()
        else {
            Dbg << " ++dim value (intlit): " << n->Arr[i].Value << " items"
                << EOL;
            _arr_dim[i] = n;
            arr_size = n->Arr[i].Value;
        }
        final_size *= arr_size;
    }
    Dbg << " ++array size: " << final_size << EOL;
    if (0 == final_size) {
        Dbg << "Warning: array final_size of 0: nothing to read" << EOL;
        return;
    }
    H3R_ENSURE(final_size >= 0 && final_size <= 1<<21, // H3M_MAX_FILE_SIZE
        "suspicious array size")
    // item size
    if (n->DType->IsMachType () || n->DType->IsEnum ()) {
        Dbg << " ++item size: " << n->DType->Size << " bytes" << EOL;
        final_size *= n->DType->Size;
        H3R_ENSURE(final_size >= 0 && final_size <= 1<<21, // H3M_MAX_FILE_SIZE
            "suspicious array size")
        _data.Resize (final_size);
        _s->Read (_data.operator byte * (), final_size);
        Dbg << " ++data: "; PrintByteSequence ();
        if ("MapString" == n->Base->Name) //LATER by attribute: [Text]
            Dbg << " ++text: " << AsString () << EOL;
    }
    else {
        int psize = n->DType->PrecomputeSize ();
        if (psize > 0) {
            Dbg << " ++item pre-computed size: " << psize << " bytes" << EOL;
            final_size *= psize;
            H3R_ENSURE(final_size >= 0 && final_size <= 1<<21,
                "suspicious array size")
            // read once
            _data.Resize (final_size);
            _s->Read (_data.operator byte * (), final_size);
            //LATER accessing those is complicated
        }
        else {
            for (int i = 0 ; i < final_size; i++) {
                Dbg << " +++item [" << i << "] (dynamic)" << EOL;
                FFDNode * f {};
                H3R_CREATE_OBJECT(f, FFDNode) {_n, _s, this};
                _fields.Add (f);
            }
        }
    }
}// FFDNode::EvalArray()

void FFDNode::FromField()
{
    Dbg << " field " << _n->Name << EOL;
    H3R_ENSURE(nullptr != _n->DType, "field->DType can't be null")

    /*_enabled = _n->HasExpr () ? EvalBoolExpr () : true;
    if (! _enabled) return;*/

    auto data_type = _n->DType;
    H3R_ENSURE(data_type->IsMachType () || data_type->IsEnum (),
        "bug: FFDNode::FromStruct() passed a field with unhandled DType")
    if (_n->Array)
        EvalArray ();
    else {
        Dbg << " field, data size: " << data_type->Size << " bytes" << EOL;
        H3R_ENSURE(data_type->Size >= 0
            && data_type->Size <= FFD_MAX_MACHTYPE_SIZE, "data_type->Size")
        _data.Resize (data_type->Size);
        _signed = data_type->Signed;
        _s->Read (_data.operator byte * (), data_type->Size);
        Dbg << " field, data: "; PrintByteSequence ();
    }
}// FFDNode::FromField()

void FFDNode::FromStruct(FFD::SNode * sn)
{
    /*_enabled = _n->HasExpr () ? EvalBoolExpr () : true;
    if (! _enabled) return;*/
    if (! sn) sn = _n;

    if (sn->VListItem) {
        Dbg << "FFD::Node::FromStruct: parse the ValueList" << EOL;
        return;
    }
    Dbg << "struct lvl " << _level << ": "  << sn->Name << EOL;
    if (_f && _f->Array) {
        EvalArray ();
        return;
    }
    for (auto n : sn->Fields) {
        FFDNode * f {};
        if (n->HasExpr () && ! EvalBoolExpr (n, this)) {
            Dbg << " Eval: false: " << n->Name << EOL;
            continue;
        }
        if (n->DType && n->DType->IsStruct ()) {
            if (n->Composite) {
                Dbg << "Composite struct: " << n->DType->Name << EOL;
                //TODO de-recursion-me when the DLL is in place:
                //     sn->Insert (n->DType->Fields)
                FromStruct (n->DType);
            }
            else
                H3R_CREATE_OBJECT(f, FFDNode) {n->DType, _s, this, n};
        }
        else
            H3R_CREATE_OBJECT(f, FFDNode) {n, _s, this};
        _fields.Add (f);
    }
}// FFD::Node::FromStruct()

NAMESPACE_H3R
