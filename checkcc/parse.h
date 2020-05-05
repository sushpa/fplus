
#pragma mark - PARSE EXPR
static ASTExpr* parseExpr(Parser* this)
{
    // there are 2 steps to this madness.
    // 1. parse a sequence of tokens into RPN using shunting-yard.
    // 2. walk the rpn stack as a sequence and copy it into a result
    // stack, collapsing the stack when you find nonterminals (ops, func
    // calls, array index, ...)

    static PtrArray rpn, ops, result;
    int prec_top = 0;
    ASTExpr* p = NULL;
    TokenKind revBrkt = TKUnknown;

    // ******* STEP 1 CONVERT TOKENS INTO RPN

    while (this->token.kind != TKNullChar //
        and this->token.kind != TKNewline
        and this->token.kind != TKLineComment) { // build RPN

        // you have to ensure that ops have a space around them, etc.
        // so don't just skip the one spaces like you do now.
        if (this->token.kind == TKOneSpace) Token_advance(&this->token);
        if (this->token.kind == TKIdentifier
            and memchr(this->token.pos, '_', this->token.matchlen))
            Parser_errorInvalidIdent(this); // but continue parsing

        ASTExpr* expr = ASTExpr_fromToken(&this->token); // dont advance yet
        int prec = expr->opPrec;
        bool rassoc = prec ? expr->opIsRightAssociative : false;
        char lookAheadChar = Token_peekCharAfter(&this->token);

        switch (expr->kind) {
        case TKIdentifier:
            switch (lookAheadChar) {
            case '(':
                expr->kind = TKFunctionCall;
                expr->opPrec = 100;
                PtrArray_push(&ops, expr);
                break;
            case '[':
                expr->kind = TKSubscript;
                expr->opPrec = 100;
                PtrArray_push(&ops, expr);
                break;
            default:
                PtrArray_push(&rpn, expr);
                break;
            }
            break;

        case TKParenOpen:
            PtrArray_push(&ops, expr);
            if (not PtrArray_empty(&ops)
                and PtrArray_topAs(ASTExpr*, &ops)->kind == TKFunctionCall)
                PtrArray_push(&rpn, expr);
            if (lookAheadChar == ')') PtrArray_push(&rpn, NULL);
            // for empty func() push null for no args
            break;

        case TKArrayOpen:
            PtrArray_push(&ops, expr);
            if (not PtrArray_empty(&ops)
                and PtrArray_topAs(ASTExpr*, &ops)->kind == TKSubscript)
                PtrArray_push(&rpn, expr);
            if (lookAheadChar == ')') PtrArray_push(&rpn, NULL);
            // for empty arr[] push null for no args
            break;

        case TKParenClose:
        case TKArrayClose:
        case TKBraceClose:

            revBrkt = TokenKind_reverseBracket(expr->kind);
            if (PtrArray_empty(&ops)) {
                // need atleast the opening bracket of the current kind
                Parser_errorParsingExpr(this);
                goto error;
            }

            else
                while (not PtrArray_empty(&ops)) {
                    p = PtrArray_pop(&ops);
                    if (p->kind == revBrkt) break;
                    PtrArray_push(&rpn, p);
                }
            // we'll push the TKArrayOpen as well to indicate a list
            // literal or comprehension
            // TKArrayOpen is a unary op.
            if ((p and p->kind == TKArrayOpen)
                and (PtrArray_empty(&ops)
                    or PtrArray_topAs(ASTExpr*, &ops)->kind != TKSubscript)
                // don't do this if its part of a subscript
                and (PtrArray_empty(&rpn)
                    or PtrArray_topAs(ASTExpr*, &rpn)->kind != TKOpColon))
                // or aa range. range exprs are handled separately. by
                // themselves they don't need a surrounding [], but for
                // grouping like 2+[8:66] they do.
                PtrArray_push(&rpn, p);

            break;
        case TKKeyword_check:
            PtrArray_push(&ops, expr);
            break;
        case TKKeyword_return:
            // for empty return, push a NULL if there is no expr coming.
            PtrArray_push(&ops, expr);
            if (lookAheadChar == '!' or lookAheadChar == '\n')
                PtrArray_push(&rpn, NULL);
            break;
        default:
            if (prec) {

                if (expr->kind == TKOpColon) {
                    if (PtrArray_empty(&rpn)
                        or (!PtrArray_top(&rpn) and !PtrArray_empty(&ops)
                            and PtrArray_topAs(ASTExpr*, &ops)->kind
                                != TKOpColon)
                        or (PtrArray_topAs(ASTExpr*, &rpn)->kind
                                == TKOpColon
                            and !PtrArray_empty(&ops)
                            and (PtrArray_topAs(ASTExpr*, &ops)->kind
                                    == TKOpComma
                                or PtrArray_topAs(ASTExpr*, &ops)->kind
                                    == TKArrayOpen)))
                        // TODO: better way to parse :, 1:, :-1, etc.
                        // while passing tokens to RPN, if you see a :
                        // with nothing on the RPN or comma or [, push a
                        // NULL. while unwinding the op stack, if you
                        // pop a : and see a NULL or comma on the rpn,
                        // push another NULL.
                        PtrArray_push(&rpn, NULL);
                    // indicates empty operand
                }
                while (not PtrArray_empty(&ops)) {
                    prec_top = PtrArray_topAs(ASTExpr*, &ops)->opPrec;
                    if (not prec_top) break; // left parenthesis
                    if (prec > prec_top) break;
                    if (prec == prec_top and rassoc) break;
                    p = PtrArray_pop(&ops);

                    if (p->kind != TKOpComma and p->kind != TKOpSemiColon
                        and p->kind != TKFunctionCall
                        and p->kind != TKSubscript
                        and PtrArray_topAs(ASTExpr*, &rpn)
                        and PtrArray_topAs(ASTExpr*, &rpn)->kind
                            == TKOpComma) {
                        Parser_errorUnexpectedToken(this);
                        goto error;
                    }

                    if (not(p->opPrec or p->opIsUnary)
                        and p->kind != TKFunctionCall
                        and p->kind != TKOpColon and p->kind != TKSubscript
                        and rpn.used < 2) {
                        Parser_errorUnexpectedToken(this);
                        goto error;
                    }

                    PtrArray_push(&rpn, p);
                }

                if (PtrArray_empty(&rpn)) {
                    Parser_errorUnexpectedToken(this);
                    goto error;
                }
                if (expr->kind == TKOpColon
                    and (lookAheadChar == ',' or lookAheadChar == ':'
                        or lookAheadChar == ']' or lookAheadChar == ')'))
                    PtrArray_push(&rpn, NULL);

                PtrArray_push(&ops, expr);
            } else {
                PtrArray_push(&rpn, expr);
            }
        }
        Token_advance(&this->token);
        if (this->token.kind == TKOneSpace) Token_advance(&this->token);
    }
exitloop:

    while (not PtrArray_empty(&ops)) {
        p = PtrArray_pop(&ops);

        if (p->kind != TKOpComma and p->kind != TKFunctionCall
            and p->kind != TKSubscript and p->kind != TKArrayOpen
            and PtrArray_topAs(ASTExpr*, &rpn)
            and PtrArray_topAs(ASTExpr*, &rpn)->kind == TKOpComma) {
            Parser_errorUnexpectedExpr(
                this, PtrArray_topAs(ASTExpr*, &rpn));
            goto error;
        }

        if (not(p->opPrec or p->opIsUnary)
            and (p->kind != TKFunctionCall and p->kind != TKSubscript)
            and rpn.used < 2) {
            Parser_errorParsingExpr(this);
            goto error;
            // TODO: even if you have more than two, neither of the top
            // two should be a comma
        }

        PtrArray_push(&rpn, p);
    }

    // *** STEP 2 CONVERT RPN INTO EXPR TREE

    ASTExpr* arg;
    for (int i = 0; i < rpn.used; i++) {
        if (not(p = rpn.ref[i])) goto justpush;
        switch (p->kind) {
        case TKFunctionCall:
        case TKSubscript:
            if (result.used > 0) {
                arg = PtrArray_pop(&result);
                p->left = arg;
            }
            break;

        case TKNumber:
        case TKString:
        case TKRegex:
        case TKInline:
        case TKUnits:
        case TKMultiDotNumber:
        case TKIdentifier:
        case TKParenOpen:
            break;

        default:
            // everything else is a nonterminal, needs left/right
            if (not p->opPrec) {
                Parser_errorParsingExpr(this);
                goto error;
            }

            if (PtrArray_empty(&result)) {
                Parser_errorParsingExpr(this);
                goto error;
            }

            p->right = PtrArray_pop(&result);

            if (not p->opIsUnary) {
                if (PtrArray_empty(&result)) {
                    Parser_errorParsingExpr(this);
                    goto error;
                }
                p->left = PtrArray_pop(&result);
            }
        }
    justpush:
        PtrArray_push(&result, p);
    }
    if (result.used != 1) {
        Parser_errorParsingExpr(this);
        goto error;
    }

    ops.used = 0;
    rpn.used = 0;
    result.used = 0;
    return result.ref[0];

error:

    while (this->token.pos < this->end
        and (this->token.kind != TKNewline
            and this->token.kind != TKLineComment))
        Token_advance(&this->token);

    if (ops.used) {
        printf("      ops: ");
        for (int i = 0; i < ops.used; i++)
            printf(
                "%s ", TokenKind_repr(((ASTExpr*)ops.ref[i])->kind, false));
        puts("");
    }

    if (rpn.used) {
        printf("      rpn: ");
        for (int i = 0; i < rpn.used; i++)
            if (not rpn.ref[i])
                printf("NUL ");
            else {
                ASTExpr* e = rpn.ref[i];
                printf("%s ",
                    e->opPrec ? TokenKind_repr(e->kind, false) : e->name);
            }
        puts("");
    }

    if (result.used) {
        printf("      result: ");
        for (int i = 0; i < result.used; i++)
            if (not result.ref[i])
                printf("NUL ");
            else {
                ASTExpr* e = result.ref[i];
                printf("%s ",
                    e->opPrec ? TokenKind_repr(e->kind, false) : e->name);
            }
        puts("");
    }

    if (p) {
        printf("      p: %s ",
            p->opPrec ? TokenKind_repr(p->kind, false) : p->name);
        puts("");
    }

    ops.used = 0; // "reset" stacks
    rpn.used = 0;
    result.used = 0;
    return NULL;
}

