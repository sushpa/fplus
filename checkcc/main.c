// TODO: need ASTEnum
//
// AS A RULE
// Nothing except Parser should allocate any memory.
// if you need new strings, Parser should have anticipated that in advance
// at its init and strdup'd them ready for use (e.g. variations of module
// names). If you use a single pool for everything, free parsers manually
// when done. OR have one pool for Parser and one for everything else. This
// way Parser will have its destructors auto called and since it is the only
// one that needs destructors to be called, this will get rid of the memory
// leaks. as a bonus, all AST objects can store a 2-byte index of the
// associated parser in the Parser pool, and store offsets to strings
// instead of char*. if bringing down ASTExpr to 16KB can help, consider
// having a separate pool only for ASTExpr, then left/right ptrs are 32-bit
// indexes into this pool.
//
//----------
// LINTER should work only on TOKENS! No need to build a tree for that.
// Right now however the linter generates the AST and dumps it back. This is
// more to understand and debug the AST generation -- the production linter
// should not do AST gen.
// NOTE that you need to generate AST for linting to do things like:
// - auto annotate var types when not obvious and remove when obvious
// - auto add function argument names at call sites / remove the first
// - change |kg/s[:,:] or [:,:]|kg/s into Number[:,:]|kg/s
// - sort imports
// - remove extra parentheses in exprs
// - fix array ranges :: -> :, 1:-1:1 -> :, ::::7::::9:: -> error
// - move types and their member functions together
// since files are expected to be small this ast-based linter should be OK?
// keep modules limited to 2000 lines, or even 1000
// BUT if errors are found, no formatting can be done. in this case, after
// errors are reported, run the this->token-based linter (formatter).
// ----
// For C gen, new_Object() should be the only way to get a new instance.
// (new means initialized instance in the sense of objc, not op new in the
// sense of cpp which calls ctor later). Object_alloc() should not be called
// by anyone except new_Object(). Best is to not have Object_alloc() and
// Object_init() separately at all but directly in the new_Object() methods
// (which may be overloaded). The reason is performance: if parameters to
// new_Object are not as expected or something goes wrong e.g. a filename
// passed in doesn't exist then new can return NULL without doing any
// allocations etc. Separating alloc and init means allowing the user to do
// a wasteful alloc only for the init to fail later.

// --
// String library will mostly work on plain char* and ask for length
// where needed. The caller should compute length once (unless it was passed
// in already) and pass it into the string function. Functions that don't
// need the length will not have it as a parameter (all funcs have to check
// \0 anyway). this doesn't make buffer overflows any more likely: the
// length is generally computed by strlen which itself is problematic.
// Therefore whenever the length can be computed by other means (e.g. diff
// of two ptrs etc.) prefer that.

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>

#include "cycle.h"

#define STEP 4

#include "chstd.h"

#pragma mark - Heap Allocation Extras

// Generally this is not to be done at runtime. But for now we can use it as
// a lightweight and builtin alternative to valgrind. Later focus on getting
// codegen right so that no leaks are possible.

// Use the ch- functions, if you use plain malloc nothing will be tracked.
/*
 #ifdef CHMALLOC
 #define chfree(ptr) f_chfree(ptr, __FILE__, __LINE__, __func__)
 #define chmalloc(size) f_chmalloc(size, __FILE__, __LINE__, __func__)
 #define chcalloc(size, count) f_chcalloc(size,count, __FILE__, __LINE__,
 __func__) #define chrealloc(ptr, size) f_chrealloc(ptr, size, __FILE__,
 __LINE__, __func__) #else #define chfree(ptr) free(ptr) #define
 chmalloc(size) malloc(size) #define chcalloc(size, count)
 calloc(size,count) #define chrealloc(ptr, size) realloc(ptr, size) #endif

 void f_chfree(void* ptr, const char* file, int line, const char* func)
 {
 if (not ad_size.has(ptr)) {
 eprintf("freeing unknown ptr in '%s' at %s line %d\n", func,
 file, line); return;
 }
 aD_file.del(ptr);
 ad_func.del(ptr);
 ad_line.del(ptr);
 ad_size.del(ptr);
 free(ptr);
 }

 void* f_chmalloc(size_t size, const char* file, int line, const char* func
 )
 {
 void* ret = malloc(size);
 if (not ret) {
 eprintf("malloc failed in '%s' at %s line %d\n", func, file,
 line); return NULL;
 }
 aD_file[ptr] = fname;
 ad_func[ptr] = func;
 ad_line[ptr] = line;
 ad_size[ptr] = size;
 }
 */

// delimiter

/*
 #pragma mark - Number Types

 template <class T>
 class Dual {
 T x, dx;
 };
 template <class T>
 class Complex {
 T re, im;
 };
 template <class T>
 class Reciprocal {
 // this can be 4B/8B, all others r pairs will be 8B/16B
 T d;
 };
 template <class T>
 class Rational {
 T n, d;
 };
 template <class T>
 class Interval {
 T hi, lo;
 };
 template <class T>
 class Point {
 T x, y;
 };
 template <class T>
 class Size {
 T w, h;
 };
 union Number {
 float f;
 uint32_t u;
 int32_t d;
 Reciprocal<float> rf;
 Reciprocal<uint32_t> ru;
 Reciprocal<int32_t> rd;
 };
 static_assert(sizeof(Number) == 4, "");

 union NumberL {
 double F;
 uint64_t U;
 int64_t D;
 Reciprocal<double> rF;
 Reciprocal<uint64_t> rU;
 Reciprocal<int64_t> rD;
 };
 static_assert(sizeof(NumberL) == 8, "");
 */

// TODO: ASTTypeSpecs will have a TypeTypes typeType; that can be used
// to determine quickly if it is a primitive. fits in 4bits btw
typedef enum TypeTypes {
    TYUnresolved = 0, // unknown type
                      // nonprimitives: means they should have their own
                      // methods to print,
                      // serialise, identify, reflect, etc.
    TYObject, // resolved to an ASTType
              // primitives that can be printed or represented with no fuss
    TYErrorType, // use this to poison an expr which has a type error
                 // and the error has been reported, to avoid
                 // reporting the same error for all parents, as the
                 // error-checking routine unwinds.
    TYSize, // this is actually uintptr_t, since actual ptrs are
    // TYObjects. maybe rename it
    TYString, // need to distinguish String and char*?
    TYBool,
    // above this, ie. > 4 or >= TYInt8, all may have units |kg.m/s etc.
    TYInt8,
    TYUInt8, //
    TYInt16,
    TYUInt16,
    TYInt32,
    TYUInt32,
    TYInt64, // for loop indices start out as size_t by default
    TYUInt64,
    //  TYReal16,
    TYReal32,
    TYReal64, // Numbers start out with Real64 by default
              // conplex, duals, intervals,etc.??
} TypeTypes;

bool TypeType_isnum(TypeTypes tyty) { return tyty >= TYInt8; }
/*
const char* TypeType_nativeName(TypeTypes tyty)
{
    switch (tyty) {
    case TYUnresolved:
        return NULL;
    case TYObject:
        return "";
    case TYSize: // this is actually uintptr_t, since actual ptrs are
                 // TYObjects. maybe rename it
        return "size_t";
    case TYString:
        return "char*";
    case TYBool:
        return "bool_t";
        // above this, ie. > 4 or >= TYInt8, all may have units |kg.m/s etc.
    case TYInt8:
        return "int8_t";
    case TYUInt8:
        return "uint8_t";
    case TYInt16:
        return "int16_t";
    case TYUInt16:
        return "uint16_t";
    case TYInt32:
        return "int32_t";
    case TYUInt32:
        return "uint32_t";
    case TYInt64: // for loop indices start out as size_t by default
        return "int64_t";
    case TYUInt64:
        return "uint64_t";
    case TYReal16:
        return "float_half";
    case TYReal32:
        return "float";
    case TYReal64: // Numbers start out with Real64 by default
        return "double";
    }
}
*/
const char* TypeType_name(TypeTypes tyty)
{
    switch (tyty) {
    case TYUnresolved:
        return NULL;
    case TYErrorType:
        return "<invalid>";
    case TYString:
        return "String";
    case TYBool:
        return "Logical";
    case TYObject:
        return "";
    case TYSize:
    case TYInt8:
    case TYUInt8:
    case TYInt16:
    case TYUInt16:
    case TYInt32:
    case TYUInt32:
    case TYInt64:
    case TYUInt64:
    // case TYReal16:
    case TYReal32:
    case TYReal64:
        return "Scalar";
    }
}

