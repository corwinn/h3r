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

#ifndef _H3R_ARRAY_H_
#define _H3R_ARRAY_H_

#include "h3r.h"
#include "h3r_os.h"

H3R_NAMESPACE

// Couple an array with its length. Testing: throws ArgumentException.
//LATER pool(A,F); 1Mb initial
// A catch: when T is an object, ~T() won't get called by free_and_nil() with
//          the default A and F.
template <typename T,
          void (*A)(T * &, size_t) = OS::Alloc,    // This is not OOP.
          void (*F)(T * &) = OS::Free> class Array //
{
    private T * _data {};
    private int _len {}; // [T]

    private void free_and_nil() { if (_data) F (_data), _len = 0; }

    public inline const T * Data() const { return _data; }
    public inline const int & Length() const { return _len; }

    public Array() {}
    public Array(const T * a, int n) { Append (a, n); }
    public Array(Array<T> && a) { a.MoveTo (*this); }
    public Array<T> & operator=(Array<T> && a)
    {
        return a.MoveTo (*this), *this;
    }
    public Array(const Array<T> & a) { a.CopyTo (*this); }
    public Array<T> & operator=(const Array<T> & a)
    {
        return a.CopyTo (*this), *this;
    }
    public bool operator==(const Array<T> & a)
    {
        return _len != a._len ? false
            : 0 == OS::Memcmp (_data, a._data, sizeof(T)*_len);
    }
    public bool operator!=(const Array<T> & a)
    {
        return ! (operator== (a));
    }
    public Array(int len) { Resize (len); }
    public ~Array() { this->free_and_nil (); }

    public operator T*() const { return _data; }

    public void MoveTo(Array<T> & a)
    {
        if (this == &a) return;
        a.free_and_nil ();
        a._data = _data, _data = nullptr;
        a._len = _len, _len = 0;
    }

    public void CopyTo(Array<T> & a) const
    {
        if (this == &a) return;
        a.free_and_nil ();
        if (_data && _len > 0) a.Append (_data, _len);
    }

    public void Resize(int len)
    {
        H3R_ARG_EXC_IF(len < 0, "len < 0")

        if (0 == len) {
            this->free_and_nil ();
            return;
        }

        T * n {};
        A (n, len);
        if (_data) OS::Memmove (n, _data,
                       (len < _len ? len : _len) * sizeof (T)),
                   this->free_and_nil ();
        _data = n, _len = len;
    }

    public template <typename Q> void Append(const Q * data, int num)
    {
        H3R_ARG_EXC_IF(sizeof(Q) != sizeof(T), "sizeof(Q) != sizeof(T)")
        H3R_ARG_EXC_IF(num < 1, "num < 1")
        H3R_ARG_EXC_IF(nullptr == data, "nullptr == data")

        T * n {};
        int l = (num + _len);
        A (n, l * sizeof(T));
        T * n1 {n + _len};
        if (_data) OS::Memmove (n, _data, _len * sizeof(T)),
                   this->free_and_nil ();
        OS::Memmove (n1, data, num * sizeof(T));
        _data = n, _len = l;
    }

    // The above one caught a lot of errors, so the tradition continues.
    // "index" is sizeof(Q)-based.
    // Insert num Q at index. Insert at [Length()] is supported.
    public template <typename Q> void Insert(
        int index, const Q * data, int num)
    {
        H3R_ARG_EXC_IF(sizeof(Q) != sizeof(T), "sizeof(Q) != sizeof(T)")
        H3R_ARG_EXC_IF(num < 1, "num < 1")
        H3R_ARG_EXC_IF(nullptr == data, "nullptr == data")
        H3R_ARG_EXC_IF(index > _len, "can't insert at non-existent place")

        if (nullptr == _data || index == _len)
            Append (data, num);
        else {
            size_t m1_num = _len - index;//4
            Resize (_len + num);//5
            OS::Memmove (_data + index + num, _data + index, sizeof(Q)*m1_num);
            OS::Memmove (_data + index, data, sizeof(Q)*num);
        }
    }

    public bool Empty() const { return 0 == _len && nullptr == _data; }

    public T & operator[](int i)
    {
        H3R_ARG_EXC_IF(0 == _len, "_len is 0; - no [] acceess")
        H3R_ARG_EXC_IF(i < 0 || i >= _len, "Your favorite message [i]")
        return _data[i];
    }

    public const T & operator[](int i) const
    {
        H3R_ARG_EXC_IF(0 == _len, "_len is 0; - no [] acceess")
        H3R_ARG_EXC_IF(i < 0 || i >= _len, "Your favorite message [i]")
        return _data[i];
    }

    // Implicit contract with the compiler: short and simple for loop.
    // Nothing simple about it however. Be very careful when using this: RTFM
    // a.k.a. "The C++ programming Language" - 6.3.6. Specify "auto &"
    // explicitly or end up with a lot of copies, because 7.7. of the same book.
    // Also, see "range_for" at the .test
    public const T * begin() const { return _data +    0; }
    public const T * end  () const { return _data + _len; }
    // public: T * begin() { return _data +    0; }
    // public: T * end  () { return _data + _len; }

    // Set everything to 0 - clear all bits.
    public void Clear()
    {
        if (_data && _len > 0)
            OS::Memset (_data, 0, _len * sizeof(T));
    }

    // This doesn't release the memory in use.
    public void Remove(int idx)
    {
        H3R_ARG_EXC_IF(_len <= 0, "Bug: delete from an empty array")
        H3R_ARG_EXC_IF(idx < 0 || idx >= _len, "Your favorite message [i]")
        if (idx < _len - 1)
            OS::Memmove (_data+idx, _data+idx+1, (_len-1-idx)*sizeof(T));
        _len--;
    }
}; // template <typename T> class Array

NAMESPACE_H3R

#endif