#pragma mark - PARSE TYPESPEC
static ASTTypeSpec* parseTypeSpec(Parser* this)
{
    this->token.flags.mergeArrayDims = true;

    ASTTypeSpec* typeSpec = NEW(ASTTypeSpec);
    typeSpec->line = this->token.line;
    typeSpec->col = this->token.col;

    if (memchr(this->token.pos, '_', this->token.matchlen))
        Parser_errorInvalidIdent(this);

    typeSpec->name = parseIdent(this);

    if (matches(this, TKArrayDims)) {
        for (int i = 0; i < this->token.matchlen; i++)
            if (this->token.pos[i] == ':') typeSpec->dims++;
        if (not typeSpec->dims) typeSpec->dims = 1;
        Token_advance(&this->token);
    }

    Parser_ignore(this, TKUnits);

    assert(this->token.kind != TKUnits);
    assert(this->token.kind != TKArrayDims);

    this->token.flags.mergeArrayDims = false;
    return typeSpec;
}

#pragma mark - PARSE VAR
static ASTVar* parseVar(Parser* this)
{
    ASTVar* var = NEW(ASTVar);
    var->flags.isVar = (this->token.kind == TKKeyword_var);
    var->flags.isLet = (this->token.kind == TKKeyword_let);

    if (var->flags.isVar) discard(this, TKKeyword_var);
    if (var->flags.isLet) discard(this, TKKeyword_let);
    if (var->flags.isVar or var->flags.isLet) discard(this, TKOneSpace);

    var->line = this->token.line;
    var->col = this->token.col;

    if (memchr(this->token.pos, '_', this->token.matchlen))
        Parser_errorInvalidIdent(this);
    if (*this->token.pos < 'a' or *this->token.pos > 'z')
        Parser_errorInvalidIdent(this);
    var->name = parseIdent(this);

    if (Parser_ignore(this, TKOneSpace)
        and Parser_ignore(this, TKKeyword_as)) {
        discard(this, TKOneSpace);
        var->typeSpec = parseTypeSpec(this);
    } else {
        var->typeSpec = NEW(ASTTypeSpec);
        var->typeSpec->line = this->token.line;
        var->typeSpec->col = this->token.col;
        var->typeSpec->name = "";
    }

    Parser_ignore(this, TKOneSpace);
    if (Parser_ignore(this, TKOpAssign)) var->init = parseExpr(this);

    return var;
}

