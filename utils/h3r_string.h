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

#ifndef _H3R_STRING_H_
#define _H3R_STRING_H_

#include "h3r.h"
#include "h3r_array.h"

H3R_NAMESPACE

// Text string. Wraps "Array<byte>", and adds a few utilities to make your code
// short and simple.
// "Array<byte>" has nothing to do with your text encoding.
// All instance utility functions shall leave the string in an immutable state:
//TODO public: String Trim(const Array<byte> & tokens, TrimFlags f) const;
//     public: String Replace(
//                 const Array<byte> & what, byte with, ReplaceFlags f) const;
// ... etc. - on as needed basis.
//TODONT optimize <=> don't use "String" when you need speed
//LATER pool; 1Mb initial; Array has A, F params
class String final
{
    private: Array<byte> _b {};
    private: template <typename T> String & append(const T * buf, size_t num);
    private: String & append(const String &);

    public: String() {}
    public: String(const char *); // Zero-terminated C-style string - often
    public: String(const byte *); // used for log messages
    public: String(const Array<byte> &);
    public: String(Array<byte> &&);
    public: String(const String &);
    public: String & operator=(const String &);
    public: String(String &&);
    public: String & operator=(String &&);

    //TODO solve: assertion will fail when "hidden" MAX_BUF_SIZE is reached
    public: static String Format(const char *, ...);

    // Direct access. "const" means: "its advised to not modify me".
    // This is the recommended way to "print" the string:
    //   FileStream.Write (foo.AsByteArray (), foo.Length ())
    public: inline const Array<byte> & AsByteArray() const { return _b; }
    public: size_t Length() const; // forward to "AsByteArray ().Length ()"

    public: String & operator+=(const String &);
    public: String & operator+=(const char *);
    public: inline String & operator+=(const char c)
    {
        _b.Append (&c, 1); return *this;
    }
    public: String operator+(const String &);
    public: String operator+(const char *);

    public: inline String operator+(const char c)
    {
        return String {*this} += c;
    }
    public: bool operator==(const String &) const;
    public: bool operator!=(const String & b) const { return ! (*this == b); }
    public: bool inline EqualsZStr(const char * b) const
    {
        var b_len = OS::Strlen (b);
        if (b_len != _b.Length ()) return false;
        return ! OS::Strncmp (b, (const char *)_b.Data (), b_len);
    }

    //TODO this thing is causing too much copying around
    public: inline Array<char> AsZStr() const
    {
        Array<char> zstr;
        if (_b.Length () > 0) zstr.Append (_b.Data (), _b.Length ());
        // 8 zero bytes - should be enough for any encoding
        return zstr.Resize (zstr.Length () + 8), zstr;
    }

    public: String ToLower() const;

    public: inline bool EndsWith(const char * s)
    {
        var a = _b.Length ();
        var b = OS::Strlen (s);
        if (b > a) return false;
        return ! OS::Strncmp (
            reinterpret_cast<const char *>((_b.Data () + a) - b), s, b);
    }
};

String operator+(const char *, const String &);

NAMESPACE_H3R

#endif