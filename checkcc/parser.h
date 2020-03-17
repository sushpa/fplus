
void Parser_advance(Parser* parser) { Token_advance(&parser->token); }

Node* nodeFromCurrentToken(Parser* parser)
{
    Token* token = &parser->token;

    Node* node = alloc_Node();
    *node = (Node) { //
        .kind = NKExpr,
        .subkind = token->kind,
        .len = token->matchlen,
        .line = token->line,
        .col = token->col,
        .name = Token_strdup(token),
        .next = NULL
    };
    // for some kinds, there is associated data to be saved
    switch (token->kind) {
    case TKString:
    case TKRegex:
    case TKIdentifier:
    case TKNumber: // not converting for now
    case TKLineComment:
        node->value.string = Token_strdup(token);
        break;
    //    node->value.real = strtod(token->pos, NULL);
    //   break;
    default:
        // error_unexpectedToken(parser, token->kind);
        node->prec = Token_getPrecedence(token->kind);
        node->rassoc = Token_isRightAssociative(token->kind);
        node->unary = Token_isUnary(token->kind);
        break;
    }
    if (token->kind == TKNumber) {
        // turn all 1.0234[DdE]+01 into 1.0234e+01
        str_tr_ip(node->value.string, 'd', 'e');
        str_tr_ip(node->value.string, 'D', 'e');
        str_tr_ip(node->value.string, 'E', 'e');
    }
    Parser_advance(parser);
    return node;
}

#pragma mark Parsing Primitives

Node* next_token_node(
    Parser* parser, TokenKind expected, const bool_t ignore_error)
{
    if (parser->token.kind == expected) {
        return nodeFromCurrentToken(parser);
    } else {
        if (!ignore_error) error_unexpectedToken(parser, expected);
        return NULL;
    }
}

// in the Parser_match case, token should be advanced on error
Node* Parser_match(Parser* parser, TokenKind expected)
{
    return next_token_node(parser, expected, false);
}

// this returns the Parser_match node or null
Node* Parser_trymatch(Parser* parser, TokenKind expected)
{
    return next_token_node(parser, expected, true);
}

// just yes or no, simple
bool_t Parser_matches(Parser* parser, TokenKind expected)
{
    return (parser->token.kind == expected);
}

bool_t Parser_ignore(Parser* parser, TokenKind expected)
{
    bool_t ret;
    if ((ret = Parser_matches(parser, expected))) Parser_advance(parser);
    return ret;
}

// this is same as Parser_match without return
void Parser_discard(Parser* parser, TokenKind expected)
{
    if (!Parser_ignore(parser, expected))
        error_unexpectedToken(parser, expected);
}
//=============================================================================
// PARSER RULE-MATCHING FUNCTIONS
//=============================================================================
#pragma mark Parser Rules

char* Parser_parseIdent(Parser* parser)
{
    char* p = Token_strdup(&parser->token);
    Parser_advance(parser);
    return p;
}

