// TODO make sempassModule -> same as analyseModule now
static void sempassType(Parser* parser, ASTType* type, ASTModule* mod);
static void sempassFunc(Parser* parser, ASTFunc* func, ASTModule* mod);

///////////////////////////////////////////////////////////////////////////
static bool isCmpOp(ASTExpr* expr)
{
    return expr->kind == tkOpLE or expr->kind == tkOpLT or expr->kind == tkOpGT or expr->kind == tkOpGE or expr->kind == tkOpEQ
        or expr->kind == tkOpNE;
}

///////////////////////////////////////////////////////////////////////////
static bool isBoolOp(ASTExpr* expr)
{
    return expr->kind == tkKeyword_and or expr->kind == tkKeyword_or or expr->kind == tkKeyword_not;
}

///////////////////////////////////////////////////////////////////////////
static void sempass(Parser* parser, ASTExpr* expr, ASTModule* mod, bool inFuncArgs)
{ // return;
    switch (expr->kind) {

    case tkFunctionCallResolved: {

        if (ASTExpr_countCommaList(expr->left) != expr->func->argCount) Parser_errorArgsCountMismatch(parser, expr);
        expr->typeType = expr->func->returnType ? expr->func->returnType->typeType : TYUnresolved; // should actually be TYVoid
        expr->elemental = expr->func->flags.isElementalFunc;
        // isElementalFunc means the func is only defined for (all)
        // Number arguments and another definition for vector args
        // doesn't exist. Basically during typecheck this should see
        // if a type mismatch is only in terms of collectionType.
        if (not expr->left) break;
        expr->elemental = expr->elemental and expr->left->elemental;
        ASTExpr* currArg = expr->left;
        foreach (ASTVar*, arg, expr->func->args) {
            ASTExpr* cArg = (currArg->kind == tkOpComma) ? currArg->left : currArg;
            if (cArg->kind == tkOpAssign) cArg = cArg->right;
            if (cArg->typeType != arg->typeSpec->typeType) Parser_errorArgTypeMismatch(parser, cArg, arg);
            // TODO: check dims mismatch
            // TODO: check units mismatch
            if (not(currArg = currArg->right)) break;
        }
    } break;

    case tkFunctionCall: {
        char buf[128] = {};
        char* bufp = buf;
        if (expr->left) sempass(parser, expr->left, mod, true);

        // TODO: need a function to make and return selector
        ASTExpr* arg1 = expr->left;
        if (arg1 and arg1->kind == tkOpComma) arg1 = arg1->left;
        if (arg1) bufp += sprintf(bufp, "%s_", ASTExpr_typeName(arg1));
        bufp += sprintf(bufp, "%s", expr->string);
        if (expr->left) ASTExpr_strarglabels(expr->left, bufp, 128 - ((int)(bufp - buf)));

        ASTFunc* found = ASTModule_getFunc(mod, buf);
        if (found) {
            expr->kind = tkFunctionCallResolved;
            expr->func = found;
            sempassFunc(parser, found, mod);
            sempass(parser, expr, mod, inFuncArgs);
            return;
        }
        Parser_errorUnrecognizedFunc(parser, expr, buf);
        if (*buf != '<') // not invalid type
            foreach (ASTFunc*, func, mod->funcs)
                if (not strcasecmp(expr->string, func->name))
                    eprintf("        \e[;2mnot viable: \e[34m%s\e[0;2m (%d args) at ./%s:%d\e[0m\n", func->selector, func->argCount,
                        parser->filename, func->line);
    } break;

    case tkVarAssign: {
        if (not expr->var->init)
            Parser_errorMissingInit(parser, expr);
        else {
            if (expr->var->typeSpec->typeType == TYUnresolved) resolveTypeSpec(parser, expr->var->typeSpec, mod);

            sempass(parser, expr->var->init, mod, false);

            expr->typeType = expr->var->init->typeType;
            expr->collectionType = expr->var->init->collectionType;
            expr->nullable = expr->var->init->nullable;
            expr->elemental = expr->var->init->elemental;

            if (expr->var->typeSpec->typeType == TYUnresolved) {
                expr->var->typeSpec->typeType = expr->var->init->typeType;
                if (expr->var->init->typeType == TYObject) {
                    if (expr->var->init->kind == tkFunctionCallResolved)
                        expr->var->typeSpec->type = expr->var->init->func->returnType->type;
                    else if (expr->var->init->kind == tkIdentifierResolved)
                        expr->var->typeSpec->type = expr->var->init->var->typeSpec->type;
                    else if (expr->var->init->kind == tkPeriod) {
                        ASTExpr* e = expr->var->init;
                        while (e->kind == tkPeriod) e = e->right;
                        // at this point, it must be a resolved ident or
                        // subscript
                        expr->var->typeSpec->type = e->var->typeSpec->type;
                    } else {
                        unreachable("%s", "var type inference failed");
                    }
                    sempassType(parser, expr->var->typeSpec->type, mod);
                }
            } else if (expr->var->typeSpec->typeType != expr->var->init->typeType) {
                Parser_errorInitMismatch(parser, expr);
                expr->typeType = TYErrorType;
            }
        }
    } break;

    case tkKeyword_else:
    case tkKeyword_if:
    case tkKeyword_for:
    case tkKeyword_while: {
        if (expr->left) sempass(parser, expr->left, mod, false);
        foreach (ASTExpr*, stmt, expr->body->stmts)
            sempass(parser, stmt, mod, inFuncArgs);
    } break;

    case tkSubscriptResolved:
    case tkSubscript:
        if (expr->left) sempass(parser, expr->left, mod, inFuncArgs);
        if (expr->kind == tkSubscriptResolved) {
            expr->typeType = expr->var->typeSpec->typeType;
            // setExprTypeInfo(parser, expr->left, false); // check args
            expr->elemental = expr->left->elemental;
            // TODO: check args in the same way as for funcs below, not
            // directly checking expr->left.}
        }
        break;

    case tkString:
        expr->typeType = TYString;
        break;

    case tkNumber:
        expr->typeType = TYReal64;
        break;

    case tkIdentifier:
        expr->typeType = TYErrorType;
        break;

    case tkIdentifierResolved:
        expr->typeType = expr->var->typeSpec->typeType;
        break;

    case tkPeriod: {
        assert(expr->left->kind == tkIdentifierResolved or expr->left->kind == tkIdentifier);
        sempass(parser, expr->left, mod, inFuncArgs);

        // The name/type resolution of expr->left may have failed.
        if (not expr->left->typeType) break;

        ASTExpr* member = expr->right;
        if (member->kind == tkPeriod) {
            member = member->left;
            if (member->kind != tkIdentifier) {
                Parser_errorUnexpectedExpr(parser, member);
                break;
            }
        }

        if (member->kind != tkIdentifier and member->kind != tkSubscript) {
            Parser_errorUnexpectedExpr(parser, member);
            break;
        }
        //  or member->kind == tkFunctionCall);
        if (member->kind != tkIdentifier) sempass(parser, member->left, mod, inFuncArgs);

        // the left must be a resolved ident
        if (expr->left->kind != tkIdentifierResolved) break;

        ASTType* type = expr->left->var->typeSpec->type;
        // Resolve the member in the scope of the type definition.
        resolveMember(parser, member, type);
        // Name resolution may fail...
        if (member->kind != tkIdentifierResolved) {
            expr->typeType = TYErrorType;
            break;
        }
        sempass(parser, member, mod, inFuncArgs);

        if (expr->right->kind == tkPeriod) sempass(parser, expr->right, mod, inFuncArgs);

        // TODO: exprs like a.b.c.d.e are not handled yet, only a.b

        expr->typeType = expr->right->typeType;
        expr->elemental = expr->right->elemental;
    } break;

    default:
        if (expr->prec) {
            if (not expr->unary) sempass(parser, expr->left, mod, inFuncArgs);
            sempass(parser, expr->right, mod, inFuncArgs);

            if (expr->kind == tkKeyword_or and expr->left->typeType != TYBool) {
                // Handle the 'or' keyword used to provide alternatives for a nullable expression.
                ;
            } else if (isCmpOp(expr) or isBoolOp(expr) or expr->kind == tkKeyword_in) {
                // Handle comparison and logical operators (always return a bool)
                expr->typeType
                    = (expr->left->typeType == TYErrorType or expr->right->typeType == TYErrorType) ? TYErrorType : TYBool;
            } else {
                // Set the type from the ->right expr for now. if an error type is on the right, this is accounted for.
                expr->typeType = expr->right->typeType;
            }
            expr->elemental = expr->right->elemental or expr->kind == tkOpColon;
            // TODO: actually, indexing by an array of integers is also an indication of an elemental op

            if (not expr->unary) expr->elemental = expr->elemental or expr->left->elemental;

            if (not expr->unary and not(inFuncArgs and (expr->kind == tkOpComma or expr->kind == tkOpAssign))) {
                // ignore , and = inside function call arguments. thing is array or dict literals passed as args will have
                // , and = which should be checked. so when you descend into their args, unset inFuncArgs.
                TypeTypes leftType = expr->left->typeType;
                TypeTypes rightType = expr->right->typeType;

                if (leftType == TYBool and (expr->kind == tkOpLE or expr->kind == tkOpLT)) {
                    // Special case: chained LE/LT operators: e.g. 0 <= yCH4 <= 1.
                    ;
                } else if (leftType != rightType) {
                    // Type mismatch for left and right operands is always an error.
                    Parser_errorTypeMismatchBinOp(parser, expr);
                    expr->typeType = TYErrorType;
                } else if (leftType == TYString and (expr->kind == tkOpAssign or expr->kind == tkOpEQ or expr->kind == tkPlusEq)) {
                    // Allow assignment, equality test and += for strings.
                    // TODO: might even allow comparison operators, and perhaps disallow +=
                    ;
                } else if (leftType == TYBool
                    and (expr->kind == tkOpAssign or expr->kind == tkOpEQ or expr->kind == tkKeyword_and
                        or expr->kind == tkKeyword_or))
                    ;
                else if (not TypeType_isnum(leftType)) {
                    // Arithmetic operators are only relevant for numeric types.
                    Parser_errorInvalidTypeForOp(parser, expr);
                }
                // check if an error type is on the left, if yes, set the expr type
                if (leftType == TYErrorType) expr->typeType = leftType;
            }
            // TODO: here statements like return etc. that are not binary
            // but need to have their types checked w.r.t. an expected type
            // TODO: some ops have a predefined type e.g. : is of type Range
            // etc,
        } else {
            unreachable("unknown expr kind: %s", TokenKind_str[expr->kind]);
        }
    }
}

