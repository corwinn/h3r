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

#include "h3r_ffd.h"
#include "h3r_filestream.h"
#include "h3r_memorystream.h"
#include "h3r_os.h"

H3R_NAMESPACE

static int global_line = 1;
static int const SYMBOL_MAX_LEN {128};
static int const EXPR_MAX_NESTED_EXPR {10};
static int const EXPR_MAX_LEN {128};

#define H3R_ENSURE_FFD(C,M) { \
    if (! (C)) printf ("Error around line %d; code:%s:%d: " M EOL, \
        global_line, __FILE__, __LINE__), \
        OS::Exit (1); }

static inline bool is_whitespace(byte b) { return b <= 32; }
static inline bool is_comment(const byte * buf, int len, int i)
{
    return '/' == buf[i] && i < len-1 && ('/' == buf[i+1] || '*' == buf[i+1]);
}
static inline bool is_eol(const byte * buf, int len, int i)
{
    if ('\r' == buf[i] && i < len - 1) return '\n' == buf[i+1];
    return '\n' == buf[i];
}

// read_while c()
template <typename F> inline static void read_while(const char * txt,
    const byte * buf, int len, int & i, F c)
{
    int j = i;
    while (i < len && c ()) {
        if (is_eol (buf, len, i)) global_line++;
        i++;
    }
    H3R_ENSURE_FFD(i > j, "Empty read_while")
    printf ("%3d: read_%s    : [%5d;%5d]" EOL, global_line, txt, j, i);
}

static void skip_whitespace(const byte * buf, int len, int & i)
{
    read_while ("whitespc", buf, len, i, [&](){return buf[i] <= 32;});
}

// ensure is_comment() returns true prior calling this one
static void skip_comment(const byte * buf, int len, int & i)
{
    if ('/' == buf[i+1])
        read_while ("comment1", buf, len, i, [&](){return buf[i] != '\n';});
    else if ('*' == buf[i+1])
        // Yes, that would be slow if I had to parse 60 files per second ...
        read_while ("commentn", buf, len, i,
              [&](){return !('*' == buf[i] && i < len-1 && '/' == buf[i+1]);});
}

// 1000 classes? No thanks.
template <typename F> struct KwParser final
{
    const int KwLen; // no need to compute it
    const char * Kw;
    F Parse;
};

/*static void skip_nonwhitespace(const byte * buf, int len, int & i)
{
    read_while("parse_me", buf, len, i,
        [&](){return buf[i] > 32 && buf[i] <= 126;});
}*/

// When "unicode", this shall become "const byte * + len"
static inline bool symbol_valid_1st(byte b)
{
    // [A-Za-z_]
    return (b >= 'A' && b <= 'Z') || (b >= 'a' && b <= 'z') || '_' == b;
}

static inline bool symbol_valid_nth(byte b)
{
    // [0-9A-Za-z_]
    return (b >= '0' && b <= '9') || symbol_valid_1st (b);
}