Node* Parser_parseExpr(Parser* parser)
{
    Token* token = &parser->token;

    // there are 2 steps to this madness.
    // 1. parse a sequence of tokens into RPN using shunting-yard.
    // 2. walk the rpn stack as a sequence and copy it into a result stack,
    // collapsing the stack when you find nonterminals
    // (ops, func calls, array index, ...)

    // we could make this static and set len to 0 upon func exit
    NodeStack rpn = { NULL, 0, 0 }, ops = { NULL, 0, 0 };
    int prec_top = 0;
    Node* p=NULL;

    // ******* STEP 1 CONVERT TOKENS INTO RPN

    while (token->kind != TKNewline
        && token->kind != TKLineComment) { // build RPN
        //        int i;
        //        puts("\nop-stack:");
        //        for (i = 0; i < ops.count; i++)
        //            if (ops.items[i])
        //                printf("%s ", Token_repr(ops.items[i]->subkind));
        //        puts("\nres-stack:");
        //        for (i = 0; i < rpn.count; i++)
        //            if (rpn.items[i])
        //                printf("%s ", Token_repr(rpn.items[i]->subkind));
        //        puts("");

        Node* node = nodeFromCurrentToken(parser);
        int prec = node->prec;
        bool_t rassoc = node->rassoc;

        if (node->subkind == TKIdentifier && token->kind == TKParenOpen) {
            node->subkind = TKFunctionCall;
            node->prec = 100;
            NodeStack_push(&ops, node);
        } else if (node->subkind == TKIdentifier
            && token->kind == TKArrayOpen) {
            // NodeStack_push(&rpn, node);
            NodeStack_push(&ops, node);
        } else if (node->subkind == TKOpColon) {
        } else if (node->subkind == TKComma) {
            // e.g. in func args list etc.
            // don't need comma because identf/identa nodes will save nargs.
        } else if (node->subkind == TKParenOpen) {
            NodeStack_push(&ops, node);
            if (!NodeStack_empty(&ops)
                && NodeStack_top(&ops)->subkind == TKFunctionCall)
                NodeStack_push(&rpn, node);
            // instead of marking with (, could consider pushing a NULL.
            // (for both func calls & array indexes)

        } else if (node->subkind == TKArrayOpen) {
            NodeStack_push(&ops, node);
            if (!NodeStack_empty(&ops)
                && NodeStack_top(&ops)->subkind == TKSubscript)
                NodeStack_push(&rpn, node);
        } else if (node->subkind == TKParenClose
            || node->subkind == TKArrayClose
            || node->subkind == TKBraceClose) {

            TokenKind revBrkt = reverseBracket(node->subkind);
            while (!NodeStack_empty(&ops)) {
                p = NodeStack_pop(&ops);
                if (p->subkind == revBrkt) break;
                NodeStack_push(&rpn, p);
            }

            // NodeStack_push(&result, node); // for funcs/arrays only. or
            // we could not use these as seps but mark the func ident as
            // TKIdentf and set nargs. NOPE u need to know nargs before
            // starting to parse args.
        } else if (prec) { // general operators

            while (!NodeStack_empty(&ops)) //
            {
                prec_top = NodeStack_top(&ops)->prec;
                if (!prec_top) break;
                if (prec > prec_top) break;
                if (prec == prec_top && rassoc) break;
                p = NodeStack_pop(&ops);
                NodeStack_push(&rpn, p);
            }
            NodeStack_push(&ops, node);
        } else {
            NodeStack_push(&rpn, node);
        }
    }
    while (!NodeStack_empty(&ops)) //
    {
        p = NodeStack_pop(&ops);
        NodeStack_push(&rpn, p);
    }

    int i;
    puts("\nop-stack:");
    for (i = 0; i < ops.count; i++) printf("%s ", Token_repr(ops.items[i]->subkind));
    puts("\nres-stack:");
    for (i = 0; i < rpn.count; i++) printf("%s ", Token_repr(rpn.items[i]->subkind));
    puts("");

    assert(ops.count == 0);

    // *** STEP 2 CONVERT RPN INTO EXPR TREE

    NodeStack result = { NULL, 0, 0 };

    for (i = 0; i < rpn.count; i++) {
        p = rpn.items[i];
        switch (p->subkind) {
        case TKFunctionCall:
             break;
        case TKNumber:
                          break;
        case TKIdentifier:
                            break;
        case TKParenOpen:
            break;
        default:
            // careful now, assuming everything except those above is a
                 // nonterminal and needs left/right
            if (!p->prec) continue;
            // operator must have some precedence, right?
            p->kind = NKExpr;
            if (!p->unary) p->right = NodeStack_pop(&result);
            p->left = NodeStack_pop(&result);
        }
        NodeStack_push(&result, p);
    }

    assert(result.count == 1);

    return result.items[0];
}

