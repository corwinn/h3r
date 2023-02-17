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

//TODO What am I? -> "const byte * buf, int len, int & i"

#include "h3r_ffd.h"
#include "h3r_filestream.h"
#include "h3r_memorystream.h"
#include "h3r_os.h"

H3R_NAMESPACE

static int global_line = 1;
static int global_column = 1;
static int const FFD_SYMBOL_MAX_LEN {128};
static int const FFD_EXPR_MAX_NESTED_EXPR {10};
static int const FFD_EXPR_MAX_LEN {128};
static int const FFD_MAX_FIELDS {64};
static int const FFD_MAX_ENUM_ITEMS {64};
static int const FFD_MAX_ARR_EXPR_LEN {32};

#define H3R_ENSURE_FFD(C,M) { \
    if (! (C)) printf ("Error around line %d, column: %d; code:%s:%d: " M EOL, \
        global_line, global_column, __FILE__, __LINE__), \
        OS::Exit (1); }

static inline bool is_whitespace(byte b) { return b <= 32; }
static inline bool is_line_whitespace(byte b) { return 32 == b || 9 == b; }
static inline bool is_comment(const byte * buf, int len, int i)
{
    return '/' == buf[i] && i < len-1 && ('/' == buf[i+1] || '*' == buf[i+1]);
}
static inline bool is_eol(const byte * buf, int len, int i)
{
    if ('\r' == buf[i] && i < len-1) return '\n' == buf[i+1];
    return '\n' == buf[i];
}
static inline void skip_eol(const byte * buf, int len, int & i)
{
    if ('\r' == buf[i] && i < len-1 && '\n' == buf[i+1])
        i+=2, global_line++, global_column = 1;
    else if ('\n' == buf[i]) i++, global_line++, global_column = 1;
}

// read_while c()
template <typename F> inline static void read_while(const char * txt,
    const byte * buf, int len, int & i, F c)
{
    int j = i;
    while (i < len && c ()) {
        if (is_eol (buf, len, i)) { global_line++; global_column = 0; }
        i++; global_column++;
    }
    H3R_ENSURE_FFD(i > j, "Empty read_while")
    printf ("%3d: read_%s    : [%5d;%5d]" EOL, global_line, txt, j, i);
}

static void skip_whitespace(const byte * buf, int len, int & i)
{
    read_while ("whitespc", buf, len, i, [&](){return buf[i] <= 32;});
}

static void skip_line_whitespace(const byte * buf, int len, int & i)
{
    read_while ("lwhitesp", buf, len, i,
        [&](){return is_line_whitespace (buf[i]);});
}

