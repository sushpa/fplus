#define ASTImport_gencpp ASTImport_genc

void ASTExpr_gencpp(ASTExpr* this, int level, bool spacing, bool inFuncArgs,
    bool escapeStrings);

void ASTTypeSpec_gencpp(ASTTypeSpec* this, int level, bool isconst)
{
    ASTTypeSpec_genc(this, level, isconst);
}

void ASTVar_gencpp(ASTVar* this, int level, bool isconst)
{
    ASTVar_genc(this, level, isconst);
}

void ASTScope_gencpp(ASTScope* this, int level)
{
    foreach (ASTVar*, local, locals, this->locals) {
        ASTVar_gencpp(local, level, false);
        puts(";");
    } // these will be declared at top and defined within the expr
    // list
    foreach (ASTExpr*, stmt, stmts, this->stmts) {
        if (stmt->kind == TKLineComment) continue;
        if (genLineNumbers) printf("#line %d\n", stmt->line);
        ASTExpr_gencpp(stmt, level, true, false, false);
        puts(";");
    }
}

void ASTType_gencpp(ASTType* this, int level)
{
    if (not this->body) return;

    printf("\n\nstruct %s ", this->name);

    if (this->super) {
        printf(": public ");
        ASTTypeSpec_gencpp(this->super, level, false);
    }
    printf(" {\n");

    foreach (ASTVar*, var, fvars, this->body->locals) {
        if (not var) continue;
        ASTVar_gencpp(var, level + STEP, false);
        printf("; \\\n");
    }

    // printf("static const char* _typeName_ = \"%s\";\n", this->name);
    printf("    void *operator new(size_t size) { return "
           "gPool.alloc(size); }\n");

    printf("    %s() {\n", this->name);
    // foreach (ASTVar*, var, vars, this->body->locals) {
    //     printf("#define %s this->%s\n", var->name, var->name);
    // }
    foreach (ASTExpr*, stmt, stmts, this->body->stmts) {
        if (not stmt or stmt->kind != TKVarAssign or !stmt->var->init)
            continue;
        ASTExpr_gencpp(stmt, level + STEP + STEP, true, false, false);
        puts(";");
    }
    // foreach (ASTVar*, var, mvars, this->body->locals) {
    //     printf("#undef %s \n", var->name);
    // }
    printf("    }\n");
    printf("};\n");

    // printf("#define _print_(p) _print_(p, STR(p))\n"  ); // put in
    // chstd.hpp
    printf("void _print_(%s* self, const char* name) {\n    printf(\"<%s "
           "'%%s' at %%p>\",name,self);\n}\n",
        this->name, this->name);

    puts("");
}

