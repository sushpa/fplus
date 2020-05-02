#define genLineNumbers 0

static void ASTImport_genc(ASTImport* import, int level)
{
    str_tr_ip(import->importFile, '.', '_', 0);
    printf("\n#include \"%s.h\"\n", import->importFile);
    if (import->hasAlias)
        printf("#define %s %s\n", import->importFile + import->aliasOffset,
            import->importFile);
    str_tr_ip(import->importFile, '_', '.', 0);
}

static void ASTImport_undefc(ASTImport* import)
{
    if (import->hasAlias)
        printf("#undef %s\n", import->importFile + import->aliasOffset);
}

static void ASTTypeSpec_genc(ASTTypeSpec* typeSpec, int level, bool isconst)
{
    if (isconst) printf("const ");
    // TODO: actually this depends on the collectionType. In general
    // Array is the default, but in other cases it may be SArray, Array64,
    // whatever
    if (typeSpec->dims) {
        if (typeSpec->dims > 1)
            // TODO: this should be TensorND, without type params?
            // well actually there isn't a TensorND, since its not always
            // double thats in a tensor but can be Complex, Range,
            // Reciprocal, Rational, whatever
            // -- sure, but double (and float) should be enough since
            // the other types are rarely needed in a tensor form
            printf("Array%dD_", typeSpec->dims);
        else
            printf("Array_");
    }

    switch (typeSpec->typeType) {
    case TYObject:
        printf("%s", typeSpec->type->name);
        break;
    case TYUnresolved:
        printf("%s", typeSpec->name);
        break;
    default:
        printf("%s", TypeType_name(typeSpec->typeType));
        break;
    }

    // if (isconst) printf(" const"); // only if a ptr type
    if (typeSpec->dims) printf("%s", "*");
    //        if (status == TSDimensionedNumber) {
    //            genc(units, level);
    //        }
}

static void ASTExpr_genc(ASTExpr* this, int level, bool spacing,
    bool inFuncArgs, bool escStrings);

static void ASTVar_genc(ASTVar* var, int level, bool isconst)
{
    // for C the variable declarations go at the top of the block, without
    // init
    printf("%.*s", level, spaces);
    if (var->typeSpec) {
        ASTTypeSpec_genc(var->typeSpec, level + STEP, isconst);
    } else {
        const char* ctyp = TokenKind_defaultType(
            var->init ? var->init->kind : TKUnknown);
        if (var->init and var->init->kind == TKFunctionCall
            and *var->init->name >= 'A' and *var->init->name <= 'Z')
            ctyp = var->init->name;
        printf("%s", ctyp);
    }
    printf(" %s", var->name);
}

// Functions like Array_any_filter, Array_count_filter etc.
// are macros and don't return a value but may set one. For these
// and other such funcs, the call must be moved to before the
// containing statement, and in place of the original call you
// should place a temporary holding the value that would have been
// "returned".
static bool mustPromote(const char* name)
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

// given an expr, generate code to print all the resolved vars in it (only
// scalars). for example in f(x + 4) + m + y[5:6], the following should be
// generated
// printf("x = %?\n", x);
// printf("m = %?\n", m);
// checks will print the vars involved in the check expr, if the check
// fails. This routine will be used there.
static void ASTExpr_genPrintVars(ASTExpr* expr, int level)
{
    assert(expr);
    // what about func args?
    switch (expr->kind) {
    case TKIdentifierResolved:
        printf("%.*sprintf(\"|   %s = %s\\n\", %s);\n", level, spaces,
            expr->var->name, TypeType_format(expr->typeType, true),
            expr->var->name);
        // printf("printf(\"%%s:%d:%d: %s = %s\\n\", THISFILE, %s);\n",
        //     expr->line, expr->col, expr->var->name,
        //     TypeType_format(expr->typeType, true), expr->var->name);
        break;
    case TKVarAssign:
        printf("%.*sprintf(\"|   %s = %s\\n\", %s);\n", level, spaces,
            expr->var->name,
            TypeType_format(expr->var->init->typeType, true),
            expr->var->name);
        ASTExpr_genPrintVars(expr->var->init, level);
        break;
    case TKFunctionCallResolved:
    case TKFunctionCall: // shouldnt happen
    case TKSubscriptResolved:
    case TKSubscript:
    case TKKeyword_if:
    case TKKeyword_for:
    case TKKeyword_while:
        ASTExpr_genPrintVars(expr->left, level);
        break;

    default:
        if (expr->opPrec) {
            if (not expr->opIsUnary)
                ASTExpr_genPrintVars(expr->left, level);
            ASTExpr_genPrintVars(expr->right, level);
        }
    }
}

