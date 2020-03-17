
void parser_advance(parser_t* parser) { token_advance(&parser->token); }

node_t* new_node_from_current_token(parser_t* parser)
{
    token_t* token = &parser->token;

    node_t* node = alloc_node_t();
    *node = (node_t){ //
        .kind = node_kind_token,
        .subkind = token->kind,
        .len = token->matchlen,
        .line = token->line,
        .col = token->col,
        .name = token_strdup(token),
        .next = NULL
    };
    // for some kinds, there is associated data to be saved
    switch (token->kind) {
    case token_kind_str:
    case token_kind_rgx:
    case token_kind_ident:
    case token_kind_number: // not converting for now
    case token_kind_linecomment:
        node->value.string = token_strdup(token);
        break;
    //    node->value.real = strtod(token->pos, NULL);
    //   break;
    default:
        // error_unexpectedToken(parser, token->kind);
        node->prec = token_prec(token->kind);
        node->rassoc = token_rassoc(token->kind);
        node->unary = token_unary(token->kind);
        break;
    }
    parser_advance(parser);
    return node;
}

#pragma mark Parsing Primitives

node_t* next_token_node(
    parser_t* parser, token_kind_e expected, const bool_t ignore_error)
{
    if (parser->token.kind == expected) {
        return new_node_from_current_token(parser);
    } else {
        if (!ignore_error)
            error_unexpectedToken(parser, expected);
        return NULL;
    }
}

// in the parser_match case, token should be advanced on error
node_t* parser_match(parser_t* parser, token_kind_e expected)
{
    return next_token_node(parser, expected, false);
}

// this returns the parser_match node or null
node_t* parser_trymatch(parser_t* parser, token_kind_e expected)
{
    return next_token_node(parser, expected, true);
}

// just yes or no, simple
bool_t parser_matches(parser_t* parser, token_kind_e expected)
{
    return (parser->token.kind == expected);
}

bool_t parser_ignore(parser_t* parser, token_kind_e expected)
{
    bool_t ret;
    if ((ret = parser_matches(parser, expected)))
        parser_advance(parser);
    return ret;
}

// this is same as parser_match without return
void parser_discard(parser_t* parser, token_kind_e expected)
{
    if (!parser_ignore(parser, expected))
        error_unexpectedToken(parser, expected);
}
//=============================================================================
// PARSER RULE-MATCHING FUNCTIONS
//=============================================================================
#pragma mark Parser Rules

char* parse_ident(parser_t* parser)
{
    char* p = token_strdup(&parser->token);
    parser_advance(parser);
    return p;
}

token_kind_e reverseBracket (token_kind_e kind) {
    switch(kind) {
    case token_kind_array_open:
        return token_kind_array_close;
    case token_kind_paren_open:
        return token_kind_paren_close;
    case token_kind_brace_open:
        return token_kind_brace_close;
    }
    return token_kind_unknown;
}