// these are DEFAULTS
const char* TypeType_format(TypeTypes tyty)
{
    switch (tyty) {
    case TYUnresolved:
        return NULL;
    case TYErrorType:
        return NULL;
    case TYObject:
        return "%p";
    case TYSize: // this is actually uintptr_t, since actual ptrs are
                 // TYObjects. maybe rename it
        return "%lu";
    case TYString:
        return "%s";
    case TYBool:
        return "%d";
        // above this, ie. > 4 or >= TYInt8, all may have units |kg.m/s etc.
        // so you can test tyty >= TYInt8 to see if *units must be
        // processed.
    case TYInt8:
        return "%d";
    case TYUInt8:
        return "%u";
    case TYInt16:
        return "%d";
    case TYUInt16:
        return "%u";
    case TYInt32:
        return "%d";
    case TYUInt32:
        return "%u";
    case TYInt64: // for loop indices start out as size_t by default
        return "%d";
    case TYUInt64:
        return "%u";
    // case TYReal16:
    // return "%g";
    case TYReal32:
        return "%g";
    case TYReal64: // Numbers start out with Real64 by default
        return "%g";
    }
}

// needed to compute stack usage
unsigned int TypeType_size(TypeTypes tyty)
{
    switch (tyty) {
    case TYUnresolved:
    case TYErrorType:
        return 0;
    case TYObject:
        return sizeof(void*);
    case TYSize:
        return sizeof(size_t);
    case TYString:
        return sizeof(char*); // what about length
    case TYBool:
        return sizeof(int);
    case TYInt8:
        return 1;
    case TYUInt8:
        return 1;
    case TYInt16:
        return 2;
    case TYUInt16:
        return 2;
    case TYInt32:
        return 4;
    case TYUInt32:
        return 4;
    case TYInt64:
        return 8;
    case TYUInt64:
        return 8;
    // case TYReal16:
    // return 2;
    case TYReal32:
        return 4;
    case TYReal64:
        return 8;
    }
}

// If we get entirely rid of type annotation, the process for determining
// type as it is now will be very convoluted. First you pass strings around
// and compare them ("String", "Scalar", etc.) and then set TypeTypes
// according to that why not set the right TypeTypes directly in analysis?
// but this is needed as long as there is ANY type annotation somewhere.
// (e.g. func args)
TypeTypes TypeType_byName(const char* spec)
{
    // does NOT use strncmp!
    if (not spec) return TYUnresolved;
    if (not strcmp(spec, "Scalar"))
        return TYReal64; // this is default, analysis might change it to
                         // more specific
    if (not strcmp(spec, "String")) return TYString;
    if (not strcmp(spec, "Logical")) return TYBool;
    // note that Vector, Matrix, etc. are actually types, so they should
    // resolve to TYObject.
    return TYUnresolved;
}

// says what exactly a collection should generate to, since in check
// source code, collections' actual kind is abstracted away. 4 bits.
typedef enum CollectionTypes {
    CTYNone = 0, // scalar value
    CTYArray,
    CTYList,
    CTYDList,
    CTYDictS,
    CTYDictU,
    CTYOrderedDictS, // String keys
    CTYSortedDictS, // UInt/Int/Ptr keys
    CTYOrderedDictU,
    CTYSortedDictU,
    CTYSet,
    CTYOrderedSet,
    CTYSortedSet,
    CTYTensor, // will need to store dims. includes vector/matrix/tensor
    CTYDataFrame,
    CTYStackArray, // computed size from init. can get later using countof()
    CTYStackArray8, // these are NOT in BYTES, but sizeof(whatever), so
                    // careful with double/int arrays
    CTYStackArray16,
    CTYStackArray32,
    CTYStackArray64,
    CTYStackArray128,
    CTYStackArray256,
    CTYStackArray512,
    CTYStackArray1024,
    CTYStackArray2048,
    CTYStackArray4096, // really? you need any larger?
} CollectionTypes;

const char* CollectionType_nativeName(CollectionTypes coty)
{
    switch (coty) {
    case CTYNone:
        return NULL;
    case CTYArray:
        return "_A";
    case CTYList:
        return "_l";
    case CTYDList:
        return "_L";
    case CTYDictS:
        return "_d";
    case CTYDictU:
        return "_D";
    case CTYOrderedDictS:
        return "_o";
    case CTYSortedDictS:
        return "_s";
    case CTYOrderedDictU:
        return "_O";
    case CTYSortedDictU:
        return "_S";
    case CTYSet:
        return "_Z";
    case CTYOrderedSet:
        return "_Y";
    case CTYSortedSet:
        return "_X";
    case CTYTensor:
        return "_T";
    case CTYDataFrame:
        return "_F";
    case CTYStackArray:
        return "[]";
    case CTYStackArray8:
        return "[8]";
    case CTYStackArray16:
        return "[16]";
    case CTYStackArray32:
        return "[32]";
    case CTYStackArray64:
        return "[64]";
    case CTYStackArray128:
        return "[128]";
    case CTYStackArray256:
        return "[256]";
    case CTYStackArray512:
        return "[512]";
    case CTYStackArray1024:
        return "[1024]";
    case CTYStackArray2048:
        return "[2048]";
    case CTYStackArray4096:
        return "[4096]";
    }
}

