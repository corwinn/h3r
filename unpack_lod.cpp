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

// Q'N'D code.

// lod file format unpacker; it unpacks to the current directory!
//c clang++ -std=c++11 unpack_lod.cpp -o unpack_lod -lz

#define int int
static_assert(4 == sizeof(int), "define 32-bit int type");

#include <stdio.h>
#include <zlib.h>
#include <stdlib.h>

struct File
{
    FILE * f {};
    File(const char * n, const char * mode = "rb") : f {fopen (n, mode)} {}
    ~File() { if (f) fclose (f), f = 0; }
    operator bool() { return 0 != f; }
    bool Seek(long pos) { return *this ? 0 == fseek (f, pos, SEEK_SET) : false; }
    long Tell() { return *this ? ftell (f) : -1; }
    template <typename T> bool Read(T & v, unsigned int n = 1)
    {
        auto b = n * sizeof (T);
        return *this ? ! b ? false : b == fread (&v, 1, b, f) : false;
    }
    template <typename T> bool Write(T & v, unsigned int n = 1)
    {
        auto b = n * sizeof (T);
        return *this ? ! b ? false : b == fwrite (&v, 1, b, f) : false;
    }
};

struct ZipStream
{
    z_stream s {};
    int r {~Z_OK};
    ZipStream() { r = inflateInit (&s); }
    ~ZipStream() { inflateEnd (&s); }
    operator bool() { return Z_OK == r; }
    bool Inflate(Bytef * a, unsigned int & ac, Bytef * b, unsigned int & bc)
    {
        // printf ("p1 in:%d, out:%d\n", ac, bc);
        s.avail_in = ac;
        s.next_in = a;
        s.avail_out = bc;
        s.next_out = b;
        r = inflate (&s, Z_FINISH);
        if (Z_STREAM_END == r) r = ! s.avail_in && ! s.avail_out ? Z_OK : r;
        inflateReset (&s);
        // printf ("p2 in:%d, out:%d, r:%d\n", s.avail_in, s.avail_out, r);
        return *this;
    }
};

template <typename T> struct Buf
{
    T * p {};
    Buf(unsigned int n = 1) { p = n ? (T*)malloc (n*sizeof(T)) : 0; }
    ~Buf() { if (p) free (p), p = 0; }
    operator bool() { return 0 != p; }
    operator T*() { return p; }
};

int main(int c, char ** v)
{
    if (2 != c)
        return printf ("usage: unpack_lod lodfile\n");
    File f {v[1]};
    if (! f)
        return printf ("error: failed to open \"%s\"\n", v[1]);
    printf ("Working with: \"%s\"\n", v[1]);
    int cnt = 0;
    if (! f.Read (cnt))
        return printf ("error: failed to read signature\n");
    if (0x444f4c != cnt)
        return printf ("error: not a lod file\n");
    if (! f.Seek (8))
        return printf ("error: failed to seek 1st 8 bytes\n");
    if (! f.Read (cnt))
        return printf ("error: failed to read item count\n");
    if (cnt <= 0 || cnt > 8192)
        return printf ("error: invalid entry count\n");
    if (! f.Seek (92))
        return printf ("error: failed to read header\n");
    printf ("Working with: %d entries\n", cnt);
    unsigned char fn[16] {};
    unsigned int of {}, s1 {}, t {}, s2 {};
    ZipStream zs;
    for (int i = 0; i < cnt; i++) {
        if (! f.Read (fn[0], 16))
            return printf ("error: failed to read item name\n");
        if (! f.Read (of))
            return printf ("error: failed to read item ofs\n");
        if (! f.Read (s1))
            return printf ("error: failed to read item s1\n");
        if (! f.Read (t))
            return printf ("error: failed to read item type\n");
        if (! f.Read (s2))
            return printf ("error: failed to read item s2\n");
        printf ("name: %16s, ofs: %8d, s1: %8d, t: %4d, s2: %8d\n",
            fn, of, s1, t, s2);
#ifdef LIST_ONLY
        continue;
#endif
        // slow and simple
        auto sentinel = f.Tell ();
        if (! f.Seek (of))
            return printf ("error: failed to seek %d\n", of);
        Buf<unsigned char> d2 {s1};
        if (s2 > s1)
            return printf ("error: the impossible has happened: %d<%d\n",
                s1, s2);
        else if (s2 > 0) { // compressed
            Buf<unsigned char> d1 {s2};
            if (! f.Read (*d1, s2))
                return printf ("error: item read failed\n");
            if (! zs.Inflate (d1, s2, d2, s1))
                return printf ("error: failed to decompress item\n");
        }
        else if (! f.Read (*d2, s1))
            return printf ("error: item read failed\n");
        File f1 {(char *)fn, "wb+"};
        if (! f1.Write (*d2, s1))
            return printf ("error: failed to write \"%s\"\n", (char *)fn);
        if (! f.Seek (sentinel))
            return printf ("error: failed to seek after item write\n");
    }
    return 0;
}
