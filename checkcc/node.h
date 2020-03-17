//=============================================================================
// NODE STRUCTS
//=============================================================================

// typedef struct node_var_t node_var_t;
// typedef struct node_function_t node_function_t;
// typedef struct node_type_t node_type_t;
// typedef struct node_typespec_t node_typespec_t;
// typedef struct node_ident_t node_ident_t;
// typedef struct node_module_t node_module_t;
// typedef struct node_expr_t node_expr_t;
// typedef struct node_scope_t node_scope_t;
// typedef struct node_if_t node_if_t;
// typedef struct node_for_t node_for_t;
// typedef struct node_while_t node_while_t;
// typedef struct node_match_t node_match_t;
// typedef struct node_literal_t node_literal_t;
// typedef struct node_token_t node_token_t;
// typedef struct node_units_t node_units_t;
// typedef struct node_test_t node_test_t;
// typedef struct node_import_t node_import_t;
typedef struct node_t node_t;

#define MAKE_STRUCT_DEFS                                                       \
    struct {                                                                   \
        node_t* typespec;                                                      \
        union {                                                                \
            struct { /*node_var_t, node_param_t */                             \
                node_t* init;                                                  \
            };                                                                 \
            struct { /* node_function_t or node_test_t */                      \
                union {                                                        \
                    node_t* args;                                              \
                    uint32_t issue;                                            \
                };                                                             \
                node_t* body;                                                  \
            };                                                                 \
        };                                                                     \
    };                                                                         \
    /* struct { / * node_expr_t * /                                            \
        node_t *left, *right;                                                  \
    };               */                                                        \
    struct { /* node_import_t */                                               \
        char* impfile;                                                         \
    };                                                                         \
    /*struct { / * node_test_t  * /                                            \
        node_t* testbody;                                                      \
        uint32_t issue;                                                        \
    }; */                                                                      \
                                                                               \
    struct { /* node_units_t */                                                \
        uint8_t putSomethingHere, powers[7];                                   \
        double* factors; /* will be double[7] */                               \
        \ 
        double factor; /* final conv factor */                                 \
    };                                                                         \
    struct { /* node_type_t */                                                 \
        node_t* members;                                                       \
        char* super;                                                           \
        node_t* checks;                                                        \
        node_t* params;                                                        \
    };                                                                         \
    struct { /* node_typespec_t */                                             \
        node_t* type;                                                          \
        /* if resolved, this is a node_type_t, else a node_ident_t*/           \
        node_t* units;                                                         \
        node_t* dims;                                                          \
    };                                                                         \
    struct { /* node_module_t */                                               \
        node_t* funcs;                                                         \
        node_t* types;                                                         \
        node_t* globals;                                                       \
        node_t* imports;                                                       \
        node_t* tests;                                                         \
    };                                                                         \
    /*struct { / * node_literal_t * /                                          \
        union {                                                                \
            char* string; / * kind 0 *  /                                      \
            double real; / * kind 1 * /                                        \
            int64_t integer; / * kind 2 * /                                    \
            char chars[8]; / * kind 3 / ensure last byte null* /               \
            bool_t boolean;                                                    \
        } / * value * /;                                                       \
    };         */                                                              \
    struct { /* node_token_t */                                                \
        uint8_t prec, rassoc : 1, unary : 1;                                   \
        union {                                                                \
            char* string; /* kind 0 */                                         \
            double real; /* kind 1 */                                          \
            int64_t integer; /* kind 2 */                                      \
            char chars[8]; /* kind 3 * ensure last byte null */                \
            bool_t boolean;                                                    \
        } value;                                                               \
    };                                                                         \
    /*struct / * node_scope_t * / {                                            \
        node_t* stmts;                                                         \
        node_t* locals;                                                        \
    };*/                                                                       \
    struct /* node_if_t , while, for */ {                                      \
        union {                                                                \
            node_t* conds; /* if */                                            \
            node_t* expr; /* while, for */                                     \
            node_t* cases; /* match */                                         \
            node_t* stmts; /* scope */                                         \
            node_t* left; /* expr */                                           \
        };                                                                     \
        union {                                                                \
            node_t* scopes; /* if, match */                                    \
            node_t* body_; /* while, for */                                    \
            node_t* locals; /* scope */                                        \
            node_t* right; /* expr */                                          \
        };                                                                     \
    };                                                                         \
    /*struct / * node_for_t * / {                                              \
        node_t* for_expr;                                                      \
        node_t* for_body;                                                      \
    };                                                                         \
    struct / * node_while_t * / {                                              \
        node_t* while_cond;                                                    \
        node_t* while_body;                                                    \
    }; * /                                                                     \
    / *struct / * node_match_t * / {                                           \
        node_t* cases;                                                         \
        node_t* bodies;                                                        \
    };*/