#define allocstat(T)                                                       \
    if (T##_allocTotal)                                                    \
        eprintf("*** %-24s %4ld B x %5d = %7ld B\n", #T, sizeof(T),        \
            T##_allocTotal, T##_allocTotal * sizeof(T));

// these should be generated by the compiler in DEBUG mode
MKSTAT(ASTExpr)
MKSTAT(ASTFunc)
MKSTAT(ASTTypeSpec)
MKSTAT(ASTType)
MKSTAT(ASTModule)
MKSTAT(ASTScope)
MKSTAT(ASTImport)
MKSTAT(ASTVar)
MKSTAT(Parser)
MKSTAT(List_ASTExpr)
MKSTAT(List_ASTFunc)
// MKSTAT(List_ASTTypeSpec)
MKSTAT(List_ASTType)
MKSTAT(List_ASTModule)
MKSTAT(List_ASTScope)
MKSTAT(List_ASTImport)
MKSTAT(List_ASTVar)

#define JOIN(x, y) x##y

#define NAME_CLASS(T) const char* JOIN(T, _typeName) = #T;

#include "tokenKind.h"

#include "token.h"

const char* const spaces = //
    "                                                                     ";

#pragma mark - AST TYPE DEFINITIONS

typedef struct ASTImport {
    char* importFile;
    uint32_t aliasOffset;
    bool isPackage, hasAlias;
} ASTImport;

typedef struct ASTUnits {
    uint8_t powers[7], something;
    double factor, factors[7];
    char* label;
} ASTUnits;

typedef struct ASTTypeSpec {
    union {
        struct ASTType* type;
        char* name;
        ASTUnits* units;
    };
    uint32_t dims : 24, col : 8;
    uint16_t line;
    TypeTypes typeType : 8;
    CollectionTypes collectionType : 8;
} ASTTypeSpec;

typedef struct ASTVar {
    ASTTypeSpec* typeSpec;
    struct ASTExpr* init;
    char* name;
    uint16_t line;
    uint8_t col;
    struct {
        bool unused : 1, //
            unchanged : 1, //
            isLet : 1, //
            isVar : 1, //
            isTarget : 1, //
            escapesFunc : 1; // for func args, does it escape the func?
                             // (returned etc.)
        /* x = f(x,y) */ //
    } flags;
} ASTVar;

typedef struct ASTExpr {
    struct {
        uint16_t line;
        union {
            struct {
                uint16_t typeType : 7, opIsUnary : 1, collectionType : 7,
                    opIsRightAssociative : 1;
            };
            uint16_t strLen;
        }; // for literals, the string length
        uint8_t opPrec;
        uint8_t col;
        TokenKind kind : 8;
    };
    // struct ASTTypeSpec* typeSpec;
    struct ASTExpr* left;
    union {
        union {
            char* string;
            double real;
            int64_t integer;
            uint64_t uinteger;
        } value; // for terminals
        char* name; // for idents or unresolved call or subscript
        struct ASTFunc* func; // for functioncall
        ASTVar* var; // for array subscript, or a TKVarAssign
        struct ASTScope* body; // for if/for/while
        struct ASTExpr* right;
    };
    // canThrow:1, mayNeedPromotion:1
    // TODO: the code motion routine should skip over exprs with
    // mayNeedPromotion=false this is set for exprs with func calls or array
    // filtering etc...
} ASTExpr;

typedef struct ASTScope {
    List(ASTExpr) * stmts;
    List(ASTVar) * locals;
    struct ASTScope* parent;
    // uint16_t tmpCount;
    // still space left
} ASTScope;

typedef struct ASTType {
    ASTTypeSpec* super;
    char* name;
    // TODO: flags: hasUserInit : whether user has defined Type() ctor
    ASTScope* body;
} ASTType;

typedef struct ASTFunc {
    ASTScope* body;
    List(ASTVar) * args;
    ASTTypeSpec* returnType;
    char* name;
    char* selector;
    struct {
        uint16_t line;
        struct {
            uint16_t usesIO : 1, nothrow : 1, isRecursive : 1, usesNet : 1,
                usesGUI : 1, usesSerialisation : 1, isExported : 1,
                usesReflection : 1, nodispatch : 1, isStmt : 1,
                isDeclare : 1, isCalledFromWithinLoop : 1;
        } flags; //= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        uint8_t argCount;
    };
} ASTFunc;

typedef struct ASTModule {
    List(ASTFunc) * funcs;
    List(ASTExpr) * exprs;
    List(ASTType) * types;
    List(ASTVar) * globals;
    List(ASTImport) * imports;
    List(ASTFunc) * tests;
    char* name;
    char* moduleName; // mod.submod.xyz.mycode
    // char* mangledName; // mod_submod_xyz_mycode
    // char* capsMangledName; // MOD_SUBMOD_XYZ_MYCODE
} ASTModule;

#pragma mark - AST IMPORT IMPL.

void ASTImport_gen(ASTImport* this, int level)
{
    printf("import %s%s%s%s\n", this->isPackage ? "@" : "",
        this->importFile, this->hasAlias ? " as " : "",
        this->hasAlias ? this->importFile + this->aliasOffset : "");
}

#pragma mark - AST UNITS IMPL.

struct ASTTypeSpec;
struct ASTType;
struct ASTFunc;
struct ASTScope;
struct ASTExpr;
struct ASTVar;

#pragma mark - AST TYPESPEC IMPL.
ASTTypeSpec* ASTTypeSpec_new(TypeTypes tt, CollectionTypes ct)
{
    ASTTypeSpec* ret = NEW(ASTTypeSpec);
    ret->typeType = tt;
    ret->collectionType = ct;
    return ret;
}

void ASTTypeSpec_gen(ASTTypeSpec* this, int level)
{
    switch (this->typeType) {
    case TYObject:
        printf("%s", this->type->name);
        break;
    case TYUnresolved:
        printf("%s", this->name);
        break;
    default:
        printf("%s", TypeType_name(this->typeType));
        break;
    }
    if (this->dims) printf("%s", "[]");
}
const char* ASTTypeSpec_name(ASTTypeSpec* this)
{
    switch (this->typeType) {
    case TYUnresolved:
        return this->name;
    case TYObject:
        return this->type->name;
    default:
        return TypeType_name(this->typeType);
    }
    // what about collectiontype???
}
const char* getDefaultValueForType(ASTTypeSpec* type)
{
    if (not type) return ""; // for no type e.g. void
    if (not strcmp(type->name, "String")) return "\"\"";
    if (not strcmp(type->name, "Scalar")) return "0";
    if (not strcmp(type->name, "Int")) return "0";
    if (not strcmp(type->name, "Int32")) return "0";
    if (not strcmp(type->name, "UInt32")) return "0";
    if (not strcmp(type->name, "UInt")) return "0";
    if (not strcmp(type->name, "UInt64")) return "0";
    if (not strcmp(type->name, "Int64")) return "0";
    if (not strcmp(type->name, "Real32")) return "0";
    if (not strcmp(type->name, "Real64")) return "0";
    if (not strcmp(type->name, "Real")) return "0";
    if (not strcmp(type->name, "Logical")) return "false";
    return "NULL";
}

#pragma mark - AST EXPR IMPL.
static uint32_t exprsAllocHistogram[128];

void expralloc_stat()
{
    int iexpr, sum = 0, thres = 0;
    for (int i = 0; i < 128; i++)
        sum += exprsAllocHistogram[i];
    thres = sum / 20;
    // eputs("-------------------------------------------------------");

    eputs("\e[1mASTExpr allocation by kind \e[0m(those above 5% of "
          "total)\n  Kind    "
          "    # "
          "     %%\n");
    for (int i = 0; i < 128; i++) {
        if ((iexpr = exprsAllocHistogram[i]) > thres)
            eprintf("  %-8s %4d %7.2f\n",
                TokenKind_repr((TokenKind)i, false), iexpr,
                iexpr * 100.0 / sum);
    }
    eputs("-------------------------------------------------------\n");
}

ASTExpr* ASTExpr_fromToken(const Token* this)
{
    ASTExpr* ret = NEW(ASTExpr);
    ret->kind = this->kind;
    ret->line = this->line;
    ret->col = this->col;

    ret->opPrec = TokenKind_getPrecedence(ret->kind);
    if (ret->opPrec) {
        ret->opIsRightAssociative = TokenKind_isRightAssociative(ret->kind);
        ret->opIsUnary = TokenKind_isUnary(ret->kind);
    }

    exprsAllocHistogram[ret->kind]++;

    switch (ret->kind) {
    case TKIdentifier:
    case TKString:
    case TKRegex:
    case TKInline:
    case TKNumber:
    case TKMultiDotNumber:
    case TKLineComment: // Comments go in the AST like regular stmts
        ret->strLen = (uint16_t)this->matchlen;
        ret->value.string = this->pos;
        break;
    default:;
    }
    // the '!' will be trampled
    if (ret->kind == TKLineComment) ret->value.string++;
    // turn all 1.0234[DdE]+01 into 1.0234e+01.
    if (ret->kind == TKNumber) {
        str_tr_ip(ret->value.string, 'd', 'e', ret->strLen);
        str_tr_ip(ret->value.string, 'D', 'e', ret->strLen);
        str_tr_ip(ret->value.string, 'E', 'e', ret->strLen);
    }
    return ret;
}

union Value ASTExpr_eval(ASTExpr* this)
{
    // TODO: value is a union of whatever
    union Value v;
    v.d = 0;
    return v;
}

bool ASTExpr_canThrow(ASTExpr* this)
{
    if (not this) return false;
    switch (this->kind) {
    case TKNumber:
    case TKMultiDotNumber:
    case TKRegex:
    case TKInline:
    case TKIdentifier:
    case TKIdentifierResolved:
    case TKString:
    case TKLineComment:
        return false;
    case TKFunctionCall:
    case TKFunctionCallResolved:
        return not this->func->flags.nothrow;
        // actually  only if the func really throws
    case TKSubscript:
    case TKSubscriptResolved:
        return ASTExpr_canThrow(this->left);
    case TKVarAssign:
        return ASTExpr_canThrow(this->var->init);
    case TKKeyword_for:
    case TKKeyword_if:
    case TKKeyword_while:
        return false; // actually the condition could throw.
    default:
        if (not this->opPrec) return false;
        return ASTExpr_canThrow(this->left)
            or ASTExpr_canThrow(this->right);
    }
}

TypeTypes evalType()
{
    // TODO:
    // this func should return union {ASTType*; TypeTypes;} with the
    // TypeTypes value reduced by 1 (since it cannot be unresolved and
    // since 0 LSB would mean valid pointer (to an ASTType)).
    // this func does all type checking too btw and reports errors.
    return TYBool;
}

void ASTExpr_gen(
    ASTExpr* this, int level, bool spacing, bool escapeStrings);

#pragma mark - AST VAR IMPL.

void ASTVar_gen(ASTVar* this, int level)
{
    printf("%.*s%s%s", level, spaces,
        this->flags.isVar ? "var " : this->flags.isLet ? "let " : "",
        this->name);
    if (not(this->init and this->init->kind == TKFunctionCall
            and !strcmp(this->init->name, this->typeSpec->name))) {
        printf(" as ");
        ASTTypeSpec_gen(this->typeSpec, level + STEP);
    }
    // }
    // else {
    //     // should make this Expr_defaultType and it does it recursively
    //     for
    //     // [, etc
    //     const char* ctyp = TokenKind_defaultType(
    //         this->init ? this->init->kind : TKUnknown);
    //     if (this->init and this->init->kind == TKArrayOpen)
    //         ctyp = TokenKind_defaultType(
    //             this->init->right ? this->init->right->kind : TKUnknown);
    //     if (this->init and this->init->kind == TKFunctionCall
    //         and *this->init->name >= 'A' and *this->init->name <= 'Z')
    //         ctyp = NULL;
    //     if (ctyp) printf(" as %s", ctyp);
    // }
    if (this->init) {
        printf(" = ");
        ASTExpr_gen(this->init, 0, true, false);
    }
}

#pragma mark - AST SCOPE IMPL.

size_t ASTScope_calcSizeUsage(ASTScope* this)
{
    size_t size = 0, sum = 0, subsize = 0, maxsubsize = 0;
    // all variables must be resolved before calling this, call it e.g.
    // during cgen
    foreach (ASTExpr*, stmt, stmts, this->stmts) {
        switch (stmt->kind) {
        case TKKeyword_if:
        case TKKeyword_else:
        case TKKeyword_for:
        case TKKeyword_while:
            subsize = ASTScope_calcSizeUsage(stmt->body);
            if (subsize > maxsubsize) maxsubsize = subsize;
            break;
        // case TKVarAssign:
        //     assert(size = TypeType_size(stmt->var->typeSpec->typeType));
        //     sum += size;
        //     break;
        default:;
        }
    }
    // some vars are not assigned, esp. temporaries _1 _2 etc.
    foreach (ASTVar*, var, vars, this->locals) {
        assert(size = TypeType_size(var->typeSpec->typeType));
        sum += size;
    }
    // add the largest size among the sizes of the sub-scopes
    sum += maxsubsize;
    return sum;
}

ASTVar* ASTScope_getVar(ASTScope* this, const char* name)
{
    // stupid linear search, no dictionary yet
    foreach (ASTVar*, local, locals, this->locals) {
        if (not strcmp(name, local->name)) return local;
    }
    // in principle this will walk up the scopes, including the
    // function's "own" scope (which has locals = func's args). It
    // doesn't yet reach up to module scope, because there isn't yet a
    // "module" scope.
    if (this->parent) return ASTScope_getVar(this->parent, name);
    return NULL;
}

void ASTScope_gen(ASTScope* this, int level)
{
    foreach (ASTExpr*, stmt, stmts, this->stmts) {
        ASTExpr_gen(stmt, level, true, false);
        puts("");
    }
}

/*
 #pragma mark - AST NodeRef

 class ASTNodeRef {
 union {
 struct {
 uint64_t typ : 3, _danger : 45, _unused : 16;
 };
 uintptr_t uiptr;
 void* ptr = NULL;
 };

 public:
 enum NodeKind { NKVar, NKExpr, NKFunc, NKType, NKModule, NKTest };

 ASTNodeRef() {}

 ASTNodeRef(ASTVar* varNode)
 {
 ptr = varNode;
 typ = NKVar;
 }

 ASTNodeRef(ASTExpr* exprNode)
 {
 ptr = exprNode;
 typ = NKExpr;
 }
 inline uintptr_t getptr() { return (uiptr & 0xFFFFFFFFFFFFFFF8); }
 uint8_t kind() { return typ; } // this only takes values 0-7, 3 bits
 //--
 ASTVar* var() { return (ASTVar*)getptr(); }
 ASTExpr* expr() { return (ASTExpr*)getptr(); }
 };
 */

#pragma mark - AST TYPE IMPL.

ASTVar* ASTType_getVar(ASTType* this, const char* name)
{
    // stupid linear search, no dictionary yet
    foreach (ASTVar*, var, vars, this->body->locals) {
        if (not strcmp(name, var->name)) return var;
    }
    if (this->super and this->super->typeType == TYObject)
        return ASTType_getVar(this->super->type, name);
    return NULL;
}

void ASTType_gen(ASTType* this, int level)
{
    if (not this->body) printf("declare ");
    printf("type %s", this->name);
    if (this->super) {
        printf(" extends ");
        ASTTypeSpec_gen(this->super, level);
    }
    puts("");
    if (not this->body) return;

    foreach (ASTExpr*, stmt, stmts, this->body->stmts) {
        if (not stmt) continue;
        ASTExpr_gen(stmt, level + STEP, true, false);
        puts("");
    }
    puts("end type\n");
}

#pragma mark - AST FUNC IMPL.

size_t ASTFunc_calcSizeUsage(ASTFunc* this)
{
    size_t size = 0, sum = 0;
    foreach (ASTVar*, arg, args, this->args) {
        // all variables must be resolved before calling this
        size = TypeType_size(arg->typeSpec->typeType);
        sum += size;
    }
    if (this->body) sum += ASTScope_calcSizeUsage(this->body);
    return sum;
}

void ASTFunc_gen(ASTFunc* this, int level)
{
    if (this->flags.isDeclare) printf("declare ");

    printf("function %s(", this->name);

    foreach (ASTVar*, arg, args, this->args) {
        ASTVar_gen(arg, level);
        printf(args->next ? ", " : "");
    }
    printf(")");

    if (this->returnType) {
        printf(" returns ");
        ASTTypeSpec_gen(this->returnType, level);
    }
    puts("");
    if (this->flags.isDeclare) return;

    ASTScope_gen(this->body, level + STEP);

    puts("end function\n");
}

#pragma mark - AST EXPR IMPL.

void ASTExpr_gen(ASTExpr* this, int level, bool spacing, bool escapeStrings)
{
    // generally an expr is not split over several lines (but maybe in
    // rare cases). so level is not passed on to recursive calls.
    printf("%.*s", level, spaces);

    switch (this->kind) {
    case TKNumber:
    case TKMultiDotNumber:
    case TKRegex:
    case TKInline:
        printf("%.*s", this->strLen, this->value.string);
        break;

    case TKIdentifier:
    case TKIdentifierResolved: {
        char* tmp = (this->kind == TKIdentifierResolved) ? this->var->name
                                                         : this->name;
        printf("%s", tmp);
    } break;

    case TKString:
        printf(escapeStrings ? "\\%.*s\\\"" : "%.*s\"", this->strLen - 1,
            this->value.string);
        break;

    case TKLineComment:
        printf("%s%.*s",
            TokenKind_repr(TKLineComment, *this->value.string != ' '),
            this->strLen, this->value.string);
        break;

    case TKFunctionCall:
    case TKFunctionCallResolved: {
        char* tmp = (this->kind == TKFunctionCallResolved)
            ? this->func->name
            : this->name;
        printf("%s(", tmp);
        if (this->left) ASTExpr_gen(this->left, 0, false, escapeStrings);
        printf(")");
    } break;

    case TKSubscript:
    case TKSubscriptResolved: {
        char* tmp = (this->kind == TKSubscriptResolved) ? this->var->name
                                                        : this->name;
        printf("%s[", tmp);
        if (this->left) ASTExpr_gen(this->left, 0, false, escapeStrings);
        printf("]");
    } break;

    case TKVarAssign:
        // var x as XYZ = abc... -> becomes an ASTVar and an ASTExpr
        // (to keep location). Send it to ASTVar_gen.
        assert(this->var != NULL);
        ASTVar_gen(this->var, 0);
        break;

    case TKKeyword_for:
    case TKKeyword_if:
    case TKKeyword_while:
        printf("%s ", TokenKind_repr(this->kind, false));
        if (this->left) ASTExpr_gen(this->left, 0, true, escapeStrings);
        puts("");
        if (this->body)
            ASTScope_gen(
                this->body, level + STEP); //, true, escapeStrings);
        printf(
            "%.*send %s", level, spaces, TokenKind_repr(this->kind, false));
        break;

    default:
        if (not this->opPrec) break;
        // not an operator, but this should be error if you reach here
        bool leftBr = this->left and this->left->opPrec
            and this->left->opPrec < this->opPrec;
        bool rightBr = this->right and this->right->opPrec
            and this->right->kind
                != TKKeyword_return // found in 'or return'
            and this->right->opPrec < this->opPrec;

        if (this->kind == TKOpColon) {
            // expressions like arr[a:x-3:2] should become
            // arr[a:(x-3):2]
            // or list literals [8, 9, 6, 77, sin(c)]
            if (this->left) switch (this->left->kind) {
                case TKNumber:
                case TKIdentifier:
                case TKString:
                case TKOpColon:
                case TKMultiDotNumber:
                case TKUnaryMinus:
                    break;
                default:
                    leftBr = true;
                }
            if (this->right) switch (this->right->kind) {
                case TKNumber:
                case TKIdentifier:
                case TKString:
                case TKOpColon:
                case TKMultiDotNumber:
                case TKUnaryMinus:
                    break;
                default:
                    rightBr = true;
                }
        }

        //        if (false and this->kind == TKKeyword_return and
        //        this->right) {
        //            switch (this->right->kind) {
        //            case TKString:
        //            case TKNumber:
        //            case TKIdentifier:
        //            case TKFunctionCall:
        //            case TKSubscript:
        //            case TKRegex:
        //            case TKMultiDotNumber:
        //                break;
        //            default:
        //                rightBr = true;
        //                break;
        //            }
        //        }

        if (this->kind == TKPower and not spacing) putc('(', stdout);

        char lpo = leftBr and this->left->kind == TKOpColon ? '[' : '(';
        char lpc = leftBr and this->left->kind == TKOpColon ? ']' : ')';
        if (leftBr) putc(lpo, stdout);
        if (this->left)
            ASTExpr_gen(this->left, 0,
                spacing and !leftBr and this->kind != TKOpColon,
                escapeStrings);
        if (leftBr) putc(lpc, stdout);

        printf("%s", TokenKind_repr(this->kind, spacing));

        char rpo = rightBr and this->right->kind == TKOpColon ? '[' : '(';
        char rpc = rightBr and this->right->kind == TKOpColon ? ']' : ')';
        if (rightBr) putc(rpo, stdout);
        if (this->right)
            ASTExpr_gen(this->right, 0,
                spacing and !rightBr and this->kind != TKOpColon,
                escapeStrings);
        if (rightBr) putc(rpc, stdout);

        if (this->kind == TKPower and not spacing) putc(')', stdout);
        if (this->kind == TKArrayOpen) putc(']', stdout);
    }
}

const char* ASTExpr_typeName(ASTExpr* this)
{
    if (not this) return "";
    switch (this->kind) {
    case TKNumber:
    case TKPlus:
    case TKMinus:
    case TKPower:
    case TKSlash:
    case TKTimes:
    case TKOpMod:
        return "Scalar";
    case TKString:
        return "String";
    case TKRegex:
        return "RegEx";
        // case TKFunctionCall: return this->name;
    case TKFunctionCallResolved: {
        const char* name = TypeType_name(this->func->returnType->typeType);
        if (!*name) name = this->func->returnType->type->name;
        return name;
    }

        //        return this->func->returnType->type->name;
        // NOO! TODO: get the name from the resolved type
        // ^^ figure it out
    case TKIdentifierResolved:
    case TKSubscriptResolved:
        // object: typetype_name will give "", then return ->type->name
        // unresolved: should not happen at this stage!
        // other: get it from typeType
        {
            const char* name = TypeType_name(this->var->typeSpec->typeType);
            if (!*name) name = this->var->typeSpec->type->name;
            return name;
        }
        // same here as for resolved func
    case TKOpEQ:
    case TKOpNE:
    case TKOpGE:
    case TKOpGT:
    case TKOpLE:
    case TKOpLT:
    case TKKeyword_and:
    case TKKeyword_or:
    case TKKeyword_not:
        // TODO: literals yes and no
        return "Logical";
    case TKOpColon:
        return "Range";
        // TODO: what else???
    default:
        fprintf(stderr, "%s\n", TokenKind_repr(this->kind, false));
        assert(0);
    }
}

void ASTExpr_catarglabels(ASTExpr* this)
{
    switch (this->kind) {
    case TKOpComma:
        ASTExpr_catarglabels(this->left);
        ASTExpr_catarglabels(this->right);
        break;
    case TKOpAssign:
        printf("_%s", this->left->name);
        break;
    default:
        break;
    }
}
int ASTExpr_strarglabels(ASTExpr* this, char* buf, int bufsize)
{
    int ret = 0;
    switch (this->kind) {
    case TKOpComma:
        ret += ASTExpr_strarglabels(this->left, buf, bufsize);
        ret += ASTExpr_strarglabels(this->right, buf + ret, bufsize - ret);
        break;
    case TKOpAssign:
        ret += snprintf(buf, bufsize, "_%s", this->left->name);
        break;
    default:
        break;
    }
    return ret;
}
int ASTExpr_countCommaList(ASTExpr* this)
{
    // whAT A MESS WITH BRANCHING
    if (not this) return 0;
    if (this->kind != TKOpComma) return 1;
    int i = 1;
    while (this->right) {
        this = this->right;
        i++;
        if (this->kind != TKOpComma) break;
    }
    return i;
}

#pragma mark - AST MODULE IMPL.

ASTType* ASTModule_getType(ASTModule* this, const char* name)
{
    // the type may be "mm.XYZType" in which case you should look in
    // module mm instead. actually the caller should have bothered about
    // that.
    foreach (ASTType*, type, types, this->types) {
        if (not strcmp(type->name, name)) return type;
    }
    // type specs must be fully qualified, so there's no need to look in
    // other modules.
    return NULL;
}

// i like this pattern, getType, getFunc, getVar, etc.
// even the module should have getVar.
// you don't need the actual ASTImport object, so this one is just a
// bool. imports just turn into a #define for the alias and an #include
// for the actual file.
bool ASTModule_hasImportAlias(ASTModule* this, const char* alias)
{
    foreach (ASTImport*, imp, imps, this->imports) {
        if (not strcmp(imp->importFile + imp->aliasOffset, alias))
            return true;
    }
    return false;
}

ASTFunc* ASTModule_getFunc(ASTModule* this, const char* name)
{
    // figure out how to deal with overloads. or set a selector field in
    // each astfunc.
    foreach (ASTFunc*, func, funcs, this->funcs) {
        if (not strcmp(func->name, name)) return func;
    }
    // again no looking anywhere else. If the name is of the form
    // "mm.func" you should have bothered to look in mm instead.
    return NULL;
}

void ASTModule_gen(ASTModule* this, int level)
{
    printf("! module %s\n", this->name);

    foreach (ASTImport*, import, imports, this->imports)
        ASTImport_gen(import, level);

    puts("");

    foreach (ASTType*, type, types, this->types)
        ASTType_gen(type, level);

    foreach (ASTFunc*, func, funcs, this->funcs)
        ASTFunc_gen(func, level);
}

#include "genc.h"
#include "genlua.h"
#include "gencpp.h"

#pragma mark - PARSER

typedef enum ParserMode {
    PMLint,
    PMGenC,
    PMGenLua,
    PMGenCpp,
    PMGenJavaScript,
    PMGenWebAsm,
    PMGenPython
} ParserMode;

typedef struct Parser {
    // bring down struct size!
    char* filename; // mod/submod/xyz/mycode.ch
    char* moduleName; // mod.submod.xyz.mycode
    char* mangledName; // mod_submod_xyz_mycode
    char* capsMangledName; // MOD_SUBMOD_XYZ_MYCODE
    // char* basename; // mycode
    // char* dirname; // mod/submod/xyz
    char *data, *end;
    char* noext;
    Token token; // current
    List(ASTModule*) * modules; // module node of the AST
                                // Stack(ASTScope*) scopes; // a stack
                                // that keeps track of scope nesting
    uint32_t errCount, warnCount;
    uint16_t errLimit;

    ParserMode mode : 8;
    bool generateCommentExprs; // set to false when compiling, set to
                               // true when linting

} Parser;

// STHEAD_POOLB(Parser, 16)

// size_t dataSize() { return end - data; }

#define STR(x) STR_(x)
#define STR_(x) #x

void Parser_fini(Parser* this)
{
    free(this->data);
    free(this->noext);
    free(this->moduleName);
    free(this->mangledName);
    free(this->capsMangledName);
}
#define FILE_SIZE_MAX 1 << 24

Parser* Parser_fromFile(char* filename, bool skipws)
{

    size_t flen = strlen(filename);

    // Error: the file might not end in .ch
    if (not str_endswith(filename, flen, ".ch", 3)) {
        eprintf(
            "F+: file '%s' invalid: name must end in '.ch'.\n", filename);
        return NULL;
    }

    struct stat sb;

    // Error: the file might not exist
    if (stat(filename, &sb) != 0) {
        eprintf("F+: file '%s' not found.\n", filename);
        return NULL;
    } else if (S_ISDIR(sb.st_mode)) {
        // Error: the "file" might really be a folder
        eprintf(
            "F+: '%s' is a folder; only files are accepted.\n", filename);
        return NULL;
    } else if (access(filename, R_OK) == -1) {
        // Error: the user might not have read permissions for the file
        eprintf("F+: no permission to read file '%s'.\n", filename);
        return NULL;
    }

    FILE* file = fopen(filename, "r");
    assert(file);

    Parser* ret = NEW(Parser);

    ret->filename = filename;
    ret->noext = str_noext(filename);
    fseek(file, 0, SEEK_END);
    const size_t size = ftell(file) + 2;

    // 2 null chars, so we can always lookahead
    if (size < FILE_SIZE_MAX) {
        ret->data = (char*)malloc(size);
        fseek(file, 0, SEEK_SET);
        if (fread(ret->data, size - 2, 1, file) != 1) {
            eprintf(
                "F+: the whole file '%s' could not be read.\n", filename);
            fclose(file);
            return NULL;
            // would leak if ret was malloc'd directly, but we have a pool
        }
        ret->data[size - 1] = 0;
        ret->data[size - 2] = 0;
        ret->moduleName = str_tr(ret->noext, '/', '.');
        ret->mangledName = str_tr(ret->noext, '/', '_');
        ret->capsMangledName = str_upper(ret->mangledName);
        // ret->basename = str_base(ret->noext, '/', flen);
        // ret->dirname = str_dir(ret->noext);
        ret->end = ret->data + size;
        ret->token.pos = ret->data;
        ret->token.flags.skipWhiteSpace = skipws;
        ret->token.flags.mergeArrayDims = false;

        ret->token.kind = TKUnknown;
        ret->token.line = 1;
        ret->token.col = 1;
        ret->mode = PMGenC; // parse args to set this
        ret->errCount = 0;
        ret->warnCount = 0;
        ret->errLimit = 20;
    } else {
        eputs("Source files larger than 16MB are not allowed.\n");
    }

    fclose(file);
    return ret;
}

#include "errors.h"

#pragma mark - PARSING BASICS

ASTExpr* exprFromCurrentToken(Parser* this)
{
    ASTExpr* expr = ASTExpr_fromToken(&this->token);
    Token_advance(&this->token);
    return expr;
}

ASTExpr* next_token_node(
    Parser* this, TokenKind expected, const bool ignore_error)
{
    if (this->token.kind == expected) {
        return exprFromCurrentToken(this);
    } else {
        if (not ignore_error) Parser_errorExpectedToken(this, expected);
        return NULL;
    }
}
// these should all be part of Token_ when converted back to C
// in the match case, this->token should be advanced on error
ASTExpr* match(Parser* this, TokenKind expected)
{
    return next_token_node(this, expected, false);
}

// this returns the match node or null
ASTExpr* trymatch(Parser* this, TokenKind expected)
{
    return next_token_node(this, expected, true);
}

// just yes or no, simple
bool matches(Parser* this, TokenKind expected)
{
    return (this->token.kind == expected);
}

bool Parser_ignore(Parser* this, TokenKind expected)
{
    bool ret;
    if ((ret = matches(this, expected))) Token_advance(&this->token);
    return ret;
}

// this is same as match without return
void discard(Parser* this, TokenKind expected)
{
    if (not Parser_ignore(this, expected))
        Parser_errorExpectedToken(this, expected);
}

char* parseIdent(Parser* this)
{
    if (this->token.kind != TKIdentifier)
        Parser_errorExpectedToken(this, TKIdentifier);
    char* p = this->token.pos;
    Token_advance(&this->token);
    return p;
}

#include "resolve.h"

#pragma mark - PARSE EXPR
ASTExpr* parseExpr(Parser* this)
{
    // there are 2 steps to this madness.
    // 1. parse a sequence of tokens into RPN using shunting-yard.
    // 2. walk the rpn stack as a sequence and copy it into a result
    // stack, collapsing the stack when you find nonterminals (ops, func
    // calls, array index, ...)

    // we could make this static and set len to 0 upon func exit
    static PtrArray /* ASTExpr*, 32 */ rpn, ops, result;
    int prec_top = 0;
    ASTExpr* p = NULL;
    TokenKind revBrkt = TKUnknown;

    // ******* STEP 1 CONVERT TOKENS INTO RPN

    while (this->token.kind != TKNullChar and this->token.kind != TKNewline
        and this->token.kind != TKLineComment) { // build RPN

        // you have to ensure that ops have a space around them, etc.
        // so don't just skip the one spaces like you do now.
        if (this->token.kind == TKOneSpace) Token_advance(&this->token);
        if (this->token.kind == TKIdentifier
            and memchr(this->token.pos, '_', this->token.matchlen)
            /*or doesKeywordMatch(expr->value.string, expr->strLen)*/)
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
            if (lookAheadChar == ')')
                PtrArray_push(&rpn,
                    NULL); // for empty func() push null for no args
            break;

        case TKArrayOpen:
            PtrArray_push(&ops, expr);
            if (not PtrArray_empty(&ops)
                and PtrArray_topAs(ASTExpr*, &ops)->kind == TKSubscript)
                PtrArray_push(&rpn, expr);
            if (lookAheadChar == ')')
                PtrArray_push(&rpn,
                    NULL); // for empty arr[] push null for no args
            break;

        case TKParenClose:
        case TKArrayClose:
        case TKBraceClose:

            revBrkt = TokenKind_reverseBracket(expr->kind);
            if (PtrArray_empty(
                    &ops)) { // need atleast the opening bracket of
                // the current kind
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
        case TKKeyword_return:
            // for empty return, push a NULL if there is no expr coming.
            PtrArray_push(&ops, expr);
            if (lookAheadChar == '!' or lookAheadChar == '\n')
                PtrArray_push(&rpn, NULL);
            break;
        default:
            if (prec) { // general operators

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
                                    == TKArrayOpen)) // <<-----
                        // yesssssssssss
                    )
                        // TODO: better way to parse :, 1:, :-1, etc.
                        // while passing tokens to RPN, if you see a :
                        // with nothing on the RPN or comma or [, push a
                        // NULL. while unwinding the op stack, if you
                        // pop a : and see a NULL or comma on the rpn,
                        // push another NULL.
                        PtrArray_push(
                            &rpn, NULL); // indicates empty operand
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
                        and p->kind != TKOpColon
                        // in case of ::, second colon will add null
                        // later
                        and p->kind != TKSubscript and rpn.used < 2) {
                        Parser_errorUnexpectedToken(this);
                        goto error;
                        // TODO: even if you have more than two, neither
                        // of the top two should be a comma
                    }

                    PtrArray_push(&rpn, p);
                }

                // when the first error is found in an expression, seek
                // to the next newline and return NULL.
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

    // TODO: run evalType() and report errors

    // for next invocation just set.used to 0, allocation will be
    // reused
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
                printf("%.*s ", e->opPrec ? 100 : e->strLen,
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
                printf("%.*s ", e->opPrec ? 100 : e->strLen,
                    e->opPrec ? TokenKind_repr(e->kind, false) : e->name);
            }
        puts("");
    }

    if (p) {
        printf("      p: %.*s ", p->opPrec ? 100 : p->strLen,
            p->opPrec ? TokenKind_repr(p->kind, false) : p->name);
        puts("");
    }

    // if (rpn.used or ops.used or result.used) puts("");

    ops.used = 0; // "reset" stacks
    rpn.used = 0;
    result.used = 0;
    return NULL;
}