static List(ASTVar) * parseArgs(Parser* this)
{
    List(ASTVar)* args = NULL;
    discard(this, TKParenOpen);
    if (Parser_ignore(this, TKParenClose)) return args;

    ASTVar* arg;
    do {
        arg = parseVar(this);
        PtrList_append(&args, arg);
    } while (Parser_ignore(this, TKOpComma));

    discard(this, TKParenClose);
    return args;
}

#pragma mark - PARSE SCOPE
static ASTScope* parseScope(
    Parser* this, ASTScope* parent, bool isTypeBody, bool isIfBlock)
{
    ASTScope* scope = NEW(ASTScope);

    ASTVar *var = NULL, *orig = NULL;
    ASTExpr* expr = NULL;
    TokenKind tt = TKUnknown;

    scope->parent = parent;
    bool startedElse = false;
    while (this->token.kind != TKKeyword_end) {

        switch (this->token.kind) {

        case TKNullChar:
            Parser_errorExpectedToken(this, TKUnknown);
            goto exitloop;

        case TKKeyword_var:
        case TKKeyword_let:
            var = parseVar(this);
            if (not var)
                continue;
            else
                Token_advance(&this->token);
            if ((orig = ASTScope_getVar(scope, var->name)))
                Parser_errorDuplicateVar(this, var, orig);
            // TODO: why only idents and binops for resolveVars??
            if (var->init
                and (var->init->opPrec or var->init->kind == TKIdentifier))
                resolveVars(this, var->init, scope, false);
            // resolveType(var->typeSpec, scope);
            // resolve BEFORE it is added to the list! in
            // `var x = x + 1` x should not resolve
            // if var->typeSpec is NULL then set the type
            // if it isn't NULL then check the types match
            PtrList_append(&scope->locals, var);
            // TODO: validation should raise issue if var->init is
            // missing
            expr = NEW(ASTExpr);
            expr->kind = TKVarAssign;
            expr->line = var->init->line; // this->token.line;
            expr->col = var->init->col;
            expr->opPrec = TokenKind_getPrecedence(TKOpAssign);
            expr->var = var;
            PtrList_append(&scope->stmts, expr);
            break;

        case TKKeyword_else:
            if (not startedElse) goto exitloop;

        case TKKeyword_if:
        case TKKeyword_for:
        case TKKeyword_while:
            if (isTypeBody) Parser_errorInvalidTypeMember(this);
            tt = this->token.kind;
            expr = match(this, tt);
            expr->left = tt != TKKeyword_else ? parseExpr(this) : NULL;

            if (tt == TKKeyword_for) {
                // TODO: new Parser_error
                if (expr->left->kind != TKOpAssign)
                    eprintf("Invalid for-loop condition: %s\n",
                        TokenKind_repr(expr->left->kind, false));
                resolveVars(this, expr->left->right, scope, false);
            } else if (expr->left) {
                resolveVars(this, expr->left, scope, false);
            } // TODO: `for` necessarily introduces a counter variable, so
            // check if that var name doesn't already exist in scope.
            // Also assert that the cond of a for expr has kind
            // TKOpAssign.
            // insert a temp scope holding the var that for declares, then
            // later move that var to the parsed scope
            expr->body
                = parseScope(this, scope, false, (tt == TKKeyword_if));
            if (tt == TKKeyword_for) {
                // TODO: here it is too late to add the variable,
                // because parseScope will call resolveVars.
                var = NEW(ASTVar);
                var->name = expr->left->left->name;
                var->init = expr->left->right;
                var->typeSpec = NEW(ASTTypeSpec);
                var->typeSpec->typeType = TYUInt32;
                PtrList_append(&expr->body->locals, var);
            }

            if (matches(this, TKKeyword_else)) {
                startedElse = true;
            } else {
                discard(this, TKKeyword_end);
                discard(this, TKOneSpace);
                discard(this, tt == TKKeyword_else ? TKKeyword_if : tt);
            }
            PtrList_append(&scope->stmts, expr);
            break;

        case TKNewline:
        case TKOneSpace:
            Token_advance(&this->token);
            break;

        case TKLineComment:
            if (this->generateCommentExprs) {
                expr = ASTExpr_fromToken(&this->token);
                PtrList_append(&scope->stmts, expr);
            }
            Token_advance(&this->token);
            break;

        default:
            expr = parseExpr(this);
            if (expr and isTypeBody) {
                Parser_errorInvalidTypeMember(this);
                expr = NULL;
            }
            if (not expr) break;
            PtrList_append(&scope->stmts, expr);
            Token_advance(&this->token); // eat the newline
            resolveVars(this, expr, scope, false);
            break;
        }
    }
exitloop:
    return scope;
}

