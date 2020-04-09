
void resolveTypeSpec(Parser* this, ASTTypeSpec* typeSpec, ASTModule* mod)
{
    // TODO: disallow a type that derives from itself!
    if (typeSpec->typeType != TYUnresolved) return;

    // TODO: DO THIS IN PARSE... stuff!!

    TypeTypes tyty = TypeType_TypeTypeforSpec(typeSpec->name);
    if (tyty) { // can be member of ASTTypeSpec!
        typeSpec->typeType = tyty;
    } else {
        foreach (ASTType*, type, types, mod->types) {
            if (not strcasecmp(typeSpec->name, type->name)) {
                // so what do you do  if types are "resolved"? Set
                // typeType and collectionType?
                //                printf("%s matched")
                typeSpec->typeType = TYObject;
                typeSpec->type = type;
                return;
            }
        }
        Parser_errorUnrecognizedType(this, typeSpec);
        return;
    }
    if (typeSpec->dims) {
        // set collection type, etc.
        // for this we will need info about the var and its usage
        // patterns. so this will probably be a separate func that is
        // called during such analysis.
    }
}

// TODO: Btw there should be a module level scope to hold lets (and
// comments). That will be the root scope which has parent==NULL.

void resolveFuncsAndTypes(Parser* this, ASTExpr* expr, ASTModule* mod)
{ // TODO: what happens if you get a TKSubscriptResolved?

    switch (expr->kind) {
    case TKFunctionCallResolved:
        if (ASTExpr_countCommaList(expr->left) != expr->func->argCount)
            Parser_errorArgsCountMismatch(this, expr);
        break;

    case TKSubscriptResolved:
    case TKSubscript:
        if (expr->left) resolveFuncsAndTypes(this, expr->left, mod);
        break;

    case TKFunctionCall: {
        foreach (ASTFunc*, func, funcs, mod->funcs) {
            if (not strncasecmp(expr->name, func->name, expr->strLen)
                and func->name[expr->strLen] == '\0') {
                expr->kind = TKFunctionCallResolved;
                expr->func = func;
                if (expr->left) resolveFuncsAndTypes(this, expr->left, mod);
                resolveFuncsAndTypes(this, expr, mod);
                return;
            }
        } // since it is known which module the func must be found in,
          // no need to scan others if function has not been found
        Parser_errorUnrecognizedFunc(this, expr);
        if (expr->left) resolveFuncsAndTypes(this, expr->left, mod);
        // but still check nested func calls
    } break;

    case TKVarAssign:
        if (not expr->var->init)
            Parser_errorMissingInit(this, expr);
        else {
            resolveTypeSpec(this, expr->var->typeSpec, mod);
            resolveFuncsAndTypes(this, expr->var->init, mod);
        }
        break;

    case TKKeyword_if:
    case TKKeyword_for:
    case TKKeyword_while: {
        resolveFuncsAndTypes(this, expr->left, mod);
        foreach (ASTExpr*, stmt, stmts, expr->body->stmts)
            resolveFuncsAndTypes(this, stmt, mod);
    } break;

    default:
        if (expr->opPrec) {
            if (not expr->opIsUnary)
                resolveFuncsAndTypes(this, expr->left, mod);
            resolveFuncsAndTypes(this, expr->right, mod);
        }
    }
}

void resolveVars(Parser* this, ASTExpr* expr, ASTScope* scope)
{ // TODO: this could be done on rpn in parseExpr, making it iterative
  // instead of recursive
    if (not expr) return;
    switch (expr->kind) {
    case TKIdentifierResolved:
        break;

    case TKIdentifier:
    case TKSubscript: {
        TokenKind ret = (expr->kind == TKIdentifier) ? TKIdentifierResolved
                                                     : TKSubscriptResolved;
        ASTScope* scp = scope;
        do {
            foreach (ASTVar*, local, locals, scp->locals) {
                if (not strncasecmp(expr->name, local->name, expr->strLen)
                    and local->name[expr->strLen] == '\0') {
                    expr->kind = ret;
                    expr->var = local; // this overwrites name btw
                    goto getout;
                }
            }
            scp = scp->parent;
        } while (scp);
        Parser_errorUnrecognizedVar(this, expr);
    getout:
        if (ret == TKSubscriptResolved) {
            resolveVars(this, expr->left, scope);
            // check subscript argument count
            if (ASTExpr_countCommaList(expr->left)
                != expr->var->typeSpec->dims)
                Parser_errorIndexDimsMismatch(this, expr);
        }
        break;
    }
    case TKFunctionCall:
        if (expr->left) resolveVars(this, expr->left, scope);
        break;

    default:
        if (expr->opPrec) {
            if (not expr->opIsUnary) resolveVars(this, expr->left, scope);
            resolveVars(this, expr->right, scope);
        }
    }
}