#pragma mark - PARSE TYPESPEC
ASTTypeSpec* parseTypeSpec(Parser* this)
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
ASTVar* parseVar(Parser* this)
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
        var->typeSpec->name = "Scalar";
    }

    Parser_ignore(this, TKOneSpace);
    if (Parser_ignore(this, TKOpAssign)) var->init = parseExpr(this);

    // TODO: set default inferred type if typeSpec is NULL but init
    // present.
    //     const char* ctyp
    // = TokenKind_defaultType(var->init ? var->init->kind : TKUnknown);

    return var;
}

List(ASTVar) * parseArgs(Parser* this)
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
ASTScope* parseScope(
    Parser* this, ASTScope* parent, bool isTypeBody, bool isIfBlock)
{
    ASTScope* scope = NEW(ASTScope);

    ASTVar *var = NULL, *orig = NULL;
    ASTExpr* expr = NULL;
    TokenKind tt = TKUnknown;

    scope->parent = parent;
    //    while (this->token.kind == TKNewline or this->token.kind ==
    //    TKOneSpace)
    //        Token_advance(&this->token);
    bool startedElse = false; //(this->token.kind == TKKeyword_else);
    while (this->token.kind != TKKeyword_end) {

        switch (this->token.kind) {

        case TKNullChar:
            Parser_errorExpectedToken(this, TKUnknown);
            goto exitloop;

        case TKKeyword_var:
        case TKKeyword_let:
            var = parseVar(this);
            if (not var) continue;
            if ((orig = ASTScope_getVar(scope, var->name)))
                Parser_errorDuplicateVar(this, var, orig);
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
            expr->line = this->token.line;
            expr->col = this->token.col;
            expr->opPrec = TokenKind_getPrecedence(TKOpAssign);
            expr->var = var;
            PtrList_append(&scope->stmts, expr);
            break;

        case TKKeyword_else:
            if (not startedElse) goto exitloop;
            //            else
            //                startedElse = false; // so that it
            // fallthrough
        case TKKeyword_if:
        case TKKeyword_for:
        case TKKeyword_while:
            if (isTypeBody) Parser_errorInvalidTypeMember(this);
            tt = this->token.kind;
            expr = match(this, tt); // will advance
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
            // insert a temp scope holding the var that for declares
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
                //                 discard(this, TKKeyword_else);
            } else { // if (matches(this, TKKeyword_end)) {
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
            if (expr and isTypeBody
                and (expr->kind != TKFunctionCall
                    or strncmp(expr->name, "check", 6))) {
                Parser_errorInvalidTypeMember(this);
                // do you want to continue to find errors in exprs that
                // are anyway invalid? If yes, allow the expr to be
                // added to the body, or else, set the expr to null
                // here.
                expr = NULL;
            }
            if (not expr) break;
            PtrList_append(&scope->stmts, expr);
            resolveVars(this, expr, scope, false);
            break;
        }
    }
exitloop:
    return scope;
}

