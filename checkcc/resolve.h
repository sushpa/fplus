
static void resolveTypeSpec(
    Parser* this, ASTTypeSpec* typeSpec, ASTModule* mod)
{
    // TODO: disallow a type that derives from itself!
    if (typeSpec->typeType != TYUnresolved) return;
    if (not*typeSpec->name) return;

    // TODO: DO THIS IN PARSE... stuff!!

    TypeTypes tyty = TypeType_byName(typeSpec->name);
    if (tyty) { // can be member of ASTTypeSpec!
        typeSpec->typeType = tyty;
    } else {
        foreach (ASTType*, type, mod->types) {
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

static void ASTScope_checkUnusedVars(Parser* this, ASTScope* scope)
{
    foreach (ASTVar*, var, scope->locals)
        if (not var->flags.used) Parser_warnUnusedVar(this, var);

    foreach (ASTExpr*, stmt, scope->stmts)
        if (isCtrlExpr(stmt) and stmt->body)
            ASTScope_checkUnusedVars(this, stmt->body);
}

static void ASTFunc_checkUnusedVars(Parser* this, ASTFunc* func)
{
    foreach (ASTVar*, arg, func->args)
        if (not arg->flags.used) Parser_warnUnusedArg(this, arg);

    ASTScope_checkUnusedVars(this, func->body);
}

// TODO: Btw there should be a module level scope to hold lets (and
// comments). That will be the root scope which has parent==NULL.

// This function is called once, after the types of basic elements
// (literals, resolved vars, operators on basic elements, basically
// everything except func calls) is assigned. Then after the call to this
// function, the type assignment pass repeats so that exprs with
// now-resolved func calls can have their types assigned.
// WAIT -- WORKS FOR NOW -- OR NOT

#if 0
static void resolveFuncCalls(
    Parser* this, ASTExpr* expr, ASTModule* mod)
{ // TODO: what happens if you get a tkSubscriptResolved?

    switch (expr->kind) {
    case tkFunctionCallResolved:
        if (ASTExpr_countCommaList(expr->left) != expr->func->argCount)
            Parser_errorArgsCountMismatch(this, expr);
        break;

    case tkSubscriptResolved:
    case tkSubscript:
        if (expr->left) resolveFuncCalls(this, expr->left, mod);
        break;

    case tkFunctionCall: {
        char buf[128] = {};
        char* bufp = buf;
        if (expr->left) resolveFuncCalls(this, expr->left, mod);

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
                resolveFuncCalls(this, expr, mod);
                return;
            }
        } // since it is known which module the func must be found in,
          // no need to scan others if function has not been found
        Parser_errorUnrecognizedFunc(this, expr, buf);
        foreach (ASTFunc*, func, mod->funcs)
            if (not strcasecmp(expr->name, func->name))
                eprintf("        \e[;2mnot viable: \e[34m%s\e[0;2m (%d "
                        "args) at ./%s:%d\e[0m\n",
                    func->selector, func->argCount, this->filename,
                    func->line);
        // but still check nested func calls
    } break;

    case tkVarAssign:
        if (not expr->var->init)
            Parser_errorMissingInit(this, expr);
        else {
            if (!*expr->var->typeSpec->name)
                resolveTypeSpec(this, expr->var->typeSpec, mod);
            resolveFuncCalls(this, expr->var->init, mod);
        }
        break;

    case tkKeyword_else:
    case tkKeyword_if:
    case tkKeyword_for:
    case tkKeyword_while: {
        if (expr->left) resolveFuncCalls(this, expr->left, mod);
        foreach (ASTExpr*, stmt, expr->body->stmts)
            resolveFuncCalls(this, stmt, mod);
    } break;

    default:
        if (expr->opPrec) {
            if (not expr->opIsUnary)
                resolveFuncCalls(this, expr->left, mod);
            resolveFuncCalls(this, expr->right, mod);
        }
    }
}
#endif

// This function is called in one pass, during the line-by-line parsing.
// (since variables cannot be "forward-declared").
static void resolveVars(
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
    case tkIdentifierResolved:
        break;

    case tkIdentifier:
    case tkSubscript: {
        TokenKind ret = (expr->kind == tkIdentifier) ? tkIdentifierResolved
                                                     : tkSubscriptResolved;
        ASTScope* scp = scope;
        do {
            foreach (ASTVar*, local, scp->locals) {
                if (not strcasecmp(expr->name, local->name)) {
                    expr->kind = ret;
                    expr->var = local; // this overwrites name btw
                    goto getout;
                }
            }
            scp = scp->parent;
        } while (scp);
        Parser_errorUnrecognizedVar(this, expr);
    getout:
        expr->var->flags.used = true;
        if (ret == tkSubscriptResolved) {
            resolveVars(this, expr->left, scope, inFuncCall);
            // check subscript argument count
            // recheck kind since the var may have failed resolution
            if (expr->kind == tkSubscriptResolved
                and ASTExpr_countCommaList(expr->left)
                    != expr->var->typeSpec->dims)
                Parser_errorIndexDimsMismatch(this, expr);
        }
        break;
    }
    case tkFunctionCall:
        if (expr->left) resolveVars(this, expr->left, scope, true);
        break;

        // case tkOpAssign:
        //         // behaves differently inside a func call and otherwise

        //         if (not inFuncCall)
        //             resolveVars(this, expr->left, scope, inFuncCall);
        //     resolveVars(this, expr->right, scope, inFuncCall);
        //             if (expr->kind == tkPlusEq or expr->kind == tkMinusEq
        //         or expr->kind == tkSlashEq or expr->kind == tkTimesEq
        //         or expr->kind == tkPowerEq
        //         or expr->kind == tkOpModEq
        //             and (expr->left->kind == tkIdentifierResolved
        //                 or expr->left->kind == tkSubscriptResolved))
        //         expr->var->flags.changed = true;
        //     break;

    default:
        if (expr->opPrec) {
            if (not expr->opIsUnary
                and not(inFuncCall and expr->kind == tkOpAssign))
                resolveVars(this, expr->left, scope, inFuncCall);
            resolveVars(this, expr->right, scope, inFuncCall);
        }
        if (expr->kind == tkPlusEq or expr->kind == tkMinusEq
            or expr->kind == tkSlashEq or expr->kind == tkTimesEq
            or expr->kind == tkPowerEq or expr->kind == tkOpModEq
            or expr->kind == tkOpAssign
                and (expr->left->kind == tkIdentifierResolved
                    or expr->left->kind == tkSubscriptResolved)) {
            expr->left->var->flags.changed = true;
            if (not expr->left->var->flags.isVar)
                Parser_errorReadOnlyVar(this, expr->left);
        }
    }
}