node_t* parse_expr(parser_t* parser)
{
    token_t* token = &parser->token;

    // we could make this static and set len to 0 upon func exit
    node_stack_t result = { NULL, 0, 0 }, ops = { NULL, 0, 0 };
    int prec_top = 0;
    node_t* p;

    while (token->kind != token_kind_nl) { // build RPN
        node_t* node = new_node_from_current_token(parser);
        int prec = node->prec; // token_prec(node->subkind);
        bool_t rassoc = node->rassoc; // token_rassoc(node->subkind);
        // build rpn
        // as it happens, node->subkind will contain the token type of the token
        // currently processed,  and token->kind holds the token type of what
        // follows (will be processed in the next iteration).
        if (node->subkind == token_kind_ident
            && token->kind == token_kind_paren_open) { // func call
            stack_push(&result, node);
        } else if (node->subkind == token_kind_ident
            && token->kind == token_kind_array_open) { // array
            stack_push(&result, node);
        } else if (node->subkind == token_kind_op_colon) { // range op
        } else if (node->subkind==token_kind_paren_open || node->subkind==token_kind_array_open || node->subkind==token_kind_brace_open) {
            stack_push(&ops, node);
            //stack_push(&result, node);
        }
        else if(node->subkind==token_kind_paren_close || node->subkind==token_kind_array_close || node->subkind==token_kind_brace_close) {
            token_kind_e revBrkt = reverseBracket(node->subkind);
            while  (!stack_empty(&ops)){
                p = stack_pop(&ops);
                if (p->subkind == revBrkt) break;
                if (p->prec) {
                    p->kind = node_kind_expr;
                    if (!p->unary)
                        p->right = stack_pop(&result);
                    p->left = stack_pop(&result);
                }
                stack_push(&result, p);
            }
            if (!stack_empty(&ops)) {
                p = stack_top(&ops);
                if (0 /*p in self.functions and token in ')>') or (p in self.arrays and token == ']' */)
                    stack_push(&result, stack_pop(&ops));
            }
            //stack_push(&result, node);
        }
        else if (prec) { // general operators
            // shunting
            // before pushing an op on the op stack, unwind the op stack as
            // long as there are tighter-binding ops

            while (!stack_empty(&ops)) //
            {
                prec_top = stack_top(&ops)->prec;
                if (prec > prec_top)
                    break;
                if (prec == prec_top && rassoc)
                    break;
                p = stack_pop(&ops);

                if (p->prec) {
                    p->kind = node_kind_expr;
                    if (!p->unary)
                        p->right = stack_pop(&result);
                    p->left = stack_pop(&result);
                }
                stack_push(&result, p);
                // if (p->subkind == node_kind_identf)
                //     stack_push(&result, ")");
                // if (p->subkind == node_kind_identa)
                //     stack_push(&result, "]");

                /* pop the op stack and stack_push the result on the rpn
                 stack BUT in our idea, we eval the rpn stack on the fly so
                 if you popped a '+' , dont stack_push it on the rpn yet
                 create an add node, pop lparam and rparam from the rpn
                 and set them as args for the add node, and stack_push the
                 add node directly this happens on each op pop! (in this
                 while loop) basically this while loop is going to be
                 repeated (without the prec cmp) when the tokens are
                 finished. */
            }
            stack_push(&ops, node);
        } else {
            //            printf("parse_expr: got '%s'\n",
            //            token_repr(node->subkind));
            stack_push(&result, node);
        }

        // parser_advance(parser);
        // } else // evaluate RPN and generate tree
        // {
        //     for (int i = 0; i < result_p; i++) {
        //         node_t* node = result[i];
        //         int nk = node->kind == node_kind_token ? node->subkind
        //                                                : node->kind;

        //         switch (nk) {
        //         case node_kind_ident:
        //         }
        //     }
    }
    while (!stack_empty(&ops)) //
    {
        p = stack_pop(&ops);
        if (p->prec) {
            p->kind = node_kind_expr;
            if (!p->unary)
                p->right = stack_pop(&result);
            p->left = stack_pop(&result);
        }
        stack_push(&result, p);
        // if (p->subkind == node_kind_identf)
        //     stack_push(&result, ")");
        // if (p->subkind == node_kind_identa)
        //     stack_push(&result, "]");
    }

    assert(result.count == 1);

    return result.items[0];
}

node_t* parse_typespec(parser_t* parser)
{ // must have ident(U), then may have "[:,:]" i.e. '[\]\[\:, ]+' , then may
  // have units. note: after ident may have params <T, S>
    parser->token.flags.mergearraydims = true;

    node_t* node = new_node_from_current_token(parser);
    node->kind = node_kind_typespec;
    // this is an ident, but we will turn it into a typespec.
    // name is in this token already
    node->dims = parser_trymatch(parser, token_kind_array_dims);
    node->units = parser_trymatch(parser, token_kind_units);
    // fixme: node->type = lookupType;

    parser->token.flags.mergearraydims = false;
    return node;
}

