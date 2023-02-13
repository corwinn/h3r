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

#ifndef _H3R_STACK_H_
#define _H3R_STACK_H_

#include "h3r.h"
#include "h3r_array.h"

H3R_NAMESPACE

#define public public:
#define private private:

// LIFO. Primary use: avoid recursive calls. Primary reason: dir enum.
// You free T *; T should get ~T() called
template <typename T> class Stack final
{
    private Array<T> _b;
    private int _sp {-1};
    public Stack(int capacity = 0) : _b{0 == capacity ? 8 : capacity} {}
    public void Push(const T & t)
    {
        if (++_sp >= static_cast<int>(_b.Length ()))
            _b.Resize (_b.Length () + (_b.Length () >> 2));
        _b[_sp] = t;
    }
    public T Pop()
    {
        H3R_ARG_EXC_IF(Empty (), "Stack underrun")
        return static_cast<T &&>(_b[_sp--]);
    }
    public bool Empty() const { return -1 == _sp; }
    public int Size() const { return _b.Length (); }

    // Act as a pipe
    public const T * begin() const { return _b.begin (); }
    public const T * end  () const { return _b.end (); }
};

#undef public
#undef private

NAMESPACE_H3R

#endif