/*
Node* Parser_parse_expro(Parser* parser)
{
    Token* token = &parser->token;

    // we could make this static and set len to 0 upon func exit
    NodeStack result = { NULL, 0, 0 }, ops = { NULL, 0, 0 };
    int prec_top = 0;
    Node* p;

    while (token->kind != TKNl && token->kind != TKLinecomment) { // build
RPN

        Node* node = nodeFromCurrentToken(parser);
        int prec = node->prec; // token_prec(node->subkind);
        bool_t rassoc = node->rassoc; // token_rassoc(node->subkind);
        // build rpn
        // as it happens, node->subkind will contain the token type of the
token
        // currently processed,  and token->kind holds the token type of
what
        // follows (will be processed in the next iteration).
        if (node->subkind == TKIdent
            && token->kind == TKParen_open) { // func call
            NodeStack_push(&result, node);
        } else if (node->subkind == TKIdent
            && token->kind == TKArray_open) { // array
            NodeStack_push(&result, node);
        } else if (node->subkind == TKOp_colon) { // range op
        } else if (node->subkind == TKParen_open
            || node->subkind == TKArray_open || node->subkind ==
TKBrace_open) { NodeStack_push(&ops, node);
            // NodeStack_push(&result, node);
        } else if (node->subkind == TKParen_close
            || node->subkind == TKArray_close
            || node->subkind == TKBrace_close) {
            TokenKind revBrkt = reverseBracket(node->subkind);
            while (!NodeStack_empty(&ops)) {
                p = NodeStack_pop(&ops);
                if (p->subkind == revBrkt)
                    break;
                //                if (p->prec) {
                //                    p->kind = NKExpr;
                //                    if (!p->unary)
                //                        p->right = NodeStack_pop(&result);
                //                    p->left = NodeStack_pop(&result);
                //                }
                NodeStack_push(&result, p);
            }
            if (!NodeStack_empty(&ops)) {
                p = NodeStack_top(&ops);
                if (0 / * p in self.functions and token in ')>') or (p in
self.arrays and token == ']' * /) NodeStack_push(&result,
NodeStack_pop(&ops));
            }
            // NodeStack_push(&result, node);
        } else if (prec) { // general operators
            // shunting
            // before pushing an op on the op stack, unwind the op stack as
            // long as there are tighter-binding ops

            while (!NodeStack_empty(&ops)) //
            {
                prec_top = NodeStack_top(&ops)->prec;
                //                if (!prec) break;
                if (prec > prec_top)
                    break;
                if (prec == prec_top && rassoc)
                    break;
                p = NodeStack_pop(&ops);

                //                if (p->prec) {
                //                    p->kind = NKExpr;
                //                    if (!p->unary)
                //                        p->right = NodeStack_pop(&result);
                //                    p->left = NodeStack_pop(&result);
                //                }
                // if (p->subkind!=TKParen_open)
                NodeStack_push(&result, p);
                // if (p->subkind == NKIdentf)
                //     NodeStack_push(&result, ")");
                // if (p->subkind == NKIdenta)
                //     NodeStack_push(&result, "]");

                / * pop the op stack and NodeStack_push the result on the
rpn stack BUT in our idea, we eval the rpn stack on the fly so if you popped
a '+' , dont NodeStack_push it on the rpn yet create an add node, pop lparam
and rparam from the rpn and set them as args for the add node, and
NodeStack_push the add node directly this happens on each op pop! (in this
                 while loop) basically this while loop is going to be
                 repeated (without the prec cmp) when the tokens are
                 finished. * /
            }
            NodeStack_push(&ops, node);
        } else {
            //            printf("Parser_parse_expr: got '%s'\n",
            //            token_repr(node->subkind));
            NodeStack_push(&result, node);
        }

        // Parser_advance(parser);
        // } else // evaluate RPN and generate tree
        // {
        //     for (int i = 0; i < result_p; i++) {
        //         Node* node = result[i];
        //         int nk = node->kind == NKToken ? node->subkind
        //                                                : node->kind;

        //         switch (nk) {
        //         case NKIdent:
        //         }
        //     }
    }
    while (!NodeStack_empty(&ops)) //
    {
        p = NodeStack_pop(&ops);
        //        if (p->prec) {
        //            p->kind = NKExpr;
        //            if (!p->unary)
        //                p->right = NodeStack_pop(&result);
        //            p->left = NodeStack_pop(&result);
        //        }
        // if (p->subkind!=TKParen_open)
        NodeStack_push(&result, p);
        // if (p->subkind == NKIdentf)
        //     NodeStack_push(&result, ")");
        // if (p->subkind == NKIdenta)
        //     NodeStack_push(&result, "]");
    }

    int i;
    puts("\nop-stack:");
    for (i = 0; i < ops.count; i++)
        if (ops.items[i])
            printf("%s ", Token_repr(ops.items[i]->subkind));
    puts("\nres-stack:");
    for (i = 0; i < result.count; i++)
        if (result.items[i])
            printf("%s ", Token_repr(result.items[i]->subkind));
    puts("");

    assert(ops.count == 0);
    assert(result.count == 1);

    return result.items[0];
}
*/

