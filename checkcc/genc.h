void ASTImport_genc(ASTImport* this, int level)
{
    str_tr_ip(this->importFile, '.', '_', 0);
    printf("\n#include \"%s.h\"\n", this->importFile);
    if (this->hasAlias)
        printf("#define %s %s\n", this->importFile + this->aliasOffset,
            this->importFile);
    str_tr_ip(this->importFile, '_', '.', 0);
}

void ASTImport_undefc(ASTImport* this)
{
    if (this->hasAlias)
        printf("#undef %s\n", this->importFile + this->aliasOffset);
}

void ASTTypeSpec_genc(ASTTypeSpec* this, int level, bool isconst)
{
    if (isconst) printf("const ");
    if (this->dims) {
        if (this->dims > 1)
            printf("SArray%dD(", this->dims);
        else
            printf("SArray(");
    }

    switch (this->typeType) {
    case TYObject:
        printf("%s", this->type->name);
        break;
    case TYUnresolved:
        printf("%s", this->name);
        break;
    default:
        printf("%s", TypeType_nativeName(this->typeType));
        break;
    }

    // if (isconst) printf(" const"); // only if a ptr type
    if (this->dims) printf("%s", ")");
    //        if (status == TSDimensionedNumber) {
    //            genc(units, level);
    //        }
}

void ASTExpr_genc(ASTExpr* this, int level, bool spacing, bool inFuncArgs,
    bool escapeStrings);

void ASTVar_genc(ASTVar* this, int level, bool isconst)
{
    // for C the variable declarations go at the top of the block, without
    // init
    printf("%.*s", level, spaces);
    if (this->typeSpec) {
        ASTTypeSpec_genc(this->typeSpec, level + STEP, isconst);
    } else {
        const char* ctyp = TokenKind_defaultType(
            this->init ? this->init->kind : TKUnknown);
        if (this->init and this->init->kind == TKFunctionCall
            and *this->init->name >= 'A' and *this->init->name <= 'Z')
            ctyp = this->init->name;
        printf("%s", ctyp);
    }
    printf(" %s", this->name);
}

#define genLineNumbers 0
void ASTScope_genc(ASTScope* this, int level)
{
    // foreach (ASTVar*, local, locals, this->locals) {
    //     ASTVar_genc(local, level, false);
    //     puts(";");
    // } // these will be declared at top and defined within the expr list
    foreach (ASTExpr*, stmt, stmts, this->stmts) {
        if (stmt->kind == TKLineComment) continue;
        if (genLineNumbers) printf("#line %d\n", stmt->line);
        ASTExpr_genc(stmt, level, true, false, false);
        puts(";");
        if (ASTExpr_canThrow(stmt))
            puts("    if (_err_ == ERROR_TRACE) goto backtrace;");
    }
}

void ASTType_genc(ASTType* this, int level)
{
    if (not this->body) return;
    printf("#define FIELDS_%s \\\n", this->name);
    foreach (ASTVar*, var, fvars, this->body->locals) {
        if (not var) continue;
        ASTVar_genc(var, level + STEP, true);
        printf("; \\\n");
    }
    printf("\n\nstruct %s {\n", this->name);

    if (this->super) {
        printf("    FIELDS_");
        ASTTypeSpec_genc(this->super, level, false);
        printf("\n");
    }

    printf("    FIELDS_%s\n};\n\n", this->name);
    printf(
        "static const char* %s_name_ = \"%s\";\n", this->name, this->name);
    printf("static %s %s_alloc_() {\n    return _Pool_alloc_(&gPool_, "
           "sizeof(struct %s));\n}\n",
        this->name, this->name, this->name);
    printf("static %s %s_init_(%s this) {\n", this->name, this->name,
        this->name);
    // TODO: rename this->checks to this->exprs or this->body
    foreach (ASTVar*, var, vars, this->body->locals) {
        printf("#define %s this->%s\n", var->name, var->name);
    }
    foreach (ASTExpr*, stmt, stmts, this->body->stmts) {
        if (not stmt or stmt->kind != TKVarAssign or !stmt->var->init)
            continue;
        ASTExpr_genc(stmt, level + STEP, true, false, false);
        puts(";");
    }
    foreach (ASTVar*, var, mvars, this->body->locals) {
        printf("#undef %s \n", var->name);
    }

    printf("    return this;\n}\n");
    printf("%s %s_new_() {\n    return "
           "%s_init_(%s_alloc_());\n}\n",
        this->name, this->name, this->name, this->name);
    printf("#define %s_print_ %s_print__(p, STR(p))\n", this->name,
        this->name);
    printf("%s %s_print__(%s this, const char* name) {\n    printf(\"<%s "
           "'%%s' at %%p>\",name,this);\n}\n",
        this->name, this->name, this->name, this->name);

    puts("");
}

