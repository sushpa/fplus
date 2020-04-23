void checkBinOpTypeMismatch(Parser* self, ASTExpr* expr) {}

void setExprTypeInfo(Parser* self, ASTExpr* expr)
{
    switch (expr->kind) {
    case TKIdentifierResolved:
        expr->typeType = expr->var->typeSpec->typeType;
        break;
    case TKSubscriptResolved:
        expr->typeType = expr->var->typeSpec->typeType;
        setExprTypeInfo(self, expr->left); // check args
        // TODO: check args in the same way as for funcs below, not directly
        // checking expr->left.
        break;
    case TKFunctionCallResolved: { // TODO: so many ifs and buts. Just
                                   // create a typeSpec for the func
        // while parsing it and set it to unresolved.
        if (expr->func->returnType)
            expr->typeType = expr->func->returnType->typeType;
        else
            expr->typeType = TYUnresolved;
        // TODO: don't just descend into ->left, take each arg one by one
        // and compare it with each func->arg. Otherwise you are basically
        // just asking the comma op to verify both its operands match.
        if (expr->left) {
            setExprTypeInfo(self, expr->left);
            ASTExpr* currArg = expr->left;
            // if (currArg->kind != TKOpComma) {
            //     // single arg
            //     ASTVar* arg = *expr->func->args->item;
            //     if (currArg->typeType != arg->typeSpec->typeType)
            //         Parser_errorArgTypeMismatch(self, currArg, arg);
            // } else {
            foreach (ASTVar*, arg, args, expr->func->args) {
                ASTExpr* cArg
                    = currArg->kind == TKOpComma ? currArg->left : currArg;
                if (cArg->kind == TKOpAssign) cArg = cArg->right;
                if (currArg->typeType != arg->typeSpec->typeType)
                    Parser_errorArgTypeMismatch(self, currArg, arg);
                if (not(currArg = currArg->right)) break;
            }
        }
        // }
    } break;
    case TKIdentifier:
        break;
    case TKSubscript:
    case TKFunctionCall:
        eprintf(
            "\nissue in setExprTypeInfo: can't handle unresolved '%s'\n",
            expr->name);
        // assert(0);
        break;
    case TKString:
        expr->typeType = TYString;
        break;
    // case TKRegex:
    //     expr->typeType = TYRegex;
    //     break;
    case TKNumber:
        expr->typeType = TYReal64;
        break;
    // case TKMultiDotNumber:
    case TKVarAssign:
        // set the variable's typespec here
        setExprTypeInfo(self, expr->var->init);
        if (not expr->var->typeSpec->typeType)
            expr->var->typeSpec->typeType = expr->var->init->typeType;
        else if (expr->var->typeSpec->typeType != expr->var->init->typeType)
            Parser_errorTypeMismatchBinOp(self, expr);
        break;

    case TKKeyword_if:
    case TKKeyword_for:
    case TKKeyword_else:
    case TKKeyword_while: {
        if (expr->left) setExprTypeInfo(self, expr->left);
        foreach (ASTExpr*, stmt, stmts, expr->body->stmts)
            setExprTypeInfo(self, stmt);
    } break;

    default:
        if (expr->opPrec) {
            if (not expr->opIsUnary) setExprTypeInfo(self, expr->left);
            setExprTypeInfo(self, expr->right);
            expr->typeType = expr->right->typeType;

            if (not expr->opIsUnary) {
                if (expr->left->typeType != expr->right->typeType)
                    Parser_errorTypeMismatchBinOp(self, expr);
                // TODO: as it stands, "x" + "y" wont be an error because
                // types are consistent. BUT types should also be valid for
                // that operator: in general operators are defined only for
                // numeric types and some keywords for logicals.
                else if (not TypeType_isnum(expr->left->typeType))
                    Parser_errorInvalidTypeForOp(self, expr);
            }
            // TODO: here statements like return etc. that are not binary
            // but need to have their types checked w.r.t. an expected type
            // TODO: some ops have a predefined type e.g. : is of type Range
            // etc,
        } else
            assert(0);
    }
}
