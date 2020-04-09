void ASTExpr_genlua(
    ASTExpr* this, int level, bool spacing, bool escapeStrings);

void ASTVar_genlua(ASTVar* this, int level)
{
    printf("%.*s%s%s", level, spaces,
        this->flags.isVar or this->flags.isLet ? "local " : "", this->name);
    if (this->init) {
        printf(" = ");
        ASTExpr_genlua(this->init, 0, true, false);
    }
}
void ASTScope_genlua(ASTScope* this, int level)
{
    foreach (ASTExpr*, stmt, stmts, this->stmts) {
        ASTExpr_genlua(stmt, level, true, false);
        puts("");
    }
}
void ASTType_genlua(ASTType* this, int level)
{
    if (not this->body) return;
    printf("--[[\ntype %s", this->name);
    if (this->super) {
        printf(" extends ");
        ASTTypeSpec_gen(this->super, level);
    }
    puts("");

    foreach (ASTExpr*, stmt, stmts, this->body->stmts) {
        if (not stmt) continue;
        ASTExpr_gen(stmt, level + STEP, true, false);
        puts("");
    }
    puts("end type\n]]--\n");
}
void ASTFunc_genlua(ASTFunc* this, int level)
{
    if (this->flags.isDeclare) return;

    printf("function %s(", this->name);

    foreach (ASTVar*, arg, args, this->args) {
        ASTVar_genlua(arg, level);
        printf(args->next ? ", " : "");
    }
    printf(")\n");

    ASTScope_genlua(this->body, level + STEP);

    puts("end\n");
}

void ASTExpr_genlua(
    ASTExpr* this, int level, bool spacing, bool escapeStrings)
{
    // generally an expr is not split over several lines (but maybe in
    // rare cases). so level is not passed on to recursive calls.
    printf("%.*s", level, spaces);

    switch (this->kind) {
    case TKNumber:
    case TKMultiDotNumber:
    case TKRegex:
    case TKInline:
        printf("%.*s", this->strLen, this->value.string);
        break;
    case TKIdentifier:
    case TKIdentifierResolved: {
        char* tmp = (this->kind == TKIdentifierResolved) ? this->var->name
                                                         : this->name;
        printf("%s", tmp);
    } break;

    case TKString:
        printf(escapeStrings ? "\\%.*s\\\"" : "%.*s\"", this->strLen - 1,
            this->value.string);
        break;

    case TKLineComment:
        printf("-- %.*s", this->strLen, this->value.string);
        break;

    case TKFunctionCall:
    case TKFunctionCallResolved: {
        char* tmp = (this->kind == TKFunctionCallResolved)
            ? this->func->name
            : this->name;
        printf("%s(", tmp);
        if (this->left) ASTExpr_genlua(this->left, 0, false, escapeStrings);
        printf(")");
    } break;

    case TKSubscript:
    case TKSubscriptResolved: {
        char* tmp = (this->kind == TKSubscriptResolved) ? this->var->name
                                                        : this->name;
        printf("%s[", tmp);
        if (this->left) ASTExpr_genlua(this->left, 0, false, escapeStrings);
        printf("]");
    } break;

    case TKVarAssign:
        // var x as XYZ = abc... -> becomes an ASTVar and an ASTExpr
        // (to keep location). Send it to ASTVar_gen.
        assert(this->var != NULL);
        ASTVar_genlua(this->var, 0);
        break;

    case TKKeyword_for:
    case TKKeyword_if:
    case TKKeyword_while:
        printf("%s ", TokenKind_repr(this->kind, false));
        if (this->left) ASTExpr_genlua(this->left, 0, true, escapeStrings);
        puts("");
        if (this->body) ASTScope_genlua(this->body, level + STEP);
        printf("%.*send", level, spaces);
        break;

    default:
        if (not this->opPrec) break;
        // not an operator, but this should be error if you reach here
        bool leftBr = this->left and this->left->opPrec
            and this->left->opPrec < this->opPrec;
        bool rightBr = this->right and this->right->opPrec
            and this->right->kind
                != TKKeyword_return // found in 'or return'
            and this->right->opPrec < this->opPrec;

        char lpo = leftBr and this->left->kind == TKOpColon ? '{' : '(';
        char lpc = leftBr and this->left->kind == TKOpColon ? '}' : ')';
        if (leftBr) putc(lpo, stdout);
        if (this->left)
            ASTExpr_genlua(this->left, 0,
                spacing and !leftBr and this->kind != TKOpColon,
                escapeStrings);
        if (leftBr) putc(lpc, stdout);

        printf("%s",
            this->kind == TKArrayOpen
                ? "{"
                : TokenKind_repr(this->kind, spacing));

        char rpo = rightBr and this->right->kind == TKOpColon ? '{' : '(';
        char rpc = rightBr and this->right->kind == TKOpColon ? '}' : ')';
        if (rightBr) putc(rpo, stdout);
        if (this->right)
            ASTExpr_genlua(this->right, 0,
                spacing and !rightBr and this->kind != TKOpColon,
                escapeStrings);
        if (rightBr) putc(rpc, stdout);

        if (this->kind == TKArrayOpen) putc('}', stdout);
    }
}
void ASTModule_genlua(ASTModule* this, int level)
{
    printf("-- module %s\n", this->name);

    // foreach (ASTImport*, import, imports, this->imports)
    //     ASTImport_gen(import, level);

    // puts("");

    foreach (ASTType*, type, types, this->types)
        ASTType_genlua(type, level);

    foreach (ASTFunc*, func, funcs, this->funcs)
        ASTFunc_genlua(func, level);

    puts("start()");
}
