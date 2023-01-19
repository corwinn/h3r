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

#ifndef _H3R_LIST_H_
#define _H3R_LIST_H_

#include "h3r.h"
#include "h3r_array.h"

H3R_NAMESPACE

#define public public:
#define private private:

// List of T. You can Add(), Remove(), Count(), Clear(), "for (a:n)", ...
// It won't warn you for duplicates. Again: T * - you're the manager; T
// should get ~T()-ed.
template <typename T> class List
{
    private Array<T> _l; // list

    public List(size_t capacity = 0) : _l{capacity} {}
    public List(const T * a, size_t n) : _l{} { _l.Append (a, n); }
    public template <typename R> bool Contains(R itm)
    {
        for (const var & i : _l) if (i == itm) return true;
        return false;
    }
    public template <typename R> R Add(R itm)
    {
        return _l.Append (&itm, 1), itm;
    }
    // Remove all occurrences. Slow. Use rarely. You want fast: use an LL.
    // Returns whether something was removed (found) or not.
    public template <typename R> bool Remove(R itm)
    {
        Array<T> n {_l.Length ()};
        size_t idx {0};
        for (const var & i : _l) if (i != itm) n[idx++] = i;
        if (_l.Length () == idx) return false;
        return n.Resize (idx), n.MoveTo (_l), true;
    }
    public T & operator[](size_t i) { return _l[i]; }
    public bool Empty() const { return _l.Empty (); }
    public size_t Count() const { return _l.Length (); }
    public void Clear() { _l.Resize (0); }
    public const T * begin() const { return _l.begin (); }
    public const T * end  () const { return _l.end (); }
};// List

#undef private
#undef public

NAMESPACE_H3R

#endif