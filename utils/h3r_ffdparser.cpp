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

#include "h3r_ffdparser.h"
#include "h3r_os.h"

H3R_NAMESPACE

#define H3R_ENSURE_LFFD(C,M) { \
    if (! (C)) { Dbg << "Error around line " << (*this).Line () \
        << ", column: " << (*this).Column () \
        << "; code:" << __FILE__ << ":" << __LINE__ << ": " << M <<  EOL; \
        OS::Exit (1); }}

bool FFDParser::IsWhitespace() { return _buf[_i] <= 32; }
static bool is_line_whitespace(byte b) { return 32 == b || 9 == b; }
bool FFDParser::IsLineWhitespace() { return 32 == _buf[_i] || 9 == _buf[_i]; }
bool FFDParser::IsComment()
{
    return '/' == _buf[_i] && _i < _len-1
        && ('/' == _buf[_i+1] || '*' == _buf[_i+1]);
}
bool FFDParser::IsEol()
{
    if ('\r' == _buf[_i] && _i < _len-1) return '\n' == _buf[_i+1];
    return '\n' == _buf[_i];
}
void FFDParser::SkipEol()
{
    if ('\r' == _buf[_i] && _i < _len-1 && '\n' == _buf[_i+1])
        _i+=2, _line++, _column = 1;
    else if ('\n' == _buf[_i]) _i++, _line++, _column = 1;
}

// afterwards:
// i points: \n\n
//            i
//           \r\n\r\n
//              i
void FFDParser::SkipUntilDoubleEol()
{
    ReadWhile ("later   ", [&](){
        if ('\n' != _buf[_i]) return true;
        if (_i < _len-1 && '\n' == _buf[_i+1]) return false; // LFLF
        if ((_i > 0 && '\r' == _buf[_len-1])
            && (_i < _len-2 && '\r' == _buf[_i+1] && '\n' == _buf[_i+2]))
            return false; // CRLFCRLF
        return true;
    });
}

void FFDParser::ReadNonWhiteSpace()
{
    ReadWhile ("nw space", [&](){ return _buf[_i] > 32 && _buf[_i] <= 126; });
}

void FFDParser::SkipWhitespace()
{
    ReadWhile ("w  space", [&](){ return _buf[_i] <= 32; });
}

void FFDParser::SkipLineWhitespace()
{
    // A perfect line.
    ReadWhile ("lwhitesp", [&](){ return IsLineWhitespace (); });
}

// ensure is_comment() returns true prior calling this one
void FFDParser::SkipComment()
{
    if ('/' == _buf[_i+1])
        ReadWhile ("comment1", [&](){ return ! IsEol (); });
    else if ('*' == _buf[_i+1]) {
        // Yes, that would be slow if I had to parse 60 files per second ...
        ReadWhile ("commentn", [&](){
            return ! ('*' == _buf[_i] && _i < _len-1 && '/' == _buf[_i+1]); });
        _i += 2; // it ends at '*', so
    }
}

bool FFDParser::SymbolValid1st() { return SymbolValid1st (_buf[_i]); }
/*static*/ bool FFDParser::SymbolValid1st(byte b)
{
    // [A-Za-z_]
    return (b >= 'A' && b <= 'Z') || (b >= 'a' && b <= 'z') || '_' == b;
}
/*static*/ bool FFDParser::SymbolValidNth(byte b)
{
    // [0-9A-Za-z_]
    return (b >= '0' && b <= '9') || SymbolValid1st (b);
}

// Handles variadic list symbols: foo.bar:value-list.
// char stop_at = '\0', bool allow_dot = false
// Handles expression symbols, like "foo)".
String FFDParser::ReadSymbol(char stop_at, bool allow_dot)
{
    H3R_ENSURE_LFFD(SymbolValid1st (), "Wrong symbol name")
    int j = _i;
    ReadWhile ("symbol  ", [&](){
        return _buf[_i] != stop_at && _buf[_i] > 32 && _buf[_i] <= 126; });
    H3R_ENSURE_LFFD((_i - j) <= FFD_SYMBOL_MAX_LEN, "Symbol too long")
    for (int k = j; k < _i; k++) // repeating, but simplifies the code
        H3R_ENSURE_LFFD(SymbolValidNth (_buf[k])
            || (allow_dot && '.' == _buf[k]), "Wrong symbol name")
    return static_cast<String &&>(String {_buf+j, _i-j});
}

static inline bool is_decimal_number(byte b)
{
    return b >= '0' && b <= '9';
}
static inline bool is_hexadecimal_number(byte b)
{
    return (b >= 'a' && b <= 'f') || (b >= 'A' && b <= 'F')
        || is_decimal_number (b);
}
static inline bool is_upper(byte b)
{
    return (b >= 'A' && b <= 'Z');
}