static void read_symbol(const byte * buf, int len, int & i)
{
    H3R_ENSURE_FFD(symbol_valid_1st (buf[i]), "Wrong symbol name")
    int j = i;
    read_while("symbol  ", buf, len, i,
        [&](){return buf[i] > 32 && buf[i] <= 126;});
    H3R_ENSURE_FFD((i - j) <= SYMBOL_MAX_LEN, "Symbol too long")
    for (int k = j; k < i; k++) // repeating, but simplifies the code
        H3R_ENSURE_FFD(symbol_valid_nth (buf[k]), "Wrong symbol name")
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
static inline int parse_int_literal(const byte * buf, int len, int & i)
{
    static int const N[16] {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    int result = 1; bool h = false;
    if ('-' == buf[i]) { result = -1; i++; }
    else if ('0' == buf[i] && i+1 < len && 'x' == buf[i+1]) { i+=2; h = true; }
    // 123 = 3*(10^0) + 2*(10^1) + 1*(10^2)
    // 1ff = f*(16^0) + f*(16^1) + 1*(16^2)
    H3R_ENSURE_FFD(i < len, "Incomplete integer literal") // -EOF
    // integer: nnnnnnnnnn; hexadecimal int: nnnnnnnn
    int j = i, tmp_result = 0;
    if (h) {
        read_while ("int lit.", buf, len, i,
                    [&](){return is_hexadecimal_number (buf[i]);});
        H3R_ENSURE_FFD(i - j <= 8, "Integer literal too long")
        for (int k = i-1, p = 1; k >= j; k--, p *= 16) {
            int number = is_decimal_number (buf[k]) ? buf[k] - '0'
                : 10 + buf[k] - (is_upper (buf[k]) ? 'A' : 'a');
            tmp_result += N[number] * p;
        }
    }
    else {
        read_while ("int lit.", buf, len, i,
                    [&](){return is_decimal_number (buf[i]);});
        H3R_ENSURE_FFD(i - j <= 10, "Integer literal too long")
        for (int k = i-1, p = 1; k >= j; k--, p *= 10)
            tmp_result += N[buf[k] - '0'] * p;
    }
    return result * tmp_result;
}// parse_int_literal()

static void read_expression(const byte * buf, int len, int & i)
{
    int b = 0;
    H3R_ENSURE_FFD('(' == buf[i], "An expression must start with (")
    int j = i;
    read_while("expr.   ", buf, len, i,
        [&](){
            // Wrong symbols shall be detected at the evaluator - no point to
            // evaluate here.
            if ('(' == buf[i]) b++;
            else if (')' == buf[i]) b--;
            H3R_ENSURE_FFD(b >= 0 && b <= EXPR_MAX_NESTED_EXPR, "Wrong expr.")
            return b > 0 && buf[i] >= 32 && buf[i] <= 126;});
    // It shall complete on ')'
    // printf ("buf[i]: %2X, j:%5d, i:%5d" EOL, buf[i], j, i);
    H3R_ENSURE_FFD(')' == buf[i], "Incomplete expr.")
    H3R_ENSURE_FFD(0 == b, "Bug: incomplete expr.")
    i++;
    H3R_ENSURE_FFD((i - j) <= EXPR_MAX_LEN, "Expr. too long")
}

FFD::FFD() {}
FFD::~FFD() {}

bool FFD::SNode::Parse(const byte * buf, int len, int & i)
{
    static int const KW_LEN {9};
    static KwParser<bool (FFD::SNode::*)(const byte * buf, int len, int & i)>
        const KW[KW_LEN] {
            {4, "type", &FFD::SNode::ParseMachType},
            {3, "???", &FFD::SNode::ParseLater},
            {1, "[", &FFD::SNode::ParseAttribute},
            {4, "list", &FFD::SNode::ParseLater},
            {5, "table", &FFD::SNode::ParseLater},
            {6, "struct", &FFD::SNode::ParseStruct},
            {6, "format", &FFD::SNode::ParseStruct},
            {5, "const", &FFD::SNode::ParseConst},
            {4, "enum", &FFD::SNode::ParseEnum}};

    int j = i;
    read_while("parse_me", buf, len, i,
        [&](){return buf[i] > 32 && buf[i] <= 126;});
    int kw_len = i - j;
    for (auto & kwp : KW)
        if (kwp.KwLen == kw_len && ! OS::Strncmp (
            kwp.Kw, reinterpret_cast<const char *>(buf+j), kw_len)) {
            if ((this->*kwp.Parse) (buf, len, i)) return true;
        }
    return false;
}// FFD::SNode::Parse()

static inline void printf_range(const byte * buf, int a , int b)
{
    for (int i = a; i < b; i++) printf ("%c", buf[i]);
}

bool FFD::SNode::ParseMachType(const byte * buf, int len, int & i)
{
    // i points right after "type". Single line. An EOL completes it.
    H3R_ENSURE_FFD(i < len, "Incomplete machine type")                // typeEOF
    H3R_ENSURE_FFD(! is_eol (buf, len, i), "Incomplete machine type") // typeEOL
    skip_whitespace (buf, len, i);
    // Name
    int j = i;
    read_symbol (buf, len, i);
    printf ("MachType: Symbol: "); printf_range (buf, j, i); printf (EOL);
    Name = static_cast<String &&>(String {buf, i-j});
    skip_whitespace (buf, len, i);
    // Size or alias.
    if (symbol_valid_1st (buf[i])) { // alias
        j = i;
        read_symbol (buf, len, i);
        printf ("MachType: Alias: "); printf_range (buf, j, i); printf (EOL);
        // Alias = Find (String {buf, i-j})
        // Size = Alias->Size;
    }
    else {// size
        Size = parse_int_literal (buf, len, i);
        if (Size < 0) {Signed = true; Size = -Size;}
        printf ("MachType: %d bytes, %s" EOL,
                Size, (Signed ? "signed" : "unsigned"));
    }
    if (is_eol (buf, len, i)) return true; // completed
    // There could be an expression
    skip_whitespace (buf, len, i);
    if ('(' == buf[i]) {
        j = i;
        read_expression (buf, len, i);
        printf ("MachType: Expr: "); printf_range (buf, j, i); printf (EOL);
        Expr = static_cast<String &&>(String {buf, i-j});
    }
    // There could be a comment. But it is handled elsewhere for now.
    return true;
}// FFD::SNode::ParseMachType()

bool FFD::SNode::ParseLater(const byte *, int, int &) { return false; }
bool FFD::SNode::ParseAttribute(const byte *, int, int &) { return false; }
bool FFD::SNode::ParseStruct(const byte *, int, int &) { return false; }
bool FFD::SNode::ParseField(const byte *, int, int &) { return false; }
bool FFD::SNode::ParseConst(const byte *, int, int &) { return false; }
bool FFD::SNode::ParseEnum(const byte *, int, int &) { return false; }

/*static*/ FFD::Node * FFD::File2Tree(const String & d, const String &)
{
    OS::FileStream fh {d, H3R_NS::OS::FileStream::Mode::ReadOnly};
    MemoryStream br {&fh, static_cast<int>(fh.Size ())};
    const byte * buf = br.Buffer ();
    int len = static_cast<int>(br.Size ());

    // pre-validate - simplifies the parser
    for (int i = 0; i < len; i++) {
        H3R_ENSURE(
            '\n' == buf[i] || '\r' == buf[i] || (buf[i] >= 32 && buf[i] <= 126),
            "Wrong chars at description")
    }

    printf ("parsing %d bytes ffd" EOL, len);
    // 1. Read the nodes in TB LR manner
    for (int i = 0, chk = 0; i < len; chk++) {
        if (is_whitespace (buf[i])) skip_whitespace (buf, len, i);
        // Skipped, for now
        else if (is_comment (buf, len, i)) skip_comment (buf, len, i);
        else {
            FFD::SNode node;
            node.Parse (buf, len, i);
        }
        H3R_ENSURE_FFD(chk < len, "infinite loop")
    }// (int i = 0; i < len;)
    // 2. Fasten DType - only those with "! Expr.Empty ()" shall remain null -
    //    they're being resolved at "runtime".

    return nullptr;
}// FFD::File2Tree()

#undef H3R_ENSURE_FFD

NAMESPACE_H3R
