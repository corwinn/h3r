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

#ifndef _H3R_DLL_H_
#define _H3R_DLL_H_

#include "h3r.h"

H3R_NAMESPACE

// Doubly-linked list of T. T shall have default constructor; use T * otherwise.
//TODO this isn't a re-usable linked list, its a linked list of T; get the T out
//     of it.
template <typename T> class LList
{
    public using Node = LList<T>;
    private Node * _next {};
    private Node * _prev {};
    public Node * Next() { return _next; }
    public Node * Prev() { return _prev; }
    public T Data;

    public LList() : Data {} {}
    public LList(const T & data) : Data {data} {}

    // Insert "n" at "this".
    // prev-this-next becomes prev-n-this-next
    public Node * Insert(Node * n)
    {
        H3R_ARG_EXC_IF(nullptr == n, "Inserting null is a wrong idea")
        H3R_ARG_EXC_IF(nullptr != n->_next || nullptr != n->_prev,
            "Can't insert a list")
        H3R_ARG_EXC_IF(this == n, "Don't do that")

        n->_next = this;
        n->_prev = _prev;
        if (_prev) _prev->_next = n;
        _prev = n;
        return n;
    }

    // Misleading name: foo.InsertAfter (bar) inserts bar after foo.
    //LATER misleading vs confusion?
    // prev-this-next becomes prev-this-n-next
    public Node * InsertAfter(Node * n)
    {
        H3R_ARG_EXC_IF(nullptr == n, "Inserting null is a wrong idea")
        H3R_ARG_EXC_IF(nullptr != n->_next || nullptr != n->_prev,
            "Can't insert a list")
        H3R_ARG_EXC_IF(this == n, "Don't do that")

        n->_prev = this;
        n->_next = _next;
        if (_next) _next->_prev = n;
        _next = n;
        return n;
    }

    public Node * Delete()
    {
        if (_next) _next->_prev = _prev;
        if (_prev) _prev->_next = _next;
        _prev = _next = nullptr;
        return this;
    }
};// LList

NAMESPACE_H3R

#endif