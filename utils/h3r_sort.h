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

// Sorting is a very simple problem - its so simple, it hasn't been solved yet.

#ifndef _H3R_SORT_H_
#define _H3R_SORT_H_

#include "h3r.h"
#include "h3r_os.h"

H3R_NAMESPACE

//  "next" - get the element after "a"
//  "last" - return true if "a" is the last element to sort
//  "cmp" - return a<b for descending order, a>b for ascending
//  "insert" - "b" prior "a"
// What do you know - you can do MVC in C :)
/*template <typename T> void sort(T * first,
    void (*next)(T * a)
    bool (*last)(T * a),
    bool (*cmp)(T * a, T * b),
    void (*insert)(T * a, T * b))
{
}*/

// I'm looking for a linked list.
template <typename T> struct ISortable // a view of your data
{
    // return nullptr for exit() with an assertion failed
    virtual T * first() { return nullptr; }
    // return nullptr when last; a != null ;
    // nullptr will get you a null dereference
    virtual T * next(T * a) { (void)a; return nullptr; }
    // return a<=b for descending, or a>=b for ascending sort;
    // a != null, b != null; nullptr will get you a null dereference
    virtual bool cmp(T * a, T * b) { (void)a; (void)b; return false; }
    // insert "b" prior "a"; b != null ; nullptr as "b" will get you a null
    // dereference; when "a" is nullptr "b" is the new last one
    virtual void insert(T * a, T * b) { (void)a; (void)b; }
};

namespace {
// "a" and "b" are sorted, and no more than "n"-long, and a+n refers b
template <typename T> void merge(ISortable<T> & c, T * a, T * & b, int n)
{
    int i = 0, j = 0;
    T * t;
    for (; nullptr != a && nullptr != b && j < n && i < n;) {
        if (c.cmp (a, b)) t = c.next (b), c.insert (a, b), b = t, j++;
        else a = c.next (a), i++;
    }

    // move b in case it wasn't
    while (j < n && nullptr != b) b = c.next (b), j++;
    //TODO not sure if the above is merge() or sort() job
}

// debug helper
template <typename T> void print_c(ISortable<T> & c)
{
    int count {0};
    OS::Log_stdout (" ");
    for (auto a = c.first (); nullptr != a; a = c.next (a))
        OS::Log_stdout (" node: %p;", a), count++;
    OS::Log_stdout (" count: %d" EOL, count);
}
} // namespace {

// 2^24 calls (-O0 -g -fexceptions -fsanitize=address,undefined,integer) with
// an array of 8 ints - all permutations of the integer numbers [1;8], both
// asc. and desc.:
//  *     my sort() [seconds] -> 155, 153, 155, 155, 154 // roundf()
//  * POSIX qsort() [seconds] ->  86,  83,  85,  85,  85 // roundf()
// ~2 times slower than qsort() for: 10 times "more simple"; MVC; no unchecked
// typecasting; and no recursion; - is very good enough for me. Yes
// numbers [1;8] can be "sorted" way faster than any sort() - but that's not the
// point here. The qsort() is a debug build as well - of "musl" libc.
//
// An array of 1<<20 distinct integers:
//  *     my sort() [seconds] -> 5.554, 5.503, 5.575, 5.449, 5.473
//  * POSIX qsort() [seconds] -> 2.620, 2.590, 2.636, 2.585, 2.587
//
// And I haven't even optimized it yet, neither did the compiler. Ou - it could
// use multiple threads if needs be - just sayin'.
template <typename T> void sort(ISortable<T> & c)
{
    H3R_ENSURE(nullptr != c.first (), "don't pass me empty views")
    // OS::Log_stdout ("Initial   ->"), print_c (c);
    // sort each 2 by 2
    int count = 1;
    const int INF_PROTECTION {1<<23};
    for (auto * a = c.first (); nullptr != a;) {
        H3R_ENSURE(count < INF_PROTECTION, "what is it that you're sorting?")
        auto * b = c.next (a);
        if (nullptr == b) break; count++;
        //O S::Log_stdout ("cmp (%p, %p)" EOL, a, b);
        if (c.cmp (a, b)) c.insert (a, b), a = c.next (a);
        else a = c.next (b); if (a) count++;
    }
    // OS::Log_stdout ("After 2x2 ->"), print_c (c);
    // OS::Log_stdout ("  count: %d" EOL, count);
    if (count <= 2) return;

    // merge
    int len = 2;
    for (auto * a = c.first () ; len <= count;) {
        auto * b = a;
        for (int i = 0; nullptr != b && i < len; i++)
            b = c.next (b);
        // OS::Log_stdout (" merge (c, %p, %p, %d)" EOL, a, b, len);
        merge (c, a, b, len);
        // OS::Log_stdout (" afer merge (c, %p, %p, %d)" EOL, a, b, len);
        if (nullptr == b) len *= 2, a = c.first ();
        else a = b;
    }
    // OS::Log_stdout ("Final     ->"), print_c (c);
} // void sort(ISortable<T> & c)

NAMESPACE_H3R

#endif

// Sorting is not solved still.