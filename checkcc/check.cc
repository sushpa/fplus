
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//=============================================================================
// STRING
//=============================================================================

// operator char*() { return str; }
char* str_noext(char* str)
{
    char* s = strdup(str);
    const size_t len = strlen(s);
    char* sc = s + len;
    while (sc > s and *sc != '.')
        sc--;
    if (sc >= s) *sc = '\0';
    return s;
}

char* str_base(char* str, char sep = '/')
{
    char* s = str;
    const size_t len = strlen(s);
    char* sc = s + len;
    while (sc > s and sc[-1] != sep)
        sc--;
    if (sc >= s) s = sc;
    return s;
}

char* str_dir(char* str)
{
    char* s = strdup(str);
    const size_t len = strlen(s);
    char* sc = s + len;
    while (sc > s and *sc != '/')
        sc--;
    if (sc >= s) *sc = '\0';
    return s;
}

char* str_upper(char* str)
{
    char* s = strdup(str);
    char* sc = s - 1;
    while (*++sc)
        if (*sc >= 'a' and *sc <= 'z') *sc -= 32;
    return s;
}

void str_tr_ip(char* str, const char oldc, const char newc)
{
    char* sc = str - 1;
    while (*++sc)
        if (*sc == oldc) *sc = newc;
}

char* str_tr(char* str, const char oldc, const char newc)
{
    char* s = strdup(str);
    str_tr_ip(s, oldc, newc);
    return s;
}

// LIST STACK etc.
template <class T> class Pool {
    T* ref;
    uint32_t cap = 0, elementsPerBlock = 512, total = 0;

    public:
    uint32_t count = elementsPerBlock;

    T* alloc()
    {
        if (count >= elementsPerBlock) {
            ref = (T*)calloc(elementsPerBlock, sizeof(T));
            count = 0;
        }
        total++;
        return &ref[count++];
    }
    static const char* _typeName()
    {
        const char* a = "Pool<";
        char* ret = strcat(a, T::_typeName());
        ret = strcat(ret, ">");
        return ret;
    }
    void stat()
    {
        fprintf(stderr, "*** %u (%s) allocated (%ld B)\n", total,
            T::_typeName(), total * sizeof(T));
    }
};

template <class T> class Stack {
    T* items = NULL;
    uint32_t cap = 0;

    public:
    uint32_t count = 0;
    //    uint32_t count() { return count; }
    T operator[](int index) { return items[index]; }
    void push(T node)
    {
        //        assert(self != NULL);
        assert(node != NULL); // really?
        if (count < cap) {
            items[count++] = node;
        } else {
            cap = cap ? 2 * cap : 8;
            items = (T*)realloc(items, sizeof(T) * cap);
            // !! realloc can NULL the ptr!
            items[count++] = node;
            for (int i = count; i < cap; i++)
                items[i] = NULL;
        }
    }

    T pop()
    {
        T ret; //= NULL;
        if (count) {
            ret = items[count - 1];
            items[count - 1] = NULL;
            count--;
        } else {
            printf("error: pop from empty list\n");
        }
        return ret; // stack->count ? stack->items[--stack->count] : NULL;
    }

    T top() { return count ? items[count - 1] : NULL; }

    bool empty() { return count == 0; }
};

template <class T> class List {
    public:
    static Pool<List<T>> pool;
    void* operator new(size_t size) { return pool.alloc(); }

    T item=NULL;

    List<T>* next = NULL;
    //    operator T() { return item; } // not a good idea for readability
    // send the *pointer* by reference. if list is NULL, then 'append'ing
    // something puts it at index 0

    List<T>() {}
    List<T>(T item) { this->item = item; }

    void append(T item) // List<T>* listItem)
    { // adds a single item, creating a wrapping list item holder.
        if (!this->item)
            this->item = item;
        else
            append(List<T>(item));
        //        auto listItem = new List<T>(item);
        // //        listItem->item = item;
        //        List<T>* last = this;
        //        while (last->next)
        //            last = last->next;
        //        last->next = listItem;
    }

    void append(List<T> listItem)
    {
        // adds a list item, including its next pointer, effectively
        // concatenating this with listItem
        auto li = new List<T>;
        *li = listItem;
        List<T>* last = this;
        while (last->next)
            last = last->next;
        last->next = li;
    }
};
template <class T> Pool<List<T>> List<T>::pool;

#define foreach(var, listp, listSrc)                                       \
    /*auto listp = &(listSrc); / *if (listp) */                            \
    auto listp = &(listSrc);                                               \
    for (auto var = listp->item; listp && (var = listp->item);             \
         listp = listp->next)

// Stack<uint64_t> pool8B;
// Stack<uint64_t[2]> pool16B;

// void* poolptrs[2] = {&pool8B, &pool16B};
//=============================================================================
// TOKEN
//=============================================================================

// Various kinds of tokens.
enum TokenKind {
    //    TK_source,
    TKNullChar,
    TKKeyword_cheater,
    TKKeyword_for,
    TKKeyword_while,
    TKKeyword_if,
    TKKeyword_end,
    TKKeyword_function,
    TKKeyword_test,
    TKKeyword_not,
    TKKeyword_and,
    TKKeyword_or,
    TKKeyword_in,
    TKKeyword_do,
    TKKeyword_then,
    TKKeyword_as,
    TKKeyword_else,
    TKKeyword_type,
    TKKeyword_print,
    TKKeyword_return,
    TKKeyword_base,
    TKKeyword_var,
    TKKeyword_let,
    TKKeyword_import,
    TKKeyword_check,
    TKIdentifier,
    TKFunctionCall,
    TKSubscript,
    TKNumber,
    //    TKWs,
    TKSpaces,
    TKOneSpace, // exactly 1 space
    TKTab,
    TKNewline,
    TKLineComment,
    TKAlphabet,
    TKAmpersand,
    TKArrayClose, // ]
    // TKArray_empty, // []
    TKArrayOpen, //  [
    TKArrayDims, // [:,:,:]
    TKAt,
    TKBraceClose,
    //    TKBrace_empty,
    TKBraceOpen,
    TKDigit,
    TKHash,
    TKExclamation,
    TKPipe,
    TKOpAssign,
    TKOpEQ,
    TKOpNE,
    TKOpGE,
    TKOpGT,
    TKOpLE,
    //    TKOpLeftShift,
    TKOpLT,
    TKOpMod,
    //    TKOpRightShift,
    TKOpResults,
    TKOpNotResults,
    TKParenClose,
    //    TKParen_empty,
    TKParenOpen,
    TKPeriod,
    TKComma,
    TKOpColon,
    TKStringBoundary, // "
    //    TKStr_empty, // ""
    TKString, // "string"
    TKRegexBoundary, // '
    //    TKRgx_empty, // ''
    TKRegex, // '[a-zA-Z0-9]+'
    TKInlineBoundary, // `
    //    TKInl_empty,
    TKInline, // `something`
    TKUnderscore,
    TKSlash,
    TKBackslash,
    TKPlus,
    TKMinus,
    TKTimes,
    TKPower,
    TKDollar,
    TKUnits,
    TKUnknown,
    TKPlusEq,
    TKMinusEq,
    TKSlashEq,
    TKTimesEq,
    TKColEq,
    TKQuestion
};