#pragma mark - PARSE PARAM
static List(ASTVar) * parseParams(Parser* this)
{
    discard(this, TKOpLT);
    List(ASTVar) * params;
    ASTVar* param;
    do {
        param = NEW(ASTVar);
        param->name = parseIdent(this);
        if (Parser_ignore(this, TKKeyword_as))
            param->typeSpec = parseTypeSpec(this);
        if (Parser_ignore(this, TKOpAssign)) param->init = parseExpr(this);
        PtrList_append(&params, param);
    } while (Parser_ignore(this, TKOpComma));
    discard(this, TKOpGT);
    return params;
}

#pragma mark - PARSE FUNC / STMT-FUNC
static ASTFunc* parseFunc(Parser* this, bool shouldParseBody)
{
    discard(this, TKKeyword_function);
    discard(this, TKOneSpace);
    ASTFunc* func = NEW(ASTFunc);

    func->line = this->token.line;

    if (memchr(this->token.pos, '_', this->token.matchlen))
        Parser_errorInvalidIdent(this);
    if (*this->token.pos < 'a' or *this->token.pos > 'z')
        Parser_errorInvalidIdent(this);
    func->name = parseIdent(this);

    func->flags.isDeclare = !shouldParseBody;

    func->args = parseArgs(this);
    func->argCount = PtrList_count(func->args);

    if (Parser_ignore(this, TKOneSpace)
        and Parser_ignore(this, TKKeyword_returns)) {
        discard(this, TKOneSpace);
        func->returnType = parseTypeSpec(this);
    }

    if (shouldParseBody) {
        discard(this, TKNewline);

        ASTScope* funcScope = NEW(ASTScope);
        funcScope->locals = func->args;
        func->body = parseScope(this, funcScope, false, false);

        discard(this, TKKeyword_end);
        discard(this, TKOneSpace);
        discard(this, TKKeyword_function);
    }

    return func;
}

