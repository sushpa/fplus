//=============================================================================
// TOKEN
//=============================================================================

// Various kinds of tokens.
typedef enum token_kind_e {
    token_kind__source,
    token_kind__end,
    token_kind_kw_cheater,
    token_kind_kw_for,
    token_kind_kw_while,
    token_kind_kw_if,
    token_kind_kw_end,
    token_kind_kw_function,
    token_kind_kw_test,
    token_kind_kw_not,
    token_kind_kw_and,
    token_kind_kw_or,
    token_kind_kw_in,
    token_kind_kw_do,
    token_kind_kw_then,
    token_kind_kw_as,
    token_kind_kw_else,
    token_kind_kw_type,
    token_kind_kw_template,
    token_kind_kw_base,
    token_kind_kw_var,
    token_kind_kw_let,
    token_kind_kw_import,
    token_kind_kw_only,
    token_kind_ident,
    token_kind_number,
    token_kind_ws,
    token_kind_spc,
    token_kind_onespc, // exactly 1 space
    token_kind_tab,
    token_kind_nl,
    token_kind_linecomment,
    token_kind_alpha,
    token_kind_amp,
    token_kind_array_close, // ]
    token_kind_array_empty, // []
    token_kind_array_open, //  [
    token_kind_array_dims, // [:,:,:]
    token_kind_at,
    token_kind_brace_close,
    token_kind_brace_empty,
    token_kind_brace_open,
    token_kind_digit,
    token_kind_hash,
    token_kind_excl,
    token_kind_pipe,
    token_kind_op_asn,
    token_kind_op_eq,
    token_kind_op_ne,
    token_kind_op_ge,
    token_kind_op_gt,
    token_kind_op_le,
    token_kind_op_lsh,
    token_kind_op_lt,
    token_kind_op_mod,
    token_kind_op_rsh,
    token_kind_op_results,
    token_kind_op_notresults,
    token_kind_paren_close,
    token_kind_paren_empty,
    token_kind_paren_open,
    token_kind_period,
    token_kind_comma,
    token_kind_op_colon,
    token_kind_str_boundary, // "
    token_kind_str_empty, // ""
    token_kind_str, // "string"
    token_kind_rgx_boundary, // '
    token_kind_rgx_empty, // ''
    token_kind_rgx, // '[a-zA-Z0-9]+'
    token_kind_inl_boundary,
    token_kind_inl_empty,
    token_kind_inl,
    token_kind_underscore,
    token_kind_slash,
    token_kind_backslash,
    token_kind_plus,
    token_kind_minus,
    token_kind_times,
    token_kind_pow,
    token_kind_usd,
    token_kind_units,
    token_kind_unknown,
    token_kind_pluseq,
    token_kind_minuseq,
    token_kind_slasheq,
    token_kind_timeseq,
    token_kind_coleq
} token_kind_e;

// Holds information about a syntax token.
typedef struct token_t {
    char* pos;
    uint32_t matchlen : 24;
    struct {
        bool_t skipws : 1, // skip whitespace
        mergearraydims : 1; // merge [:,:,:] into one token
    } flags;
    uint16_t line;
    uint8_t col;
    token_kind_e kind : 8;
} token_t;

