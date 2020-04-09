
void ASTVar::gen(int level)
{
    printf("%.*s%s%s", level, spaces,
        flags.isVar ? "var " : flags.isLet ? "let " : "", name);
    if (typeSpec) {
        if (!(init and init->kind == TKFunctionCall
                and !strcmp(init->name, typeSpec->name))) {
            printf(" as ");
            typeSpec->gen(level + STEP);
        }
    } else {
        // should make this Expr_defaultType and it does it recursively for
        // [, etc
        const char* ctyp = TokenKind_defaultType(init ? init->kind : TKUnknown);
        if (init and init->kind == TKArrayOpen)
            ctyp = TokenKind_defaultType(
                init->right ? init->right->kind : TKUnknown);
        if (init and init->kind == TKFunctionCall and *init->name >= 'A'
            and *init->name <= 'Z')
            ctyp = NULL;
        if (ctyp) printf(" as %s", ctyp);
    }
    if (init) {
        printf(" = ");
        init->gen(0, true, false);
    }
}

void ASTVar::genc(int level, bool isconst)
{
    // for C the variable declarations go at the top of the block, without
    // init
    printf("%.*s", level, spaces);
    if (typeSpec) {
        typeSpec->genc(level + STEP, isconst);
    } else {
        const char* ctyp = TokenKind_defaultType(init ? init->kind : TKUnknown);
        if (init and init->kind == TKFunctionCall and *init->name >= 'A'
            and *init->name <= 'Z')
            ctyp = init->name;
        printf("%s", ctyp);
    }
    printf(" %s", name);
}