// Return the repr of a token kind (for debug)
const char* TokenKind_repr(const TokenKind kind)
{
    switch (kind) {
        //    case TK_source:
        //        return "(src)";
    case TKNullChar:
        return "EOF";
    case TKKeyword_cheater:
        return "cheater";
    case TKKeyword_for:
        return "for";
    case TKKeyword_while:
        return "while";
    case TKKeyword_if:
        return "if";
    case TKKeyword_then:
        return "then";
    case TKKeyword_as:
        return "as";
    case TKKeyword_end:
        return "end";
    case TKKeyword_function:
        return "func";
    case TKKeyword_test:
        return "test";
    case TKKeyword_not:
        return "not";
    case TKKeyword_and:
        return "and";
    case TKKeyword_or:
        return "or";
    case TKKeyword_in:
        return "in";
    case TKKeyword_else:
        return "else";
    case TKKeyword_type:
        return "type";
    case TKKeyword_check:
        return "check";
    case TKKeyword_base:
        return "base";
    case TKKeyword_var:
        return "var";
    case TKKeyword_let:
        return "let";
    case TKKeyword_import:
        return "import";
    case TKKeyword_print:
        return "print";
    case TKKeyword_return:
        return "return";
    case TKIdentifier:
        return "id";
    case TKFunctionCall:
        return "f()";
    case TKSubscript:
        return "a[]";
    case TKNumber:
        return "#";
        //    case TKWs:
        //        return "(ws)";
    case TKSpaces:
        return "(spc)";
    case TKTab:
        return "(tab)";
    case TKNewline:
        return "(nl)";
    case TKLineComment:
        return "(cmt)";
    case TKAmpersand:
        return "&";
    case TKDigit:
        return "1";
    case TKPower:
        return "^";
    case TKUnits:
        return "|kg";
    case TKAlphabet:
        return "a";
    case TKArrayClose:
        return "]";
        //    case TKArrayEmpty:
        //        return "[]";
    case TKArrayOpen:
        return "[";
    case TKArrayDims:
        return "[:,:]";
    case TKAt:
        return "@";
    case TKBraceClose:
        return "}";
        //    case TKBrace_empty:
        //        return "{}";
    case TKBraceOpen:
        return "{";
    case TKHash:
        return "#";
    case TKExclamation:
        return "!";
    case TKPipe:
        return "!";
    case TKOpAssign:
        return "=";
    case TKOpEQ:
        return "==";
    case TKOpNE:
        return "=/";
    case TKOpGE:
        return ">=";
    case TKOpGT:
        return ">";
    case TKOpLE:
        return "<";
        //    case TKOp_lsh:
        //        return "<<";
    case TKOpLT:
        return "<";
    case TKOpMod:
        return "%";
        //    case TKOpRightShift:
        //        return ">>";
    case TKOpResults:
        return "=>";
    case TKOpNotResults:
        return "=/>";
    case TKParenClose:
        return ")";
        //    case TKParen_empty:
        //        return "()";
    case TKParenOpen:
        return "(";
    case TKPeriod:
        return ".";
    case TKComma:
        return ",";
    case TKOpColon:
        return ":";
    case TKStringBoundary:
        return "\"";
        //    case TKStringEmpty:
        //        return "\"\"";
    case TKString:
        return "\"..\"";
    case TKRegexBoundary:
        return "'";
        //    case TKRegexEmpty:
        //        return "''";
    case TKRegex:
        return "(rgx)";
    case TKInlineBoundary:
        return "`";
        //    case TKInlineEmpty:
        //        return "``";
    case TKInline:
        return "`..`";
    case TKUnderscore:
        return "_";
    case TKSlash:
        return "/";
    case TKBackslash:
        return "\\";
    case TKPlus:
        return "+";
    case TKMinus:
        return "-";
    case TKTimes:
        return "*";
    case TKDollar:
        return "$";
    case TKUnknown:
        return "(?)";
    case TKKeyword_do:
        return "do";
    case TKColEq:
        return ":=";
    case TKPlusEq:
        return "+=";
    case TKMinusEq:
        return "-=";
    case TKTimesEq:
        return "*=";
    case TKSlashEq:
        return "/=";
    case TKOneSpace:
        return "(sp1)";
    case TKQuestion:
        return "?";
    }
    printf("unknown kind: %d\n", kind);
    return "(!unk)";
}

bool TokenKind_isUnary(TokenKind kind)
{
    return kind == TKKeyword_not; // unary - is handled separately
}

bool TokenKind_isRightAssociative(TokenKind kind)
{
    return kind == TKPeriod or kind == TKPower;
}

uint8_t TokenKind_getPrecedence(TokenKind kind)
{ // if templateStr then precedence of < and > should be 0
    switch (kind) {
    case TKPeriod:
        return 90;
    case TKPipe:
        return 80;
    case TKPower:
        return 70;
    case TKTimes:
    case TKSlash:
        return 60;
    case TKPlus:
    case TKMinus:
        return 50;
    case TKOpColon:
        return 45;
    case TKOpLE:
    case TKOpLT:
    case TKOpGT:
    case TKOpGE:
    case TKKeyword_in:
        // case TKKw_notin:
        return 41;
    case TKOpEQ:
    case TKOpNE:
        return 40;
    case TKKeyword_not:
        return 32;
    case TKKeyword_and:
        return 31;
    case TKKeyword_or:
        return 30;
    case TKOpAssign:
        return 22;
    case TKPlusEq:
    case TKColEq:
    case TKMinusEq:
    case TKTimesEq:
    case TKSlashEq:
        return 20;
    case TKComma:
        return 10;
    case TKKeyword_do:
        return 5;
        //    case TKParenOpen:
        //    case TKParenClose:
        //        return 0;
        //    case TKBraceOpen:
        //    case TKBraceClose:
        //        return 0;
        //    case TKArrayOpen:
        //    case TKArrayClose:
        //        return 0;
    default:
        return 0;
    }
}

TokenKind TokenKind_reverseBracket(TokenKind kind)
{
    switch (kind) {
    case TKArrayOpen:
        return TKArrayClose;
    case TKParenOpen:
        return TKParenClose;
    case TKBraceOpen:
        return TKBraceClose;
    case TKArrayClose:
        return TKArrayOpen;
    case TKBraceClose:
        return TKBraceOpen;
    case TKParenClose:
        return TKParenOpen;
    default:
        printf("unexpected at %s:%d\n", __FILE__, __LINE__);
        return TKUnknown;
    }
}

// Holds information about a syntax token.
class Token {
    public:
    static Pool<Token> pool;
    void* operator new(size_t size) { return pool.alloc(); }
    static const char* _typeName() { return "Token"; }

    char* pos = NULL;
    uint32_t matchlen : 24;
    struct {
        bool //
            skipWhiteSpace : 1, // skip whitespace
            mergeArrayDims : 1, // merge [:,:,:] into one token
            noKeywordDetect : 1, // leave keywords as idents
            strictSpacing : 1; // missing spacing around operators etc. is a
                               // compile error YES YOU HEARD IT RIGHT
                               // but why need this AND skipWhiteSpace?
    } flags = { true, false, false };
    uint16_t line = 1;
    uint8_t col = 1;
    TokenKind kind : 8; //= TKUnknown;

    Token()
        : kind(TKUnknown)
        , matchlen(0)
    {
    }
    const char* repr() { return TokenKind_repr(kind); }
    char* dup() const // const Token* const token)
    {
        return strndup(pos, matchlen);
    }

    // Advance the token position by one char.
    void advance1()
    {
        // this is called after a token has been consumed and the pointer
        // needs to advance. If skipws is set, loop until the next
        // non-space. do {
        pos++;

        // } while (token.skipws and *(token.pos) == ' ');
    }