node_t* parse_args(parser_t* parser)
{
    parser_match(parser, token_kind_paren_open);
    node_t* node = NULL;
    node_t* arg = NULL;
    do {
        if (parser_matches(parser, token_kind_ident)) {
            arg = new_node_from_current_token(parser);
            arg->kind = node_kind_var;
            if (parser_ignore(parser, token_kind_op_colon))
                arg->typespec = parse_typespec(parser);
            if (parser_ignore(parser, token_kind_op_asn))
                arg->init = parse_expr(parser);
            list_append(&node, arg);
            node->nargs++;
        }
    } while (parser_ignore(parser, token_kind_comma));

    parser_match(parser, token_kind_paren_close);
    parser->token.flags.mergearraydims = false;
    return node;
}

node_t* parse_var(parser_t* parser)
{
    node_t* node = new_node_from_current_token(parser);
    if (node->subkind == token_kind_kw_var) {
        // var / let, read the next one. in func args you won't find this
        parser_ignore(parser, token_kind_onespc);
        node->name = parse_ident(parser);
        node->flags.var.isvar = true;
    }
    node->kind = node_kind_var;
    if (parser_ignore(parser, token_kind_op_colon)) {
#ifdef PARSE_STRICT
        parser_discard(parser, token_kind_onespc);
#endif
        node->typespec = parse_typespec(parser);
    }
#ifdef PARSE_STRICT
    parser_discard(parser, token_kind_onespc);
#endif
    if (parser_ignore(parser, token_kind_op_asn)) {
#ifdef PARSE_STRICT
        parser_discard(parser, token_kind_onespc);
#endif
        node->init = parse_expr(parser);
    }
    return node;
}

node_t* parse_scope(parser_t* parser, node_t* parent)
{
    token_kind_e kind = parser->token.kind;
    node_t* scope = alloc_node_t();
    scope->kind = node_kind_scope;
    node_t* node = NULL;
    // don't conflate this with the while in parse(): it checks against file
    // end, this checks against the keyword 'end'.
    while ((kind = parser->token.kind) != token_kind_kw_end) {
        //        kind
        switch (kind) {
        case token_kind__end:
            error_unexpectedToken(parser, token_kind_unknown);
            goto exitloop;
        case token_kind_kw_var:
            if ((node = parse_var(parser))) {
                list_append(&scope->stmts, node);
            }
            break;
        case token_kind_kw_base:
            parser_discard(parser, token_kind_kw_base);
            if (parent && parent->kind == node_kind_type)
                parent->super = parse_ident(parser);
        case token_kind_ident: // check, return
            parser_advance(parser); // fixme
            break;
        case token_kind_nl:
        case token_kind_linecomment:
            parser_advance(parser); // fixme
            break;
        default:
            // this shouldn't be an error actually, just put it through
            // parse_expr
            error_unexpectedToken(parser, token_kind_unknown);
            parser_advance(parser);
            break;
        }
        //        parser_ignore(parser, token_kind_linecomment);
        //        parser_ignore(parser, token_kind_nl);
    }
exitloop:
    return scope;
}

node_t* parse_params(parser_t* parser)
{
    parser_match(parser, token_kind_op_lt);
    node_t* node = NULL;
    node_t* param = NULL;
    do {
        if (parser_matches(parser, token_kind_ident)) {
            param = new_node_from_current_token(parser);
            param->kind = node_kind_param;
            // name is already in param->name
            if (parser_ignore(parser, token_kind_op_colon))
                param->typespec = parse_typespec(parser);
            if (parser_ignore(parser, token_kind_op_asn))
                param->init = parse_expr(parser);
            list_append(&node, param);
        }
    } while (parser_ignore(parser, token_kind_comma));

    parser_match(parser, token_kind_op_gt);
    return node;
}