// Return the repr of a token kind (for debug)
const char* token_repr(const token_kind_e kind)
{
    switch (kind) {
        case token_kind__source:
            return "(src)";
        case token_kind__end:
            return "(EOF)";
        case token_kind_kw_cheater:
            return "cheater";
        case token_kind_kw_for:
            return "for";
        case token_kind_kw_while:
            return "while";
        case token_kind_kw_if:
            return "if";
        case token_kind_kw_then:
            return "then";
        case token_kind_kw_as:
            return "as";
        case token_kind_kw_end:
            return "end";
        case token_kind_kw_function:
            return "func";
        case token_kind_kw_test:
            return "test";
        case token_kind_kw_not:
            return "not";
        case token_kind_kw_and:
            return "and";
        case token_kind_kw_or:
            return "or";
        case token_kind_kw_in:
            return "in";
        case token_kind_kw_else:
            return "else";
        case token_kind_kw_type:
            return "type";
        case token_kind_kw_template:
            return "template";
        case token_kind_kw_base:
            return "base";
        case token_kind_kw_var:
            return "var";
        case token_kind_kw_let:
            return "let";
        case token_kind_kw_import:
            return "import";
        case token_kind_kw_only:
            return "only";
        case token_kind_ident:
            return "(id)";
        case token_kind_number:
            return "(num)";
        case token_kind_ws:
            return "(ws)";
        case token_kind_spc:
            return "(spc)";
        case token_kind_tab:
            return "(tab)";
        case token_kind_nl:
            return "(nl)";
        case token_kind_linecomment:
            return "(cmt)";
        case token_kind_amp:
            return "&";
        case token_kind_digit:
            return "1";
        case token_kind_pow:
            return "^";
        case token_kind_units:
            return "|kg";
        case token_kind_alpha:
            return "a";
        case token_kind_array_close:
            return "]";
        case token_kind_array_empty:
            return "[]";
        case token_kind_array_open:
            return "[";
        case token_kind_array_dims:
            return "[:,:]";
        case token_kind_at:
            return "@";
        case token_kind_brace_close:
            return "}";
        case token_kind_brace_empty:
            return "{}";
        case token_kind_brace_open:
            return "{";
        case token_kind_hash:
            return "#";
        case token_kind_excl:
            return "!";
        case token_kind_pipe:
            return "!";
        case token_kind_op_asn:
            return "=";
        case token_kind_op_eq:
            return "==";
        case token_kind_op_ne:
            return "=/";
        case token_kind_op_ge:
            return ">=";
        case token_kind_op_gt:
            return ">";
        case token_kind_op_le:
            return "<";
        case token_kind_op_lsh:
            return "<<";
        case token_kind_op_lt:
            return "<";
        case token_kind_op_mod:
            return "%";
        case token_kind_op_rsh:
            return ">>";
        case token_kind_op_results:
            return "=>";
        case token_kind_op_notresults:
            return "=/>";
        case token_kind_paren_close:
            return ")";
        case token_kind_paren_empty:
            return "()";
        case token_kind_paren_open:
            return "(";
        case token_kind_period:
            return ".";
        case token_kind_comma:
            return ",";
        case token_kind_op_colon:
            return ":";
        case token_kind_str_boundary:
            return "\"";
        case token_kind_str_empty:
            return "\"\"";
        case token_kind_str:
            return "(str)";
        case token_kind_rgx_boundary:
            return "'";
        case token_kind_rgx_empty:
            return "''";
        case token_kind_rgx:
            return "(rgx)";
        case token_kind_inl_boundary:
            return "`";
        case token_kind_inl_empty:
            return "``";
        case token_kind_inl:
            return "(inl)";
        case token_kind_underscore:
            return "_";
        case token_kind_slash:
            return "/";
        case token_kind_backslash:
            return "\\";
        case token_kind_plus:
            return "+";
        case token_kind_minus:
            return "-";
        case token_kind_times:
            return "*";
        case token_kind_usd:
            return "$";
        case token_kind_unknown:
            return "(unk)";
        case token_kind_kw_do:
            return "do";
        case token_kind_coleq:
            return ":=";
        case token_kind_pluseq:
            return "+=";
        case token_kind_minuseq:
            return "-=";
        case token_kind_timeseq:
            return "*=";
        case token_kind_slasheq:
            return "/=";
        case token_kind_onespc:
            return "sp1";
    }
    printf("unknown kind: %d\n", kind);
    return "(!unk)";
}

bool_t token_unary(token_kind_e kind)
{
    return kind == token_kind_kw_not; // unary - is handled separately
}

bool_t token_rassoc(token_kind_e kind)
{
    return kind == token_kind_period || kind == token_kind_pow;
}