// Promotion scan & promotion happens AFTER resolving functions!
static ASTExpr* ASTExpr_promotionCandidate(ASTExpr* expr)
{
    assert(expr);
    ASTExpr* ret;

    // what about func args?
    switch (expr->kind) {
    case TKFunctionCallResolved:
        // promote innermost first, so check args
        if ((ret = ASTExpr_promotionCandidate(expr->left)))
            return ret;
        else if (mustPromote(expr->func->selector))
            return expr;
        break;

    case TKSubscriptResolved:
        // TODO: here see if the subscript itself needs to be promoted up
        return ASTExpr_promotionCandidate(expr->left);

    case TKSubscript:
        return ASTExpr_promotionCandidate(expr->left);

    case TKKeyword_if:
    case TKKeyword_for:
    case TKKeyword_while:
        return ASTExpr_promotionCandidate(expr->left);
        // body will be handled by parent scope

    case TKVarAssign:
        if ((ret = ASTExpr_promotionCandidate(expr->var->init))) return ret;
        break;

    case TKFunctionCall: // unresolved
        assert(0);
        if ((ret = ASTExpr_promotionCandidate(expr->left))) return ret;
        break;

    default:
        if (expr->opPrec) {
            if ((ret = ASTExpr_promotionCandidate(expr->right))) return ret;
            if (not expr->opIsUnary)
                if ((ret = ASTExpr_promotionCandidate(expr->left)))
                    return ret;
        }
    }
    return NULL;
}