node_t* parse_func(parser_t* parser)
{
    // shouldn't this be new_node_from...? or change parse_type & others for
    // consistency
    node_t* node = parser_match(parser, token_kind_kw_function);

    if (node) {
        node->kind = node_kind_func;
        node->name = parse_ident(parser);
        node->args = parse_args(parser);
        if (parser_ignore(parser, token_kind_op_colon)) {
            node->typespec = parse_typespec(parser);
        }
        // if (!parser_matches(parser, token_kind_kw_end)) {
        node->body = parse_scope(parser, node); //}
    }
    parser_discard(parser, token_kind_kw_end);
    parser_discard(parser, token_kind_kw_function);
    return node;
}

node_t* parse_test(parser_t* parser) { return NULL; }

node_t* parse_type(parser_t* parser)
{
    node_t* node = new_node_from_current_token(parser);
    node->kind = node_kind_type;
    parser_ignore(parser, token_kind_onespc);
    node->name = parse_ident(parser);
    if (parser_matches(parser, token_kind_op_lt))
        node->params = parse_params(parser);

    node_t* body = parse_scope(parser, node); // will set super for this type
    node->members = body->stmts;
    node_t* item = NULL;
    //    for (item = body->stmts; //
    //         item != NULL; //
    //         item = item->next) {
    // can't have locals and stmts separately since next ptr is embedded.
    // so the same element cant be part of more than one list.
    // for types, we cap wipe the next as we go and append checks and vars
    // to different lists.
    // for scopes however there cant be a locals -- you have to walk the stmts
    // and extract vars in order to process locals.
    // YOU HAVE TO DO THE SAME HERE FOR TYPES! how will you get item->next to
    // advance this loop if you go on wiping it inside the loop
    //        switch (item->kind) {
    //            case node_kind_check:
    //                item->next=NULL;
    //                list_append(&node->checks, item);
    //                break;
    //            case node_kind_var:
    //                item->next=NULL;
    //                list_append(&node->vars, item);
    //                break;
    //            default:
    //                //error
    //                break;
    //        }
    //        if (item->kind == node_kind_check) {
    //            list_append(&(node->checks), item);
    //        } else {
    //            // error
    //        }
    //    }
    parser_discard(parser, token_kind_kw_end);
    /* !strict ? parser_ignore : */ parser_discard(parser, token_kind_kw_type);
    return node;
}

node_t* parse_import(parser_t* parser)
{
    node_t* node = new_node_from_current_token(parser);
    node->kind = node_kind_import;
    node->name = "";
    node->impfile = parse_ident(parser);
    parser_ignore(parser, token_kind_onespc);
    if (parser_ignore(parser, token_kind_kw_as)) {
        parser_ignore(parser, token_kind_onespc);
        node->name = parse_ident(parser);
    }
    return node;
}

bool_t skipws = true;

//#define PRINTTOKENS
// parse functions all have signature: node_t* parse_xxxxx(parser_t* parser)
node_t* parse(parser_t* parser)
{
    node_t* root = alloc_node_t();
    root->kind = node_kind_mod;
    root->name = parser->moduleName;
    node_t* node;
    token_kind_e kind;

    while ((kind = parser->token.kind) != token_kind__end) {
#ifdef PRINTTOKENS
        printf("%s %2d %3d %3d %3d %-6s\t%.*s\n", parser->basename,
            parser->token.line, parser->token.col,
            parser->token.col + parser->token.matchlen, parser->token.matchlen,
            token_repr(kind),
            kind == token_kind_nl ? 0 : parser->token.matchlen,
            parser->token.pos);
#endif
        switch (kind) {
        case token_kind_kw_function:
            if ((node = parse_func(parser)))
                list_append(&(root->funcs), node);
            break;
        case token_kind_kw_type:
            if ((node = parse_type(parser)))
                list_append(&(root->types), node);
            break;
        case token_kind_kw_import:
            if ((node = parse_import(parser))) {
                list_append(&(root->imports), node);
                parser_t* iparser = parser_new(node->impfile, skipws);
                node_t* imod = parse(iparser);
                list_append(&(parser->modules), imod);
            }
            break;
        case token_kind_kw_test:
            if ((node = parse_test(parser)))
                list_append(&(root->tests), node);
            break;
        case token_kind_kw_var:
            if ((node = parse_var(parser)))
                list_append(&(root->globals), node);
            break;
        case token_kind_nl:
            break;
        case token_kind_linecomment:
            break;
#ifndef PRINTTOKENS
        default:
#endif
            printf("other token: %s at %d:%d len %d\n", token_repr(kind),
                parser->token.line, parser->token.col, parser->token.matchlen);
            parser_advance(parser);
        }
        // parser_advance(parser);// this shouldnt be here, the specific
        // funcs do it
        parser_ignore(parser, token_kind_nl);
        parser_ignore(parser, token_kind_linecomment);
    }
    list_append(&(parser->modules), root);
    return parser->modules;
}