uint8_t token_prec(token_kind_e kind)
{ // if templateStr then precedence of < and > should be 0
    switch (kind) {
        case token_kind_period:
            return 90;
        case token_kind_pipe:
            return 80;
        case token_kind_pow:
            return 70;
        case token_kind_times:
        case token_kind_slash:
            return 60;
        case token_kind_plus:
        case token_kind_minus:
            return 50;
        case token_kind_op_colon:
            return 45;
        case token_kind_op_le:
        case token_kind_op_lt:
        case token_kind_op_gt:
        case token_kind_op_ge:
        case token_kind_kw_in:
            // case token_kind_kw_notin:
            return 41;
        case token_kind_op_eq:
        case token_kind_op_ne:
            return 40;
        case token_kind_kw_not:
            return 32;
        case token_kind_kw_and:
            return 31;
        case token_kind_kw_or:
            return 30;
        case token_kind_op_asn:
            return 22;
        case token_kind_pluseq:
        case token_kind_coleq:
        case token_kind_minuseq:
        case token_kind_timeseq:
        case token_kind_slasheq:
            return 20;
        case token_kind_comma:
            return 10;
        case token_kind_kw_do:
            return 5;
        case token_kind_paren_open:
        case token_kind_paren_close:
            return 0;
        case token_kind_brace_open:
        case token_kind_brace_close:
            return 0;
        case token_kind_array_open:
        case token_kind_array_close:
            return 0;
        default:
            return 255;
    }
}

char* token_strdup(const token_t* const token)
{
    return strndup(token->pos, token->matchlen);
}

// Advance the token position by one char.
void token_advance1(token_t* token)
{
    // this is called after a token has been consumed and the pointer
    // needs to advance. If skipws is set, loop until the next non-space.
    // do {
    token->pos++;
    
    // } while (token->skipws && *(token->pos) == ' ');
}

// Peek at the char after the current (complete) token
char peekcharafter(token_t* token)
{
    return *(token->pos + token->matchlen + 1);
}