void ASTType_genh(ASTType* this, int level)
{
    if (not this->body) return;
    printf("typedef struct %s* %s; struct %s;\n", this->name, this->name,
        this->name);
}

void ASTFunc_genc(ASTFunc* this, int level)
{
    if (this->flags.isDeclare) return;

    printf("\n// ------------------------ function: %s ", this->name);
    printf("\n// ------------- approx. stack usage per call: %lu B \n",
        ASTFunc_calcSizeUsage(this));
    printf("#define DEFAULT_VALUE %s\n",
        getDefaultValueForType(this->returnType));
    if (not this->flags.isExported) printf("static ");
    if (this->returnType) {
        ASTTypeSpec_genc(this->returnType, level, false);
    } else
        printf("void");
    str_tr_ip(this->name, '.', '_', 0);
    printf(" %s", this->name);
    str_tr_ip(this->name, '_', '.', 0);
    // TODO: here add the type of the first arg, unless it is a method
    // of a type
    if (this->args and this->args->next) { // means at least 2 args
        foreach (ASTVar*, arg, nargs, (this->args->next)) {
            // start from the 2nd
            printf("_%s", arg->name);
        }
    }
    printf("(");
    foreach (ASTVar*, arg, args, this->args) {
        ASTVar_genc(arg, level, true);
        printf(args->next ? ", " : "");
    }

    printf("\n#ifdef DEBUG\n"
           "    %c const char* callsite_ "
           "\n#endif\n",
        ((this->args and this->args->item ? ',' : ' ')));

    // TODO: if (flags.throws) printf("const char** _err_");
    puts(") {");
    printf("#ifdef DEBUG\n"
           "    static const char* sig_ = \"");
    printf("%s%s(", this->flags.isStmt ? "" : "function ", this->name);

    foreach (ASTVar*, arg, chargs, this->args) {
        ASTVar_gen(arg, level);
        printf(chargs->next ? ", " : ")");
    }
    if (this->returnType) {
        printf(" returns ");
        ASTTypeSpec_gen(this->returnType, level);
    }
    puts("\";\n#endif");

    printf("%s",
        "#ifndef NOSTACKCHECK\n"
        "    STACKDEPTH_UP\n"
        "    if (_scStart_ - (char*)&a > _scSize_) {\n"
        "#ifdef DEBUG\n"
        "        _scPrintAbove_ = _scDepth_ - _btLimit_;\n"
        "        printf(\"\\033[31mfatal: stack overflow at call depth "
        "%d.\\n   "
        " in %s\\033[0m\\n\", _scDepth_, sig_);\n"
        "        printf(\"\\033[90mBacktrace (innermost "
        "first):\\n\");\n"
        "        if (_scDepth_ > 2*_btLimit_)\n        "
        "printf(\"    limited to %d outer and %d inner entries.\\n\", "
        "_btLimit_, _btLimit_);\n"
        "        printf(\"[%d] "
        "\\033[36m%s\\n\", _scDepth_, callsite_);\n"
        "#else\n"
        "        printf(\"\\033[31mfatal: stack "
        "overflow.\\033[0m\\n\");\n"
        "#endif\n"
        "        DOBACKTRACE\n    }\n"
        "#endif\n");

    ASTScope_genc(this->body, level + STEP);

    puts("    // ------------ error handling\n"
         "    return DEFAULT_VALUE;\n    assert(0);\n"
         "error:\n"
         "#ifdef DEBUG\n"
         "    eprintf(\"error: %s\\n\",_err_);\n"
         "#endif\n"
         "backtrace:\n"
         "#ifdef DEBUG\n"

         "    if (_scDepth_ <= _btLimit_ || "
         "_scDepth_ > _scPrintAbove_)\n"
         "        printf(\"\\033[90m[%d] \\033[36m"
         "%s\\n\", _scDepth_, callsite_);\n"
         "    else if (_scDepth_ == _scPrintAbove_)\n"
         "        printf(\"\\033[90m... truncated ...\\033[0m\\n\");\n"
         "#endif\n"
         "done:\n"
         "#ifndef NOSTACKCHECK\n"
         "    STACKDEPTH_DOWN\n"
         "#endif\n"
         "    return DEFAULT_VALUE;");
    puts("}\n#undef DEFAULT_VALUE");
}

