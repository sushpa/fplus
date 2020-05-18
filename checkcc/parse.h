
#pragma mark - PARSE EXPR
static ASTExpr* parseExpr(Parser* self)
{
    // there are 2 steps to this madness.
    // 1. parse a sequence of tokens into RPN using shunting-yard.
    // 2. walk the rpn stack as a sequence and copy it into a result
    // stack, collapsing the stack when you find nonterminals (ops, func
    // calls, array index, ...)

    static PtrArray rpn, ops, result;
    int prec_top = 0;
    ASTExpr* p = NULL;
    TokenKind revBrkt = tkUnknown;

    // ******* STEP 1 CONVERT TOKENS INTO RPN

    while (self->token.kind != tkNullChar //
        and self->token.kind != tkNewline
        and self->token.kind != tkLineComment) { // build RPN

        // you have to ensure that ops have a space around them, etc.
        // so don't just skip the one spaces like you do now.
        if (self->token.kind == tkOneSpace) Token_advance(&self->token);
        if (self->token.kind == tkIdentifier
            and memchr(self->token.pos, '_', self->token.matchlen))
            Parser_errorInvalidIdent(self); // but continue parsing

        ASTExpr* expr = ASTExpr_fromToken(&self->token); // dont advance yet
        int prec = expr->prec;
        bool rassoc = prec ? expr->rassoc : false;
        char lookAheadChar = Token_peekCharAfter(&self->token);

        switch (expr->kind) {
        case tkIdentifier:
            switch (lookAheadChar) {
            case '(':
                expr->kind = tkFunctionCall;
                expr->prec = 60;
                PtrArray_push(&ops, expr);
                break;
            case '[':
                expr->kind = tkSubscript;
                expr->prec = 60;
                PtrArray_push(&ops, expr);
                break;
            default:
                PtrArray_push(&rpn, expr);
                break;
            }
            break;

        case tkParenOpen:
            PtrArray_push(&ops, expr);
            if (not PtrArray_empty(&ops)
                and PtrArray_topAs(ASTExpr*, &ops)->kind == tkFunctionCall)
                PtrArray_push(&rpn, expr);
            if (lookAheadChar == ')') PtrArray_push(&rpn, NULL);
            // for empty func() push null for no args
            break;

        case tkArrayOpen:
            PtrArray_push(&ops, expr);
            if (not PtrArray_empty(&ops)
                and PtrArray_topAs(ASTExpr*, &ops)->kind == tkSubscript)
                PtrArray_push(&rpn, expr);
            if (lookAheadChar == ')') PtrArray_push(&rpn, NULL);
            // for empty arr[] push null for no args
            break;

        case tkParenClose:
        case tkArrayClose:
        case tkBraceClose:

            revBrkt = TokenKind_reverseBracket(expr->kind);
            if (PtrArray_empty(&ops)) {
                // need atleast the opening bracket of the current kind
                Parser_errorParsingExpr(self);
                goto error;
            }

            else
                while (not PtrArray_empty(&ops)) {
                    p = PtrArray_pop(&ops);
                    if (p->kind == revBrkt) break;
                    PtrArray_push(&rpn, p);
                }
            // we'll push the tkArrayOpen as well to indicate a list
            // literal or comprehension
            // tkArrayOpen is a unary op.
            if ((p and p->kind == tkArrayOpen)
                and (PtrArray_empty(&ops)
                    or PtrArray_topAs(ASTExpr*, &ops)->kind != tkSubscript)
                // don't do this if its part of a subscript
                and (PtrArray_empty(&rpn)
                    or PtrArray_topAs(ASTExpr*, &rpn)->kind != tkOpColon))
                // or aa range. range exprs are handled separately. by
                // themselves they don't need a surrounding [], but for
                // grouping like 2+[8:66] they do.
                PtrArray_push(&rpn, p);

            break;
        case tkKeyword_check:
            PtrArray_push(&ops, expr);
            break;
        case tkKeyword_return:
            // for empty return, push a NULL if there is no expr coming.
            PtrArray_push(&ops, expr);
            if (lookAheadChar == '!' or lookAheadChar == '\n')
                PtrArray_push(&rpn, NULL);
            break;
        default:
            if (prec) {

                if (expr->kind == tkOpColon) {
                    if (PtrArray_empty(&rpn)
                        or (!PtrArray_top(&rpn) and !PtrArray_empty(&ops)
                            and PtrArray_topAs(ASTExpr*, &ops)->kind
                                != tkOpColon)
                        or (PtrArray_topAs(ASTExpr*, &rpn)->kind == tkOpColon
                            and !PtrArray_empty(&ops)
                            and (PtrArray_topAs(ASTExpr*, &ops)->kind
                                    == tkOpComma
                                or PtrArray_topAs(ASTExpr*, &ops)->kind
                                    == tkArrayOpen)))
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
                    prec_top = PtrArray_topAs(ASTExpr*, &ops)->prec;
                    if (not prec_top) break; // left parenthesis
                    if (prec > prec_top) break;
                    if (prec == prec_top and rassoc) break;
                    p = PtrArray_pop(&ops);

                    if (p->kind != tkOpComma and p->kind != tkOpSemiColon
                        and p->kind != tkFunctionCall and p->kind != tkSubscript
                        and PtrArray_topAs(ASTExpr*, &rpn)
                        and PtrArray_topAs(ASTExpr*, &rpn)->kind == tkOpComma) {
                        Parser_errorUnexpectedToken(self);
                        goto error;
                    }

                    if (not(p->prec or p->unary) and p->kind != tkFunctionCall
                        and p->kind != tkOpColon and p->kind != tkSubscript
                        and rpn.used < 2) {
                        Parser_errorUnexpectedToken(self);
                        goto error;
                    }

                    PtrArray_push(&rpn, p);
                }

                if (PtrArray_empty(&rpn)) {
                    Parser_errorUnexpectedToken(self);
                    goto error;
                }
                if (expr->kind == tkOpColon
                    and (lookAheadChar == ',' or lookAheadChar == ':'
                        or lookAheadChar == ']' or lookAheadChar == ')'))
                    PtrArray_push(&rpn, NULL);

                PtrArray_push(&ops, expr);
            } else {
                PtrArray_push(&rpn, expr);
            }
        }
        Token_advance(&self->token);
        if (self->token.kind == tkOneSpace) Token_advance(&self->token);
    }
exitloop:

    while (not PtrArray_empty(&ops)) {
        p = PtrArray_pop(&ops);

        if (p->kind != tkOpComma and p->kind != tkFunctionCall
            and p->kind != tkSubscript and p->kind != tkArrayOpen
            and PtrArray_topAs(ASTExpr*, &rpn)
            and PtrArray_topAs(ASTExpr*, &rpn)->kind == tkOpComma) {
            Parser_errorUnexpectedExpr(self, PtrArray_topAs(ASTExpr*, &rpn));
            goto error;
        }

        if (not(p->prec or p->unary)
            and (p->kind != tkFunctionCall and p->kind != tkSubscript)
            and rpn.used < 2) {
            Parser_errorParsingExpr(self);
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
        case tkFunctionCall:
        case tkSubscript:
            if (result.used > 0) {
                arg = PtrArray_pop(&result);
                p->left = arg;
            }
            break;

        case tkNumber:
        case tkString:
        case tkRegex:
        case tkInline:
        case tkUnits:
        case tkMultiDotNumber:
        case tkIdentifier:
        case tkParenOpen:
            break;

        default:
            // everything else is a nonterminal, needs left/right
            if (not p->prec) {
                Parser_errorParsingExpr(self);
                goto error;
            }

            if (PtrArray_empty(&result)) {
                Parser_errorParsingExpr(self);
                goto error;
            }

            p->right = PtrArray_pop(&result);

            if (not p->unary) {
                if (PtrArray_empty(&result)) {
                    Parser_errorParsingExpr(self);
                    goto error;
                }
                p->left = PtrArray_pop(&result);
            }
        }
    justpush:
        PtrArray_push(&result, p);
    }
    if (result.used != 1) {
        Parser_errorParsingExpr(self);
        goto error;
    }

    ops.used = 0;
    rpn.used = 0;
    result.used = 0;
    return result.ref[0];

error:

    while (self->token.pos < self->end
        and (self->token.kind != tkNewline
            and self->token.kind != tkLineComment))
        Token_advance(&self->token);

    if (ops.used) {
        printf("      ops: ");
        for (int i = 0; i < ops.used; i++)
            printf("%s ", TokenKind_repr(((ASTExpr*)ops.ref[i])->kind, false));
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
                    e->prec ? TokenKind_repr(e->kind, false) : e->string);
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
                    e->prec ? TokenKind_repr(e->kind, false) : e->string);
            }
        puts("");
    }

    if (p) {
        printf("      p: %s ",
            p->prec ? TokenKind_repr(p->kind, false) : p->string);
        puts("");
    }

    ops.used = 0; // "reset" stacks
    rpn.used = 0;
    result.used = 0;
    return NULL;
}

