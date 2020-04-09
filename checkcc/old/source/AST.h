
struct ASTTypeSpec;
struct ASTType;
struct ASTFunc;
struct ASTScope;
struct ASTExpr;
struct ASTVar;

struct ASTImport { // canbe 8
    STHEAD(ASTImport, 32)

    char* importFile;
    uint32_t aliasOffset;

    // generally this is not changed (the alias), at least not by the
    // linter. So we can only store the offset from importFile. In general
    // it would be nice to allocate the file contents buffer with a little
    // extra space and use that as a string pool. This way we can make new
    // strings while still referring to them using a uint32.
    bool isPackage = false, hasAlias = false;
    void gen(int level);
    void genc(int level);
    void undefc();
};

struct ASTUnits {

    STHEAD(ASTUnits, 32)

    uint8_t powers[7], something;
    double factor, factors[7];
    char* label;
    void gen(int level) { printf("|%s", label); }
};

#pragma mark - AST TypeSpec

struct ASTTypeSpec {

    STHEAD(ASTTypeSpec, 128)

    union {
        ASTType* type;
        char* name = NULL;
        ASTUnits* units;
    };

    union {
        struct {
            uint32_t dims : 24, col : 8;
        }; // need more than 24M? come on
        uint32_t __whack = 0;
    };
    // enum TypeSpecStatus {
    //     TSUnresolved, // name ptr is set
    //     TSResolved, // type ptr is set and points to the type
    //     TSDimensionedNumber
    // };
    // TypeSpecStatus status = TSUnresolved;
    // if false, name is set, else type is set
    union {
        struct {
            TypeTypes typeType : 8;
            CollectionTypes collectionType : 8;
        };
        uint16_t __garbage2 = 0;
    };

    uint16_t line = 0;
    //    uint8_t col;

    void gen(int level = 0);

    void genc(int level = 0, bool isconst = false);
};

struct ASTType {

    STHEAD(ASTType, 32)

    PtrList<ASTVar> vars;
    ASTTypeSpec* super = NULL;
    char* name = NULL;
    // TODO: flags: hasUserInit : whether user has defined Type() ctor

    PtrList<ASTExpr> checks; // invariants

    PtrList<ASTVar> params; // params of this type

    ASTVar* getVar(const char* name);

    void genc(int level = 0);
    void genh(int level = 0);
    void gen(int level = 0);
};

struct ASTScope {

    STHEAD(ASTScope, 32)

    PtrList<ASTExpr> stmts;
    PtrList<ASTVar> locals;
    ASTScope* parent = NULL;
    void gen(int level);
    void genc(int level);

    ASTVar* getVar(const char* name);
};

struct ASTVar {

    STHEAD(ASTVar, 128)

    ASTTypeSpec* typeSpec = NULL;
    ASTExpr* init = NULL;
    char* name = NULL;

    struct {
        bool unused : 1, //
            unset : 1, //
            isLet : 1, //
            isVar : 1, //
            isTarget : 1;
        /* x = f(x,y) */ //
    } flags;

    void gen(int level = 0);
    void genc(int level = 0, bool isconst = false);
};

struct ASTExpr {

    STHEAD(ASTExpr, 1024)
    // this should have at least a TypeTypes if not an ASTType*
    struct {
        uint16_t line = 0;
        union {
            struct {
                bool opIsUnary, opIsRightAssociative;
            };
            uint16_t strLength;
        }; // for literals, the string length
        uint8_t opPrecedence;
        uint8_t col = 0;
        TokenKind kind : 8; // TokenKind must be updated to add
                            // TKFuncCallResolved TKSubscriptResolved etc.
                            // to indicate a resolved identifier. while you
                            // are at it add TKNumberAsString etc. then
                            // instead of name you will use the corr.
                            // object. OR instead of extra enum values add a
                            // bool bit resolved
    };

    ASTExpr* left = NULL;

    union {
        union {
            char* string;
            double real;
            int64_t integer;
            uint64_t uinteger;
        } value; // for terminals
        char* name; // for unresolved functioncall or subscript, or
                    // identifiers
        ASTFunc* func = NULL; // for functioncall
        ASTVar* var; // for array subscript, or to refer to a variable, e.g.
                     // in TKVarAssign
        ASTScope* body; // for if/for/while
        ASTExpr* right;
        /* struct {uint32_t left, right}; // for 32-bit ptrs to local pool
         */
    };

    ASTExpr();
    ASTExpr(const Token* token);

    void gen(int level, bool spacing, bool escapeStrings);
    void genc(int level, bool spacing, bool inFuncArgs, bool escapeStrings);
    void catarglabels(); // helper  for genc
    Value eval();
    TypeTypes evalType();
};

struct ASTFunc {

    STHEAD(ASTFunc, 64)

    ASTScope* body = NULL;
    PtrList<ASTVar> args;
    ASTTypeSpec* returnType = NULL;
    char* mangledName = NULL;
    char* owner = NULL; // if method of a type
    char* name = NULL;
    struct {
        uint16_t line;
        struct {
            uint16_t io : 1, throws : 1, recurs : 1, net : 1, gui : 1,
                exported : 1, refl : 1, nodispatch : 1, isStmt : 1,
                isDeclare : 1;
        } flags = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        uint8_t col; // not needed
    };
    const char* getDefaultValueForType(ASTTypeSpec* type);
    void genc(int level = 0);
    void gen(int level = 0);
    void genh(int level = 0);
};

struct ASTModule {

    STHEAD(ASTModule, 16)

    PtrList<ASTFunc> funcs;
    PtrList<ASTExpr> exprs;
    PtrList<ASTType> types;
    PtrList<ASTVar> globals;
    PtrList<ASTImport> imports;
    PtrList<ASTFunc> tests;

    char* name;
    char* moduleName = NULL; // mod.submod.xyz.mycode
    char* mangledName = NULL; // mod_submod_xyz_mycode
    char* capsMangledName = NULL; // MOD_SUBMOD_XYZ_MYCODE
    void genc(int level = 0);
    bool hasImportAlias(const char* alias);
    void gen(int level = 0);
    ASTFunc* getFunc(const char* name);

    ASTType* getType(const char* name);
};