static ASTFunc* parseStmtFunc(Parser* this)
{
    ASTFunc* func = NEW(ASTFunc);

    func->line = this->token.line;
    func->flags.isStmt = true;

    if (memchr(this->token.pos, '_', this->token.matchlen))
        Parser_errorInvalidIdent(this);
    if (*this->token.pos < 'a' or *this->token.pos > 'z')
        Parser_errorInvalidIdent(this);
    func->name = parseIdent(this);

    func->args = parseArgs(this);
    func->argCount = PtrList_count(func->args);

    if (Parser_ignore(this, TKOneSpace)
        and Parser_ignore(this, TKKeyword_as)) {
        discard(this, TKOneSpace);
        func->returnType = parseTypeSpec(this);
        Parser_ignore(this, TKOneSpace);
    } else {
        func->returnType = NEW(ASTTypeSpec);
        func->returnType->line = this->token.line;
        func->returnType->col = this->token.col;
        func->returnType->name = "";
    }

    ASTExpr* ret = exprFromCurrentToken(this);
    assert(ret->kind == TKColEq);
    ret->kind = TKKeyword_return;
    ret->opIsUnary = true;

    ret->right = parseExpr(this);
    ASTScope* scope = NEW(ASTScope);
    PtrList_append(&scope->stmts, ret);

    ASTScope* funcScope = NEW(ASTScope);
    funcScope->locals = func->args;
    scope->parent = funcScope;
    func->body = scope;

    resolveVars(this, ret->right, funcScope, false);

    return func;
}