static char* newTmpVarName(int num)
{
    char buf[8];
    int l = snprintf(buf, 8, "_%d", num);
    return pstrndup(buf, l);
}
static void ASTScope_lowerElementalOps(ASTScope* scope)
{
    //    int tmpCount = 0;
    foreach (ASTExpr*, stmt, stmts, scope->stmts) {

        if (stmt->kind == TKKeyword_if //
            or stmt->kind == TKKeyword_for //
            or stmt->kind == TKKeyword_while //
            or stmt->kind == TKKeyword_else //
                and stmt->body)
            ASTScope_lowerElementalOps(stmt->body);

        if (not stmt->isElementalOp) continue;

        // wrap it in an empty block (or use if true)
        ASTExpr* ifblk = NEW(ASTExpr);
        ifblk->kind = TKKeyword_if;
        ifblk->left = NEW(ASTExpr);
        ifblk->left->kind = TKNumber;
        ifblk->string = "1";

        // look top-down for subscripts. if you encounter a node with
        // isElementalOp=false, don't process it further even if it may have
        // ranges inside. e.g.
        // vec[7:9] = arr2[6:8] + sin(arr2[-6:-1:-4]) + test[[8,6,5]]
        // + 3 + count(vec[vec < 5]) + M ** x[-8:-1:-4]
        // the matmul above is not
        // elemental, but the range inside it is.
        // the Array_count_filter will be promoted and isnt elemental
        // (unless you plan to set elemental op on boolean subscripts.)
        // Even so, count is a reduce op and will unset isElementalOp.
        // --
        // as you find each subscript, add 2 local vars to the ifblk body
        // so then you might have for the above example :
        // T* vec_p1 = vec->start + 7;
        // // ^ this func could be membptr(a,i) -> i<0 ? a->end-i :
        // a->start+i #define vec_1 *vec_p1 // these could be ASTVars with
        // an isCMacro flag T2* arr2_p1 = membptr(arr2, 6); #define arr2_1
        // *arr2_p1 T3* arr2_p2 = membptr(arr2, -6); #define arr2_2 *arr2_p2
        // ...
        // // now add vars for each slice end and delta
        // const T* const vec_e1 = vec->start + 9; // use membptr
        // const T2* const arr2_e1 = arr2->start + 8; // membptr
        // const T3* const arr2_e2 = membptr(arr2, -4);
        // what about test[[8,6,5]]?
        // const T* const vec_d1 = 1;
        // const T2* const arr2_d1 = 1;
        // const T3* const arr2_d2 = -1;
        // ...
        // // the ends (and starts) could be used for BC.
        // ...
        // // now add a check / separate checks for count match and bounds
        // check_span1deq(vec_e1,vec_p1,arr2_e1,arr2_p1,col1,col2,"vec[7:9]","arr2[6:8]",__FILE__,__LINE__);
        // check_span1deq(arr2_e1,arr2_p1,arr2_e2,arr2_p2,col1,col2,"arr2[6:8]","arr2[-6:-4]",__FILE__,__LINE__);
        // check_inbounds1d(vec, vec_p1, vec_e1,col1,
        // "vec[7:9]",__FILE__,__LINE__) check_inbounds1d(arr2, arr2_p1,
        // arr2_e1,col1, "arr2[6:8]",__FILE__,__LINE__) now change the
        // subscripts in the stmt to unresolved idents, and change the ident
        // by appending _1, _2 etc. based on their position. so when they
        // are generated they will refer to the current item of that array.
        // then wrap the stmt in a for expr 'forblk'. put the for expr in
        // ifblk. the active scope is now the for's body. generate the stmt.
        // it should come out in scalar form if all went well.. add
        // increments for each ptr. vec_p1 += vec_d1; arr2_p1 += arr2_d1;
        // arr2_p2 += arr2_d2;
        // ...
        // all done, at the end put the ifblk at the spot of stmt in the
        // original scope. stmt is already inside ifblk inside forblk.
    }
}

static void ASTScope_promoteCandidates(ASTScope* scope)
{
    int tmpCount = 0;
    ASTExpr* pc = NULL;
    List(ASTExpr)* prev = NULL; // this->stmts;
    foreach (ASTExpr*, stmt, stmts, scope->stmts) {
        // TODO:
        // if (not stmt->flags.mayNeedPromotion) {prev=stmts;continue;}

        if (stmt->kind == TKKeyword_if //
            or stmt->kind == TKKeyword_for //
            or stmt->kind == TKKeyword_while //
            or stmt->kind == TKKeyword_else //
                and stmt->body)
            ASTScope_promoteCandidates(stmt->body);

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
        tmpvar->name = newTmpVarName(++tmpCount);
        tmpvar->typeSpec = NEW(ASTTypeSpec);
        tmpvar->typeSpec->typeType = TYReal64; // FIXME
        // TODO: setup tmpvar->typeSpec
        PtrList_append(&scope->locals, tmpvar);

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
            scope->stmts = PtrList_with(pcClone);
            scope->stmts->next = stmts;
            prev = scope->stmts;
        } else {
            prev->next = PtrList_with(pcClone);
            prev->next->next = stmts;
            prev = prev->next;
        } // List(ASTExpr)* insertionPos = prev ? prev->next : this->stmts;
          //  insertionPos
        //  = insertionPos;
        goto startloop; // it will continue there if no more promotions are
                        // needed

        prev = stmts;
    }
}