///////////////////////////////////////////////////////////////////////////
static void sempassType(Parser* parser, ASTType* type, ASTModule* mod)
{
    if (type->flags.sempassDone) return;
    // eprintf(
    //     "sempass: %s at ./%s:%d\n", type->name, parser->filename,
    //     type->line);
    if (type->super) {
        resolveTypeSpec(parser, type->super, mod);
        if (type->super->type == type) Parser_errorTypeInheritsSelf(parser, type);
    }
    // TODO: this should be replaced by a dict query
    foreach (ASTType*, type2, mod->types) {
        if (type2 == type) break;
        if (not strcasecmp(type->name, type2->name)) Parser_errorDuplicateType(parser, type, type2);
    }
    // Mark the semantic pass as done for this type, so that recursive
    // paths through calls found in initializers will not cause the compiler
    // to recur. This might be a problem if e.g. the type has a, b, c and
    // the initializer for b has a dependency on the type's .c member, whose
    // type has not been set by the time b's initializer is processed.
    // However if you set sempassDone after all statements, then you risk
    // getting caught in a recursive path. One way to fix it is to have
    // granularity at the member level, so not entire types but their
    // individual members are processed. In that case the only problem can
    // be a recursive path between a member var and a function that it calls
    // in its initializer.
    type->flags.sempassDone = true;
    // nothing to do for declared/empty types etc. with no body
    if (type->body) foreach (ASTExpr*, stmt, type->body->stmts)
            sempass(parser, stmt, mod, false);
}

