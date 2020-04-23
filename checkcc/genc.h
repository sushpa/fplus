#define genLineNumbers 0

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
    // TODO: actually this depends on the collectionType. In general
    // Array is the default, but in other cases it may be SArray, Array64,
    // whatever
    if (this->dims) {
        if (this->dims > 1)
            // TODO: this should be TensorND, without type params?
            // well actually there isn't a TensorND, since its not always
            // double thats in a tensor but can be Complex, Range,
            // Reciprocal, Rational, whatever
            // -- sure, but double (and float) should be enough since
            // the other types are rarely needed in a tensor form
            printf("Array%dD_", this->dims);
        else
            printf("Array_");
    }

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

    // if (isconst) printf(" const"); // only if a ptr type
    if (this->dims) printf("%s", "*");
    //        if (status == TSDimensionedNumber) {
    //            genc(units, level);
    //        }
}

void ASTExpr_genc(ASTExpr* this, int level, bool spacing, bool inFuncArgs,
    bool escStrings);

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

// Functions like Array_any_filter, Array_count_filter etc.
// are macros and don't return a value but may set one. For these
// and other such funcs, the call must be moved to before the
// containing statement, and in place of the original call you
// should place a temporary holding the value that would have been
// "returned".
bool mustPromote(const char* name)
{
    // TODO: at some point these should go into a dict or trie or MPH
    // whatever
    if (not strcmp(name, "Array_any_filter")) return true;
    if (not strcmp(name, "Array_all_filter")) return true;
    if (not strcmp(name, "Array_count_filter")) return true;
    if (not strcmp(name, "Array_write_filter")) return true;
    if (not strcmp(name, "Strs_print_filter")) return true;
    return false;
}

// Promotion scan & promotion happens AFTER resolving functions!
ASTExpr* ASTExpr_promotionCandidate(ASTExpr* this)
{
    assert(this);
    ASTExpr* ret;

    // TODO: make this a switch after all
    // what about func args?
    switch (this->kind) {
    case TKFunctionCallResolved:
        // promote innermost first, so check args
        if ((ret = ASTExpr_promotionCandidate(this->left)))
            return ret;
        else if (mustPromote(this->func->selector))
            return this;
        break;

    case TKSubscriptResolved:
        // TODO: here see if the subscript itself needs to be promoted up
        return ASTExpr_promotionCandidate(this->left);

    case TKSubscript:
        return ASTExpr_promotionCandidate(this->left);

    case TKKeyword_if:
    case TKKeyword_for:
    case TKKeyword_while:
        return ASTExpr_promotionCandidate(this->left);
        // body will be handled by parent scope

    case TKVarAssign:
        if ((ret = ASTExpr_promotionCandidate(this->var->init))) return ret;
        break;

    case TKFunctionCall: // unresolved
        assert(0);
        if ((ret = ASTExpr_promotionCandidate(this->left))) return ret;
        break;

    default:
        if (this->opPrec) {
            if ((ret = ASTExpr_promotionCandidate(this->right))) return ret;
            if (not this->opIsUnary)
                if ((ret = ASTExpr_promotionCandidate(this->left)))
                    return ret;
        }
    }
    return NULL;
}

char* ASTScope_newTmpVarName(ASTScope* this, int num)
{
    char buf[8];
    int l = snprintf(buf, 8, "_%d", num);
    return pstrndup(buf, l);
}