#pragma mark Print AST
const char* spaces
    = "                                                                ";
void print_tree(const node_t* const node, int level)
{
    if (!node)
        return;

    switch (node->kind) {
    case node_kind_mod:
        printf("! module %s\n", node->name);
        node_t* type = node->types;
        while (type) {
            print_tree(type, level);
            type = type->next;
        }
        node_t* func = node->funcs;
        while (func) {
            print_tree(func, level);
            func = func->next;
        }
        break;

    case node_kind_var:
        printf("%.*s%s %s", level * 4, spaces,
            node->flags.var.islet ? "let" : "var", node->name);
        if (node->typespec)
            print_tree(node->typespec, level + 1);
        else
            printf(": UnknownType");
        if (node->init) {
            printf(" = ");
            print_tree(node->init, level + 1);
        }
        puts("");
        break;

    case node_kind_func:
        printf("function %s(", node->name);
        node_t* arg = node->args;
        while (arg) {
            printf("%s: %s", arg->name, arg->typespec->name);
            if ((arg = arg->next))
                printf(", ");
        }
        if (node->typespec)
            printf("): %s\n", node->typespec->name);
        print_tree(node->body, level);
        puts("end function\n");
        break;

    case node_kind_type:
        printf("type %s\n", node->name);
        if (node->super)
            printf("    base %s\n", node->super);
        node_t* member = node->members;
        while (member) {
            print_tree(member, level + 1);
            member = member->next;
        }
        puts("end type\n");
        break;

    case node_kind_scope:
        printf("!begin scope\n");
        node_t* stmt = node->stmts;
        while (stmt) {
            print_tree(stmt, level + 1);
            stmt = stmt->next;
        }
        break;

    case node_kind_token:
        // printf(
        //    "@@@ found unresolved token of kind %s",
        //    token_repr(node->subkind));
        // actually you should raise an error here since node_kind_token should
        // be resolved to node_kind_literal or something while parse_expr pushes
        // it on the result stack. but for now let's work with it:
        //        switch(node->subkind) {
        //                case token_kind_number:
        //                  printf("%g")
        printf("%.*s", node->len, node->value.string);
        //}
        break;

    case node_kind_expr:
        if (node->left)
            print_tree(node->left, level + 1);
        printf(" %s ", token_repr(node->subkind));
        if (node->right)
            print_tree(node->right, level + 1);
        break;

    case node_kind_unit:

    case node_kind_typespec:
        printf(": %s", node->name);
        if (node->dims)
            printf("%s", node->dims->name);
        break;

    case node_kind_index:

    case node_kind_lit:
        switch (node->flags.literal.type) {
        case lit_kind_string:
        case lit_kind_real:
        case lit_kind_int:
        case lit_kind_bool:
        case lit_kind_regex:
        default:
            break;
        }
    default:
        printf("!!! can't print node of kind %s", node_repr(node->kind));
        break;
    }
}
