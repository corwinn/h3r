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

    private Array<byte> _data {};
    private Stream * _s {}; // reference
    private FFD::SNode * _n {}; // reference
    private bool _enabled {}; // set by bool expr.
    private bool _signed {}; // signed machine types
    private List<FFDNode *> _fields {}; // LL ?
    public FFDNode(FFD::SNode *, Stream *);
    public void FromStruct();
    public void FromField();
    public ~FFDNode();
    public String AsString();
    public int AsInt();
    public inline bool Enabled() const { return _enabled; }
    private bool EvalBoolExpr();
    // [dbg]
    private inline void PrintByteSequence()
    {
        if (_data.Length () <= 0) return;
        Dbg << Dbg.Fmt ("[%002X", _data[0]);
        for (int i = 1; i < _data.Length (); i++)
            Dbg << Dbg.Fmt (" %002X", _data[i]);
        Dbg << "]" << EOL;
    }

#undef public
#undef private
};// FFDNode

NAMESPACE_H3R

#endif