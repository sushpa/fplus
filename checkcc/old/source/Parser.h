
class Parser {
    // bring down struct size!
    char* filename = NULL; // mod/submod/xyz/mycode.ch
    char* moduleName = NULL; // mod.submod.xyz.mycode
    char* mangledName = NULL; // mod_submod_xyz_mycode
    char* capsMangledName = NULL; // MOD_SUBMOD_XYZ_MYCODE
    char* basename = NULL; // mycode
    char* dirname = NULL; // mod/submod/xyz
    char *data = NULL, *end = NULL;
    bool generateCommentExprs
        = false; // set to false when compiling, set to true when linting
    char* noext = NULL;
    Token token; // current
    PtrList<ASTModule> modules; // module node of the AST
    Stack<ASTScope*> scopes; // a stack that keeps track of scope nesting

public:
    STHEAD_POOLB(Parser, 16)

    size_t dataSize() { return end - data; }

#define STR(x) STR_(x)
#define STR_(x) #x

    void fini()
    {
        free(data);
        free(noext);
        free(moduleName);
        free(mangledName);
        free(capsMangledName);
        free(dirname);
    }
    ~Parser() { fini(); }
    uint32_t errCount = 0, warnCount = 0;
    Parser(char* filename, bool skipws = true)
    {

        static const auto FILE_SIZE_MAX = 1 << 24;
        FILE* file = fopen(filename, "r");
        size_t flen = strlen(filename);

        this->filename = filename;
        noext = str_noext(filename);
        fseek(file, 0, SEEK_END);
        const size_t size = ftell(file) + 2;
        // 2 null chars, so we can always lookahead
        if (size < FILE_SIZE_MAX) {
            data = (char*)malloc(size);
            fseek(file, 0, SEEK_SET);
            fread(data, size, 1, file);
            data[size - 1] = 0;
            data[size - 2] = 0;
            moduleName = str_tr(noext, '/', '.');
            mangledName = str_tr(noext, '/', '_');
            capsMangledName = str_upper(mangledName);
            basename = str_base(noext, '/', flen);
            dirname = str_dir(noext);
            end = data + size;
            token.pos = data;
            token.flags.skipWhiteSpace = skipws;
        } else {
            fprintf(stderr, "Source files larger than 24MB are not allowed.\n");
        }

        fclose(file);
        return;
    }

#pragma mark - Error Reporting

    static const auto errLimit = 20;
#define fatal(str, ...)                                                        \
    {                                                                          \
        fprintf(stderr, str, __VA_ARGS__);                                     \
        exit(1);                                                               \
    }