#pragma mark - PARSE PARAM
List(ASTVar) * parseParams(Parser* this)
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
ASTFunc* parseFunc(Parser* this, bool shouldParseBody)
{
    discard(this, TKKeyword_function);
    discard(this, TKOneSpace);
    ASTFunc* func = NEW(ASTFunc);

    func->line = this->token.line;
    // func->col = this->token.col;

    if (memchr(this->token.pos, '_', this->token.matchlen))
        Parser_errorInvalidIdent(this);
    if (*this->token.pos < 'a' or *this->token.pos > 'z')
        Parser_errorInvalidIdent(this);
    func->name = parseIdent(this);

    func->flags.isDeclare = !shouldParseBody;

    func->args = parseArgs(this);
    func->argCount = PtrList_count(func->args);

    // func->selector = PoolB_alloc(&strPool, getSelectorLength(func));
    // getSelector(func);

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

ASTFunc* parseStmtFunc(Parser* this)
{
    ASTFunc* func = NEW(ASTFunc);

    func->line = this->token.line;
    // func->col = this->token.col;

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
    }

    ASTExpr* ret = exprFromCurrentToken(this);
    assert(ret->kind == TKColEq);
    ret->kind = TKKeyword_return;
    ret->opIsUnary = true;

    ret->right = parseExpr(this);
    ASTScope* scope = NEW(ASTScope);
    PtrList_append(&scope->stmts, ret);

    func->body = scope;

    return func;
}

