
static void ASTImport_gen(ASTImport* this, int level)
{
    printf("import %s%s%s%s\n", this->isPackage ? "@" : "",
        this->importFile, this->hasAlias ? " as " : "",
        this->hasAlias ? this->importFile + this->aliasOffset : "");
}

static void ASTTypeSpec_gen(ASTTypeSpec* this, int level)
{
    switch (this->typeType) {
    case TYObject:
        printf("%s", this->type->name);
        break;
    case TYUnresolved:
        printf("%s", this->name);
        break;
    default:
        printf("%s", TypeType_name(this->typeType));
        break;
    }
    if (this->dims) printf("%s", "[]");
}

static void ASTExpr_gen(
    ASTExpr* this, int level, bool spacing, bool escapeStrings);

static void ASTVar_gen(ASTVar* this, int level)
{
    printf("%.*s%s%s", level, spaces,
        this->flags.isVar ? "var " : this->flags.isLet ? "let " : "",
        this->name);
    if (not(this->init and this->init->kind == TKFunctionCall
            and !strcmp(this->init->name, this->typeSpec->name))) {
        printf(" as ");
        ASTTypeSpec_gen(this->typeSpec, level + STEP);
    }
    // }
    // else {
    //     // should make this Expr_defaultType and it does it recursively
    //     for
    //     // [, etc
    //     const char* ctyp = TokenKind_defaultType(
    //         this->init ? this->init->kind : TKUnknown);
    //     if (this->init and this->init->kind == TKArrayOpen)
    //         ctyp = TokenKind_defaultType(
    //             this->init->right ? this->init->right->kind : TKUnknown);
    //     if (this->init and this->init->kind == TKFunctionCall
    //         and *this->init->name >= 'A' and *this->init->name <= 'Z')
    //         ctyp = NULL;
    //     if (ctyp) printf(" as %s", ctyp);
    // }
    if (this->init) {
        printf(" = ");
        ASTExpr_gen(this->init, 0, true, false);
    }
}

static void ASTScope_gen(ASTScope* this, int level)
{
    foreach (ASTExpr*, stmt, stmts, this->stmts) {
        ASTExpr_gen(stmt, level, true, false);
        puts("");
    }
}

static void ASTType_gen(ASTType* this, int level)
{
    if (not this->body) printf("declare ");
    printf("type %s", this->name);
    if (this->super) {
        printf(" extends ");
        ASTTypeSpec_gen(this->super, level);
    }
    puts("");
    if (not this->body) return;

    foreach (ASTExpr*, stmt, stmts, this->body->stmts) {
        if (not stmt) continue;
        ASTExpr_gen(stmt, level + STEP, true, false);
        puts("");
    }
    puts("end type\n");
}

static void ASTFunc_gen(ASTFunc* this, int level)
{
    if (this->flags.isDeclare) printf("declare ");

    printf("function %s(", this->name);

    foreach (ASTVar*, arg, args, this->args) {
        ASTVar_gen(arg, level);
        printf(args->next ? ", " : "");
    }
    printf(")");

    if (this->returnType) {
        printf(" returns ");
        ASTTypeSpec_gen(this->returnType, level);
    }
    puts("");
    if (this->flags.isDeclare) return;

    ASTScope_gen(this->body, level + STEP);

    puts("end function\n");
}

