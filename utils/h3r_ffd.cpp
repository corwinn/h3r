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

// All a have is a war I can not win, but I can never stop fighting.

#include "h3r_ffd.h"
#include "h3r_filestream.h"
#include "h3r_memorystream.h"
#include "h3r_os.h"
#include "h3r_dbg.h"
#include "h3r_log.h"
#include "h3r_zipinflatestream.h"
#include "h3r_ffdnode.h"

#include <new>

H3R_NAMESPACE

// 1000 classes? No thanks.
template <typename F> struct KwParser final // Keyword Parser
{
    const int KwLen; // Keyword Length - no need to compute it
    const char * Kw; // Keyword
    F Parse;         // parse function
};

bool FFD::SNode::Parse(FFDParser & parser)
{
    static int const KW_LEN {9};
    static KwParser<bool (FFD::SNode::*)(FFDParser &)>
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

    int j = parser.Tell ();
    // Attribute "correction"
    // Dbg << "column: " << parser.Column () << ", buf[j]:"
    //     << Dbg.Fmt (%2X,*(parser.BufAt (j))) << EOL;
    while (1 == parser.Column () && parser.AtAttributeStart ())
        return ParseAttribute (parser);
    //
    parser.ReadNonWhiteSpace ();
    int kw_len = parser.Tell () - j;
    for (auto & kwp : KW)
        if (kwp.KwLen == kw_len && ! OS::Strncmp (
            kwp.Kw, parser.BufAt (j), kw_len)) {
            H3R_ENSURE_FFD(parser.HasMoreData (), "Incomplete node") // .*EOF
            H3R_ENSURE_FFD(! parser.IsEol (), "Incomplete node") // .*EOL
            if ((this->*kwp.Parse) (parser)) {
                if (6 == kwp.KwLen && ! OS::Strncmp (kwp.Kw, "format", 6))
                    Type = FFD::SType::Format;
                return true;
            }
            else
                return false;
        }
    return false;
}// FFD::SNode::Parse()

// type {symbol} {literal}|{symbol} [{expr}]
//     i
// Single line. An EOL completes it.
bool FFD::SNode::ParseMachType(FFDParser & parser)
{
    Type = FFD::SType::MachType;

    parser.SkipLineWhitespace ();
    // name
    Name = static_cast<String &&>(parser.ReadSymbol ());
    Dbg << "MachType: Symbol: " << Name << EOL;
    parser.SkipLineWhitespace ();
    // size or alias
    if (parser.SymbolValid1st ()) { // alias
        String alias = static_cast<String &&>(parser.ReadSymbol ());
        Dbg << "MachType: Alias: " << alias << EOL;
        auto an = NodeByName (alias);
        H3R_ENSURE_FFD(an != nullptr, "The alias must exist prior whats "
            "referencing it. I know you want infinite loops; plenty elsewhere.")
        Size = an->Size;
        Signed = an->Signed;
    }
    else {// size
        Size = parser.ParseIntLiteral ();
        if (Size < 0) { Signed = true; Size = -Size; }
        Dbg << "MachType: " << Size << " bytes, "
            << (Signed ? "signed" : "unsigned") << EOL;
    }
    if (parser.IsEol ()) return true; // completed
    // There could be an expression
    parser.SkipLineWhitespace ();
    if (parser.AtExprStart ())
        Expr = static_cast<List<FFDParser::ExprToken> &&>(
            parser.TokenizeExpression ());
    // There could be a comment. But it is handled elsewhere for now.
    return true;
}// FFD::SNode::ParseMachType()

// Be careful: leaves i pointing to '\n', or '\r' in the '\r\n' case.
// {symbol} .*EOLEOL
//         i
bool FFD::SNode::ParseLater(FFDParser & parser)
{
    parser.SkipUntilDoubleEol ();
    return true;
}

// ^[.*]
//  i
// Single line. An EOL completes it.
bool FFD::SNode::ParseAttribute(FFDParser & parser)
{
    Type = FFD::SType::Attribute;

//np ReadExpression (open: '[', close: ']');
    Attribute = static_cast<String &&>(parser.ReadExpression ('[', ']'));
    parser.SkipCommentWhitespaceSequence ();
    Dbg << "Attribute: " << Attribute << EOL;
    return true;
}

