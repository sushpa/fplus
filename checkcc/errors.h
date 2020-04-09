#define RELF(s) (*s == '/' ? "" : "./"), s

#define fatal(str, ...)                                                    \
    {                                                                      \
        eprintf(str, __VA_ARGS__);                                         \
        exit(1);                                                           \
    }

void Parser_errorIncrement(Parser* this)
{
    if (++this->errCount >= this->errLimit)
        fatal("too many errors (%d), quitting\n", this->errLimit);
}

void Parser_errorExpectedToken(Parser* this, TokenKind expected)
{
    eprintf("(%d) \033[31merror:\033[0m at %s%s:%d:%d\n"
            "      expected '%s' found '%s'\n",
        this->errCount + 1, RELF(this->filename), this->token.line,
        this->token.col, TokenKind_repr(expected, false),
        TokenKind_repr(this->token.kind, false));
    Parser_errorIncrement(this);
}

void Parser_errorParsingExpr(Parser* this)
{
    // fputs(dashes, stderr);
    eprintf("(%d) \033[31merror:\033[0m at %s%s:%d/%d\n"
            "      failed to parse expr",
        this->errCount + 1, RELF(this->filename), this->token.line - 1,
        this->token.line);
    // parseExpr will move to next line IF there was no hanging comment
    Parser_errorIncrement(this);
}

void Parser_errorInvalidIdent(Parser* this)
{
    eprintf("(%d) \033[31merror:\033[0m invalid name '%.*s' at "
            "%s%s:%d:%d\n",
        this->errCount + 1, this->token.matchlen, this->token.pos,
        RELF(this->filename), this->token.line, this->token.col);
    Parser_errorIncrement(this);
}

void Parser_errorInvalidTypeMember(Parser* this)
{
    eprintf("(%d) \033[31merror:\033[0m invalid member at %s%s:%d\n",
        this->errCount + 1, RELF(this->filename), this->token.line - 1);
    Parser_errorIncrement(this);
}

void Parser_errorUnrecognizedVar(Parser* this, ASTExpr* expr)
{
    eprintf("(%d) \033[31merror:\033[0m unknown variable "
            "\033[34m%.*s\033[0m at "
            "%s%s:%d:%d\n",
        this->errCount + 1, expr->strLen, expr->value.string,
        RELF(this->filename), expr->line, expr->col);
    Parser_errorIncrement(this);
}

void Parser_errorDuplicateVar(Parser* this, ASTVar* var, ASTVar* orig)
{
    eprintf("(%d) \033[31merror:\033[0m duplicate variable "
            "\033[34m%s\033[0m at "
            "%s:%d:%d\n   "
            "          already declared at %s%s:%d:%d\n",
        this->errCount + 1, var->name, this->filename, var->init->line, 9,
        RELF(this->filename), orig->init->line,
        9); // every var has init!! and every var is indented 4 spc ;-)
    Parser_errorIncrement(this);
}

void Parser_errorUnrecognizedFunc(Parser* this, ASTExpr* expr)
{
    eprintf("(%d) \033[31merror:\033[0m unknown function "
            "\033[34m%.*s\033[0m at "
            "%s%s:%d:%d\n",
        this->errCount + 1, expr->strLen, expr->value.string,
        RELF(this->filename), expr->line, expr->col);
    Parser_errorIncrement(this);
}
void Parser_errorArgsCountMismatch(Parser* this, ASTExpr* expr)
{
    assert(expr->kind == TKFunctionCallResolved);
    eprintf("(%d) \033[31merror:\033[0m args count mismatch for "
            "\033[34m%s\033[0m at %s%s:%d:%d\n"
            "          have %d args, need %d, func defined at %s%s:%d\n",
        this->errCount + 1, expr->func->name, RELF(this->filename),
        expr->line, expr->col, ASTExpr_countCommaList(expr->left),
        expr->func->argCount, RELF(this->filename), expr->func->line);
    Parser_errorIncrement(this);
}
void Parser_errorIndexDimsMismatch(Parser* this, ASTExpr* expr)
{
    assert(expr->kind == TKSubscriptResolved);
    eprintf("(%d) \033[31merror:\033[0m index dims mismatch for "
            "\033[34m%s\033[0m at %s%s:%d:%d\n",
        this->errCount + 1, expr->var->name, RELF(this->filename),
        expr->line, expr->col);
    Parser_errorIncrement(this);
}
void Parser_errorMissingInit(Parser* this, ASTExpr* expr)
{
    assert(expr->kind == TKVarAssign);
    eprintf("(%d) \033[31merror:\033[0m missing initializer for "
            "\033[34m%s\033[0m at "
            "%s%s:%d-%d\n",
        this->errCount + 1, expr->var->name, RELF(this->filename),
        expr->line - 1, expr->line);
    Parser_errorIncrement(this);
}

void Parser_errorUnrecognizedType(Parser* this, ASTTypeSpec* typeSpec)
{
    eprintf("(%d) \033[31merror:\033[0m unknown typespec \033[33m%s\033[0m "
            "at %s%s:%d:%d\n",
        this->errCount + 1, typeSpec->name, RELF(this->filename),
        typeSpec->line, typeSpec->col);
    Parser_errorIncrement(this);
}

void Parser_errorUnexpectedToken(Parser* this)
{
    eprintf("(%d) \033[31merror:\033[0m at %s%s:%d:%d\n      unexpected "
            "token "
            "'%.*s'\n",
        this->errCount + 1, RELF(this->filename), this->token.line,
        this->token.col, this->token.matchlen, this->token.pos);
    Parser_errorIncrement(this);
}

void Parser_errorUnexpectedExpr(Parser* this, const ASTExpr* expr)
{
    eprintf("(%d) \033[31merror:\033[0m at %s%s:%d:%d\n"
            "      unexpected expr '%.*s'",
        this->errCount + 1, RELF(this->filename), expr->line, expr->col,
        expr->opPrec ? 100 : expr->strLen,
        expr->opPrec ? TokenKind_repr(expr->kind, false) : expr->name);
    Parser_errorIncrement(this);
}
