// TODO make sempassModule -> same as analyseModule now

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
    // nothing to do for declared/empty types etc. with no body
    if (type->body) foreach (ASTExpr*, stmt, type->body->stmts)
            sempass(self, stmt, mod, false);
}

static void sempassFunc(Parser* self, ASTFunc* func, ASTModule* mod)
{
    if (not func->body) return;
    // TODO: this should be replaced by a dict query
    foreach (ASTFunc*, func2, mod->funcs) {
        if (func == func2) break;
        if (not strcasecmp(func->selector, func2->selector))
            Parser_errorDuplicateFunc(self, func, func2);
    }

    ASTFunc_checkUnusedVars(self, func);
    foreach (ASTExpr*, stmt, func->body->stmts)
        sempass(self, stmt, mod, false);

    if (func->flags.isStmt) setStmtFuncTypeInfo(self, func);

    if (not self->errCount) {
        ASTScope_lowerElementalOps(func->body);
        ASTScope_promoteCandidates(func->body);
    }
}