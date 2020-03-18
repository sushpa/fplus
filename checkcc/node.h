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
typedef struct Nodeold Nodeold;

/*
typedef enum {
    NKAdd,
    NKAddeq,
    NKAnd,
    NKArglist,
    NKCall,
    NKCheck,
    NKColeq, // :=
    NKColon, // :
    NKColcol, // ::
    NKComma, // ', *(! .*)?\n*'
    NKComment, //'(! .*)'
    NKCtor,
    NKDict,
    NKDiv,
    NKDiveq,
    NKDo,
    NKEq,
    NKEqeq,
    NKFunc,
    NKGe,
    NKGives,
    NKGt,
    NKHashtag,
    NKIn,
    NKIdent,
    NKIndex,
    NKImport,
    NKLe,
    NKList,
    NKLit,
    NKLt,
    NKMod,
    NKModeq,
    NKMul,
    NKMuleq,
    NKNeq,
    NKNot,
    NKNotgives,
    NKNotin,
    NKOr,
    NKParam,
    NKPow,
    NKPoweq,
    NKRange,
    NKSemi,
    NKExpr,
    NKSub,
    NKSubeq,
    NKScope,
    NKThen,
    NKType,
    NKTypespec,
    NKToken, // unprocessed token
    NKUnit,
    NKVar
} NodseKind; */

/*
char* node_repr(NodeKind kind)
{
    switch (kind) {
    case NKAdd:
        return "NKAdd";
    case NKAddeq:
        return "NKAddeq";
    case NKAnd:
        return "NKAnd";
    case NKArglist:
        return "NKArglist";
    case NKCall:
        return "NKCall";
    case NKCheck:
        return "NKCheck";
    case NKColeq:
        return "NKColeq";
    case NKColon:
        return "NKColon";
    case NKColcol:
        return "NKColcol";
    case NKComma:
        return "NKComma";
    case NKComment:
        return "NKComment";
    case NKCtor:
        return "NKCtor";
    case NKDict:
        return "NKDict";
    case NKDiv:
        return "NKDiv";
    case NKDiveq:
        return "NKDiveq";
    case NKDo:
        return "NKDo";
    case NKEq:
        return "NKEq";
    case NKEqeq:
        return "NKEqeq";
    case NKFunc:
        return "NKFunc";
    case NKGe:
        return "NKGe";
    case NKGives:
        return "NKGives";
    case NKGt:
        return "NKGt";
    case NKHashtag:
        return "NKHashtag";
    case NKIn:
        return "NKIn";
    case NKIdent:
        return "NKIdent";
    case NKIndex:
        return "NKIndex";
    case NKExpr:
        return "NKExpr";
    case NKParam:
        return "NKParam";
    case NKScope:
        return "NKScope";
    case NKLe:
        return "NKLe";
    case NKList:
        return "NKList";
    case NKLit:
        return "NKLit";
    case NKLt:
        return "NKLt";
    case NKMod:
        return "NKMod";
    case NKModeq:
        return "NKModeq";
    case NKMul:
        return "NKMul";
    case NKImport:
        return "NKImport";

    case NKMuleq:
        return "NKMuleq";
    case NKNeq:
        return "NKNeq";
    case NKNot:
        return "NKNot";
    case NKNotgives:
        return "NKNotgives";
    case NKNotin:
        return "NKNotin";
    case NKOr:
        return "NKOr";
    case NKPow:
        return "NKPow";
    case NKPoweq:
        return "NKPoweq";
    case NKRange:
        return "NKRange";
    case NKSemi:
        return "NKSemi";
    case NKSub:
        return "NKSub";
    case NKSubeq:
        return "NKSubeq";
    case NKThen:
        return "NKThen";
    case NKType:
        return "NKType";
    case NKTypespec:
        return "NKTypespec";
    case NKToken:
        return "NKToken";
    case NKUnit:
        return "NKUnit";
    case NKVar:
        return "NKVar";
    }
    return "node_uNKNown";
} */