// ensure is_comment() returns true prior calling this one
static void skip_comment(const byte * buf, int len, int & i)
{
    if ('/' == buf[i+1])
        read_while (
            "comment1", buf, len, i, [&](){return ! is_eol (buf, len, i);});
    else if ('*' == buf[i+1]) {
        // Yes, that would be slow if I had to parse 60 files per second ...
        read_while ("commentn", buf, len, i,
              [&](){return !('*' == buf[i] && i < len-1 && '/' == buf[i+1]);});
        i+=2; // it ends at '*', so
    }
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

static void read_symbol(const byte * buf, int len, int & i,
    // handle variadic list symbols: foo.bar:value-list
    char stop_at = '\0', bool allow_dot = false)
{
    H3R_ENSURE_FFD(symbol_valid_1st (buf[i]), "Wrong symbol name")
    int j = i;
    read_while("symbol  ", buf, len, i,
        [&](){return buf[i] != stop_at && buf[i] > 32 && buf[i] <= 126;});
    H3R_ENSURE_FFD((i - j) <= FFD_SYMBOL_MAX_LEN, "Symbol too long")
    for (int k = j; k < i; k++) // repeating, but simplifies the code
        H3R_ENSURE_FFD(symbol_valid_nth (buf[k])
            || (allow_dot && '.' == buf[k]), "Wrong symbol name")
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

// Multi-line not handled.
static void read_expression(const byte * buf, int len, int & i,
    char open = '(', char close = ')')
{
    int b = 0;
    H3R_ENSURE_FFD(open == buf[i], "An expression must start with (")
    int j = i;
    read_while("expr.   ", buf, len, i,
        [&](){
            // Wrong symbols shall be detected at the evaluator - no point to
            // evaluate here.
            if (open == buf[i]) b++;
            else if (close == buf[i]) b--;
            H3R_ENSURE_FFD(
                b >= 0 && b <= FFD_EXPR_MAX_NESTED_EXPR, "Wrong expr.")
            return b > 0 && buf[i] >= 32 && buf[i] <= 126;});
    // It shall complete on "close"
    // printf ("buf[i]: %2X, j:%5d, i:%5d" EOL, buf[i], j, i);
    H3R_ENSURE_FFD(close == buf[i], "Incomplete expr.")
    H3R_ENSURE_FFD(0 == b, "Bug: incomplete expr.")
    i++;
    H3R_ENSURE_FFD((i - j) <= FFD_EXPR_MAX_LEN, "Expr. too long")
}

// Skip any sequence of white-space and comments (MAX_SW_ITEMS max), up to and
// including an EOL|EOF (multi-line comment EOL(s) doesn't count).
// Long name on purpose. TODO re-design me for a short name, or modify the
// grammar.
static inline void skip_comment_whitespace_sequence(
    const byte * buf, int len, int & i)
{
    int const MAX_SW_ITEMS {10};
    for (int chk = 0; i < len; chk++) {
        H3R_ENSURE_FFD(chk < MAX_SW_ITEMS,
            "I'm sorry, this ain't a novelette file format")
        if (is_line_whitespace (buf[i])) skip_line_whitespace (buf, len, i);
        else if (is_comment (buf, len, i)) skip_comment (buf, len, i);
        else if (is_eol (buf, len, i)) {
            skip_eol (buf, len, i);
            break;
        }
        else
            H3R_ENSURE_FFD(42^42, "Unexpected element")
    }
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
    // Attribute "correction"
    // printf ("global_column: %d, bif[j]:%2X" EOL, global_column, buf[j]);
    if (1 == global_column && '[' == buf[j])
        return ParseAttribute (buf, len, i);
    //
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
    skip_line_whitespace (buf, len, i);
    // Name
    int j = i;
    read_symbol (buf, len, i);
    printf ("MachType: Symbol: "); printf_range (buf, j, i); printf (EOL);
    Name = static_cast<String &&>(String {buf+j, i-j});
    skip_line_whitespace (buf, len, i);
    // Size or alias.
    if (symbol_valid_1st (buf[i])) { // alias
        j = i;
        read_symbol (buf, len, i);
        printf ("MachType: Alias: "); printf_range (buf, j, i); printf (EOL);
        // Alias = Find (String {buf+j, i-j})
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
    skip_line_whitespace (buf, len, i);
    if ('(' == buf[i]) {
        j = i;
        read_expression (buf, len, i);
        printf ("MachType: Expr: "); printf_range (buf, j, i); printf (EOL);
        Expr = static_cast<String &&>(String {buf+j, i-j});
    }
    // There could be a comment. But it is handled elsewhere for now.
    return true;
}// FFD::SNode::ParseMachType()

// Be careful: leaves i pointing to '\n', or '\r' in the '\r\n' case.
bool FFD::SNode::ParseLater(const byte * buf, int len, int & i)
{
    read_while ("later   ", buf, len, i,
        [&](){
            int ofs = 1;
            if ('\r' == buf[i] && i < len-1 && '\n' == buf[i])
                ofs = 2;
            return !(is_eol (buf, len, i)
                && i < len-ofs && is_eol (buf, len, i+ofs));});
    return true;
}

// ^[.*]
bool FFD::SNode::ParseAttribute(const byte * buf, int len, int & i)
{
    H3R_ENSURE_FFD(i < len, "Incomplete attribute")                // [EOF
    H3R_ENSURE_FFD(! is_eol (buf, len, i), "Incomplete attribute") // [EOL
    int j = i;
//np read_expression (buf: buf, len: len, i: i, open: '[', close: ']');
    read_expression (buf, len, i, '[', ']');
    printf ("Attribute: "); printf_range (buf, j, i); printf (EOL);
    Attribute = static_cast<String &&>(String {buf+j, i-j});
    return true;
}

// struct Obj.EdId:26 // "Event" <=> grep -n ^ ObjNames.txt | grep ^$((n+1))
bool FFD::SNode::ParseStruct(const byte * buf, int len, int & i)
{
    // i points right after "struct". Multi-line. An EOLEOL completes it.
    // Are these really necessary; save for verbose debugging?
    H3R_ENSURE_FFD(i < len, "Incomplete struct")                // structEOF
    H3R_ENSURE_FFD(! is_eol (buf, len, i), "Incomplete struct") // structEOL
    skip_line_whitespace (buf, len, i);
    int j = i;
//np read_symbol (buf: buf, len: len, i: i, stop_at: ':', allow_dot: true);
    read_symbol (buf, len, i, ':', true);
    printf ("Struct: "); printf_range (buf, j, i); printf (EOL);
    Name = static_cast<String &&>(String {buf+j, i-j});
    if (':' == buf[i]) {
        printf (", variadic list item" EOL);
        VListItem = true;
        i++;
        H3R_ENSURE_FFD(i < len, "Incomplete value-list")
        j = i;
        read_while ("val-list", buf, len, i,
            [&](){return
                is_decimal_number (buf[i]) || '-' == buf[i] || ',' == buf[i];});
        printf ("VList: "); printf_range (buf, j, i); printf (EOL);
    }
    // No expression.
    // It completes with EOLEOL or EOF, and contains fields.
    // Skip remaining white-space(s) and comment(s).
    H3R_ENSURE_FFD(i < len, "Incomplete struct")      // struct foo.*EOF
    // printf ("i (%002X) prior scws: %d" EOL, buf[i], i);
    skip_comment_whitespace_sequence (buf, len, i);
    // printf ("i after scws: %d" EOL, i);
    H3R_ENSURE_FFD(i < len, "Incomplete struct")      // struct foo.*EOLEOF
    H3R_ENSURE_FFD(! is_eol (buf, len, i), "Empty struct") // struct foo.*EOLEOL
    // Fields
    for (int chk = 0; ; chk++) {
        H3R_ENSURE_FFD(chk < FFD_MAX_FIELDS, "Refine your design")
        //TODO handle result; add to this->Fields
        ParseField (buf, len, i); // it skips its EOL if any
        // if (i < len)
        //    printf ("done reading a field. %d:%002X" EOL, i, buf[i]);
        if (i >= len) return true; // fieldEOF
        if (is_eol (buf, len, i)) {
            skip_eol (buf, len, i);
            if (i >= len) return true; // fieldEOLEOF
            return true;
        }
    }
    H3R_ENSURE_FFD(42^42, "Bug: You're not handling some use-case")
    return false;
}// FFD::SNode::ParseStruct()

// It isn't "parse" per se, but. "j" - base (where it starts)
bool FFD::SNode::ParseCompositeField(const byte * buf, int, int i, int j)
{
    Composite = true;
    printf ("Field: composite: "); printf_range (buf, j, i); printf (EOL);
    Name = static_cast<String &&>(String {buf+j, i-j});
    return true;
}

bool FFD::SNode::ParseField(const byte * buf, int len, int & i)
{
    //DONE shouldn't I enforce it? I should.
    skip_line_whitespace (buf, len, i);
    // Either a symbol or a comment
    while (is_comment (buf, len, i)) { // multi-one-line comments
        skip_comment_whitespace_sequence (buf, len, i);
        H3R_ENSURE_FFD(i < len, "Incomplete field")
        skip_line_whitespace (buf, len, i);
    }
    // What is it? "type<>type[] symbol" or "typeEOL" or "type " or "..."
    int j = i;
    for (;; i++) {
        if ('<' == buf[i]) {// hash field
            H3R_ENSURE_FFD(i < len, "wrong hash field") // foo<EOF
            i++;
            H3R_ENSURE_FFD(i < len && '>' == buf[i], "wrong hash field")
            printf ("Field: implicit node \"0hash key\" of type: ");
                printf_range (buf, j, i-1); printf (EOL);
            i++;
            H3R_ENSURE_FFD(i < len, "wrong hash field") // foo<>EOF
            // Type - array Type; look for a field of said array type
            j = i;
            read_symbol (buf, len, i, '[');
            H3R_ENSURE_FFD(i < len, "wrong hash field") // foo<>bar[EOF
            i++;
            H3R_ENSURE_FFD(i < len && ']' == buf[i], "wrong hash field")
            i++;
            H3R_ENSURE_FFD(i < len, "wrong hash field") // foo<>bar[]EOF
            printf ("Field: hash type: ");
                printf_range (buf, j, i); printf (EOL);
            // Name - required; hash fields can't be nameless
            skip_line_whitespace (buf, len, i);
            j = i;
            read_symbol (buf, len, i);
            printf ("Field: <> name "); printf_range (buf, j, i); printf (EOL);
            Name = static_cast<String &&>(String {buf+j, i-j});
            break;
        }
        else if ('.' == buf[i]) { // variadic
            Variadic = true;
            H3R_ENSURE_FFD(i < len-2, "incomplete variadic field")
            H3R_ENSURE_FFD('.' == buf[i+1], "incomplete variadic field")
            H3R_ENSURE_FFD('.' == buf[i+2], "incomplete variadic field")
            i+=3;
            H3R_ENSURE_FFD(i < len, "wrong variadic field") // ...EOF
            skip_line_whitespace (buf, len, i);
            j = i;
            read_symbol (buf, len, i, '\0', true);
            printf ("Field: variadic: ");
                printf_range (buf, j, i); printf (EOL);
            Name = static_cast<String &&>(String {buf+j, i-j});
            break;
        }
        else if (is_eol(buf, len, i)) // compositeEOL
            { ParseCompositeField (buf, len, i, j); break; }
        else if (is_line_whitespace (buf[i])) {// type or composite type
            int p = i;
            skip_line_whitespace (buf, len, i);
            if (is_comment (buf, len, i) || buf[i] == '(' ) // composite
                { ParseCompositeField (buf, len, i=p, j); break; }
            i = p;
            printf ("Field: type: "); printf_range (buf, j, i); printf (EOL);
            DTypeName = static_cast<String &&>(String {buf+j, i-j});
            skip_line_whitespace (buf, len, i);
            j = i;
            read_symbol (buf, len, i, '[');
            if ('[' == buf[i]) { //TODO array: where to store dimensions?
                i++;
                H3R_ENSURE_FFD(i < len, "incomplete array") // [EOF
                H3R_ENSURE_FFD(
                    ! is_eol (buf, len, i), "incomplete array") // [EOL
                Array = true;
                for (int arr = 1; arr < 4; arr++) {
                    int s = i;
                    read_while ("arr     ", buf, len, i,
                        [&](){return ']' != buf[i];});
                    H3R_ENSURE_FFD(']' == buf[i], "incomplete array")
                    H3R_ENSURE_FFD(i-s <= FFD_MAX_ARR_EXPR_LEN,
                        "Simplify your array expression")
                    i++;
                    if (i >= len) break; // foo[.*]EOF
                    if (buf[i] != '[') break;
                    else {
                        i++;
                        H3R_ENSURE_FFD(arr != 3, "array: too many dimensions")
                    }
                    H3R_ENSURE_FFD(i < len, "incomplete array") // [EOF
                }
            }// array
            printf ("Field: name: "); printf_range (buf, j, i); printf (EOL);
            Name = static_cast<String &&>(String {buf+j, i-j});
            break;
        }// (is_line_whitespace (buf[i]))
    }// (;; i++)
    // Read optional: expression, etc.
    if (i >= len) return true;
    if (is_eol (buf, len, i)) { skip_eol (buf, len, i) ; return true; }
    skip_line_whitespace (buf, len, i);
    if ('(' == buf[i]) {
        j = i;
        read_expression (buf, len, i);
        printf ("Field: expr: "); printf_range (buf, j, i); printf (EOL);
        Expr = static_cast<String &&>(String {buf+j, i-j});
    }
    skip_comment_whitespace_sequence (buf, len, i);
    return true;
}// FFD::SNode::ParseField()

bool FFD::SNode::ParseConst(const byte * buf, int len, int & i)
{
    // const {symbol} {literal} [{expr}]
    skip_line_whitespace (buf, len, i);
    int j = i;
    read_symbol (buf, len, i);
    printf ("Const: name: "); printf_range (buf, j, i); printf (EOL);
    Name = static_cast<String &&>(String {buf+j, i-j});
    skip_line_whitespace (buf, len, i);
    // int or string
    if ('"' == buf[i]) {
        j = i+1;
        H3R_ENSURE_FFD(j < len, "incomplete string literal") // "EOF
        read_expression (buf, len, i, '"', '"'); // yep, it shall read "a"a"a"
        Const = FFD::SConstType::Text;
        StringLiteral = static_cast<String &&>(String {buf+j, i-j});
        printf ("Const: string: %s" EOL, StringLiteral.AsZStr ());
    }
    else {
        Const = FFD::SConstType::Int;
        IntLiteral = parse_int_literal (buf, len, i);
        printf ("Const: integer: %d" EOL, IntLiteral);
    }
    if (is_eol (buf, len, i)) { skip_eol (buf, len, i); return true; }
    skip_line_whitespace (buf, len, i);
    if ('(' == buf[i]) {
        j = i;
        read_expression (buf, len, i);
        printf ("Const: expr: "); printf_range (buf, j, i); printf (EOL);
        Expr = static_cast<String &&>(String {buf+j, i-j});
    }
    else
        skip_comment_whitespace_sequence (buf, len, i);
    return true;
}// FFD::SNode::ParseConst()

bool FFD::SNode::ParseEnum(const byte * buf, int len, int & i)
{
    // enum {symbol} {machine type} [{expr}]
    //   {symbol} {int literal} [{expr}]
    skip_line_whitespace (buf, len, i);
    int j = i;
    read_symbol (buf, len, i);
    printf ("Enum: name: "); printf_range (buf, j, i); printf (EOL);
    Name = static_cast<String &&>(String {buf+j, i-j});
    skip_line_whitespace (buf, len, i);
    j = i;
    read_symbol (buf, len, i);
    printf ("Enum: type: "); printf_range (buf, j, i); printf (EOL);
    DTypeName = static_cast<String &&>(String {buf+j, i-j});
    if (is_eol (buf, len, i))
        skip_eol (buf, len, i);
    else {
        if ('(' == buf[i]) {
            j = i;
            read_expression (buf, len, i);
            printf ("Enum: expr: "); printf_range (buf, j, i); printf (EOL);
            Expr = static_cast<String &&>(String {buf+j, i-j});
        }
        else {
            H3R_ENSURE_FFD(i < len, "incomplete enum")      // enum foo.*EOF
            skip_comment_whitespace_sequence (buf, len, i);
        }
    }
    H3R_ENSURE_FFD(i < len, "incomplete enum")      // enum foo.*EOLEOF
    H3R_ENSURE_FFD(! is_eol (buf, len, i), "Empty enum") // enum foo.*EOLEOL
    for (int chk = 0; ; chk++) {
        H3R_ENSURE_FFD(chk < FFD_MAX_ENUM_ITEMS, "Refine your design")
        // enum item; white-space mandatory;
        //   {symbol} {int literal} [{expr}]
        //TODO where will these go
        skip_line_whitespace (buf, len, i);
        j = i;
        read_symbol (buf, len, i);
        printf ("EnumItem: Name: "); printf_range (buf, j, i); printf (EOL);
        skip_line_whitespace (buf, len, i);
        int foo = parse_int_literal (buf, len, i);
        printf ("EnumItem: Value: %d" EOL, foo);
        if (is_eol (buf, len, i))
            skip_eol (buf, len, i);
        else {
            if ('(' == buf[i]) {
                j = i;
                read_expression (buf, len, i);
                printf ("EnumItem: expr: ");
                    printf_range (buf, j, i); printf (EOL);
                //TODO EnumItem.Expr
            }
            else
                skip_comment_whitespace_sequence (buf, len, i);
        }
        //
        if (i >= len) return true; // itemEOF
        if (is_eol (buf, len, i)) {
            skip_eol (buf, len, i);
            if (i >= len) return true; // itemEOLEOF
            return true;
        }
    }
    H3R_ENSURE_FFD(42^42, "Bug: You're not handling some use-case")
    return false;
}// FFD::SNode::ParseEnum()

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