Node* Parser_parseTypeSpec(Parser* parser)
{ // must have ident(U), then may have "[:,:]" i.e. '[\]\[\:, ]+' , then may
  // have units. note: after ident may have params <T, S>
    parser->token.flags.mergearraydims = true;

    Node* node = nodeFromCurrentToken(parser);
    node->kind = NKTypeSpec;
    // this is an ident, but we will turn it into a typespec.
    // name is in this token already
    node->dims = Parser_trymatch(parser, TKArrayDims);
    node->units = Parser_trymatch(parser, TKUnits);
    // fixme: node->type = lookupType;

    parser->token.flags.mergearraydims = false;
    return node;
}

Node* Parser_parseArgs(Parser* parser)
{
    Parser_match(parser, TKParenOpen);
    Node* node = NULL;
    Node* arg = NULL;
    do {
        if (Parser_matches(parser, TKIdentifier)) {
            arg = nodeFromCurrentToken(parser);
            arg->kind = NKVar;
            if (Parser_ignore(parser, TKOpColon))
                arg->typespec = Parser_parseTypeSpec(parser);
            if (Parser_ignore(parser, TKOpAssign))
                arg->init = Parser_parseExpr(parser);
            list_append(&node, arg);
            node->nArgs++;
        }
    } while (Parser_ignore(parser, TKComma));

    Parser_match(parser, TKParenClose);
    parser->token.flags.mergearraydims = false;
    return node;
}

Node* Parser_parseVar(Parser* parser)
{
    Node* node = nodeFromCurrentToken(parser);
    if (node->subkind == TKKeyword_var) {
        // var / let, read the next one. in func args you won't find this
        Parser_ignore(parser, TKOneSpace);
        node->name = Parser_parseIdent(parser);
        node->flags.var.isVar = true;
    }
    node->kind = NKVar;
    if (Parser_ignore(parser, TKOpColon)) {
#ifdef Parser_parse_STRICT
        Parser_discard(parser, TKOneSpace);
#endif
        node->typespec = Parser_parseTypeSpec(parser);
    }
#ifdef Parser_parse_STRICT
    Parser_discard(parser, TKOnespc);
#endif
    if (Parser_ignore(parser, TKOpAssign)) {
#ifdef Parser_parse_STRICT
        Parser_discard(parser, TKOneSpace);
#endif
        node->init = Parser_parseExpr(parser);
    }
    return node;
}

Node* Parser_parseScope(Parser* parser, Node* parent)
{
    TokenKind kind = parser->token.kind;
    Node* scope = alloc_Node();
    scope->kind = NKScope;
    Node* node = NULL;
    // don't conflate this with the while in parse(): it checks against file
    // end, this checks against the keyword 'end'.
    while ((kind = parser->token.kind) != TKKeyword_end) {
        switch (kind) {
        case TKNullChar:
            error_unexpectedToken(parser, TKUnknown);
            goto exitloop;
        case TKKeyword_var:
            if ((node = Parser_parseVar(parser)))
                list_append(&scope->stmts, node);
            break;
        case TKKeyword_base:
            Parser_discard(parser, TKKeyword_base);
            if (parent && parent->kind == NKType)
                parent->super = Parser_parseIdent(parser);
        case TKIdentifier: // check, return
            Parser_advance(parser); // fixme
            break;
        case TKNewline:
        case TKLineComment:
            Parser_advance(parser); // fixme
            break;
        default:
            // this shouldn't be an error actually, just put it through
            // Parser_parse_expr
            error_unexpectedToken(parser, TKUnknown);
            Parser_advance(parser);
            break;
        }
        //        Parser_ignore(parser, TKLinecomment);
        //        Parser_ignore(parser, TKNl);
    }
exitloop:
    return scope;
}

Node* Parser_parseParams(Parser* parser)
{
    Parser_match(parser, TKOpLT);
    Node* node = NULL;
    Node* param = NULL;
    do {
        if (Parser_matches(parser, TKIdentifier)) {
            param = nodeFromCurrentToken(parser);
            param->kind = NKVar;
            // name is already in param->name
            if (Parser_ignore(parser, TKOpColon))
                param->typespec = Parser_parseTypeSpec(parser);
            if (Parser_ignore(parser, TKOpAssign))
                param->init = Parser_parseExpr(parser);
            list_append(&node, param);
        }
    } while (Parser_ignore(parser, TKComma));

    Parser_match(parser, TKOpGT);
    return node;
}

