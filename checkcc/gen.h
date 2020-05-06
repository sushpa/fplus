
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
    if (not(this->init and this->init->kind == tkFunctionCall
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
    //         this->init ? this->init->kind : tkUnknown);
    //     if (this->init and this->init->kind == tkArrayOpen)
    //         ctyp = TokenKind_defaultType(
    //             this->init->right ? this->init->right->kind : tkUnknown);
    //     if (this->init and this->init->kind == tkFunctionCall
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
    foreach (ASTExpr*, stmt, this->stmts) {
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

    foreach (ASTExpr*, stmt, this->body->stmts) {
        if (not stmt) continue;
        ASTExpr_gen(stmt, level + STEP, true, false);
        puts("");
    }
    puts("end type\n");
}

static void ASTFunc_gen(ASTFunc* this, int level)
{
    if (this->flags.isDeclare) printf("declare ");

    printf("%s%s(", this->flags.isStmt ? "\n" : "function ", this->name);

    foreachn(ASTVar*, arg, args, this->args)
    {
        ASTVar_gen(arg, level);
        printf(args->next ? ", " : "");
    }
    printf(")");

    if (this->returnType and not this->flags.isStmt) {
        printf(" returns ");
        ASTTypeSpec_gen(this->returnType, level);
    }
    if (this->flags.isDeclare) {
        puts("");
        return;
    } else if (not this->flags.isStmt) {
        puts("");
        ASTScope_gen(this->body, level + STEP);
        puts("end function\n");
    } else {
        ASTExpr* def = this->body->stmts->item;
        def = def->right; // its a return expr
        printf(" := ");
        ASTExpr_gen(def, 0, true, false);
        puts("\n");
    }
}

static void ASTExpr_gen(
    ASTExpr* this, int level, bool spacing, bool escapeStrings)
{
    // generally an expr is not split over several lines (but maybe in
    // rare cases). so level is not passed on to recursive calls.
    printf("%.*s", level, spaces);

    switch (this->kind) {
    case tkNumber:
    case tkMultiDotNumber:
        printf("%s", this->string);
        break;
    case tkRegex:
        printf("'%s'", this->string + 1);
        break;
    case tkInline:
        printf("`%s`", this->string + 1);
        break;

    case tkIdentifier:
    case tkIdentifierResolved: {
        char* tmp = (this->kind == tkIdentifierResolved) ? this->var->name
                                                         : this->name;
        printf("%s", tmp);
    } break;

    case tkString:
        printf(escapeStrings ? "\\%s\\\"" : "%s\"", this->string);
        break;

    case tkLineComment:
        printf("%s%s", TokenKind_repr(tkLineComment, *this->string != ' '),
            this->string);
        break;

    case tkFunctionCall:
    case tkFunctionCallResolved: {
        char* tmp = (this->kind == tkFunctionCallResolved)
            ? this->func->name
            : this->name;
        printf("%s(", tmp);
        if (this->left) ASTExpr_gen(this->left, 0, spacing, escapeStrings);
        printf(")");
    } break;

    case tkSubscript:
    case tkSubscriptResolved: {
        char* tmp = (this->kind == tkSubscriptResolved) ? this->var->name
                                                        : this->name;
        printf("%s[", tmp);
        if (this->left) ASTExpr_gen(this->left, 0, false, escapeStrings);
        printf("]");
    } break;

    case tkVarAssign:
        // var x as XYZ = abc... -> becomes an ASTVar and an ASTExpr
        // (to keep location). Send it to ASTVar_gen.
        assert(this->var != NULL);
        ASTVar_gen(this->var, 0);
        break;

    case tkKeyword_for:
    case tkKeyword_if:
    case tkKeyword_while:
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
                != tkKeyword_return // found in 'or return'
            and this->right->opPrec < this->opPrec;

        if (this->kind == tkOpColon) {
            // expressions like arr[a:x-3:2] should become
            // arr[a:(x-3):2]
            // or list literals [8, 9, 6, 77, sin(c)]
            if (this->left) switch (this->left->kind) {
                case tkNumber:
                case tkIdentifier:
                case tkString:
                case tkOpColon:
                case tkMultiDotNumber:
                case tkUnaryMinus:
                    break;
                default:
                    leftBr = true;
                }
            if (this->right) switch (this->right->kind) {
                case tkNumber:
                case tkIdentifier:
                case tkString:
                case tkOpColon:
                case tkMultiDotNumber:
                case tkUnaryMinus:
                    break;
                default:
                    rightBr = true;
                }
        }

        //        if (false and this->kind == tkKeyword_return and
        //        this->right) {
        //            switch (this->right->kind) {
        //            case tkString:
        //            case tkNumber:
        //            case tkIdentifier:
        //            case tkFunctionCall:
        //            case tkSubscript:
        //            case tkRegex:
        //            case tkMultiDotNumber:
        //                break;
        //            default:
        //                rightBr = true;
        //                break;
        //            }
        //        }

        if (this->kind == tkPower and not spacing) putc('(', stdout);

        char lpo = leftBr and this->left->kind == tkOpColon ? '[' : '(';
        char lpc = leftBr and this->left->kind == tkOpColon ? ']' : ')';
        if (leftBr) putc(lpo, stdout);
        if (this->left)
            ASTExpr_gen(this->left, 0,
                spacing and !leftBr and this->kind != tkOpColon,
                escapeStrings);
        if (leftBr) putc(lpc, stdout);

        printf("%s", TokenKind_repr(this->kind, spacing));

        char rpo = rightBr and this->right->kind == tkOpColon ? '[' : '(';
        char rpc = rightBr and this->right->kind == tkOpColon ? ']' : ')';
        if (rightBr) putc(rpo, stdout);
        if (this->right)
            ASTExpr_gen(this->right, 0,
                spacing and !rightBr and this->kind != tkOpColon,
                escapeStrings);
        if (rightBr) putc(rpc, stdout);

        if (this->kind == tkPower and not spacing) putc(')', stdout);
        if (this->kind == tkArrayOpen) putc(']', stdout);
    }
}

static void ASTModule_gen(ASTModule* this, int level)
{
    printf("! module %s\n", this->name);

    foreach (ASTImport*, import, this->imports)
        ASTImport_gen(import, level);

    puts("");

    foreach (ASTType*, type, this->types)
        ASTType_gen(type, level);

    foreach (ASTFunc*, func, this->funcs)
        ASTFunc_gen(func, level);
}