#pragma mark - PARSE TYPESPEC
static ASTTypeSpec* parseTypeSpec(Parser* self)
{
    self->token.mergeArrayDims = true;

    ASTTypeSpec* typeSpec = NEW(ASTTypeSpec);
    typeSpec->line = self->token.line;
    typeSpec->col = self->token.col;

    if (memchr(self->token.pos, '_', self->token.matchlen))
        Parser_errorInvalidIdent(self);

    typeSpec->name = parseIdent(self);

    if (matches(self, tkArrayDims)) {
        for (int i = 0; i < self->token.matchlen; i++)
            if (self->token.pos[i] == ':') typeSpec->dims++;
        if (not typeSpec->dims) typeSpec->dims = 1;
        Token_advance(&self->token);
    }

    Parser_ignore(self, tkUnits);

    assert(self->token.kind != tkUnits);
    assert(self->token.kind != tkArrayDims);

    self->token.mergeArrayDims = false;
    return typeSpec;
}

#pragma mark - PARSE VAR
static ASTVar* parseVar(Parser* self)
{
    ASTVar* var = NEW(ASTVar);
    var->isVar = (self->token.kind == tkKeyword_var);
    var->isLet = (self->token.kind == tkKeyword_let);

    if (var->isVar) discard(self, tkKeyword_var);
    if (var->isLet) discard(self, tkKeyword_let);
    if (var->isVar or var->isLet) discard(self, tkOneSpace);

    var->line = self->token.line;
    var->col = self->token.col;

    if (memchr(self->token.pos, '_', self->token.matchlen))
        Parser_errorInvalidIdent(self);
    if (*self->token.pos < 'a' or *self->token.pos > 'z')
        Parser_errorInvalidIdent(self);
    var->name = parseIdent(self);

    if (Parser_ignore(self, tkOneSpace) and Parser_ignore(self, tkKeyword_as)) {
        discard(self, tkOneSpace);
        var->typeSpec = parseTypeSpec(self);
    } else {
        var->typeSpec = NEW(ASTTypeSpec);
        var->typeSpec->line = self->token.line;
        var->typeSpec->col = self->token.col;
        var->typeSpec->name = "";
    }

    Parser_ignore(self, tkOneSpace);
    if (Parser_ignore(self, tkOpAssign)) var->init = parseExpr(self);

    return var;
}

