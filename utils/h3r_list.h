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
#include "h3r_os.h"

H3R_NAMESPACE

template <typename T> struct LD { void operator()(T & t) { t.~T (); } };
template <typename T> struct LD<T *> { void operator()(T *&) {} };

// List of T. You can Add(), Remove(), Count(), Clear(), "for (a:n)", ...
//
//  - T * - you're the manager;
//  - T     should get ~T()-ed;
//  - T shall be copyable
//  - T shall be moveable
//  - T shall default-constructable
//
// Do not use this for simple types - it is slow. Use the Array instead.
// Consider this a List of T where T is non-trivial object.
//
// Why not final: because I might need to extend it for specific T.
template <typename T,
          void (*A)(T * &, size_t) = OS::Alloc,
          void (*F)(T * &) = OS::Free> class List //LATER Insert()
{
    // it can not be an array; private Array<T> _l; // list
    private T * _list {};
    private int _cnt {}, _cap {};

    private void FreeObjects(T * list, int n)
    {
        for (int i = 0; i < n; i++) LD<T>{} (list[i]);
    }
    private void FreeObjects()
    {
        if (_list) FreeObjects (_list, _cap), F (_list), _cnt = _cap = 0;
    }

    private void CopyObjects(T * dst, const T * src, int n) const
    {
        if (dst == src) return;
        for (int i = 0; i < n; i++) dst[i] = src[i];
    }

    private void Grow(int cap = 0)
    {
        if (_cnt == _cap) {
            if (cap && cap > _cap) _cap = cap; else _cap += 4;
            T * list {};
            A (list, _cap);
            OS::Memcpy (list, _list, _cnt*sizeof(T));
            F (_list);
            _list = list;
            for (int i = _cnt; i < _cap; i++) new (_list+i) T{};
        }
    }

    public void CopyTo(List<T> & dst) const
    {
        if (this == &dst) return;
        dst.FreeObjects ();
        dst._cnt = _cnt;
        dst._cap = _cap;
        if (_cap <= 0) return;
        A (dst._list, _cap);
        CopyObjects (dst._list, _list, _cap);
    }

    public void MoveTo(List<T> & dst)
    {
        if (this == &dst) return;
        dst.FreeObjects ();
        dst._list = _list, _list = nullptr;
        dst._cnt = _cnt, _cnt = 0;
        dst._cap = _cap, _cap = 0;
    }

    public List(List<T> && a) { a.MoveTo (*this); }
    public List<T> & operator=(List<T> && a)
    {
        return a.MoveTo (*this), *this;
    }
    public List(const List<T> & a) { a.CopyTo (*this); }
    public List<T> & operator=(const List<T> & a)
    {
        return a.CopyTo (*this), *this;
    }

    public List(int capacity = 0)
        : _cap{capacity}
    {
        H3R_ARG_EXC_IF(capacity < 0, "capacity out of range")
        if (! _cap) return;
        A (_list, _cap*sizeof(T));
        for (int i = 0; i < _cap; i++) new (_list+i) T{};
    }
    public List(const T * a, int n)
        : _cnt{n}, _cap{n}
    {
        H3R_ARG_EXC_IF(n <= 0, "n out of range")
        A (_list, _cap*sizeof(T));
        CopyObjects (_list, a, n);
    }
    /* Because you could compare to something that is implicitly cast-able to T
     * thus calling pointless initialization. Don't do:
     * public template <typename R> bool Contains(R itm)
    {
        for (const auto & i : _l) if (i == itm) return true;
        return false;
    }*/

    public ~List() { FreeObjects (); }
    public bool Contains(const T & itm) const
    {
        for (int i = 0; i < _cnt; i++) if (_list[i] == itm) return true;
        return false;
    }
    public T & Add(const T & itm)
    {
        Grow ();
        return _list[_cnt++] = itm;
    }
    public List<T> & Put(T && itm)
    {
        Grow ();
        _list[_cnt++] = static_cast<T &&>(itm);
        return *this;
    }
    // Remove all occurrences. Slow. Use rarely. You want fast: use an LL.
    // Returns whether something was removed (found) or not.
    public bool Remove(const T & itm)
    {
        int idx {0}, i {0}, cnt {_cnt};
        for (; i < _cnt; i++)
            if (_list[i] != itm) {
                LD<T>{} (_list[idx]);
                _list[idx++] = static_cast<T &&>(_list[i]);
            }
            else
                cnt--;
        return _cnt = cnt, idx < i;
    }
    public void RemoveAt(int i)
    {
        H3R_ARG_EXC_IF(i < 0 || i >= _cnt, "Out of range exception")
        LD<T>{} (_list[i]);
        _cnt--;
        if (i != _cnt) OS::Memmove (_list+i, _list+i+1, (_cnt-i)*sizeof(T));
    }
    public T & operator[](int i)
    {
        H3R_ARG_EXC_IF(i < 0 || i >= _cnt, "Out of range exception")
        return _list[i];
    }
    public const T & operator[](int i) const
    {
        H3R_ARG_EXC_IF(i < 0 || i >= _cnt, "Out of range exception")
        return _list[i];
    }
    public bool Empty() const { return _cnt <= 0; }
    public int Count() const { return _cnt; }
    public void Clear() { FreeObjects (); }

    // A word about "range-based for loop" and event-driven programming: just
    // don't use them together.
    //TODO remove these
    public const T * begin() const { return _list; }
    public const T * end  () const { return _list + _cnt; }

    // Another init. path; lets see...
    public List<T> & operator<<(const T & r)
    {
        return Add (r), *this;
    }

    public void Resize(int len)
    {
        H3R_ARG_EXC_IF(len < 0, "len can't be < 0")
        if (len == _cnt) return;
        if (0 == len) FreeObjects ();
        else if (len > _cnt) Grow (len);
        else while (--_cnt != len) LD<T>{} (_list[_cnt]);
    }

    // F: bool (*)(const T &)
    public template <typename Fd> T * Find(Fd on_itm)//TODO testme
    {
        for (int i = 0; i < _cnt; i++)
            if (on_itm (static_cast<const T &>(_list[i])))
                return &(_list[i]);
        return nullptr;
    }
};// List

NAMESPACE_H3R

#endif