#pragma mark - PARSE TEST
ASTFunc* parseTest(Parser* this) { return NULL; }

#pragma mark - PARSE UNITS
ASTUnits* parseUnits(Parser* this) { return NULL; }

#pragma mark - PARSE TYPE
ASTType* parseType(Parser* this, bool shouldParseBody)
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

#pragma mark - PARSE IMPORT
ASTImport* parseImport(Parser* this)
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
void getSelector(ASTFunc* func)
{
    if (func->argCount) {
        size_t selLen = 0;
        int remain = 128, wrote = 0;
        char buf[128];
        buf[127] = 0;
        char* bufp = buf;
        ASTVar* arg1 = (ASTVar*)func->args->item;
        wrote = snprintf(
            bufp, remain, "%s_", ASTTypeSpec_name(arg1->typeSpec));
        selLen += wrote;
        bufp += wrote;
        remain -= wrote;

        wrote = snprintf(bufp, remain, "%s", func->name);
        selLen += wrote;
        bufp += wrote;
        remain -= wrote;

        foreach (ASTVar*, arg, args, func->args->next) {
            wrote = snprintf(bufp, remain, "_%s", arg->name);
            selLen += wrote;
            bufp += wrote;
            remain -= wrote;
        }
        func->selector = PoolB_alloc(&strPool, selLen + 1);
        memcpy(func->selector, buf, selLen + 1);
    } else
        func->selector = func->name;
    printf("// got func %s: %s\n", func->name, func->selector);
}