FFD::SNode::~SNode()
{
    for (int i = 0; i < Fields.Count (); i++)
        H3R_DESTROY_NESTED_OBJECT(Fields[i], FFD::SNode, SNode)
}

// struct {symbol}|{variadic-list-item:{value-list}}
//       i
// Multi-line. An EOLEOL or EOF, after at least 1 field, completes it.
// No expression.
bool FFD::SNode::ParseStruct(FFDParser & parser)
{
    Type = FFD::SType::Struct;

    parser.SkipLineWhitespace ();
//np ReadSymbol (stop_at: ':', allow_dot: true);
    Name = static_cast<String &&>(parser.ReadSymbol (':', true));
    Dbg << "Struct: " << Name << EOL;
    if (parser.AtVListSep ()) {
        Dbg << "Struct: variadic list item" << EOL;
        VListItem = true;
        parser.SkipOneByte ();
        H3R_ENSURE_FFD(parser.HasMoreData (), "Incomplete value-list")
        ValueList = static_cast<String &&>(parser.ReadValueList ());
        Dbg << "Struct: ValueList: " << ValueList << EOL;
    }
    // skip remaining white-space(s) and comment(s)
    H3R_ENSURE_FFD(parser.HasMoreData (), "Incomplete struct")// struct .*EOF
    parser.SkipCommentWhitespaceSequence ();
    H3R_ENSURE_FFD(parser.HasMoreData (), "Incomplete struct")// struct .*EOLEOF
    H3R_ENSURE_FFD(! parser.IsEol (), "Empty struct") // struct foo.*EOLEOL
    // fields
    for (int chk = 0; ; chk++) {
        H3R_ENSURE_FFD(chk < FFD_MAX_FIELDS, "Refine your design")
        SNode * node;
        H3R_CREATE_OBJECT(node, FFD::SNode) {};
        node->Base = this;
        Fields.Add (node);
        if (! node->ParseField (parser)) // it skips its EOL if any
            return false;
        // if (parser.HasMoreData ())
        //    Dbg << "done reading a field. " << parser.Tell () << ":"
        //        << Dbg.Fmt (%002X, *(parser.BufAt (parser.Tell ()))) << EOL;
        if (! parser.HasMoreData ()) return true; // fieldEOF
        if (parser.IsEol ()) {
            parser.SkipEol ();
            if (! parser.HasMoreData ()) return true; // fieldEOLEOF
            return true;
        }
    }
    return false;
}// FFD::SNode::ParseStruct()

// {whitespace} {symbol} [{expr}]
// "j" - base (where it starts)
bool FFD::SNode::ParseCompositeField(FFDParser & parser, int j)
{
    Composite = true;
    DTypeName = static_cast<String &&>(parser.StringAt (j, parser.Tell ()-j));
    Name = "{composite}";
    Dbg << "Field: composite. DTypeName: " << DTypeName << EOL;
    if (parser.IsEol ()) return true;
    parser.SkipLineWhitespace ();
    if (parser.AtExprStart ())
        Expr = static_cast<List<FFDParser::ExprToken> &&>(
            parser.TokenizeExpression ());
    // comment(s) and whitespace are handled by FFD::SNode::ParseField()
    return true;
}