    void errorIncrement()
    {
        if (++errCount >= errLimit)
            fatal("too many errors (%d), quitting\n", errLimit);
    }

#define errorExpectedToken(a) errorExpectedToken_(a, __func__)
    void errorExpectedToken_(TokenKind expected, const char* funcName)
    {
        // fputs(dashes, stderr);
        fprintf(stderr,
            "(%d) \033[31merror:\033[0m %s at %s:%d:%d\n      expected "
            "'%s' "
            "found '%s'\n",
            errCount + 1, funcName, filename, token.line, token.col,
            TokenKind_repr(expected), token.repr());
        errorIncrement();
    }

#define errorParsingExpr() errorParsingExpr_(__func__)
    void errorParsingExpr_(const char* funcName)
    {
        // fputs(dashes, stderr);
        fprintf(stderr,
            "(%d) \033[31merror:\033[0m %s at %s line %d or %d\n"
            "      failed to parse expr",
            errCount + 1, funcName, filename, token.line - 1, token.line);
        // parseExpr will move to next line IF there was no hanging comment
        errorIncrement();
    }

#define errorInvalidIdent(expr) errorInvalidIdent_(__func__, expr)
    void errorInvalidIdent_(const char* funcName, ASTExpr* expr)
    {
        // fputs(dashes, stderr);
        fprintf(stderr,
            "(%d) \033[31merror:\033[0m invalid name '%.*s' at %s:%d:%d\n",
            errCount + 1, expr->strLength, expr->value.string, filename,
            expr->line - 1,
            expr->col); // parseExpr will move to next line
        errorIncrement();
    }

#define errorUnrecognizedVar(expr) errorUnrecognizedVar_(__func__, expr)
    void errorUnrecognizedVar_(const char* funcName, ASTExpr* expr)
    {
        // fputs(dashes, stderr);
        fprintf(stderr,
            "(%d) \033[31merror:\033[0m unknown var '%.*s' at %s:%d:%d\n",
            errCount + 1, expr->strLength, expr->value.string, filename,
            expr->line, expr->col);
        errorIncrement();
    }
    //#define errorUnrecognizedVar(expr) errorUnrecognizedVar_(__func__,
    // expr)
    void errorDuplicateVar(ASTVar* var, ASTVar* orig)
    {
        // fputs(dashes, stderr);
        fprintf(stderr,
            "(%d) \033[31merror:\033[0m duplicate var '%s' at %s:%d:%d\n   "
            "          already declared at %s:%d:%d\n",
            errCount + 1, var->name, filename, var->init->line, 9, filename,
            orig->init->line,
            9); // every var has init!! and every var is indented 4 spc ;-)
        errorIncrement();
    }

#define errorUnrecognizedFunc(expr) errorUnrecognizedFunc_(__func__, expr)
    void errorUnrecognizedFunc_(const char* funcName, ASTExpr* expr)
    {
        // fputs(dashes, stderr);
        fprintf(stderr,
            "(%d) \033[31merror:\033[0m unknown function '%.*s' at "
            "%s:%d:%d\n",
            errCount + 1, expr->strLength, expr->value.string, filename,
            expr->line, expr->col);
        errorIncrement();
    }
#define errorUnrecognizedType(expr) errorUnrecognizedType_(__func__, expr)
    void errorUnrecognizedType_(const char* funcName, ASTTypeSpec* typeSpec)
    {
        // fputs(dashes, stderr);
        // if it is unrecognized, its typeType is TYUnresolved and its
        // ->name exists.
        fprintf(stderr,
            "(%d) \033[31merror:\033[0m unknown type '%s' at %s:%d:%d\n",
            errCount + 1, typeSpec->name, filename, typeSpec->line,
            typeSpec->col);
        errorIncrement();
    }

#define errorUnexpectedToken() errorUnexpectedToken_(__func__)
    void errorUnexpectedToken_(const char* funcName)
    {
        // fputs(dashes, stderr);
        fprintf(stderr,
            "(%d) \033[31merror:\033[0m %s at %s:%d:%d\n      unexpected "
            "token "
            "'%.*s'\n",
            errCount + 1, funcName, filename, token.line, token.col,
            token.matchlen, token.pos);
        errorIncrement();
    }

#define errorUnexpectedExpr(e) errorUnexpectedExpr_(e, __func__)
    void errorUnexpectedExpr_(const ASTExpr* expr, const char* funcName)
    {
        // fputs(dashes, stderr);
        fprintf(stderr,
            // isatty(stderr)?
            "(%d) \033[31merror:\033[0m %s at %s:%d:%d\n      unexpected "
            "expr "
            "'%.*s'", //:
                      // "(%d) error: %s at %s:%d:%d\n      unexpected expr
                      // '%.*s'" ,
            errCount + 1, funcName, filename, expr->line, expr->col,
            expr->opPrecedence ? 100 : expr->strLength,
            expr->opPrecedence ? TokenKind_repr(expr->kind) : expr->name);
        errorIncrement();
    }

#pragma mark -

    ASTExpr* exprFromCurrentToken()
    {
        auto expr = new ASTExpr(&token);
        token.advance();
        return expr;
    }

    ASTExpr* next_token_node(TokenKind expected, const bool ignore_error)
    {
        if (token.kind == expected) {
            return exprFromCurrentToken();
        } else {
            if (not ignore_error) errorExpectedToken(expected);
            return NULL;
        }
    }
    // these should all be part of Token_ when converted back to C
    // in the match case, token should be advanced on error
    ASTExpr* match(TokenKind expected)
    {
        return next_token_node(expected, false);
    }

    // this returns the match node or null
    ASTExpr* trymatch(TokenKind expected)
    {
        return next_token_node(expected, true);
    }

    // just yes or no, simple
    bool matches(TokenKind expected) { return (token.kind == expected); }

    bool ignore(TokenKind expected)
    {
        bool ret;
        if ((ret = matches(expected))) token.advance();
        return ret;
    }

    // this is same as match without return
    void discard(TokenKind expected)
    {
        if (not ignore(expected)) errorExpectedToken(expected);
    }

    char* parseIdent()
    {
        if (token.kind != TKIdentifier) errorExpectedToken(TKIdentifier);
        char* p = token.pos;
        token.advance();
        return p;
    }

    //    void resolveFunc(ASTFunc* func, ASTModule* mod) {}
    void resolveTypeSpec(ASTTypeSpec* typeSpec, ASTModule* mod)
    {
        if (typeSpec->typeType != TYUnresolved) return;

        TypeTypes tyty = TypeType_TypeTypeforSpec(typeSpec->name);
        if (tyty) // can be member of ASTTypeSpec!
        {
            typeSpec->typeType = tyty;
        } else {
            foreach (type, types, mod->types) {

                // ident ends have been trampled by the time types are
                // checked, so you don't need strncmp
                if (!strcmp(typeSpec->name, type->name)) {
                    // so what do you do  if types are "resolved"? Set
                    // typeType and collectionType?
                    //                printf("%s matched")
                    typeSpec->typeType = TYObject;
                    typeSpec->type = type; // is in a union with name remem
                    //                expr->func = func;
                    return;
                }
            }
            errorUnrecognizedType(typeSpec);
            return;
        }
        if (typeSpec->dims) {
            // set collection type, etc.
            // for this we will need info about the var and its usage
            // patterns. so this will probably be a separate func that is
            // called during such analysis.
        }
    }

