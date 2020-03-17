//=============================================================================
// TOKEN
//=============================================================================

// Various kinds of tokens.
typedef enum TokenKind {
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
} TokenKind;

// Holds information about a syntax token.
typedef struct Token {
    char* pos;
    uint32_t matchlen : 24;
    struct {
        bool_t //
            skipws : 1, // skip whitespace
            mergearraydims : 1; // merge [:,:,:] into one token
    } flags;
    uint16_t line;
    uint8_t col;
    TokenKind kind : 8;
} Token;


// Return the repr of a token kind (for debug)
const char* Token_repr(const TokenKind kind)
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

bool_t Token_isUnary(TokenKind kind)
{
    return kind == TKKeyword_not; // unary - is handled separately
}

bool_t Token_isRightAssociative(TokenKind kind)
{
    return kind == TKPeriod || kind == TKPower;
}

uint8_t Token_getPrecedence(TokenKind kind)
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

TokenKind reverseBracket(TokenKind kind)
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

char* Token_strdup(const Token* const token)
{
    return strndup(token->pos, token->matchlen);
}

// Advance the token position by one char.
void Token_advance1(Token* token)
{
    // this is called after a token has been consumed and the pointer
    // needs to advance. If skipws is set, loop until the next non-space.
    // do {
    token->pos++;

    // } while (token->skipws && *(token->pos) == ' ');
}

// Peek at the char after the current (complete) token
char Token_peekCharAfter(Token* token)
{
    return *(token->pos + token->matchlen + 1);
}