    // Peek at the char after the current (complete) token
    char peekCharAfter() { return *(pos + matchlen + 1); }

#define Token_compareKeyword(tok)                                          \
    if (sizeof(#tok) - 1 == l and not strncmp(#tok, s, l)) {               \
        kind = TKKeyword_##tok;                                            \
        return;                                                            \
    }

    // Check if an (ident) token matches a keyword and return its type
    // accordingly.
    void tryKeywordMatch()
    {
        if (flags.noKeywordDetect or kind != TKIdentifier) return; // kind;

        const char* s = pos;
        const int l = matchlen;

        Token_compareKeyword(and)
        Token_compareKeyword(cheater)
        Token_compareKeyword(for)
        Token_compareKeyword(do)
        Token_compareKeyword(while)
        Token_compareKeyword(if)
        Token_compareKeyword(then)
        Token_compareKeyword(end)
        Token_compareKeyword(function)
        Token_compareKeyword(test)
        Token_compareKeyword(not)
        Token_compareKeyword(and)
        Token_compareKeyword(or)
        Token_compareKeyword(in)
        Token_compareKeyword(else)
        Token_compareKeyword(type)
        Token_compareKeyword(check)
        Token_compareKeyword(base)
        Token_compareKeyword(var)
        Token_compareKeyword(let)
        Token_compareKeyword(import)
        Token_compareKeyword(return)
        Token_compareKeyword(as)
        Token_compareKeyword(print);
        // return TKIdentifier;
    }

    // Get the type based on the char at the current position.
    inline TokenKind gettype() { return getTypeAtOffset(0); }

    // Get the type based on the char after the current position (don't
    // conflate this with char* Token_peekCharAfter(token))
    inline TokenKind peek_nextchar() { return getTypeAtOffset(1); }
    //    inline TokenKind peek_prevchar() { return getTypeAtOffset(-1); }

    // Get the token kind based only on the char at the current position (or
    // an offset).
    TokenKind getTypeAtOffset(const size_t offset)
    {
        const char c = pos[offset];
        const char cn = c ? pos[1 + offset] : 0;

        switch (c) {
        case 0:
            return TKNullChar;
        case '\n':
            return TKNewline;
        case ' ':
            return TKSpaces;
        case '\t':
            return TKTab;
        case ':':
            return TKOpColon;
        case ',':
            return TKComma;
        case '?':
            return TKQuestion;
        case '"':
            return TKStringBoundary;
        case '`':
            return TKInlineBoundary;
        case '[':
            // switch (cn) {
            // default:
            return TKArrayOpen;
            // }
        case '$':
            return TKDollar;
        case '%':
            return TKOpMod;
        case '.':
            return TKPeriod;
        case '\'':
            return TKRegexBoundary;
        case '&':
            return TKAmpersand;
        case '^':
            return TKPower;
        case '@':
            return TKAt;
        case '#':
            return TKHash;
        case '|':
            return TKPipe;
        case '{':
            // switch (cn) {
            // case '}':
            //     return TKBraceEmpty;
            // default:
            return TKBraceOpen;
            // }
        case '(':
            // switch (cn) {
            // case ')':
            //     return TKParenEmpty;
            // default:
            return TKParenOpen;
            // }
        case ')':
            return TKParenClose;
        case '}':
            return TKBraceClose;
        case ']':
            return TKArrayClose;
        case '<':
            switch (cn) {
            case '=':
                return TKOpLE;
                //        case '<':
                //            return TKOpLeftShift;
            default:
                return TKOpLT;
            }
        case '>':
            switch (cn) {
            case '=':
                return TKOpGE;
                //        case '>':
                //            return TKOp_rsh;
            default:
                return TKOpGT;
            }
        case '=':
            switch (cn) {
            case '=':
                return TKOpEQ;
            case '/':
                return TKOpNE;
            case '>':
                return TKOpResults;
            default:
                return TKOpAssign;
            }
        case '!':
            return TKExclamation;
        case '/':
            return TKSlash;
        case '\\':
            return TKBackslash;
        case '+':
            return TKPlus;
        case '-':
            return TKMinus; // if prev token was +-*/^(<> (and some
                            // others?) then this is unary -
        case '*':
            return TKTimes;
        default:
            if (isalpha(c) or c == '_') {
                return TKAlphabet;
            } else if (isdigit(c)) {
                return TKDigit;
            } else {
                return TKUnknown;
            }
            break;
        }
    }

    // Scans ahead from the current position until the actual end of the
    // token.
    void detect()
    {
        TokenKind tt = gettype();
        TokenKind tt_ret = TKUnknown; // = tt;
        static TokenKind tt_last = TKUnknown;
        TokenKind tmp;
        char* start = pos;
        bool found_e = false, found_dot = false, found_cmt = false;
        uint8_t found_spc = 0;

        switch (tt) {
        case TKStringBoundary:
        case TKInlineBoundary:
        case TKRegexBoundary:
            tmp = tt; // remember which it is exactly

            // Incrementing pos is a side effect of gettype(...)
            while (tt != TKNullChar) {
                // here we want to consume the ending " so we move next
                // before
                advance1();
                tt = gettype();
                if (tt == TKNullChar or tt == tmp) {
                    advance1();
                    break;
                }
                if (tt == TKBackslash)
                    if (peek_nextchar() == tmp) { // why if?
                        advance1();
                    }
            }
            switch (tmp) {
            case TKStringBoundary:
                tt_ret = TKString;
                break;
            case TKInlineBoundary:
                tt_ret = TKInline;
                break;
            case TKRegexBoundary:
                tt_ret = TKRegex;
                break;
            default:
                tt_ret = TKUnknown;
                printf("unreachable %s:%d\n", __FILE__, __LINE__);
            }
            break;

        case TKSpaces:
            //            tt=peek_prevchar();

            if (tt_last == TKOneSpace) // if prev char was a space return
                                       // this as a run of spaces
                while (tt != TKNullChar) {
                    // here we dont want to consume the end char, so break
                    // before
                    tt = peek_nextchar();
                    advance1();
                    if (tt != TKSpaces) break;
                }
            else
                advance1();
            // else its a single space
            tt_ret = TKSpaces;
            break;

        case TKComma:
            while (tt != TKNullChar) {
                tt = peek_nextchar();
                advance1();
                // line number should be incremented for line continuations
                if (tt == TKSpaces) {
                    found_spc++;
                }
                if (tt == TKExclamation) {
                    found_cmt = true;
                }
                if (tt == TKNewline) {
                    line++;
                    col = -found_spc - 1; // account for extra spaces
                                          // after , and for nl itself
                    found_spc = 0;
                }
                if (found_cmt and tt != TKNewline) {
                    found_spc++;
                    continue;
                }
                if (tt != TKSpaces and tt != TKNewline) break;
            }
            tt_ret = TKComma;
            break;

        case TKArrayOpen:
            // mergearraydims should be set only when reading func args
            if (not flags.mergeArrayDims) goto defaultToken;

            while (tt != TKNullChar) {
                tt = peek_nextchar();
                advance1();
                if (tt != TKOpColon and tt != TKComma) break;
            }
            tt = gettype();
            if (tt != TKArrayClose) {
                fprintf(stderr, "expected a ']', found a '%c'. now what?\n",
                    *pos);
            }
            advance1();
            tt_ret = TKArrayDims;
            break;

        case TKAlphabet:
            while (tt != TKNullChar) {
                tt = peek_nextchar();
                advance1();
                if (tt != TKAlphabet //
                    and tt != TKDigit //
                    and tt != TKUnderscore and tt != TKPeriod)
                    break; /// validate in parser not here
            }
            tt_ret = TKIdentifier;
            break;

        case TKExclamation:
            while (tt != TKNullChar) {
                tt = peek_nextchar();
                advance1();
                if (tt == TKNewline) break;
            }
            tt_ret = TKLineComment;
            break;

        case TKPipe:
            while (tt != TKNullChar) {
                tt = peek_nextchar();
                advance1();
                if (tt != TKAlphabet and tt != TKDigit and tt != TKSlash
                    and tt != TKPeriod)
                    break;
            }
            tt_ret = TKUnits;
            break;

        case TKDigit:
            while (tt != TKNullChar) // EOF, basically null char
            {
                tt = peek_nextchar();
                // numbers such as 1234500.00 are allowed
                // very crude, error-checking is parser's job
                //            if (tt==TKDigit //
                //                or tt==TKPlus //
                //                or tt==TKMinus //
                //                or tt==TKPeriod //
                //                or *token.pos == 'e' //
                //                or *token.pos == 'E' //
                //                or *token.pos == 'd'
                //                or *token.pos == 'D')
                advance1();

                if (*pos == 'e' //
                    or *pos == 'E' //
                    or *pos == 'd'
                    or *pos == 'D') { // will all be changed to e btw
                    found_e = true;
                    continue;
                }
                if (found_e) { //}and (*token.pos == '-' or *token.pos ==
                               //'+'))
                               //{
                    found_e = false;
                    continue;
                }
                if (tt == TKPeriod) {
                    found_dot = true;
                    continue;
                }
                //            if (found_dot and tt == TKPeriod) {
                //                fprintf(stderr, "raise error: multiple
                //                dots in number\n"); break;
                //            }
                // lets allow that for IP addresses etc., or just bytes
                if (tt != TKDigit and tt != TKPeriod) break;
            }
            tt_ret = TKNumber;
            break;

            /* case TKNl:
                 token.line++;
                 token.col=1;
                 break; */

        case TKOpEQ:
        case TKOpGE:
        case TKOpLE:
        case TKOpNE:
        case TKOpResults:
        case TKOpNotResults: // this is 3-char, is it not?
        // case TKBraceEmpty:
        case TKBackslash:
            // case TKParenEmpty:
            // two-char tokens
            advance1();
            // don'self break
        default:
        defaultToken:
            tt_ret = tt;
            advance1();
            break;
        }

        matchlen = (uint32_t)(pos - start);
        pos = start; // rewind. but why! then again advance rewind
                     // advance rewind
        kind = tt_ret;
        // keywords have their own token type
        if (kind == TKIdentifier) tryKeywordMatch();
        // exactly one space is TKOnespc, otherwise TKSpc.
        // the compiler needs to check one space frequently in strict mode.
        // FIXME figure it out later
        if (kind == TKSpaces and matchlen == 1) kind = TKOneSpace;

        tt_last = kind;
    }

    // Advances the parser to the next token and skips whitespace if the
    // parser's flag `skipws` is set.
    void advance()
    {
        pos += matchlen;
        col += matchlen;
        matchlen = 0;
        detect();
        if (flags.skipWhiteSpace
            and (kind == TKSpaces
                or (flags.strictSpacing and kind == TKOneSpace)))
            advance();
        if (kind == TKNewline) {
            line++;
            col = 0; // position of the nl itself is 0
        }
    }
};

const char* const spaces = "                              "
                           "                                  ";

struct ASTImport {
    char* importFile;
    char* alias;
    bool isPackage;
    static Pool<ASTImport> pool;
    void* operator new(size_t size) { return pool.alloc(); }
    static const char* _typeName() { return "ASTImport"; }
};

struct ASTUnits {
    static Pool<ASTUnits> pool;
    void* operator new(size_t size) { return pool.alloc(); }
    static const char* _typeName() { return "ASTUnits"; }

