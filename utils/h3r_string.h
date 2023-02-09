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

#define public public:
#define private private:

// Lesson(s) learned:
//
//   * The use-cases proved that it shall manage 0-es at its end. The "const
//     char *" is required at too many places, so it shall not cause copy.

// Text string. Wraps "Array<byte>", and adds a few utilities to make your code
// short and simple.
// "Array<byte>" has nothing to do with your text encoding.
// All instance utility functions shall leave the string in an immutable state:
//TODO public: String Trim(const Array<byte> & tokens, TrimFlags f) const;
//     public: String Replace(
//                 const Array<byte> & what, byte with, ReplaceFlags f) const;
// ... etc. - on as needed basis.
//TODONT optimize <=> don't use "String" when you need speed
class String final
{
    private static constexpr int _NZ {4}; // number of zeroes
    private Array<byte> _b {};

    private template <typename T> String & append(const T * buf, int num);

    public String() {}
    public String(const char *); // Zero-terminated

    // Warning! All 0 at the end shall be ignored!
    public explicit String(const byte *, int);

    private String(Array<byte> &&);
    public String(const String &);
    public String & operator=(const String &);
    public String(String &&);
    public String & operator=(String &&);

    //TODO solve: assertion will fail when "hidden" MAX_BUF_SIZE is reached
    public static String Format(const char *, ...);

    public inline int Length() const
    {
        H3R_ENSURE(0 == _b.Length () || _b.Length () >= _NZ,
            "Unknown state")
        return _b.Length () < _NZ ? 0 : _b.Length () - _NZ;
    }

    public String & operator+=(const String &);
    public String & operator+=(const char *);
    public inline String & operator+=(const char c) { return append (&c, 1); }
    public String operator+(const String &);
    public String operator+(const char *);

    // This gets selected for "const char * = (String + size_t)"
    //LATER see the standard; understand the logic if any
    // The safety measure remains in place.
    public template <typename NoYouDont> String operator+(NoYouDont) = delete;
    public template <> String operator+(const char c)
    {
        return String {*this} += c;
    }
    // See EndsWith() for a trigger.
    // public template <> String operator+(size_t) = delete;

    public bool operator==(const String &) const;
    public bool inline operator!=(const String & b) const
    {
        return ! operator== (b);
    }
    public bool inline operator==(const char * b) const
    {
        auto b_len = static_cast<int>(OS::Strlen (b));
        if (b_len != Length ()) return false;
        return ! OS::Strncmp (b, *this, b_len);
    }
    public inline operator const char *() const
    {
        static char empty[_NZ] {};
        if (Length () <= 0) return empty;
        return reinterpret_cast<const char *>(_b.Data ());
    }
    // Convenience method for variadic functions.
    public inline const char * AsZStr() const { return *this; }
    // Direct access.
    // The recommended way to "print" the string:
    //   FileStream.Write (foo.AsByteArray (), foo.Length ())
    public inline const byte * AsByteArray() const { return _b.Data (); }
    // Avoid pointless copying. Take care: String.Length() and Array.Length()
    // might differ. TODO testme
    public inline explicit operator const Array<byte> &() const { return _b; }

    // Using POSIX tolower().
    public String ToLower() const;

    public inline bool EndsWith(const char * s)
    {
        auto a = Length ();
        auto b = static_cast<int>(OS::Strlen (s));
        if (b > a) return false;
        //DONE why is this compiling ?! (the test segfaults)
        //     return ! OS::Strncmp (((*this + a) - b), s, b);
        //     Ok it calls "String operator+(const char c)"; because ?!
        return ! OS::Strncmp (((this->operator const char * () + a) - b), s, b);
    }

    public String Replace(const char *, const char *);

    public bool Empty() const { return ! (Length () > 0); }
};// String

inline bool operator==(const char * c, const String & s)
{
    return s.operator== (c);
}

String operator+(const char *, const String &);

#undef private
#undef public

NAMESPACE_H3R

#endif