// MAKE_STRUCT_DEFS
//=============================================================================
// NODE
//=============================================================================

typedef enum node_kind_e {
    node_kind_add,
    node_kind_addeq,
    node_kind_and,
    node_kind_arglist,
    node_kind_call,
    node_kind_check,
    node_kind_coleq, // :=
    node_kind_colon, // :
    node_kind_colcol, // ::
    node_kind_comma, // ', *(! .*)?\n*'
    node_kind_comment, //'(! .*)'
    node_kind_ctor,
    node_kind_dict,
    node_kind_div,
    node_kind_diveq,
    node_kind_do,
    node_kind_eq,
    node_kind_eqeq,
    node_kind_func,
    node_kind_ge,
    node_kind_gives,
    node_kind_gt,
    node_kind_hashtag,
    node_kind_in,
    node_kind_ident,
    node_kind_index,
    node_kind_import,
    node_kind_le,
    node_kind_list,
    node_kind_lit,
    node_kind_lt,
    node_kind_mod,
    node_kind_modeq,
    node_kind_mul,
    node_kind_muleq,
    node_kind_neq,
    node_kind_not,
    node_kind_notgives,
    node_kind_notin,
    node_kind_or,
    node_kind_param,
    node_kind_pow,
    node_kind_poweq,
    node_kind_range,
    node_kind_semi,
    node_kind_expr,
    node_kind_sub,
    node_kind_subeq,
    node_kind_scope,
    node_kind_then,
    node_kind_type,
    node_kind_typespec,
    node_kind_token, // unprocessed token
    node_kind_unit,
    node_kind_var
} node_kind_e;

char* node_repr(node_kind_e kind)
{
    switch (kind) {
    case node_kind_add:
        return "node_kind_add";
    case node_kind_addeq:
        return "node_kind_addeq";
    case node_kind_and:
        return "node_kind_and";
    case node_kind_arglist:
        return "node_kind_arglist";
    case node_kind_call:
        return "node_kind_call";
    case node_kind_check:
        return "node_kind_check";
    case node_kind_coleq:
        return "node_kind_coleq";
    case node_kind_colon:
        return "node_kind_colon";
    case node_kind_colcol:
        return "node_kind_colcol";
    case node_kind_comma:
        return "node_kind_comma";
    case node_kind_comment:
        return "node_kind_comment";
    case node_kind_ctor:
        return "node_kind_ctor";
    case node_kind_dict:
        return "node_kind_dict";
    case node_kind_div:
        return "node_kind_div";
    case node_kind_diveq:
        return "node_kind_diveq";
    case node_kind_do:
        return "node_kind_do";
    case node_kind_eq:
        return "node_kind_eq";
    case node_kind_eqeq:
        return "node_kind_eqeq";
    case node_kind_func:
        return "node_kind_func";
    case node_kind_ge:
        return "node_kind_ge";
    case node_kind_gives:
        return "node_kind_gives";
    case node_kind_gt:
        return "node_kind_gt";
    case node_kind_hashtag:
        return "node_kind_hashtag";
    case node_kind_in:
        return "node_kind_in";
    case node_kind_ident:
        return "node_kind_ident";
    case node_kind_index:
        return "node_kind_index";
    case node_kind_le:
        return "node_kind_le";
    case node_kind_list:
        return "node_kind_list";
    case node_kind_lit:
        return "node_kind_lit";
    case node_kind_lt:
        return "node_kind_lt";
    case node_kind_mod:
        return "node_kind_mod";
    case node_kind_modeq:
        return "node_kind_modeq";
    case node_kind_mul:
        return "node_kind_mul";
    case node_kind_muleq:
        return "node_kind_muleq";
    case node_kind_neq:
        return "node_kind_neq";
    case node_kind_not:
        return "node_kind_not";
    case node_kind_notgives:
        return "node_kind_notgives";
    case node_kind_notin:
        return "node_kind_notin";
    case node_kind_or:
        return "node_kind_or";
    case node_kind_pow:
        return "node_kind_pow";
    case node_kind_poweq:
        return "node_kind_poweq";
    case node_kind_range:
        return "node_kind_range";
    case node_kind_semi:
        return "node_kind_semi";
    case node_kind_sub:
        return "node_kind_sub";
    case node_kind_subeq:
        return "node_kind_subeq";
    case node_kind_then:
        return "node_kind_then";
    case node_kind_type:
        return "node_kind_type";
    case node_kind_typespec:
        return "node_kind_typespec";
    case node_kind_token:
        return "node_kind_token";
    case node_kind_unit:
        return "node_kind_unit";
    case node_kind_var:
        return "node_kind_var";
    }
    return "node_unknown";
}

