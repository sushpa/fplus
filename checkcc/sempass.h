// TODO make sempassModule -> same as analyseModule now
static void sempassType(Parser* self, ASTType* type, ASTModule* mod);
static void sempassFunc(Parser* self, ASTFunc* func, ASTModule* mod);

static void sempass(
    Parser* self, ASTExpr* expr, ASTModule* mod, bool inFuncArgs)
{ // return;
    switch (expr->kind) {

    case tkFunctionCallResolved: {

        if (ASTExpr_countCommaList(expr->left) != expr->func->argCount)
            Parser_errorArgsCountMismatch(self, expr);

        expr->typeType = expr->func->returnType
            ? expr->func->returnType->typeType
            : TYUnresolved; // should actually be TYVoid

        if (not expr->left) break;

        sempass(self, expr->left, mod, true);
        // but this call shouldn't check
        // for equal types on both sides of a comma

        expr->isElementalOp = expr->left->isElementalOp
            and expr->func->flags.isElementalFunc;
        // isElementalFunc means the func is only defined for (all)
        // scalar arguments and another definition for vector args
        // doesn't exist. Basically during typecheck this should see
        // if a type mismatch is only in terms of collectionType.

        ASTExpr* currArg = expr->left;
        foreach (ASTVar*, arg, expr->func->args) {
            ASTExpr* cArg
                = currArg->kind == tkOpComma ? currArg->left : currArg;
            if (cArg->kind == tkOpAssign) cArg = cArg->right;
            if (cArg->typeType != arg->typeSpec->typeType)
                Parser_errorArgTypeMismatch(self, cArg, arg);
            if (not(currArg = currArg->right)) break;
        }
    } break;

    case tkFunctionCall: {
        char buf[128] = {};
        char* bufp = buf;
        if (expr->left) sempass(self, expr->left, mod, inFuncArgs);

        // TODO: need a function to make and return selector
        ASTExpr* arg1 = expr->left;
        if (arg1 and arg1->kind == tkOpComma) arg1 = arg1->left;
        if (arg1) {
            const char* tyname = ASTExpr_typeName(arg1);
            bufp += sprintf(bufp, "%s_", tyname);
        }
        bufp += sprintf(bufp, "%s", expr->name);
        if (expr->left)
            ASTExpr_strarglabels(
                expr->left, bufp, 128 - ((int)(bufp - buf)));

        foreach (ASTFunc*, func, mod->funcs) {
            if (not strcasecmp(buf, func->selector)) {
                expr->kind = tkFunctionCallResolved;
                expr->func = func;
                sempassFunc(self, func, mod);
                sempass(self, expr, mod, inFuncArgs);
                return;
            }
        } // since it is known which module the func must be found in,
          // no need to scan others if function has not been found
        Parser_errorUnrecognizedFunc(self, expr, buf);
        foreach (ASTFunc*, func, mod->funcs)
            if (not strcasecmp(expr->name, func->name))
                eprintf("        \e[;2mnot viable: \e[34m%s\e[0;2m (%d "
                        "args) at ./%s:%d\e[0m\n",
                    func->selector, func->argCount, self->filename,
                    func->line);
        // but still check nested func calls
    } break;

    case tkVarAssign: {
        if (not expr->var->init)
            Parser_errorMissingInit(self, expr);
        else {
            if (!*expr->var->typeSpec->name)
                resolveTypeSpec(self, expr->var->typeSpec, mod);
            // sempass(self, expr->var->init, mod, inFuncArgs);

            sempass(self, expr->var->init, mod, false);

            expr->typeType = expr->var->init->typeType;
            expr->isElementalOp = expr->var->init->isElementalOp;

            if (not expr->var->typeSpec->typeType) {
                expr->var->typeSpec->typeType = expr->var->init->typeType;
                if (expr->var->init->typeType == TYObject) {
                    if (expr->var->init->kind == tkFunctionCallResolved)
                        expr->var->typeSpec->type
                            = expr->var->init->func->returnType->type;
                    else if (expr->var->init->kind == tkIdentifierResolved)
                        expr->var->typeSpec->type
                            = expr->var->init->var->typeSpec->type;
                    else
                        unreachable("%s", "var type inference failed");
                    sempassType(self, expr->var->typeSpec->type, mod);
                }
            } else if (expr->var->typeSpec->typeType
                != expr->var->init->typeType)
                Parser_errorTypeMismatchBinOp(self, expr);
        }
    } break;

    case tkKeyword_else:
    case tkKeyword_if:
    case tkKeyword_for:
    case tkKeyword_while: {
        if (expr->left) sempass(self, expr->left, mod, false);
        foreach (ASTExpr*, stmt, expr->body->stmts)
            sempass(self, stmt, mod, inFuncArgs);
    } break;

    case tkSubscriptResolved:
    case tkSubscript:
        if (expr->left) sempass(self, expr->left, mod, inFuncArgs);
        if (expr->kind == tkSubscriptResolved) {
            expr->typeType = expr->var->typeSpec->typeType;
            // setExprTypeInfo(self, expr->left, false); // check args
            expr->isElementalOp = expr->left->isElementalOp;
            // TODO: check args in the same way as for funcs below, not
            // directly checking expr->left.}
        }
        break;

    case tkString:
        expr->typeType = TYString;
        expr->isElementalOp = false;
        break;
    case tkNumber:
        expr->typeType = TYReal64;
        expr->isElementalOp = false;
        break;

    case tkIdentifierResolved:
        expr->typeType = expr->var->typeSpec->typeType;
        expr->isElementalOp = false;
        break;

    default:
        if (expr->opPrec) {
            if (not expr->opIsUnary)
                sempass(self, expr->left, mod, inFuncArgs);
            sempass(self, expr->right, mod, inFuncArgs);

            if (expr->kind == tkKeyword_or
                and expr->left->typeType != TYBool) {
                ; // this is the x = func(...) or break / or continue / or
                  // return (null handling)
            } else if (expr->kind == tkOpLE or expr->kind == tkOpLT
                or expr->kind == tkOpGT or expr->kind == tkOpGE
                or expr->kind == tkOpEQ or expr->kind == tkOpNE
                or expr->kind == tkKeyword_and or expr->kind == tkKeyword_or
                or expr->kind == tkKeyword_not
                or expr->kind == tkKeyword_in)
                expr->typeType = TYBool;
            else
                expr->typeType = expr->right->typeType;

            expr->isElementalOp
                = expr->right->isElementalOp or expr->kind == tkOpColon;
            // TODO: actually, indexing by an array of integers is also an
            // indication of an elemental op
            if (not expr->opIsUnary)
                expr->isElementalOp
                    = expr->isElementalOp or expr->left->isElementalOp;

            if (not expr->opIsUnary
                and not(inFuncArgs
                    and (expr->kind == tkOpComma
                        or expr->kind == tkOpAssign))) {
                TypeTypes leftType = expr->left->typeType;
                TypeTypes rightType = expr->right->typeType;

                // ignore , and = inside function call arguments.
                // thing is array or dict literals passed as args will have
                // , and = which should be checked. so when you descend into
                // their args, unset inFuncArgs.

                if (leftType == TYBool
                    and (expr->kind == tkOpLE or expr->kind == tkOpLT))
                    ;
                else if (leftType != rightType)
                    Parser_errorTypeMismatchBinOp(self, expr);
                // TODO: as it stands, "x" + "y" wont be an error because
                // types are consistent. BUT types should also be valid for
                // that operator: in general operators are defined only for
                // numeric types and some keywords for logicals.
                else if (leftType == TYString and //
                    (expr->kind == tkOpAssign //
                        or expr->kind == tkOpEQ //
                        or expr->kind == tkPlusEq))
                    ;
                else if (leftType == TYBool and //
                    (expr->kind == tkOpAssign //
                        or expr->kind == tkOpEQ
                        or expr->kind == tkKeyword_and
                        or expr->kind == tkKeyword_or))
                    ;
                else if (not TypeType_isnum(leftType))
                    Parser_errorInvalidTypeForOp(self, expr);

                // expr->typeType = leftType;
            }
            // TODO: here statements like return etc. that are not binary
            // but need to have their types checked w.r.t. an expected type
            // TODO: some ops have a predefined type e.g. : is of type Range
            // etc,
        } else
            assert(0);
    }
}