    uint8_t powers[7], something;
    double factor, factors[7];
    char* label;
    void gen(int level) { printf("|%s", label); }
};

struct ASTTypeSpec;
struct ASTType;
struct ASTFunc;
struct ASTVar;

struct ASTExpr {
    static Pool<ASTExpr> pool;
    void* operator new(size_t size) { return pool.alloc(); }

    static const char* _typeName() { return "ASTExpr"; }
    struct {
        uint16_t line = 0;
        union {
            struct {
                bool opIsUnary, opIsRightAssociative;
            };
            uint16_t strLength;
        }; // for literals, the string length
        uint8_t opPrecedence;
        uint8_t col = 0;
        TokenKind
            kind : 8; // TokenKind must be updated to add
                      // TKFuncCallResolved TKSubscriptResolved etc. to
                      // indicate a resolved identifier. while you are at
                      // it add TKNumberAsString etc. then instead of
                      // name you will use the corr. object.
    };

    // Expr : left right next
    // Literals: value next
    // FuncCall : value args next

    union {
        ASTExpr *left = NULL, *args, *indexes;
    }; // for Expr, FuncCall and Subscript respectively
    ASTExpr* next = NULL;
    //    union { // when you write the version with specific types, DO NOT
    //    put prec/rassoc/etc in a union with literals.
    // you can put the literal in a union with either left/right. put
    // prec/rassoc&co in basics along with line/col now that 8B of kind will
    // be free (subkind becomes kind).
    union {
        union {
            char* string;
            double real;
            int64_t integer;
            uint64_t uinteger;
        } value; // for terminals
        char* name; // for unresolved functioncall or subscript, or
                    // identifiers
        ASTFunc* func = NULL; // for functioncall
        ASTVar* var; // for array subscript, or to refer to a variable
        ASTExpr* right;
    };
    //    };

    ASTExpr() {}
    ASTExpr(const Token* token)
    {
        kind = token->kind;
        line = token->line;
        col = token->col;

        opPrecedence = TokenKind_getPrecedence(kind);
        if (opPrecedence) {
            opIsRightAssociative = TokenKind_isRightAssociative(kind);
            opIsUnary = TokenKind_isUnary(kind);
        }
        switch (kind) {
        case TKIdentifier:
            strLength = (uint16_t)token->matchlen;
            name = token->dup();
            break;
        case TKString:
        case TKRegex:
        case TKNumber:
            strLength = (uint16_t)token->matchlen;
            value.string = token->dup();
            break;
        default:;
            // what else? errror
        }
        if (kind == TKNumber) {
            // turn all 1.0234[DdE]+01 into 1.0234e+01
            str_tr_ip(value.string, 'd', 'e');
            str_tr_ip(value.string, 'D', 'e');
            str_tr_ip(value.string, 'E', 'e');
        }
    }

    void gen(int level)
    {
        switch (kind) {
        case TKIdentifier:
        case TKNumber:
        case TKString:
            printf("%.*s", strLength, value.string);
            break;
        case TKFunctionCall:
        case TKSubscript:
            // NYI
            break;
        default:
            if (not opPrecedence)
                break; // not an operator, but this should be error if you
                       // reach here
            bool leftBr
                = left->opPrecedence and left->opPrecedence < opPrecedence;
            bool rightBr = right->opPrecedence
                and right->opPrecedence < opPrecedence;
            if (left) {
                if (leftBr) putc('(', stdout);
                left->gen(level + 1);
                if (leftBr) putc(')', stdout);
            }
            printf(" %s ", TokenKind_repr(kind));
            if (right) {
                if (rightBr) putc('(', stdout);
                right->gen(level + 1);
                if (rightBr) putc(')', stdout);
            }
        }
    }
}; // how about if, for, etc. all impl using ASTExpr?

struct ASTTypeSpec {
    static Pool<ASTTypeSpec> pool;
    void* operator new(size_t size) { return pool.alloc(); }
    static const char* _typeName() { return "ASTTypeSpec"; }

    // char* typename; // use name
    ASTType* type = NULL;
    ASTUnits* units = NULL;
    char* name = "";

    uint32_t dims = 0;

    void gen(int level = 0)
    {
        printf("%s", name);
        if (dims) printf("%s", dimsGenStr(dims));
        if (units) units->gen(level);
    }
    //
    // bool dimsvalid(char* dimsstr) {
    //    bool valid = true;
    //    char* str=dimsstr;
    //    valid = valid and (*str++ == '[');
    //    while (*str != ']' and *str != 0) {
    //        valid = valid and (*str++ == ':');
    //        if (*str==',') {
    //            valid = valid and (*str++ == ',');
    //            valid = valid and (*str++ == ' ');
    //        }
    //    }
    //    return valid;
    //}
    int32_t dimsCount(char* dimsstr)
    {
        int32_t count = 0;
        char* str = dimsstr;
        while (*str)
            if (*str++ == ':' or *str == '[') count++;
        return count;
    }

