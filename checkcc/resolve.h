
void resolveTypeSpec(Parser* this, ASTTypeSpec* typeSpec, ASTModule* mod)
{
    // TODO: disallow a type that derives from itself!
    if (typeSpec->typeType != TYUnresolved) return;

    // TODO: DO THIS IN PARSE... stuff!!

    TypeTypes tyty = TypeType_byName(typeSpec->name);
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
        char buf[128];
        char* bufp = buf;
        if (expr->left) resolveFuncsAndTypes(this, expr->left, mod);

        ASTExpr* arg1 = expr->left;

        if (arg1 and arg1->kind == TKOpComma) arg1 = arg1->left;
        if (arg1) {
            bufp += sprintf(bufp, "%s_", ASTExpr_typeName(arg1));
        }
        bufp += sprintf(bufp, "%s", expr->name);
        ASTExpr_strarglabels(expr->left, bufp, 128 - ((int)(bufp - buf)));

        foreach (ASTFunc*, func, funcs, mod->funcs) {
            if (not strcasecmp(buf, func->selector)) {
                expr->kind = TKFunctionCallResolved;
                expr->func = func;
                resolveFuncsAndTypes(this, expr, mod);
                return;
            }
        } // since it is known which module the func must be found in,
          // no need to scan others if function has not been found
        Parser_errorUnrecognizedFunc(this, expr, buf);
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

    case TKKeyword_else:
    case TKKeyword_if:
    case TKKeyword_for:
    case TKKeyword_while: {
        if (expr->left) resolveFuncsAndTypes(this, expr->left, mod);
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

void resolveVars(
    Parser* this, ASTExpr* expr, ASTScope* scope, bool inFuncCall)
{ // TODO: this could be done on rpn in parseExpr, making it iterative
  // instead of recursive
  // = behaves differently inside a func call: the ->left is not
  // resolved to a var, but to an argument label of the called func.
  // it would make sense to just skip checking it here for now, and
  // let resolveFuncs use it to construct the func selector for lookup.
  // At some point though it would be nice if the compiler could tell
  // the user 'you missed the arg label "xyz"' for which the basename
  // of the func could be used to get a list of all selectors having
  // that basename as a prefix.

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
            resolveVars(this, expr->left, scope, inFuncCall);
            // check subscript argument count
            // recheck kind since the var may have failed resolution
            if (expr->kind == TKSubscriptResolved
                and ASTExpr_countCommaList(expr->left)
                    != expr->var->typeSpec->dims)
                Parser_errorIndexDimsMismatch(this, expr);
        }
        break;
    }
    case TKFunctionCall:
        if (expr->left) resolveVars(this, expr->left, scope, true);
        break;

    case TKOpAssign:
        // behaves differently inside a func call and otherwise
        if (not inFuncCall)
            resolveVars(this, expr->left, scope, inFuncCall);
        resolveVars(this, expr->right, scope, inFuncCall);
        break;

    default:
        if (expr->opPrec) {
            if (not expr->opIsUnary)
                resolveVars(this, expr->left, scope, inFuncCall);
            resolveVars(this, expr->right, scope, inFuncCall);
        }
    }
}