    void resolveTypeSpecsInExpr(ASTExpr* expr, ASTModule* mod)
    {
        //        if (expr->kind == TKFunctionCall) {
        //            foreach (func, funcs, mod->funcs) {
        //                if (!strncmp(expr->name, func->name,
        //                expr->strLength) and
        //                func->name[expr->strLength]=='\0') {
        //                    expr->kind = TKFunctionCallResolved;
        //                    expr->func = func;
        //                    resolveFuncCall(expr->left, mod); // check
        //                    nested func calls return;
        //                }
        //            } // since it is known which module the func must be
        //            found in, no need to scan others
        //              // function has not been found
        //            errorUnrecognizedFunc(expr);
        //            resolveFuncCall(expr->left, mod); // but still check
        //            nested func calls
        //        }
        //        else
        if (expr->kind == TKVarAssign and expr->var->typeSpec) {
            resolveTypeSpec(expr->var->typeSpec, mod);
        } else {
            //            if (expr->opPrecedence) {
            //                if (!expr->opIsUnary)
            //                resolveFuncCall(expr->left, mod);
            //                resolveFuncCall(expr->right, mod);
            //            }
            //            else
            if (expr->kind == TKKeyword_if or expr->kind == TKKeyword_for
                or expr->kind == TKKeyword_while) {
                // this shouldnt be here, either resolveFuncCall should take
                // astscope* or there must be a helper func that takes
                // astscope* so you can descend easily
                //                resolveFuncCall(expr->left, mod);
                foreach (stmt, stmts, expr->body->stmts) {
                    resolveTypeSpecsInExpr(stmt, mod);
                }
            } // else assert(0);
        }
    }

    // TODO: Btw there should be a module level scope to hold lets (and
    // comments). That will be the root scope which has parent==NULL.
    void checkShadowing(ASTVar* var, ASTScope* scope) {}

    void resolveFuncCall(ASTExpr* expr, ASTModule* mod)
    {
        if (expr->kind == TKFunctionCallResolved) {
        } else if (expr->kind == TKFunctionCall) {
            foreach (func, funcs, mod->funcs) {
                if (!strncmp(expr->name, func->name, expr->strLength)
                    and func->name[expr->strLength] == '\0') {
                    expr->kind = TKFunctionCallResolved;
                    expr->func = func;
                    if (expr->left) resolveFuncCall(expr->left, mod);
                    // check nested func calls
                    return;
                }
            } // since it is known which module the func must be found in,
              // no need to scan others function has not been found
            errorUnrecognizedFunc(expr);
            if (expr->left)
                resolveFuncCall(
                    expr->left, mod); // but still check nested func calls
        } else if (expr->kind == TKVarAssign) {
            //            if (!expr->var->init) errorMissingInit(expr);
            resolveFuncCall(expr->var->init, mod);
        } else {
            if (expr->opPrecedence) {
                if (!expr->opIsUnary) resolveFuncCall(expr->left, mod);
                resolveFuncCall(expr->right, mod);
            } else if (expr->kind == TKKeyword_if or expr->kind == TKKeyword_for
                or expr->kind == TKKeyword_while) {
                // this shouldnt be here, either resolveFuncCall should take
                // astscope* or there must be a helper func that takes
                // astscope* so you can descend easily
                resolveFuncCall(expr->left, mod);
                foreach (stmt, stmts, expr->body->stmts) {
                    resolveFuncCall(stmt, mod);
                }
            } // else assert(0);
        }
    }

