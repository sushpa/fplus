static void checkBinOpTypeMismatch(Parser* self, ASTExpr* expr) {}

static void setExprTypeInfo(Parser* self, ASTExpr* expr, bool inFuncArgs)
{
    switch (expr->kind) {
    case TKIdentifierResolved:
        expr->typeType = expr->var->typeSpec->typeType;
        break;
    case TKSubscriptResolved:
        expr->typeType = expr->var->typeSpec->typeType;
        setExprTypeInfo(self, expr->left, false); // check args
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
            ASTExpr* currArg = expr->left;
            foreach (ASTVar*, arg, args, expr->func->args) {
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
        break;
    case TKNumber:
        expr->typeType = TYReal64;
        break;
    case TKVarAssign:
        setExprTypeInfo(self, expr->var->init, false);
        if (not expr->var->typeSpec->typeType)
            expr->var->typeSpec->typeType = expr->var->init->typeType;
        else if (expr->var->typeSpec->typeType != expr->var->init->typeType)
            Parser_errorTypeMismatchBinOp(self, expr);
        break;

    case TKKeyword_if:
    case TKKeyword_for:
    case TKKeyword_else:
    case TKKeyword_while: {
        if (expr->left) setExprTypeInfo(self, expr->left, false);
        foreach (ASTExpr*, stmt, stmts, expr->body->stmts)
            setExprTypeInfo(self, stmt, false);
    } break;

    default:
        if (expr->opPrec) {
            if (not expr->opIsUnary)
                setExprTypeInfo(self, expr->left, inFuncArgs);
            setExprTypeInfo(self, expr->right, inFuncArgs);
            expr->typeType = expr->right->typeType;

            if (not expr->opIsUnary
                and not(inFuncArgs
                    and (expr->kind == TKOpComma
                        or expr->kind == TKOpAssign))) {
                // ignore , and = inside function calls.
                // thing is array or dict literals passed as args will have
                // , and = which should be checked. so when you descend into
                // their args, unset inFuncArgs.

                if (expr->left->typeType != expr->right->typeType)
                    Parser_errorTypeMismatchBinOp(self, expr);
                // TODO: as it stands, "x" + "y" wont be an error because
                // types are consistent. BUT types should also be valid for
                // that operator: in general operators are defined only for
                // numeric types and some keywords for logicals.
                else if (not TypeType_isnum(expr->left->typeType))
                    Parser_errorInvalidTypeForOp(self, expr);

                expr->typeType = expr->left->typeType;
            }
            // TODO: here statements like return etc. that are not binary
            // but need to have their types checked w.r.t. an expected type
            // TODO: some ops have a predefined type e.g. : is of type Range
            // etc,
        } else
            assert(0);
    }
}