#pragma mark - PARSE TEST
static ASTFunc* parseTest(Parser* this) { return NULL; }

#pragma mark - PARSE UNITS
static ASTUnits* parseUnits(Parser* this) { return NULL; }

#pragma mark - PARSE TYPE
static ASTType* parseType(Parser* this, bool shouldParseBody)
{
    ASTType* type = NEW(ASTType);

    discard(this, TKKeyword_type);
    discard(this, TKOneSpace);

    if (memchr(this->token.pos, '_', this->token.matchlen))
        Parser_errorInvalidIdent(this);
    if (*this->token.pos < 'A' or *this->token.pos > 'Z')
        Parser_errorInvalidIdent(this);
    type->name = parseIdent(this);

    if (Parser_ignore(this, TKOneSpace)
        and Parser_ignore(this, TKKeyword_extends)) {
        discard(this, TKOneSpace);
        type->super = parseTypeSpec(this);
    }

    type->body = NULL; // this means type is declare
    if (not shouldParseBody) return type;
    type->body = parseScope(this, NULL, true, false);

    discard(this, TKKeyword_end);
    discard(this, TKOneSpace);
    discard(this, TKKeyword_type);

    return type;
}

static ASTImport* parseImport(Parser* this)
{
    ASTImport* import = NEW(ASTImport);
    char* tmp;
    discard(this, TKKeyword_import);
    discard(this, TKOneSpace);

    import->isPackage = Parser_ignore(this, TKAt);

    if (memchr(this->token.pos, '_', this->token.matchlen))
        Parser_errorInvalidIdent(this);

    import->importFile = parseIdent(this);
    size_t len = this->token.pos - import->importFile;
    Parser_ignore(this, TKOneSpace);
    if (Parser_ignore(this, TKKeyword_as)) {

        Parser_ignore(this, TKOneSpace);
        import->hasAlias = true;

        if (memchr(this->token.pos, '_', this->token.matchlen))
            Parser_errorInvalidIdent(this);

        tmp = parseIdent(this);
        if (tmp) import->aliasOffset = (uint32_t)(tmp - import->importFile);

    } else {
        import->aliasOffset = (uint32_t)(
            str_base(import->importFile, '.', len) - import->importFile);
    }

    Parser_ignore(this, TKOneSpace);

    if (this->token.kind != TKLineComment and this->token.kind != TKNewline)
        Parser_errorUnexpectedToken(this);
    while (
        this->token.kind != TKLineComment and this->token.kind != TKNewline)
        Token_advance(&this->token);
    return import;
}

