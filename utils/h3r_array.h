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
template <typename T,
          void (*A)(T * &, size_t) = OS::Alloc,    // This is not OOP.
          void (*F)(T * &) = OS::Free> class Array //
{
    private: T * _data {};
    private: size_t _len {}; // [T]
    private: void free_and_nil() { if (_data) F (_data), _len = 0; }

    public: inline const T * Data() const { return _data; }
    public: inline const size_t & Length() const { return _len; }

    public: Array() {}
    public: Array(Array<T> && a) { a.MoveTo (*this); }
    public: Array<T> & operator=(Array<T> && a)
    {
        return a.MoveTo (*this), *this;
    }
    public: Array(const Array<T> & a) { a.CopyTo (*this); }
    public: Array<T> & operator=(const Array<T> & a)
    {
        return a.CopyTo (*this), *this;
    }
    public: Array(size_t len) { Resize (len); }
    public: ~Array() { this->free_and_nil (); }

    public: operator T*() const { return _data; }

    public: void MoveTo(Array<T> & a)
    {
        if (this == &a) return;
        a.free_and_nil ();
        a._data = _data, _data = nullptr;
        a._len = _len, _len = 0;
    }

    public: void CopyTo(Array<T> & a) const
    {
        if (this == &a) return;
        a.free_and_nil ();
        if (_data && _len > 0) a.Append (_data, _len);
    }

    public: void Resize(size_t len)
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

    public: template <typename Q> void Append(const Q * data, size_t num)
    {
        H3R_ARG_EXC_IF(sizeof(Q) != sizeof(T), "sizeof(Q) != sizeof(T)")
        H3R_ARG_EXC_IF(num < 1, "num < 1")
        H3R_ARG_EXC_IF(nullptr == data, "nullptr == data")

        T * n {};
        size_t l = (num + _len);
        A (n, l * sizeof(T));
        T * n1 {n + _len};
        if (_data) OS::Memmove (n, _data, _len * sizeof(T)),
                   this->free_and_nil ();
        OS::Memmove (n1, data, num * sizeof(T));
        _data = n, _len = l;
    }

    public: bool Empty() const { return 0 == _len && nullptr == _data; }

    public: T & operator[](size_t i)
    {
        H3R_ARG_EXC_IF(0 == _len, "_len is 0; - no [] acceess")
        H3R_ARG_EXC_IF(i < 0 || i >= _len, "Your favorite message [i]")
        return _data[i];
    }

    // Implicit contract with the compiler: short and simple for loop
    public: const T * begin() const { return _data +    0; }
    public: const T * end  () const { return _data + _len; }
}; // template <typename T> class Array

NAMESPACE_H3R

#endif