static void ASTScope_genc(ASTScope* scope, int level)
{
    foreach (ASTVar*, local, locals, scope->locals) {
        ASTVar_genc(local, level, false);
        puts(";");
    } // these will be declared at top and defined within the expr list
    foreach (ASTExpr*, stmt, stmts, scope->stmts) {
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
        // ASTExpr_genPrintVars(stmt); // just for kicks
        // puts("//--//");
        if (ASTExpr_canThrow(stmt))
            puts("    if (_err_ == ERROR_TRACE) goto backtrace;");
    }
}

static void ASTType_genc(ASTType* type, int level)
{
    if (not type->body) return;
    printf("#define FIELDS_%s \\\n", type->name);
    foreach (ASTVar*, var, fvars, type->body->locals) {
        if (not var) continue;
        ASTVar_genc(var, level + STEP, true);
        printf("; \\\n");
    }
    printf("\n\nstruct %s {\n", type->name);

    if (type->super) {
        printf("    FIELDS_");
        ASTTypeSpec_genc(type->super, level, false);
        printf("\n");
    }

    printf("    FIELDS_%s\n};\n\n", type->name);
    printf(
        "static const char* %s_name_ = \"%s\";\n", type->name, type->name);
    printf("static %s %s_alloc_() {\n    return _Pool_alloc_(&gPool_, "
           "sizeof(struct %s));\n}\n",
        type->name, type->name, type->name);
    printf("static %s %s_init_(%s this) {\n", type->name, type->name,
        type->name);
    // TODO: rename this->checks to this->exprs or this->body
    foreach (ASTVar*, var, vars, type->body->locals) {
        printf("#define %s this->%s\n", var->name, var->name);
    }
    foreach (ASTExpr*, stmt, stmts, type->body->stmts) {
        if (not stmt or stmt->kind != TKVarAssign or !stmt->var->init)
            continue;
        ASTExpr_genc(stmt, level + STEP, true, false, false);
        puts(";");
    }
    foreach (ASTVar*, var, mvars, type->body->locals) {
        printf("#undef %s \n", var->name);
    }

    printf("    return this;\n}\n");
    printf("%s %s_new_() {\n    return "
           "%s_init_(%s_alloc_());\n}\n",
        type->name, type->name, type->name, type->name);
    printf("#define %s_print_ %s_print__(p, STR(p))\n", type->name,
        type->name);
    printf("%s %s_print__(%s this, const char* name) {\n    printf(\"<%s "
           "'%%s' at %%p>\",name,this);\n}\n",
        type->name, type->name, type->name, type->name);

    puts("");
}

static void ASTType_genh(ASTType* type, int level)
{
    if (not type->body) return;
    printf("typedef struct %s* %s; struct %s;\n", type->name, type->name,
        type->name);
}