static PtrList* parseModule(Parser* this)
{
    ASTModule* root = NEW(ASTModule);
    root->name = this->moduleName;
    const bool onlyPrintTokens = false;
    Token_advance(&this->token); // maybe put this in parser ctor
    ASTImport* import = NULL;

    // The take away is (for C gen):
    // Every caller who calls append(List) should keep a local List*
    // to follow the list top as items are appended. Each actual append
    // call must be followed by an update of this pointer to its own
    // ->next. Append should be called on the last item of the list, not
    // the first. (it will work but seek through the whole list every
    // time).

    List(ASTFunc)** funcsTop = &root->funcs;
    List(ASTImport)** importsTop = &root->imports;
    List(ASTType)** typesTop = &root->types;
    // List(ASTFunc)** testsTop = &root->tests;
    // List(ASTVar)** globalsTop = &root->globals;

    while (this->token.kind != TKNullChar) {
        if (onlyPrintTokens) {
            printf("%s %2d %3d %3d %-6s\t%.*s\n", this->moduleName,
                this->token.line, this->token.col, this->token.matchlen,
                TokenKind_repr(this->token.kind, false),
                this->token.kind == TKNewline ? 0 : this->token.matchlen,
                this->token.pos);
            Token_advance(&this->token);
            continue;
        }
        switch (this->token.kind) {
        case TKKeyword_declare:
            Token_advance(&this->token);
            discard(this, TKOneSpace);
            if (this->token.kind == TKKeyword_function) {
                PtrList_append(funcsTop, parseFunc(this, false));
                if ((*funcsTop)->next) funcsTop = &(*funcsTop)->next;
            }
            if (this->token.kind == TKKeyword_type) {
                PtrList_append(typesTop, parseType(this, false));
                if ((*typesTop)->next) typesTop = &(*typesTop)->next;
            }
            break;

        case TKKeyword_function:
            PtrList_append(funcsTop, parseFunc(this, true));
            if ((*funcsTop)->next) funcsTop = &(*funcsTop)->next;
            break;

        case TKKeyword_type:
            PtrList_append(typesTop, parseType(this, true));
            if ((*typesTop)->next) typesTop = &(*typesTop)->next;
            break;

        case TKKeyword_import:
            import = parseImport(this);
            if (import) {
                PtrList_append(importsTop, import);
                if ((*importsTop)->next) importsTop = &(*importsTop)->next;
                //                    auto subParser = new
                //                    Parser(import->importFile);
                //                    List<ASTModule*> subMods =
                //                    parse(subParser);
                //                    PtrList_append(&modules, subMods);
            }
            break;
        // case TKKeyword_test:
        //     PtrList_append(testsTop, parseTest(this));
        //     if ((*testsTop)->next) testsTop = &(*testsTop)->next;
        //     break;
        // case TKKeyword_var:
        // case TKKeyword_let:
        // TODO: add these to exprs
        // PtrList_append(globalsTop, parseVar(this));
        // if ((*globalsTop)->next) globalsTop =
        // &(*globalsTop)->next;
        // break;
        case TKNewline:
        case TKLineComment:
        // TODO: add line comment to module exprs
        case TKOneSpace:
            Token_advance(&this->token);
            break;
        case TKIdentifier: // stmt funcs: f(x) := f(y, w = 4) etc.
            if (Token_peekCharAfter(&this->token) == '(') {
                PtrList_append(funcsTop, parseStmtFunc(this));
                if ((*funcsTop)->next) funcsTop = &(*funcsTop)->next;
                break;
            }
        default:
            Parser_errorUnexpectedToken(this);
            while (this->token.kind != TKNewline
                and this->token.kind != TKLineComment)
                Token_advance(&this->token);
        }
    }
    // also keep modulesTop

    // do some analysis that happens after the entire module is loaded

    foreach (ASTType*, type, root->types) {
        if (type->super) resolveTypeSpec(this, type->super, root);
        if (not type->body) continue;
        foreach (ASTExpr*, stmt, type->body->stmts)
            resolveFuncsAndTypes(this, stmt, root);
    }

    foreach (ASTFunc*, func, root->funcs) {
        foreach (ASTVar*, arg, func->args) {
            resolveTypeSpec(this, arg->typeSpec, root);
        }
        if (func->returnType) resolveTypeSpec(this, func->returnType, root);
        getSelector(func);
    }

    foreach (ASTFunc*, func, root->funcs) {
        if (func->body) {
            ASTFunc_checkUnusedVars(this, func);
            foreach (ASTExpr*, stmt, func->body->stmts) {
                resolveFuncsAndTypes(this, stmt, root);
                setExprTypeInfo(this, stmt, false);
            }
            if (func->flags.isStmt) setStmtFuncTypeInfo(this, func);

            if (not this->errCount) {
                ASTScope_lowerElementalOps(func->body);
                ASTScope_promoteCandidates(func->body);
            }
        }
    }

    PtrList_append(&this->modules, root);
    return this->modules;
}