static List(ASTVar) * parseArgs(Parser* self)
{
    List(ASTVar)* args = NULL;
    discard(self, tkParenOpen);
    if (Parser_ignore(self, tkParenClose)) return args;

    ASTVar* arg;
    do {
        arg = parseVar(self);
        PtrList_append(&args, arg);
    } while (Parser_ignore(self, tkOpComma));

    discard(self, tkParenClose);
    return args;
}

#pragma mark - PARSE SCOPE
static ASTScope* parseScope(
    Parser* self, ASTScope* parent, bool isTypeBody, bool isIfBlock)
{
    ASTScope* scope = NEW(ASTScope);

    ASTVar *var = NULL, *orig = NULL;
    ASTExpr* expr = NULL;
    TokenKind tt = tkUnknown;

    scope->parent = parent;
    bool startedElse = false;
    while (self->token.kind != tkKeyword_end) {

        switch (self->token.kind) {

        case tkNullChar:
            Parser_errorExpectedToken(self, tkUnknown);
            goto exitloop;

        case tkKeyword_var:
        case tkKeyword_let:
            var = parseVar(self);
            if (not var)
                continue;
            else
                Token_advance(&self->token);
            if ((orig = ASTScope_getVar(scope, var->name)))
                Parser_errorDuplicateVar(self, var, orig);
            // TODO: why only idents and binops for resolveVars??
            if (var->init
                and (var->init->prec or var->init->kind == tkIdentifier))
                resolveVars(self, var->init, scope, false);
            // resolveType(var->typeSpec, scope);
            // resolve BEFORE it is added to the list! in
            // `var x = x + 1` x should not resolve
            // if var->typeSpec is NULL then set the type
            // if it isn't NULL then check the types match
            PtrList_append(&scope->locals, var);
            // TODO: validation should raise issue if var->init is
            // missing
            expr = NEW(ASTExpr);
            expr->kind = tkVarAssign;
            expr->line = var->init ? var->init->line : self->token.line;
            expr->col = var->init ? var->init->col : 1;
            expr->prec = TokenKind_getPrecedence(tkOpAssign);
            expr->var = var;
            PtrList_append(&scope->stmts, expr);
            break;

        case tkKeyword_else:
            if (not startedElse) goto exitloop;

        case tkKeyword_if:
        case tkKeyword_for:
        case tkKeyword_while:
            if (isTypeBody) Parser_errorInvalidTypeMember(self);
            tt = self->token.kind;
            expr = match(self, tt);
            expr->left = tt != tkKeyword_else ? parseExpr(self) : NULL;

            if (tt == tkKeyword_for) {
                // TODO: new Parser_error
                if (expr->left->kind != tkOpAssign)
                    eprintf("Invalid for-loop condition: %s\n",
                        TokenKind_repr(expr->left->kind, false));
                resolveVars(self, expr->left->right, scope, false);
            } else if (expr->left) {
                resolveVars(self, expr->left, scope, false);
            } // TODO: `for` necessarily introduces a counter variable, so
            // check if that var name doesn't already exist in scope.
            // Also assert that the cond of a for expr has kind
            // tkOpAssign.
            // insert a temp scope holding the var that for declares, then
            // later move that var to the parsed scope
            expr->body = parseScope(self, scope, false, (tt == tkKeyword_if));
            if (tt == tkKeyword_for) {
                // TODO: here it is too late to add the variable,
                // because parseScope will call resolveVars.
                var = NEW(ASTVar);
                var->name = expr->left->left->string;
                var->init = expr->left->right;
                var->typeSpec = NEW(ASTTypeSpec);
                var->typeSpec->typeType = TYUInt32;
                PtrList_append(&expr->body->locals, var);
            }

            if (matches(self, tkKeyword_else)) {
                startedElse = true;
            } else {
                discard(self, tkKeyword_end);
                discard(self, tkOneSpace);
                discard(self, tt == tkKeyword_else ? tkKeyword_if : tt);
            }
            PtrList_append(&scope->stmts, expr);
            break;

        case tkNewline:
        case tkOneSpace:
            Token_advance(&self->token);
            break;

        case tkLineComment:
            if (self->generateCommentExprs) {
                expr = ASTExpr_fromToken(&self->token);
                PtrList_append(&scope->stmts, expr);
            }
            Token_advance(&self->token);
            break;

        default:
            expr = parseExpr(self);
            if (expr and isTypeBody) {
                Parser_errorInvalidTypeMember(self);
                expr = NULL;
            }
            if (not expr) break;
            PtrList_append(&scope->stmts, expr);
            Token_advance(&self->token); // eat the newline
            resolveVars(self, expr, scope, false);
            break;
        }
    }
exitloop:
    return scope;
}

