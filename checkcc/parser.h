
void parser_advance(parser_t* parser) { token_advance(&parser->token); }

node_t* new_node_from_current_token(parser_t* parser)
{
    token_t* token = &parser->token;

    node_t* node = alloc_node_t();
    *node = (node_t) { //
        .kind = node_kind_token,
        .subkind = token->kind,
        .len = token->matchlen,
        .line = token->line,
        .col = token->col,
        .name = token_strdup(token)
    };
    // for some kinds, there is associated data to be saved
    switch (token->kind) {
        case token_kind_str:
        case token_kind_rgx:
        case token_kind_ident:
        case token_kind_linecomment:
            node->value.string = token_strdup(token);
            break;
        case token_kind_number:
            node->value.real = strtod(token->pos, NULL);
            break;
        default:
            //error_unexpectedToken(parser, token->kind);
            break;
    }
    parser_advance(parser);
    return node;
}

#pragma mark Parsing Primitives

node_t* next_token_node(parser_t* parser, token_kind_e expected, const bool_t ignore_error)
{
    if (parser->token.kind == expected) {
        return new_node_from_current_token(parser);
    } else {
        if (!ignore_error) error_unexpectedToken(parser, expected);
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
    if ((ret = parser_matches(parser, expected))) parser_advance(parser);
    return ret;
}

// this is same as parser_match without return
void parser_discard(parser_t* parser, token_kind_e expected)
{
    if (!parser_ignore(parser, expected)) error_unexpectedToken(parser, expected);
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


node_t* parse_expr(parser_t* parser)
{
    token_t* token = &parser->token;

    node_stack_t result = { NULL, 0, 0 }, ops = { NULL, 0, 0 };

    while (token->kind != token_kind_nl) { // build RPN
        node_t* node = new_node_from_current_token(parser);
        int prec = node->prec;
        bool_t rassoc = node->rassoc;
        // build rpn
        if (token->kind == token_kind_ident
            && peekcharafter(token) == '(') { // func call
        } else if (token->kind == token_kind_ident
                   && peekcharafter(token) == '[') { // array
        } else if (token->kind == token_kind_op_colon) { // range op
        } else if (prec > 0) {
            // shunting
            // before pushing an op on the op stack, unwind the op stack as
            // long as there are tighter-binding ops
            int prec_top = stack_top(&ops)->prec;
            while (!stack_empty(&ops)
                   && (prec_top < prec || (rassoc && (prec_top == prec)))) //
            {
                /* pop the op stack and stack_push the result on the rpn stack
                 BUT in our idea, we eval the rpn stack on the fly
                 so if you popped a '+' , dont stack_push it on the rpn yet
                 create an add node, pop lparam and rparam from the rpn
                 and set them as args for the add node, and stack_push the add
                 node directly this happens on each op pop! (in this
                 while loop) basically this while loop is going to be
                 repeated (without the prec cmp) when the tokens are
                 finished. */
            }
            stack_push(&ops, node);
        }
        stack_push(&result, node);

        parser_advance(parser);
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
    return result.items[0];
}

node_t* parse_typespec(parser_t* parser)
{ // must have ident(U), then may have "[:,:]" i.e. '[\]\[\:, ]+' , then may
  // have units
    node_t* node = new_node_from_current_token(parser);
    // this is an ident, but we will turn it into a typespec.
    // name is in this token already
    node->kind = node_kind_typespec;
    //node->name = node->value.string;
    node->dims = parser_trymatch(parser, token_kind_array_dims);
    node->units = parser_trymatch(parser, token_kind_units);
    // fixme: node->type = lookupType;
    switch(parser->token.kind) {
        case token_kind_op_asn:
        case token_kind_linecomment:
        case token_kind_nl:
            break;
        default:
            error_unexpectedToken(parser, token_kind_nl);
    }

    return node;
}
node_t* parse_args(parser_t* parser)
{
    parser->token.flags.mergearraydims = true;
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
    if (parser_ignore(parser, token_kind_op_colon))
        parser_discard(parser, token_kind_onespc);
        node->typespec = parse_typespec(parser);
    if (parser_ignore(parser, token_kind_op_asn))
        node->init = parse_expr(parser);
    return node;
}

node_t* parse_scope(parser_t* parser)
{
    token_kind_e kind;
    node_t* scope = alloc_node_t();
    scope->kind = node_kind_scope;
    node_t* node = NULL;
    // don't conflate this with the while in parse(): it checks against file end,
    // this checks against the keyword 'end'.
    while ((kind = parser->token.kind) != token_kind_kw_end) {
        parser_ignore(parser, token_kind_linecomment);
        parser_ignore(parser, token_kind_nl);
        switch (kind) {
            case token_kind_kw_var:
                if ((node = parse_var(parser))) list_append(&scope->locals, node);
                break;

            default:
                error_unexpectedToken(parser, kind);
                break;
        }
    }
    return NULL;
}

node_t* parse_params(parser_t* parser) {
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
    node_t* node = parser_match(parser, token_kind_kw_function);

    if (node) {
        node->kind = node_kind_func;
        node->name = parse_ident(parser);
        node->args = parse_args(parser);
        if (parser_ignore(parser, token_kind_op_colon)) {
            node->typespec = parse_typespec(parser);
        }
        if (!parser_matches(parser, token_kind_kw_end)) {
            node->body = parse_scope(parser);
        }
    }
    parser_match(parser, token_kind_kw_end);
    parser_match(parser, token_kind_kw_function);
    return node;
}

node_t* parse_test(parser_t* parser) { return NULL; }

node_t* parse_type(parser_t* parser)
{
    node_t* node = new_node_from_current_token(parser);
    node->kind = node_kind_type;
    parser_ignore(parser, token_kind_onespc);
    node->name = parse_ident(parser);
    if (parser_matches(parser, token_kind_op_lt)) node->params = parse_params(parser);

    node_t* body = parse_scope(parser);
    node->vars = body->locals;
    node_t* item = NULL;
    for ( item = body->stmts; //
         item != NULL; //
         item = item->next) {
        if (item->kind == node_kind_check) {
            list_append(&(node->checks), item);
        } else {
            // error
        }
    }
    parser_match(parser, token_kind_kw_end);
    /* !strict ? parser_ignore : */ parser_match(parser, token_kind_kw_type);
    return node;
}

node_t* parse_import(parser_t* parser) {
    node_t* node = new_node_from_current_token(parser);
    node->kind = node_kind_import;
    node->name="";
    return NULL;
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
               parser->token.col + parser->token.matchlen,
               parser->token.matchlen, token_repr(kind),
               kind==token_kind_nl ? 0 : parser->token.matchlen,
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
                printf("other token: %s at %d:%d len %d\n", token_repr(kind), parser->token.line, parser->token.col, parser->token.matchlen);
        }
        parser_advance(parser);
    }
    list_append(&(parser->modules), root);
    return parser->modules;
}

#pragma mark Print AST

void print_tree(const node_t* const node, int level)
{
    // printf("%s\n", "Tree:");
    // printf("%s\n", node_repr(node->kind));
    switch (node->kind) {
        case node_kind_mod:
            printf("! module %s\n", node->name);
            node_t* func = node->funcs;
            while (func) {
                print_tree(func, level + 1);
                func = func->next;
            }
            break;
        case node_kind_var:
        case node_kind_func:
            printf("function %s(", node->name);
            node_t* arg = node->args;
            while (arg) {
                printf("%s: %s", arg->name, arg->typespec->name);
                if ((arg = arg->next)) printf(", ");
            }
            printf("): %s\n", node->typespec->name);
            break;
        case node_kind_type:
        case node_kind_unit:
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
            break;
    }
}