    const char* dimsGenStr(int32_t dims)
    {
        if (dims==0) return "";
        if (dims==1) return "[]";
        int32_t i;
        int32_t sz = 2 + dims + (dims ? (dims - 1) : 0) + 1;
        char* str = (char*)malloc(sz * sizeof(char));
        str[sz * 0] = '[';
        str[sz - 1] = 0;
        for (i = 0; i < dims; i++) {
            str[i * 2 + 1] = ':';
            str[i * 2 + 2] = ',';
        }
        str[sz - 2] = ']';
        return str;
    }
};

struct ASTVar {
    static Pool<ASTVar> pool;
    void* operator new(size_t size) { return pool.alloc(); }
    static const char* _typeName() { return "ASTVar"; }

    ASTTypeSpec* typeSpec = NULL;
    ASTExpr* init = NULL;
    char* name = "";

    struct {
        bool unused : 1, //
            unset : 1, //
            isLet : 1, //
            isVar : 1, //
            isTarget : 1;
        /* x = f(x,y) */ //
    } flags;

    void gen(int level = 0)
    {
        printf("%.*s%s %s",
               level * 4,
               spaces,
               flags.isLet ? "let" : "var",
            name);
        if (typeSpec)
        {printf(": ");
                typeSpec->gen(level + 1);
            }
        else
            printf(": Unknown");
        if (init) {
            printf(" = ");
            init->gen(level + 1);
        }
//        puts("");
    }
};

class ASTNodeRef {
    union {
        struct {
            uint64_t typ : 3, _danger : 45, _unused : 16;
        };
        uintptr_t uiptr;
        void* ptr = NULL;
    };

    public:
    enum NodeKind { NKVar, NKExpr, NKFunc, NKType, NKModule, NKTest };

    ASTNodeRef() {}

    ASTNodeRef(ASTVar* varNode)
    {
        ptr = varNode;
        typ = NKVar;
    }

    ASTNodeRef(ASTExpr* exprNode)
    {
        ptr = exprNode;
        typ = NKExpr;
    }
    inline uintptr_t getptr() { return (uiptr & 0xFFFFFFFFFFFFFFF8); }
    uint8_t kind() { return typ; } // this only takes values 0-7, 3 bits
    //--
    ASTVar* var() { return (ASTVar*)getptr(); }
    ASTExpr* expr() { return (ASTExpr*)getptr(); }
};

void tes()
{
    ASTNodeRef member;
    if (member.kind() == ASTNodeRef::NKVar) {
    }
}

struct ASTType {
    static Pool<ASTType> pool;
    void* operator new(size_t size) { return pool.alloc(); }
    static const char* _typeName() { return "ASTType"; }

    List<ASTVar*> vars; // vars contained in this type
    ASTTypeSpec* super = NULL;
    char* name = NULL;

    List<ASTExpr*>
        checks; // invariants
                // make sure each Expr has null next. If not, split the
                // Expr into two and add one after the other. If you
                // will add automatic checks based on expressions
                // elsewhere, clone them and set null next, because we
                // need next to build this list here.
    List<ASTVar*>
        params; // params of this type (if generic type / template)

    void gen(int level = 0)
    {
        printf("type %s\n", name);
        // gen params
        if (super) {
            printf("    base ");
            super->gen(level);
            puts("");
        } // %s\n", node->type.super);
        //        ASTNode* member = node->type.members;
        //        auto vars = &(this->vars);
        //        if (vars)
        //            for (auto var = vars->item;
        //                 vars;
        //                 vars = vars->next,
        //                 var = vars->item)
        //        if (this->vars.item) {
        //            auto vars = &(this->vars);
        ////            if (vars)
        //                for (auto var = vars->item;
        //                     vars && (var = vars->item);
        //                     vars = vars->next
        //                     )

        foreach (var, vars, this->vars) {
            if (!var) continue;
            var->gen(level + 1);            puts("");

        }
        //        }
        //        if (this->checks.item) {
        foreach (check, checks, this->checks) {
            if (!check) continue;
            check->gen(level + 1);            puts("");

        }
        //        }
        puts("end type\n");
    }
};

// typedef struct {
//    union {
//        ASTExpr* expr;
//        ASTVar* var;
//    };
//    struct ASTStmt* next; // Expr has its own next... so clean this up
//} ASTStmt;

struct ASTScope {
    static Pool<ASTScope> pool;
    void* operator new(size_t size) { return pool.alloc(); }
    static const char* _typeName() { return "ASTScope"; }

    List<ASTExpr*> stmts;
    List<ASTVar*> locals;
    ASTScope* parent = NULL; // fixme: can be type, func, or scope

    void gen(int level)
    {
        // List<ASTExpr*> stmts = this->stmts;
        // ASTExpr* stmt;
        foreach (local, locals, this->locals) {
//            if (!stmt) continue;
            local->gen(level );
            puts("");
        }
        foreach (stmt, stmts, this->stmts) {
//            if (!stmt) continue;
            stmt->gen(level );            puts("");

        } // while((stmts = *(stmts.next)));
    }
};

struct ASTFunc {
    static Pool<ASTFunc> pool;
    void* operator new(size_t size) { return pool.alloc(); }
    static const char* _typeName() { return "ASTFunc"; }

    ASTScope* body;
    List<ASTVar*> args;
    ASTTypeSpec* returnType;
    char* mangledName;
    char* owner; // if method of a type
    char* name;
    struct {
        uint16_t line;
        struct {
            uint16_t prints : 1, //
                throws : 1, //
                recurs : 1, //
                net : 1, //
                gui : 1, //
                file : 1, //
                refl : 1, //
                nodispatch : 1;
        } flags;
        uint8_t col;
    };

    void gen(int level = 0)
    {
        printf("function %s(", name);
        //        List<ASTVar*> args = this->args;
        //        ASTVar* arg = NULL;
        //        do {
        //            if (!(arg = args.item)) continue; // embedded nulls
        //            arg->gen(level);
        //            printf(args.next ? ", " : ")");
        //            args = *(args.next);
        //        } while (args.next);

        //        List<ASTVar*>* args = this->args;
        //        ASTVar* arg = NULL;
        //        do {
        //            if (!(arg = args.item)) continue; // embedded nulls
        //            arg->gen(level);
        //            printf(args.next ? ", " : ")");
        //            args = (args.next);
        //        } while (args.next);

        foreach (arg, args, this->args) {
//            if (!arg) continue;
            arg->gen(level);
            printf(args->next ? ", " : ")");
        }

        if (returnType) {printf(": "); returnType->gen(level);}
        puts("");
        body->gen(level + 1);
        puts("end function\n");
    }
};

struct ASTModule {
    static Pool<ASTModule> pool;
    void* operator new(size_t size) { return pool.alloc(); }
    static const char* _typeName() { return "ASTModule"; }