#pragma mark - PARSE PARAM
static List(ASTVar) * parseParams(Parser* self)
{
    discard(self, tkOpLT);
    List(ASTVar) * params;
    ASTVar* param;
    do {
        param = NEW(ASTVar);
        param->name = parseIdent(self);
        if (Parser_ignore(self, tkKeyword_as))
            param->typeSpec = parseTypeSpec(self);
        if (Parser_ignore(self, tkOpAssign)) param->init = parseExpr(self);
        PtrList_append(&params, param);
    } while (Parser_ignore(self, tkOpComma));
    discard(self, tkOpGT);
    return params;
}

#pragma mark - PARSE FUNC / STMT-FUNC
static ASTFunc* parseFunc(Parser* self, bool shouldParseBody)
{
    discard(self, tkKeyword_function);
    discard(self, tkOneSpace);
    ASTFunc* func = NEW(ASTFunc);

    func->line = self->token.line;

    if (memchr(self->token.pos, '_', self->token.matchlen))
        Parser_errorInvalidIdent(self);
    func->name = parseIdent(self);

    func->isDeclare = not shouldParseBody;

    func->args = parseArgs(self);
    func->argCount = PtrList_count(func->args);

    if (Parser_ignore(self, tkOneSpace)
        and Parser_ignore(self, tkKeyword_returns)) {
        discard(self, tkOneSpace);
        func->returnSpec = parseTypeSpec(self);
    }

    if (shouldParseBody) {
        discard(self, tkNewline);

        ASTScope* funcScope = NEW(ASTScope);
        funcScope->locals = func->args;
        func->body = parseScope(self, funcScope, false, false);

        discard(self, tkKeyword_end);
        discard(self, tkOneSpace);
        discard(self, tkKeyword_function);
    }

    return func;
}