struct node_t {
    union {
        uint16_t len, nargs;
    };
    uint16_t line;
    uint8_t col;
    node_kind_e kind : 8; //:8
    token_kind_e subkind : 8; //:8
    union {
        struct {
            bool_t resolved;
        } typespec;
        struct {
            bool_t //
                islonglen : 1, //
                isunowned : 1, //
                type : 2, //
                base : 2,
                isrealstr : 1, //
                ishugeint : 1; // base: 0:10,1:16,2:2,3:8
        } literal;
        struct {
            bool_t //
                prints : 1, //
                throws : 1, //
                recurs : 1, //
                net : 1, //
                gui : 1, //
                file : 1, //
                refl : 1, //
                nodispatch : 1;
        } function;
        struct {
            bool_t unused : 1, //
                unset : 1, //
                islet : 1, //
                isvar : 1, //
                isarray : 1,
                hasunit : 1, //
                hasinit : 1, //
                hastype : 1;
        } var;
        struct {
            bool_t //
                unused : 1, //
                istarget : 1 /* x = f(x,y) */, //
                isarray : 1, //
                hasunit : 1, //
                hasinit : 1, //
                hastype : 1;
        } farg;
    } flags;
    union {
        MAKE_STRUCT_DEFS
    };
    /*node_ident_t*/ char* name;
    node_t* next;
};

static void print_sizes()
{
    static int s = sizeof(node_t);
    printf("sizeof node     %d\n", s);
}

typedef enum lit_kind_e {
    lit_kind_string,
    lit_kind_real,
    lit_kind_int,
    lit_kind_bool,
    lit_kind_regex
} lit_kind_e;

//=============================================================================
// NODE STACK
//=============================================================================

typedef struct node_stack_t {
    node_t** items;
    uint32_t count, cap;
} node_stack_t;

static void stack_push(node_stack_t* stack, node_t* node)
{
    assert(stack != NULL);
    assert(node != NULL); // really?
    if (stack->count < stack->cap) {
        stack->items[stack->count++] = node;
    } else {
        stack->cap = stack->cap ? 2 * stack->cap : 8;
        stack->items
            = realloc(stack->items, stack->cap); // !! realloc can NULL the ptr!
        stack->items[stack->count++] = node;
    }
}

node_t* stack_pop(node_stack_t* stack)
{
    return stack->count ? stack->items[--stack->count] : NULL;
}

node_t* stack_top(const node_stack_t* const stack)
{
    return stack->count ? stack->items[stack->count - 1] : NULL;
}

bool_t stack_empty(const node_stack_t* const stack)
{
    return stack->count == 0;
}

// send the *pointer* by reference. if list is NULL, then 'append'ing
// something puts it at index 0
void list_append(node_t** list, node_t* item)
{
    assert(list != NULL);
    if (*list) {
        node_t* l = *list;
        while (l->next && l != l->next)
            l = l->next;
        l->next = item;
    } else {
        *list = item;
    }
}

// send the *pointer* by reference
void list_prepend(node_t** list, node_t* item)
{
    assert(item != NULL);
    item->next = *list;
    *list = item;
}