typedef enum {
    NKImport,
    NKUnits,
    NKExpr,
    NKVar,
    NKType,
    NKStmt,
    NKScope,
    NKTypeSpec,
    NKFunc,
    NKModule
} NodeKind;

const char* NodeKind_repr(NodeKind kind)
{
    switch (kind) {
    case NKImport:
        return "NKImport";
    case NKUnits:
        return "NKUnits";
    case NKExpr:
        return "NKExpr";
    case NKVar:
        return "NKVar";
    case NKType:
        return "NKType";
    case NKStmt:
        return "NKStmt";
    case NKScope:
        return "NKScope";
    case NKTypeSpec:
        return "NKTypeSpec";
    case NKFunc:
        return "NKFunc";
    case NKModule:
        return "NKModule";
    }
}
typedef struct ASTNode ASTNode;

typedef struct {
    char* importFile;
} ASTImport;

typedef struct {
    uint8_t powers[7], something;
    double factor, factors[7];
} ASTUnits;

typedef struct {
    ASTNode *left, *right, *next;
//    union { // when you write the version with specific types, DO NOT put prec/rassoc/etc in a union with literals.
    // you can put the literal in a union with either left/right. put prec/rassoc&co in basics along with line/col now that
    // 8B of kind will be free (subkind becomes kind).
        struct {
            bool_t rassoc, unary;
            int8_t prec;
        } op; // for nonterminals
        union {
            char* string;
            double real;
            int64_t integer;
            uint64_t uinteger;
        } value; // for terminals
//    };
} ASTExpr; // how about if, for, etc. all impl using ASTExpr?
typedef struct ASTTypeSpec ASTTypeSpec;

typedef struct {
    ASTNode* typeSpec;
    ASTNode* init;
} ASTVar;

typedef struct {
    ASTNode* members; // vars contained in this type
    char* super; // should be TypeNode
    //ASTNode* checks; // make sure each Expr has null next. If not, split the
                     // Expr into two and add one after the other. If you
                     // will add automatic checks based on expressions
                     // elsewhere, clone them and set null next, because we
                     // need next to build this list here.
    ASTNode* params; // params of this type (if generic type / template)
} ASTType;

//typedef struct {
//    union {
//        ASTExpr* expr;
//        ASTVar* var;
//    };
//    struct ASTStmt* next; // Expr has its own next... so clean this up
//} ASTStmt;

typedef struct {
    ASTNode* stmts;
    ASTNode* locals;
    ASTNode* parent; // fixme
} ASTScope;

 struct ASTTypeSpec {
    // char* typename; // use name
    ASTNode* type;
    ASTNode* units;
    ASTNode* dims;
} ;

typedef struct {
    ASTNode* body;
    ASTNode* args;
    ASTNode* returnType;
    // char* name;
    char* mangledName;
    char* owner; // if method of a type
} ASTFunc;

typedef struct {
    ASTNode* funcs;
    ASTNode* types;
    ASTNode* globals;
    ASTNode* imports;
    ASTNode* tests;
} ASTModule;

struct ASTNode {
    struct {
        union {
        uint16_t len, nArgs;
    };
    uint16_t line;
    uint8_t col;
    NodeKind kind : 8; //:8
    TokenKind subkind : 8; //:8
    union {
        struct {
            bool_t resolved;
        } typespec;
        struct {
            bool_t //
                isLonglen : 1, //
                isUnowned : 1, //
                type : 2, //
                base : 2,
                isRealStr : 1, //
                isHugeInt : 1; // base: 0:10,1:16,2:2,3:8
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
                isLet : 1, //
                isVar : 1, //
                isArray : 1,
                hasUnit : 1, //
                hasInit : 1, //
                hasType : 1;
        } var;
        struct {
            bool_t //
                unused : 1, //
                isTarget : 1 /* x = f(x,y) */, //
                isArray : 1, //
                hasUnit : 1, //
                hasInit : 1, //
                hasType : 1;
        } farg;
    } flags;
    };
    /* --- 8B */
    char* name;
    /* --- 16B */
    ASTNode* next;
    /* --- 24B */
    union {
        ASTImport import;
       // ASTUnits units;
        ASTExpr expr;
        ASTVar var;
        ASTType type;
//        ASTStmt stmt;
        ASTScope scope;
        ASTTypeSpec typeSpec;
        ASTFunc func;
        ASTModule module;
    };
};