static ASTFunc* parseStmtFunc(Parser* self)
{
    ASTFunc* func = NEW(ASTFunc);

    func->line = self->token.line;
    func->isStmt = true;

    if (memchr(self->token.pos, '_', self->token.matchlen))
        Parser_errorInvalidIdent(self);
    func->name = parseIdent(self);

    func->args = parseArgs(self);
    func->argCount = PtrList_count(func->args);
    Parser_ignore(self, tkOneSpace);

    func->returnSpec = NEW(ASTTypeSpec);
    func->returnSpec->line = self->token.line;
    func->returnSpec->col = self->token.col;
    func->returnSpec->name = "";

    ASTExpr* ret = exprFromCurrentToken(self);
    assert(ret->kind == tkColEq);
    ret->kind = tkKeyword_return;
    ret->unary = true;

    ret->right = parseExpr(self);
    ASTScope* scope = NEW(ASTScope);
    PtrList_append(&scope->stmts, ret);

    ASTScope* funcScope = NEW(ASTScope);
    funcScope->locals = func->args;
    scope->parent = funcScope;
    func->body = scope;

    resolveVars(self, ret->right, funcScope, false);

    return func;
}

#pragma mark - PARSE TEST
static ASTTest* parseTest(Parser* self)
{
    discard(self, tkKeyword_test);
    discard(self, tkOneSpace);
    ASTTest* test = NEW(ASTTest);

    test->line = self->token.line;

    if (*self->token.pos != '"') Parser_errorInvalidTestName(self);
    test->name = self->token.pos + 1;
    Token_advance(&self->token);

    discard(self, tkNewline);

    test->body = parseScope(self, NULL, false, false);

    discard(self, tkKeyword_end);
    discard(self, tkOneSpace);
    discard(self, tkKeyword_test);

    return test;
}

#pragma mark - PARSE UNITS
static ASTUnits* parseUnits(Parser* self) { return NULL; }

#pragma mark - PARSE TYPE
static ASTType* parseType(Parser* self, bool shouldParseBody)
{
    ASTType* type = NEW(ASTType);

    discard(self, tkKeyword_type);
    discard(self, tkOneSpace);

    type->line = self->token.line;
    type->col = self->token.col;

    if (memchr(self->token.pos, '_', self->token.matchlen))
        Parser_errorInvalidIdent(self);
    if (*self->token.pos < 'A' or *self->token.pos > 'Z')
        Parser_errorInvalidIdent(self);
    type->name = parseIdent(self);

    if (Parser_ignore(self, tkOneSpace)
        and Parser_ignore(self, tkKeyword_extends)) {
        discard(self, tkOneSpace);
        type->super = parseTypeSpec(self);
    }
    Parser_ignore(self, tkNewline);

    type->body = NULL; // this means type is declare
    if (TypeType_byName(type->name) != TYUnresolved) {
        Parser_errorDuplicateType(self, type, NULL);
        return type;
    }

    if (not shouldParseBody) return type;

    type->body = parseScope(self, NULL, true, false);

    discard(self, tkKeyword_end);
    discard(self, tkOneSpace);
    discard(self, tkKeyword_type);

    return type;
}