    List<ASTFunc*> funcs;
    List<ASTType*> types;
    List<ASTVar*> globals;
    List<ASTImport*> imports;
    List<ASTFunc*> tests;
    char* name;
    void gen(int level = 0)
    {
        printf("! module %s\n", name);

        // nope you will mess up the order
        //        List<ASTType*> types = this->types;
        //        ASTType* type;
        //        do
        if (this->types.item) {
            foreach (type, types, this->types) {
                if (!type) continue;
                type->gen(level);
            }
        } // while((types = types.next));

        //        List<ASTFunc*> funcs = this->funcs;
        //        ASTFunc* func;
        //        do
        if (this->funcs.item) {
            foreach (func, funcs, this->funcs) {
                if (!func) continue;
                func->gen(level);
            }
        } // while (    (funcs =funcs.next));
    }
};

static void print_sizes()
{
    printf("ASTImport %lu\n", sizeof(ASTImport));
    printf("ASTUnits %lu\n", sizeof(ASTUnits));
    printf("ASTExpr %lu\n", sizeof(ASTExpr));
    printf("ASTVar %lu\n", sizeof(ASTVar));
    printf("ASTType %lu\n", sizeof(ASTType));
    //    printf("ASTStmt %lu\n", sizeof(ASTStmt));
    printf("ASTScope %lu\n", sizeof(ASTScope));
    printf("ASTTypeSpec %lu\n", sizeof(ASTTypeSpec));
    printf("ASTFunc %lu\n", sizeof(ASTFunc));
    printf("ASTModule %lu\n", sizeof(ASTModule));
}
// POOLS
// 1-way pools based on type (no freeing)
// rename this to node pool

void alloc_stat()
{
    //    ASTExpr::pool.stat();
    ASTImport::pool.stat();
    //    ASTUnits::pool.stat();
    ASTExpr::pool.stat();
    ASTVar::pool.stat();
    ASTType::pool.stat();
    //    ASTStmt::pool.stat();
    ASTScope::pool.stat();
    ASTTypeSpec::pool.stat();
    ASTFunc::pool.stat();
    ASTModule::pool.stat();
}

//=============================================================================
// PARSER CONTEXT
//=============================================================================

// char nullchar=0;

class Parser {
    char* filename = NULL; // mod/submod/xyz/mycode.ch
    char* moduleName = NULL; // mod.submod.xyz.mycode
    char* mangledName = NULL; // mod_submod_xyz_mycode
    char* capsMangledName = NULL; // MOD_SUBMOD_XYZ_MYCODE
    char* basename = NULL; // mycode
    char* dirname = NULL; // mod/submod/xyz
    char *data = NULL, *end = NULL;
    Token token; // current
    List<ASTModule*> modules; // module node of the AST
    Stack<ASTScope*> scopes; // a stack that keeps track of scope nesting

    public:
    static Pool<Parser> pool;
    void* operator new(size_t size) { return pool.alloc(); }
    static const char* _typeName() { return "Parser"; }

#define STR(x) STR_(x)
#define STR_(x) #x

    uint32_t errCount = 0;
    Parser(char* filename, bool skipws = true)
    {
        static const char* _funcSig
            = "%s:%d: Parser::Parser(filename = \"%s\", skipws = %s)";
        // the file and line should be of the call site, not the definition!
        // ie every func must return errcode in check generated source. if
        // err code is unhandled then backtrace. func prints its string?
        // caller needs to supply file,line as arg then. or else caller
        // prints, needs str ptr then. all str ptrs could be made const
        // static globals.

        static const auto FILE_SIZE_MAX = 1 << 24;
        FILE* file = fopen(filename, "r");
        fprintf(stdout, "compiling %s\n", filename);
        char* noext = str_noext(filename);
        fseek(file, 0, SEEK_END);
        const size_t size
            = ftell(file) + 2; // 2 null chars, so we can always lookahead
        if (size < FILE_SIZE_MAX) { // 16MB max
            data = (char*)malloc(size);
            fseek(file, 0, SEEK_SET);
            fread(data, size, 1, file);
            data[size - 1] = 0;
            data[size - 2] = 0;
            this->filename = filename;
            moduleName = str_tr(noext, '/', '.');
            mangledName = str_tr(noext, '/', '_');
            capsMangledName = str_upper(str_tr(noext, '/', '_'));
            basename = str_base(noext);
            dirname = str_dir(noext);
            end = data + size;
            token.pos = data;
            token.flags.skipWhiteSpace = skipws; //
        }

        fclose(file);
        return;

    _printbt:
        fprintf(stderr, _funcSig, __LINE__, filename, skipws);
    }

    //=============================================================================
    // ERROR REPORTING
    //=============================================================================

    static const auto errLimit = 10;
#define fatal(str, ...)                                                    \
    {                                                                      \
        fprintf(stderr, str, __VA_ARGS__);                                 \
        exit(1);                                                           \
    }

    void errorIncrement()
    {
        if (++errCount >= errLimit)
            fatal("too many errors (%d), quitting\n", errLimit);
    }

#define errorExpectedToken(a) errorExpectedToken_(a, __func__)
    void errorExpectedToken_(TokenKind expected, const char* funcName)
    {
        fprintf(stderr, "in %s: ./%s:%d:%d: expected '%s' found '%.*s'\n",
            funcName, filename, token.line, token.col,
            TokenKind_repr(expected), token.matchlen, token.pos);
        errorIncrement();
    }

    ASTExpr* exprFromCurrentToken()
    {
        auto expr = new ASTExpr(&token);

        // for some kinds, there is associated data to be saved
        // switch (token.kind) {
        // case TKString:
        // case TKRegex:
        // case TKIdentifier:
        // case TKNumber: // not converting for now
        // case TKLineComment:
        //     node->value.string = token.dup();
        //     break;
        // //    node->value.real = strtod(token.pos, NULL);
        // //   break;
        // default:
        //     // errorExpectedToken(parser, token.kind);
        //     node->opPrecedence = TokenKind_getPrecedence(token.kind);
        //     node->opIsRightAssociative
        //         = TokenKind_isRightAssociative(token.kind);
        //     node->opIsUnary = TokenKind_isUnary(token.kind);
        //     break;
        // }
        // if (token.kind == TKNumber) {
        //     // turn all 1.0234[DdE]+01 into 1.0234e+01
        //     str_tr_ip(node->value.string, 'd', 'e');
        //     str_tr_ip(node->value.string, 'D', 'e');
        //     str_tr_ip(node->value.string, 'E', 'e');
        // }
        token.advance();
        return expr;
    }

    //#pragma mark Parsing Primitives

    ASTExpr* next_token_node(TokenKind expected, const bool ignore_error)
    {
        if (token.kind == expected) {
            return exprFromCurrentToken();
        } else {
            if (not ignore_error) errorExpectedToken(expected);
            return NULL;
        }
    }

    // in the match case, token should be advanced on error
    ASTExpr* match(TokenKind expected)
    {
        return next_token_node(expected, false);
    }

    // this returns the match node or null
    ASTExpr* trymatch(TokenKind expected)
    {
        return next_token_node(expected, true);
    }

    // just yes or no, simple
    bool matches(TokenKind expected) { return (token.kind == expected); }

    bool ignore(TokenKind expected)
    {
        bool ret;
        if ((ret = matches(expected))) token.advance();
        return ret;
    }

    // this is same as match without return
    void discard(TokenKind expected)
    {
        if (not ignore(expected)) errorExpectedToken(expected);
    }
    //=============================================================================
    // PARSER RULE-MATCHING FUNCTIONS
    //=============================================================================
    //#pragma mark Parser Rules

    char* parseIdent()
    {
        if (token.kind != TKIdentifier) errorExpectedToken(TKIdentifier);
        char* p = token.dup();
        token.advance();
        return p;
    }