///////////////////////////////////////////////////////////////////////////
static void sempassFunc(Parser* parser, ASTFunc* func, ASTModule* mod)
{
    if (func->flags.semPassDone) return;
    // eprintf("sempass: %s at ./%s:%d\n", func->selector, parser->filename,
    // func->line);

    bool foundCtor = false;
    // Check if the function is a constructor call and identify the type.
    // TODO: this should be replaced by a dict query
    foreach (ASTType*, type, mod->types) {
        if (not strcasecmp(func->name, type->name)) {
            if (func->returnType and not(func->flags.isStmt or func->flags.isDefCtor)) Parser_errorCtorHasType(parser, func, type);
            if (not func->returnType) {
                func->returnType = ASTTypeSpec_new(TYObject, CTYNone);
                func->returnType->type = type;
            }
            // TODO: isStmt Ctors should have the correct type so e.g.
            // you cannot have
            // Point(x as Number) := 3 + 5 * 12
            // but must return a Point instead. This cannot be enforced
            // here since type resolution hasn't been done at this
            // stage. Check this after the type inference step when the
            // stmt func has its return type assigned.
            // if (func->flags.isStmt)
            //     Parser_errorCtorHasType(this, func, type);
            if (not isupper(*func->name)) Parser_warnCtorCase(parser, func);

            func->name = type->name;
            foundCtor = true;
        }
    }

    // Capitalized names are not allowed unless they are constructors.
    if (not func->flags.isDefCtor and not foundCtor and isupper(*func->name)) Parser_errorUnrecognizedCtor(parser, func);

    // The rest of the processing is on the contents of the function.
    if (not func->body) {
        func->flags.semPassDone = true;
        return;
    }

    // Check for duplicate functions (same selectors) and report errors.
    // TODO: this should be replaced by a dict query
    foreach (ASTFunc*, func2, mod->funcs) {
        if (func == func2) break;
        if (not strcasecmp(func->selector, func2->selector)) Parser_errorDuplicateFunc(parser, func, func2);
    }

    // Check unused variables in the function and report warnings.
    ASTFunc_checkUnusedVars(parser, func);

    // Mark the semantic pass as done for this function, so that recursive
    // calls found in the statements will not cause the compiler to recur.
    func->flags.semPassDone = true;

    // Run the statement-level semantic pass on the function body.
    foreach (ASTExpr*, stmt, func->body->stmts)
        sempass(parser, stmt, mod, false);

    // Statement functions are written without an explicit return type.
    // Figure out the type (now that the body has been analyzed).
    if (func->flags.isStmt) setStmtFuncTypeInfo(parser, func);
    // TODO: for normal funcs, sempass should check return statements to
    // have the same type as the declared return type.

    // Do optimisations or ANY lowering only if there are no errors
    if (not parser->errCount and parser->mode == PMGenC) {
        // Handle elemental operations like arr[4:50] = mx[14:60] + 3
        ASTScope_lowerElementalOps(func->body);
        // Extract subexprs like count(arr[arr<1e-15]) and promote them to
        // full statements corresponding to their C macros e.g.
        // Number _1; Array_count_filter(arr, arr<1e-15, _1);
        ASTScope_promoteCandidates(func->body);
    }
}