Node* Parser_parseFunc(Parser* parser)
{
    // shouldn't this be new_node_from...? or change Parser_parse_type &
    // others for consistency
    Node* node = Parser_match(parser, TKKeyword_function);

    if (node) {
        node->kind = NKFunc;
        node->name = Parser_parseIdent(parser);
        node->args = Parser_parseArgs(parser);
        if (Parser_ignore(parser, TKOpColon)) {
            node->typespec = Parser_parseTypeSpec(parser);
        }
        // if (!Parser_matches(parser, TKKw_end)) {
        node->body = Parser_parseScope(parser, node); //}
    }
    Parser_discard(parser, TKKeyword_end);
    Parser_discard(parser, TKKeyword_function);
    return node;
}

Node* Parser_parseTest(Parser* parser) { return NULL; }

Node* Parser_parseType(Parser* parser)
{
    Node* node = nodeFromCurrentToken(parser);
    node->kind = NKType;
    Parser_ignore(parser, TKOneSpace);
    node->name = Parser_parseIdent(parser);
    if (Parser_matches(parser, TKOpLT))
        node->params = Parser_parseParams(parser);

    Node* body
        = Parser_parseScope(parser, node); // will set super for this type
    node->members = body->stmts;
    // Node* item = NULL;
    //    for (item = body->stmts; //
    //         item != NULL; //
    //         item = item->next) {
    // can't have locals and stmts separately since next ptr is embedded.
    // so the same element cant be part of more than one list.
    // for types, we cap wipe the next as we go and append checks and vars
    // to different lists.
    // for scopes however there cant be a locals -- you have to walk the
    // stmts and extract vars in order to process locals. YOU HAVE TO DO THE
    // SAME HERE FOR TYPES! how will you get item->next to advance this loop
    // if you go on wiping it inside the loop
    //        switch (item->kind) {
    //            case NKCheck:
    //                item->next=NULL;
    //                list_append(&node->checks, item);
    //                break;
    //            case NKVar:
    //                item->next=NULL;
    //                list_append(&node->vars, item);
    //                break;
    //            default:
    //                //error
    //                break;
    //        }
    //        if (item->kind == NKCheck) {
    //            list_append(&(node->checks), item);
    //        } else {
    //            // error
    //        }
    //    }
    Parser_discard(parser, TKKeyword_end);
    /* !strict ? Parser_ignore : */
    Parser_discard(parser, TKKeyword_type);
    return node;
}

Node* Parser_parseImport(Parser* parser)
{
    Node* node = nodeFromCurrentToken(parser);
    node->kind = NKImport;
    node->name = "";
    node->impfile = Parser_parseIdent(parser);
    Parser_ignore(parser, TKOneSpace);
    if (Parser_ignore(parser, TKKeyword_as)) {
        Parser_ignore(parser, TKOneSpace);
        node->name = Parser_parseIdent(parser);
    }
    return node;
}

bool_t skipws = true;

//#define PRINTTOKENS
// parse functions all have signature: Node* Parser_parse_xxxxx(Parser_t*
// parser)
Node* Parser_parse(Parser* parser)
{
    Node* root = alloc_Node();
    root->kind = NKModule;
    root->name = parser->moduleName;
    Node* node;
    TokenKind kind;
    Parser_advance(parser);
    while ((kind = parser->token.kind) != TKNullChar) {
#ifdef PRINTTOKENS
        printf("%s %2d %3d %3d %3d %-6s\t%.*s\n", parser->basename,
            parser->token.line, parser->token.col,
            parser->token.col + parser->token.matchlen,
            parser->token.matchlen, token_repr(kind),
            kind == TKNewline ? 0 : parser->token.matchlen,
            parser->token.pos);
#endif
        switch (kind) {
        case TKKeyword_function:
            if ((node = Parser_parseFunc(parser))) list_append(&(root->funcs), node);
            break;
        case TKKeyword_type:
            if ((node = Parser_parseType(parser))) list_append(&(root->types), node);
            break;
        case TKKeyword_import:
            if ((node = Parser_parseImport(parser))) {
                list_append(&(root->imports), node);
                Parser* iparser = Parser_new(node->impfile, skipws);
                Node* imod = Parser_parse(iparser);
                list_append(&(parser->modules), imod);
            }
            break;
        case TKKeyword_test:
            if ((node = Parser_parseTest(parser))) list_append(&(root->tests), node);
            break;
        case TKKeyword_var:
        case TKKeyword_let:
            if ((node = Parser_parseVar(parser))) list_append(&(root->globals), node);
            break;
        case TKNewline:
        case TKLineComment:
            break;
#ifndef PRINTTOKENS
        default:
#endif
            printf("other token: %s at %d:%d len %d\n", Token_repr(kind),
                parser->token.line, parser->token.col, parser->token.matchlen);
            Parser_advance(parser);
        }
        // Parser_advance(parser);// this shouldnt be here, the specific
        // funcs do it
        Parser_ignore(parser, TKNewline);
        Parser_ignore(parser, TKLineComment);
    }
    list_append(&(parser->modules), root);
    return parser->modules;
}

