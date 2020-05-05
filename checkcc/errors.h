#define RELF(s) (*s == '/' ? "" : "./"), s

#define fatal(str, ...)                                                    \
    {                                                                      \
        eprintf(str, __VA_ARGS__);                                         \
        exit(1);                                                           \
    }

static void Parser_errorIncrement(Parser* const this)
{
    if (++this->errCount >= this->errLimit)
        fatal("\ntoo many errors (%d), quitting\n", this->errLimit);
}

static void Parser_errorExpectedToken(
    Parser* const this, const TokenKind expected)
{
    eprintf("\n(%d) \e[31merror:\e[0m at %s%s:%d:%d\n"
            "      expected '%s' found '%s'\n",
        this->errCount + 1, RELF(this->filename), this->token.line,
        this->token.col, TokenKind_repr(expected, false),
        TokenKind_repr(this->token.kind, false));
    Parser_errorIncrement(this);
}

static void Parser_errorParsingExpr(Parser* const this)
{
    eprintf("\n(%d) \e[31merror:\e[0m at %s%s:%d/%d\n"
            "      failed to parse expr, giving up\n",
        this->errCount + 1, RELF(this->filename), this->token.line - 1,
        this->token.line);
    Parser_errorIncrement(this);
}

static void Parser_errorInvalidIdent(Parser* const this)
{
    eprintf("\n(%d) \e[31merror:\e[0m invalid name '%.*s' at "
            "%s%s:%d:%d\n",
        this->errCount + 1, this->token.matchlen, this->token.pos,
        RELF(this->filename), this->token.line, this->token.col);
    Parser_errorIncrement(this);
}

static void Parser_errorInvalidTypeMember(Parser* const this)
{
    eprintf("\n(%d) \e[31merror:\e[0m invalid member at %s%s:%d\n",
        this->errCount + 1, RELF(this->filename), this->token.line - 1);
    Parser_errorIncrement(this);
}

static void Parser_errorUnrecognizedVar(
    Parser* const this, const ASTExpr* const expr)
{
    eprintf("\n(%d) \e[31merror:\e[0m unknown variable "
            "\e[34m%s\e[0m at %s%s:%d:%d\n",
        this->errCount + 1, expr->string, RELF(this->filename), expr->line,
        expr->col);
    Parser_errorIncrement(this);
}

static void Parser_warnUnusedArg(
    Parser* const this, const ASTVar* const var)
{
    eprintf("\n(%d) \e[33mwarning:\e[0m unused argument "
            "\e[34m%s\e[0m at %s%s:%d:%d\n",
        ++this->warnCount, var->name, RELF(this->filename), var->line,
        var->col);
}

static void Parser_warnUnusedVar(
    Parser* const this, const ASTVar* const var)
{
    eprintf("\n(%d) \e[33mwarning:\e[0m unused variable "
            "\e[34m%s\e[0m at %s%s:%d:%d\n",
        ++this->warnCount, var->name, RELF(this->filename), var->line,
        var->col);
}

static void Parser_errorDuplicateVar(
    Parser* const this, const ASTVar* const var, const ASTVar* const orig)
{
    eprintf("\n(%d) \e[31merror:\e[0m duplicate variable "
            "\e[34m%s\e[0m at %s%s:%d:%d\n   "
            "          already declared at %s%s:%d:%d\n",
        this->errCount + 1, var->name, RELF(this->filename), var->line,
        var->col, RELF(this->filename), orig->line, orig->col);
    Parser_errorIncrement(this);
}

static void Parser_errorUnrecognizedFunc(Parser* const this,
    const ASTExpr* const expr, const char* const selector)
{
    eprintf("\n(%d) \e[31merror:\e[0m can't resolve call to "
            "\e[34m%s\e[0m at %s%s:%d:%d\n"
            "        selector is \e[34m%s\e[0m\n",
        this->errCount + 1, expr->string, RELF(this->filename), expr->line,
        expr->col, selector);
    Parser_errorIncrement(this);
}

static void Parser_errorArgsCountMismatch(
    Parser* const this, const ASTExpr* const expr)
{
    assert(expr->kind == TKFunctionCallResolved);
    eprintf("\n(%d) \e[31merror:\e[0m arg count mismatch for "
            "\e[34m%s\e[0m at %s%s:%d:%d\n"
            "          have %d args, need %d, func defined at %s%s:%d\n",
        this->errCount + 1, expr->func->name, RELF(this->filename),
        expr->line, expr->col, ASTExpr_countCommaList(expr->left),
        expr->func->argCount, RELF(this->filename), expr->func->line);
    Parser_errorIncrement(this);
}