// {whitespace} {symbol} [{symbol}] [{expr}]
// {whitespace} {symbol}<>{symbol}[] {symbol}
// {whitespace} ... {variadic-list-name}
// i
bool FFD::SNode::ParseField(FFDParser & parser)
{
    Type = FFD::SType::Field;

    parser.SkipLineWhitespace ();
    // Either a symbol or a comment
    while (parser.IsComment ()) { // multi-one-line comments
        parser.SkipCommentWhitespaceSequence ();
        H3R_ENSURE_FFD(parser.HasMoreData (), "Incomplete field")
        parser.SkipLineWhitespace ();
    }
    // What is it? "type<>type[] symbol" or "typeEOL" or "type " or "..."
    int j = parser.Tell ();
    for (;; parser.SkipOneByte ()) {
        if (parser.AtHashStart ()) {// hash field
            HashKey = true; // this field is hash key: the actual value is:
                            // HashType[value]
            H3R_ENSURE_FFD(parser.HasMoreData (), "wrong hash field") // foo<EOF
            parser.SkipOneByte ();
            H3R_ENSURE_FFD(parser.HasMoreData () && parser.AtHashEnd (),
                "wrong hash field")
            // resolve: pass 1
            String hash_key_type = static_cast<String &&>(
                parser.StringAt (j, parser.Tell ()-1-j));
            Dbg << "Field: hash. Key type: " << hash_key_type << EOL;
            DType = Base->NodeByName (hash_key_type);
            parser.SkipOneByte (); // move after "<>"
            H3R_ENSURE_FFD(parser.HasMoreData (), "wrong hash field") // .*<>EOF
            // Type - array Type; look for a field of said array type.
            //LATER why matching by type only?
            HashType = static_cast<String &&>(parser.ReadSymbol ('['));
            H3R_ENSURE_FFD(parser.HasMoreData (), "wrong hash field") // .*[EOF
            parser.SkipOneByte ();
            H3R_ENSURE_FFD(parser.HasMoreData () && parser.AtArrEnd (),
                "wrong hash field")
            parser.SkipOneByte ();
            H3R_ENSURE_FFD(parser.HasMoreData (), "wrong hash field") // .*[]EOF
            Dbg << "Field: hash. HashType: " << HashType << EOL;
            // Name - required; hash fields can't be nameless
            parser.SkipLineWhitespace ();
            Name = static_cast<String &&>(parser.ReadSymbol ());
            Dbg << "Field: hash. Name: " << Name << EOL;
            break;
        }
        else if (parser.AtVariadicStart ()) { // variadic
            Variadic = true;
            parser.ReadVariadicField (); // "..."
            H3R_ENSURE_FFD(parser.HasMoreData (), "Wrong variadic field") //.EOF
            parser.SkipLineWhitespace ();
            Name = static_cast<String &&>(parser.ReadSymbol ('\0', true));
            Dbg << "Field: variadic. Name: " << Name << EOL;
            break;
        }
        else if (parser.IsEol ()) // compositeEOL
            { ParseCompositeField (parser, j); break; }
        else if (parser.IsLineWhitespace ()) {// type or composite type
            int p = parser.Tell (); // look-ahead
            parser.SkipLineWhitespace ();
            if (parser.IsComment () || parser.AtExprStart ()) // composite
                { ParseCompositeField (parser.SetCurrent (p), j); break; }
            parser.SetCurrent (p);
            DTypeName =
                static_cast<String &&>(parser.StringAt (j, parser.Tell ()-j));
            Dbg << "Field: type: " << DTypeName << EOL;
            DType = Base->NodeByName (DTypeName); // resolve: pass 1
            parser.SkipLineWhitespace ();
            Name = static_cast<String &&>(parser.ReadSymbol ('['));
            if (parser.AtArrStart ()) {
                parser.SkipOneByte ();
                H3R_ENSURE_FFD(parser.HasMoreData (), "Incomplete array")// [EOF
                H3R_ENSURE_FFD(! parser.IsEol (), "Incomplete array") // [EOL
                Array = true;
                for (int arr = 0; arr < 3; arr++) {
                    if (parser.SymbolValid1st ())
                        Arr[arr].Name =
                            static_cast<String &&>(parser.ReadArrDim ());
                    else { // int literal; allow line whitespace around it
                        if (parser.IsLineWhitespace ()) // trim start
                            parser.SkipLineWhitespace ();
                        Arr[arr].Value = parser.ParseIntLiteral ();
                        if (parser.IsLineWhitespace ()) // trim end
                            parser.SkipLineWhitespace ();
                        H3R_ENSURE(parser.AtArrEnd (), "wrong arr dim")
                        parser.SkipOneByte (); // ']'.Next()
                    }
                    Dbg << "Field: array[" << arr << "]= ";
                    Arr[arr].DbgPrint (); Dbg << EOL;
                    if (! parser.HasMoreData ()) break; // foo[.*]EOF
                    if (! parser.AtArrStart ()) break;
                    else {
                        parser.SkipOneByte ();
                        H3R_ENSURE_FFD(arr != 2, "array: too many dimensions")
                    }
                    H3R_ENSURE_FFD(parser.HasMoreData (),
                        "incomplete array") // [EOF
                }
            }// array
            Dbg << "Field: name: " << Name << EOL;
            break;
        }// (parser.IsLineWhitespace ())
    }// (;; i++)
    // Read optional: expression, etc.
    if (! parser.HasMoreData ()) return true;
    if (parser.IsEol ()) { parser.SkipEol () ; return true; }
    parser.SkipLineWhitespace ();
    if (parser.AtExprStart ())
        Expr = static_cast<List<FFDParser::ExprToken> &&>(
            parser.TokenizeExpression ());
    parser.SkipCommentWhitespaceSequence ();
    return true;
}// FFD::SNode::ParseField()