    ASTExpr* parseExpr()
    {

        // there are 2 steps to this madness.
        // 1. parse a sequence of tokens into RPN using shunting-yard.
        // 2. walk the rpn stack as a sequence and copy it into a result
        // stack, collapsing the stack when you find nonterminals (ops, func
        // calls, array index, ...)

        // we could make this static and set len to 0 upon func exit
        Stack<ASTExpr*> rpn, ops, result;
        int prec_top = 0;
        ASTExpr* p = NULL;

        // ******* STEP 1 CONVERT TOKENS INTO RPN

        while (token.kind != TKNullChar and token.kind != TKNewline
            and token.kind != TKLineComment) { // build RPN

            // you have to ensure that ops have a space around them, etc.
            // so don't just skip the one spaces like you do now.
            if (token.kind == TKOneSpace) token.advance();

            ASTExpr* expr = exprFromCurrentToken();
            int prec = expr->opPrecedence;
            bool rassoc = expr->opIsRightAssociative;

            switch (expr->kind) {
            case TKIdentifier:
                if (token.kind == TKParenOpen) {
                    expr->kind = TKFunctionCall;
                    expr->opPrecedence = 100;
                    ops.push(expr);
                } else if (token.kind == TKArrayOpen) {
                    expr->kind = TKSubscript;
                    expr->opPrecedence = 100;
                    ops.push(expr);
                } else {
                    rpn.push(expr);
                }
                break;
            case TKOpColon: {
            } break;
            case TKComma: {
                // e.g. in func args list etc.
                // don't need comma because identf/identa exprs will save
                // nargs.
            } break;
            case TKParenOpen: {
                ops.push(expr);
                if (not ops.empty() and ops.top()->kind == TKFunctionCall)
                    rpn.push(expr);
                // instead of marking with (, could consider pushing a NULL.
                // (for both func calls & array indexes)

            } break;
            case TKArrayOpen: {
                ops.push(expr);
                if (not ops.empty() and ops.top()->kind == TKSubscript)
                    rpn.push(expr);
            } break;
            case TKParenClose:
            case TKArrayClose:
            case TKBraceClose: {

                TokenKind revBrkt = TokenKind_reverseBracket(expr->kind);
                while (not ops.empty()) {
                    p = ops.pop();
                    if (p->kind == revBrkt) break;
                    rpn.push(p);
                }

                // result.push( expr); // for funcs/arrays only.
                // or we could not use these as seps but mark the func ident
                // as TKIdentf and set nargs. NOPE u need to know nargs
                // before starting to parse args.
            } break;
            default:
                if (prec) { // general operators
                    while (not ops.empty()) //
                    {
                        prec_top = ops.top()->opPrecedence;
                        if (not prec_top) break;
                        if (prec > prec_top) break;
                        if (prec == prec_top and rassoc) break;
                        p = ops.pop();
                        rpn.push(p);
                    }
                    ops.push(expr);
                } else {
                    rpn.push(expr);
                }
            }

            if (token.kind == TKOneSpace) token.advance();
        }
    exitloop:

        while (not ops.empty()) //
        {
            p = ops.pop();
            rpn.push(p);
        }

        int i;
        printf("\nops: ");
        for (i = 0; i < ops.count; i++)
            printf("%s ", TokenKind_repr(ops[i]->kind));
        printf("\nrpn: ");
        for (i = 0; i < rpn.count; i++)
            printf("%s ", TokenKind_repr(rpn[i]->kind));
        puts("\n");

        assert(ops.count == 0);

        // *** STEP 2 CONVERT RPN INTO EXPR TREE

        // Stack<ASTExpr*> result = { NULL, 0, 0 };

        for (i = 0; i < rpn.count; i++) {
            p = rpn[i];
            switch (p->kind) {
            case TKFunctionCall:
                break;
            case TKNumber:
                break;
            case TKIdentifier:
                break;
            case TKParenOpen:
                break;
            default:
                // careful now, assuming everything except those above is a
                // nonterminal and needs left/right
                if (not p->opPrecedence) continue;
                // operator must have some precedence, right?
                // p->kind = NKExpr;
                if (not p->opIsUnary) p->right = result.pop();
                p->left = result.pop();
            }
            result.push(p);
        }

        assert(result.count == 1);

        return result[0];
    }

    ASTTypeSpec* parseTypeSpec()
    { // must have ident(U), then may have "[:,:]" i.e. '[\]\[\:, ]+' , then
      // may have units. note: after ident may have params <T, S>
        token.flags.mergeArrayDims = true;

        auto typeSpec = new ASTTypeSpec;
        // exprFromCurrentToken();
        // node->kind = NKTypeSpec;
        // this is an ident, but we will turn it into a typespec.
        // name is in this token already
        //  discard(TKOpColon);
        //        discard(TKOneSpace);
        typeSpec->name = parseIdent();
        //        typeSpec->params = parseParams();
        if (matches(TKArrayDims)) {
            for (int i = 0; i < token.matchlen; i++)
                if (token.pos[i] == ':') typeSpec->dims++;
            if (!typeSpec->dims) typeSpec->dims = 1; // [] is 1 dim
            token.advance();
        }
        //        typeSpec->dims = trymatch(TKArrayDims);
        // these funcs should return a Token or TokenRef (char*,len,kind) ,
        // not Expr
        typeSpec->units = parseUnits();
        ignore(TKUnits);
        // fixme: node->type = lookupType;

        token.flags.mergeArrayDims = false;
        return typeSpec;
    }

    List<ASTVar*> parseArgs()
    {
        token.flags.mergeArrayDims = true;
        discard(TKParenOpen);

        List<ASTVar*> args;
        ASTVar* arg;
        do {
            arg = parseVar (); //new ASTVar;
//            arg->name = parseIdent();
//            if (ignore(TKOpColon)) {
//                discard(TKOneSpace);
//                arg->typeSpec = parseTypeSpec();
//            }
//            if (ignore(TKOneSpace)) {
//                discard(TKOpAssign);
//                discard(TKOneSpace);
//                arg->init = parseExpr();
//            }
            args.append(arg);
        } while (ignore(TKComma));

        discard(TKParenClose);
        token.flags.mergeArrayDims = false;

        return args;
    }

    ASTVar* parseVar()
    {
        auto var = new ASTVar; // exprFromCurrentToken();
        var->flags.isVar = (token.kind == TKKeyword_var);
        var->flags.isLet = (token.kind == TKKeyword_let);

        if (var->flags.isVar) discard(TKKeyword_var);
        if( var->flags.isLet) discard(TKKeyword_var);
        if (var->flags.isVar or var->flags.isLet) discard(TKOneSpace);

        token.flags.noKeywordDetect = true;
        var->name = parseIdent();

        if (ignore(TKOpColon)) {
            //            if (token.flags.strictSpacing)
            discard(TKOneSpace);
            var->typeSpec = parseTypeSpec();
        }
        token.flags.noKeywordDetect = false;

        //        if (token.flags.strictSpacing)

        if (ignore(TKOneSpace) ) {
            //            if (token.flags.strictSpacing)
            discard(TKOpAssign); discard(TKOneSpace);
            var->init = parseExpr();
        }
        return var;
    }

    ASTScope* parseScope()
    {
        // TokenKind kind = token.kind;
        auto scope = new ASTScope;
        // scope->parent
        // scope->kind = NKScope;
        union {
            ASTScope* subScope;
            ASTVar* var = NULL;
        }; // last variable encountered

        // don't conflate this with the while in parse(): it checks against
        // file end, this checks against the keyword 'end'.
        while (token.kind != TKKeyword_end) {
            switch (token.kind) {
            case TKNullChar:
                errorExpectedToken(TKUnknown); // return NULL;
                goto exitloop;
            case TKKeyword_var:
                var = parseVar();
                if (!var) continue;
                scope->locals.append(var);

                break;
            // case TKKeyword_base:
            //     discard(TKKeyword_base);
            //     if (parent and parent->kind == NKType)
            //         parent->type.super = parseIdent();
            // case TKIdentifier: // check, return
            // break;
            case TKKeyword_check:
            case TKKeyword_print:
            case TKKeyword_return:

                break;
            case TKKeyword_if:
            case TKKeyword_for:
            case TKKeyword_while:
                // not quite
                subScope = parseScope();
                subScope->parent = scope;
                break;
            case TKNewline:
            case TKLineComment:
            case TKOneSpace: // found at beginning of line
                break;
            default:
                // this shouldn't be an error actually, just put it through
                // parse_expr
                errorExpectedToken(TKUnknown);
                // token.advance();
                break;
            }
            token.advance();
        }
    exitloop:
        return scope;
    }