#include "typecheck.h"

#pragma mark - PARSE MODULE
List(ASTModule) * parseModule(Parser* this)
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
            // printf("other token: %s at %d:%d len %d\n",
            //     TokenKind_repr(this->token.kind, false),
            //     this->token.line, this->token.col,
            //     this->token.matchlen);
            Parser_errorUnexpectedToken(this);
            while (this->token.kind != TKNewline
                and this->token.kind != TKLineComment)
                Token_advance(&this->token);
        }
    }
    // also keep modulesTop

    // do some analysis that happens after the entire module is loaded

    foreach (ASTType*, type, types, root->types) {
        if (type->super) resolveTypeSpec(this, type->super, root);
        if (not type->body) continue;
        foreach (ASTExpr*, stmt, stmts, type->body->stmts)
            resolveFuncsAndTypes(this, stmt, root);
    }

    foreach (ASTFunc*, func, funcs, root->funcs) {
        foreach (ASTVar*, arg, args, func->args)
            resolveTypeSpec(this, arg->typeSpec, root);
        if (func->returnType) resolveTypeSpec(this, func->returnType, root);
        getSelector(func);
    }
    foreach (ASTFunc*, func, mfuncs, root->funcs) {
        if (func->body) {
            foreach (ASTExpr*, stmt, stmts, func->body->stmts) {
                resolveFuncsAndTypes(this, stmt, root);
                setExprTypeInfo(this, stmt);
            } // should be part of astmodule, and
              // resolveVars should be part of astscope
              // resolveTypeSpecs(this, stmt, root);

            if (not this->errCount) ASTScope_promoteCandidates(func->body);
            // no point doing code motion if there were errors
        }
    }

    PtrList_append(&this->modules, root);
    return this->modules;
}