// const {symbol} {literal} [{expr}]
//      i
bool FFD::SNode::ParseConst(FFDParser & parser)
{
    Type = FFD::SType::Const;

    parser.SkipLineWhitespace ();
    Name = static_cast<String &&>(parser.ReadSymbol ());
    Dbg << "Const: name: " << Name << EOL;
    parser.SkipLineWhitespace ();
    // int or string
    if (parser.AtDoubleQuote ()) {
        Const = FFD::SConstType::Text;
        StringLiteral = static_cast<String &&>(parser.ReadStringLiteral ());
        Dbg << "Const: string: " << StringLiteral << EOL;
    }
    else {
        Const = FFD::SConstType::Int;
        IntLiteral = parser.ParseIntLiteral ();
        Dbg << "Const: integer: " << IntLiteral << EOL;
    }
    if (parser.IsEol ()) { parser.SkipEol (); return true; }
    parser.SkipLineWhitespace ();
    if (parser.AtExprStart ())
        Expr = static_cast<List<FFDParser::ExprToken> &&>(
            parser.TokenizeExpression ());
    else
        parser.SkipCommentWhitespaceSequence ();
    return true;
}// FFD::SNode::ParseConst()

// enum {symbol} {machine type} [{expr}]
//     i
//   {whitespace} {symbol} {int literal} [{expr}]
bool FFD::SNode::ParseEnum(FFDParser & parser)
{
    Type = FFD::SType::Enum;

    parser.SkipLineWhitespace ();
    Name = static_cast<String &&>(parser.ReadSymbol ());
    Dbg << "Enum: name: " << Name << EOL;
    parser.SkipLineWhitespace ();
    DTypeName = static_cast<String &&>(parser.ReadSymbol ());
    Dbg << "Enum: type: " << DTypeName << EOL;
    DType = NodeByName (DTypeName); // resolve: pass 1
    H3R_ENSURE_FFD(nullptr != DType, "Enum shall resolve to a machine type")
    Size = DType->Size;
    Signed = DType->Signed;
    if (parser.IsEol ())
        parser.SkipEol ();
    else {
        if (parser.AtExprStart ())
            Expr = static_cast<List<FFDParser::ExprToken> &&>(
                parser.TokenizeExpression ());
        H3R_ENSURE_FFD(parser.HasMoreData (), "Incomplete enum") // foo.*)EOF
        parser.SkipCommentWhitespaceSequence ();
    }
    H3R_ENSURE_FFD(parser.HasMoreData (), "Incomplete enum") // enum foo.*EOLEOF
    H3R_ENSURE_FFD(! parser.IsEol (), "Empty enum") // enum foo.*EOLEOL
    for (int chk = 0; ; chk++) {
        H3R_ENSURE_FFD(chk < FFD_MAX_ENUM_ITEMS, "Refine your design")
        // {whitespace} {symbol} {int literal} [{expr}]
        // i
        parser.SkipLineWhitespace ();
        EnumItem itm;
        itm.Name = static_cast<String &&>(parser.ReadSymbol ());
        Dbg << "EnumItem: Name: " << itm.Name << EOL;
        parser.SkipLineWhitespace ();
        itm.Value = parser.ParseIntLiteral ();
        Dbg << "EnumItem: Value: " << itm.Value << EOL;
        if (parser.IsEol ())
            parser.SkipEol ();
        else {
            if (parser.AtExprStart ())
                itm.Expr = static_cast<List<FFDParser::ExprToken> &&>(
                    parser.TokenizeExpression ());
            else
                parser.SkipCommentWhitespaceSequence ();
        }
        EnumItems.Add (itm);
        //
        if (! parser.HasMoreData ()) return true; // itemEOF
        if (parser.IsEol ()) {
            parser.SkipEol ();
            if (! parser.HasMoreData ()) return true; // itemEOLEOF
            return true;
        }
    }
    return false;
}// FFD::SNode::ParseEnum()

