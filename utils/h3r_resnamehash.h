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

#ifndef _H3R_RESNAMEHASH_H_
#define _H3R_RESNAMEHASH_H_

#include "h3r.h"
#include "h3r_array.h"

H3R_NAMESPACE

#define public public:
#define private private:

// A hash for the resource names. T is the value.
// T has to has a copy constructor. The copies are managed by the map! - be very
// careful at T::~T().
// For use at the TexCache; this is the "Cache" part.
//
//  Inserted: 34324 keys; time: ~6110 [msec] <-> with all sanitizers
//  Query: ~210963 keys/s                    <-> the same keys that were inserted
// No sanitizers -O0 -g:
//  Inserted: 34324 keys; time: ~3898 [msec]
//  Query: ~613240 keys/s
// The timings do include the building of the composite key.
// And I haven't even began optimizing (there are 34324 alloc() for the table
// alone). Fast enough for me.
// The test is using around 4Mb RAM (without sanitizers; includes lodfs and the
// def parser).
template <typename T> class ResNameHash final
{
    private template <typename V> struct KeyValue
    {
        KeyValue(const Array<byte> & key, const V & value)
            : Key {key}, Value {value} {}
        Array<byte> Key;
        V Value;
    };
    private Array<KeyValue<T> *> _tbl {};
    public ~ResNameHash()
    {
        printf (
            "ResNameHash: Used: %d KV pairs; %lu bytes; hits: %d, misses: %d"
            EOL, _tbl.Length (), sizeof(KeyValue<T>)*_tbl.Length (), _hit_cnt,
            _miss_cnt);
        for (auto kv : _tbl) H3R_DESTROY_OBJECT(kv, KeyValue<T>)
    }
    private int Cmp(const Array<byte> & a, const Array<byte> & b)
    {
        if (a.Length () < b.Length ()) return -1;
        else if (a.Length () > b.Length ()) return 1;
        else return OS::Strncmp (
            reinterpret_cast<const char *>(a.Data ()),
            reinterpret_cast<const char *>(b.Data ()), a.Length ());
    }
    // This one is special: a. no recursion; b. identifies Insert() index.
    // Returns -1 when a key is not found.
    //PERHAPS call me when it becomes too slow, e.g. it begins requiring prime
    //        numbers, rolling bits, linked lists, walking trees. OK - without
    //        the last one :)
    private int BSearch(const Array<byte> & key, int & i)
    {
        if (_tbl.Length () <= 0) { i = 0; return -1; }
        int a {0}, b {_tbl.Length ()-1};
        for (;;) {
            int m = a + (b-a)/2;
            int c = Cmp (key, _tbl[m]->Key);
            if (! c) return i = m;
            if (c < 0) {// a;m)
                b = m-1;
                if (b < a) return i=a, -1;
            }
            else { // (m;b
                a = m+1;
                if (a > b) return i=a, -1;
            }
        }
    }
    public void Add(const Array<byte> & key, const T & value)
    {
        int idx {-1};
        int res = BSearch (key, idx);
        H3R_ARG_EXC_IF(res != -1, "Duplicate Key")

        KeyValue<T> * kv;
        H3R_CREATE_OBJECT(kv, KeyValue<T>) {key, value};
        _tbl.Insert (idx, &kv, 1);
    }
    private int _hit_cnt {}, _miss_cnt {};
    public bool TryGetValue(const Array<byte> & key, T & value)
    {
        int idx {-1};
        int res = BSearch (key, idx);
        if (res != -1) return value = _tbl[res]->Value, _hit_cnt++, true;
        return _miss_cnt++, false;
    }

    //TODO testme
    public int Count() const { return _tbl.Length (); }
    public KeyValue<T> ** begin()
    {
        return _tbl.operator KeyValue<T> ** ();
    }
    public KeyValue<T> ** end  ()
    {
        return _tbl.operator KeyValue<T> ** () + _tbl.Length ();
    }

#undef public
#undef private
};// ResNameHash

NAMESPACE_H3R

#endif