#define token_compare_kw(tok) \
if (sizeof(#tok) - 1 == l && !strncmp(#tok, s, l)) \
return token_kind_kw_##tok

// Check if an (ident) token matches a keyword and return its type
// accordingly.
token_kind_e token_trykwmatch(const token_t* token)
{
    if (token->kind != token_kind_ident)
        return token->kind;
    
    const char* s = token->pos;
    const int l = token->matchlen;
    
    token_compare_kw(and);
    token_compare_kw(cheater);
    token_compare_kw(for);
    token_compare_kw(while);
    token_compare_kw(if);
    token_compare_kw(as);
    token_compare_kw(end);
    token_compare_kw(function);
    token_compare_kw(test);
    token_compare_kw(not);
    token_compare_kw(and);
    token_compare_kw(or);
    token_compare_kw(in);
    token_compare_kw(else);
    token_compare_kw(type);
    token_compare_kw(template);
    token_compare_kw(base);
    token_compare_kw(var);
    token_compare_kw(let);
    token_compare_kw(import);
    token_compare_kw(only);
    // token_compare_kw(notin);
    token_compare_kw(do);
    token_compare_kw(then);
    return token_kind_ident;
}

// Get the type based on the char at the current position.
#define token_gettype(t) token_gettype_atoffset(t, 0)

// Get the type based on the char after the current position (don't conflate
// this with char* peekcharafter(token))
#define token_peek_nextchar(t) token_gettype_atoffset(t, 1)

// Get the token kind based only on the char at the current position (or an
// offset).
token_kind_e token_gettype_atoffset(
                                           const token_t* self, const size_t offset)
{
    const char c = self->pos[offset];
    const char cn = c ? self->pos[1 + offset] : 0;
    
    switch (c) {
        case 0:
            return token_kind__end;
        case '\n':
            return token_kind_nl;
        case ' ':
            return token_kind_spc;
        case '\t':
            return token_kind_tab;
        case ':':
            return token_kind_op_colon;
        case ',':
            return token_kind_comma;
        case '"':
            return token_kind_str_boundary;
        case '`':
            return token_kind_inl_boundary;
        case '[':
            switch (cn) {
                    
                default:
                    return token_kind_array_open;
            }
        case '$':
            return token_kind_usd;
        case '%':
            return token_kind_op_mod;
        case '.':
            return token_kind_period;
        case '\'':
            return token_kind_rgx_boundary;
        case '&':
            return token_kind_amp;
        case '@':
            return token_kind_at;
        case '#':
            return token_kind_hash;
        case '|':
            return token_kind_pipe;
        case '{':
            switch (cn) {
                case '}':
                    return token_kind_brace_empty;
                default:
                    return token_kind_brace_open;
            }
        case '(':
            switch (cn) {
                case ')':
                    return token_kind_paren_empty;
                default:
                    return token_kind_paren_open;
            }
        case ')':
            return token_kind_paren_close;
        case '}':
            return token_kind_brace_close;
        case ']':
            return token_kind_array_close;
        case '<':
            switch (cn) {
                case '=':
                    return token_kind_op_le;
                case '<':
                    return token_kind_op_lsh;
                default:
                    return token_kind_op_lt;
            }
        case '>':
            switch (cn) {
                case '=':
                    return token_kind_op_ge;
                case '>':
                    return token_kind_op_rsh;
                default:
                    return token_kind_op_gt;
            }
        case '=':
            switch (cn) {
                case '=':
                    return token_kind_op_eq;
                case '/':
                    return token_kind_op_ne;
                case '>':
                    return token_kind_op_results;
                default:
                    return token_kind_op_asn;
            }
        case '!':
            return token_kind_excl;
        case '/':
            return token_kind_slash;
        case '\\':
            return token_kind_backslash;
        case '+':
            return token_kind_plus;
        case '-':
            return token_kind_minus;
        case '*':
            return token_kind_times;
        default:
            if (isalpha(c) || c == '_') {
                return token_kind_alpha;
            } else if (isdigit(c)) {
                return token_kind_digit;
            } else {
                return token_kind_unknown;
            }
            break;
    }
}

// Scans ahead from the current position until the actual end of the token.
void token_detect(token_t* token)
{
    token_kind_e tt = token_gettype(token);
    token_kind_e tt_ret;// = tt;
    token_kind_e tmp;
    char* start = token->pos;
    bool_t found_e = false, found_dot = false, found_cmt = false;
    uint8_t found_spc = 0;

    switch (tt) {
        case token_kind_str_boundary:
        case token_kind_inl_boundary:
        case token_kind_rgx_boundary:
            tmp = tt; // remember which it is exactly
            
            // Incrementing pos is a side effect of token_gettype(...)
            while (tt != token_kind__end) {
                // here we want to consume the ending " so we move next before
                token_advance1(token);
                tt = token_gettype(token);
                if (tt == token_kind__end || tt == tmp) {
                    token_advance1(token);
                    break;
                }
                if (tt == token_kind_backslash)
                    if (token_peek_nextchar(token) == tmp) { // why if?
                        token_advance1(token);
                    }
            }
            switch (tmp) {
                case token_kind_str_boundary:
                    tt_ret = token_kind_str;
                    break;
                case token_kind_inl_boundary:
                    tt_ret = token_kind_inl;
                    break;
                case token_kind_rgx_boundary:
                    tt_ret = token_kind_rgx;
                    break;
                default:
                    tt_ret = token_kind_unknown;
                    printf("unreachable\n");
            }
            break;
            
        case token_kind_spc:
            while (tt != token_kind__end) {
                // here we dont want to consume the end char, so break before
                tt = token_peek_nextchar(token);
                token_advance1(token);
                if (tt != token_kind_spc)
                    break;
            }
            tt_ret = token_kind_spc;
            break;
            
        case token_kind_comma:
            while (tt != token_kind__end) {
                tt = token_peek_nextchar(token);
                token_advance1(token);
                // line number should be incremented for line continuations
                if (tt == token_kind_spc) {
                    found_spc++;
                }
                if (tt == token_kind_excl) {
                    found_cmt = true;
                }
                if (tt == token_kind_nl) {
                    token->line++;
                    token->col = -found_spc - 1; // account for extra spaces
                                                 // after , and for nl itself
                    found_spc = 0;
                }
                if (found_cmt && tt != token_kind_nl) {
                    found_spc++;
                    continue;
                }
                if (tt != token_kind_spc && tt != token_kind_nl)
                    break;
            }
            tt_ret = token_kind_comma;
            break;
            
        case token_kind_array_open:
            // mergearraydims should be set only when reading func args
            if (!token->flags.mergearraydims)
                goto defaultToken;
            
            while (tt != token_kind__end) {
                tt = token_peek_nextchar(token);
                token_advance1(token);
                if (tt != token_kind_op_colon && tt != token_kind_comma)
                    break;
            }
            tt = token_gettype(token);
            if (tt != token_kind_array_close) {
                fprintf(stderr, "expected a ']', found a '%c'. now what?\n",
                        *token->pos);
            }
            token_advance1(token);
            tt_ret = token_kind_array_dims;
            break;
            
        case token_kind_alpha:
            while (tt != token_kind__end) {
                tt = token_peek_nextchar(token);
                token_advance1(token);
                if (tt != token_kind_alpha //
                    && tt != token_kind_digit //
                    && tt != token_kind_underscore)
                    break;
            }
            tt_ret = token_kind_ident;
            break;
            
        case token_kind_excl:
            while (tt != token_kind__end) {
                tt = token_peek_nextchar(token);
                token_advance1(token);
                if (tt == token_kind_nl)
                    break;
            }
            tt_ret = token_kind_linecomment;
            break;
            
        case token_kind_pipe:
            while (tt != token_kind__end) {
                tt = token_peek_nextchar(token);
                token_advance1(token);
                if (tt != token_kind_alpha && tt != token_kind_digit
                    && tt != token_kind_slash && tt != token_kind_period)
                    break;
            }
            tt_ret = token_kind_units;
            break;
            
        case token_kind_digit:
            while (tt != token_kind__end) // EOF, basically null char
            {
                tt = token_peek_nextchar(token);
                // numbers such as 1234500.00 are allowed
                // very crude, error-checking is parser's job
                token_advance1(token);
                if (*token->pos == 'e' || *token->pos == 'E' || *token->pos == 'd'
                    || *token->pos == 'D') {
                    found_e = true;
                    continue;
                }
                if (found_e && (*token->pos == '-' || *token->pos == '+')) {
                    found_e = false;
                    continue;
                }
                if (tt == token_kind_period) {
                    found_dot = true;
                    continue;
                }
                if (found_dot && tt == token_kind_period) {
                    fprintf(stderr, "raise error: multiple dots in number\n");
                    break;
                }
                if (tt != token_kind_digit && tt != token_kind_period)
                    break;
            }
            tt_ret = token_kind_number;
            break;

       /* case token_kind_nl:
            token->line++;
            token->col=1;
            break; */
            
        case token_kind_op_eq:
        case token_kind_op_ge:
        case token_kind_op_le:
        case token_kind_op_ne:
        case token_kind_op_results:
        case token_kind_op_notresults: // this is 3-char, is it not?
        case token_kind_brace_empty:
        case token_kind_backslash:
        case token_kind_paren_empty:
            // two-char tokens
            token_advance1(token);
            // don'self break
        default:
        defaultToken:
            tt_ret = tt;
            token_advance1(token);
            break;
    }

    token->matchlen = (uint32_t)(token->pos - start);
    token->pos = start; // rewind. but why! then again advance rewind
                        // advance rewind
    token->kind = tt_ret;
    // keywords have their own token type
    if (token->kind == token_kind_ident) token->kind = token_trykwmatch(token);
    // exactly one space is token_kind_onespc, otherwise token_kind_spc.
    // the compiler needs to check one space frequently in strict mode.
    // FIXME figure it out later
if (token->kind == token_kind_spc && token->matchlen==1) token->kind = token_kind_onespc;
}

// Advances the parser to the next token and skips whitespace if the
// parser's flag `skipws` is set.
 void token_advance(token_t* token)
{
    token->pos += token->matchlen;
    token->col += token->matchlen;
    token->matchlen = 0;
    //token_advance1(token);
    token_detect(token);
    if (token->flags.skipws && token->kind == token_kind_spc)
        token_advance(token);
    if (token->kind == token_kind_nl) {
        token->line++;
        token->col = 0; // position of the nl itself is 0
    }
}