FFD::FFD() {}

FFD::~FFD()
{
    while (_tail) {
        auto dnode = _tail;
        _tail = _tail->Prev;
        H3R_DESTROY_NESTED_OBJECT(dnode, FFD::SNode, SNode)
    }
    H3R_ENSURE(nullptr == _tail, "bug: something like an LL")
    _head = _tail;
}

//LATER use h3r_resnamehash
FFD::SNode * FFD::SNode::NodeByName(const String & query)
{
    FFD::SNode * result = {};
    WalkBackwards([&](FFD::SNode * node) {
        // Dbg << "ba n: " << node->Name << "vs. " << query << EOL;
        if (node->Name == query && node->Usable ()) {
            result = node;
            return false;
        }
        return true;
    });
    if (nullptr == result && Next)
        Next->WalkForward([&](FFD::SNode * node) {
            // Dbg << "fo n: " << node->Name << "vs. " << query << EOL;
            if (node->Name == query && node->Usable ()) {
                result = node;
                return false;
            }
            return true;
        });
    return result;
}

FFD::EnumItem * FFD::SNode::FindEnumItem(const String & name)
{//TODO enum.Expr > 0
    for (int i = 0; i < EnumItems.Count (); i++)
        if (EnumItems[i].Name == name) return &(EnumItems[i]);
    return nullptr;
}

List<FFD::SNode *> FFD::SNode::NodesByName(const String & query)
{
    List<FFD::SNode *> result = {};
    WalkBackwards([&](FFD::SNode * node) {
        if (node->Name == query) result.Add (node);
        return true;
    });
    if (Next)
        Next->WalkForward([&](FFD::SNode * node) {
            if (node->Name == query) result.Add (node);
            return true;
        });
    return static_cast<List<FFD::SNode *> &&>(result);
}

// Debug purposes
void FFD::SNode::DbgPrint()
{
    Dbg << "+" << TypeToString () << ": ";
    if (HashKey) Dbg << "[hk]";
    if (Array) Dbg << "[arr]";
    if (Variadic) Dbg << "[var]";
    if (VListItem) Dbg << "[vli]";
    if (Composite) Dbg << "[comp]";
    if (Signed) Dbg << "[signed]";
    if (IsAttribute ())
        Dbg << " Value: \"" << Attribute << "\"";
    else
        Dbg << " Name: \"" << Name << "\"";
    Dbg << ", DType: \"";
    if (nullptr == DType) {
        if (! NoDType ())
            Dbg << "unresolved:" << DTypeName;
    }
    else Dbg << DType->Name;
    Dbg << "\"" << EOL;
}
static void print_tree(FFD::SNode * n)
{
    if (nullptr == n) {
        Dbg << "null tree - nothing to print" << EOL;
        return;
    }
    Dbg << "The tree:" << EOL;
    n->WalkForward ([&](FFD::SNode * n) -> bool {
        if (nullptr == n) { Dbg << "+[null]" << EOL; return true; }
        n->DbgPrint ();
        for (auto sn : n->Fields) {
            if (nullptr == sn) { Dbg << "+[null]" << EOL; continue; }
            Dbg << "  "; sn->DbgPrint ();
        }
        return true;
    });
}