    List<ASTVar*> parseParams()
    {
        discard(TKOpLT);
        List<ASTVar*> params;
        ASTVar* param;
        do {
            param = new ASTVar;
            param->name = parseIdent();
            if (ignore(TKOpColon)) param->typeSpec = parseTypeSpec();
            if (ignore(TKOpAssign)) param->init = parseExpr();
            params.append(param);
        } while (ignore(TKComma));

        discard(TKOpGT);
        return params;
    }

    ASTFunc* parseFunc()
    {
        // shouldn't this be new_node_from...? or change parse_type &
        // others for consistency
        discard(TKKeyword_function);
        discard(TKOneSpace);
        auto func = new ASTFunc; // match(TKKeyword_function);

        // if (func) {
        // node->kind = NKFunc;
        token.flags.noKeywordDetect = true;

        func->name = parseIdent();
        func->args = parseArgs();
        if (ignore(TKOpColon)) {
            discard(TKOneSpace);
            func->returnType = parseTypeSpec();
        }
        token.flags.noKeywordDetect = false;
        // if (!matches( TKKw_end)) {
        func->body = parseScope(); //}
        // }
        discard(TKKeyword_end);
        discard(TKOneSpace);
        discard(TKKeyword_function);
        return func;
    }

    ASTFunc* parseTest() { return NULL; }

    ASTUnits* parseUnits() { return NULL; }

    ASTType* parseType()
    {
        auto type = new ASTType; // exprFromCurrentToken();
        union {
            ASTExpr* expr;
            ASTVar* var;
        };
        // node->kind = NKType;
        discard(TKKeyword_type);
        discard(TKOneSpace);
        type->name = parseIdent();
        if (matches(TKOpLT)) type->params = parseParams();

        while (token.kind != TKKeyword_end) {
            switch (token.kind) {
            case TKNullChar:
                errorExpectedToken(TKUnknown); // return NULL;
                goto exitloop;
            case TKKeyword_var:
                var = parseVar();
                if (!var) continue;
                type->vars.append(var);

                break;
            case TKKeyword_base:
                discard(TKKeyword_base);
                discard(TKOneSpace);
                type->super = parseTypeSpec();
            case TKIdentifier:
                break;
            case TKKeyword_check:
                break;
            case TKNewline:
            case TKLineComment:
                break;
            case TKOneSpace:
                // token.advance(); // indentation at begin of line etc.
                break;
            default:
                // this shouldn't be an error actually, just put it through
                // parse_expr
                errorExpectedToken(TKUnknown);
                // token.advance();
                break;
            }
            token.advance();
        }
    exitloop:

        //        ASTScope* body = parseScope();
        // will set super for this type
        //        type->members = scope->stmts;
        // Node* item = NULL;
        //    for (item = body->stmts; //
        //         item != NULL; //
        //         item = item->next) {
        // can't have locals and stmts separately since next ptr is
        // embedded. so the same element cant be part of more than one list.
        // for types, we cap wipe the next as we go and append checks and
        // vars to different lists. for scopes however there cant be a
        // locals -- you have to walk the stmts and extract vars in order to
        // process locals. YOU HAVE TO DO THE SAME HERE FOR TYPES! how will
        // you get item->next to advance this loop if you go on wiping it
        // inside the loop
        //        switch (item->kind) {
        //            case NKCheck:
        //                item->next=NULL;
        //                append(&node->checks, item);
        //                break;
        //            case NKVar:
        //                item->next=NULL;
        //                append(&node->vars, item);
        //                break;
        //            default:
        //                //error
        //                break;
        //        }
        //        if (item->kind == NKCheck) {
        //            append(&(node->checks), item);
        //        } else {
        //            // error
        //        }
        //    }
        discard(TKKeyword_end);
        discard(TKOneSpace);
        /* !strict ? ignore : */
        discard(TKKeyword_type);
        return type;
    }

    ASTImport* parseImport()
    {
        auto import = new ASTImport;
        discard(TKKeyword_import);
        discard(TKOneSpace);
        token.flags.noKeywordDetect = true;
        import->isPackage = ignore(TKAt);
        import->importFile = parseIdent();
        // set default name?
        token.flags.noKeywordDetect = false;
        ignore(TKOneSpace);
        if (ignore(TKKeyword_as)) {
            token.flags.noKeywordDetect = true;
            ignore(TKOneSpace);
            import->alias = parseIdent();
            token.flags.noKeywordDetect = false;
        } else {
            import->alias = str_base(import->importFile, '.');
        }
        //        assert(matches(TKNewline));
        return import;
    }

    List<ASTModule*> parse()
    {
        auto root = new ASTModule;
        // root->kind = NKModule;
        root->name = moduleName;
        const bool onlyPrintTokens = false;
        // ASTNode* node;
        // TokenKind kind;
        token.advance(); // maybe put this in parser ctor
        ASTImport* import = NULL;

        while (token.kind != TKNullChar) {
            if (onlyPrintTokens) {
                printf("%s %2d %3d %3d %-6s\t%.*s\n", basename, token.line,
                    token.col, token.matchlen, TokenKind_repr(token.kind),
                    token.kind == TKNewline ? 0 : token.matchlen,
                    token.pos);
                token.advance();
                continue;
            }
            switch (token.kind) {
            case TKKeyword_function:
                //                node.func = parseFunc();
                //                if (node.func)
                root->funcs.append(parseFunc());
                break;
            case TKKeyword_type:
                //                ASTType* type = ;
                //                if (type)
                root->types.append(parseType());
                break;
            case TKKeyword_import:
                import = parseImport();
                if (import) {
                    root->imports.append(import);
                    //                    auto subParser = new
                    //                    Parser(import->importFile);
                    //                    List<ASTModule*> subMods =
                    //                    subParser->parse();
                    //                    modules.append(subMods);
                }
                break;
            case TKKeyword_test:
                //                ASTFunc* test =
                //                if (test)
                root->tests.append(parseTest());
                break;
            case TKKeyword_var:
            case TKKeyword_let:
                //                ASTVar* var = parseVar();
                //                if (var)
                root->globals.append(parseVar());
                break;
            case TKNewline:
            case TKLineComment:
                //            case TKOneSpace:
                break;
                //#ifndef PRINTTOKENS
            default:
                //#endif
                printf("other token: %s at %d:%d len %d\n", token.repr(),
                    token.line, token.col, token.matchlen);
                token.advance();
            }
            // token.advance();// this shouldnt be here, the specific
            // funcs do it
            ignore(TKNewline);
            ignore(TKLineComment);
        }
        modules.append(root);
        return modules;
    }
};

Pool<ASTTypeSpec> ASTTypeSpec::pool;
Pool<ASTVar> ASTVar::pool;
Pool<ASTImport> ASTImport::pool;
Pool<ASTModule> ASTModule::pool;
Pool<ASTFunc> ASTFunc::pool;
Pool<ASTType> ASTType::pool;
Pool<ASTExpr> ASTExpr::pool;
Pool<ASTScope> ASTScope::pool;
Pool<Parser> Parser::pool;

//#pragma mark Print AST

int main(int argc, char* argv[])
{
    if (argc == 1) return 1;
    //    print_sizes();
    auto parser = new Parser(argv[1]);

    List<ASTModule*> modules = parser->parse(); // parser.modules;
    //    ASTModule* module;
    if (!parser->errCount) {
        if (modules.item) {
            foreach (mod, mods, modules) {
                if (!mod) continue;
                mod->gen();
            }
        }
    } // while ((modules = modules.next)); // (modules, 0);
    alloc_stat();
    return 0;
}

// linter needs to :
// 1. format spacing line breaks etc.
// 2. replace E in floats with e
// 3. rearrange comments
// 4. replace end with end if/for/etc.