// TODO: this should be in ASTModule open/close
void Parser_genc_open(Parser* this)
{
    printf("#ifndef HAVE_%s\n#define HAVE_%s\n\n", this->capsMangledName,
        this->capsMangledName);
    printf("#define THISMODULE %s\n", this->mangledName);
    printf("#define THISFILE \"%s\"\n", this->filename);
}
void Parser_genc_close(Parser* this)
{
    printf("#undef THISMODULE\n");
    printf("#undef THISFILE\n");
    printf("#endif // HAVE_%s\n", this->capsMangledName);
}

#define List_ASTExpr PtrList
#define List_ASTVar PtrList
#define List_ASTModule PtrList
#define List_ASTFunc PtrList
#define List_ASTType PtrList
#define List_ASTImport PtrList
#define List_ASTScope PtrList

void alloc_stat() {}

ASTTypeSpec* standardTypeSpecs[16][16];
// ^ these will correspond to enum TypeTypes

#pragma mark - main
int main(int argc, char* argv[])
{
    if (argc == 1) {
        eputs("F+: no input files.\n");
        return 1;
    }
    bool printDiagnostics = (argc > 2 && *argv[2] == 'd') or false;

    //    for (int i = 2; i < 16; i++) // first 2 are unresolved and object
    //    resp.
    //        for (int j = 2; j < 18; j++)
    //            standardTypeSpecs[i][j-2]
    //                = ASTTypeSpec_new((TypeTypes)i, (CollectionTypes)j);
    //    // new sets typeType and name, sets collectionType to none

    ticks t0 = getticks();

    List(ASTModule) * modules;
    Parser* parser;

    parser = Parser_fromFile(argv[1], true);
    if (not parser) {
        return 2;
    }
    modules = parseModule(parser);

    if (not(parser->errCount or parser->warnCount)) {
        switch (parser->mode) {
        case PMLint: {
            foreach (ASTModule*, mod, mods, modules)
                ASTModule_gen(mod, 0);
        } break;

        case PMGenLua: {
            foreach (ASTModule*, mod, mods, modules)
                ASTModule_genlua(mod, 0);
        } break;
        case PMGenC: {
            printf("#include \"checkstd.h\"\n");
            Parser_genc_open(parser);
            foreach (ASTModule*, mod, mods, modules)
                ASTModule_genc(mod, 0);
            Parser_genc_close(parser);
        } break;
        case PMGenCpp: {
            printf("#include \"checkstd.hpp\"\n");
            Parser_genc_open(parser);
            foreach (ASTModule*, mod, mods, modules)
                ASTModule_gencpp(mod, 0);
            Parser_genc_close(parser);
        } break;
        default:
            break;
        }
    }
    double tms = elapsed(getticks(), t0) / 1e6;

    if (printDiagnostics) {
        eputs("\n======================================================="
              "\n");
        eputs("\e[1mPARSER STATISTICS\e[0m\n");
        eputs("-------------------------------------------------------"
              "\n");

        eputs("\e[1mNode allocations:\e[0m\n");
        allocstat(ASTImport);
        allocstat(ASTExpr);
        allocstat(ASTVar);
        allocstat(ASTType);
        allocstat(ASTScope);
        allocstat(ASTTypeSpec);
        allocstat(ASTFunc);
        allocstat(ASTModule);
        allocstat(PtrList);
        allocstat(List_ASTExpr);
        allocstat(List_ASTVar);
        allocstat(List_ASTModule);
        allocstat(List_ASTFunc);
        allocstat(List_ASTType);
        allocstat(List_ASTImport);
        allocstat(List_ASTScope);
        allocstat(Parser);
        eputs("-------------------------------------------------------"
              "\n");
        eprintf("*** Total size of nodes                     = %7d B\n",
            gPool.usedTotal);
        eprintf("*** Space alloated for nodes                = %7d B\n",
            gPool.capTotal);
        eprintf("*** Node space utilisation                  = %7.2f %%\n",
            gPool.usedTotal * 100.0 / gPool.capTotal);
        eputs("-------------------------------------------------------"
              "\n");
        eprintf("*** File size                               = %7lu B\n",
            parser->end - parser->data - 2);
        eprintf("*** Node size to file size ratio            = %7.2f x\n",
            gPool.usedTotal * 1.0 / (parser->end - parser->data - 2));
        eputs("-------------------------------------------------------"
              "\n");
        eprintf("*** Space used for strings                  = %7u B\n",
            strPool.usedTotal);
        eprintf("*** Allocated for strings                   = %7u B\n",
            strPool.capTotal);
        eprintf("*** Space utilisation                       = %7.2f %%\n",
            strPool.usedTotal * 100.0 / strPool.capTotal);
        eputs("-------------------------------------------------------"
              "\n");
        eputs("\e[1mMemory-related calls\e[0m\n");
        eprintf("  calloc: %-7d | malloc: %-7d | realloc: %-7d\n",
            globalCallocCount, globalMallocCount, globalReallocCount);
        eprintf("  strlen: %-7d | strdup: %-7d |\n", globalStrlenCount,
            globalStrdupCount);
        eputs("-------------------------------------------------------"
              "\n");

        expralloc_stat();
    }

    if (printDiagnostics) {
        eprintf("\e[1mTime elapsed:\e[0m %.1f ms (%.1f ms / 32kB)\n", tms,
            tms * 32768.0
                / (parser->end - parser->data - 2)); // sw.print();
    }
    if (parser->errCount)
        eprintf("\n\e[31m*** %d errors\e[0m\n", parser->errCount);
    if (parser->warnCount)
        fprintf(
            stderr, "\n\e[33m*** %d warnings\e[0m\n", parser->warnCount);
    return (parser->errCount or parser->warnCount);
}
