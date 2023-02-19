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

#ifndef _H3R_FFDPARSER_H_
#define _H3R_FFDPARSER_H_

#include "h3r.h"
#include "h3r_string.h"

H3R_NAMESPACE

#define H3R_ENSURE_FFD(C,M) { \
    if (! (C)) printf ("Error around line %d, column: %d; code:%s:%d: " M EOL, \
        parser.Line (), parser.Column (), __FILE__, __LINE__), \
        OS::Exit (1); }

#define FFD_SYMBOL_MAX_LEN 128
#define FFD_EXPR_MAX_NESTED_EXPR 10
#define FFD_EXPR_MAX_LEN 128
#define FFD_MAX_FIELDS 64
#define FFD_MAX_ENUM_ITEMS 64
#define FFD_MAX_ARR_EXPR_LEN 32

// File Format Description parser.
class FFDParser
{
#define public public:
#define private private:

    private const byte * _buf;
    private int _len;
    private int _i {0};
    private int _line {1};
    private int _column {1};
    public FFDParser(const byte * buf, int len) : _buf{buf}, _len{len} {}
    public ~FFDParser() {}

    public inline int Line() const { return _line; }
    public inline int Column() const { return _column; }
    public inline int Tell() const { return _i; }
    //public inline bool TokenIs(byte v) const { return byte == _buf[_i]; }
    public inline bool AtAttributeStart() const { return '[' == _buf[_i]; }
    public inline bool AtExprStart() const { return '(' == _buf[_i]; }
    public inline bool AtHashStart() const { return '<' == _buf[_i]; }
    public inline bool AtHashEnd() const { return '>' == _buf[_i]; }
    public inline bool AtArrEnd() const { return ']' == _buf[_i]; }
    public inline bool AtArrStart() const { return '[' == _buf[_i]; }
    public inline bool AtVariadicStart() const { return '.' == _buf[_i]; }
    // At value-list separator.
    public inline bool AtVListSep() const { return ':' == _buf[_i]; }
    public inline bool AtDoubleQuote() const { return '"' == _buf[_i]; }
    public inline FFDParser & SetCurrent(int idx) { return _i = idx, *this; }
    public inline const char * BufAt(int index) const
    {
        return reinterpret_cast<const char *>(_buf+index);
    }
    public inline String StringAt(int index, int num_bytes) const
    {
        return static_cast<String &&>(String {_buf+index, num_bytes});
    }

    public bool IsWhitespace();
    public bool IsLineWhitespace();
    public bool IsComment();
    public bool IsEol();
    public void SkipEol();
    public void SkipUntilDoubleEol();
    // read_while c()
    public template <typename F> inline void ReadWhile(const char * txt, F c)
    {
        int j = _i;
        while (_i < _len && c ()) {
            if (IsEol ()) { _line++; _column = 0; }
            _i++; _column++;
        }
        auto parser = *this;
        printf ("%3d: read_%s    : [%5d;%5d]" EOL, _line, txt, j, _i);
        H3R_ENSURE_FFD(_i > j, "Empty read_while")
    }
    public void ReadNonWhiteSpace();
    public void SkipWhitespace();
    public void SkipLineWhitespace();
    public void SkipComment();
    public String ReadSymbol(char stop_at = '\0', bool allow_dot = false);
    public int ParseIntLiteral();
    public String ReadExpression(char open = '(', char close = ')');
    public void SkipCommentWhitespaceSequence();

    public bool SymbolValid1st();
    // When "unicode", this shall become "const byte * + len"
    private bool SymbolValid1st(byte);
    private bool SymbolValidNth(byte);
    public String ReadValueList();
    public inline void SkipOneByte() { _i++; }

    public inline bool HasMoreData() { return _i < _len; }
    public void ReadVariadicField(); // _i shall be at the 1st "."
    // _i shall be right after the [. Later i shall be right after the ].
    // The result is between the [].
    public String ReadArrDim();
    // _i is at the opening ". Later i shall be after the closing one.
    // The result is between them.
    public String ReadStringLiteral();

#undef public
#undef private
};// FFDParser

NAMESPACE_H3R

#endif