void ASTFunc_genh(ASTFunc* this, int level)
{
    if (this->flags.isDeclare) return;
    if (not this->flags.isExported) printf("static ");
    if (this->returnType) {
        ASTTypeSpec_genc(this->returnType, level, false);
    } else
        printf("void");
    str_tr_ip(this->name, '.', '_', 0);
    printf(" %s", this->name);
    str_tr_ip(this->name, '_', '.', 0);
    if (this->args and this->args->next) { // means at least 2 args
        foreach (ASTVar*, arg, nargs, this->args->next) {
            printf("_%s", arg->name);
        }
    }
    printf("(");

    foreach (ASTVar*, arg, args, this->args) {
        ASTVar_genc(arg, level, true);
        printf(args->next ? ", " : "");
    }
    printf("\n#ifdef DEBUG\n    %c const char* callsite_ "
           "\n#endif\n",
        ((this->args and this->args->item) ? ',' : ' '));
    puts(");\n");
}

void ASTExpr_genc(ASTExpr* this, int level, bool spacing, bool inFuncArgs,
    bool escapeStrings)
{
    // generally an expr is not split over several lines (but maybe in
    // rare cases). so level is not passed on to recursive calls.

    printf("%.*s", level, spaces);
    switch (this->kind) {
    case TKNumber:
    case TKMultiDotNumber:
        printf("%.*s", this->strLen, this->value.string);
        break;

    case TKString:
        printf(escapeStrings ? "\\%.*s\\\"" : "%.*s\"", this->strLen - 1,
            this->value.string);
        break;

    case TKIdentifier:
    case TKIdentifierResolved:
        // convert a.b.c.d to DEREF3(a,b,c,d), a.b to DEREF(a,b) etc.
        {
            char* tmp = (this->kind == TKIdentifierResolved)
                ? this->var->name
                : this->name;
            int8_t dotCount = 0, i = 0;
            for (i = 0; tmp[i]; i++) {
                if (tmp[i] == '.') {
                    dotCount++;
                    tmp[i] = ',';
                }
            }
            if (dotCount)
                printf("DEREF%d(%s)", dotCount, tmp);
            else
                printf("%s", tmp);

            for (i = 0; tmp[i]; i++)
                if (tmp[i] == ',') tmp[i] = '.';
        }
        break;

    case TKRegex:
        this->value.string[0] = '"';
        this->value.string[this->strLen - 1] = '"';
        printf("%.*s", this->strLen, this->value.string);
        this->value.string[0] = '\'';
        this->value.string[this->strLen - 1] = '\'';
        break;

    case TKInline:
        this->value.string[0] = '"';
        this->value.string[this->strLen - 1] = '"';
        printf("mkRe_(%.*s)", this->strLen, this->value.string);
        this->value.string[0] = '`';
        this->value.string[this->strLen - 1] = '`';
        break;

    case TKLineComment:
        // TODO: skip  comments in generated code
        printf("// %.*s", this->strLen, this->value.string);
        break;

    case TKFunctionCall:
    case TKFunctionCallResolved: {
        char* tmp = (this->kind == TKFunctionCallResolved)
            ? this->func->name
            : this->name;
        str_tr_ip(tmp, '.', '_', 0); // this should have been done in a
                                     // previous stage prepc() or lower()
        printf("%s", tmp);
        if (*tmp >= 'A' and *tmp <= 'Z' and not strchr(tmp, '_'))
            printf("_new_"); // MyType() generates MyType_new_()
                             // TODO: if constructors for MyType are
        // defined, they should
        // generate both a _init_arg1_arg2 function AND a corresponding
        // _new_arg1_arg2 func.
        if (this->left) ASTExpr_catarglabels(this->left);
        str_tr_ip(tmp, '_', '.', 0);
        // this won't be needed, prepc will do the "mangling"
        printf("(");

        if (this->left)
            ASTExpr_genc(this->left, 0, false, true, escapeStrings);

        if (strcmp(tmp, "print")) {
            // more generally this IF is for those funcs that are
            // standard and dont need any instrumentation
            printf("\n#ifdef DEBUG\n"
                   "      %c THISFILE \":%d:\\033[0m\\n     -> ",
                this->left ? ',' : ' ', this->line);
            ASTExpr_gen(this, 0, false, true);
            printf("\"\n"
                   "#endif\n        ");
        }
        printf(")");
        break;
    }

    case TKSubscriptResolved:
    case TKSubscript: // slice here should be slice1D slice2D etc.
    {
        char* tmp = (this->kind == TKSubscriptResolved) ? this->var->name
                                                        : this->name;
        printf("slice(%s, {", tmp);
        if (this->left)
            ASTExpr_genc(this->left, 0, false, inFuncArgs, escapeStrings);
        printf("})");
        break;
    }

    case TKOpAssign:
        if (not inFuncArgs) {
            ASTExpr_genc(this->left, 0, spacing, inFuncArgs, escapeStrings);
            printf("%s", TokenKind_repr(TKOpAssign, spacing));
        }
        ASTExpr_genc(this->right, 0, spacing, inFuncArgs, escapeStrings);
        // check various types of lhs  here, eg arr[9:87] = 0,
        // map["uuyt"]="hello" etc.
        break;

    case TKArrayOpen:
        // TODO: send parent ASTExpr* as an arg to this function. Then
        // here do various things based on whether parent is a =,
        // funcCall, etc.
        printf("mkarr((%s[]) {", "double"); // FIXME
        // TODO: MKARR should be different based on the CollectionType
        // of the var or arg in question, eg stack cArray, heap
        // allocated Array, etc.
        ASTExpr_genc(this->right, 0, spacing, inFuncArgs, escapeStrings);
        printf("}");
        printf(", %d)", ASTExpr_countCommaList(this->right));
        break;

    case TKOpColon: // convert 3:4:5 to range(...)
                    // must do bounds check first!
        printf("%s(",
            this->left->kind != TKOpColon ? "range_to" : "range_to_by");
        if (this->left->kind == TKOpColon) {
            this->left->kind = TKOpComma;
            ASTExpr_genc(this->left, 0, false, inFuncArgs, escapeStrings);
            this->left->kind = TKOpColon;
        } else
            ASTExpr_genc(this->left, 0, false, inFuncArgs, escapeStrings);
        printf(", ");
        ASTExpr_genc(this->right, 0, false, inFuncArgs, escapeStrings);
        printf(")");
        break;

    case TKVarAssign: // basically a TKOpAssign corresponding to a local
                      // var
        // var x as XYZ = abc... -> becomes an ASTVar and an
        // ASTExpr (to keep location). Send it to ASTVar::gen.
        ASTVar_genc(this->var, 0, false);
        if (this->var->init != NULL) {
            printf(" = "); //, this->var->name);
            ASTExpr_genc(
                this->var->init, 0, true, inFuncArgs, escapeStrings);
        }
        break;

    case TKKeyword_for:
    case TKKeyword_if:
    case TKKeyword_while:
        if (this->kind == TKKeyword_for)
            printf("FOR(");
        else
            printf("%s (", TokenKind_repr(this->kind, true));
        if (this->kind == TKKeyword_for) this->left->kind = TKOpComma;
        if (this->left)
            ASTExpr_genc(this->left, 0, spacing, inFuncArgs, escapeStrings);
        if (this->kind == TKKeyword_for) this->left->kind = TKOpAssign;
        puts(") {");
        if (this->body) ASTScope_genc(this->body, level + STEP);
        printf("%.*s}", level, spaces);
        break;

    case TKPower:
        printf("pow(");
        ASTExpr_genc(this->left, 0, false, inFuncArgs, escapeStrings);
        printf(",");
        ASTExpr_genc(this->right, 0, false, inFuncArgs, escapeStrings);
        printf(")");
        break;

    case TKKeyword_return:
        printf("{_err_ = NULL; _scDepth_--; return ");
        ASTExpr_genc(this->right, 0, spacing, inFuncArgs, escapeStrings);
        printf(";}\n");
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

        char lpo = '(';
        char lpc = ')';
        if (leftBr) putc(lpo, stdout);
        if (this->left)
            ASTExpr_genc(this->left, 0,
                spacing and !leftBr and this->kind != TKOpColon, inFuncArgs,
                escapeStrings);
        if (leftBr) putc(lpc, stdout);

        if (this->kind == TKArrayOpen)
            putc('{', stdout);
        else
            printf("%s", TokenKind_repr(this->kind, spacing));

        char rpo = '(';
        char rpc = ')';
        if (rightBr) putc(rpo, stdout);
        if (this->right)
            ASTExpr_genc(this->right, 0,
                spacing and !rightBr and this->kind != TKOpColon,
                inFuncArgs, escapeStrings);
        if (rightBr) putc(rpc, stdout);

        if (this->kind == TKArrayOpen) putc('}', stdout);
    }
}

void ASTModule_genc(ASTModule* this, int level)
{
    foreach (ASTImport*, import, imports, this->imports)
        ASTImport_genc(import, level);

    puts("");

    foreach (ASTType*, type, types, this->types)
        ASTType_genh(type, level);

    foreach (ASTFunc*, func, funcs, this->funcs)
        ASTFunc_genh(func, level);

    foreach (ASTType*, type, mtypes, this->types)
        ASTType_genc(type, level);

    foreach (ASTFunc*, func, mfuncs, this->funcs)
        ASTFunc_genc(func, level);

    foreach (ASTImport*, simport, simports, this->imports)
        ASTImport_undefc(simport);
}