static ASTImport* parseImport(Parser* self)
{
    ASTImport* import = NEW(ASTImport);
    char* tmp;
    discard(self, tkKeyword_import);
    discard(self, tkOneSpace);

    import->isPackage = Parser_ignore(self, tkAt);

    if (memchr(self->token.pos, '_', self->token.matchlen))
        Parser_errorInvalidIdent(self);

    import->importFile = parseIdent(self);
    size_t len = self->token.pos - import->importFile;
    Parser_ignore(self, tkOneSpace);
    if (Parser_ignore(self, tkKeyword_as)) {

        Parser_ignore(self, tkOneSpace);
        import->hasAlias = true;

        if (memchr(self->token.pos, '_', self->token.matchlen))
            Parser_errorInvalidIdent(self);

        tmp = parseIdent(self);
        if (tmp) import->aliasOffset = (uint32_t)(tmp - import->importFile);

    } else {
        import->aliasOffset = (uint32_t)(
            str_base(import->importFile, '.', len) - import->importFile);
    }

    Parser_ignore(self, tkOneSpace);

    if (self->token.kind != tkLineComment and self->token.kind != tkNewline)
        Parser_errorUnexpectedToken(self);
    while (self->token.kind != tkLineComment and self->token.kind != tkNewline)
        Token_advance(&self->token);
    return import;
}
void analyseModule(Parser* self, ASTModule* mod);

static PtrList* parseModule(Parser* self)
{
    ASTModule* root = NEW(ASTModule);
    root->name = self->moduleName;
    const bool onlyPrintTokens = false;
    Token_advance(&self->token); // maybe put this in parser ctor
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
    List(ASTTest)** testsTop = &root->tests;
    // List(ASTVar)** globalsTop = &root->globals;

    while (self->token.kind != tkNullChar) {
        if (onlyPrintTokens) {
            printf("%s %2d %3d %3d %-20s\t%.*s\n", self->moduleName,
                self->token.line, self->token.col, self->token.matchlen,
                TokenKind_str[self->token.kind],
                self->token.kind == tkNewline ? 0 : self->token.matchlen,
                self->token.pos);
            Token_advance(&self->token);
            continue;
        }
        switch (self->token.kind) {
        case tkKeyword_declare:
            Token_advance(&self->token);
            discard(self, tkOneSpace);
            if (self->token.kind == tkKeyword_function) {
                PtrList_append(funcsTop, parseFunc(self, false));
                if ((*funcsTop)->next) funcsTop = &(*funcsTop)->next;
            }
            if (self->token.kind == tkKeyword_type) {
                PtrList_append(typesTop, parseType(self, false));
                if ((*typesTop)->next) typesTop = &(*typesTop)->next;
            }
            break;

        case tkKeyword_function:
            PtrList_append(funcsTop, parseFunc(self, true));
            if ((*funcsTop)->next) funcsTop = &(*funcsTop)->next;
            break;

        case tkKeyword_type: {
            ASTType* type = parseType(self, true);
            PtrList_append(typesTop, type);
            if ((*typesTop)->next) typesTop = &(*typesTop)->next;

            // create default constructor
            ASTFunc* ctor = NEW(ASTFunc);
            ctor->line = type->line;
            ctor->isDefCtor = true;
            // Ctors must AlWAYS return a new object.
            // even Ctors with args.
            ctor->returnsNewObjectAlways = true;
            ctor->name = type->name;
            char buf[128];
            int l = snprintf(buf, 128, "%s_new_", type->name);
            ctor->selector = pstrndup(buf, l);
            ASTTypeSpec* tspec = ASTTypeSpec_new(TYObject, CTYNone);
            tspec->type = type;
            ctor->returnSpec = tspec;
            PtrList_append(funcsTop, ctor);
            if ((*funcsTop)->next) funcsTop = &(*funcsTop)->next;

            // create some extra function declares
            char* defFuncs[] = { "json", "print", "describe" };
            for (int i = 0; i < countof(defFuncs); i++) {
                ASTFunc* func
                    = ASTFunc_createDeclWithArg(defFuncs[i], NULL, type->name);
                PtrList_append(funcsTop, func);
                if ((*funcsTop)->next) funcsTop = &(*funcsTop)->next;
            }

        } break;

        case tkKeyword_import:
            import = parseImport(self);
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

        case tkKeyword_test:
            PtrList_append(testsTop, parseTest(self));
            if ((*testsTop)->next) testsTop = &(*testsTop)->next;
            break;
        // case tkKeyword_var:
        // case tkKeyword_let:
        // TODO: add these to exprs
        // PtrList_append(globalsTop, parseVar(self));
        // if ((*globalsTop)->next) globalsTop =
        // &(*globalsTop)->next;
        // break;
        case tkNewline:
        case tkLineComment:
        // TODO: add line comment to module exprs
        case tkOneSpace:
            Token_advance(&self->token);
            break;
        case tkIdentifier: // stmt funcs: f(x) := f(y, w = 4) etc.
            if (Token_peekCharAfter(&self->token) == '(') {
                PtrList_append(funcsTop, parseStmtFunc(self));
                if ((*funcsTop)->next) funcsTop = &(*funcsTop)->next;
                break;
            }
        default:
            Parser_errorUnexpectedToken(self);
            while (self->token.kind != tkNewline
                and self->token.kind != tkLineComment)
                Token_advance(&self->token);
        }
    }
    // also keep modulesTop

    // Add some default functions "built-ins"
    // TODO: move this into a function later

    char* defTypes[] = { "String", "Number", "Boolean" };
    char* defFuncs[] = { "json", "print", "describe" };
    char* retTypes[countof(defFuncs)] = {}; // fill these for non-void funcs

    for (int j = 0; j < countof(defTypes); j++)
        for (int i = 0; i < countof(defFuncs); i++) {
            ASTFunc* func = ASTFunc_createDeclWithArg(
                defFuncs[i], retTypes[i], defTypes[j]);
            PtrList_append(funcsTop, func);
            if ((*funcsTop)->next) funcsTop = &(*funcsTop)->next;
        }

    // do some analysis that happens after the entire module is loaded
    analyseModule(self, root);

    PtrList_append(&self->modules, root);
    return self->modules;
}