#define Token_compareKeyword(tok)                                          \
    if (sizeof(#tok) - 1 == l && !strncmp(#tok, s, l))                     \
    return TKKeyword_##tok

// Check if an (ident) token matches a keyword and return its type
// accordingly.
TokenKind Token_tryKeywordMatch(const Token* token)
{
    if (token->kind != TKIdentifier) return token->kind;

    const char* s = token->pos;
    const int l = token->matchlen;

    Token_compareKeyword(and);
    Token_compareKeyword(cheater);
    Token_compareKeyword(for);
    Token_compareKeyword(do);
    Token_compareKeyword(while);
    Token_compareKeyword(if);
    Token_compareKeyword(then);
    Token_compareKeyword(end);
    Token_compareKeyword(function);
    Token_compareKeyword(test);
    Token_compareKeyword(not);
    Token_compareKeyword(and);
    Token_compareKeyword(or);
    Token_compareKeyword(in);
    Token_compareKeyword(else);
    Token_compareKeyword(type);
    Token_compareKeyword(check);
    Token_compareKeyword(base);
    Token_compareKeyword(var);
    Token_compareKeyword(let);
    Token_compareKeyword(import);
    Token_compareKeyword(as);
//    Token_compareKeyword(only);
    Token_compareKeyword(print);
    return TKIdentifier;
}

// Get the type based on the char at the current position.
#define Token_gettype(t) Token_getTypeAtOffset(t, 0)

// Get the type based on the char after the current position (don't conflate
// this with char* Token_peekCharAfter(token))
#define Token_peek_nextchar(t) Token_getTypeAtOffset(t, 1)

// Get the token kind based only on the char at the current position (or an
// offset).
TokenKind Token_getTypeAtOffset(const Token* self, const size_t offset)
{
    const char c = self->pos[offset];
    const char cn = c ? self->pos[1 + offset] : 0;

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
        if (isalpha(c) || c == '_') {
            return TKAlphabet;
        } else if (isdigit(c)) {
            return TKDigit;
        } else {
            return TKUnknown;
        }
        break;
    }
}

// Scans ahead from the current position until the actual end of the token.
void Token_detect(Token* token)
{
    TokenKind tt = Token_gettype(token);
    TokenKind tt_ret; // = tt;
    TokenKind tmp;
    char* start = token->pos;
    bool_t found_e = false, found_dot = false, found_cmt = false;
    uint8_t found_spc = 0;

    switch (tt) {
    case TKStringBoundary:
    case TKInlineBoundary:
    case TKRegexBoundary:
        tmp = tt; // remember which it is exactly

        // Incrementing pos is a side effect of Token_gettype(...)
        while (tt != TKNullChar) {
            // here we want to consume the ending " so we move next before
            Token_advance1(token);
            tt = Token_gettype(token);
            if (tt == TKNullChar || tt == tmp) {
                Token_advance1(token);
                break;
            }
            if (tt == TKBackslash)
                if (Token_peek_nextchar(token) == tmp) { // why if?
                    Token_advance1(token);
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
        while (tt != TKNullChar) {
            // here we dont want to consume the end char, so break before
            tt = Token_peek_nextchar(token);
            Token_advance1(token);
            if (tt != TKSpaces) break;
        }
        tt_ret = TKSpaces;
        break;

    case TKComma:
        while (tt != TKNullChar) {
            tt = Token_peek_nextchar(token);
            Token_advance1(token);
            // line number should be incremented for line continuations
            if (tt == TKSpaces) {
                found_spc++;
            }
            if (tt == TKExclamation) {
                found_cmt = true;
            }
            if (tt == TKNewline) {
                token->line++;
                token->col = -found_spc - 1; // account for extra spaces
                                             // after , and for nl itself
                found_spc = 0;
            }
            if (found_cmt && tt != TKNewline) {
                found_spc++;
                continue;
            }
            if (tt != TKSpaces && tt != TKNewline) break;
        }
        tt_ret = TKComma;
        break;

    case TKArrayOpen:
        // mergearraydims should be set only when reading func args
        if (!token->flags.mergearraydims) goto defaultToken;

        while (tt != TKNullChar) {
            tt = Token_peek_nextchar(token);
            Token_advance1(token);
            if (tt != TKOpColon && tt != TKComma) break;
        }
        tt = Token_gettype(token);
        if (tt != TKArrayClose) {
            fprintf(stderr, "expected a ']', found a '%c'. now what?\n",
                *token->pos);
        }
        Token_advance1(token);
        tt_ret = TKArrayDims;
        break;

    case TKAlphabet:
        while (tt != TKNullChar) {
            tt = Token_peek_nextchar(token);
            Token_advance1(token);
            if (tt != TKAlphabet //
                && tt != TKDigit //
                && tt != TKUnderscore && tt != TKPeriod)
                break; /// validate in parser not here
        }
        tt_ret = TKIdentifier;
        break;

    case TKExclamation:
        while (tt != TKNullChar) {
            tt = Token_peek_nextchar(token);
            Token_advance1(token);
            if (tt == TKNewline) break;
        }
        tt_ret = TKLineComment;
        break;

    case TKPipe:
        while (tt != TKNullChar) {
            tt = Token_peek_nextchar(token);
            Token_advance1(token);
            if (tt != TKAlphabet && tt != TKDigit && tt != TKSlash
                && tt != TKPeriod)
                break;
        }
        tt_ret = TKUnits;
        break;

    case TKDigit:
        while (tt != TKNullChar) // EOF, basically null char
        {
            tt = Token_peek_nextchar(token);
            // numbers such as 1234500.00 are allowed
            // very crude, error-checking is parser's job
            //            if (tt==TKDigit //
            //                || tt==TKPlus //
            //                || tt==TKMinus //
            //                || tt==TKPeriod //
            //                || *token->pos == 'e' //
            //                || *token->pos == 'E' //
            //                || *token->pos == 'd'
            //                || *token->pos == 'D')
            Token_advance1(token);

            if (*token->pos == 'e' //
                || *token->pos == 'E' //
                || *token->pos == 'd'
                || *token->pos == 'D') { // will all be changed to e btw
                found_e = true;
                continue;
            }
            if (found_e) { //}&& (*token->pos == '-' || *token->pos == '+'))
                           //{
                found_e = false;
                continue;
            }
            if (tt == TKPeriod) {
                found_dot = true;
                continue;
            }
            //            if (found_dot && tt == TKPeriod) {
            //                fprintf(stderr, "raise error: multiple dots in
            //                number\n"); break;
            //            }
            // lets allow that for IP addresses etc., or just bytes
            if (tt != TKDigit && tt != TKPeriod) break;
        }
        tt_ret = TKNumber;
        break;

        /* case TKNl:
             token->line++;
             token->col=1;
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
        Token_advance1(token);
        // don'self break
    default:
    defaultToken:
        tt_ret = tt;
        Token_advance1(token);
        break;
    }

    token->matchlen = (uint32_t)(token->pos - start);
    token->pos = start; // rewind. but why! then again advance rewind
                        // advance rewind
    token->kind = tt_ret;
    // keywords have their own token type
    if (token->kind == TKIdentifier)
        token->kind = Token_tryKeywordMatch(token);
    // exactly one space is TKOnespc, otherwise TKSpc.
    // the compiler needs to check one space frequently in strict mode.
    // FIXME figure it out later
    if (token->kind == TKSpaces && token->matchlen == 1)
        token->kind = TKOneSpace;
}

// Advances the parser to the next token and skips whitespace if the
// parser's flag `skipws` is set.
void Token_advance(Token* token)
{
    token->pos += token->matchlen;
    token->col += token->matchlen;
    token->matchlen = 0;
    // Token_advance1(token);
    Token_detect(token);
    if (token->flags.skipws
        && (token->kind == TKSpaces
#ifndef PARSE_STRICT
            || token->kind == TKOneSpace
#endif
            ))
        Token_advance(token);
    if (token->kind == TKNewline) {
        token->line++;
        token->col = 0; // position of the nl itself is 0
    }
}
