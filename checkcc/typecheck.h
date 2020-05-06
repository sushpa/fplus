static void checkBinOpTypeMismatch(Parser* self, ASTExpr* expr) {}

// TODO: rework type distribution.
// It needs to be 2-pass: first the types of everything except function
// calls is set, then in the second pass the exprs with func calls and those
// above them are assigned types. This is because in order to resolve
// functions, the type of the first arg is required, and thus func
// resolution cannot happen before type distribution (first pass). So the
// order is first type resolution, then name resolution, then second type
// resolution.

// TODO: call this check-or-set-func-type and run it on all funcs to ensure
// funcs return what they declare
static void setStmtFuncTypeInfo(Parser* self, ASTFunc* func)
{
    // this assumes that setExprTypeInfo has been called on the func body
    const ASTExpr* stmt = func->body->stmts->item;
    if (not func->returnType->typeType)
        func->returnType->typeType = stmt->typeType;
    else if (func->returnType->typeType != stmt->typeType)
        Parser_errorTypeMismatchBinOp(self, stmt);
}

#if 0
static void setExprTypeInfo(
    Parser* self, ASTExpr* expr, bool inFuncArgs)
{ // return;
    switch (expr->kind) {
    case tkIdentifierResolved:
        expr->typeType = expr->var->typeSpec->typeType;
        expr->isElementalOp = false;
        break;
    case tkSubscriptResolved:
        expr->typeType = expr->var->typeSpec->typeType;
        setExprTypeInfo(self, expr->left, false); // check args
        expr->isElementalOp = expr->left->isElementalOp;
        // TODO: check args in the same way as for funcs below, not directly
        // checking expr->left.
        break;
    case tkFunctionCallResolved: {
        // TODO: so many ifs and buts. Just
        // create a typeSpec for the func
        // while parsing it and set it to unresolved.
        if (expr->func->returnType)
            expr->typeType = expr->func->returnType->typeType;
        else
            expr->typeType = TYUnresolved; // should actually be TYVoid

        if (expr->left) {
            setExprTypeInfo(self, expr->left, true);
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
        }
    } break;
    case tkIdentifier:
        break;
    case tkSubscript:
    case tkFunctionCall:
        // allow this: the first pass will fall here and silently pass
        // eprintf(
        //     "\nissue in setExprTypeInfo: can't handle unresolved '%s'\n",
        //     expr->name);
        break;
    case tkString:
        expr->typeType = TYString;
        expr->isElementalOp = false;
        break;
    case tkNumber:
        expr->typeType = TYReal64;
        expr->isElementalOp = false;
        break;
    case tkVarAssign:
        setExprTypeInfo(self, expr->var->init, false);
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
            }
        } else if (expr->var->typeSpec->typeType
            != expr->var->init->typeType)
            Parser_errorTypeMismatchBinOp(self, expr);
        break;

    case tkKeyword_if: // elemental ops in if cond?
    case tkKeyword_for:
    case tkKeyword_else:
    case tkKeyword_while: {
        if (expr->left) setExprTypeInfo(self, expr->left, false);
        foreach (ASTExpr*, stmt, expr->body->stmts)
            setExprTypeInfo(self, stmt, false);
    } break;

    default:
        if (expr->opPrec) {
            if (not expr->opIsUnary)
                setExprTypeInfo(self, expr->left, inFuncArgs);
            setExprTypeInfo(self, expr->right, inFuncArgs);

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
#endif