// TODO: move this to separate file or to analysis.c [sempass.c]

static void ASTModule_unmarkTypesVisited(ASTModule* mod);
static int ASTExpr_markTypesVisited(Parser* self, ASTExpr* expr);
static int ASTType_checkCycles(Parser* self, ASTType* type);

void analyseModule(Parser* self, ASTModule* mod)
{
    // If function calls are going to be resolved based on the type of
    // first arg, then ALL functions must be visited in order to
    // generate their selectors and resolve their typespecs. (this does
    // not set the resolved flag on the func -- that is done by the
    // semantic pass)
    foreach (ASTFunc*, func, mod->funcs) {
        foreach (ASTVar*, arg, func->args)
            resolveTypeSpec(self, arg->typeSpec, mod);
        if (func->returnSpec) resolveTypeSpec(self, func->returnSpec, mod);
        getSelector(func);
    }

    ASTFunc* fmain = NULL;
    // don't break on the first match, keep looking so that duplicate mains
    // can be found
    foreach (ASTFunc*, func, mod->funcs)
        if (not strcmp(func->name, "main")) fmain = func;

    if (fmain) {
        analyseFunc(self, fmain, mod);
        // Check dead code -- unused funcs and types, and report warnings.
        foreach (ASTFunc*, func, mod->funcs)
            if (not func->analysed and not func->isDefCtor)
                Parser_warnUnusedFunc(self, func);
        foreach (ASTType*, type, mod->types)
            if (not type->analysed) Parser_warnUnusedType(self, type);
    } else if (self->mode == PMGenTests) {
        foreach (ASTTest*, test, mod->tests)
            analyseTest(self, test, mod);
    } else { // TODO: new error, unless you want to get rid of main
        eputs(
            "\n\e[31m*** error:\e[0m cannot find function \e[33mmain\e[0m.\n");
    }

    // Check each type for cycles in inheritance graph.
    // Actually if there is no inheritance and composition is favoured, you have
    // to check each statement in the type body instead of just walking up the
    // super chain. If any statements are initialized by constructors, mark the
    // type of that statement as visited and recur into that type to check its
    // statements to see if you ever revisit anything. Unfortunately it does not
    // seem that this would be easy to do iteratively (not recursively), as it
    // can be done for just checking supers.
    // foreach (ASTType*, type, mod->types) {
    //     if (not type->analysed or not type->super) continue;
    //     assert(type->super->typeType == TYObject);

    //     // traverse the type hierarchy for this type and see if you revisit
    //     any ASTType* superType = type->super->type; while (superType) {
    //         if (superType->visited) {
    //             Parser_errorInheritanceCycle(self, type);
    //             break;
    //         }
    //         superType->visited = true;
    //         if (not superType->super) break;
    //         assert(superType->super->typeType == TYObject);
    //         superType = superType->super->type;
    //     }

    //     // reset the cycle check flag on all types
    //     foreach (ASTType*, etype, mod->types)
    //         if (type->analysed) etype->visited = false;
    // }

    // check each stmt in each type to find cycles.
    foreach (ASTType*, type, mod->types)
        if (type->analysed and type->body and not type->visited) {
            if (ASTType_checkCycles(self, type)) {
                // cycle was detected. err has been reported along with a
                // backtrace. now just unset the dim control codes.
                eprintf(" ...%s\n", "\e[0m");
                // just report the first cycle found. typically there will be
                // only one cycle and you will end up reporting the same cycle
                // for all types that are in it, which is useless.
                // break;
                // the last type (for which the error was reported) won't have
                // its cycle check flags cleared, but who cares.
                // OTHER IDEA: clear the flag only if there was no error. that
                // way the next iteration will skip over those whose flags are
                // already set.
            } else
                ASTModule_unmarkTypesVisited(mod);
        }
}