    void resolveVars(ASTExpr* expr, ASTScope* scope)
    {
        if (expr->kind == TKIdentifierResolved) {
        } else if (expr->kind == TKIdentifier or expr->kind == TKSubscript) {
            TokenKind ret = (expr->kind == TKIdentifier) ? TKIdentifierResolved
                                                         : TKSubscriptResolved;
            ASTScope* scp = scope;
            do {
                foreach (local, locals, scp->locals) {
                    if (!strncmp(expr->name, local->name, expr->strLength)
                        and local->name[expr->strLength] == '\0') {
                        expr->kind = ret;
                        expr->var = local; // this overwrites name btw
                        //                        printf("got
                        //                        %s\n",local->name);
                        goto getout;
                    }
                }
                scp = scp->parent;
            } while (scp);
            errorUnrecognizedVar(expr);
            //            printf("unresolved %s\n",expr->name);
        getout:
            if (ret == TKSubscriptResolved) resolveVars(expr->left, scope);
            // descend into the args of the subscript and resolve inner vars
        } else if (expr->kind == TKFunctionCall) {
            if (expr->left) resolveVars(expr->left, scope);
        } else {
            if (expr->opPrecedence) {
                if (!expr->opIsUnary) resolveVars(expr->left, scope);
                resolveVars(expr->right, scope);
            }
        }
    }

#pragma mark -
    ASTExpr* parseExpr()
    {
        // there are 2 steps to this madness.
        // 1. parse a sequence of tokens into RPN using shunting-yard.
        // 2. walk the rpn stack as a sequence and copy it into a result
        // stack, collapsing the stack when you find nonterminals (ops, func
        // calls, array index, ...)

        // we could make this static and set len to 0 upon func exit
        static Stack<ASTExpr*, 32> rpn, ops, result;
        int prec_top = 0;
        ASTExpr* p = NULL;
        TokenKind revBrkt = TKUnknown;

        // ******* STEP 1 CONVERT TOKENS INTO RPN

        while (token.kind != TKNullChar and token.kind != TKNewline
            and token.kind != TKLineComment) { // build RPN

            // you have to ensure that ops have a space around them, etc.
            // so don't just skip the one spaces like you do now.
            if (token.kind == TKOneSpace) token.advance();

            ASTExpr* expr = new ASTExpr(&token); // dont advance yet
            int prec = expr->opPrecedence;
            bool rassoc = prec ? expr->opIsRightAssociative : false;
            char lookAheadChar = token.peekCharAfter();

            switch (expr->kind) {
            case TKIdentifier:
                if (memchr(expr->value.string, '_', expr->strLength)
                    or doesKeywordMatch(expr->value.string, expr->strLength))
                    errorInvalidIdent(expr); // but continue parsing
                switch (lookAheadChar) {
                case '(':
                    expr->kind = TKFunctionCall;
                    expr->opPrecedence = 100;
                    ops.push(expr);
                    break;
                case '[':
                    expr->kind = TKSubscript;
                    expr->opPrecedence = 100;
                    ops.push(expr);
                    break;
                default:
                    rpn.push(expr);
                    break;
                }
                break;
            case TKParenOpen:
                ops.push(expr);
                if (not ops.empty() and ops.top()->kind == TKFunctionCall)
                    rpn.push(expr);
                if (lookAheadChar == ')')
                    rpn.push(NULL); // for empty func() push null for no args
                break;

            case TKArrayOpen:
                ops.push(expr);
                if (not ops.empty() and ops.top()->kind == TKSubscript)
                    rpn.push(expr);
                if (lookAheadChar == ')')
                    rpn.push(NULL); // for empty arr[] push null for no args
                break;

            case TKParenClose:
            case TKArrayClose:
            case TKBraceClose:

                revBrkt = TokenKind_reverseBracket(expr->kind);
                if (ops.empty()) { // need atleast the opening bracket of
                                   // the current kind
                    errorParsingExpr();
                    goto error;
                }

                else
                    while (not ops.empty()) {
                        p = ops.pop();
                        if (p->kind == revBrkt) break;
                        rpn.push(p);
                    }
                // we'll push the TKArrayOpen as well to indicate a list
                // literal or comprehension
                // TKArrayOpen is a unary op.
                if ((p and p->kind == TKArrayOpen)
                    and (ops.empty() or ops.top()->kind != TKSubscript)
                    // don't do this if its part of a subscript
                    and (rpn.empty() or rpn.top()->kind != TKOpColon))
                    // or aa range. range exprs are handled separately. by
                    // themselves they don't need a surrounding [], but for
                    // grouping like 2+[8:66] they do.
                    rpn.push(p);

                break;
            case TKKeyword_return:
                // for empty return, push a NULL if there is no expr coming.
                ops.push(expr);
                if (lookAheadChar == '!' or lookAheadChar == '\n')
                    rpn.push(NULL);
                break;
            default:
                if (prec) { // general operators

                    if (expr->kind == TKOpColon) {
                        if (rpn.empty()
                            or (!rpn.top() and !ops.empty()
                                and ops.top()->kind != TKOpColon)
                            or (rpn.top() and rpn.top()->kind == TKOpColon
                                and !ops.empty()
                                and (ops.top()->kind == TKOpComma
                                    or ops.top()->kind
                                        == TKArrayOpen)) // <<-----
                                                         // yesssssssssss
                        )

                            rpn.push(NULL); // indicates empty operand
                    }
                    while (not ops.empty()) {
                        prec_top = ops.top()->opPrecedence;
                        if (not prec_top) break; // left parenthesis
                        if (prec > prec_top) break;
                        if (prec == prec_top and rassoc) break;
                        p = ops.pop();

                        if (p->kind != TKOpComma and p->kind != TKOpSemiColon
                            and p->kind != TKFunctionCall
                            and p->kind != TKSubscript and rpn.top()
                            and rpn.top()->kind == TKOpComma) {
                            errorUnexpectedToken();
                            goto error;
                        }

                        if (!(p->opPrecedence or p->opIsUnary)
                            and p->kind != TKFunctionCall
                            and p->kind != TKOpColon
                            // in case of ::, second colon will add null
                            // later
                            and p->kind != TKSubscript and rpn.count < 2) {
                            errorUnexpectedToken();
                            goto error;
                            // TODO: even if you have more than two, neither
                            // of the top two should be a comma
                        }

                        rpn.push(p);
                    }

                    // when the first error is found in an expression, seek
                    // to the next newline and return NULL.
                    if (rpn.empty()) {
                        errorUnexpectedToken();
                        goto error;
                    }
                    if (expr->kind == TKOpColon
                        and (lookAheadChar == ',' or lookAheadChar == ':'
                            or lookAheadChar == ']' or lookAheadChar == ')'))
                        rpn.push(NULL);

                    ops.push(expr);
                } else {
                    rpn.push(expr);
                }
            }
            token.advance();
            if (token.kind == TKOneSpace) token.advance();
        }
    exitloop:

        while (not ops.empty()) {
            p = ops.pop();

            if (p->kind != TKOpComma and p->kind != TKFunctionCall
                and p->kind != TKSubscript and p->kind != TKArrayOpen
                and rpn.top() and rpn.top()->kind == TKOpComma) {
                errorUnexpectedExpr(rpn.top());
                goto error;
            }

            if (!(p->opPrecedence or p->opIsUnary)
                and (p->kind != TKFunctionCall and p->kind != TKSubscript)
                and rpn.count < 2) {
                errorParsingExpr();
                goto error;
                // TODO: even if you have more than two, neither of the top
                // two should be a comma
            }

            rpn.push(p);
        }

        // *** STEP 2 CONVERT RPN INTO EXPR TREE

        ASTExpr* arg;
        for (int i = 0; i < rpn.count; i++) {
            if (!(p = rpn[i])) goto justpush;
            switch (p->kind) {
            case TKFunctionCall:
            case TKSubscript:
                if (result.count > 0) {
                    arg = result.pop();
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
                if (!p->opPrecedence) {
                    errorParsingExpr();
                    goto error;
                }

                if (result.empty()) {
                    errorParsingExpr();
                    goto error;
                }

                p->right = result.pop();

                if (not p->opIsUnary) {
                    if (result.empty()) {
                        errorParsingExpr();
                        goto error;
                    }
                    p->left = result.pop();
                }
            }
        justpush:
            result.push(p);
        }
        if (result.count != 1) {
            errorParsingExpr();
            goto error;
        }

        // TODO: run evalType() and report errors

        // for next invocation just set count to 0, allocation will be
        // reused
        ops.count = 0;
        rpn.count = 0;
        result.count = 0;
        return result[0];

    error:

        while (token.pos < this->end
            and (token.kind != TKNewline and token.kind != TKLineComment))
            token.advance();

        if (ops.count) printf("\n      ops: ");
        for (int i = 0; i < ops.count; i++)
            printf("%s ", TokenKind_repr(ops[i]->kind));

        if (rpn.count) printf("\n      rpn: ");
        for (int i = 0; i < rpn.count; i++)
            if (!rpn[i])
                printf("NUL ");
            else
                printf("%.*s ", rpn[i]->opPrecedence ? 100 : rpn[i]->strLength,
                    rpn[i]->opPrecedence ? TokenKind_repr(rpn[i]->kind)
                                         : rpn[i]->name);

        if (result.count) printf("\n      rpn: ");
        for (int i = 0; i < result.count; i++)
            if (!result[i])
                printf("NUL ");
            else
                printf("%.*s ",
                    result[i]->opPrecedence ? 100 : result[i]->strLength,
                    result[i]->opPrecedence ? TokenKind_repr(result[i]->kind)
                                            : result[i]->name);

        if (p)
            printf("\n      p: %.*s ", p->opPrecedence ? 100 : p->strLength,
                p->opPrecedence ? TokenKind_repr(p->kind) : p->name);

        if (rpn.count or ops.count or result.count) puts("");

        ops.count = 0; // "reset" stacks
        rpn.count = 0;
        result.count = 0;
        return NULL;
    }

#pragma mark -
    ASTTypeSpec* parseTypeSpec()
    { // must have ident(U), then may have "[:,:]" i.e. '[\]\[\:, ]+' , then
      // may have units. note: after ident may have params <T, S>
        token.flags.mergeArrayDims = true;

        auto typeSpec = new ASTTypeSpec;
        typeSpec->line = token.line;
        typeSpec->col = token.col;

        typeSpec->name = parseIdent();
        //        typeSpec->params = parseParams();
        // have a loop and switch here so you can parse both
        // Number[:,:]|kg/s and Number|kg/s[:,:]. linter will fix.

        if (matches(TKArrayDims)) {
            for (int i = 0; i < token.matchlen; i++)
                if (token.pos[i] == ':') typeSpec->dims++;
            if (!typeSpec->dims) typeSpec->dims = 1; // [] is 1 dim
            token.advance();
        }

        ignore(TKUnits);
        // fixme: node->type = lookupType;

        assert(token.kind != TKUnits);
        assert(token.kind != TKArrayDims);

        token.flags.mergeArrayDims = false;
        return typeSpec;
    }

    PtrList<ASTVar> parseArgs()
    {
        PtrList<ASTVar> args;
        token.flags.mergeArrayDims = true;
        discard(TKParenOpen);
        if (ignore(TKParenClose)) return args;

        ASTVar* arg;
        do {
            arg = parseVar();
            args.append(arg);
        } while (ignore(TKOpComma));

        discard(TKParenClose);

        token.flags.mergeArrayDims = false;
        return args;
    }

#pragma mark -
    ASTVar* parseVar()
    {
        auto var = new ASTVar;
        var->flags.isVar = (token.kind == TKKeyword_var);
        var->flags.isLet = (token.kind == TKKeyword_let);

        if (var->flags.isVar) discard(TKKeyword_var);
        if (var->flags.isLet) discard(TKKeyword_let);
        if (var->flags.isVar or var->flags.isLet) discard(TKOneSpace);

        var->name = parseIdent();

        if (ignore(TKOneSpace) and ignore(TKKeyword_as)) {
            discard(TKOneSpace);
            var->typeSpec = parseTypeSpec();
        }

        ignore(TKOneSpace);
        if (ignore(TKOpAssign)) var->init = parseExpr();

        // TODO: set default inferred type if typeSpec is NULL but init
        // present.
        //     const char* ctyp
        // = TokenKind_defaultType(var->init ? var->init->kind : TKUnknown);

        return var;
    }

#pragma mark -
    ASTScope* parseScope(ASTScope* parent)
    {
        auto scope = new ASTScope;

        ASTVar *var = NULL, *orig = NULL;
        ASTExpr* expr = NULL;
        TokenKind tt = TKUnknown;

        scope->parent = parent;

        // don't conflate this with the while in parse(): it checks against
        // file end, this checks against the keyword 'end'.
        while (token.kind != TKKeyword_end) {

            switch (token.kind) {

            case TKNullChar:
                errorExpectedToken(TKUnknown);
                goto exitloop;

            case TKKeyword_var:
            case TKKeyword_let:
                var = parseVar();
                if (!var) continue;
                if ((orig = scope->getVar(var->name)))
                    errorDuplicateVar(var, orig);
                if (var->init
                    and (var->init->opPrecedence
                        or var->init->kind == TKIdentifier))
                    resolveVars(var->init, scope);
                // resolveType(var->typeSpec, scope);
                // resolve BEFORE it is added to the list! in
                // `var x = x + 1` x should not resolve
                // if var->typeSpec is NULL then set the type
                // if it isn't NULL then check the types match
                scope->locals.append(var);
                // TODO: validation should raise issue if var->init is
                // missing
                expr = new ASTExpr;
                expr->kind = TKVarAssign;
                expr->line = token.line;
                expr->col = token.col;
                expr->opPrecedence = TokenKind_getPrecedence(TKOpAssign);
                expr->var = var;
                scope->stmts.append(expr);
                break;

            case TKKeyword_if:
            case TKKeyword_for:
            case TKKeyword_while:
                tt = token.kind;
                expr = match(tt); // will advance
                expr->left = parseExpr();
                resolveVars(expr->left, scope);
                // TODO: `for` necessarily introduces a counter variable, so
                // check if that var name doesn't already exist in scope.
                // Also assert that the cond of a for expr has kind
                // TKOpAssign.

                expr->body = parseScope(scope);
                discard(TKKeyword_end);
                discard(TKOneSpace);
                discard(tt);
                scope->stmts.append(expr);
                break;

            case TKNewline:
            case TKOneSpace: // found at beginning of line]
                token.advance();
                break;

            case TKLineComment:
                if (generateCommentExprs) {
                    expr = new ASTExpr(&token);
                    scope->stmts.append(expr);
                }
                token.advance();
                break;

            default:
                expr = parseExpr();
                if (!expr) break;
                scope->stmts.append(expr);
                resolveVars(expr, scope);
                break;
            }
        }
    exitloop:
        return scope;
    }

#pragma mark -
    PtrList<ASTVar> parseParams()
    {
        discard(TKOpLT);
        PtrList<ASTVar> params;
        ASTVar* param;
        do {
            param = new ASTVar;
            param->name = parseIdent();
            if (ignore(TKKeyword_as)) param->typeSpec = parseTypeSpec();
            if (ignore(TKOpAssign)) param->init = parseExpr();
            params.append(param);
        } while (ignore(TKOpComma));

        discard(TKOpGT);
        return params;
    }

#pragma mark -
    ASTFunc* parseFunc(bool shouldParseBody = true)
    {
        discard(TKKeyword_function);
        discard(TKOneSpace);
        auto func = new ASTFunc;

        func->line = token.line;
        func->col = token.col;

        func->name = parseIdent();

        func->flags.isDeclare = !shouldParseBody;

        if (!strcmp(func->name, "print")) func->flags.throws = 0;

        func->args = parseArgs();
        if (ignore(TKOneSpace) and ignore(TKKeyword_returns)) {
            discard(TKOneSpace);
            func->returnType = parseTypeSpec();
        }

        if (shouldParseBody) {
            discard(TKNewline);

            auto funcScope = new ASTScope;
            funcScope->locals = func->args;
            func->body = parseScope(funcScope);

            discard(TKKeyword_end);
            discard(TKOneSpace);
            discard(TKKeyword_function);
        } else {
            func->body = NULL;
        }

        return func;
    }

    ASTFunc* parseStmtFunc()
    {
        auto func = new ASTFunc;

        func->line = token.line;
        func->col = token.col;

        func->name = parseIdent();
        func->args = parseArgs();
        if (ignore(TKOneSpace) and ignore(TKKeyword_as)) {
            discard(TKOneSpace);
            func->returnType = parseTypeSpec();
        }

        auto ret = exprFromCurrentToken();
        assert(ret->kind == TKColEq);
        ret->kind = TKKeyword_return;
        ret->opIsUnary = true;

        ret->right = parseExpr();
        auto scope = new ASTScope;
        scope->stmts.append(ret);

        func->body = scope;

        return func;
    }

#pragma mark -
    ASTFunc* parseTest() { return NULL; }

#pragma mark -
    ASTUnits* parseUnits() { return NULL; }

#pragma mark -
    ASTType* parseType()
    {
        auto type = new ASTType;

        ASTExpr* expr;
        ASTVar *var, *orig;

        discard(TKKeyword_type);
        discard(TKOneSpace);
        type->name = parseIdent();
        if (matches(TKOpLT)) type->params = parseParams();

        if (ignore(TKOneSpace) and ignore(TKKeyword_extends)) {
            discard(TKOneSpace);
            type->super = parseTypeSpec();
        }

        // JUST parse base manually here and then call parseScope!
        // at the end of calling parseScope, examine the resulting scope
        // and raise an error if anything other than var or check is found

        // DEFINITELY shouldbe calling parseScope here
        while (token.kind != TKKeyword_end) {
            switch (token.kind) {
            case TKNullChar:
                errorUnexpectedToken();
                goto exitloop;

            case TKKeyword_var:
                var = parseVar();
                if (!var) continue;
                if ((orig = type->getVar(var->name)))
                    errorDuplicateVar(var, orig);
                // resolveVars(var->init, <#ASTScope *scope#>) SEE WHY WE
                // NEED TO CALL parseScope here?!
                type->vars.append(var);
                // vars holds the local vars just like parseScope
                // does. but each var also goes in the expr list
                // to keep ordering. the expr list is named
                // 'checks' unfortunately.

                expr = new ASTExpr; // skip comments when not linting
                expr->kind = TKVarAssign;
                expr->opPrecedence = TokenKind_getPrecedence(TKOpAssign);
                expr->var = var;
                expr->line = token.line;
                expr->col = token.col;

                type->checks.append(expr);
                break;

            case TKKeyword_extends:
                discard(TKKeyword_extends);
                discard(TKOneSpace);
                type->super = parseTypeSpec();
                break;

            case TKNewline:
            case TKOneSpace:
                token.advance();
                break;
            case TKLineComment:
                if (generateCommentExprs) {
                    expr = new ASTExpr(&token);
                    type->checks.append(expr);
                }
                token.advance();
                break;
            case TKIdentifier:
                if (!strncmp("check", token.pos, 5)) {
                    expr = parseExpr();
                    type->checks.append(expr);
                    break;
                }
            default:
                // general exprs are not allowed. Report the error as
                // unexpected token (first token on the line), then seek to
                // the next newline.
                errorUnexpectedToken();
                fprintf(stderr,
                    "      only 'var', 'let', and 'check' statements are "
                    "allowed in types\n");
                while (token.kind != TKNewline and token.kind != TKLineComment)
                    token.advance();
                break;
            }
        }
    exitloop:

        discard(TKKeyword_end);
        discard(TKOneSpace);
        discard(TKKeyword_type);

        return type;
    }

#pragma mark -
    ASTImport* parseImport()
    {
        auto import = new ASTImport;
        char* tmp;
        discard(TKKeyword_import);
        discard(TKOneSpace);

        import->isPackage = ignore(TKAt);
        import->importFile = parseIdent();
        size_t len = token.pos - import->importFile;
        ignore(TKOneSpace);
        if (ignore(TKKeyword_as)) {

            ignore(TKOneSpace);
            import->hasAlias = true;
            tmp = parseIdent();
            if (tmp) import->aliasOffset = (uint32_t)(tmp - import->importFile);

        } else {
            import->aliasOffset = (uint32_t)(
                str_base(import->importFile, '.', len) - import->importFile);
        }

        ignore(TKOneSpace);

        if (token.kind != TKLineComment and token.kind != TKNewline)
            errorUnexpectedToken();
        while (token.kind != TKLineComment and token.kind != TKNewline)
            token.advance();
        return import;
    }

#pragma mark -
    PtrList<ASTModule> parse()
    {
        auto root = new ASTModule;
        root->name = moduleName;
        const bool onlyPrintTokens = false;
        token.advance(); // maybe put this in parser ctor
        ASTImport* import = NULL;

        // The take away is (for C gen):
        // Every caller who calls List->append() should keep a local List*
        // to follow the list top as items are appended. Each actual append
        // call must be followed by an update of this pointer to its own
        // ->next. Append should be called on the last item of the list, not
        // the first. (it will work but seek through the whole list every
        // time).

        PtrList<ASTFunc>* funcsTop = &root->funcs;
        PtrList<ASTImport>* importsTop = &root->imports;
        PtrList<ASTType>* typesTop = &root->types;
        PtrList<ASTFunc>* testsTop = &root->tests;
        PtrList<ASTVar>* globalsTop = &root->globals;

        while (token.kind != TKNullChar) {
            if (onlyPrintTokens) {
                printf("%s %2d %3d %3d %-6s\t%.*s\n", basename, token.line,
                    token.col, token.matchlen, TokenKind_repr(token.kind),
                    token.kind == TKNewline ? 0 : token.matchlen, token.pos);
                token.advance();
                continue;
            }
            switch (token.kind) {
            case TKKeyword_declare:
                token.advance(); // discard(TKKeyword_declare);
                discard(TKOneSpace);
                funcsTop->append(parseFunc(false));
                if (funcsTop->next) funcsTop = funcsTop->next;
                break;

            case TKKeyword_function:
                funcsTop->append(parseFunc());
                if (funcsTop->next) funcsTop = funcsTop->next;
                break;
            // case TKKeyword_declare:
            //     funcsTop
            case TKKeyword_type:
                typesTop->append(parseType());
                if (typesTop->next) typesTop = typesTop->next;
                break;
            case TKKeyword_import:
                import = parseImport();
                if (import) {
                    importsTop->append(import);
                    if (importsTop->next) importsTop = importsTop->next;
                    //                    auto subParser = new
                    //                    Parser(import->importFile);
                    //                    List<ASTModule*> subMods =
                    //                    subParser->parse();
                    //                    modules.append(subMods);
                }
                break;
            case TKKeyword_test:
                testsTop->append(parseTest());
                if (testsTop->next) testsTop = testsTop->next;
                break;
            case TKKeyword_var:
            case TKKeyword_let:
                globalsTop->append(parseVar());
                if (globalsTop->next) globalsTop = globalsTop->next;
                break;
            case TKNewline:
            case TKLineComment:
            case TKOneSpace:
                token.advance();
                break;
            case TKIdentifier: // stmt funcs: f(x) := f(y, w = 4) etc.
                if (token.peekCharAfter() == '(') {
                    funcsTop->append(parseStmtFunc());
                    if (funcsTop->next) funcsTop = funcsTop->next;
                    break;
                }
            default:
                printf("other token: %s at %d:%d len %d\n", token.repr(),
                    token.line, token.col, token.matchlen);
                errorUnexpectedToken();
                while (token.kind != TKNewline and token.kind != TKLineComment)
                    token.advance();
            }
        }
        // also keep modulesTop

        // do some analysis that happens after the entire module is loaded
        foreach (func, funcs, root->funcs) {
            if (!func->body) continue;
            foreach (arg, args, func->args) {
                resolveTypeSpec(arg->typeSpec, root);
            }
            if (func->returnType) resolveTypeSpec(func->returnType, root);
            foreach (stmt, stmts, func->body->stmts) {
                resolveFuncCall(
                    stmt, root); // should be part of astmodule, and
                                 // resolveVars should be part of astscope
                resolveTypeSpecsInExpr(stmt, root);
            }
        }

        modules.append(root);
        return modules;
    }
    void genc_open()
    {
        printf("#ifndef HAVE_%s\n#define HAVE_%s\n\n", this->capsMangledName,
            this->capsMangledName);
        printf("#define THISMODULE %s\n", this->mangledName);
        printf("#define THISFILE \"%s\"\n", this->filename);
        printf("#line 1 \"%s\"\n", this->filename);
        //        puts("#ifdef DEBUG");
        //        printf("static const char* %s_filename =
        //        \"%s\";\n",this->mangledName, this->filename);
        //        printf("static const char* %s_basename =
        //        \"%s\";\n",this->mangledName, this->basename);
        //        printf("static const char* %s_modname = \"%s\";\n",
        //        this->mangledName,this->moduleName); puts("#endif");
    }
    void genc_close()
    {
        printf("#undef THISMODULE\n");
        printf("#endif // HAVE_%s\n", this->capsMangledName);
    }
};