static void Parser_errorIndexDimsMismatch(
    Parser* const this, const ASTExpr* const expr)
{
    assert(expr->kind == TKSubscriptResolved);
    int reqdDims = expr->var->typeSpec->dims;
    if (not reqdDims)
        eprintf("\n(%d) \e[31merror:\e[0m not an array: "
                "\e[34m%s\e[0m at %s%s:%d:%d\n"
                "          indexing a non-array with %d dims, var defined "
                "at %s%s:%d\n",
            this->errCount + 1, expr->var->name, RELF(this->filename),
            expr->line, expr->col, ASTExpr_countCommaList(expr->left),
            RELF(this->filename), expr->var->typeSpec->line);
    else
        eprintf(
            "(%d) \e[31merror:\e[0m index dims mismatch for "
            "\e[34m%s\e[0m at %s%s:%d:%d\n"
            "          have %d indexes, need %d, var defined at %s%s:%d\n",
            this->errCount + 1, expr->var->name, RELF(this->filename),
            expr->line, expr->col, ASTExpr_countCommaList(expr->left),
            reqdDims, RELF(this->filename), expr->var->typeSpec->line);
    Parser_errorIncrement(this);
}

static void Parser_errorMissingInit(
    Parser* const this, const ASTExpr* const expr)
{
    assert(expr->kind == TKVarAssign);
    eprintf("\n(%d) \e[31merror:\e[0m missing initializer for "
            "\e[34m%s\e[0m at %s%s:%d-%d\n",
        this->errCount + 1, expr->var->name, RELF(this->filename),
        expr->line - 1, expr->line);
    Parser_errorIncrement(this);
}

static void Parser_errorUnrecognizedType(
    Parser* const this, const ASTTypeSpec* const typeSpec)
{
    eprintf("\n(%d) \e[31merror:\e[0m unknown typespec \e[33m%s\e[0m "
            "at %s%s:%d:%d\n",
        this->errCount + 1, typeSpec->name, RELF(this->filename),
        typeSpec->line, typeSpec->col);
    Parser_errorIncrement(this);
}

static void Parser_errorTypeMismatchBinOp(
    Parser* const this, const ASTExpr* const expr)
{
    eprintf("\n(%d) \e[31merror:\e[0m type mismatch for operands of '"
            "\e[34m%s\e[0m' at %s%s:%d:%d\n",
        this->errCount + 1, TokenKind_repr(expr->kind, false),
        RELF(this->filename), expr->line, expr->col);
    Parser_errorIncrement(this);
}

static void Parser_errorReadOnlyVar(
    Parser* const this, const ASTExpr* const expr)
{
    eprintf("\n(%d) \e[31merror:\e[0m mutating read-only variable '"
            "\e[34m%s\e[0m' at %s%s:%d:%d\n",
        this->errCount + 1, expr->var->name, RELF(this->filename),
        expr->line, expr->col);
    Parser_errorIncrement(this);
}

static void Parser_errorInvalidTypeForOp(
    Parser* const this, const ASTExpr* const expr)
{
    eprintf("\n(%d) \e[31merror:\e[0m invalid types for operator '"
            "\e[34m%s\e[0m' at %s%s:%d:%d\n",
        this->errCount + 1, TokenKind_repr(expr->kind, false),
        RELF(this->filename), expr->line, expr->col);
    Parser_errorIncrement(this);
}

static void Parser_errorArgTypeMismatch(
    Parser* const this, const ASTExpr* const expr, const ASTVar* const var)
{
    eprintf("\n(%d) \e[31merror:\e[0m type mismatch for argument '"
            "\e[34m%s\e[0m' at %s%s:%d:%d\n",
        this->errCount + 1, var->name, RELF(this->filename), expr->line,
        expr->col);
    Parser_errorIncrement(this);
}

static void Parser_errorUnexpectedToken(Parser* const this)
{
    eprintf("\n(%d) \e[31merror:\e[0m at %s%s:%d:%d\n      unexpected "
            "token '%.*s'\n",
        this->errCount + 1, RELF(this->filename), this->token.line,
        this->token.col, this->token.matchlen, this->token.pos);
    Parser_errorIncrement(this);
}

static void Parser_errorUnexpectedExpr(
    Parser* const this, const ASTExpr* const expr)
{
    eprintf("\n(%d) \e[31merror:\e[0m at %s%s:%d:%d\n"
            "      unexpected expr '%s'",
        this->errCount + 1, RELF(this->filename), expr->line, expr->col,
        expr->opPrec ? TokenKind_repr(expr->kind, false) : expr->name);
    Parser_errorIncrement(this);
}