void ASTScope_promoteCandidates(ASTScope* this)
{
    int tmpCount = 0;
    ASTExpr* pc = NULL;
    List(ASTExpr)* prev = NULL; // this->stmts;
    foreach (ASTExpr*, stmt, stmts, this->stmts) {
        // TODO:
        // if (not stmt->flags.mayNeedPromotion) {prev=stmts;continue;}

        if (stmt->kind == TKKeyword_if or stmt->kind == TKKeyword_for
            or stmt->kind == TKKeyword_while
            or stmt->kind == TKKeyword_else and stmt->body) {
            ASTScope_promoteCandidates(stmt->body);
        }

    startloop:

        if (not(pc = ASTExpr_promotionCandidate(stmt))) { // most likely
            prev = stmts;
            continue;
        }
        if (pc == stmt) {
            // possible, less likely: stmt already at toplevel.
            // TODO: in this case, you still have to add the extra arg.
            prev = stmts;
            continue;
        }

        ASTExpr* pcClone = NEW(ASTExpr);
        *pcClone = *pc;

        // 1. add a temp var to the scope
        ASTVar* tmpvar = NEW(ASTVar);
        tmpvar->name = ASTScope_newTmpVarName(this, ++tmpCount);
        tmpvar->typeSpec = NEW(ASTTypeSpec);
        tmpvar->typeSpec->typeType = TYReal64; // FIXME
        // TODO: setup tmpvar->typeSpec
        PtrList_append(&this->locals, tmpvar);

        // 2. change the original to an ident
        pc->kind = TKIdentifierResolved;
        pc->opPrec = 0;
        pc->var = tmpvar;

        // 3. insert the tmp var as an additional argument into the call

        if (not pcClone->left)
            pcClone->left = pc;
        else if (pcClone->left->kind != TKOpComma) {
            // single arg
            ASTExpr* com = NEW(ASTExpr);
            // TODO: really should have an astexpr ctor
            com->opPrec = TokenKind_getPrecedence(TKOpComma);
            com->kind = TKOpComma;
            com->left = pcClone->left;
            com->right = pc;
            pcClone->left = com;
        } else {
            ASTExpr* argn = pcClone->left;
            while (
                argn->kind == TKOpComma and argn->right->kind == TKOpComma)
                argn = argn->right;
            ASTExpr* com = NEW(ASTExpr);
            // TODO: really should have an astexpr ctor
            com->opPrec = TokenKind_getPrecedence(TKOpComma);
            com->kind = TKOpComma;
            com->left = argn->right;
            com->right = pc;
            argn->right = com;
        }

        // 4. insert the promoted expr BEFORE the current stmt
        //        PtrList_append(prev ? &prev : &this->stmts, pcClone);
        //        PtrList* tmp = prev->next;
        // THIS SHOULD BE in PtrList as insertAfter method
        if (!prev) {
            this->stmts = PtrList_with(pcClone);
            this->stmts->next = stmts;
            prev = this->stmts;
        } else {
            prev->next = PtrList_with(pcClone);
            prev->next->next = stmts;
            prev = prev->next;
        } // List(ASTExpr)* insertionPos = prev ? prev->next : this->stmts;
          //  insertionPos
        //  = insertionPos;
        goto startloop; // it will continue if no more promotions are needed

        prev = stmts;
    }
}

void ASTScope_genc(ASTScope* this, int level)
{
    foreach (ASTVar*, local, locals, this->locals) {
        ASTVar_genc(local, level, false);
        puts(";");
    } // these will be declared at top and defined within the expr list
    foreach (ASTExpr*, stmt, stmts, this->stmts) {
        if (stmt->kind == TKLineComment) continue;
        if (genLineNumbers) printf("#line %d\n", stmt->line);
        ASTExpr_genc(stmt, level, true, false, false);
        if (stmt->kind != TKKeyword_else and stmt->kind != TKKeyword_if
            and stmt->kind != TKKeyword_for
            and stmt->kind != TKKeyword_while
            and stmt->kind != TKKeyword_return)
            puts(";");
        else
            puts("");
        // convert this into a flag which is set in the resolution pass
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
    size_t stackUsage = ASTFunc_calcSizeUsage(this);

    printf("\n// ------------------------ function: %s ", this->name);
    printf("\n// ------------- approx. stack usage per call: %lu B \n",
        stackUsage);
    // it seems that actual stack usage is higher due to stack protection,
    // frame bookkeeping whatever and in debug mode callsite needs
    // sizeof(char*) more the magic number 32 may change on 32-bit systems
    // or diff arch
    printf("#ifdef DEBUG\n"
           "#define MYSTACKUSAGE (%lu + 4*sizeof(void*) + sizeof(char*))\n"
           "#else\n"
           "#define MYSTACKUSAGE (%lu + 4*sizeof(void*))\n"
           "#endif\n",
        stackUsage, stackUsage);

    printf("#define DEFAULT_VALUE %s\n",
        getDefaultValueForType(this->returnType));
    if (not this->flags.isExported) printf("static ");
    if (this->returnType) {
        ASTTypeSpec_genc(this->returnType, level, false);
    } else {
        printf("void");
    } // str_tr_ip(this->selector, '.', '_', 0);
    printf(" %s", this->selector);
    // str_tr_ip(this->selector, '_', '.', 0);
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
        "//printf(\"%8lu %8lu\\n\",_scUsage_, _scSize_);\n"
        "    if (_scUsage_ >= _scSize_) {\n"
        "#ifdef DEBUG\n"
        "        _scPrintAbove_ = _scDepth_ - _btLimit_;\n"
        "        printf(\"\\e[31mfatal: stack overflow at call depth "
        "%lu.\\n   "
        " in %s\\e[0m\\n\", _scDepth_, sig_);\n"
        "        printf(\"\\e[90mBacktrace (innermost first):\\n\");\n"
        "        if (_scDepth_ > 2*_btLimit_)\n        "
        "printf(\"    limited to %d outer and %d inner entries.\\n\", "
        "_btLimit_, _btLimit_);\n"
        "        printf(\"[%lu] "
        "\\e[36m%s\\n\", _scDepth_, callsite_);\n"
        "#else\n"
        "        printf(\"\\e[31mfatal: stack "
        "overflow at call depth %lu.\\e[0m\\n\",_scDepth_);\n"
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
         "        printf(\"\\e[90m[%lu] \\e[36m"
         "%s\\n\", _scDepth_, callsite_);\n"
         "    else if (_scDepth_ == _scPrintAbove_)\n"
         "        printf(\"\\e[90m... truncated ...\\e[0m\\n\");\n"
         "#endif\n"
         "done:\n"
         "#ifndef NOSTACKCHECK\n"
         "    STACKDEPTH_DOWN\n"
         "#endif\n"
         "    return DEFAULT_VALUE;");
    puts("}\n#undef DEFAULT_VALUE");
    puts("#undef MYSTACKUSAGE");
}