#define MAKE_STRUCT_DEFS                                                   \
    struct {                                                               \
        Node* typespec;                                                    \
        union {                                                            \
            struct { /*node_var_t, node_param_t */                         \
                Node* init;                                                \
            };                                                             \
            struct { /* node_function_t or node_test_t */                  \
                union {                                                    \
                    Node* args;                                            \
                    uint32_t issue;                                        \
                };                                                         \
                Node* body;                                                \
            };                                                             \
        };                                                                 \
    };                                                                     \
    /* struct { / * node_expr_t * /                                        \
        Node *left, *right;                                                \
    };               */                                                    \
    struct { /* node_import_t */                                           \
        char* impfile;                                                     \
    };                                                                     \
    /*struct { / * node_test_t  * /                                        \
        Node* testbody;                                                    \
        uint32_t issue;                                                    \
    }; */                                                                  \
                                                                           \
    struct { /* node_units_t */                                            \
        uint8_t putSomethingHere, powers[7];                               \
        double* factors; /* will be double[7] */                           \
                                                                           \
        double factor; /* final conv factor */                             \
    };                                                                     \
    struct { /* node_type_t */                                             \
        Node* members;                                                     \
        char* super;                                                       \
        Node* checks;                                                      \
        Node* params;                                                      \
    };                                                                     \
    struct { /* node_typespec_t */                                         \
        Node* type;                                                        \
        /* if resolved, this is a node_type_t, else a node_ident_t*/       \
        Node* units;                                                       \
        Node* dims;                                                        \
    };                                                                     \
    struct { /* node_module_t */                                           \
        Node* funcs;                                                       \
        Node* types;                                                       \
        Node* globals;                                                     \
        Node* imports;                                                     \
        Node* tests;                                                       \
    };                                                                     \
    /*struct { / * node_literal_t * /                                      \
        union {                                                            \
            char* string; / * kind 0 *  /                                  \
            double real; / * kind 1 * /                                    \
            int64_t integer; / * kind 2 * /                                \
            char chars[8]; / * kind 3 / ensure last byte null* /           \
            bool_t boolean;                                                \
        } / * value * /;                                                   \
    };         */                                                          \
    struct { /* node_token_t */                                            \
        uint8_t prec, rassoc : 1, unary : 1;                               \
        union {                                                            \
            char* string; /* kind 0 */                                     \
            double real; /* kind 1 */                                      \
            int64_t integer; /* kind 2 */                                  \
            char chars[8]; /* kind 3 * ensure last byte null */            \
            bool_t boolean;                                                \
        } value;                                                           \
    };                                                                     \
    /*struct / * node_scope_t * / {                                        \
        Node* stmts;                                                       \
        Node* locals;                                                      \
    };*/                                                                   \
    struct /* node_if_t , while, for */ {                                  \
        union {                                                            \
            Node* conds; /* if */                                          \
            Node* expr; /* while, for */                                   \
            Node* cases; /* match */                                       \
            Node* stmts; /* scope */                                       \
            Node* left; /* expr */                                         \
        };                                                                 \
        union {                                                            \
            Node* scopes; /* if, match */                                  \
            Node* body_; /* while, for */                                  \
            Node* locals; /* scope */                                      \
            Node* right; /* expr */                                        \
        };                                                                 \
    };                                                                     \
    /*struct / * node_for_t * / {                                          \
        Node* for_expr;                                                    \
        Node* for_body;                                                    \
    };                                                                     \
    struct / * node_while_t * / {                                          \
        Node* while_cond;                                                  \
        Node* while_body;                                                  \
    }; * /                                                                 \
    / *struct / * node_match_t * / {                                       \
        Node* cases;                                                       \
        Node* bodies;                                                      \
    };*/
