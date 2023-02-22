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

FFDNode::FFDNode(FFD::SNode * n, Stream * br)
    : _s{br}, _n{n}
{
    if (n->IsField ()) FromField ();
    else if (n->IsStruct ()) FromStruct ();
    else
        Dbg << "Can't handle " << n->TypeToString () << EOL;
}

bool FFDNode::EvalBoolExpr()
{
    Dbg << "FFD::Node::EvalBoolExpr: implement me" << EOL;
    return false;
}

void FFDNode::FromField()
{
    Dbg << " field " << _n->Name << EOL;
    H3R_ENSURE(nullptr != _n->DType, "field->DType can't be null")
    auto data_type = _n->DType;
    if (! data_type->IsMachType () && ! data_type->IsEnum ()) {
        Dbg << "FFD::Node::FromStruct: create another node" << EOL;
        return;
    }
    Dbg << " field, data size: " << data_type->Size << EOL;
    H3R_ENSURE(data_type->Size >= 0
            && data_type->Size <= FFD_MAX_MACHTYPE_SIZE, "data_type->Size")
    _data.Resize (data_type->Size);
    _signed = data_type->Signed;
    _s->Read (_data.operator byte * (), data_type->Size);
    Dbg << " field, data: "; PrintByteSequence ();
}

void FFDNode::FromStruct()
{
    _enabled = _n->HasExpr () ? EvalBoolExpr () : true;
    if (! _enabled) return;
    if (_n->VListItem) {
        Dbg << "FFD::Node::FromStruct: parse the ValueList" << EOL;
        return;
    }
    Dbg << "struct " << _n->Name << EOL;
    for (auto n : _n->Fields) {
        FFDNode * f {};
        H3R_CREATE_OBJECT(f, FFDNode) {n, _s};
        _fields.Add (f);
    }
}// FFD::Node::FromStruct()

NAMESPACE_H3R