// return 0 on no cycle found, -1 on cycle found
static int ASTType_checkCycles(Parser* self, ASTType* type)
{
    foreach (ASTExpr*, stmt, type->body->stmts)
        if (ASTExpr_markTypesVisited(self, stmt)) {
            eprintf("  -> created in type \e[;1;2m%s\e[0;2m at ./%s:%d:%d \n",
                type->name, self->filename, stmt->line, stmt->col);
            return -1;
        }
    return 0;
}

static int ASTExpr_markTypesVisited(Parser* self, ASTExpr* expr)
{
    ASTType* type = NULL;
    if (!expr) return 0;
    switch (expr->kind) {
    case tkVarAssign:
        return ASTExpr_markTypesVisited(self, expr->var->init);
    case tkFunctionCall:
        return ASTExpr_markTypesVisited(self, expr->left);
    case tkFunctionCallResolved:
        if (ASTExpr_markTypesVisited(self, expr->left)) return -1;
        // if (expr->func->isDefCtor) type =
        // expr->func->returnSpec->type;
        if (expr->func->returnSpec->typeType == TYObject
            and expr->func->returnsNewObjectAlways)
            type = expr->func->returnSpec->type;
        break;
    case tkSubscriptResolved:
        return ASTExpr_markTypesVisited(self, expr->left);
    case tkIdentifierResolved:
    case tkString:
    case tkNumber:
    case tkRegex:
        return 0;
    default:
        if (expr->prec) {
            int ret = 0;
            if (not expr->unary)
                ret += ASTExpr_markTypesVisited(self, expr->left);
            ret += ASTExpr_markTypesVisited(self, expr->right);
            if (ret) return ret;
        } else
            unreachable("unknown expr kind: %s at %d:%d\n",
                TokenKind_str[expr->kind], expr->line, expr->col)
    }
    if (not type) return 0;
    if (type->visited) {
        Parser_errorConstructorHasCycle(self, type);
        eprintf("%s", "\e[;2m"); // Backtrace (innermost first):\n");
        return -1;
    }
    type->visited = true;
    return ASTType_checkCycles(self, type);
}

static void ASTModule_unmarkTypesVisited(ASTModule* mod)
{
    // reset the cycle check flag on all types
    foreach (ASTType*, type, mod->types)
        type->visited = false;
}