static void sempassType(Parser* self, ASTType* type, ASTModule* mod)
{
    if (type->flags.sempassDone) return;
    eprintf(
        "sempass: %s at ./%s:%d\n", type->name, self->filename, type->line);
    if (type->super) {
        resolveTypeSpec(self, type->super, mod);
        if (type->super->type == type)
            Parser_errorTypeInheritsSelf(self, type);
    }
    // TODO: this should be replaced by a dict query
    foreach (ASTType*, type2, mod->types) {
        if (type2 == type) break;
        if (not strcasecmp(type->name, type2->name))
            Parser_errorDuplicateType(self, type, type2);
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
            sempass(self, stmt, mod, false);
}

static void sempassFunc(Parser* self, ASTFunc* func, ASTModule* mod)

{
    if (func->flags.semPassDone) return;
    eprintf("sempass: %s: %s at ./%s:%d\n", func->name, func->selector,
        self->filename, func->line);

    bool foundCtor = false;
    // Check if the function is a constructor call and identify the type.
    // TODO: this should be replaced by a dict query
    foreach (ASTType*, type, mod->types) {
        if (not strcasecmp(func->name, type->name)) {
            if (func->returnType
                and not(func->flags.isStmt or func->flags.isDefCtor))
                Parser_errorCtorHasType(self, func, type);
            if (not func->returnType) {
                func->returnType = ASTTypeSpec_new(TYObject, CTYNone);
                func->returnType->type = type;
            }
            // TODO: isStmt Ctors should have the correct type so e.g.
            // you cannot have
            // Point(x as Scalar) := 3 + 5 * 12
            // but must return a Point instead. This cannot be enforced
            // here since type resolution hasn't been done at this
            // stage. Check this after the type inference step when the
            // stmt func has its return type assigned.
            // if (func->flags.isStmt)
            //     Parser_errorCtorHasType(this, func, type);
            if (*func->name < 'A' or *func->name > 'Z')
                Parser_warnCtorCase(self, func, type);

            func->name = type->name;
            foundCtor = true;
        }
    }

    // Capitalized names are not allowed unless they are constructors.
    if (not func->flags.isDefCtor and not foundCtor //
        and *func->name >= 'A' and *func->name <= 'Z')
        Parser_errorUnrecognizedCtor(self, func);

    // The rest of the processing is on the contents of the function.
    if (not func->body) {
        func->flags.semPassDone = true;
        return;
    }

    // Check for duplicate functions (same selectors) and report errors.
    // TODO: this should be replaced by a dict query
    foreach (ASTFunc*, func2, mod->funcs) {
        if (func == func2) break;
        if (not strcasecmp(func->selector, func2->selector))
            Parser_errorDuplicateFunc(self, func, func2);
    }

    // Check unused variables in the function and report warnings.
    ASTFunc_checkUnusedVars(self, func);

    // Mark the semantic pass as done for this function, so that recursive
    // calls found in the statements will not cause the compiler to recur.
    func->flags.semPassDone = true;

    // Run the statement-level semantic pass on the function body.
    foreach (ASTExpr*, stmt, func->body->stmts)
        sempass(self, stmt, mod, false);

    // Statement functions are written without an explicit return type.
    // Figure out the type (now that the body has been analyzed).
    if (func->flags.isStmt) setStmtFuncTypeInfo(self, func);
    // TODO: for normal funcs, sempass should check return statements to
    // have the same type as the declared return type.

    // Do optimisations or ANY lowering only if there are no errors
    if (not self->errCount) {
        // Handle elemental operations like arr[4:50] = mx[14:60] + 3
        ASTScope_lowerElementalOps(func->body);
        // Extract subexprs like count(arr[arr<1e-15]) and promote them to
        // full statements corresponding to their C macros e.g.
        // Scalar _1; Array_count_filter(arr, arr<1e-15, _1);
        ASTScope_promoteCandidates(func->body);
    }
}