static void ASTExpr_gen(
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
        printf("%.*s", this->strLen, this->string);
        break;

    case TKIdentifier:
    case TKIdentifierResolved: {
        char* tmp = (this->kind == TKIdentifierResolved) ? this->var->name
                                                         : this->name;
        printf("%s", tmp);
    } break;

    case TKString:
        printf(escapeStrings ? "\\%.*s\\\"" : "%.*s\"", this->strLen - 1,
            this->string);
        break;

    case TKLineComment:
        printf("%s%.*s",
            TokenKind_repr(TKLineComment, *this->string != ' '),
            this->strLen, this->string);
        break;

    case TKFunctionCall:
    case TKFunctionCallResolved: {
        char* tmp = (this->kind == TKFunctionCallResolved)
            ? this->func->name
            : this->name;
        printf("%s(", tmp);
        if (this->left) ASTExpr_gen(this->left, 0, false, escapeStrings);
        printf(")");
    } break;

    case TKSubscript:
    case TKSubscriptResolved: {
        char* tmp = (this->kind == TKSubscriptResolved) ? this->var->name
                                                        : this->name;
        printf("%s[", tmp);
        if (this->left) ASTExpr_gen(this->left, 0, false, escapeStrings);
        printf("]");
    } break;

    case TKVarAssign:
        // var x as XYZ = abc... -> becomes an ASTVar and an ASTExpr
        // (to keep location). Send it to ASTVar_gen.
        assert(this->var != NULL);
        ASTVar_gen(this->var, 0);
        break;

    case TKKeyword_for:
    case TKKeyword_if:
    case TKKeyword_while:
        printf("%s ", TokenKind_repr(this->kind, false));
        if (this->left) ASTExpr_gen(this->left, 0, true, escapeStrings);
        puts("");
        if (this->body)
            ASTScope_gen(
                this->body, level + STEP); //, true, escapeStrings);
        printf(
            "%.*send %s", level, spaces, TokenKind_repr(this->kind, false));
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

        if (this->kind == TKOpColon) {
            // expressions like arr[a:x-3:2] should become
            // arr[a:(x-3):2]
            // or list literals [8, 9, 6, 77, sin(c)]
            if (this->left) switch (this->left->kind) {
                case TKNumber:
                case TKIdentifier:
                case TKString:
                case TKOpColon:
                case TKMultiDotNumber:
                case TKUnaryMinus:
                    break;
                default:
                    leftBr = true;
                }
            if (this->right) switch (this->right->kind) {
                case TKNumber:
                case TKIdentifier:
                case TKString:
                case TKOpColon:
                case TKMultiDotNumber:
                case TKUnaryMinus:
                    break;
                default:
                    rightBr = true;
                }
        }

        //        if (false and this->kind == TKKeyword_return and
        //        this->right) {
        //            switch (this->right->kind) {
        //            case TKString:
        //            case TKNumber:
        //            case TKIdentifier:
        //            case TKFunctionCall:
        //            case TKSubscript:
        //            case TKRegex:
        //            case TKMultiDotNumber:
        //                break;
        //            default:
        //                rightBr = true;
        //                break;
        //            }
        //        }

        if (this->kind == TKPower and not spacing) putc('(', stdout);

        char lpo = leftBr and this->left->kind == TKOpColon ? '[' : '(';
        char lpc = leftBr and this->left->kind == TKOpColon ? ']' : ')';
        if (leftBr) putc(lpo, stdout);
        if (this->left)
            ASTExpr_gen(this->left, 0,
                spacing and !leftBr and this->kind != TKOpColon,
                escapeStrings);
        if (leftBr) putc(lpc, stdout);

        printf("%s", TokenKind_repr(this->kind, spacing));

        char rpo = rightBr and this->right->kind == TKOpColon ? '[' : '(';
        char rpc = rightBr and this->right->kind == TKOpColon ? ']' : ')';
        if (rightBr) putc(rpo, stdout);
        if (this->right)
            ASTExpr_gen(this->right, 0,
                spacing and !rightBr and this->kind != TKOpColon,
                escapeStrings);
        if (rightBr) putc(rpc, stdout);

        if (this->kind == TKPower and not spacing) putc(')', stdout);
        if (this->kind == TKArrayOpen) putc(']', stdout);
    }
}

static void ASTModule_gen(ASTModule* this, int level)
{
    printf("! module %s\n", this->name);

    foreach (ASTImport*, import, imports, this->imports)
        ASTImport_gen(import, level);

    puts("");

    foreach (ASTType*, type, types, this->types)
        ASTType_gen(type, level);

    foreach (ASTFunc*, func, funcs, this->funcs)
        ASTFunc_gen(func, level);
}