//TODO testme
int FFDParser::ParseIntLiteral()
{
    static int const N[16] {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    int result = 1; bool h = false;
    if ('-' == _buf[_i]) { result = -1; _i++; }
    else if ('0' == _buf[_i] && _i+1 < _len && 'x' == _buf[_i+1]) {
        _i += 2;
        h = true;
    }
    // 123 = 3*(10^0) + 2*(10^1) + 1*(10^2)
    // 1ff = f*(16^0) + f*(16^1) + 1*(16^2)
    H3R_ENSURE_LFFD(_i < _len, "Incomplete integer literal") // -EOF
    // integer: nnnnnnnnnn; hexadecimal int: nnnnnnnn
    int j = _i, tmp_result = 0;
    if (h) {
        ReadWhile ("int lit.",
                    [&](){ return is_hexadecimal_number (_buf[_i]); });
        H3R_ENSURE_LFFD(_i - j <= 8, "Integer literal too long")
        for (int k = _i-1, p = 1; k >= j; k--, p *= 16) {
            int number = is_decimal_number (_buf[k]) ? _buf[k] - '0'
                : 10 + _buf[k] - (is_upper (_buf[k]) ? 'A' : 'a');
            tmp_result += N[number] * p;
        }
    }
    else {
        ReadWhile ("int lit.", [&](){ return is_decimal_number (_buf[_i]); });
        H3R_ENSURE_LFFD(_i - j <= 10, "Integer literal too long")
        for (int k = _i-1, p = 1; k >= j; k--, p *= 10)
            tmp_result += N[_buf[k] - '0'] * p;
    }
    return result * tmp_result;
}// FFDParser::ParseIntLiteral()

// Multi-line not handled.
String FFDParser::ReadExpression(char open, char close)
{
    H3R_ENSURE_LFFD(open != '(' && close != ')',
        "These are handled by TokenizeExpression()")
    int b = 0;
    H3R_ENSURE_LFFD(open == _buf[_i], "An expression must start with (")
    int j = _i;
    ReadWhile("expr.   ",
        [&](){
            // Wrong symbols shall be detected at the evaluator - no point to
            // evaluate here.
            if (open == _buf[_i]) b++;
            else if (close == _buf[_i]) b--;
            H3R_ENSURE_LFFD(
                b >= 0 && b <= FFD_EXPR_MAX_NESTED_EXPR, "Wrong expr.")
            return b > 0 && _buf[_i] >= 32 && _buf[_i] <= 126;});
    // It shall complete on "close"
    H3R_ENSURE_LFFD(close == _buf[_i], "Incomplete expr.")
    H3R_ENSURE_LFFD(0 == b, "Bug: incomplete expr.")
    _i++;
    H3R_ENSURE_LFFD((_i - j) <= FFD_EXPR_MAX_LEN, "Expr. too long")
    return static_cast<String &&>(String {_buf+j, _i-j});
}

// Skip any sequence of white-space and comments (MAX_SW_ITEMS max), up to and
// including an EOL|EOF (multi-line comment EOL(s) doesn't count).
// Long name on purpose. TODO re-design me for a short name, or modify the
// grammar.
void FFDParser::SkipCommentWhitespaceSequence()
{
    int const MAX_SW_ITEMS {10};
    for (int chk = 0; _i < _len; chk++) {
        H3R_ENSURE_LFFD(chk < MAX_SW_ITEMS,
            "I'm sorry, this ain't a novelette file format")
        if (IsLineWhitespace ()) SkipLineWhitespace ();
        else if (IsComment ()) SkipComment ();
        else if (IsEol ()) {
            SkipEol ();
            break;
        }
        else
            H3R_ENSURE_LFFD(42^42, "Unexpected element")
    }
}

String FFDParser::ReadValueList()
{
    int j = _i;
    ReadWhile ("val-list", [&](){ return
        is_decimal_number (_buf[_i]) || '-' == _buf[_i] || ',' == _buf[_i]; });
    return static_cast<String &&>(String {_buf+j, _i-j});
}

void FFDParser::ReadVariadicField()
{
    H3R_ENSURE_LFFD(_i < _len-2, "Incomplete variadic field")
    H3R_ENSURE_LFFD('.' == _buf[_i+1], "Incomplete variadic field")
    H3R_ENSURE_LFFD('.' == _buf[_i+2], "Incomplete variadic field")
    _i+=3;
}

String FFDParser::ReadArrDim()
{
    int s = _i;
    ReadWhile ("arr     ", [&](){ return ']' != _buf[_i]; });
    H3R_ENSURE_LFFD(']' == _buf[_i], "Incomplete array")
    H3R_ENSURE_LFFD(_i-s <= FFD_MAX_ARR_EXPR_LEN,
        "Simplify your array expression")
    _i++;
    return static_cast<String &&>(String {_buf+s, _i-1-s});
}

String FFDParser::ReadStringLiteral()
{
    H3R_ENSURE_LFFD(_i < _len-1, "Incomplete string literal") // "EOF
    //LATER fix: yep, it shall read "a"a"a"
    return static_cast<String &&>(ReadExpression ('"', '"'));
}

// Handle supported operations: > < == >= <= != || && ! .
// Makes FFDParser::TokenizeExpression wy more read-able.
FFDParser::ExprTokenType FFDParser::TokenizeExpressionOp()
{
    H3R_ENSURE_LFFD(_i < _len-2, "Incomplete expr.")
    switch (_buf[_i]) {
        case '!':
            switch (_buf[_i+1]) {
                case '=' :
                    H3R_ENSURE_LFFD(is_line_whitespace (_buf[_i+2]), "Wrong Op")
                    Dbg << "!= "; return _i+=2, ExprTokenType::opNE;
                case '(' : Dbg << "! "; return ++_i, ExprTokenType::opN;
                default:
                    H3R_ENSURE_LFFD(SymbolValid1st (_buf[_i+1]), "Wrong Op");
                    Dbg << "! "; return ++_i, ExprTokenType::opN;
            }
        case '<':
            if (is_line_whitespace (_buf[_i+1]))
                return ++_i, Dbg << "< ", ExprTokenType::opL;
            H3R_ENSURE_LFFD('=' == _buf[_i+1], "Wrong Op")
            H3R_ENSURE_LFFD(is_line_whitespace (_buf[_i+2]), "Wrong Op")
            Dbg << "<= ";
            return _i+=2, ExprTokenType::opLE;
        case '>':
            if (is_line_whitespace (_buf[_i+1]))
                return ++_i, Dbg << "> ", ExprTokenType::opG;
            H3R_ENSURE_LFFD('=' == _buf[_i+1], "Wrong Op")
            H3R_ENSURE_LFFD(is_line_whitespace (_buf[_i+2]), "Wrong Op")
            Dbg << ">= ";
            return _i+=2, ExprTokenType::opGE;
        case '=':
            H3R_ENSURE_LFFD('=' == _buf[_i+1], "Wrong Op")
            H3R_ENSURE_LFFD(is_line_whitespace (_buf[_i+2]), "Wrong Op")
            Dbg << "== ";
            return _i+=2, ExprTokenType::opE;
        case '|':
            H3R_ENSURE_LFFD('|' == _buf[_i+1], "Wrong Op")
            H3R_ENSURE_LFFD(is_line_whitespace (_buf[_i+2]), "Wrong Op")
            Dbg << "|| ";
            return _i+=2, ExprTokenType::opOr;
        case '&':
            H3R_ENSURE_LFFD('&' == _buf[_i+1], "Wrong Op")
            H3R_ENSURE_LFFD(is_line_whitespace (_buf[_i+2]), "Wrong Op")
            Dbg << "&& ";
            return _i+=2, ExprTokenType::opAnd;
        default: H3R_ENSURE_LFFD(1^1, "Unknown Op")
    }// switch (_buf[_i])
}// FFDParser::TokenizeExpressionOp()

// Handle (.*)
List<FFDParser::ExprToken> FFDParser::TokenizeExpression()
{
    Dbg << "TokenizeExpression: ";
    List<FFDParser::ExprToken> result {}; // (, foo, )
    int depth {};
    int chk {};
    do {
        if ('(' == _buf[_i]) { Dbg << "( ";
            H3R_ENSURE_LFFD(chk++ < FFD_EXPR_MAX_NESTED_EXPR, "Wrong expr.")
            depth++; _i++;
            result.Put (FFDParser::ExprToken {ExprTokenType::Open});
        }
        else if (')' == _buf[_i]) { Dbg << ") ";
            depth--; if (depth) _i++;
            result.Put (FFDParser::ExprToken {ExprTokenType::Close});
        }
        else if (SymbolValid1st (_buf[_i])) {
            FFDParser::ExprToken t {ExprTokenType::Symbol};
            t.Symbol = ReadSymbol (')', true); Dbg << "{" << t.Symbol << "} ";
            result.Put (static_cast<FFDParser::ExprToken &&>(t));
        }
        else if (is_decimal_number (_buf[_i])) {
            FFDParser::ExprToken t {ExprTokenType::Number};
            t.Value = ParseIntLiteral (); Dbg << t.Value << " ";
            result.Put (static_cast<FFDParser::ExprToken &&>(t));
        }
        else if (IsLineWhitespace ()) { Dbg << "{} "; SkipLineWhitespace (); }
        else
            result.Put (FFDParser::ExprToken {TokenizeExpressionOp ()});
    } while (depth && _i < _len);
    H3R_ENSURE_LFFD(')' == _buf[_i], "Incomplete expr.")
    H3R_ENSURE_LFFD(0 == depth, "Bug: incomplete expr.")
    _i++;
    Dbg << EOL;
    return static_cast<List<ExprToken> &&>(result);
}// FFDParser::TokenizeExpression()

#undef H3R_ENSURE_LFFD

NAMESPACE_H3R
