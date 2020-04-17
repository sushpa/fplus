
const uint8_t TokenKindTable[256] = {
    /* 0 */ TKNullChar, /* 1 */ TKUnknown, /* 2 */ TKUnknown,
    /* 3 */ TKUnknown, /* 4 */ TKUnknown, /* 5 */ TKUnknown,
    /* 6 */ TKUnknown, /* 7 */ TKUnknown, /* 8 */ TKUnknown,
    /* 9 */ TKUnknown, /* 10 */ TKNewline, /* 11 */ TKUnknown,
    /* 12 */ TKUnknown, /* 13 */ TKUnknown, /* 14 */ TKUnknown,
    /* 15 */ TKUnknown, /* 16 */ TKUnknown, /* 17 */ TKUnknown,
    /* 18 */ TKUnknown, /* 19 */ TKUnknown, /* 20 */ TKUnknown,
    /* 21 */ TKUnknown, /* 22 */ TKUnknown, /* 23 */ TKUnknown,
    /* 24 */ TKUnknown, /* 25 */ TKUnknown, /* 26 */ TKUnknown,
    /* 27 */ TKUnknown, /* 28 */ TKUnknown, /* 29 */ TKUnknown,
    /* 30 */ TKUnknown, /* 31 */ TKUnknown,
    /* 32   */ TKSpaces, /* 33 ! */ TKExclamation,
    /* 34 " */ TKStringBoundary, /* 35 # */ TKHash, /* 36 $ */ TKDollar,
    /* 37 % */ TKOpMod, /* 38 & */ TKAmpersand, /* 39 ' */ TKRegexBoundary,
    /* 40 ( */ TKParenOpen, /* 41 ) */ TKParenClose, /* 42 * */ TKTimes,
    /* 43 + */ TKPlus, /* 44 , */ TKOpComma, /* 45 - */ TKMinus,
    /* 46 . */ TKPeriod, /* 47 / */ TKSlash, /* 48 0 */ TKDigit,
    /* 49 1 */ TKDigit, /* 50 2 */ TKDigit, /* 51 3 */ TKDigit,
    /* 52 4 */ TKDigit, /* 53 5 */ TKDigit, /* 54 6 */ TKDigit,
    /* 55 7 */ TKDigit, /* 56 8 */ TKDigit, /* 57 9 */ TKDigit,
    /* 58 : */ TKOpColon, /* 59 ; */ TKOpSemiColon, /* 60 < */ TKOpLT,
    /* 61 = */ TKOpAssign, /* 62 > */ TKOpGT, /* 63 ? */ TKQuestion,
    /* 64 @ */ TKAt,
    /* 65 A */ TKAlphabet, /* 66 B */ TKAlphabet, /* 67 C */ TKAlphabet,
    /* 68 D */ TKAlphabet, /* 69 E */ TKAlphabet, /* 70 F */ TKAlphabet,
    /* 71 G */ TKAlphabet, /* 72 H */ TKAlphabet, /* 73 I */ TKAlphabet,
    /* 74 J */ TKAlphabet, /* 75 K */ TKAlphabet, /* 76 L */ TKAlphabet,
    /* 77 M */ TKAlphabet, /* 78 N */ TKAlphabet, /* 79 O */ TKAlphabet,
    /* 80 P */ TKAlphabet, /* 81 Q */ TKAlphabet, /* 82 R */ TKAlphabet,
    /* 83 S */ TKAlphabet, /* 84 T */ TKAlphabet, /* 85 U */ TKAlphabet,
    /* 86 V */ TKAlphabet, /* 87 W */ TKAlphabet, /* 88 X */ TKAlphabet,
    /* 89 Y */ TKAlphabet, /* 90 Z */ TKAlphabet,
    /* 91 [ */ TKArrayOpen, /* 92 \ */ TKBackslash, /* 93 ] */ TKArrayClose,
    /* 94 ^ */ TKPower, /* 95 _ */ TKUnderscore,
    /* 96 ` */ TKInlineBoundary,
    /* 97 a */ TKAlphabet, /* 98 b */ TKAlphabet, /* 99 c */ TKAlphabet,
    /* 100 d */ TKAlphabet, /* 101 e */ TKAlphabet, /* 102 f */ TKAlphabet,
    /* 103 g */ TKAlphabet, /* 104 h */ TKAlphabet, /* 105 i */ TKAlphabet,
    /* 106 j */ TKAlphabet, /* 107 k */ TKAlphabet, /* 108 l */ TKAlphabet,
    /* 109 m */ TKAlphabet, /* 110 n */ TKAlphabet, /* 111 o */ TKAlphabet,
    /* 112 p */ TKAlphabet, /* 113 q */ TKAlphabet, /* 114 r */ TKAlphabet,
    /* 115 s */ TKAlphabet, /* 116 t */ TKAlphabet, /* 117 u */ TKAlphabet,
    /* 118 v */ TKAlphabet, /* 119 w */ TKAlphabet, /* 120 x */ TKAlphabet,
    /* 121 y */ TKAlphabet, /* 122 z */ TKAlphabet,
    /* 123 { */ TKBraceOpen, /* 124 | */ TKPipe, /* 125 } */ TKBraceClose,
    /* 126 ~ */ TKTilde,
    /* 127 */ TKUnknown, /* 128 */ TKUnknown, /* 129 */ TKUnknown,
    /* 130 */ TKUnknown, /* 131 */ TKUnknown, /* 132 */ TKUnknown,
    /* 133 */ TKUnknown, /* 134 */ TKUnknown, /* 135 */ TKUnknown,
    /* 136 */ TKUnknown, /* 137 */ TKUnknown, /* 138 */ TKUnknown,
    /* 139 */ TKUnknown, /* 140 */ TKUnknown, /* 141 */ TKUnknown,
    /* 142 */ TKUnknown, /* 143 */ TKUnknown, /* 144 */ TKUnknown,
    /* 145 */ TKUnknown, /* 146 */ TKUnknown, /* 147 */ TKUnknown,
    /* 148 */ TKUnknown, /* 149 */ TKUnknown, /* 150 */ TKUnknown,
    /* 151 */ TKUnknown, /* 152 */ TKUnknown, /* 153 */ TKUnknown,
    /* 154 */ TKUnknown, /* 155 */ TKUnknown, /* 156 */ TKUnknown,
    /* 157 */ TKUnknown, /* 158 */ TKUnknown, /* 159 */ TKUnknown,
    /* 160 */ TKUnknown, /* 161 */ TKUnknown, /* 162 */ TKUnknown,
    /* 163 */ TKUnknown, /* 164 */ TKUnknown, /* 165 */ TKUnknown,
    /* 166 */ TKUnknown, /* 167 */ TKUnknown, /* 168 */ TKUnknown,
    /* 169 */ TKUnknown, /* 170 */ TKUnknown, /* 171 */ TKUnknown,
    /* 172 */ TKUnknown, /* 173 */ TKUnknown, /* 174 */ TKUnknown,
    /* 175 */ TKUnknown, /* 176 */ TKUnknown, /* 177 */ TKUnknown,
    /* 178 */ TKUnknown, /* 179 */ TKUnknown, /* 180 */ TKUnknown,
    /* 181 */ TKUnknown, /* 182 */ TKUnknown, /* 183 */ TKUnknown,
    /* 184 */ TKUnknown, /* 185 */ TKUnknown, /* 186 */ TKUnknown,
    /* 187 */ TKUnknown, /* 188 */ TKUnknown, /* 189 */ TKUnknown,
    /* 190 */ TKUnknown, /* 191 */ TKUnknown, /* 192 */ TKUnknown,
    /* 193 */ TKUnknown, /* 194 */ TKUnknown, /* 195 */ TKUnknown,
    /* 196 */ TKUnknown, /* 197 */ TKUnknown, /* 198 */ TKUnknown,
    /* 199 */ TKUnknown, /* 200 */ TKUnknown, /* 201 */ TKUnknown,
    /* 202 */ TKUnknown, /* 203 */ TKUnknown, /* 204 */ TKUnknown,
    /* 205 */ TKUnknown, /* 206 */ TKUnknown, /* 207 */ TKUnknown,
    /* 208 */ TKUnknown, /* 209 */ TKUnknown, /* 210 */ TKUnknown,
    /* 211 */ TKUnknown, /* 212 */ TKUnknown, /* 213 */ TKUnknown,
    /* 214 */ TKUnknown, /* 215 */ TKUnknown, /* 216 */ TKUnknown,
    /* 217 */ TKUnknown, /* 218 */ TKUnknown, /* 219 */ TKUnknown,
    /* 220 */ TKUnknown, /* 221 */ TKUnknown, /* 222 */ TKUnknown,
    /* 223 */ TKUnknown, /* 224 */ TKUnknown, /* 225 */ TKUnknown,
    /* 226 */ TKUnknown, /* 227 */ TKUnknown, /* 228 */ TKUnknown,
    /* 229 */ TKUnknown, /* 230 */ TKUnknown, /* 231 */ TKUnknown,
    /* 232 */ TKUnknown, /* 233 */ TKUnknown, /* 234 */ TKUnknown,
    /* 235 */ TKUnknown, /* 236 */ TKUnknown, /* 237 */ TKUnknown,
    /* 238 */ TKUnknown, /* 239 */ TKUnknown, /* 240 */ TKUnknown,
    /* 241 */ TKUnknown, /* 242 */ TKUnknown, /* 243 */ TKUnknown,
    /* 244 */ TKUnknown, /* 245 */ TKUnknown, /* 246 */ TKUnknown,
    /* 247 */ TKUnknown, /* 248 */ TKUnknown, /* 249 */ TKUnknown,
    /* 250 */ TKUnknown, /* 251 */ TKUnknown, /* 252 */ TKUnknown,
    /* 253 */ TKUnknown, /* 254 */ TKUnknown, /* 255 */ TKUnknown
};
#define Token_matchesKeyword(tok)                                          \
    if (sizeof(#tok) - 1 == l and not strncmp(#tok, s, l)) return true;

bool doesKeywordMatch(const char* s, const int l)
{
    //        const char* s = pos;
    //        const int l = matchlen;

    Token_matchesKeyword(and)
    Token_matchesKeyword(cheater)
    Token_matchesKeyword(for)
    Token_matchesKeyword(do)
    Token_matchesKeyword(while)
    Token_matchesKeyword(if)
    Token_matchesKeyword(then)
    Token_matchesKeyword(end)
    Token_matchesKeyword(function)
    Token_matchesKeyword(declare)
    Token_matchesKeyword(test)
    Token_matchesKeyword(not)
    Token_matchesKeyword(and)
    Token_matchesKeyword(or)
    Token_matchesKeyword(in)
    Token_matchesKeyword(else)
    Token_matchesKeyword(type)
    //    matchesen_compareKeyword(check)
    Token_matchesKeyword(extends)
    Token_matchesKeyword(var)
    Token_matchesKeyword(let)
    Token_matchesKeyword(import)
    Token_matchesKeyword(return)
    Token_matchesKeyword(returns)
    Token_matchesKeyword(as)
    return false;
}

// Holds information about a syntax this->token.
typedef struct Token {

    char* pos;
    uint32_t matchlen : 24;
    struct {
        bool skipWhiteSpace : 1,
            mergeArrayDims : 1, // merge [:,:,:] into one this->token
            noKeywosrdDetect : 1, // leave keywords as idents
            strictSpacing : 1; // missing spacing around operators etc. is a
                               // compile error YES YOU HEARD IT RIGHT
                               // but why need this AND skipWhiteSpace?
    } flags;
    uint16_t line;
    uint8_t col;
    TokenKind kind : 8;
} Token;

// Peek at the char after the current (complete) token
char Token_peekCharAfter(Token* this)
{
    char* s = this->pos + this->matchlen;
    if (this->flags.skipWhiteSpace)
        while (*s == ' ')
            s++;
    return *s;
}

#define Token_compareKeyword(tok)                                          \
    if (sizeof(#tok) - 1 == l and not strncmp(#tok, s, l)) {               \
        this->kind = TKKeyword_##tok;                                      \
        return;                                                            \
    }

// Check if an (ident) this->token matches a keyword and return its type
// accordingly.
void Token_tryKeywordMatch(Token* this)
{
    if (this->kind != TKIdentifier) return;

    const char* s = this->pos;
    const int l = this->matchlen;

    Token_compareKeyword(and)
    Token_compareKeyword(cheater)
    Token_compareKeyword(for)
    Token_compareKeyword(do)
    Token_compareKeyword(while)
    Token_compareKeyword(if)
    Token_compareKeyword(then)
    Token_compareKeyword(end)
    Token_compareKeyword(function)
    Token_compareKeyword(declare)
    Token_compareKeyword(test)
    Token_compareKeyword(not)
    Token_compareKeyword(and)
    Token_compareKeyword(or)
    Token_compareKeyword(in)
    Token_compareKeyword(else)
    Token_compareKeyword(type)
    //        Token_compareKeyword(check)
    Token_compareKeyword(extends)
    Token_compareKeyword(var)
    Token_compareKeyword(let)
    Token_compareKeyword(import)
    Token_compareKeyword(return)
    Token_compareKeyword(returns)
    Token_compareKeyword(as)
    //        Token_compareKeyword(print);
}

// Get the token kind based only on the char at the current position
// (or an offset).
TokenKind Token_getType(Token* this, const size_t offset)
{
    const char c = this->pos[offset];
    const char cn = c ? this->pos[1 + offset] : 0;
    TokenKind ret = (TokenKind)TokenKindTable[c];
    switch (c) {
    case '<':
        switch (cn) {
        case '=':
            return TKOpLE;
        default:
            return TKOpLT;
        }
    case '>':
        switch (cn) {
        case '=':
            return TKOpGE;
        default:
            return TKOpGT;
        }
    case '=':
        switch (cn) {
        case '=':
            return TKOpEQ;
        case '>':
            return TKOpResults;
        default:
            return TKOpAssign;
        }
    case '+':
        switch (cn) {
        case '=':
            return TKPlusEq;
        }
        return TKPlus;
    case '-':
        switch (cn) {
        case '=':
            return TKMinusEq;
        }
        return TKMinus;
    case '*':
        switch (cn) {
        case '=':
            return TKTimesEq;
        }
        return TKTimes;
    case '/':
        switch (cn) {
        case '=':
            return TKSlashEq;
        }
        return TKSlash;
    case '^':
        switch (cn) {
        case '=':
            return TKPowerEq;
        }
        return TKPower;
    case '%':
        switch (cn) {
        case '=':
            return TKOpModEq;
        }
        return TKOpMod;
    case '!':
        switch (cn) {
        case '=':
            return TKOpNE;
        }
        return TKExclamation;
    case ':':
        switch (cn) {
        case '=':
            return TKColEq;
        default:
            return TKOpColon;
        }
    default:
        return ret;
    }
}

void Token_detect(Token* this)
{
    TokenKind tt = Token_getType(this, 0);
    TokenKind tt_ret = TKUnknown; // = tt;
    static TokenKind tt_last
        = TKUnknown; // the previous this->token that was found
    static TokenKind tt_lastNonSpace
        = TKUnknown; // the last non-space this->token found
    TokenKind tmp;
    char* start = this->pos;
    bool found_e = false, found_dot = false, found_cmt = false;
    uint8_t found_spc = 0;

    switch (tt) {
    case TKStringBoundary:
    case TKInlineBoundary:
    case TKRegexBoundary:
        tmp = tt; // remember which it is exactly

        // Incrementing pos is a side effect of getTypeAtCurrentPos(...)
        while (tt != TKNullChar) {
            // here we want to consume the ending " so we move next
            // before
            this->pos++;
            tt = Token_getType(this, 0);
            if (tt == TKNullChar or tt == tmp) {
                this->pos++;
                break;
            }
            if (tt == TKBackslash)
                if (Token_getType(this, 1) == tmp) { // why if?
                    this->pos++;
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
        if (tt_last == TKOneSpace) // if prev char was a space return
                                   // this as a run of spaces
            while (tt != TKNullChar) {
                // here we dont want to consume the end char, so break
                // before
                tt = Token_getType(this, 1);
                this->pos++;
                if (tt != TKSpaces) break;
            }
        else
            this->pos++;
        // else its a single space
        tt_ret = TKSpaces;
        break;

    case TKOpComma:
    case TKOpSemiColon:
        //        line continuation tokens
        tt_ret = tt;

        while (tt != TKNullChar) {
            tt = Token_getType(this, 1);
            this->pos++;
            // line number should be incremented for line continuations
            if (tt == TKSpaces) {
                found_spc++;
            }
            if (tt == TKExclamation) {
                found_cmt = true;
            }
            if (tt == TKNewline) {
                this->line++;
                this->col = -found_spc - 1; // account for extra spaces
                                            // after , and for nl itself
                found_spc = 0;
            }
            if (found_cmt and tt != TKNewline) {
                found_spc++;
                continue;
            }
            if (tt != TKSpaces and tt != TKNewline) break;
        }
        break;

    case TKArrayOpen:
        // mergearraydims should be set only when reading func args
        if (not this->flags.mergeArrayDims) goto defaultToken;

        while (tt != TKNullChar) {
            tt = Token_getType(this, 1);
            this->pos++;
            if (tt != TKOpColon and tt != TKOpComma) break;
        }
        tt = Token_getType(this, 0);
        if (tt != TKArrayClose) {
            eprintf(
                "expected a ']', found a '%c'. now what?\n", *this->pos);
        }
        this->pos++;
        tt_ret = TKArrayDims;
        break;

    case TKAlphabet:
    case TKPeriod:
    case TKUnderscore:
        while (tt != TKNullChar) {
            tt = Token_getType(this, 1);
            this->pos++;
            if (tt != TKAlphabet and tt != TKDigit and tt != TKUnderscore
                and tt != TKPeriod)
                break; /// validate in parser not here
        }
        tt_ret = TKIdentifier;
        break;

    case TKHash: // TKExclamation:
        while (tt != TKNullChar) {
            tt = Token_getType(this, 1);
            this->pos++;
            if (tt == TKNewline) break;
        }
        tt_ret = TKLineComment;
        break;

    case TKPipe:
        while (tt != TKNullChar) {
            tt = Token_getType(this, 1);
            this->pos++;
            if (tt != TKAlphabet and tt != TKDigit and tt != TKSlash
                and tt != TKPeriod)
                break;
        }
        tt_ret = TKUnits;
        break;

    case TKDigit:
        tt_ret = TKNumber;

        while (tt != TKNullChar) // EOF, basically null char
        {
            tt = Token_getType(this, 1);
            // numbers such as 1234500.00 are allowed
            // very crude, error-checking is parser's job
            this->pos++;

            if (*this->pos == 'e' or *this->pos == 'E' or *this->pos == 'd'
                or *this->pos == 'D') { // will all be changed to e btw
                found_e = true;
                continue;
            }
            if (found_e) {
                found_e = false;
                continue;
            }
            if (tt == TKPeriod) {
                found_dot = true;
                continue;
            }
            if (found_dot and tt == TKPeriod) tt_ret = TKMultiDotNumber;

            if (tt != TKDigit and tt != TKPeriod and *this->pos != 'i')
                break;
        }
        break;

    case TKMinus:

        switch (tt_lastNonSpace) {
        case TKParenClose:
        case TKIdentifier: // keywords too?
        case TKNumber:
        case TKArrayClose:
        case TKArrayDims:
        case TKMultiDotNumber:
            tt_ret = tt;
            break;
        default:
            tt_ret = TKUnaryMinus;
            break;
        }
        this->pos++;
        break;

    case TKOpNotResults:
        // 3-char tokens
        this->pos++;
    case TKOpEQ:
    case TKOpGE:
    case TKOpLE:
    case TKOpNE:
    case TKOpResults:
    case TKBackslash:
    case TKColEq:
    case TKPlusEq:
    case TKMinusEq:
    case TKTimesEq:
    case TKSlashEq:
    case TKPowerEq:
    case TKOpModEq:

        // 2-char tokens
        this->pos++;
    default:
    defaultToken:
        tt_ret = tt;
        this->pos++;
        break;
    }

    this->matchlen = (uint32_t)(this->pos - start);
    this->pos = start;
    this->kind = tt_ret;

    if (this->kind == TKIdentifier) Token_tryKeywordMatch(this);

    if (this->kind == TKSpaces and this->matchlen == 1)
        this->kind = TKOneSpace;

    tt_last = this->kind;
    if (tt_last != TKOneSpace and tt_last != TKSpaces)
        tt_lastNonSpace = tt_last;
}

// Advance to the next this->token (skip whitespace if `skipws` is set).
void Token_advance(Token* this)
{
    switch (this->kind) {
    case TKIdentifier:
    case TKString:
    case TKNumber:
    case TKMultiDotNumber:
    case TKFunctionCall:
    case TKSubscript:
    case TKDigit:
    case TKAlphabet:
    case TKRegex:
    case TKInline:
    case TKUnits:
    case TKKeyword_cheater:
    case TKKeyword_for:
    case TKKeyword_while:
    case TKKeyword_if:
    case TKKeyword_end:
    case TKKeyword_function:
    case TKKeyword_test:
    case TKKeyword_not:
    case TKKeyword_and:
    case TKKeyword_or:
    case TKKeyword_in:
    case TKKeyword_do:
    case TKKeyword_then:
    case TKKeyword_as:
    case TKKeyword_else:
    case TKKeyword_type:
    case TKKeyword_return:
    case TKKeyword_extends:
    case TKKeyword_var:
    case TKKeyword_let:
    case TKKeyword_import:
    case TKUnknown: // bcz start of the file is this
        break;
    default:
        *this->pos = 0; // trample it so that idents etc. can be assigned
                        // in-situ
    }

    this->pos += this->matchlen;
    this->col += this->matchlen;
    this->matchlen = 0;
    Token_detect(this);

    if (this->kind == TKNewline) {
        // WHY don't you do this->token advance here?
        this->line++;
        this->col = 0; // position of the nl itself is 0
    }
    if (this->flags.skipWhiteSpace
        and (this->kind == TKSpaces
            or (this->flags.strictSpacing and this->kind == TKOneSpace)))
        Token_advance(this);
}
//
