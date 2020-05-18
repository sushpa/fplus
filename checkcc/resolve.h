
static bool isSelfMutOp(ASTExpr* expr)
{
    return expr->kind == tkPlusEq or expr->kind == tkMinusEq
        or expr->kind == tkSlashEq or expr->kind == tkTimesEq
        or expr->kind == tkPowerEq or expr->kind == tkOpModEq
        or expr->kind == tkOpAssign;
}

static void resolveTypeSpec(
    Parser* parser, ASTTypeSpec* typeSpec, ASTModule* mod)
{
    // TODO: disallow a type that derives from itself!
    if (typeSpec->typeType != TYUnresolved) return;
    if (not*typeSpec->name) return;

    // TODO: DO THIS IN PARSE... stuff!!

    TypeTypes tyty = TypeType_byName(typeSpec->name);
    if (tyty) { // can be member of ASTTypeSpec!
        typeSpec->typeType = tyty;
    } else {
        ASTType* type = ASTModule_getType(mod, typeSpec->name);
        if (type) {
            typeSpec->typeType = TYObject;
            typeSpec->type = type;
            return;
        }
        Parser_errorUnrecognizedType(parser, typeSpec);
        return;
    }
    if (typeSpec->dims) {
        // set collection type, etc.
        // for this we will need info about the var and its usage
        // patterns. so this will probably be a separate func that is
        // called during such analysis.
    }
}

static void ASTScope_checkUnusedVars(Parser* parser, ASTScope* scope)
{
    foreach (ASTVar*, var, scope->locals)
        if (not var->used) Parser_warnUnusedVar(parser, var);

    foreach (ASTExpr*, stmt, scope->stmts)
        if (isCtrlExpr(stmt) and stmt->body)
            ASTScope_checkUnusedVars(parser, stmt->body);
}

static void ASTFunc_checkUnusedVars(Parser* parser, ASTFunc* func)
{
    foreach (ASTVar*, arg, func->args)
        if (not arg->used) Parser_warnUnusedArg(parser, arg);

    ASTScope_checkUnusedVars(parser, func->body);
}

static void ASTTest_checkUnusedVars(Parser* parser, ASTTest* test)
{
    ASTScope_checkUnusedVars(parser, test->body);
}

// TODO: Btw there should be a module level scope to hold lets (and
// comments). That will be the root scope which has parent==NULL.

static void resolveMember(Parser* parser, ASTExpr* expr, ASTType* type)

{
    assert(expr->kind == tkIdentifier or expr->kind == tkSubscript);
    TokenKind ret = (expr->kind == tkIdentifier) ? tkIdentifierResolved
                                                 : tkSubscriptResolved;
    ASTVar* found = ASTScope_getVar(type->body, expr->string);
    if (found) {
        expr->kind = ret;
        expr->var = found;
        expr->var->used = true;
    } else {
        Parser_errorUnrecognizedMember(parser, type, expr);
    }
}

// This function is called in one pass, during the line-by-line parsing.
// (since variables cannot be "forward-declared").
static void resolveVars(
    Parser* parser, ASTExpr* expr, ASTScope* scope, bool inFuncCall)
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
        ASTVar* found = ASTScope_getVar(scope, expr->string);
        if (found) {
            expr->kind = ret;
            expr->var = found;
            expr->var->used = true;
        } else {
            Parser_errorUnrecognizedVar(parser, expr);
        }
        if (expr->kind == tkSubscriptResolved or expr->kind == tkSubscript) {
            resolveVars(parser, expr->left, scope, inFuncCall);
            // check subscript argument count
            // recheck kind since the var may have failed resolution
            // TODO: handle dims 0 as dims 1 because arr[] is the same as arr[:]
            if (expr->kind == tkSubscriptResolved
                and ASTExpr_countCommaList(expr->left)
                    != expr->var->typeSpec->dims)
                Parser_errorIndexDimsMismatch(parser, expr);
        }
        break;
    }
    case tkFunctionCall:
        if (expr->left) resolveVars(parser, expr->left, scope, true);
        break;

    case tkPeriod:
        if (expr->left) resolveVars(parser, expr->left, scope, inFuncCall);
        // expr->right is not to be resolved in the same scope, but in
        // the type body of the type of expr->left. So you cannot call
        // resolveVars on expr->right from here. Neither can you assume
        // that types have been resolved, because name resolution
        // happens as the file is read, while type resolution happens
        // only after the tree is fully built. The way to fix it is to
        // have analyseExpr call the name resolution (as it already does for
        // exprs like a.b but not a.b.c) for expr->right, AFTER the type
        // for expr->left has been resolved.
        //        if (expr->right->kind==tkPeriod) resolveVars(this,
        //        expr->right, scope, inFuncCall);
        // besides an ident, the ->right of a . can be either another
        // dot, a subscript, or a func call if we allow member funcs
        if (expr->right->kind == tkSubscript
            or expr->right->kind == tkSubscriptResolved)
            resolveVars(parser, expr->right->left, scope, inFuncCall);

        break;

    default:
        if (expr->prec) {
            if (not expr->unary
                and not(inFuncCall and expr->kind == tkOpAssign))
                resolveVars(parser, expr->left, scope, inFuncCall);
            resolveVars(parser, expr->right, scope, inFuncCall);
        }
        if (isSelfMutOp(expr)
            and (expr->left->kind == tkIdentifierResolved
                or expr->left->kind == tkSubscriptResolved)) {
            // TODO: If you will allow changing the first arg of a function,
            // using an & op or whatever, check for those mutations here
            expr->left->var->changed = true;
            if (not expr->left->var->isVar)
                Parser_errorReadOnlyVar(parser, expr->left);
        }
    }
}