// MAKE_STRUCT_DEFS
//=============================================================================
// NODE
//=============================================================================
/*
struct Nodeold {
    union {
        uint16_t len, nArgs;
    };
    uint16_t line;
    uint8_t col;
    NodeKind kind : 8; //:8
    TokenKind subkind : 8; //:8
    union {
        struct {
            bool_t resolved;
        } typespec;
        struct {
            bool_t //
                isLonglen : 1, //
                isUnowned : 1, //
                type : 2, //
                base : 2,
                isRealStr : 1, //
                isHugeInt : 1; // base: 0:10,1:16,2:2,3:8
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
                isLet : 1, //
                isVar : 1, //
                isArray : 1,
                hasUnit : 1, //
                hasInit : 1, //
                hasType : 1;
        } var;
        struct {
            bool_t //
                unused : 1, //
                isTarget : 1 / * x = f(x,y) * /, //
                isArray : 1, //
                hasUnit : 1, //
                hasInit : 1, //
                hasType : 1;
        } farg;
    } flags;
    union {
        MAKE_STRUCT_DEFS
    };
    / *node_ident_t* / char* name;
    Node* next;
}; */

static void print_sizes()
{
//    static int s = sizeof(ASTNode);
    printf("sizeof ASTNode %lu\n", sizeof(ASTNode));
//    printf("ASTImport %lu\n", sizeof(ASTImport));
//    printf("ASTUnits %lu\n", sizeof(ASTUnits));
//    printf("ASTExpr %lu\n", sizeof(ASTExpr));
//    printf("ASTVar %lu\n", sizeof(ASTVar));
//    printf("ASTType %lu\n", sizeof(ASTType));
// / /    printf("ASTStmt %lu\n", sizeof(ASTStmt));
//    printf("ASTScope %lu\n", sizeof(ASTScope));
//    printf("ASTTypeSpec %lu\n", sizeof(ASTTypeSpec));
//    printf("ASTFunc %lu\n", sizeof(ASTFunc));
//    printf("ASTModule %lu\n", sizeof(ASTModule));
}

// typedef enum lit_kind_e {
//    lit_kind_string,
//    lit_kind_real,
//    lit_kind_int,
//    lit_kind_bool,
//    lit_kind_regex
//} lit_kind_e;

//=============================================================================
// NODE STACK
//=============================================================================

typedef struct NodeStack {
    ASTNode** items;
    uint32_t count, cap;
} ASTNodeStack;

static void NodeStack_push(ASTNodeStack* self, ASTNode* node)
{
    assert(self != NULL);
    assert(node != NULL); // really?
    if (self->count < self->cap) {
        self->items[self->count++] = node;
    } else {
        self->cap = self->cap ? 2 * self->cap : 8;
        self->items = (ASTNode**)realloc(self->items,
            sizeof(ASTNode*) * self->cap); // !! realloc can NULL the ptr!
        self->items[self->count++] = node;
        int i;
        for (i = self->count; i < self->cap; i++)
            self->items[i] = NULL;
    }
}

ASTNode* NodeStack_pop(ASTNodeStack* self)
{
    ASTNode* ret = NULL;
    if (self->count) {
        ret = self->items[self->count - 1];
        self->items[self->count - 1] = NULL;
        self->count--;
    } else {
        printf("error: pop from empty list\n");
    }
    return ret; // stack->count ? stack->items[--stack->count] : NULL;
}

ASTNode* NodeStack_top(const ASTNodeStack* const self)
{
    return self->count ? self->items[self->count - 1] : NULL;
}

bool_t NodeStack_empty(const ASTNodeStack* const self)
{
    return self->count == 0;
}

// send the *pointer* by reference. if list is NULL, then 'append'ing
// something puts it at index 0
void list_append(ASTNode** list, ASTNode* item)
{
    assert(list != NULL);
    if (*list) {
        ASTNode* l = *list;
        while (l->next && l != l->next)
            l = l->next;
        l->next = item;
    } else {
        *list = item;
    }
}

// send the *pointer* by reference
void list_prepend(ASTNode** list, ASTNode* item)
{
    assert(item != NULL);
    item->next = *list;
    *list = item;
}