void FFD::SNode::ResolveTypes()
{
    for (auto sn : Fields) sn->ResolveTypes ();

    if (DType || NoDType ()) return;
    if (DTypeName.Empty ()) {
        Dbg << "neither dtype nor dtypename: \"" << Name << "\"" << EOL;
        return;
    }
    DType = Base ? Base->NodeByName (DTypeName)
        : NodeByName (DTypeName);
}
static void resolve_all_types(FFD::SNode * n)
{
    n->WalkForward ([&](FFD::SNode * nn){ nn->ResolveTypes (); return true; });
}

/*static*/ FFDNode * FFD::File2Tree(const String & d, const String & f)
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

    Dbg << "TB LR parsing " << len << " bytes ffd" << EOL;
    FFD ffd {};
    FFDParser parser {buf, len};
    Dbg.Enabled = false;
    for (int chk = 0; parser.HasMoreData (); chk++) {
        if (parser.IsWhitespace ()) parser.SkipWhitespace ();
        // Skipped, for now
        else if (parser.IsComment ()) parser.SkipComment ();
        else {
            FFD::SNode * node;
            H3R_CREATE_OBJECT(node, FFD::SNode) {};
            if (nullptr == ffd._tail)
                ffd._tail = ffd._head = node;
            else {
                node->Prev = ffd._tail;
                node->Next = ffd._tail->Next;
                if (ffd._tail->Next) ffd._tail->Next->Prev = node;
                ffd._tail->Next = node;
                ffd._tail = node;
            }
            node->Parse (parser);
            if (node->IsRoot ()) {
                H3R_ENSURE_FFD(nullptr == ffd._root, "Multiple formats in a "
                    "single description aren't supported yet")
                ffd._root = node;
                Dbg.Enabled = true;
                Dbg << "Ready to parse: " << node->Name << EOL;
            }
        }
        H3R_ENSURE_FFD(chk < len, "infinite loop")
    }// (int i = 0; i < len;)

    // 2. Fasten DType - only those with "! Expr.Empty ()" shall remain null -
    //    they're being resolved at "runtime".
    resolve_all_types (ffd._head);
    print_tree (ffd._head);

    // 3. Apply
    OS::FileStream fh2 {f, H3R_NS::OS::FileStream::Mode::ReadOnly};
    Stream * s {&fh2};

    // This code is specific to the map parser. zlibMapStream is the same
    // as the ZipInflateStream, but has specific initializaiton.
    const int H3M_MAX_FILE_SIZE = 1<<21; // 6167 maps: the largest: 375560 bytes
                                         // unpacked: 1342755 bytes
    H3R_ENSURE(fh2.Size () < static_cast<off_t>(H3M_MAX_FILE_SIZE),
        "File too large")
    auto h3m_zstream_attr = ffd._root->GetAttr ("[Stream(type: zlibMapStream)]");
    if (h3m_zstream_attr) {
        int h, usize, size = static_cast<int>(fh2.Size ());
        Stream::Read (fh2, &h);
        if (h != 0x88b1f)
            Dbg << "Unknown map format. Load could fail." << EOL;
        else {
            Stream::Read (fh2.End ().Seek (-4), &usize);
            H3R_ENSURE(usize > size && usize < H3M_MAX_FILE_SIZE,
                "Map too large")
            ZipInflateStream * zstr {};
            bool h3map = true;
            fh2.Begin ();
            H3R_CREATE_OBJECT(zstr, ZipInflateStream) {
                &fh2, size, usize, h3map};
            s = zstr;
        }
    }
    else
        Dbg << "zlibMapStream not found. Load could fail." << EOL;

    FFDNode * data_root {};
    Dbg << "Parsing " << f << EOL;
    H3R_CREATE_OBJECT(data_root, FFDNode) {ffd._root, s};
    if (s != &fh2) H3R_DESTROY_OBJECT(s, Stream)
    return data_root;
}// FFD::File2Tree()

#undef H3R_ENSURE_FFD

NAMESPACE_H3R