void ASTFunc_genh(ASTFunc* this, int level)
{
    if (this->flags.isDeclare) return;
    if (not this->flags.isExported) printf("static ");
    if (this->returnType) {
        ASTTypeSpec_genc(this->returnType, level, false);
    } else {
        printf("void");
    } // str_tr_ip(this->name, '.', '_', 0);
    printf(" %s", this->selector);
    // str_tr_ip(this->name, '_', '.', 0);
    // if (this->args and this->args->next) { // means at least 2 args
    //     foreach (ASTVar*, arg, nargs, this->args->next) {
    //         printf("_%s", arg->name);
    //     }
    // }
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
    bool escStrings)
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
        printf(escStrings ? "\\%.*s\\\"" : "%.*s\"", this->strLen - 1,
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
            ? this->func->name // TODO: should be selector
            : this->name;
        ASTExpr* firstArg = this->left; // find first argument

        if (firstArg->kind == TKOpComma) firstArg = firstArg->left;

        // refactor the following into a func, much needed
        if (firstArg) printf("%s_", ASTExpr_typeName(firstArg));

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
        // if (this->left) {
        //     char buf[256];
        //     ASTExpr_strarglabels(this->left, buf, 256);
        //     printf("%s", buf);
        // }
        str_tr_ip(tmp, '_', '.', 0);
        // this won't be needed, prepc will do the "mangling"
        printf("(");

        if (this->left)
            ASTExpr_genc(this->left, 0, false, true, escStrings);

        if (strcmp(tmp, "print")) {
            // more generally this IF is for those funcs that are
            // standard and dont need any instrumentation
            printf("\n#ifdef DEBUG\n"
                   "      %c THISFILE \":%d:\\e[0m\\n     -> ",
                this->left ? ',' : ' ', this->line);
            ASTExpr_gen(this, 0, false, true);
            printf("\"\n"
                   "#endif\n        ");
        }
        printf(")");
        break;
    }

    case TKSubscript: // slice here should be slice1D slice2D etc.
        assert(0);
        break;

    case TKSubscriptResolved: {
        char* name
            = /*(this->kind == TKSubscriptResolved) ?*/ this->var->name;
        //: this->name;
        ASTExpr* index = this->left;
        assert(index);
        switch (index->kind) {
        case TKNumber:
            // indexing with a single number
            printf("Array_get_%s(%s, %s)",
                ASTTypeSpec_name(this->var->typeSpec), name,
                index->value.string);
            break;

        case TKString:
        case TKRegex:
            // indexing with single string
            printf("Dict_get_CString_%s(%s, %s)",
                ASTTypeSpec_name(this->var->typeSpec), name,
                index->value.string);
            break;

        case TKOpComma:
            // higher dimensions. validation etc. has been done by this
            // stage.

            // this is for cases like arr[2, 3, 4].
            printf("Tensor%dD_get_%s(%s, {", this->var->typeSpec->dims,
                ASTTypeSpec_name(this->var->typeSpec), name);
            ASTExpr_genc(index, 0, false, inFuncArgs, escStrings);
            printf("})");

            // TODO: cases like arr[2:3, 4:5, 1:end]

            break;

        case TKOpColon:
            // a single range.
            printf("Array_getSlice_%s(%s, ",
                ASTTypeSpec_name(this->var->typeSpec), name);
            ASTExpr_genc(index, 0, false, inFuncArgs, escStrings);
            printf(")");
            break;
            // what about mixed cases, e.g. arr[2:3, 5, 3:end]
            // make this portion a recursive function then, or promote
            // all indexes to ranges first and then let opcomma handle it

        case TKOpEQ:
        case TKOpLE:
        case TKOpGE:
        case TKOpGT:
        case TKOpLT:
        case TKOpNE:
        case TKKeyword_and:
        case TKKeyword_or:
        case TKKeyword_not:
            // indexing by a logical expression (filter)
            // by default this implies a copy, but certain funcs e.g. print
            // min max sum count etc. can be done in-place without a copy
            // since they are not mutating the array. That requires either
            // the user to call print(arr, filter = arr < 5) instead of
            // print(arr[arr < 5]), or the compiler to transform the second
            // into the first transparently.
            // Probably the TKFunctionCall should check if its argument is
            // a TKSubscript with a logical index, and then tip the user
            // to call the optimised function instead (or just generate it).
            // For now, and in the absence of more context, this is a copy.
            // Array_copy_filter is implemented as a C macro for loop, as
            // are most other filtering-enabled functions on arrays.
            // TODO: be careful with the "template" style call here xx()()
            // TODO: actually I think arr[arr < 5] etc. should just be
            // promoted
            //    and then the generation will follow the modified AST.
            //    Don't handle this as a special case at the code generation
            //    stage.
            printf("Array_copy_filter_%s(%s, ",
                ASTTypeSpec_name(this->var->typeSpec), name);
            ASTExpr_genc(index, 0, false, inFuncArgs, escStrings);
            printf(")");
            break;

        default:
            assert(0);
            //            printf("Tensor%dD_get(CString, %s)(%s,
            //                ASTExpr_genc(this->left, 0, false, inFuncArgs,
            //                escStrings);
            //        printf(")");
            break;
        }
        break;
    }

    case TKOpAssign:
    case TKPlusEq:
    case TKMinusEq:
    case TKTimesEq:
    case TKSlashEq:
    case TKPowerEq:
    case TKOpModEq:

        switch (this->left->kind) {
        case TKSubscriptResolved:
            switch (this->left->left->kind) {
            case TKNumber:
            case TKString:
            case TKRegex:
                // TODO: astexpr_typename should return Array_Scalar or
                // Tensor2D_Scalar or Dict_String_Scalar etc.
                printf("%s_set(%s, %s,%s, ", ASTExpr_typeName(this->left),
                    this->left->var->name, this->left->left->value.string,
                    TokenKind_repr(this->kind, spacing));
                // ASTExpr_genc(this->left->left,0,spacing,inFuncArgs,escStrings);
                // printf(", ");
                ASTExpr_genc(
                    this->right, 0, spacing, inFuncArgs, escStrings);
                printf(")");
                break;

            case TKOpColon:
                printf("%s_setSlice(%s, ", ASTExpr_typeName(this->left),
                    this->left->var->name);
                ASTExpr_genc(
                    this->left->left, 0, spacing, inFuncArgs, escStrings);
                printf(",%s, ", TokenKind_repr(this->kind, spacing));
                ASTExpr_genc(
                    this->right, 0, spacing, inFuncArgs, escStrings);
                printf(")");
                break;

            case TKOpEQ:
            case TKOpGE:
            case TKOpNE:
            case TKOpGT:
            case TKOpLE:
            case TKOpLT:
            case TKKeyword_and:
            case TKKeyword_or:
            case TKKeyword_not:
                printf("%s_setFiltered(%s, ", ASTExpr_typeName(this->left),
                    this->left->var->name);
                ASTExpr_genc(
                    this->left->left, 0, spacing, inFuncArgs, escStrings);
                printf(",%s, ", TokenKind_repr(this->kind, spacing));
                ASTExpr_genc(
                    this->right, 0, spacing, inFuncArgs, escStrings);
                printf(")");
                break;

            case TKOpComma:
            // figure out the type of each element
            // there should be a RangeND just like TensorND and SliceND
            // then you can just pass that to _setSlice
            case TKIdentifierResolved:
                // lookup the var type. note that it need not be scalar,
                // string, range etc. it could be an arbitrary object in
                // case you are indexing a Dict with keys of that type.

            case TKSubscriptResolved:
            // arr[arr2[4]] etc.
            case TKFunctionCallResolved:
            // arr[func(x)]
            default:
                assert(0);
            }
            break;
        case TKIdentifierResolved:
            ASTExpr_genc(this->left, 0, spacing, inFuncArgs, escStrings);
            printf("%s", TokenKind_repr(TKOpAssign, spacing));
            ASTExpr_genc(this->right, 0, spacing, inFuncArgs, escStrings);
            break;
        case TKIdentifier:
            assert(inFuncArgs);
            ASTExpr_genc(this->right, 0, spacing, inFuncArgs, escStrings);
            // function call arg label, do not generate ->left
            break;
        default:
            // error: not a valid lvalue
            // TODO: you should at some point e,g, during resolution check
            // for assignments to invalid lvalues and raise an error
            assert(0);
        }
        // if (not inFuncArgs) {
        //     ASTExpr_genc(this->left, 0, spacing, inFuncArgs,
        //     escStrings); printf("%s", TokenKind_repr(TKOpAssign,
        //     spacing));
        // }
        // ASTExpr_genc(this->right, 0, spacing, inFuncArgs, escStrings);
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
        ASTExpr_genc(this->right, 0, spacing, inFuncArgs, escStrings);
        printf("}");
        printf(", %d)", ASTExpr_countCommaList(this->right));
        break;

    case TKOpColon: // convert 3:4:5 to range(...)
                    // must do bounds check first!
        printf("%s(",
            this->left->kind != TKOpColon ? "range_to" : "range_to_by");
        if (this->left->kind == TKOpColon) {
            this->left->kind = TKOpComma;
            ASTExpr_genc(this->left, 0, false, inFuncArgs, escStrings);
            this->left->kind = TKOpColon;
        } else
            ASTExpr_genc(this->left, 0, false, inFuncArgs, escStrings);
        printf(", ");
        ASTExpr_genc(this->right, 0, false, inFuncArgs, escStrings);
        printf(")");
        break;

    case TKVarAssign: // basically a TKOpAssign corresponding to a local
                      // var
        // var x as XYZ = abc... -> becomes an ASTVar and an
        // ASTExpr (to keep location). Send it to ASTVar::gen.
        //        ASTVar_genc(this->var, 0, false);
        if (this->var->init != NULL) {
            printf("%s = ", this->var->name);
            ASTExpr_genc(this->var->init, 0, true, inFuncArgs, escStrings);
        }
        break;

    case TKKeyword_else:
        puts("else {");
        if (this->body) ASTScope_genc(this->body, level + STEP);
        printf("%.*s}", level, spaces);
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
            ASTExpr_genc(this->left, 0, spacing, inFuncArgs, escStrings);
        if (this->kind == TKKeyword_for) this->left->kind = TKOpAssign;
        puts(") {");
        if (this->body) ASTScope_genc(this->body, level + STEP);
        printf("%.*s}", level, spaces);
        break;

    case TKPower:
        printf("pow(");
        ASTExpr_genc(this->left, 0, false, inFuncArgs, escStrings);
        printf(",");
        ASTExpr_genc(this->right, 0, false, inFuncArgs, escStrings);
        printf(")");
        break;

    case TKKeyword_return:
        printf("{_err_ = NULL; \n#ifndef NOSTACKCHECK\n    "
               "STACKDEPTH_DOWN\n#endif\nreturn ");
        ASTExpr_genc(this->right, 0, spacing, inFuncArgs, escStrings);
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
                escStrings);
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
                inFuncArgs, escStrings);
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