void ASTFunc_gencpp(ASTFunc* this, int level)
{
    if (this->flags.isDeclare) return;

    printf("\n// ------------------------ function: %s ", this->name);
    printf("\n// ------------- approx. stack usage per call: %lu B \n",
        ASTFunc_calcSizeUsage(this));
    printf("#define DEFAULT_VALUE %s\n",
        getDefaultValueForType(this->returnType));
    if (not this->flags.isExported) printf("static ");
    if (this->returnType) {
        ASTTypeSpec_gencpp(this->returnType, level, false);
    } else
        printf("void");
    str_tr_ip(this->name, '.', '_', 0);
    printf(" %s", this->name);
    str_tr_ip(this->name, '_', '.', 0);
    // TODO: here add the type of the first arg, unless it is a method
    // of a type
    // if (this->args and this->args->next) { // means at least 2 args
    //     foreach (ASTVar*, arg, nargs, (this->args->next)) {
    //         // start from the 2nd
    //         printf("_%s", arg->name);
    //     }
    // }
    printf("(");
    foreach (ASTVar*, arg, args, this->args) {
        ASTVar_gencpp(arg, level, true);
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

    // printf("%s",
    //     "#ifndef NOSTACKCHECK\n"
    //     "    STACKDEPTH_UP\n"
    //     "    if (_scStart_ - (char*)&a > _scSize_) {\n"
    //     "#ifdef DEBUG\n"
    //     "        _scPrintAbove_ = _scDepth_ - _btLimit_;\n"
    //     "        printf(\"\\e[31mfatal: stack overflow at call
    //     depth "
    //     "%d.\\n   "
    //     " in %s\\e[0m\\n\", _scDepth_, sig_);\n"
    //     "        printf(\"\\e[90mBacktrace (innermost "
    //     "first):\\n\");\n"
    //     "        if (_scDepth_ > 2*_btLimit_)\n        "
    //     "printf(\"    limited to %d outer and %d inner entries.\\n\",
    //     "
    //     "_btLimit_, _btLimit_);\n"
    //     "        printf(\"[%d] "
    //     "\\e[36m%s\\n\", _scDepth_, callsite_);\n"
    //     "#else\n"
    //     "        printf(\"\\e[31mfatal: stack "
    //     "overflow.\\e[0m\\n\");\n"
    //     "#endif\n"
    //     "        DOBACKTRACE\n    }\n"
    //     "#endif\n");

    ASTScope_gencpp(this->body, level + STEP);

    // puts("    // ------------ error handling\n"
    //      "    return DEFAULT_VALUE;\n    assert(0);\n"
    //      "error:\n"
    //      "#ifdef DEBUG\n"
    //      "    eprintf(\"error: %s\\n\",_err_);\n"
    //      "#endif\n"
    //      "backtrace:\n"
    //      "#ifdef DEBUG\n"

    //      "    if (_scDepth_ <= _btLimit_ || "
    //      "_scDepth_ > _scPrintAbove_)\n"
    //      "        printf(\"\\e[90m[%d] \\e[36m"
    //      "%s\\n\", _scDepth_, callsite_);\n"
    //      "    else if (_scDepth_ == _scPrintAbove_)\n"
    //      "        printf(\"\\e[90m... truncated
    //      ...\\e[0m\\n\");\n"
    //      "#endif\n"
    //      "done:\n"
    //      "#ifndef NOSTACKCHECK\n"
    //      "    STACKDEPTH_DOWN\n"
    //      "#endif\n"
    puts("    return DEFAULT_VALUE;");
    puts("}\n#undef DEFAULT_VALUE");
}

void ASTExpr_gencpp(ASTExpr* this, int level, bool spacing, bool inFuncArgs,
    bool escapeStrings)
{
    // generally an expr is not split over several lines (but maybe in
    // rare cases). so level is not passed on to recursive calls.

    printf("%.*s", level, spaces);
    switch (this->kind) {
    case TKNumber:
    case TKMultiDotNumber:
        printf("%s", this->string);
        break;

    case TKString:
        printf(escapeStrings ? "\\%s\\\"" : "%s\"", this->string);
        break;

    case TKIdentifier:
    case TKIdentifierResolved:
        // convert a.b.c.d to DEREF3(a,b,c,d), a.b to DEREF(a,b) etc.
        {
            char* tmp = (this->kind == TKIdentifierResolved)
                ? this->var->name
                : this->name;
            // int8_t dotCount = 0, i = 0;
            // for (i = 0; tmp[i]; i++) {
            //     if (tmp[i] == '.') {
            //         dotCount++;
            //         tmp[i] = ',';
            //     }
            // }
            // if (dotCount)
            //     printf("DEREF%d(%s)", dotCount, tmp);
            // else
            printf("%s", tmp);

            // for (i = 0; tmp[i]; i++)
            //     if (tmp[i] == ',') tmp[i] = '.';
        }
        break;

    case TKRegex:
        //        this->string[0] = '"';
        //        this->string[this->strLen - 1] = '"';
        printf("\"%s\"", this->string + 1);
        //        this->string[0] = '\'';
        //        this->string[this->strLen - 1] = '\'';
        break;

    case TKInline:
        //        this->string[0] = '"';
        //        this->string[this->strLen - 1] = '"';
        printf("mkRe_(\"%s\")", this->string + 1);
        //        this->string[0] = '`';
        //        this->string[this->strLen - 1] = '`';
        break;

    case TKLineComment:
        // TODO: skip  comments in generated code
        printf("// %s", this->string);
        break;

    case TKFunctionCall:
    case TKFunctionCallResolved: {
        char* tmp = (this->kind == TKFunctionCallResolved)
            ? this->func->name
            : this->name;
        // str_tr_ip(tmp, '.', '_', 0); // this should have been done in
        // a previous stage prepc() or lower()
        if (*tmp >= 'A' and *tmp <= 'Z' and not strchr(tmp, '_'))
            printf("new "); // MyType() generates MyType_new_()
        printf("%s", tmp);

        // TODO: if constructors for MyType are
        // defined, they should
        // generate both a _init_arg1_arg2 function AND a corresponding
        // _new_arg1_arg2 func.
        // if (this->left) ASTExpr_catarglabels(this->left);
        // str_tr_ip(tmp, '_', '.', 0);
        // this won't be needed, prepc will do the "mangling"
        printf("(");

        if (this->left)
            ASTExpr_gencpp(this->left, 0, false, true, escapeStrings);

        if (strcmp(tmp, "print")) {
            // more generally this IF is for those funcs that are
            // standard and dont need any instrumentation
            printf("\n#ifdef DEBUG\n"
                   "      %c THISFILE \":%d:\\e[0m\\n     -> ",
                this->left ? ',' : ' ', this->line);
            ASTExpr_gen(this, 0, false, true);
            printf("\"\n#endif\n        ");
        }
        printf(")");
        break;
    }

    case TKSubscriptResolved:
    case TKSubscript:
        // here should be slice1D slice2D etc.
        {
            char* tmp = (this->kind == TKSubscriptResolved)
                ? this->var->name
                : this->name;
            printf("slice(%s, {", tmp);
            if (this->left)
                ASTExpr_gencpp(
                    this->left, 0, false, inFuncArgs, escapeStrings);
            printf("})");
            break;
        }

    case TKOpAssign:
        if (not inFuncArgs) {
            ASTExpr_gencpp(
                this->left, 0, spacing, inFuncArgs, escapeStrings);
            printf("%s", TokenKind_repr(TKOpAssign, spacing));
        }
        ASTExpr_gencpp(this->right, 0, spacing, inFuncArgs, escapeStrings);
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
        ASTExpr_gencpp(this->right, 0, spacing, inFuncArgs, escapeStrings);
        printf("}");
        printf(", %d)", ASTExpr_countCommaList(this->right));
        break;

    case TKOpColon: // convert 3:4:5 to range(...)
                    // must do bounds check first!
        printf("%s(",
            this->left->kind != TKOpColon ? "range_to" : "range_to_by");
        if (this->left->kind == TKOpColon) {
            this->left->kind = TKOpComma;
            ASTExpr_gencpp(this->left, 0, false, inFuncArgs, escapeStrings);
            this->left->kind = TKOpColon;
        } else
            ASTExpr_gencpp(this->left, 0, false, inFuncArgs, escapeStrings);
        printf(", ");
        ASTExpr_gencpp(this->right, 0, false, inFuncArgs, escapeStrings);
        printf(")");
        break;

    case TKVarAssign: // basically a TKOpAssign corresponding to a local
                      // var
        // var x as XYZ = abc... -> becomes an ASTVar and an
        // ASTExpr (to keep location). Send it to ASTVar::gen.
        // ASTVar_gencpp(this->var, 0, false);

        if (this->var->init != NULL) {
            printf("%s = ", this->var->name);
            ASTExpr_gencpp(
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
            ASTExpr_gencpp(
                this->left, 0, spacing, inFuncArgs, escapeStrings);
        if (this->kind == TKKeyword_for) this->left->kind = TKOpAssign;
        puts(") {");
        if (this->body) ASTScope_gencpp(this->body, level + STEP);
        printf("%.*s}", level, spaces);
        break;

    case TKPower:
        printf("pow(");
        ASTExpr_gencpp(this->left, 0, false, inFuncArgs, escapeStrings);
        printf(",");
        ASTExpr_gencpp(this->right, 0, false, inFuncArgs, escapeStrings);
        printf(")");
        break;

    case TKKeyword_return:
        printf("{_err_ = NULL; _scDepth_--; return ");
        ASTExpr_gencpp(this->right, 0, spacing, inFuncArgs, escapeStrings);
        printf(";}\n");
        break;

    default:
        if (not this->prec) break;
        // not an operator, but this should be error if you reach here
        bool leftBr = this->left and this->left->prec
            and this->left->prec < this->prec;
        bool rightBr = this->right and this->right->prec
            and this->right->kind
                != TKKeyword_return // found in 'or return'
            and this->right->prec < this->prec;

        char lpo = '(';
        char lpc = ')';
        if (leftBr) putc(lpo, stdout);
        if (this->left)
            ASTExpr_gencpp(this->left, 0,
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
            ASTExpr_gencpp(this->right, 0,
                spacing and !rightBr and this->kind != TKOpColon,
                inFuncArgs, escapeStrings);
        if (rightBr) putc(rpc, stdout);

        if (this->kind == TKArrayOpen) putc('}', stdout);
    }
}

void ASTModule_gencpp(ASTModule* this, int level)
{
    foreach (ASTImport*, import, imports, this->imports)
        ASTImport_gencpp(import, level);

    puts("");

    // foreach (ASTType*, type, types, this->types)
    //     ASTType_genhpp(type, level);

    // foreach (ASTFunc*, func, funcs, this->funcs)
    //     ASTFunc_genh(func, level);

    foreach (ASTType*, type, mtypes, this->types)
        ASTType_gencpp(type, level);

    foreach (ASTFunc*, func, mfuncs, this->funcs)
        ASTFunc_gencpp(func, level);

    foreach (ASTImport*, simport, simports, this->imports)
        ASTImport_undefc(simport);
}
