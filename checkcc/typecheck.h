static void checkBinOpTypeMismatch(Parser* self, ASTExpr* expr) {}

static void setStmtFuncTypeInfo(Parser* self, ASTFunc* func)
{
    // this assumes that setExprTypeInfo has been called on the func body
    const ASTExpr* stmt = func->body->stmts->item;
    if (not func->returnType->typeType)
        func->returnType->typeType = stmt->typeType;
    else if (func->returnType->typeType != stmt->typeType)
        Parser_errorTypeMismatchBinOp(self, stmt);
}

static void setExprTypeInfo(Parser* self, ASTExpr* expr, bool inFuncArgs)
{ // return;
    switch (expr->kind) {
    case TKIdentifierResolved:
        expr->typeType = expr->var->typeSpec->typeType;
        expr->isElementalOp = false;
        break;
    case TKSubscriptResolved:
        expr->typeType = expr->var->typeSpec->typeType;
        setExprTypeInfo(self, expr->left, false); // check args
        expr->isElementalOp = expr->left->isElementalOp;
        // TODO: check args in the same way as for funcs below, not directly
        // checking expr->left.
        break;
    case TKFunctionCallResolved: { // TODO: so many ifs and buts. Just
                                   // create a typeSpec for the func
        // while parsing it and set it to unresolved.
        if (expr->func->returnType)
            expr->typeType = expr->func->returnType->typeType;
        else
            expr->typeType = TYUnresolved; // should actually be TYVoid

        if (expr->left) {
            setExprTypeInfo(
                self, expr->left, true); // but this call shouldn't check
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
                    = currArg->kind == TKOpComma ? currArg->left : currArg;
                if (cArg->kind == TKOpAssign) cArg = cArg->right;
                if (cArg->typeType != arg->typeSpec->typeType)
                    Parser_errorArgTypeMismatch(self, cArg, arg);
                if (not(currArg = currArg->right)) break;
            }
        }
    } break;
    case TKIdentifier:
        break;
    case TKSubscript:
    case TKFunctionCall:
        eprintf(
            "\nissue in setExprTypeInfo: can't handle unresolved '%s'\n",
            expr->name);
        break;
    case TKString:
        expr->typeType = TYString;
        expr->isElementalOp = false;
        break;
    case TKNumber:
        expr->typeType = TYReal64;
        expr->isElementalOp = false;
        break;
    case TKVarAssign:
        setExprTypeInfo(self, expr->var->init, false);
        expr->typeType = expr->var->init->typeType;
        expr->isElementalOp = expr->var->init->isElementalOp;

        if (not expr->var->typeSpec->typeType)
            expr->var->typeSpec->typeType = expr->var->init->typeType;
        else if (expr->var->typeSpec->typeType != expr->var->init->typeType)
            Parser_errorTypeMismatchBinOp(self, expr);
        break;

    case TKKeyword_if: // elemental ops in if cond?
    case TKKeyword_for:
    case TKKeyword_else:
    case TKKeyword_while: {
        if (expr->left) setExprTypeInfo(self, expr->left, false);
        foreach (ASTExpr*, stmt, expr->body->stmts)
            setExprTypeInfo(self, stmt, false);
    } break;

    default:
        if (expr->opPrec) {
            if (not expr->opIsUnary)
                setExprTypeInfo(self, expr->left, inFuncArgs);
            setExprTypeInfo(self, expr->right, inFuncArgs);

            if (expr->kind == TKKeyword_or
                and expr->left->typeType != TYBool) {
                ; // this is the x = func(...) or break / or continue / or
                  // return (null handling)
            } else if (expr->kind == TKOpLE or expr->kind == TKOpLT
                or expr->kind == TKOpGT or expr->kind == TKOpGE
                or expr->kind == TKOpEQ or expr->kind == TKOpNE
                or expr->kind == TKKeyword_and or expr->kind == TKKeyword_or
                or expr->kind == TKKeyword_not
                or expr->kind == TKKeyword_in)
                expr->typeType = TYBool;
            else
                expr->typeType = expr->right->typeType;

            expr->isElementalOp
                = expr->right->isElementalOp or expr->kind == TKOpColon;
            // TODO: actually, indexing by an array of integers is also an
            // indication of an elemental op
            if (not expr->opIsUnary)
                expr->isElementalOp
                    = expr->isElementalOp or expr->left->isElementalOp;

            if (not expr->opIsUnary
                and not(inFuncArgs
                    and (expr->kind == TKOpComma
                        or expr->kind == TKOpAssign))) {
                TypeTypes leftType = expr->left->typeType;
                TypeTypes rightType = expr->right->typeType;

                // ignore , and = inside function call arguments.
                // thing is array or dict literals passed as args will have
                // , and = which should be checked. so when you descend into
                // their args, unset inFuncArgs.

                if (leftType == TYBool
                    and (expr->kind == TKOpLE or expr->kind == TKOpLT))
                    ;
                else if (leftType != rightType)
                    Parser_errorTypeMismatchBinOp(self, expr);
                // TODO: as it stands, "x" + "y" wont be an error because
                // types are consistent. BUT types should also be valid for
                // that operator: in general operators are defined only for
                // numeric types and some keywords for logicals.
                else if (leftType == TYString and //
                    (expr->kind == TKOpAssign //
                        or expr->kind == TKOpEQ //
                        or expr->kind == TKPlusEq))
                    ;
                else if (leftType == TYBool and //
                    (expr->kind == TKOpAssign //
                        or expr->kind == TKOpEQ
                        or expr->kind == TKKeyword_and
                        or expr->kind == TKKeyword_or))
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