#pragma mark Print AST
const char* spaces = "                              "
                     "                                  ";

void print_tree(const Node* const node, int level)
{
    if (!node) return;

    switch (node->kind) {
    case NKModule:
        printf("! module %s\n", node->name);
        Node* type = node->types;
        while (type) {
            print_tree(type, level);
            type = type->next;
        }
        Node* func = node->funcs;
        while (func) {
            print_tree(func, level);
            func = func->next;
        }
        break;

    case NKVar:
        printf("%.*s%s %s", level * 4, spaces,
            node->flags.var.isLet ? "let" : "var", node->name);
        if (node->typespec)
            print_tree(node->typespec, level + 1);
        else
            printf(": UNKNownType");
        if (node->init) {
            printf(" = ");
            print_tree(node->init, level + 1);
        }
        puts("");
        break;

    case NKFunc:
        printf("function %s(", node->name);
        Node* arg = node->args;
        while (arg) {
            printf("%s: %s", arg->name, arg->typespec->name);
            if ((arg = arg->next)) printf(", ");
        }
        if (node->typespec) printf("): %s\n", node->typespec->name);
        print_tree(node->body, level);
        puts("end function\n");
        break;

    case NKType:
        printf("type %s\n", node->name);
        if (node->super) printf("    base %s\n", node->super);
        Node* member = node->members;
        while (member) {
            print_tree(member, level + 1);
            member = member->next;
        }
        puts("end type\n");
        break;

    case NKScope:
        printf("!begin scope\n");
        Node* stmt = node->stmts;
        while (stmt) {
            print_tree(stmt, level + 1);
            stmt = stmt->next;
        }
        break;

        // case NKToken:
        // printf(
        //    "@@@ found unresolved token of kind %s",
        //    token_repr(node->subkind));
        // actually you should raise an error here since NKToken should
        // be resolved to NKLiteral or something while Parser_parse_expr
        // pushes it on the result stack. but for now let's work with it:
        //        switch(node->subkind) {
        //                case TKNumber:
        //                  printf("%g")
        //}
        // break;

    case NKExpr:
        switch (node->subkind) {
        case TKIdentifier:
        case TKNumber:
        case TKString:
            printf("%.*s", node->len, node->value.string);
            break;
        case TKFunctionCall:
        case TKSubscript:
            // NYI
            break;
        default:
            if (!node->prec) break; // not an operator
            if (node->left) {
                if (node->left->kind == NKExpr && node->left->prec < node->prec)
                    putc('(', stdout);
                print_tree(node->left, level + 1);
                if (node->left->kind == NKExpr && node->left->prec < node->prec)
                    putc(')', stdout);
            }
            printf(" %s ", Token_repr(node->subkind));
            if (node->right) {
                if (node->right->kind == NKExpr && node->right->prec < node->prec)
                    putc('(', stdout);
                print_tree(node->right, level + 1);
                if (node->right->kind == NKExpr && node->right->prec < node->prec)
                    putc(')', stdout);
            }
        }
        break;

    case NKUnits:
        printf("|%s", node->name);
        break;

    case NKTypeSpec:
        printf(": %s", node->name);
        if (node->dims) printf("%s", node->dims->name);
        break;

    default:
        printf("!!! can't print %s", NodeKind_repr(node->kind));
        break;
    }
}