static void ASTFunc_genc(ASTFunc* func, int level)
{
    if (func->flags.isDeclare) return;
    size_t stackUsage = ASTFunc_calcSizeUsage(func);

    // it seems that actual stack usage is higher due to stack protection,
    // frame bookkeeping whatever, and in debug mode the callsite needs
    // sizeof(char*) more.
    printf("#ifdef DEBUG\n"
           "#define MYSTACKUSAGE (%lu + 4*sizeof(void*) + sizeof(char*))\n"
           "#else\n"
           "#define MYSTACKUSAGE (%lu + 4*sizeof(void*))\n"
           "#endif\n",
        stackUsage, stackUsage);

    printf("#define DEFAULT_VALUE %s\n",
        getDefaultValueForType(func->returnType));
    if (not func->flags.isExported) printf("static ");
    if (func->returnType) {
        ASTTypeSpec_genc(func->returnType, level, false);
    } else {
        printf("void");
    }
    printf(" %s(", func->selector);
    foreach (ASTVar*, arg, args, func->args) {
        ASTVar_genc(arg, level, true);
        printf(args->next ? ", " : "");
    }

    printf("\n#ifdef DEBUG\n"
           "    %c const char* callsite_ "
           "\n#endif\n",
        ((func->args and func->args->item ? ',' : ' ')));

    // TODO: if (flags.throws) printf("const char** _err_");
    puts(") {");
    printf("#ifdef DEBUG\n"
           "    static const char* sig_ = \"");
    printf("%s%s(", func->flags.isStmt ? "" : "function ", func->name);

    foreach (ASTVar*, arg, chargs, func->args) {
        ASTVar_gen(arg, level);
        printf(chargs->next ? ", " : ")");
    }
    if (func->returnType) {
        printf(" returns ");
        ASTTypeSpec_gen(func->returnType, level);
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

    ASTScope_genc(func->body, level + STEP);

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

static void ASTFunc_genh(ASTFunc* func, int level)
{
    if (func->flags.isDeclare) return;
    if (not func->flags.isExported) printf("static ");
    if (func->returnType) {
        ASTTypeSpec_genc(func->returnType, level, false);
    } else {
        printf("void");
    }
    printf(" %s(", func->selector);
    foreach (ASTVar*, arg, args, func->args) {
        ASTVar_genc(arg, level, true);
        printf(args->next ? ", " : "");
    }
    printf("\n#ifdef DEBUG\n    %c const char* callsite_ "
           "\n#endif\n",
        ((func->args and func->args->item) ? ',' : ' '));
    puts(");\n");
}

static void ASTExpr_genc(ASTExpr* expr, int level, bool spacing,
    bool inFuncArgs, bool escStrings)
{
    // generally an expr is not split over several lines (but maybe in
    // rare cases). so level is not passed on to recursive calls.

    printf("%.*s", level, spaces);
    switch (expr->kind) {
    case TKNumber:
    case TKMultiDotNumber:
        printf("%.*s", expr->strLen, expr->string);
        break;

    case TKString:
        printf(escStrings ? "\\%.*s\\\"" : "%.*s\"", expr->strLen - 1,
            expr->string);
        break;

    case TKIdentifier:
    case TKIdentifierResolved:
        // convert a.b.c.d to DEREF3(a,b,c,d), a.b to DEREF(a,b) etc.
        {
            char* tmp = (expr->kind == TKIdentifierResolved)
                ? expr->var->name
                : expr->name;
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
        expr->string[0] = '"';
        expr->string[expr->strLen - 1] = '"';
        printf("%.*s", expr->strLen, expr->string);
        expr->string[0] = '\'';
        expr->string[expr->strLen - 1] = '\'';
        break;

    case TKInline:
        expr->string[0] = '"';
        expr->string[expr->strLen - 1] = '"';
        printf("mkRe_(%.*s)", expr->strLen, expr->string);
        expr->string[0] = '`';
        expr->string[expr->strLen - 1] = '`';
        break;

    case TKLineComment:
        // TODO: skip  comments in generated code
        printf("// %.*s", expr->strLen, expr->string);
        break;

    case TKFunctionCall:
    case TKFunctionCallResolved: {
        char* tmp = (expr->kind == TKFunctionCallResolved)
            ? expr->func->name
            : expr->name;
        ASTExpr* firstArg = expr->left;

        if (firstArg->kind == TKOpComma) firstArg = firstArg->left;

        // TODO: refactor the following into a func, much needed
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
        if (expr->left) ASTExpr_catarglabels(expr->left);

        str_tr_ip(tmp, '_', '.', 0);
        // this won't be needed, prepc will do the "mangling"
        printf("(");

        if (expr->left)
            ASTExpr_genc(expr->left, 0, false, true, escStrings);

        if (strcmp(tmp, "print")) { // FIXME
            // more generally this IF is for those funcs that are
            // standard and dont need any instrumentation
            printf("\n#ifdef DEBUG\n"
                   "      %c THISFILE \":%d:\\e[0m\\n     -> ",
                expr->left ? ',' : ' ', expr->line);
            ASTExpr_gen(expr, 0, false, true);
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
            = /*(this->kind == TKSubscriptResolved) ?*/ expr->var->name;
        //: this->name;
        ASTExpr* index = expr->left;
        assert(index);
        switch (index->kind) {
        case TKNumber:
            // indexing with a single number
            printf("Array_get_%s(%s, %s)",
                ASTTypeSpec_name(expr->var->typeSpec), name, index->string);
            break;

        case TKString:
        case TKRegex:
            // indexing with single string or regex
            printf("Dict_get_CString_%s(%s, %s)",
                ASTTypeSpec_name(expr->var->typeSpec), name, index->string);
            break;

        case TKOpComma:
            // higher dimensions. validation etc. has been done by this
            // stage.

            // this is for cases like arr[2, 3, 4].
            printf("Tensor%dD_get_%s(%s, {", expr->var->typeSpec->dims,
                ASTTypeSpec_name(expr->var->typeSpec), name);
            ASTExpr_genc(index, 0, false, inFuncArgs, escStrings);
            printf("})");

            // TODO: cases like arr[2:3, 4:5, 1:end]
            // basically the idea is to generate getijk/getIJK/getIJk etc.
            // where a caps means range and lowercase means single number.
            // so arr[2:3, 4:5, 1:end] should generate `getIJK`,
            // arr[2:3, 4, 2:end] should generate `getIjK` and so on.
            // Those are then macros in the "runtime" that have for loops
            // for the ranges and nothing special for the single indices.
            // but they should be put into a tmpvar to avoid repeated eval.

            break;

        case TKOpColon:
            // a single range.
            printf("Array_getSlice_%s(%s, ",
                ASTTypeSpec_name(expr->var->typeSpec), name);
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
                ASTTypeSpec_name(expr->var->typeSpec), name);
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

        switch (expr->left->kind) {
        case TKSubscriptResolved:
            switch (expr->left->left->kind) {
            case TKNumber:
            case TKString:
            case TKRegex:
                // TODO: astexpr_typename should return Array_Scalar or
                // Tensor2D_Scalar or Dict_String_Scalar etc.
                printf("%s_set(%s, %s,%s, ", ASTExpr_typeName(expr->left),
                    expr->left->var->name, expr->left->left->string,
                    TokenKind_repr(expr->kind, spacing));
                // ASTExpr_genc(this->left->left,0,spacing,inFuncArgs,escStrings);
                // printf(", ");
                ASTExpr_genc(
                    expr->right, 0, spacing, inFuncArgs, escStrings);
                printf(")");
                break;

            case TKOpColon:
                printf("%s_setSlice(%s, ", ASTExpr_typeName(expr->left),
                    expr->left->var->name);
                ASTExpr_genc(
                    expr->left->left, 0, spacing, inFuncArgs, escStrings);
                printf(",%s, ", TokenKind_repr(expr->kind, spacing));
                ASTExpr_genc(
                    expr->right, 0, spacing, inFuncArgs, escStrings);
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
                printf("%s_setFiltered(%s, ", ASTExpr_typeName(expr->left),
                    expr->left->var->name);
                ASTExpr_genc(
                    expr->left->left, 0, spacing, inFuncArgs, escStrings);
                printf(",%s, ", TokenKind_repr(expr->kind, spacing));
                ASTExpr_genc(
                    expr->right, 0, spacing, inFuncArgs, escStrings);
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
            ASTExpr_genc(expr->left, 0, spacing, inFuncArgs, escStrings);
            printf("%s", TokenKind_repr(TKOpAssign, spacing));
            ASTExpr_genc(expr->right, 0, spacing, inFuncArgs, escStrings);
            break;
        case TKIdentifier:
            assert(inFuncArgs);
            ASTExpr_genc(expr->right, 0, spacing, inFuncArgs, escStrings);
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
        ASTExpr_genc(expr->right, 0, spacing, inFuncArgs, escStrings);
        printf("}");
        printf(", %d)", ASTExpr_countCommaList(expr->right));
        break;

    case TKOpColon: // convert 3:4:5 to range(...)
                    // must do bounds check first!
        printf("%s(",
            expr->left->kind != TKOpColon ? "range_to" : "range_to_by");
        if (expr->left->kind == TKOpColon) {
            expr->left->kind = TKOpComma;
            ASTExpr_genc(expr->left, 0, false, inFuncArgs, escStrings);
            expr->left->kind = TKOpColon;
        } else
            ASTExpr_genc(expr->left, 0, false, inFuncArgs, escStrings);
        printf(", ");
        ASTExpr_genc(expr->right, 0, false, inFuncArgs, escStrings);
        printf(")");
        break;

    case TKVarAssign: // basically a TKOpAssign corresponding to a local
                      // var
        // var x as XYZ = abc... -> becomes an ASTVar and an
        // ASTExpr (to keep location). Send it to ASTVar::gen.
        //        ASTVar_genc(this->var, 0, false);
        if (expr->var->init != NULL) {
            printf("%s = ", expr->var->name);
            ASTExpr_genc(expr->var->init, 0, true, inFuncArgs, escStrings);
        }
        break;

    case TKKeyword_else:
        puts("else {");
        if (expr->body) ASTScope_genc(expr->body, level + STEP);
        printf("%.*s}", level, spaces);
        break;

    case TKKeyword_for:
    case TKKeyword_if:
    case TKKeyword_while:
        if (expr->kind == TKKeyword_for)
            printf("FOR(");
        else
            printf("%s (", TokenKind_repr(expr->kind, true));
        if (expr->kind == TKKeyword_for) expr->left->kind = TKOpComma;
        if (expr->left)
            ASTExpr_genc(expr->left, 0, spacing, inFuncArgs, escStrings);
        if (expr->kind == TKKeyword_for) expr->left->kind = TKOpAssign;
        puts(") {");
        if (expr->body) ASTScope_genc(expr->body, level + STEP);
        printf("%.*s}", level, spaces);
        break;

    case TKPower:
        printf("pow(");
        ASTExpr_genc(expr->left, 0, false, inFuncArgs, escStrings);
        printf(",");
        ASTExpr_genc(expr->right, 0, false, inFuncArgs, escStrings);
        printf(")");
        break;

    case TKKeyword_return:
        printf("{_err_ = NULL; \n#ifndef NOSTACKCHECK\n    "
               "STACKDEPTH_DOWN\n#endif\nreturn ");
        ASTExpr_genc(expr->right, 0, spacing, inFuncArgs, escStrings);
        printf(";}\n");
        break;
    case TKKeyword_check:
        // switch (expr->right->kind) {
        // case TKOpLE:
        // case TKOpLT:
        // case TKOpGE:
        // case TKOpGT:
        // case TKOpNE:
        // case TKOpEQ:
        // case TKKeyword_and:
        // case TKKeyword_or:
        // case TKKeyword_not:
        printf("{\n");
        if (not expr->right->opIsUnary) {
            printf("%.*s%s _lhs = ", level, spaces,
                ASTExpr_typeName(expr->right->left));
            ASTExpr_genc(expr->right->left, 0, spacing, false, false);
            printf(";\n");
        }
        printf("%.*s%s _rhs = ", level, spaces,
            ASTExpr_typeName(expr->right->right));
        ASTExpr_genc(expr->right->right, 0, spacing, false, false);
        printf(";\n");
        printf("%.*sif (not(", level, spaces);
        // ASTExpr_genc(expr->right, 0, spacing, false, false);
        printf("%s%s_rhs", expr->right->opIsUnary ? "" : "_lhs",
            TokenKind_repr(expr->right->kind, true));
        printf(")) {\n");
        // printf("%.*sprintf(\"\\n%%s\\n\",_undersc72_);\n", level + STEP,
        // spaces);
        printf("%.*sprintf(\"\\n|\\n| check failed at ./%%s:%d:\\n|   "
               "%%s\\n|\\n| because\\n\", "
               "THISFILE, \"",
            level + STEP, spaces, expr->line);
        ASTExpr_gen(expr->right, 0, spacing, true);
        printf("\");\n");

        if (not expr->right->opIsUnary) {
            // dont print literals or arrays
            if (expr->right->left->collectionType == CTYNone) {
                printf(
                    "%.*s%s", level + STEP, spaces, "printf(\"|   %s = ");
                printf("%s",
                    TypeType_format(expr->right->left->typeType, true));
                printf("%s", "\\n\", \"");
                ASTExpr_gen(expr->right->left, 0, spacing, true);
                printf("%s", "\", _lhs);\n");
            }
        }
        if (expr->right->right->collectionType == CTYNone) {
            printf("%.*s%s", level + STEP, spaces, "printf(\"|   %s = ");
            printf(
                "%s", TypeType_format(expr->right->right->typeType, true));
            printf("%s", "\\n\", \"");
            ASTExpr_gen(expr->right->right, 0, spacing, true);
            printf("%s", "\", _rhs);\n");
        }
        // ASTExpr_gen(expr->right->right, 0, spacing, true);

        ASTExpr_genPrintVars(expr->right, level + STEP);
        // printf(
        //     "%.*sprintf(\"%%s\\n\",_undersc72_);\n", level + STEP,
        //     spaces);
        printf("%.*sprintf(\"|\\n\");\n", level + STEP, spaces);
        printf("\n%.*s}\n", level, spaces);
        printf("%.*s}", level, spaces);
        break;

        // default:
        //     assert(0); // NO, just allow whatever. I think in fact we
        //     don't
        //                // need the switch at all
        //     break;
        // }
        break;
    default:
        if (not expr->opPrec) break;
        // not an operator, but this should be error if you reach here
        bool leftBr = expr->left and expr->left->opPrec
            and expr->left->opPrec < expr->opPrec;
        bool rightBr = expr->right and expr->right->opPrec
            and expr->right->kind
                != TKKeyword_return // found in 'or return'
            and expr->right->opPrec < expr->opPrec;

        char lpo = '(';
        char lpc = ')';
        if (leftBr) putc(lpo, stdout);
        if (expr->left)
            ASTExpr_genc(expr->left, 0,
                spacing and !leftBr and expr->kind != TKOpColon, inFuncArgs,
                escStrings);
        if (leftBr) putc(lpc, stdout);

        if (expr->kind == TKArrayOpen)
            putc('{', stdout);
        else
            printf("%s", TokenKind_repr(expr->kind, spacing));

        char rpo = '(';
        char rpc = ')';
        if (rightBr) putc(rpo, stdout);
        if (expr->right)
            ASTExpr_genc(expr->right, 0,
                spacing and !rightBr and expr->kind != TKOpColon,
                inFuncArgs, escStrings);
        if (rightBr) putc(rpc, stdout);

        if (expr->kind == TKArrayOpen) putc('}', stdout);
    }
}

static void ASTModule_genc(ASTModule* module, int level)
{
    foreach (ASTImport*, import, imports, module->imports)
        ASTImport_genc(import, level);

    puts("");

    foreach (ASTType*, type, types, module->types)
        ASTType_genh(type, level);

    foreach (ASTFunc*, func, funcs, module->funcs)
        ASTFunc_genh(func, level);

    foreach (ASTType*, type, mtypes, module->types)
        ASTType_genc(type, level);

    foreach (ASTFunc*, func, mfuncs, module->funcs)
        ASTFunc_genc(func, level);

    foreach (ASTImport*, simport, simports, module->imports)
        ASTImport_undefc(simport);
}
