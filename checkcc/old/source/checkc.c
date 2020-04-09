// TODO: need ASTEnum
//
// AS A RULE
// Nothing except Parser should allocate any memory.
// if you need new strings, Parser should have anticipated that in advance
// at its init and strdup'd them ready for use (e.g. variations of module
// names). If you use a single pool for everything, free parsers manually
// when done. OR have one pool for Parser and one for everything else. This
// way Parser will have its destructors auto called and since it is the only
// one that needs destructors to be called, self will get rid of the memory
// leaks. as a bonus, all AST objects can store a 2-byte index of the
// associated parser in the Parser pool, and store offsets to strings
// instead of char*. if bringing down ASTExpr to 16KB can help, consider
// having a separate pool only for ASTExpr, then left/right ptrs are 32-bit
// indexes into self pool.
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
// since files are expected to be small self ast-based linter should be OK?
// keep modules limited to 2000 lines, or even 1000
// BUT if errors are found, no formatting can be done. in self case, after
// errors are reported, run the self->token-based linter (formatter).
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
// \0 anyway). self doesn't make buffer overflows any more likely: the
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

#include "cycle.h"

#define STEP 4

#pragma mark - Heap Allocation Extras

// Generally self is not to be done at runtime. But for now we can use it as
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
chmalloc(size) malloc(size) #define chcalloc(size, count) calloc(size,count)
#define chrealloc(ptr, size) realloc(ptr, size)
#endif

void f_chfree(void* ptr, const char* file, int line, const char* func)
{
    if (!ad_size.has(ptr)) {
        fprintf(stderr,"freeing unknown ptr in '%s' at %s line %d\n", func,
file, line); return;
    }
    aD_file.del(ptr);
    ad_func.del(ptr);
    ad_line.del(ptr);
    ad_size.del(ptr);
    free(ptr);
}

void* f_chmalloc(size_t size, const char* file, int line, const char* func )
{
    void* ret = malloc(size);
    if (!ret) {
        fprintf(stderr,"malloc failed in '%s' at %s line %d\n", func, file,
line); return NULL;
    }
    aD_file[ptr] = fname;
    ad_func[ptr] = func;
    ad_line[ptr] = line;
    ad_size[ptr] = size;
}
*/

// delimiter

// #ifdef _WIN32
// #define WHICH_DELIMITER ";"
// #else
// #define WHICH_DELIMITER ":"
// #endif
// JUST CHECK FOR /usr/bin/cc NO FANCY WHICH WHAT ETC
// char* which(const char* name, int* out_len) { return which_path(name,
// getenv("PATH"), out_len); } char* which_path(const char* name, const
// char* path, int* out_len)
// {
//     if (NULL == path) return NULL;
//     char* tok = (char*)path;

//     tok = strtok(tok, WHICH_DELIMITER);

//     while (tok) {
//         // path
// //        int len = strlen(tok) + 2 + strlen(name);

//         sprintf(file, "%s/%s", tok, name);

//         // executable
//         if (0 == access(file, X_OK)) {
//             return file;
//         }

//         // next self->token
//         tok = strtok(NULL, WHICH_DELIMITER);

//     }

//     return NULL;
// }
#define and &&
#define or ||
#define not !

static int globalCallocCount = 0;
#define calloc(n, s)                                                       \
    calloc(n, s);                                                          \
    globalCallocCount++;
static int globalMallocCount = 0;
#define malloc(s)                                                          \
    malloc(s);                                                             \
    globalMallocCount++;
static int globalStrdupCount = 0;
#define strdup(s)                                                          \
    strdup(s);                                                             \
    globalStrdupCount++;
static int globalReallocCount = 0;
#define realloc(ptr, s)                                                    \
    realloc(ptr, s);                                                       \
    globalReallocCount++;
static int globalStrlenCount = 0;
#define strlen(s)                                                          \
    strlen(s);                                                             \
    globalStrlenCount++;

#pragma mark - String Functions

char* str_noext(char* str)
{
    char* s = strdup(str);
    const size_t len = strlen(s);
    char* sc = s + len;
    while (sc > s and *sc != '.')
        sc--;
    if (sc >= s) *sc = '\0';
    return s;
}

char* str_base(char* str, char sep, size_t slen)
{
    if (!slen)
        return str; // you should pass in the len. len 0 is actually valid
                    // since basename for 'mod' is 'mod' itself, and self
                    // would have caused a call to strlen below. so len 0
                    // now means really just return what came in.
    char* s = str;
    char* sc = s + slen;
    while (sc > s and sc[-1] != sep)
        sc--;
    if (sc >= s) s = sc;
    return s;
}

char* str_dir(char* str)
{
    char* s = strdup(str);
    const size_t len = strlen(s);
    char* sc = s + len;
    while (sc > s and *sc != '/')
        sc--;
    if (sc >= s) *sc = '\0';
    return s;
}

char* str_upper(char* str)
{
    char* s = strdup(str);
    char* sc = s - 1;
    while (*++sc)
        if (*sc >= 'a' and *sc <= 'z') *sc -= 32;
    return s;
}

// in place
void str_tr_ip(
    char* str, const char oldc, const char newc, const size_t length)
{

    char* sc = str - 1;
    char* end = length ? str + length : (char*)0xFFFFFFFFFFFFFFFF;
    while (*++sc && sc < end)
        if (*sc == oldc) *sc = newc;
}

char* str_tr(char* str, const char oldc, const char newc)
{
    size_t len = strlen(str);
    char* s = strndup(str, len);
    str_tr_ip(s, oldc, newc, len);
    return s;
}
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
    // self can be 4B/8B, all others r pairs will be 8B/16B
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
typedef char bool;
#define true 1
#define false 0

union Value {
    // think about returning larger things like Interval etc.
    char* s;
    int64_t i;
    uint64_t u;
    double d;
};

// TODO: ASTTypeSpecs will have a TypeTypes typeType; that can be used
// to determine quickly if it is a primitive. fits in 4bits btw
typedef enum TypeTypes {
    TYUnresolved = 0, // unknown type
                      // nonprimitives: means they should have their own
                      // methods to print,
    // serialise, identify, reflect, etc.
    TYObject, // resolved to an ASTType
    // primitives that can be printed or represented with no fuss
    TYSize, // self is actually uintptr_t, since actual ptrs are
    // TYObjects. maybe rename it
    TYString, // need to distinguish String and char*?
    TYBool,
    // above self, ie. > 4 or >= TYInt8, all may have units |kg.m/s etc.
    TYInt8,
    TYUInt8,
    TYInt16,
    TYUInt16,
    TYInt32,
    TYUInt32,
    TYInt64, // for loop indices start out as size_t by default
    TYUInt64,
    TYReal16,
    TYReal32,
    TYReal64, // Numbers start out with Real64 by default
    // conplex, duals, intervals,etc.??
} TypeTypes;

bool TypeType_isnum(TypeTypes tyty) { return tyty >= TYInt8; }

const char* TypeType_nativeName(TypeTypes tyty)
{
    switch (tyty) {
    case TYUnresolved:
        return NULL;
    case TYObject:
        return "";
    case TYSize: // self is actually uintptr_t, since actual ptrs are
        // TYObjects. maybe rename it
        return "size_t";
    case TYString:
        return "char*";
    case TYBool:
        return "bool_t";
    // above self, ie. > 4 or >= TYInt8, all may have units |kg.m/s etc.
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

const char* TypeType_name(TypeTypes tyty)
{
    switch (tyty) {
    case TYUnresolved:
        return NULL;
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
    case TYReal16:
    case TYReal32:
    case TYReal64:
        return "Scalar";
    }
}

// these are DEFAULTS, most likely more specific ones will be used
// depending on context
const char* TypeType_defaultFormatSpec(TypeTypes tyty)
{
    switch (tyty) {
    case TYUnresolved:
        return NULL;
    case TYObject:
        return "%p";
    case TYSize: // self is actually uintptr_t, since actual ptrs are
        // TYObjects. maybe rename it
        return "%lu";
    case TYString:
        return "%s";
    case TYBool:
        return "%d";
    // above self, ie. > 4 or >= TYInt8, all may have units |kg.m/s etc.
    // so you can test tyty >= TYInt8 to see if *units must be processed.
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
    case TYReal16:
        return "%g";
    case TYReal32:
        return "%g";
    case TYReal64: // Numbers start out with Real64 by default
        return "%g";
    }
}

unsigned int TypeType_nativeSizeForType(TypeTypes tyty)
{
    switch (tyty) {
    case TYUnresolved:
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
    case TYReal16:
        return 2;
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
// but self is needed as long as there is ANY type annotation somewhere.
// (e.g. func args)
TypeTypes TypeType_TypeTypeforSpec(const char* spec)
{
    // does NOT use strncmp!
    if (!spec) return TYUnresolved;
    if (!strcmp(spec, "Scalar"))
        return TYReal64; // self is default, analysis might change it to
                         // more specific
    if (!strcmp(spec, "String")) return TYReal64;
    if (!strcmp(spec, "Logical")) return TYBool;
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

#pragma mark - Stack

// template <class T, int initialSize = 8>
// TODO: COMBINE STACK AND POOL!! THEY ARE THE SAME THING KIND OF
typedef struct Stack {
    void** ref;
    uint32_t used;
    uint32_t cap;
} Stack;

// public:
// T& operator[](int index) { return items[index]; }
void Stack_free(Stack* self)
{
    if (self->cap) free(self->ref);
}

void Stack_growTo(Stack* self, size_t size)
{
    self->ref = realloc(self->ref, sizeof(void*) * size);
}

void Stack_push(Stack* self, void* node)
{
    if (self->used < self->cap) {
        self->ref[self->used++] = node;
    } else {
        self->cap = self->cap ? 2 * self->cap : 8;
        self->ref = realloc(self->ref, sizeof(void*) * self->cap);
        // !! realloc can NULL the ptr!
        self->ref[self->used++] = node;
        for (int i = self->used; i < self->cap; i++)
            self->ref[i] = NULL;
        // DON't do self, either do nothing or memset
    }
}

void* Stack_pop(Stack* self)
{
    void* ret = NULL;
    if (self->used) {
        ret = self->ref[self->used - 1];
        self->ref[self->used - 1] = NULL;
        self->used--;
    } else {
        printf("error: pop from empty list\n");
    }
    return ret;
}

void* Stack_top(Stack* self)
{
    return self->used ? self->ref[self->used - 1] : NULL;
}
#define Stack_topAs(T, self) ((T)Stack_top(self))

bool Stack_empty(Stack* self) { return self->used == 0; }

#pragma mark - Pool

#define stat(T)                                                            \
    fprintf(stderr, "*** %-24s %4ld B x %5d = %7ld B\n", #T, sizeof(T),    \
        T##_allocTotal, T##_allocTotal * sizeof(T));

#define MKSTAT(T) static int T##_allocTotal = 0;
MKSTAT(ASTExpr)
MKSTAT(ASTFunc)
MKSTAT(ASTTypeSpec)
MKSTAT(ASTType)
MKSTAT(ASTModule)
MKSTAT(ASTScope)
MKSTAT(ASTImport)
MKSTAT(ASTVar) MKSTAT(Parser) MKSTAT(PtrList) MKSTAT(List_ASTExpr)
    MKSTAT(List_ASTFunc) MKSTAT(List_ASTTypeSpec) MKSTAT(List_ASTType)
        MKSTAT(List_ASTModule) MKSTAT(List_ASTScope) MKSTAT(List_ASTImport)
            MKSTAT(List_ASTVar)

#define min(a, b) ((a) < (b)) ? (a) : (b)
#define KB *1024

                typedef struct PoolB {
    //    int32_t sizePerPool = 0;
    void* ref;
    uint32_t cap; // used = 0;
    //    Stack<void*, 32> ptrs;

    uint32_t used;
} PoolB;

void* PoolB_alloc(PoolB* self, size_t reqd)
{
    void* ans = NULL;

    // This is a pool for single objects, not arrays or large strings.
    // dont ask for a big fat chunk larger than 16KB (or up to 256KB
    // depending on how much is already there) all at one time.
    if (self->used + reqd > self->cap) {
        //            if (ref) Stack_push(&ptrs, ref);
        self->cap += (self->cap > 64 KB ? 256 KB : 16 KB);
        self->ref = realloc(self->ref, self->cap);
        assert(self->ref != NULL);
        //            pos = 0;
        //            globalMemAllocBytes += sizePerPool;
    }
    //        count++;
    //        used += size;
    ans = (void*)((uintptr_t)self->ref + self->used);
    self->used += reqd;
    if (((uintptr_t)ans / 8) * 8 != (uintptr_t)ans) printf("");
    return ans;
}

void* PoolB_deref(PoolB* self, uint32_t ptr)
{
    // ptr is in steps of sizeof(void*), not in steps of any sizeof(T*)
    return (void*)((uintptr_t)self->ref + ptr);
}

void PoolB_free(PoolB* self)
{
    if (self->cap) free(self->ref);
}

PoolB globalPool;

#define NEW(T) PoolB_alloc(&globalPool, sizeof(T))

#define JOIN(x, y) x##y

#define NAME_CLASS(T) const char* JOIN(T, _typeName) = #T;

#pragma mark - List

#define List(T) PtrList
typedef struct PtrList {
    void** item;
    struct PtrList* next;
} PtrList;

PtrList* PtrList_with(void* item)
{
    PtrList* li = NEW(PtrList);
    li->item = item;
    return li;
}

void PtrList_append(PtrList** selfp, void* item)
{
    if (*selfp == NULL) // first append call
        *selfp = PtrList_with(item);
    else {
        PtrList* self = *selfp;
        while (self->next)
            self = self->next;
        self->next = PtrList_with(item);
    }
}

#define foreach(T, var, listp, listSrc)                                    \
    PtrList* listp = listSrc;                                              \
    for (T var = (T)listp->item; listp && (var = (T)listp->item);          \
         listp = listp->next)

#pragma mark - Token Kinds

typedef enum TokenKind {
    TKNullChar,
    TKKeyword_cheater,
    TKKeyword_for,
    TKKeyword_while,
    TKKeyword_if,
    TKKeyword_end,
    TKKeyword_function,
    TKKeyword_declare,
    TKKeyword_test,
    TKKeyword_not,
    TKKeyword_and,
    TKKeyword_or,
    TKKeyword_in,
    TKKeyword_do,
    TKKeyword_then,
    TKKeyword_as,
    TKKeyword_else,
    TKKeyword_type,
    TKKeyword_return,
    TKKeyword_returns,
    TKKeyword_extends,
    TKKeyword_var,
    TKKeyword_let,
    TKKeyword_import,
    TKIdentifier,
    TKFunctionCall,
    TKSubscript,
    TKNumber, // the number is still stored as a string by default!
    TKIdentifierResolved, // points to ASTVar instead of a string name
    TKFunctionCallResolved, // points to ASTFunction
    TKSubscriptResolved, // points to ASTVar
    TKNumberAsInt, // stores int instead of number as string
    TKNumberAsDbl, // stores double
    TKNumberAsUInt, // stores uint
    TKMultiDotNumber,
    TKSpaces,
    TKOneSpace, // exactly 1 space
    TKTab,
    TKNewline,
    TKLineComment,
    TKAlphabet,
    TKAmpersand,
    TKArrayClose, // ]
    // TKArray_empty, // []
    TKArrayOpen, //  [
    TKArrayDims, // [:,:,:]
    TKAt,
    TKBraceClose,
    // TKBrace_empty,
    TKBraceOpen,
    TKDigit,
    TKHash,
    TKExclamation,
    TKPipe,
    TKOpAssign,
    TKVarAssign, // self is an expr that is made at the point of a var decl
    TKOpEQ,
    TKOpNE,
    TKOpGE,
    TKOpGT,
    TKOpLE,
    TKOpLT,
    TKOpMod,
    TKOpResults,
    TKOpNotResults,
    TKParenClose,
    TKParenOpen,
    TKPeriod,
    TKOpComma,
    TKOpSemiColon,
    TKOpColon,
    TKStringBoundary, // "
    // TKStr_empty, // ""
    TKString, // "string"
    TKRegexBoundary, // '
    // TKRgx_empty, // ''
    TKRegex, // '[a-zA-Z0-9]+'
    TKInlineBoundary, // `
    // TKInl_empty,
    TKInline, // `something`
    TKUnderscore,
    TKSlash,
    TKBackslash,
    TKPlus,
    TKMinus,
    TKUnaryMinus,
    TKTimes,
    TKPower,
    TKTilde,
    TKDollar,
    TKUnits,
    TKUnknown,
    TKPlusEq,
    TKMinusEq,
    TKSlashEq,
    TKTimesEq,
    TKColEq,
    TKQuestion
} TokenKind;

// Return the repr of a self->token kind (for debug)
const char* TokenKind_repr(const TokenKind kind, bool spacing)
{
    switch (kind) {
    case TKNullChar:
        return "EOF";
    case TKKeyword_cheater:
        return "cheater";
    case TKKeyword_for:
        return "for";
    case TKKeyword_while:
        return "while";
    case TKKeyword_if:
        return "if";
    case TKKeyword_then:
        return "then";
    case TKKeyword_as:
        return "as";
    case TKKeyword_end:
        return "end";
    case TKKeyword_function:
        return "function";
    case TKKeyword_declare:
        return "declare";
    case TKKeyword_test:
        return "test";
    case TKKeyword_not:
        return "not ";
    case TKKeyword_and:
        return " and ";
    case TKKeyword_or:
        return " or ";
    case TKKeyword_in:
        return " in ";
    case TKKeyword_else:
        return "else";
    case TKKeyword_type:
        return "type";
    case TKKeyword_extends:
        return "extends";
    case TKKeyword_var:
        return "var";
    case TKKeyword_let:
        return "let";
    case TKKeyword_import:
        return "import";
    case TKKeyword_return:
        return "return ";
    case TKKeyword_returns:
        return "returns";
    case TKIdentifier:
        return "id";
    case TKFunctionCall:
        return "f()";
    case TKSubscript:
        return "a[]";
    case TKNumber:
        return "num";
    case TKMultiDotNumber:
        return "1.2.3.4";
    case TKSpaces:
        return "spc";
    case TKTab:
        return "(tab)";
    case TKNewline:
        return "nl";
    case TKLineComment:
        return spacing ? "# " : "#";
    case TKAmpersand:
        return "&";
    case TKDigit:
        return "1";
    case TKPower:
        return "^";
    case TKUnits:
        return "|kg";
    case TKAlphabet:
        return "a";
    case TKArrayClose:
        return "]";
    case TKArrayOpen:
        return "[";
    case TKArrayDims:
        return "[:]";
    case TKAt:
        return "@";
    case TKBraceClose:
        return "}";
    case TKBraceOpen:
        return "{";
    case TKHash:
        return "#";
    case TKExclamation:
        return "!";
    case TKPipe:
        return "|";
    case TKOpAssign:
    case TKVarAssign:
        return spacing ? " = " : "=";
    case TKOpEQ:
        return spacing ? " == " : "==";
    case TKOpNE:
        return spacing ? " != " : "!=";
    case TKOpGE:
        return spacing ? " >= " : ">=";
    case TKOpGT:
        return spacing ? " > " : ">";
    case TKOpLE:
        return spacing ? " <= " : "<=";
    case TKOpLT:
        return spacing ? " < " : "<";
    case TKOpMod:
        return spacing ? " % " : "%";
    case TKOpResults:
        return " => ";
    case TKOpNotResults:
        return " =/> ";
    case TKParenClose:
        return ")";
    case TKParenOpen:
        return "(";
    case TKPeriod:
        return ".";
    case TKOpComma:
        return ", ";
    case TKOpColon:
        return ":";
    case TKOpSemiColon:
        return "; ";
    case TKStringBoundary:
        return "\"";
    case TKString:
        return "str";
    case TKRegexBoundary:
        return "'";
    case TKRegex:
        return "rgx";
    case TKInlineBoundary:
        return "`";
    case TKInline:
        return "`..`";
    case TKUnderscore:
        return "_";
    case TKSlash:
        return spacing ? " / " : "/";
    case TKTilde:
        return spacing ? " ~ " : "~";
    case TKBackslash:
        return "\\";
    case TKPlus:
        return spacing ? " + " : "+";
    case TKMinus:
        return spacing ? " - " : "-";
    case TKTimes:
        return spacing ? " * " : "*";
    case TKDollar:
        return "$";
    case TKUnknown:
        return "(?)";
    case TKKeyword_do:
        return "do";
    case TKColEq:
        return spacing ? " := " : ":=";
    case TKPlusEq:
        return spacing ? " += " : "+=";
    case TKMinusEq:
        return spacing ? " -= " : "-=";
    case TKTimesEq:
        return spacing ? " *= " : "*=";
    case TKSlashEq:
        return spacing ? " /= " : "/=";
    case TKOneSpace:
        return "(sp1)";
    case TKQuestion:
        return "?";
    case TKUnaryMinus:
        return "-"; // the previous op will provide spacing
    case TKFunctionCallResolved:
        return "f()";
    case TKIdentifierResolved:
        return "id";
    case TKSubscriptResolved:
        return "a[]";
    case TKNumberAsDbl:
        return "0";
    case TKNumberAsUInt:
        return "0";
    case TKNumberAsInt:
        return "0";
    }
    printf("unknown kind: %d\n", kind);
    return "(!unk)";
}

// Return the repr of a self->token kind (for debug)
const char* TokenKind_defaultType(const TokenKind kind)
{
    switch (kind) {
    case TKKeyword_not:
    case TKKeyword_and:
    case TKKeyword_or:
    case TKKeyword_in:
    case TKOpEQ:
    case TKOpNE:
    case TKOpGE:
    case TKOpGT:
    case TKOpLE:
    case TKOpLT:
    case TKOpResults:
    case TKOpNotResults:
        return "Boolean";

    case TKPower:
    case TKOpMod:
    case TKSlash:
    case TKPlus:
    case TKMinus:
    case TKTimes:
    case TKUnaryMinus:
    case TKDigit:
    case TKNumber:
        return "Scalar";

    case TKMultiDotNumber:
        return "IPv4";

    case TKArrayOpen:
        return "[";
    case TKArrayDims:
        return "[:]";
    case TKBraceOpen:
        return "{";

    case TKSubscript:
        return "Slice"; // not if index resolves to 1 scalar, then it's well
                        // Scalar

    case TKParenClose:
        return ")";
    case TKParenOpen:
        return "(";
    case TKPeriod:
        return ".";
    case TKOpComma:
        return ", ";
    case TKOpColon:
        return "Range";
    case TKOpSemiColon:
        return "Tensor";
    case TKStringBoundary:
    case TKString:
    case TKRegexBoundary:
    case TKRegex:
    case TKInlineBoundary:
    case TKInline:
        return "String";
    default:
        return "UnknownType";
    }
    //    printf("unknown kind: %d\n", kind);
}

bool TokenKind_isUnary(TokenKind kind)
{
    return kind == TKKeyword_not or kind == TKUnaryMinus
        or kind == TKKeyword_return or kind == TKArrayOpen;
}

bool TokenKind_isRightAssociative(TokenKind kind)
{
    return kind == TKPeriod or kind == TKPower;
}

uint8_t TokenKind_getPrecedence(TokenKind kind)
{ // if templateStr then precedence of < and > should be 0
    switch (kind) {
    case TKUnaryMinus:
        return 105;
    case TKPeriod:
        return 90;
    case TKPipe:
        return 80;
    case TKPower:
        return 70;
    case TKTimes:
    case TKSlash:
    case TKOpMod:
        return 60;
    case TKPlus:
    case TKMinus:
        return 50;
    case TKOpColon:
        return 45;
    case TKOpLE:
    case TKOpLT:
    case TKOpGT:
    case TKOpGE:
    case TKKeyword_in:
        return 41;
    case TKOpEQ:
    case TKTilde: // regex match op e.g. sIdent ~ '[a-zA-Z_][a-zA-Z0-9_]'
    case TKOpNE:
        return 40;
    case TKKeyword_not:
        return 32;
    case TKKeyword_and:
        return 31;
    case TKKeyword_or:
        return 30;
    case TKKeyword_return:
        return 25;
    case TKOpAssign:
        return 22;
    case TKPlusEq:
    case TKColEq:
    case TKMinusEq:
    case TKTimesEq:
    case TKSlashEq:
        return 20;
    case TKOpComma:
        return 10;
    case TKOpSemiColon:
        return 9;
    case TKKeyword_do:
        return 5;
    case TKArrayOpen:
        return 1;
    default:
        return 0;
    }
}

TokenKind TokenKind_reverseBracket(TokenKind kind)
{
    switch (kind) {
    case TKArrayOpen:
        return TKArrayClose;
    case TKParenOpen:
        return TKParenClose;
    case TKBraceOpen:
        return TKBraceClose;
    case TKArrayClose:
        return TKArrayOpen;
    case TKBraceClose:
        return TKBraceOpen;
    case TKParenClose:
        return TKParenOpen;
    default:
        printf("unexpected at %s:%d\n", __FILE__, __LINE__);
        return TKUnknown;
    }
}

#pragma mark - Token

const uint8_t TokenKindTable[256] = {
    /* 0 */ TKNullChar, /* 1 */ TKUnknown, /* 2 */ TKUnknown,
    /* 3 */ TKUnknown, /* 4 */ TKUnknown, /* 5 */ TKUnknown,
    /* 6 */ TKUnknown, /* 7 */ TKUnknown, /* 8 */ TKUnknown,
    /* 9 */ TKUnknown, /* 10 */ TKNewline, /* 11 */ TKUnknown,
    /* 12 */ TKUnknown, /* 13 */ TKUnknown, /* 14 */ TKUnknown,
    /* 15 */ TKUnknown, /* 16 */ TKUnknown, /* 17 */ TKUnknown,
    /* 18 */ TKUnknown, /* 19 */ TKUnknown, /* 20 */ TKUnknown,
    /* 21 */ TKUnknown, /* 22 */ TKUnknown, /* 23 */ TKUnknown,
    /* 24 */ TKUnknown, /* 25 */ TKUnknown, /* 26 */ TKUnknown,
    /* 27 */ TKUnknown, /* 28 */ TKUnknown, /* 29 */ TKUnknown,
    /* 30 */ TKUnknown, /* 31 */ TKUnknown,
    /* 32   */ TKSpaces, /* 33 ! */ TKExclamation,
    /* 34 " */ TKStringBoundary, /* 35 # */ TKHash, /* 36 $ */ TKDollar,
    /* 37 % */ TKOpMod, /* 38 & */ TKAmpersand, /* 39 ' */ TKRegexBoundary,
    /* 40 ( */ TKParenOpen, /* 41 ) */ TKParenClose, /* 42 * */ TKTimes,
    /* 43 + */ TKPlus, /* 44 , */ TKOpComma, /* 45 - */ TKMinus,
    /* 46 . */ TKPeriod, /* 47 / */ TKSlash, /* 48 0 */ TKDigit,
    /* 49 1 */ TKDigit, /* 50 2 */ TKDigit, /* 51 3 */ TKDigit,
    /* 52 4 */ TKDigit, /* 53 5 */ TKDigit, /* 54 6 */ TKDigit,
    /* 55 7 */ TKDigit, /* 56 8 */ TKDigit, /* 57 9 */ TKDigit,
    /* 58 : */ TKOpColon, /* 59 ; */ TKOpSemiColon, /* 60 < */ TKOpLT,
    /* 61 = */ TKOpAssign, /* 62 > */ TKOpGT, /* 63 ? */ TKQuestion,
    /* 64 @ */ TKAt,
    /* 65 A */ TKAlphabet, /* 66 B */ TKAlphabet, /* 67 C */ TKAlphabet,
    /* 68 D */ TKAlphabet, /* 69 E */ TKAlphabet, /* 70 F */ TKAlphabet,
    /* 71 G */ TKAlphabet, /* 72 H */ TKAlphabet, /* 73 I */ TKAlphabet,
    /* 74 J */ TKAlphabet, /* 75 K */ TKAlphabet, /* 76 L */ TKAlphabet,
    /* 77 M */ TKAlphabet, /* 78 N */ TKAlphabet, /* 79 O */ TKAlphabet,
    /* 80 P */ TKAlphabet, /* 81 Q */ TKAlphabet, /* 82 R */ TKAlphabet,
    /* 83 S */ TKAlphabet, /* 84 T */ TKAlphabet, /* 85 U */ TKAlphabet,
    /* 86 V */ TKAlphabet, /* 87 W */ TKAlphabet, /* 88 X */ TKAlphabet,
    /* 89 Y */ TKAlphabet, /* 90 Z */ TKAlphabet,
    /* 91 [ */ TKArrayOpen, /* 92 \ */ TKBackslash, /* 93 ] */ TKArrayClose,
    /* 94 ^ */ TKPower, /* 95 _ */ TKUnderscore,
    /* 96 ` */ TKInlineBoundary,
    /* 97 a */ TKAlphabet, /* 98 b */ TKAlphabet, /* 99 c */ TKAlphabet,
    /* 100 d */ TKAlphabet, /* 101 e */ TKAlphabet, /* 102 f */ TKAlphabet,
    /* 103 g */ TKAlphabet, /* 104 h */ TKAlphabet, /* 105 i */ TKAlphabet,
    /* 106 j */ TKAlphabet, /* 107 k */ TKAlphabet, /* 108 l */ TKAlphabet,
    /* 109 m */ TKAlphabet, /* 110 n */ TKAlphabet, /* 111 o */ TKAlphabet,
    /* 112 p */ TKAlphabet, /* 113 q */ TKAlphabet, /* 114 r */ TKAlphabet,
    /* 115 s */ TKAlphabet, /* 116 t */ TKAlphabet, /* 117 u */ TKAlphabet,
    /* 118 v */ TKAlphabet, /* 119 w */ TKAlphabet, /* 120 x */ TKAlphabet,
    /* 121 y */ TKAlphabet, /* 122 z */ TKAlphabet,
    /* 123 { */ TKBraceOpen, /* 124 | */ TKPipe, /* 125 } */ TKBraceClose,
    /* 126 ~ */ TKTilde,
    /* 127 */ TKUnknown, /* 128 */ TKUnknown, /* 129 */ TKUnknown,
    /* 130 */ TKUnknown, /* 131 */ TKUnknown, /* 132 */ TKUnknown,
    /* 133 */ TKUnknown, /* 134 */ TKUnknown, /* 135 */ TKUnknown,
    /* 136 */ TKUnknown, /* 137 */ TKUnknown, /* 138 */ TKUnknown,
    /* 139 */ TKUnknown, /* 140 */ TKUnknown, /* 141 */ TKUnknown,
    /* 142 */ TKUnknown, /* 143 */ TKUnknown, /* 144 */ TKUnknown,
    /* 145 */ TKUnknown, /* 146 */ TKUnknown, /* 147 */ TKUnknown,
    /* 148 */ TKUnknown, /* 149 */ TKUnknown, /* 150 */ TKUnknown,
    /* 151 */ TKUnknown, /* 152 */ TKUnknown, /* 153 */ TKUnknown,
    /* 154 */ TKUnknown, /* 155 */ TKUnknown, /* 156 */ TKUnknown,
    /* 157 */ TKUnknown, /* 158 */ TKUnknown, /* 159 */ TKUnknown,
    /* 160 */ TKUnknown, /* 161 */ TKUnknown, /* 162 */ TKUnknown,
    /* 163 */ TKUnknown, /* 164 */ TKUnknown, /* 165 */ TKUnknown,
    /* 166 */ TKUnknown, /* 167 */ TKUnknown, /* 168 */ TKUnknown,
    /* 169 */ TKUnknown, /* 170 */ TKUnknown, /* 171 */ TKUnknown,
    /* 172 */ TKUnknown, /* 173 */ TKUnknown, /* 174 */ TKUnknown,
    /* 175 */ TKUnknown, /* 176 */ TKUnknown, /* 177 */ TKUnknown,
    /* 178 */ TKUnknown, /* 179 */ TKUnknown, /* 180 */ TKUnknown,
    /* 181 */ TKUnknown, /* 182 */ TKUnknown, /* 183 */ TKUnknown,
    /* 184 */ TKUnknown, /* 185 */ TKUnknown, /* 186 */ TKUnknown,
    /* 187 */ TKUnknown, /* 188 */ TKUnknown, /* 189 */ TKUnknown,
    /* 190 */ TKUnknown, /* 191 */ TKUnknown, /* 192 */ TKUnknown,
    /* 193 */ TKUnknown, /* 194 */ TKUnknown, /* 195 */ TKUnknown,
    /* 196 */ TKUnknown, /* 197 */ TKUnknown, /* 198 */ TKUnknown,
    /* 199 */ TKUnknown, /* 200 */ TKUnknown, /* 201 */ TKUnknown,
    /* 202 */ TKUnknown, /* 203 */ TKUnknown, /* 204 */ TKUnknown,
    /* 205 */ TKUnknown, /* 206 */ TKUnknown, /* 207 */ TKUnknown,
    /* 208 */ TKUnknown, /* 209 */ TKUnknown, /* 210 */ TKUnknown,
    /* 211 */ TKUnknown, /* 212 */ TKUnknown, /* 213 */ TKUnknown,
    /* 214 */ TKUnknown, /* 215 */ TKUnknown, /* 216 */ TKUnknown,
    /* 217 */ TKUnknown, /* 218 */ TKUnknown, /* 219 */ TKUnknown,
    /* 220 */ TKUnknown, /* 221 */ TKUnknown, /* 222 */ TKUnknown,
    /* 223 */ TKUnknown, /* 224 */ TKUnknown, /* 225 */ TKUnknown,
    /* 226 */ TKUnknown, /* 227 */ TKUnknown, /* 228 */ TKUnknown,
    /* 229 */ TKUnknown, /* 230 */ TKUnknown, /* 231 */ TKUnknown,
    /* 232 */ TKUnknown, /* 233 */ TKUnknown, /* 234 */ TKUnknown,
    /* 235 */ TKUnknown, /* 236 */ TKUnknown, /* 237 */ TKUnknown,
    /* 238 */ TKUnknown, /* 239 */ TKUnknown, /* 240 */ TKUnknown,
    /* 241 */ TKUnknown, /* 242 */ TKUnknown, /* 243 */ TKUnknown,
    /* 244 */ TKUnknown, /* 245 */ TKUnknown, /* 246 */ TKUnknown,
    /* 247 */ TKUnknown, /* 248 */ TKUnknown, /* 249 */ TKUnknown,
    /* 250 */ TKUnknown, /* 251 */ TKUnknown, /* 252 */ TKUnknown,
    /* 253 */ TKUnknown, /* 254 */ TKUnknown, /* 255 */ TKUnknown
};
#define Token_matchesKeyword(tok)                                          \
    if (sizeof(#tok) - 1 == l and not strncmp(#tok, s, l)) return true;

bool doesKeywordMatch(const char* s, const int l)
{
    //        const char* s = pos;
    //        const int l = matchlen;

    Token_matchesKeyword(and)
    Token_matchesKeyword(cheater)
    Token_matchesKeyword(for)
    Token_matchesKeyword(do)
    Token_matchesKeyword(while)
    Token_matchesKeyword(if)
    Token_matchesKeyword(then)
    Token_matchesKeyword(end)
    Token_matchesKeyword(function)
    Token_matchesKeyword(declare)
    Token_matchesKeyword(test)
    Token_matchesKeyword(not)
    Token_matchesKeyword(and)
    Token_matchesKeyword(or)
    Token_matchesKeyword(in)
    Token_matchesKeyword(else)
    Token_matchesKeyword(type)
    //    matchesen_compareKeyword(check)
    Token_matchesKeyword(extends)
    Token_matchesKeyword(var)
    Token_matchesKeyword(let)
    Token_matchesKeyword(import)
    Token_matchesKeyword(return)
    Token_matchesKeyword(returns)
    Token_matchesKeyword(as)
    return false;
}

// Holds information about a syntax self->token.
typedef struct Token {

    char* pos;
    uint32_t matchlen : 24;
    struct {
        bool skipWhiteSpace : 1,
            mergeArrayDims : 1, // merge [:,:,:] into one self->token
            noKeywosrdDetect : 1, // leave keywords as idents
            strictSpacing : 1; // missing spacing around operators etc. is a
                               // compile error YES YOU HEARD IT RIGHT
                               // but why need self AND skipWhiteSpace?
    } flags;
    uint16_t line;
    uint8_t col;
    TokenKind kind : 8;
} Token;
// Token()
//     : kind(TKUnknown)
//     , matchlen(0)
// {
// }
// const char* repr() { return TokenKind_repr(kind); }

// Advance the self->token position by one char.
// inline void advance1() { pos++; }

// Peek at the char after the current (complete) self->token
char Token_peekCharAfter(Token* self)
{
    char* s = self->pos + self->matchlen;
    if (self->flags.skipWhiteSpace)
        while (*s == ' ')
            s++;
    return *s;
}

#define Token_compareKeyword(tok)                                          \
    if (sizeof(#tok) - 1 == l and not strncmp(#tok, s, l)) {               \
        self->kind = TKKeyword_##tok;                                      \
        return;                                                            \
    }

// Check if an (ident) self->token matches a keyword and return its type
// accordingly.
void Token_tryKeywordMatch(Token* self)
{
    if (/*flags.noKeywordDetect or */ self->kind != TKIdentifier) return;

    const char* s = self->pos;
    const int l = self->matchlen;

        Token_compareKeyword(and)
        Token_compareKeyword(cheater)
        Token_compareKeyword(for)
        Token_compareKeyword(do)
        Token_compareKeyword(while)
        Token_compareKeyword(if)
        Token_compareKeyword(then)
        Token_compareKeyword(end)
        Token_compareKeyword(function)
        Token_compareKeyword(declare)
        Token_compareKeyword(test)
        Token_compareKeyword(not)
        Token_compareKeyword(and)
        Token_compareKeyword(or)
        Token_compareKeyword(in)
        Token_compareKeyword(else)
        Token_compareKeyword(type)
//        Token_compareKeyword(check)
        Token_compareKeyword(extends)
        Token_compareKeyword(var)
        Token_compareKeyword(let)
        Token_compareKeyword(import)
        Token_compareKeyword(return)
        Token_compareKeyword(returns)
        Token_compareKeyword(as)
        //        Token_compareKeyword(print);
}

// Get the type based on the char at the current position.
// inline TokenKind getTypeAtCurrentPos() { return Token_getType(0); }

// Get the type based on the char after the current position (don't
// conflate self with char* Token_peekCharAfter(self->token))
// inline TokenKind getTypeAtNextChar() { return getType(1); }
//    inline TokenKind peek_prevchar() { return getType(-1); }

// Get the self->token kind based only on the char at the current position
// (or an offset).
TokenKind Token_getType(Token* self, const size_t offset)
{
    const char c = self->pos[offset];
    const char cn = c ? self->pos[1 + offset] : 0;
    TokenKind ret = (TokenKind)TokenKindTable[c];
    switch (c) {
    case '<':
        switch (cn) {
        case '=':
            return TKOpLE;
        default:
            return TKOpLT;
        }
    case '>':
        switch (cn) {
        case '=':
            return TKOpGE;
        default:
            return TKOpGT;
        }
    case '=':
        switch (cn) {
        case '=':
            return TKOpEQ;
        case '>':
            return TKOpResults;
        default:
            return TKOpAssign;
        }
    case '!':
        switch (cn) {
        case '=':
            return TKOpNE;
        }
        return TKExclamation;
    case ':':
        switch (cn) {
        case '=':
            return TKColEq;
        default:
            return TKOpColon;
        }
    default:
        return ret;
    }
}

void Token_detect(Token* self)
{
    TokenKind tt = Token_getType(self,0);
    TokenKind tt_ret = TKUnknown; // = tt;
    static TokenKind tt_last
        = TKUnknown; // the previous self->token that was found
    static TokenKind tt_lastNonSpace
        = TKUnknown; // the last non-space self->token found
    TokenKind tmp;
    char* start = self->pos;
    bool found_e = false, found_dot = false, found_cmt = false;
    uint8_t found_spc = 0;

    switch (tt) {
    case TKStringBoundary:
    case TKInlineBoundary:
    case TKRegexBoundary:
        tmp = tt; // remember which it is exactly

        // Incrementing pos is a side effect of getTypeAtCurrentPos(...)
        while (tt != TKNullChar) {
            // here we want to consume the ending " so we move next
            // before
            self->pos++;
            tt = Token_getType(self,0);
            if (tt == TKNullChar or tt == tmp) {
                self->pos++;
                break;
            }
            if (tt == TKBackslash)
                if (Token_getType(self,1) == tmp) { // why if?
                    self->pos++;
                }
        }
        switch (tmp) {
        case TKStringBoundary:
            tt_ret = TKString;
            break;
        case TKInlineBoundary:
            tt_ret = TKInline;
            break;
        case TKRegexBoundary:
            tt_ret = TKRegex;
            break;
        default:
            tt_ret = TKUnknown;
            printf("unreachable %s:%d\n", __FILE__, __LINE__);
        }
        break;

    case TKSpaces:
        if (tt_last == TKOneSpace) // if prev char was a space return
                                   // self as a run of spaces
            while (tt != TKNullChar) {
                // here we dont want to consume the end char, so break
                // before
                tt = Token_getType(self,1);
                self->pos++;
                if (tt != TKSpaces) break;
            }
        else
            self->pos++;
        // else its a single space
        tt_ret = TKSpaces;
        break;

    case TKOpComma:
    case TKOpSemiColon:
        //        line continuation tokens
        tt_ret = tt;

        while (tt != TKNullChar) {
            tt = Token_getType(self,1);
            self->pos++;
            // line number should be incremented for line continuations
            if (tt == TKSpaces) {
                found_spc++;
            }
            if (tt == TKExclamation) {
                found_cmt = true;
            }
            if (tt == TKNewline) {
                self->line++;
                self->col = -found_spc - 1; // account for extra spaces
                                            // after , and for nl itself
                found_spc = 0;
            }
            if (found_cmt and tt != TKNewline) {
                found_spc++;
                continue;
            }
            if (tt != TKSpaces and tt != TKNewline) break;
        }
        break;

    case TKArrayOpen:
        // mergearraydims should be set only when reading func args
        if (not self->flags.mergeArrayDims) goto defaultToken;

        while (tt != TKNullChar) {
            tt = Token_getType(self,1);
            self->pos++;
            if (tt != TKOpColon and tt != TKOpComma) break;
        }
        tt = Token_getType(self,0);
        if (tt != TKArrayClose) {
            fprintf(stderr, "expected a ']', found a '%c'. now what?\n",
                *self->pos);
        }
        self->pos++;
        tt_ret = TKArrayDims;
        break;

    case TKAlphabet:
    case TKPeriod:
    case TKUnderscore:
        while (tt != TKNullChar) {
            tt = Token_getType(self,1);
            self->pos++;
            if (tt != TKAlphabet and tt != TKDigit and tt != TKUnderscore
                and tt != TKPeriod)
                break; /// validate in parser not here
        }
        tt_ret = TKIdentifier;
        break;

    case TKHash: // TKExclamation:
        while (tt != TKNullChar) {
            tt = Token_getType(self,1);
            self->pos++;
            if (tt == TKNewline) break;
        }
        tt_ret = TKLineComment;
        break;

    case TKPipe:
        while (tt != TKNullChar) {
            tt = Token_getType(self,1);
            self->pos++;
            if (tt != TKAlphabet and tt != TKDigit and tt != TKSlash
                and tt != TKPeriod)
                break;
        }
        tt_ret = TKUnits;
        break;

    case TKDigit:
        tt_ret = TKNumber;

        while (tt != TKNullChar) // EOF, basically null char
        {
            tt = Token_getType(self,1);
            // numbers such as 1234500.00 are allowed
            // very crude, error-checking is parser's job
            self->pos++;

            if (*self->pos == 'e' or *self->pos == 'E' or *self->pos == 'd'
                or *self->pos == 'D') { // will all be changed to e btw
                found_e = true;
                continue;
            }
            if (found_e) {
                found_e = false;
                continue;
            }
            if (tt == TKPeriod) {
                found_dot = true;
                continue;
            }
            if (found_dot and tt == TKPeriod) tt_ret = TKMultiDotNumber;

            if (tt != TKDigit and tt != TKPeriod and *self->pos != 'i')
                break;
        }
        break;

    case TKMinus:

        switch (tt_lastNonSpace) {
        case TKParenClose:
        case TKIdentifier: // keywords too?
        case TKNumber:
        case TKArrayClose:
        case TKArrayDims:
        case TKMultiDotNumber:
            tt_ret = tt;
            break;
        default:
            tt_ret = TKUnaryMinus;
            break;
        }
        self->pos++;
        break;

    case TKOpNotResults:
        // 3-char tokens
        self->pos++;
    case TKOpEQ:
    case TKOpGE:
    case TKOpLE:
    case TKOpNE:
    case TKOpResults:
    case TKBackslash:
    case TKColEq:
        // 2-char tokens
        self->pos++;
    default:
    defaultToken:
        tt_ret = tt;
        self->pos++;
        break;
    }

    self->matchlen = (uint32_t)(self->pos - start);
    self->pos = start;
    self->kind = tt_ret;

    if (self->kind == TKIdentifier) Token_tryKeywordMatch(self);

    if (self->kind == TKSpaces and self->matchlen == 1)
        self->kind = TKOneSpace;

    tt_last = self->kind;
    if (tt_last != TKOneSpace and tt_last != TKSpaces)
        tt_lastNonSpace = tt_last;
}

// Advance to the next self->token (skip whitespace if `skipws` is set).
void Token_advance(Token* self)
{
    switch (self->kind) {
    case TKIdentifier:
    case TKString:
    case TKNumber:
    case TKMultiDotNumber:
    case TKFunctionCall:
    case TKSubscript:
    case TKDigit:
    case TKAlphabet:
    case TKRegex:
    case TKInline:
    case TKUnits:
    case TKKeyword_cheater:
    case TKKeyword_for:
    case TKKeyword_while:
    case TKKeyword_if:
    case TKKeyword_end:
    case TKKeyword_function:
    case TKKeyword_test:
    case TKKeyword_not:
    case TKKeyword_and:
    case TKKeyword_or:
    case TKKeyword_in:
    case TKKeyword_do:
    case TKKeyword_then:
    case TKKeyword_as:
    case TKKeyword_else:
    case TKKeyword_type:
    case TKKeyword_return:
    case TKKeyword_extends:
    case TKKeyword_var:
    case TKKeyword_let:
    case TKKeyword_import:
    case TKUnknown: // bcz start of the file is self
        break;
    default:
        *self->pos = 0; // trample it so that idents etc. can be assigned
                        // in-situ
    }

    self->pos += self->matchlen;
    self->col += self->matchlen;
    self->matchlen = 0;
    Token_detect(self);

    if (self->kind == TKNewline) {
        // WHY don't you do self->token advance here?
        self->line++;
        self->col = 0; // position of the nl itself is 0
    }
    if (self->flags.skipWhiteSpace
        and (self->kind == TKSpaces
            or (self->flags.strictSpacing and self->kind == TKOneSpace)))
        Token_advance(self);
}

const char* const spaces = //
    "                                                                     ";
const char* const dashes = //
    "\n_________________________________________________________________\n";
const char* const equaltos = //
    "\n=================================================================\n";

#pragma mark - AST Import

typedef struct ASTImport { // canbe 8
    // STHEAD(ASTImport, 32)

    char* importFile;
    uint32_t aliasOffset;

    // generally self is not changed (the alias), at least not by the
    // linter. So we can only store the offset from importFile. In general
    // it would be nice to allocate the file contents buffer with a little
    // extra space and use that as a string pool. This way we can make new
    // strings while still referring to them using a uint32.
    bool isPackage, hasAlias;
} ASTImport;

void ASTImport_gen(ASTImport* self, int level)
{
    printf("import %s%s%s%s\n", self->isPackage ? "@" : "",
        self->importFile, self->hasAlias ? " as " : "",
        self->hasAlias ? self->importFile + self->aliasOffset : "");
}
void ASTImport_genc(ASTImport* self, int level)
{
    str_tr_ip(self->importFile, '.', '_', 0);
    printf("\n#include \"%s.h\"\n", self->importFile);
    if (self->hasAlias)
        printf("#define %s %s\n", self->importFile + self->aliasOffset,
            self->importFile);
    str_tr_ip(self->importFile, '_', '.', 0);
}
void ASTImport_undefc(ASTImport* self)
{
    if (self->hasAlias)
        printf("#undef %s\n", self->importFile + self->aliasOffset);
}

#pragma mark - AST Units

typedef struct ASTUnits {

    // STHEAD(ASTUnits, 32)

    uint8_t powers[7], something;
    double factor, factors[7];
    char* label;
    // void gen(int level) { printf("|%s", label); }
} ASTUnits;

struct ASTTypeSpec;
struct ASTType;
struct ASTFunc;
struct ASTScope;
struct ASTExpr;
struct ASTVar;

#pragma mark - AST TypeSpec

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

void ASTTypeSpec_gen(ASTTypeSpec* self, int level)
{
    // if (status == TSUnresolved)
    printf("%s", self->name);
    //        if (status==TSResolved) printf("%s", type->name);
    if (self->dims) printf("%s", "[]" /*dimsGenStr(dims)*/);
    // if (status == TSDimensionedNumber) gen(units, level);
}

#pragma mark - AST Var
static uint32_t exprsAllocHistogram[128];

void expralloc_stat()
{
    int iexpr, sum = 0, thres = 0;
    for (int i = 0; i < 128; i++)
        sum += exprsAllocHistogram[i];
    thres = sum / 20;
    fprintf(stderr,
        "Expr alloc distribution (>5%%):\n  Kind        #      %%\n");
    for (int i = 0; i < 128; i++) {
        if ((iexpr = exprsAllocHistogram[i]) > thres)
            fprintf(stderr, "  %-8s %4d %6.2f\n",
                TokenKind_repr((TokenKind)i, false), iexpr,
                iexpr * 100.0 / sum);
    }
}

typedef struct ASTVar {

    // STHEAD(ASTVar, 128)

    ASTTypeSpec* typeSpec;
    struct ASTExpr* init;
    char* name;

    struct {
        bool unused : 1, //
            unset : 1, //
            isLet : 1, //
            isVar : 1, //
            isTarget : 1;
        /* x = f(x,y) */ //
    } flags;

} ASTVar;

typedef struct ASTExpr {

    // STHEAD(ASTExpr, 1024)
    // self should have at least a TypeTypes if not an ASTType*
    struct {
        uint16_t line;
        union {
            struct {
                bool opIsUnary, opIsRightAssociative;
            };
            uint16_t strLength;
        }; // for literals, the string length
        uint8_t opPrecedence;
        uint8_t col;
        TokenKind kind : 8; // TokenKind must be updated to add
                            // TKFuncCallResolved TKSubscriptResolved etc.
                            // to indicate a resolved identifier. while you
                            // are at it add TKNumberAsString etc. then
                            // instead of name you will use the corr.
                            // object. OR instead of extra enum values add a
                            // bool bit resolved
    };

    struct ASTExpr* left;

    union {
        union {
            char* string;
            double real;
            int64_t integer;
            uint64_t uinteger;
        } value; // for terminals
        char* name; // for unresolved functioncall or subscript, or
                    // identifiers
        struct ASTFunc* func; // for functioncall
        ASTVar* var; // for array subscript, or to refer to a variable, e.g.
                     // in TKVarAssign
        struct ASTScope* body; // for if/for/while
        struct ASTExpr* right;
        /* struct {uint32_t left, right}; // for 32-bit ptrs to local pool
         */
    };
} ASTExpr;

// ASTExpr() {}
ASTExpr* ASTExpr_fromToken(const Token* self)
{
    ASTExpr* ret = NEW(ASTExpr);
    ret->kind = self->kind;
    ret->line = self->line;
    ret->col = self->col;

    ret->opPrecedence = TokenKind_getPrecedence(ret->kind);
    if (ret->opPrecedence) {
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
        ret->strLength = (uint16_t)self->matchlen;
        ret->value.string = self->pos;
        break;
    default:;
    }
    // the '!' will be trampled
    if (ret->kind == TKLineComment) ret->value.string++;
    // turn all 1.0234[DdE]+01 into 1.0234e+01.
    if (ret->kind == TKNumber) {
        str_tr_ip(ret->value.string, 'd', 'e', ret->strLength);
        str_tr_ip(ret->value.string, 'D', 'e', ret->strLength);
        str_tr_ip(ret->value.string, 'E', 'e', ret->strLength);
    }
    return ret;
}

// void gen(int level, bool spacing, bool escapeStrings);
// void genc(int level, bool spacing, bool inFuncArgs, bool escapeStrings);
// void catarglabels(); // helper  for genc

union Value ASTExpr_eval(ASTExpr* self)
{
    // TODO: value is a union of whatever
    union Value v;
    v.d = 0;
    return v;
}

TypeTypes evalType()
{
    // TODO:
    // self func should return union {ASTType*; TypeTypes;} with the
    // TypeTypes value reduced by 1 (since it cannot be unresolved and
    // since 0 LSB would mean valid pointer (to an ASTType)).
    // self func does all type checking too btw and reports errors.
    return TYBool;
}
void ASTExpr_gen(ASTExpr* self, int level, bool spacing, bool escapeStrings)
;
void ASTVar_gen(ASTVar* self, int level)
{
    printf("%.*s%s%s", level, spaces,
        self->flags.isVar ? "var " : self->flags.isLet ? "let " : "",
        self->name);
    if (self->typeSpec) {
        if (!(self->init and self->init->kind == TKFunctionCall
                and !strcmp(self->init->name, self->typeSpec->name))) {
            printf(" as ");
            ASTTypeSpec_gen(self->typeSpec, level + STEP);
        }
    } else {
        // should make self Expr_defaultType and it does it recursively for
        // [, etc
        const char* ctyp = TokenKind_defaultType(
            self->init ? self->init->kind : TKUnknown);
        if (self->init and self->init->kind == TKArrayOpen)
            ctyp = TokenKind_defaultType(
                self->init->right ? self->init->right->kind : TKUnknown);
        if (self->init and self->init->kind == TKFunctionCall
            and *self->init->name >= 'A' and *self->init->name <= 'Z')
            ctyp = NULL;
        if (ctyp) printf(" as %s", ctyp);
    }
    if (self->init) {
        printf(" = ");
        ASTExpr_gen(self->init, 0, true, false);
    }
}
void ASTTypeSpec_genc(ASTTypeSpec* self, int level, bool isconst)
;
void ASTVar_genc(ASTVar* self, int level, bool isconst)
{
    // for C the variable declarations go at the top of the block, without
    // init
    printf("%.*s", level, spaces);
    if (self->typeSpec) {
        ASTTypeSpec_genc(self->typeSpec, level + STEP, isconst);
    } else {
        const char* ctyp = TokenKind_defaultType(
            self->init ? self->init->kind : TKUnknown);
        if (self->init and self->init->kind == TKFunctionCall
            and *self->init->name >= 'A' and *self->init->name <= 'Z')
            ctyp = self->init->name;
        printf("%s", ctyp);
    }
    printf(" %s", self->name);
}

#pragma mark - AST Scope

typedef struct ASTScope {

    // STHEAD(ASTScope, 32)

    List(ASTExpr) * stmts;
    List(ASTVar) * locals;
    struct ASTScope* parent;
    // void gen(int level);
    // void genc(int level);
} ASTScope;
size_t ASTScope_calcSizeUsage(ASTScope* self)
{
    size_t size = 0, sum = 0;
    // all variables must be resolved before calling self, call it e.g.
    // during cgen
    foreach (ASTExpr*, stmt, stmts, self->stmts) {
        switch (stmt->kind) {
        case TKKeyword_if:
        case TKKeyword_for:
        case TKKeyword_while:
            sum += ASTScope_calcSizeUsage(stmt->body);
            break;
        case TKVarAssign:
            //                if (stmt->var->typeSpec) // NO NO NO
            assert(size = TypeType_nativeSizeForType(
                       stmt->var->typeSpec->typeType));
            sum += size;
            break;
        default:;
        }
    }
    return sum;
}

ASTVar* ASTScope_getVar(ASTScope* self, const char* name)
{
    // stupid linear search, no dictionary yet
    foreach (ASTVar*, local, locals, self->locals) {
        if (!strcmp(name, local->name)) return local;
    }
    // in principle self will walk up the scopes, including the
    // function's "own" scope (which has locals = func's args). It
    // doesn't yet reach up to module scope, because there isn't yet a
    // "module" scope.
    if (self->parent) return ASTScope_getVar(self->parent, name);
    return NULL;
}

void ASTScope_gen(ASTScope* self, int level)
{
    foreach (ASTExpr*, stmt, stmts, self->stmts) {
        ASTExpr_gen(stmt, level, true, false);
        puts("");
    }
}
void ASTExpr_genc(ASTExpr* self, int level, bool spacing, bool inFuncArgs,
    bool escapeStrings);

#define genLineNumbers 0
void ASTScope_genc(ASTScope* self, int level)
{
    foreach (ASTVar*, local, locals, self->locals) {
        ASTVar_genc(local, level, false);
        puts(";");
    } // these will be declared at top and defined within the expr list
    foreach (ASTExpr*, stmt, stmts, self->stmts) {
        if (stmt->kind == TKLineComment) continue;
        if (genLineNumbers) printf("#line %d\n", stmt->line);
        ASTExpr_genc(stmt, level, true, false, false);
        puts(";");
        puts("    if (_err_ == ERROR_TRACE) goto backtrace;");
    }
}

#pragma mark - AST Expr

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
    uint8_t kind() { return typ; } // self only takes values 0-7, 3 bits
    //--
    ASTVar* var() { return (ASTVar*)getptr(); }
    ASTExpr* expr() { return (ASTExpr*)getptr(); }
};
*/
#pragma mark - AST Type


typedef struct ASTType {

    // STHEAD(ASTType, 32)

    List(ASTVar) * vars;
    ASTTypeSpec* super;
    char* name;
    // TODO: flags: hasUserInit : whether user has defined Type() ctor

    List(ASTExpr) * checks; // invariants

    List(ASTVar) * params; // params of self type
} ASTType;

ASTVar* ASTType_getVar(ASTType* self, const char* name)
{
    // stupid linear search, no dictionary yet
    foreach (ASTVar*, var, vars, self->vars) {
        if (!strcmp(name, var->name)) return var;
    }
    if (self->super and self->super->typeType == TYObject)
        return ASTType_getVar(self->super->type, name);
    return NULL;
}

void ASTType_gen(ASTType* self, int level)
{
    printf("type %s", self->name);
    if (self->params->item) {
        ; // TODO: figure out what to do about params
    }
    if (self->super) {
        printf(" extends ");
        ASTTypeSpec_gen(self->super, level);
    }
    puts("");

    // TODO: rename to exprs or body. self contains vars also
    foreach (ASTExpr*, check, checks, self->checks) {
        if (!check) continue;
        ASTExpr_gen(check, level + STEP, true, false);
        puts("");
    }

    puts("end type\n");
}

void ASTTypeSpec_genc(ASTTypeSpec* self, int level, bool isconst)
{
    if (isconst) printf("const ");
    if (self->dims) printf("Array%dD(", self->dims);

    switch (self->typeType) {
    case TYObject:
        printf("%s", self->type->name);
        break;
    case TYUnresolved:
        printf("%s", self->name);
        break;
    default:
        printf("%s", TypeType_nativeName(self->typeType));
        break;
    }

    // if (isconst) printf(" const"); // only if a ptr type
    if (self->dims) printf("%s", ")");
    //        if (status == TSDimensionedNumber) {
    //            genc(units, level);
    //        }
}

void ASTType_genc(ASTType* self, int level)
{
    printf("#define FIELDS_%s \\\n", self->name);
    foreach (ASTVar*, var, fvars, self->vars) {
        if (!var) continue;
        ASTVar_genc(var, level + STEP, true);
        printf("; \\\n");
    }
    printf("\n\nstruct %s {\n", self->name);

    if (self->super) {
        printf("    FIELDS_");
        ASTTypeSpec_genc(self->super, level, false);
        printf("\n");
    }

    printf("    FIELDS_%s\n};\n\n", self->name);
    printf(
        "static const char* %s_name_ = \"%s\";\n", self->name, self->name);
    printf("static %s %s_alloc_() {\n    return _Pool_alloc_(&gPool_, "
           "sizeof(struct %s));\n}\n",
        self->name, self->name, self->name);
    printf("static %s %s_init_(%s self) {\n", self->name, self->name,
        self->name);
    // TODO: rename self->checks to self->exprs or self->body
    foreach (ASTVar*, var, vars, self->vars) {
        printf("#define %s self->%s\n", var->name, var->name);
    }
    foreach (ASTExpr*, check, checks, self->checks) {
        if (!check or check->kind != TKVarAssign or !check->var->init)
            continue;
        ASTExpr_genc(check, level + STEP, true, false, false);
        puts(";");
    }
    foreach (ASTVar*, var, mvars, self->vars) {
        printf("#undef %s \n", var->name);
    }

    printf("    return self;\n}\n");
    printf("%s %s_new_() {\n    return "
           "%s_init_(%s_alloc_());\n}\n",
        self->name, self->name, self->name, self->name);
    printf("#define %s_print_ %s_print__(p, STR(p))\n", self->name,
        self->name);
    printf("%s %s_print__(%s self, const char* name) {\n    printf(\"<%s "
           "'%%s' at %%p>\",name,self);\n}\n",
        self->name, self->name, self->name, self->name);

    puts("");
}

void ASTType_genh(ASTType* self, int level)
{
    printf("typedef struct %s* %s; struct %s;\n", self->name, self->name,
        self->name);
}

#pragma mark - AST Func

typedef struct ASTFunc {

    // STHEAD(ASTFunc, 64)

    ASTScope* body;
    List(ASTVar) * args;
    ASTTypeSpec* returnType;
    char* mangledName;
    char* owner; // if method of a type
    char* name;
    struct {
        uint16_t line;
        struct {
            uint16_t io : 1, throws : 1, recurs : 1, net : 1, gui : 1,
                exported : 1, refl : 1, nodispatch : 1, isStmt : 1,
                isDeclare : 1;
        } flags; //= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        uint8_t col; // not needed
    };
} ASTFunc;

size_t ASTFunc_calcSizeUsage(ASTFunc* self)
{
    size_t size = 0, sum = 0;
    foreach (ASTVar*, arg, args, self->args) {
        // means all variables must be resolved before calling self
        assert(size = TypeType_nativeSizeForType(arg->typeSpec->typeType));
        sum += size;
    }
    if (self->body) sum += ASTScope_calcSizeUsage(self->body);
    return sum;
}

void ASTFunc_gen(ASTFunc* self, int level)
{
    printf("function %s(", self->name);

    foreach (ASTVar*, arg, args, self->args) {
        ASTVar_gen(arg, level);
        printf(args->next ? ", " : "");
    }
    printf(")");

    if (self->returnType) {
        printf(" as ");
        ASTTypeSpec_gen(self->returnType, level);
    }
    puts("");

    ASTScope_gen(self->body, level + STEP);

    puts("end function\n");
}

const char* getDefaultValueForType(ASTTypeSpec* type)
{
    if (!type) return ""; // for no type e.g. void
    if (!strcmp(type->name, "String")) return "\"\"";
    if (!strcmp(type->name, "Scalar")) return "0";
    if (!strcmp(type->name, "Int")) return "0";
    if (!strcmp(type->name, "Int32")) return "0";
    if (!strcmp(type->name, "UInt32")) return "0";
    if (!strcmp(type->name, "UInt")) return "0";
    if (!strcmp(type->name, "UInt64")) return "0";
    if (!strcmp(type->name, "Int64")) return "0";
    if (!strcmp(type->name, "Real32")) return "0";
    if (!strcmp(type->name, "Real64")) return "0";
    if (!strcmp(type->name, "Real")) return "0";
    if (!strcmp(type->name, "Logical")) return "false";
    return "NULL";
}

void ASTFunc_genc(ASTFunc* self, int level)
{
    if (self->flags.isDeclare) return;

    printf("\n// ------------------------ function: %s ", self->name);
    printf("\n// ------------- approx. stack usage per call: %lu B \n",
        ASTFunc_calcSizeUsage(self));
    printf("#define DEFAULT_VALUE %s\n",
        getDefaultValueForType(self->returnType));
    if (!self->flags.exported) printf("static ");
    if (self->returnType) {
        ASTTypeSpec_genc(self->returnType, level, false);
    } else
        printf("void");
    str_tr_ip(self->name, '.', '_', 0);
    printf(" %s", self->name);
    str_tr_ip(self->name, '_', '.', 0);
    // TODO: here add the type of the first arg, unless it is a method
    // of a type
    if (self->args->next) { // means at least 2 args
        foreach (ASTVar*, arg, nargs, (self->args->next)) {
            // start from the 2nd
            printf("_%s", arg->name);
        }
    }
    printf("(");
    foreach (ASTVar*, arg, args, self->args) {
        ASTVar_genc(arg, level, true);
        printf(args->next ? ", " : "");
    }

    printf("\n#ifdef DEBUG\n"
           "    %c const char* callsite_ "
           "\n#endif\n",
        ((self->args->item ? ',' : ' ')));

    // TODO: if (flags.throws) printf("const char** _err_");
    puts(") {");
    printf("#ifdef DEBUG\n"
           "    static const char* sig_ = \"");
    printf("%s%s(", self->flags.isStmt ? "" : "function ", self->name);
    foreach (ASTVar*, arg, chargs, self->args) {
        ASTVar_gen(arg, level);
        printf(chargs->next ? ", " : ")");
    }
    if (self->returnType) {
        printf(" as ");
        ASTTypeSpec_gen(self->returnType, level);
    }
    puts("\";\n#endif");

    printf("%s",
        "#ifndef NOSTACKCHECK\n"
        "    STACKDEPTH_UP\n"
        "    if (_scStart_ - (char*)&a > _scSize_) {\n"
        "#ifdef DEBUG\n"
        "        _scPrintAbove_ = _scDepth_ - _btLimit_;\n"
        "        printf(\"\\033[31mfatal: stack overflow at call depth "
        "%d.\\n   "
        " in %s\\033[0m\\n\", _scDepth_, sig_);\n"
        "        printf(\"\\033[90mBacktrace (innermost "
        "first):\\n\");\n"
        "        if (_scDepth_ > 2*_btLimit_)\n        "
        "printf(\"    limited to %d outer and %d inner entries.\\n\", "
        "_btLimit_, _btLimit_);\n"
        "        printf(\"[%d] "
        "\\033[36m%s\\n\", _scDepth_, callsite_);\n"
        "#else\n"
        "        printf(\"\\033[31mfatal: stack "
        "overflow.\\033[0m\\n\");\n"
        "#endif\n"
        "        DOBACKTRACE\n    }\n"
        "#endif\n");

    ASTScope_genc(self->body, level + STEP);

    puts("    // ------------ error handling\n"
         "    return DEFAULT_VALUE;\n    assert(0);\n"
         "error:\n"
         "#ifdef DEBUG\n"
         "    fprintf(stderr,\"error: %s\\n\",_err_);\n"
         "#endif\n"
         "backtrace:\n"
         "#ifdef DEBUG\n"

         "    if (_scDepth_ <= _btLimit_ || "
         "_scDepth_ > _scPrintAbove_)\n"
         "        printf(\"\\033[90m[%d] \\033[36m"
         "%s\\n\", _scDepth_, callsite_);\n"
         "    else if (_scDepth_ == _scPrintAbove_)\n"
         "        printf(\"\\033[90m... truncated ...\\033[0m\\n\");\n"
         "#endif\n"
         "done:\n"
         "#ifndef NOSTACKCHECK\n"
         "    STACKDEPTH_DOWN\n"
         "#endif\n"
         "    return DEFAULT_VALUE;");
    puts("}\n#undef DEFAULT_VALUE");
}

void ASTFunc_genh(ASTFunc* self, int level)
{
    if (self->flags.isDeclare) return;
    if (!self->flags.exported) printf("static ");
    if (self->returnType) {
        ASTTypeSpec_genc(self->returnType, level, false);
    } else
        printf("void");
    str_tr_ip(self->name, '.', '_', 0);
    printf(" %s", self->name);
    str_tr_ip(self->name, '_', '.', 0);
    if (self->args->next) { // means at least 2 args
        foreach (ASTVar*, arg, nargs, self->args->next) {
            printf("_%s", arg->name);
        }
    }
    printf("(");

    foreach (ASTVar*, arg, args, self->args) {
        ASTVar_genc(arg, level, true);
        printf(args->next ? ", " : "");
    }
    printf("\n#ifdef DEBUG\n    %c const char* callsite_ "
           "\n#endif\n",
        ((self->args->item) ? ',' : ' '));
    puts(");\n");
}

void ASTExpr_gen(ASTExpr* self, int level, bool spacing, bool escapeStrings)
{
    // generally an expr is not split over several lines (but maybe in
    // rare cases). so level is not passed on to recursive calls.
    printf("%.*s", level, spaces);

    switch (self->kind) {
    case TKIdentifier:
    case TKNumber:
    case TKMultiDotNumber:
    case TKRegex:
    case TKInline:
        printf("%.*s", self->strLength, self->value.string);
        break;

    case TKString:
        printf(escapeStrings ? "\\%.*s\\\"" : "%.*s\"", self->strLength - 1,
            self->value.string);
        break;

    case TKLineComment:
        printf("%s%.*s",
            TokenKind_repr(TKLineComment, *self->value.string != ' '),
            self->strLength, self->value.string);
        break;

    case TKFunctionCall:
        printf("%.*s(", self->strLength, self->name);
        if (self->left) ASTExpr_gen(self->left, 0, false, escapeStrings);
        printf(")");
        break;

    case TKSubscript:
        printf("%.*s[", self->strLength, self->name);
        if (self->left) ASTExpr_gen(self->left, 0, false, escapeStrings);
        printf("]");
        break;

    case TKVarAssign:
        // var x as XYZ = abc... -> becomes an ASTVar and an ASTExpr
        // (to keep location). Send it to ASTVar_gen.
        assert(self->var != NULL);
        ASTVar_gen(self->var,0);
        break;

    case TKKeyword_for:
    case TKKeyword_if:
    case TKKeyword_while:
        printf("%s ", TokenKind_repr(self->kind, false));
        if (self->left) ASTExpr_gen(self->left, 0, true, escapeStrings);
        puts("");
        if (self->body)
            ASTScope_gen(self->body, level + STEP); //, true, escapeStrings);
        printf(
            "%.*send %s", level, spaces, TokenKind_repr(self->kind, false));
        break;

    default:
        if (not self->opPrecedence) break;
        // not an operator, but self should be error if you reach here
        bool leftBr = self->left and self->left->opPrecedence
            and self->left->opPrecedence < self->opPrecedence;
        bool rightBr = self->right and self->right->opPrecedence
            and self->right->kind
                != TKKeyword_return // found in 'or return'
            and self->right->opPrecedence < self->opPrecedence;

        if (self->kind == TKOpColon) {
            // expressions like arr[a:x-3:2] should become
            // arr[a:(x-3):2]
            // or list literals [8, 9, 6, 77, sin(c)]
            if (self->left) switch (self->left->kind) {
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
            if (self->right) switch (self->right->kind) {
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

        if (false and self->kind == TKKeyword_return and self->right) {
            switch (self->right->kind) {
            case TKString:
            case TKNumber:
            case TKIdentifier:
            case TKFunctionCall:
            case TKSubscript:
            case TKRegex:
            case TKMultiDotNumber:
                break;
            default:
                rightBr = true;
                break;
            }
        }

        if (self->kind == TKPower and not spacing) putc('(', stdout);

        char lpo = leftBr and self->left->kind == TKOpColon ? '[' : '(';
        char lpc = leftBr and self->left->kind == TKOpColon ? ']' : ')';
        if (leftBr) putc(lpo, stdout);
        if (self->left)
            ASTExpr_gen(self->left, 0,
                spacing and !leftBr and self->kind != TKOpColon,
                escapeStrings);
        if (leftBr) putc(lpc, stdout);

        printf("%s", TokenKind_repr(self->kind, spacing));

        char rpo = rightBr and self->right->kind == TKOpColon ? '[' : '(';
        char rpc = rightBr and self->right->kind == TKOpColon ? ']' : ')';
        if (rightBr) putc(rpo, stdout);
        if (self->right)
            ASTExpr_gen(self->right, 0,
                spacing and !rightBr and self->kind != TKOpColon,
                escapeStrings);
        if (rightBr) putc(rpc, stdout);

        if (self->kind == TKPower and not spacing) putc(')', stdout);
        if (self->kind == TKArrayOpen) putc(']', stdout);
    }
}
void ASTExpr_catarglabels(ASTExpr* self)
{
    switch (self->kind) {
    case TKOpComma:
        ASTExpr_catarglabels(self->left);
        ASTExpr_catarglabels(self->right);
        break;
    case TKOpAssign:
        printf("_%s", self->left->name);
        break;
    default:
        break;
    }
}
void ASTExpr_genc(ASTExpr* self, int level, bool spacing, bool inFuncArgs,
    bool escapeStrings)
{
    // generally an expr is not split over several lines (but maybe in
    // rare cases). so level is not passed on to recursive calls.

    printf("%.*s", level, spaces);
    switch (self->kind) {
    case TKNumber:
    case TKMultiDotNumber:
        printf("%.*s", self->strLength, self->value.string);
        break;

    case TKString:
        printf(escapeStrings ? "\\%.*s\\\"" : "%.*s\"", self->strLength - 1,
            self->value.string);
        break;

    case TKIdentifier:
    case TKIdentifierResolved:
        // convert a.b.c.d to DEREF3(a,b,c,d), a.b to DEREF(a,b) etc.
        {
            char* tmp = (self->kind == TKIdentifierResolved)
                ? self->var->name
                : self->name;
            int8_t dotCount = 0, i = 0;
            for (i = 0; tmp[i]; i++) {
                if (tmp[i] == '.') {
                    dotCount++;
                    tmp[i] = ',';
                }
            }
            if (dotCount)
                printf("DEREF%d(%s)", dotCount, tmp);
            else
                printf("%s", tmp);

            for (i = 0; tmp[i]; i++)
                if (tmp[i] == ',') tmp[i] = '.';
        }
        break;

    case TKRegex:
        self->value.string[0] = '"';
        self->value.string[self->strLength - 1] = '"';
        printf("%.*s", self->strLength, self->value.string);
        self->value.string[0] = '\'';
        self->value.string[self->strLength - 1] = '\'';
        break;

    case TKInline:
        self->value.string[0] = '"';
        self->value.string[self->strLength - 1] = '"';
        printf("mkRe_(%.*s)", self->strLength, self->value.string);
        self->value.string[0] = '`';
        self->value.string[self->strLength - 1] = '`';
        break;

    case TKLineComment:
        // TODO: skip  comments in generated code
        printf("// %.*s", self->strLength, self->value.string);
        break;

    case TKFunctionCall:
    case TKFunctionCallResolved: {
        char* tmp = (self->kind == TKFunctionCallResolved)
            ? self->func->name
            : self->name;
        str_tr_ip(tmp, '.', '_', 0); // self should have been done in a
                                     // previous stage prepc() or lower()
        printf("%s", tmp);
        if (*tmp >= 'A' and *tmp <= 'Z' and not strchr(tmp, '_'))
            printf("_new_"); // MyType() generates MyType_new_()
                             // TODO: if constructors for MyType are
                             // defined, they should
        // generate both a _init_arg1_arg2 function AND a corresponding
        // _new_arg1_arg2 func.
        if (self->left) ASTExpr_catarglabels(self->left);
        str_tr_ip(tmp, '_', '.', 0);
        // self won't be needed, prepc will do the "mangling"
        printf("(");

        if (self->left)
            ASTExpr_genc(self->left, 0, false, true, escapeStrings);

        if (strcmp(tmp, "print")) {
            // more generally self IF is for those funcs that are standard
            // and dont need any instrumentation
            printf("\n#ifdef DEBUG\n"
                   "      %c THISFILE \":%d:\\033[0m\\n     -> ",
                self->left ? ',' : ' ', self->line);
            ASTExpr_gen(self, 0, false, true);
            printf("\"\n"
                   "#endif\n        ");
            // printf("_err_"); // temporary -- self
            // should be if the function throws. resolve the func first
        }
        printf(")");
        break;
    }

    case TKSubscriptResolved:
    case TKSubscript: // TODO
                      // TODO: lookup the var, its typespec, then its dims.
                      // then slice
        // here should be slice1D slice2D etc.
        {
            char* tmp = (self->kind == TKSubscriptResolved)
                ? self->var->name
                : self->name;
            printf("slice(%s, {", tmp);
            if (self->left)
                ASTExpr_genc(
                    self->left, 0, false, inFuncArgs, escapeStrings);
            printf("})");
            break;
        }
    case TKOpAssign:
        if (!inFuncArgs) {
            ASTExpr_genc(self->left, 0, spacing, inFuncArgs, escapeStrings);
            printf("%s", TokenKind_repr(TKOpAssign, spacing));
        }
        ASTExpr_genc(self->right, 0, spacing, inFuncArgs, escapeStrings);
        // check various types of lhs  here, eg arr[9:87] = 0,
        // map["uuyt"]="hello" etc.
        break;

    case TKOpColon: // convert 3:4:5 to range(...)
                    // must do bounds check first!
        printf("%s(",
            self->left->kind != TKOpColon ? "range_to" : "range_to_by");
        if (self->left->kind == TKOpColon) {
            self->left->kind = TKOpComma;
            ASTExpr_genc(self->left, 0, false, inFuncArgs, escapeStrings);
            self->left->kind = TKOpColon;
        } else
            ASTExpr_genc(self->left, 0, false, inFuncArgs, escapeStrings);
        printf(", ");
        ASTExpr_genc(self->right, 0, false, inFuncArgs, escapeStrings);
        printf(")");
        break;

    case TKVarAssign: // basically a TKOpAssign corresponding to a local var
                      // var x as XYZ = abc... -> becomes an ASTVar and an
                      // ASTExpr (to keep location). Send it to ASTVar::gen.
        if (self->var->init != NULL) {
            printf("%s = ", self->var->name);
            ASTExpr_genc(
                self->var->init, 0, true, inFuncArgs, escapeStrings);
        }
        break;

    case TKKeyword_for:
    case TKKeyword_if:
    case TKKeyword_while:
        if (self->kind == TKKeyword_for)
            printf("FOR(");
        else
            printf("%s (", TokenKind_repr(self->kind, true));
        if (self->kind == TKKeyword_for) self->left->kind = TKOpComma;
        if (self->left)
            ASTExpr_genc(self->left, 0, spacing, inFuncArgs, escapeStrings);
        if (self->kind == TKKeyword_for) self->left->kind = TKOpAssign;
        puts(") {");
        if (self->body) ASTScope_genc(self->body, level + STEP);
        printf("%.*s}", level, spaces);
        break;

    case TKPower:
        printf("pow(");
        ASTExpr_genc(self->left, 0, false, inFuncArgs, escapeStrings);
        printf(",");
        ASTExpr_genc(self->right, 0, false, inFuncArgs, escapeStrings);
        printf(")");
        break;

    case TKKeyword_return:
        printf("{_err_ = NULL; _scDepth_--; return ");
        ASTExpr_genc(self->right, 0, spacing, inFuncArgs, escapeStrings);
        printf(";}\n");
        break;

    default:
        if (not self->opPrecedence) break;
        // not an operator, but self should be error if you reach here
        bool leftBr = self->left and self->left->opPrecedence
            and self->left->opPrecedence < self->opPrecedence;
        bool rightBr = self->right and self->right->opPrecedence
            and self->right->kind
                != TKKeyword_return // found in 'or return'
            and self->right->opPrecedence < self->opPrecedence;

        char lpo = '(';
        char lpc = ')';
        if (leftBr) putc(lpo, stdout);
        if (self->left)
            ASTExpr_genc(self->left, 0,
                spacing and !leftBr and self->kind != TKOpColon, inFuncArgs,
                escapeStrings);
        if (leftBr) putc(lpc, stdout);

        if (self->kind == TKArrayOpen)
            putc('{', stdout);
        else
            printf("%s", TokenKind_repr(self->kind, spacing));

        char rpo = '(';
        char rpc = ')';
        if (rightBr) putc(rpo, stdout);
        if (self->right)
            ASTExpr_genc(self->right, 0,
                spacing and !rightBr and self->kind != TKOpColon,
                inFuncArgs, escapeStrings);
        if (rightBr) putc(rpc, stdout);

        if (self->kind == TKArrayOpen) putc('}', stdout);
    }
}

#pragma mark - AST Module

typedef struct ASTModule {

    // STHEAD(ASTModule, 16)

    List(ASTFunc) * funcs;
    List(ASTExpr) * exprs;
    List(ASTType) * types;
    List(ASTVar) * globals;
    List(ASTImport) * imports;
    List(ASTFunc) * tests;

    char* name;
    char* moduleName; // mod.submod.xyz.mycode
    char* mangledName; // mod_submod_xyz_mycode
    char* capsMangledName; // MOD_SUBMOD_XYZ_MYCODE
} ASTModule;
ASTType* ASTModule_getType(ASTModule* self, const char* name)
{
    // the type may be "mm.XYZType" in which case you should look in
    // module mm instead. actually the caller should have bothered about
    // that.
    foreach (ASTType*, type, types, self->types) {
        if (!strcmp(type->name, name)) return type;
    }
    // type specs must be fully qualified, so there's no need to look in
    // other modules.
    return NULL;
}

// i like self pattern, getType, getFunc, getVar, etc.
// even the module should have getVar.
// you don't need the actual ASTImport object, so self one is just a
// bool. imports just turn into a #define for the alias and an #include
// for the actual file.
bool ASTModule_hasImportAlias(ASTModule* self, const char* alias)
{
    foreach (ASTImport*, imp, imps, self->imports) {
        if (!strcmp(imp->importFile + imp->aliasOffset, alias)) return true;
    }
    return false;
}

ASTFunc* ASTModule_getFunc(ASTModule* self, const char* name)
{
    // figure out how to deal with overloads. or set a selector field in
    // each astfunc.
    foreach (ASTFunc*, func, funcs, self->funcs) {
        if (!strcmp(func->name, name)) return func;
    }
    // again no looking anywhere else. If the name is of the form
    // "mm.func" you should have bothered to look in mm instead.
    return NULL;
}

void ASTModule_gen(ASTModule* self, int level)
{
    printf("! module %s\n", self->name);

    foreach (ASTImport*, import, imports, self->imports)
        ASTImport_gen(import, level);

    puts("");

    foreach (ASTType*, type, types, self->types)
        ASTType_gen(type, level);

    foreach (ASTFunc*, func, funcs, self->funcs)
        ASTFunc_gen(func, level);
}

void ASTModule_genc(ASTModule* self, int level)
{
    foreach (ASTImport*, import, imports, self->imports)
        ASTImport_genc(import, level);

    puts("");

    foreach (ASTType*, type, types, self->types)
        ASTType_genh(type, level);

    foreach (ASTFunc*, func, funcs, self->funcs)
        ASTFunc_genh(func, level);

    foreach (ASTType*, type, mtypes, self->types)
        ASTType_genc(type, level);

    foreach (ASTFunc*, func, mfuncs, self->funcs)
        ASTFunc_genc(func, level);

    foreach (ASTImport*, simport, simports, self->imports)
        ASTImport_undefc(simport);
}

#pragma mark - Parser

typedef struct Parser {
    // bring down struct size!
    char* filename; // mod/submod/xyz/mycode.ch
    char* moduleName; // mod.submod.xyz.mycode
    char* mangledName; // mod_submod_xyz_mycode
    char* capsMangledName; // MOD_SUBMOD_XYZ_MYCODE
    char* basename; // mycode
    char* dirname; // mod/submod/xyz
    char *data, *end;
    bool generateCommentExprs; // set to false when compiling, set to true
                               // when linting
    char* noext;
    Token token; // current
    List(ASTModule*) * modules; // module node of the AST
    // Stack(ASTScope*) scopes; // a stack that keeps track of scope nesting
    uint32_t errCount, warnCount;
    uint32_t errLimit;

} Parser;

// STHEAD_POOLB(Parser, 16)

// size_t dataSize() { return end - data; }

#define STR(x) STR_(x)
#define STR_(x) #x

void Parser_fini(Parser* self)
{
    free(self->data);
    free(self->noext);
    free(self->moduleName);
    free(self->mangledName);
    free(self->capsMangledName);
    free(self->dirname);
}
// ~Parser() { fini(); }
#define FILE_SIZE_MAX 1 << 24

Parser* Parser_fromFile(char* filename, bool skipws)
{
    Parser* ret = NEW(Parser);
    FILE* file = fopen(filename, "r");
    size_t flen = strlen(filename);

    ret->filename = filename;
    ret->noext = str_noext(filename);
    fseek(file, 0, SEEK_END);
    const size_t size = ftell(file) + 2;
    // 2 null chars, so we can always lookahead
    if (size < FILE_SIZE_MAX) {
        ret->data = (char*)malloc(size);
        fseek(file, 0, SEEK_SET);
        fread(ret->data, size, 1, file);
        ret->data[size - 1] = 0;
        ret->data[size - 2] = 0;
        ret->moduleName = str_tr(ret->noext, '/', '.');
        ret->mangledName = str_tr(ret->noext, '/', '_');
        ret->capsMangledName = str_upper(ret->mangledName);
        ret->basename = str_base(ret->noext, '/', flen);
        ret->dirname = str_dir(ret->noext);
        ret->end = ret->data + size;
        ret->token.pos = ret->data;
        ret->token.flags.skipWhiteSpace = skipws;
        ret->errCount = 0;
        ret->warnCount = 0;
        ret->errLimit = 20;
    } else {
        fprintf(stderr, "Source files larger than 24MB are not allowed.\n");
    }

    fclose(file);
    return ret;
}

#pragma mark - Error Reporting

#define fatal(str, ...)                                                    \
    {                                                                      \
        fprintf(stderr, str, __VA_ARGS__);                                 \
        exit(1);                                                           \
    }

void Parser_errorIncrement(Parser* self)
{
    if (++self->errCount >= self->errLimit)
        fatal("too many errors (%d), quitting\n", self->errLimit);
}

#define Parser_errorExpectedToken(s, a)                                    \
    Parser_errorExpectedToken_(s, a, __func__)
void Parser_errorExpectedToken_(
    Parser* self, TokenKind expected, const char* funcName)
{
    // fputs(dashes, stderr);
    fprintf(stderr,
        "(%d) \033[31merror:\033[0m %s at %s:%d:%d\n      expected "
        "'%s' "
        "found '%s'\n",
        self->errCount + 1, funcName, self->filename, self->token.line,
        self->token.col, TokenKind_repr(expected, false),
        TokenKind_repr(self->token.kind, false));
    Parser_errorIncrement(self);
}

#define Parser_errorParsingExpr(s) Parser_errorParsingExpr_(s, __func__)
void Parser_errorParsingExpr_(Parser* self, const char* funcName)
{
    // fputs(dashes, stderr);
    fprintf(stderr,
        "(%d) \033[31merror:\033[0m %s at %s line %d or %d\n"
        "      failed to parse expr",
        self->errCount + 1, funcName, self->filename, self->token.line - 1,
        self->token.line);
    // parseExpr will move to next line IF there was no hanging comment
    Parser_errorIncrement(self);
}

#define Parser_errorInvalidIdent(s, expr)                                  \
    Parser_errorInvalidIdent_(s, __func__, expr)
void Parser_errorInvalidIdent_(
    Parser* self, const char* funcName, ASTExpr* expr)
{
    // fputs(dashes, stderr);
    fprintf(stderr,
        "(%d) \033[31merror:\033[0m invalid name '%.*s' at %s:%d:%d\n",
        self->errCount + 1, expr->strLength, expr->value.string,
        self->filename, expr->line - 1,
        expr->col); // parseExpr will move to next line
    Parser_errorIncrement(self);
}

#define Parser_errorUnrecognizedVar(s, expr)                               \
    Parser_errorUnrecognizedVar_(s, __func__, expr)
void Parser_errorUnrecognizedVar_(
    Parser* self, const char* funcName, ASTExpr* expr)
{
    // fputs(dashes, stderr);
    fprintf(stderr,
        "(%d) \033[31merror:\033[0m unknown var '%.*s' at %s:%d:%d\n",
        self->errCount + 1, expr->strLength, expr->value.string,
        self->filename, expr->line, expr->col);
    Parser_errorIncrement(self);
}
//#define Parser_errorUnrecognizedVar(expr)
// Parser_errorUnrecognizedVar_(__func__,
// expr)
void Parser_errorDuplicateVar(Parser* self, ASTVar* var, ASTVar* orig)
{
    // fputs(dashes, stderr);
    fprintf(stderr,
        "(%d) \033[31merror:\033[0m duplicate var '%s' at %s:%d:%d\n   "
        "          already declared at %s:%d:%d\n",
        self->errCount + 1, var->name, self->filename, var->init->line, 9,
        self->filename, orig->init->line,
        9); // every var has init!! and every var is indented 4 spc ;-)
    Parser_errorIncrement(self);
}

#define Parser_errorUnrecognizedFunc(s, expr)                              \
    Parser_errorUnrecognizedFunc_(s, __func__, expr)
void Parser_errorUnrecognizedFunc_(
    Parser* self, const char* funcName, ASTExpr* expr)
{
    // fputs(dashes, stderr);
    fprintf(stderr,
        "(%d) \033[31merror:\033[0m unknown function '%.*s' at "
        "%s:%d:%d\n",
        self->errCount + 1, expr->strLength, expr->value.string,
        self->filename, expr->line, expr->col);
    Parser_errorIncrement(self);
}
#define Parser_errorUnrecognizedType(s, expr)                              \
    Parser_errorUnrecognizedType_(s, __func__, expr)
void Parser_errorUnrecognizedType_(
    Parser* self, const char* funcName, ASTTypeSpec* typeSpec)
{
    // fputs(dashes, stderr);
    // if it is unrecognized, its typeType is TYUnresolved and its
    // ->name exists.
    fprintf(stderr,
        "(%d) \033[31merror:\033[0m unknown type '%s' at %s:%d:%d\n",
        self->errCount + 1, typeSpec->name, self->filename, typeSpec->line,
        typeSpec->col);
    Parser_errorIncrement(self);
}

#define Parser_errorUnexpectedToken(s)                                     \
    Parser_errorUnexpectedToken_(s, __func__)
void Parser_errorUnexpectedToken_(Parser* self, const char* funcName)
{
    // fputs(dashes, stderr);
    fprintf(stderr,
        "(%d) \033[31merror:\033[0m %s at %s:%d:%d\n      unexpected "
        "self->token "
        "'%.*s'\n",
        self->errCount + 1, funcName, self->filename, self->token.line,
        self->token.col, self->token.matchlen, self->token.pos);
    Parser_errorIncrement(self);
}

#define Parser_errorUnexpectedExpr(s, e)                                   \
    Parser_errorUnexpectedExpr_(s, e, __func__)
void Parser_errorUnexpectedExpr_(
    Parser* self, const ASTExpr* expr, const char* funcName)
{
    // fputs(dashes, stderr);
    fprintf(stderr,
        // isatty(stderr)?
        "(%d) \033[31merror:\033[0m %s at %s:%d:%d\n      unexpected "
        "expr "
        "'%.*s'", //:
                  // "(%d) error: %s at %s:%d:%d\n      unexpected expr
                  // '%.*s'" ,
        self->errCount + 1, funcName, self->filename, expr->line, expr->col,
        expr->opPrecedence ? 100 : expr->strLength,
        expr->opPrecedence ? TokenKind_repr(expr->kind, false)
                           : expr->name);
    Parser_errorIncrement(self);
}

#pragma mark -

ASTExpr* exprFromCurrentToken(Parser* self)
{
    ASTExpr* expr = ASTExpr_fromToken(&self->token);
    Token_advance(&self->token);
    return expr;
}

ASTExpr* next_token_node(
    Parser* self, TokenKind expected, const bool ignore_error)
{
    if (self->token.kind == expected) {
        return exprFromCurrentToken(self);
    } else {
        if (not ignore_error) Parser_errorExpectedToken(self, expected);
        return NULL;
    }
}
// these should all be part of Token_ when converted back to C
// in the match case, self->token should be advanced on error
ASTExpr* match(Parser* self, TokenKind expected)
{
    return next_token_node(self, expected, false);
}

// self returns the match node or null
ASTExpr* trymatch(Parser* self, TokenKind expected)
{
    return next_token_node(self, expected, true);
}

// just yes or no, simple
bool matches(Parser* self, TokenKind expected)
{
    return (self->token.kind == expected);
}

bool Parser_ignore(Parser* self, TokenKind expected)
{
    bool ret;
    if ((ret = matches(self, expected))) Token_advance(&self->token);
    return ret;
}

// self is same as match without return
void discard(Parser* self, TokenKind expected)
{
    if (not Parser_ignore(self, expected))
        Parser_errorExpectedToken(self, expected);
}

char* parseIdent(Parser* self)
{
    if (self->token.kind != TKIdentifier)
        Parser_errorExpectedToken(self, TKIdentifier);
    char* p = self->token.pos;
    Token_advance(&self->token);
    return p;
}

//    void resolveFunc(ASTFunc* func, ASTModule* mod) {}
void resolveTypeSpec(Parser* self, ASTTypeSpec* typeSpec, ASTModule* mod)
{
    if (typeSpec->typeType != TYUnresolved) return;

    TypeTypes tyty = TypeType_TypeTypeforSpec(typeSpec->name);
    if (tyty) // can be member of ASTTypeSpec!
    {
        typeSpec->typeType = tyty;
    } else {
        foreach (ASTType*, type, types, mod->types) {

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
        Parser_errorUnrecognizedType(self, typeSpec);
        return;
    }
    if (typeSpec->dims) {
        // set collection type, etc.
        // for self we will need info about the var and its usage
        // patterns. so self will probably be a separate func that is
        // called during such analysis.
    }
}

void resolveTypeSpecsInExpr(Parser* self, ASTExpr* expr, ASTModule* mod)
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
        resolveTypeSpec(self, expr->var->typeSpec, mod);
    } else {
        //            if (expr->opPrecedence) {
        //                if (!expr->opIsUnary)
        //                resolveFuncCall(expr->left, mod);
        //                resolveFuncCall(expr->right, mod);
        //            }
        //            else
        if (expr->kind == TKKeyword_if or expr->kind == TKKeyword_for
            or expr->kind == TKKeyword_while) {
            // self shouldnt be here, either resolveFuncCall should take
            // astscope* or there must be a helper func that takes
            // astscope* so you can descend easily
            //                resolveFuncCall(expr->left, mod);
            foreach (ASTExpr*, stmt, stmts, expr->body->stmts) {
                resolveTypeSpecsInExpr(self, stmt, mod);
            }
        } // else assert(0);
    }
}

// TODO: Btw there should be a module level scope to hold lets (and
// comments). That will be the root scope which has parent==NULL.
// void checkShadowing(ASTVar* var, ASTScope* scope) {}

void resolveFuncCall(Parser* self, ASTExpr* expr, ASTModule* mod)
{
    // TODO: what happens if you get a TKSubscriptResolved?
    if (expr->kind == TKFunctionCallResolved) {
    } else if (expr->kind == TKSubscriptResolved
        or expr->kind == TKSubscript) {
        if (expr->left)
            resolveFuncCall(
                self, expr->left, mod); // check nested func calls

    } else if (expr->kind == TKFunctionCall) {
        foreach (ASTFunc*, func, funcs, mod->funcs) {
            if (!strncmp(expr->name, func->name, expr->strLength)
                and func->name[expr->strLength] == '\0') {
                expr->kind = TKFunctionCallResolved;
                expr->func = func;
                if (expr->left) resolveFuncCall(self, expr->left, mod);
                // check nested func calls
                return;
            }
        } // since it is known which module the func must be found in,
          // no need to scan others function has not been found
        Parser_errorUnrecognizedFunc(self, expr);
        if (expr->left) resolveFuncCall(self, expr->left, mod);
        // but still check nested func calls
    } else if (expr->kind == TKVarAssign) {
        //            if (!expr->var->init) errorMissingInit(expr);
        resolveFuncCall(self, expr->var->init, mod);
    } else {
        if (expr->opPrecedence) {
            if (!expr->opIsUnary) resolveFuncCall(self, expr->left, mod);
            resolveFuncCall(self, expr->right, mod);
        } else if (expr->kind == TKKeyword_if or expr->kind == TKKeyword_for
            or expr->kind == TKKeyword_while) {
            // self shouldnt be here, either resolveFuncCall should take
            // astscope* or there must be a helper func that takes
            // astscope* so you can descend easily
            resolveFuncCall(self, expr->left, mod);
            foreach (ASTExpr*, stmt, stmts, expr->body->stmts) {
                resolveFuncCall(self, stmt, mod);
            }
        } // else assert(0);
    }
}

void resolveVars(Parser* self, ASTExpr* expr, ASTScope* scope)
{
    if (expr->kind == TKIdentifierResolved) {
    } else if (expr->kind == TKIdentifier or expr->kind == TKSubscript) {
        TokenKind ret = (expr->kind == TKIdentifier) ? TKIdentifierResolved
                                                     : TKSubscriptResolved;
        ASTScope* scp = scope;
        do {
            foreach (ASTVar*, local, locals, scp->locals) {
                if (!strncmp(expr->name, local->name, expr->strLength)
                    and local->name[expr->strLength] == '\0') {
                    expr->kind = ret;
                    expr->var = local; // self overwrites name btw
                    //                        printf("got
                    //                        %s\n",local->name);
                    goto getout;
                }
            }
            scp = scp->parent;
        } while (scp);
        Parser_errorUnrecognizedVar(self, expr);
        //            printf("unresolved %s\n",expr->name);
    getout:
        // TODO: there needs tobe a single resolve() function that does
        // both vars and funcs, because subscripts and funcs can be
        // nested indefinitely within each other. NO WAIT it seems to
        // work as it is I think.

        if (ret == TKSubscriptResolved) {
            resolveVars(self, expr->left,
                scope); /*resolveFuncCall(expr->left, mod)*/
        }
        // descend into the args of the subscript and resolve inner vars
    } else if (expr->kind == TKFunctionCall) {
        if (expr->left) resolveVars(self, expr->left, scope);
    } else {
        if (expr->opPrecedence) {
            if (!expr->opIsUnary) resolveVars(self, expr->left, scope);
            resolveVars(self, expr->right, scope);
        }
    }
}

#pragma mark -
ASTExpr* parseExpr(Parser* self)
{
    // there are 2 steps to self madness.
    // 1. parse a sequence of tokens into RPN using shunting-yard.
    // 2. walk the rpn stack as a sequence and copy it into a result
    // stack, collapsing the stack when you find nonterminals (ops, func
    // calls, array index, ...)

    // we could make self static and set len to 0 upon func exit
    static Stack /* ASTExpr*, 32 */ rpn, ops, result;
    int prec_top = 0;
    ASTExpr* p = NULL;
    TokenKind revBrkt = TKUnknown;

    // ******* STEP 1 CONVERT TOKENS INTO RPN

    while (self->token.kind != TKNullChar and self->token.kind != TKNewline
        and self->token.kind != TKLineComment) { // build RPN

        // you have to ensure that ops have a space around them, etc.
        // so don't just skip the one spaces like you do now.
        if (self->token.kind == TKOneSpace) Token_advance(&self->token);

        ASTExpr* expr = ASTExpr_fromToken(&self->token); // dont advance yet
        int prec = expr->opPrecedence;
        bool rassoc = prec ? expr->opIsRightAssociative : false;
        char lookAheadChar = Token_peekCharAfter(&self->token);

        switch (expr->kind) {
        case TKIdentifier:
            if (memchr(expr->value.string, '_', expr->strLength)
                or doesKeywordMatch(expr->value.string, expr->strLength))
                Parser_errorInvalidIdent(
                    self, expr); // but continue parsing
            switch (lookAheadChar) {
            case '(':
                expr->kind = TKFunctionCall;
                expr->opPrecedence = 100;
                Stack_push(&ops, expr);
                break;
            case '[':
                expr->kind = TKSubscript;
                expr->opPrecedence = 100;
                Stack_push(&ops, expr);
                break;
            default:
                Stack_push(&rpn, expr);
                break;
            }
            break;
        case TKParenOpen:
            Stack_push(&ops, expr);
            if (not Stack_empty(&ops)
                and Stack_topAs(ASTExpr*, &ops)->kind == TKFunctionCall)
                Stack_push(&rpn, expr);
            if (lookAheadChar == ')')
                Stack_push(
                    &rpn, NULL); // for empty func() push null for no args
            break;

        case TKArrayOpen:
            Stack_push(&ops, expr);
            if (not Stack_empty(&ops)
                and Stack_topAs(ASTExpr*, &ops)->kind == TKSubscript)
                Stack_push(&rpn, expr);
            if (lookAheadChar == ')')
                Stack_push(
                    &rpn, NULL); // for empty arr[] push null for no args
            break;

        case TKParenClose:
        case TKArrayClose:
        case TKBraceClose:

            revBrkt = TokenKind_reverseBracket(expr->kind);
            if (Stack_empty(&ops)) { // need atleast the opening bracket of
                // the current kind
                Parser_errorParsingExpr(self);
                goto error;
            }

            else
                while (not Stack_empty(&ops)) {
                    p = Stack_pop(&ops);
                    if (p->kind == revBrkt) break;
                    Stack_push(&rpn, p);
                }
            // we'll push the TKArrayOpen as well to indicate a list
            // literal or comprehension
            // TKArrayOpen is a unary op.
            if ((p and p->kind == TKArrayOpen)
                and (Stack_empty(&ops)
                    or Stack_topAs(ASTExpr*, &ops)->kind != TKSubscript)
                // don't do self if its part of a subscript
                and (Stack_empty(&rpn)
                    or Stack_topAs(ASTExpr*, &rpn)->kind != TKOpColon))
                // or aa range. range exprs are handled separately. by
                // themselves they don't need a surrounding [], but for
                // grouping like 2+[8:66] they do.
                Stack_push(&rpn, p);

            break;
        case TKKeyword_return:
            // for empty return, push a NULL if there is no expr coming.
            Stack_push(&ops, expr);
            if (lookAheadChar == '!' or lookAheadChar == '\n')
                Stack_push(&rpn, NULL);
            break;
        default:
            if (prec) { // general operators

                if (expr->kind == TKOpColon) {
                    if (Stack_empty(&rpn)
                        or (!Stack_top(&rpn) and !Stack_empty(&ops)
                            and Stack_topAs(ASTExpr*, &ops)->kind
                                != TKOpColon)
                        or (Stack_topAs(ASTExpr*, &rpn)
                            and Stack_topAs(ASTExpr*, &rpn)->kind
                                == TKOpColon
                            and !Stack_empty(&ops)
                            and (Stack_topAs(ASTExpr*, &ops)->kind
                                    == TKOpComma
                                or Stack_topAs(ASTExpr*, &ops)->kind
                                    == TKArrayOpen)) // <<-----
                                                     // yesssssssssss
                    )

                        Stack_push(&rpn, NULL); // indicates empty operand
                }
                while (not Stack_empty(&ops)) {
                    prec_top = Stack_topAs(ASTExpr*, &ops)->opPrecedence;
                    if (not prec_top) break; // left parenthesis
                    if (prec > prec_top) break;
                    if (prec == prec_top and rassoc) break;
                    p = Stack_pop(&ops);

                    if (p->kind != TKOpComma and p->kind != TKOpSemiColon
                        and p->kind != TKFunctionCall
                        and p->kind != TKSubscript
                        and Stack_topAs(ASTExpr*, &rpn)
                        and Stack_topAs(ASTExpr*, &rpn)->kind
                            == TKOpComma) {
                        Parser_errorUnexpectedToken(self);
                        goto error;
                    }

                    if (!(p->opPrecedence or p->opIsUnary)
                        and p->kind != TKFunctionCall
                        and p->kind != TKOpColon
                        // in case of ::, second colon will add null
                        // later
                        and p->kind != TKSubscript and rpn.used < 2) {
                        Parser_errorUnexpectedToken(self);
                        goto error;
                        // TODO: even if you have more than two, neither
                        // of the top two should be a comma
                    }

                    Stack_push(&rpn, p);
                }

                // when the first error is found in an expression, seek
                // to the next newline and return NULL.
                if (Stack_empty(&rpn)) {
                    Parser_errorUnexpectedToken(self);
                    goto error;
                }
                if (expr->kind == TKOpColon
                    and (lookAheadChar == ',' or lookAheadChar == ':'
                        or lookAheadChar == ']' or lookAheadChar == ')'))
                    Stack_push(&rpn, NULL);

                Stack_push(&ops, expr);
            } else {
                Stack_push(&rpn, expr);
            }
        }
        Token_advance(&self->token);
        if (self->token.kind == TKOneSpace) Token_advance(&self->token);
    }
exitloop:

    while (not Stack_empty(&ops)) {
        p = Stack_pop(&ops);

        if (p->kind != TKOpComma and p->kind != TKFunctionCall
            and p->kind != TKSubscript and p->kind != TKArrayOpen
            and Stack_topAs(ASTExpr*, &rpn)
            and Stack_topAs(ASTExpr*, &rpn)->kind == TKOpComma) {
            Parser_errorUnexpectedExpr(self, Stack_topAs(ASTExpr*, &rpn));
            goto error;
        }

        if (!(p->opPrecedence or p->opIsUnary)
            and (p->kind != TKFunctionCall and p->kind != TKSubscript)
            and rpn.used < 2) {
            Parser_errorParsingExpr(self);
            goto error;
            // TODO: even if you have more than two, neither of the top
            // two should be a comma
        }

        Stack_push(&rpn, p);
    }

    // *** STEP 2 CONVERT RPN INTO EXPR TREE

    ASTExpr* arg;
    for (int i = 0; i < rpn.used; i++) {
        if (!(p = rpn.ref[i])) goto justpush;
        switch (p->kind) {
        case TKFunctionCall:
        case TKSubscript:
            if (result.used > 0) {
                arg = Stack_pop(&result);
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
                Parser_errorParsingExpr(self);
                goto error;
            }

            if (Stack_empty(&result)) {
                Parser_errorParsingExpr(self);
                goto error;
            }

            p->right = Stack_pop(&result);

            if (not p->opIsUnary) {
                if (Stack_empty(&result)) {
                    Parser_errorParsingExpr(self);
                    goto error;
                }
                p->left = Stack_pop(&result);
            }
        }
    justpush:
        Stack_push(&result, p);
    }
    if (result.used != 1) {
        Parser_errorParsingExpr(self);
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

    while (self->token.pos < self->end
        and (self->token.kind != TKNewline
            and self->token.kind != TKLineComment))
        Token_advance(&self->token);

    if (ops.used) printf("\n      ops: ");
    for (int i = 0; i < ops.used; i++)
        printf("%s ", TokenKind_repr(((ASTExpr*)ops.ref[i])->kind, false));

    if (rpn.used) printf("\n      rpn: ");
    for (int i = 0; i < rpn.used; i++)
        if (!rpn.ref[i])
            printf("NUL ");
        else {
            ASTExpr* e = rpn.ref[i];
            printf("%.*s ", e->opPrecedence ? 100 : e->strLength,
                e->opPrecedence ? TokenKind_repr(e->kind, false) : e->name);
        }

    if (result.used) printf("\n      rpn: ");
    for (int i = 0; i < result.used; i++)
        if (!result.ref[i])
            printf("NUL ");
        else {
            ASTExpr* e = result.ref[i];
            printf("%.*s ", e->opPrecedence ? 100 : e->strLength,
                e->opPrecedence ? TokenKind_repr(e->kind, false) : e->name);
        }

    if (p)
        printf("\n      p: %.*s ", p->opPrecedence ? 100 : p->strLength,
            p->opPrecedence ? TokenKind_repr(p->kind, false) : p->name);

    if (rpn.used or ops.used or result.used) puts("");

    ops.used = 0; // "reset" stacks
    rpn.used = 0;
    result.used = 0;
    return NULL;
}

#pragma mark -
ASTTypeSpec* parseTypeSpec(Parser* self)
{ // must have ident(U), then may have "[:,:]" i.e. '[\]\[\:, ]+' , then
  // may have units. note: after ident may have params <T, S>
    self->token.flags.mergeArrayDims = true;

    ASTTypeSpec* typeSpec = NEW(ASTTypeSpec);
    typeSpec->line = self->token.line;
    typeSpec->col = self->token.col;

    typeSpec->name = parseIdent(self);
    //        typeSpec->params = parseParams();
    // have a loop and switch here so you can parse both
    // Number[:,:]|kg/s and Number|kg/s[:,:]. linter will fix.

    if (matches(self, TKArrayDims)) {
        for (int i = 0; i < self->token.matchlen; i++)
            if (self->token.pos[i] == ':') typeSpec->dims++;
        if (!typeSpec->dims) typeSpec->dims = 1; // [] is 1 dim
        Token_advance(&self->token);
    }

    Parser_ignore(self, TKUnits);
    // fixme: node->type = lookupType;

    assert(self->token.kind != TKUnits);
    assert(self->token.kind != TKArrayDims);

    self->token.flags.mergeArrayDims = false;
    return typeSpec;
}

#pragma mark -
ASTVar* parseVar(Parser* self)
{
    ASTVar* var = NEW(ASTVar);
    var->flags.isVar = (self->token.kind == TKKeyword_var);
    var->flags.isLet = (self->token.kind == TKKeyword_let);

    if (var->flags.isVar) discard(self, TKKeyword_var);
    if (var->flags.isLet) discard(self, TKKeyword_let);
    if (var->flags.isVar or var->flags.isLet) discard(self, TKOneSpace);

    var->name = parseIdent(self);

    if (Parser_ignore(self, TKOneSpace)
        and Parser_ignore(self, TKKeyword_as)) {
        discard(self, TKOneSpace);
        var->typeSpec = parseTypeSpec(self);
    } else {
        var->typeSpec = NEW(ASTTypeSpec);
        var->typeSpec->line = self->token.line;
        var->typeSpec->col = self->token.col;
        var->typeSpec->name = "Scalar";
    }

    Parser_ignore(self, TKOneSpace);
    if (Parser_ignore(self, TKOpAssign)) var->init = parseExpr(self);

    // TODO: set default inferred type if typeSpec is NULL but init
    // present.
    //     const char* ctyp
    // = TokenKind_defaultType(var->init ? var->init->kind : TKUnknown);

    return var;
}

List(ASTVar) * parseArgs(Parser* self)
{
    List(ASTVar) * args;
    self->token.flags.mergeArrayDims = true;
    discard(self, TKParenOpen);
    if (Parser_ignore(self, TKParenClose)) return args;

    ASTVar* arg;
    do {
        arg = parseVar(self);
        PtrList_append(&args, arg);
    } while (Parser_ignore(self, TKOpComma));

    discard(self, TKParenClose);

    self->token.flags.mergeArrayDims = false;
    return args;
}

#pragma mark -
ASTScope* parseScope(Parser* self, ASTScope* parent)
{
    ASTScope* scope = NEW(ASTScope);

    ASTVar *var = NULL, *orig = NULL;
    ASTExpr* expr = NULL;
    TokenKind tt = TKUnknown;

    scope->parent = parent;

    // don't conflate self with the while in parse(): it checks against
    // file end, self checks against the keyword 'end'.
    while (self->token.kind != TKKeyword_end) {

        switch (self->token.kind) {

        case TKNullChar:
            Parser_errorExpectedToken(self, TKUnknown);
            goto exitloop;

        case TKKeyword_var:
        case TKKeyword_let:
            var = parseVar(self);
            if (!var) continue;
            if ((orig = ASTScope_getVar(scope, var->name)))
                Parser_errorDuplicateVar(self, var, orig);
            if (var->init
                and (var->init->opPrecedence
                    or var->init->kind == TKIdentifier))
                resolveVars(self, var->init, scope);
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
            expr->line = self->token.line;
            expr->col = self->token.col;
            expr->opPrecedence = TokenKind_getPrecedence(TKOpAssign);
            expr->var = var;
            PtrList_append(&scope->stmts, expr);
            break;

        case TKKeyword_if:
        case TKKeyword_for:
        case TKKeyword_while:
            tt = self->token.kind;
            expr = match(self, tt); // will advance
            expr->left = parseExpr(self);
            resolveVars(self, expr->left, scope);
            // TODO: `for` necessarily introduces a counter variable, so
            // check if that var name doesn't already exist in scope.
            // Also assert that the cond of a for expr has kind
            // TKOpAssign.

            expr->body = parseScope(self, scope);
            discard(self, TKKeyword_end);
            discard(self, TKOneSpace);
            discard(self, tt);
            PtrList_append(&scope->stmts, expr);
            break;

        case TKNewline:
        case TKOneSpace: // found at beginning of line]
            Token_advance(&self->token);
            break;

        case TKLineComment:
            if (self->generateCommentExprs) {
                expr = ASTExpr_fromToken(&self->token);
                PtrList_append(&scope->stmts, expr);
            }
            Token_advance(&self->token);
            break;

        default:
            expr = parseExpr(self);
            if (!expr) break;
            PtrList_append(&scope->stmts, expr);
            resolveVars(self, expr, scope);
            break;
        }
    }
exitloop:
    return scope;
}

#pragma mark -
List(ASTVar) * parseParams(Parser* self)
{
    discard(self, TKOpLT);
    List(ASTVar) * params;
    ASTVar* param;
    do {
        param = NEW(ASTVar);
        param->name = parseIdent(self);
        if (Parser_ignore(self, TKKeyword_as))
            param->typeSpec = parseTypeSpec(self);
        if (Parser_ignore(self, TKOpAssign)) param->init = parseExpr(self);
        PtrList_append(&params, param);
    } while (Parser_ignore(self, TKOpComma));

    discard(self, TKOpGT);
    return params;
}

#pragma mark -
ASTFunc* parseFunc(Parser* self, bool shouldParseBody)
{
    discard(self, TKKeyword_function);
    discard(self, TKOneSpace);
    ASTFunc* func = NEW(ASTFunc);

    func->line = self->token.line;
    func->col = self->token.col;

    func->name = parseIdent(self);

    func->flags.isDeclare = !shouldParseBody;

    if (!strcmp(func->name, "print")) func->flags.throws = 0;

    func->args = parseArgs(self);
    if (Parser_ignore(self, TKOneSpace)
        and Parser_ignore(self, TKKeyword_returns)) {
        discard(self, TKOneSpace);
        func->returnType = parseTypeSpec(self);
    }

    if (shouldParseBody) {
        discard(self, TKNewline);

        ASTScope* funcScope = NEW(ASTScope);
        funcScope->locals = func->args;
        func->body = parseScope(self, funcScope);

        discard(self, TKKeyword_end);
        discard(self, TKOneSpace);
        discard(self, TKKeyword_function);
    } else {
        func->body = NULL;
    }

    return func;
}

ASTFunc* parseStmtFunc(Parser* self)
{
    ASTFunc* func = NEW(ASTFunc);

    func->line = self->token.line;
    func->col = self->token.col;

    func->name = parseIdent(self);
    func->args = parseArgs(self);
    if (Parser_ignore(self, TKOneSpace)
        and Parser_ignore(self, TKKeyword_as)) {
        discard(self, TKOneSpace);
        func->returnType = parseTypeSpec(self);
    }

    ASTExpr* ret = exprFromCurrentToken(self);
    assert(ret->kind == TKColEq);
    ret->kind = TKKeyword_return;
    ret->opIsUnary = true;

    ret->right = parseExpr(self);
    ASTScope* scope = NEW(ASTScope);
    PtrList_append(&scope->stmts, ret);

    func->body = scope;

    return func;
}

#pragma mark -
ASTFunc* parseTest(Parser* self) { return NULL; }

#pragma mark -
ASTUnits* parseUnits(Parser* self) { return NULL; }

#pragma mark -
ASTType* parseType(Parser* self)
{
    ASTType* type = NEW(ASTType);

    ASTExpr* expr;
    ASTVar *var, *orig;

    discard(self, TKKeyword_type);
    discard(self, TKOneSpace);
    type->name = parseIdent(self);
    if (matches(self, TKOpLT)) type->params = parseParams(self);

    if (Parser_ignore(self, TKOneSpace)
        and Parser_ignore(self, TKKeyword_extends)) {
        discard(self, TKOneSpace);
        type->super = parseTypeSpec(self);
    }

    // JUST parse base manually here and then call parseScope!
    // at the end of calling parseScope, examine the resulting scope
    // and raise an error if anything other than var or check is found

    // DEFINITELY shouldbe calling parseScope here
    while (self->token.kind != TKKeyword_end) {
        switch (self->token.kind) {
        case TKNullChar:
            Parser_errorUnexpectedToken(self);
            goto exitloop;

        case TKKeyword_var:
            var = parseVar(self);
            if (!var) continue;
            if ((orig = ASTType_getVar(type, var->name)))
                Parser_errorDuplicateVar(self, var, orig);
            // resolveVars(var->init, <#ASTScope *scope#>) SEE WHY WE
            // NEED TO CALL parseScope here?!
            PtrList_append(&type->vars, var);
            // vars holds the local vars just like parseScope
            // does. but each var also goes in the expr list
            // to keep ordering. the expr list is named
            // 'checks' unfortunately.

            expr = NEW(ASTExpr); // skip comments when not linting
            expr->kind = TKVarAssign;
            expr->opPrecedence = TokenKind_getPrecedence(TKOpAssign);
            expr->var = var;
            expr->line = self->token.line;
            expr->col = self->token.col;

            PtrList_append(&type->checks, expr);
            break;

        case TKKeyword_extends:
            discard(self, TKKeyword_extends);
            discard(self, TKOneSpace);
            type->super = parseTypeSpec(self);
            break;

        case TKNewline:
        case TKOneSpace:
            Token_advance(&self->token);
            break;
        case TKLineComment:
            if (self->generateCommentExprs) {
                expr = ASTExpr_fromToken(&self->token);
                PtrList_append(&type->checks, expr);
            }
            Token_advance(&self->token);
            break;
        case TKIdentifier:
            if (!strncmp("check", self->token.pos, 5)) {
                expr = parseExpr(self);
                PtrList_append(&type->checks, expr);
                break;
            }
        default:
            // general exprs are not allowed. Report the error as
            // unexpected self->token (first self->token on the line), then
            // seek to the next newline.
            Parser_errorUnexpectedToken(self);
            fprintf(stderr,
                "      only 'var', 'let', and 'check' statements are "
                "allowed in types\n");
            while (self->token.kind != TKNewline
                and self->token.kind != TKLineComment)
                Token_advance(&self->token);
            break;
        }
    }
exitloop:

    discard(self, TKKeyword_end);
    discard(self, TKOneSpace);
    discard(self, TKKeyword_type);

    return type;
}

#pragma mark -
ASTImport* parseImport(Parser* self)
{
    ASTImport* import = NEW(ASTImport);
    char* tmp;
    discard(self, TKKeyword_import);
    discard(self, TKOneSpace);

    import->isPackage = Parser_ignore(self, TKAt);
    import->importFile = parseIdent(self);
    size_t len = self->token.pos - import->importFile;
    Parser_ignore(self, TKOneSpace);
    if (Parser_ignore(self, TKKeyword_as)) {

        Parser_ignore(self, TKOneSpace);
        import->hasAlias = true;
        tmp = parseIdent(self);
        if (tmp) import->aliasOffset = (uint32_t)(tmp - import->importFile);

    } else {
        import->aliasOffset = (uint32_t)(
            str_base(import->importFile, '.', len) - import->importFile);
    }

    Parser_ignore(self, TKOneSpace);

    if (self->token.kind != TKLineComment and self->token.kind != TKNewline)
        Parser_errorUnexpectedToken(self);
    while (
        self->token.kind != TKLineComment and self->token.kind != TKNewline)
        Token_advance(&self->token);
    return import;
}

#pragma mark -
List(ASTModule) * parse(Parser* self)
{
    ASTModule* root = NEW(ASTModule);
    root->name = self->moduleName;
    const bool onlyPrintTokens = false;
    Token_advance(&self->token); // maybe put self in parser ctor
    ASTImport* import = NULL;

    // The take away is (for C gen):
    // Every caller who calls append(List) should keep a local List*
    // to follow the list top as items are appended. Each actual append
    // call must be followed by an update of self pointer to its own
    // ->next. Append should be called on the last item of the list, not
    // the first. (it will work but seek through the whole list every
    // time).

    List(ASTFunc)* funcsTop = root->funcs;
    List(ASTImport)* importsTop = root->imports;
    List(ASTType)* typesTop = root->types;
    List(ASTFunc)* testsTop = root->tests;
    List(ASTVar)* globalsTop = root->globals;

    while (self->token.kind != TKNullChar) {
        if (onlyPrintTokens) {
            printf("%s %2d %3d %3d %-6s\t%.*s\n", self->basename,
                self->token.line, self->token.col, self->token.matchlen,
                TokenKind_repr(self->token.kind, false),
                self->token.kind == TKNewline ? 0 : self->token.matchlen,
                self->token.pos);
            Token_advance(&self->token);
            continue;
        }
        switch (self->token.kind) {
        case TKKeyword_declare:
            Token_advance(
                &self->token); // discard(self, TKKeyword_declare);
            discard(self, TKOneSpace);
            PtrList_append(&funcsTop, parseFunc(self, false));
            if (funcsTop->next) funcsTop = funcsTop->next;
            break;

        case TKKeyword_function:
            PtrList_append(&funcsTop, parseFunc(self, true));
            if (funcsTop->next) funcsTop = funcsTop->next;
            break;
        // case TKKeyword_declare:
        //     funcsTop
        case TKKeyword_type:
            PtrList_append(&typesTop, parseType(self));
            if (typesTop->next) typesTop = typesTop->next;
            break;
        case TKKeyword_import:
            import = parseImport(self);
            if (import) {
                PtrList_append(&importsTop, import);
                if (importsTop->next) importsTop = importsTop->next;
                //                    auto subParser = new
                //                    Parser(import->importFile);
                //                    List<ASTModule*> subMods =
                //                    parse(subParser);
                //                    PtrList_append(&modules, subMods);
            }
            break;
        case TKKeyword_test:
            PtrList_append(&testsTop, parseTest(self));
            if (testsTop->next) testsTop = testsTop->next;
            break;
        case TKKeyword_var:
        case TKKeyword_let:
            PtrList_append(&globalsTop, parseVar(self));
            if (globalsTop->next) globalsTop = globalsTop->next;
            break;
        case TKNewline:
        case TKLineComment:
        case TKOneSpace:
            Token_advance(&self->token);
            break;
        case TKIdentifier: // stmt funcs: f(x) := f(y, w = 4) etc.
            if (Token_peekCharAfter(&self->token) == '(') {
                PtrList_append(&funcsTop, parseStmtFunc(self));
                if (funcsTop->next) funcsTop = funcsTop->next;
                break;
            }
        default:
            printf("other self->token: %s at %d:%d len %d\n",
                TokenKind_repr(self->token.kind, false), self->token.line,
                self->token.col, self->token.matchlen);
            Parser_errorUnexpectedToken(self);
            while (self->token.kind != TKNewline
                and self->token.kind != TKLineComment)
                Token_advance(&self->token);
        }
    }
    // also keep modulesTop

    // do some analysis that happens after the entire module is loaded
    foreach (ASTFunc*, func, funcs, root->funcs) {
        if (!func->body) continue;
        foreach (ASTVar*, arg, args, func->args) {
            resolveTypeSpec(self, arg->typeSpec, root);
        }
        if (func->returnType) resolveTypeSpec(self, func->returnType, root);
        foreach (ASTExpr*, stmt, stmts, func->body->stmts) {
            resolveFuncCall(
                self, stmt, root); // should be part of astmodule, and
                                   // resolveVars should be part of astscope
            resolveTypeSpecsInExpr(self, stmt, root);
        }
    }

    PtrList_append(&self->modules, root);
    return self->modules;
}

// TODO: this should be in ASTModule open/close
void Parser_genc_open(Parser* self)
{
    printf("#ifndef HAVE_%s\n#define HAVE_%s\n\n", self->capsMangledName,
        self->capsMangledName);
    printf("#define THISMODULE %s\n", self->mangledName);
    printf("#define THISFILE \"%s\"\n", self->filename);
    printf("#line 1 \"%s\"\n", self->filename);
    //        puts("#ifdef DEBUG");
    //        printf("static const char* %s_filename =
    //        \"%s\";\n",self->mangledName, self->filename);
    //        printf("static const char* %s_basename =
    //        \"%s\";\n",self->mangledName, self->basename);
    //        printf("static const char* %s_modname = \"%s\";\n",
    //        self->mangledName,self->moduleName); puts("#endif");
}
void Parser_genc_close(Parser* self)
{
    printf("#undef THISMODULE\n");
    printf("#endif // HAVE_%s\n", self->capsMangledName);
}


#define List_ASTExpr PtrList
#define List_ASTVar PtrList
#define List_ASTModule PtrList
#define List_ASTFunc PtrList
#define List_ASTType PtrList
#define List_ASTImport PtrList
#define List_ASTScope PtrList

void alloc_stat()
{
    stat(ASTImport);
    stat(ASTExpr);
    stat(ASTVar);
    stat(ASTType);
    stat(ASTScope);
    stat(ASTTypeSpec);
    stat(ASTFunc);
    stat(ASTModule);
    stat(List_ASTExpr);
    stat(List_ASTVar);
    stat(List_ASTModule);
    stat(List_ASTFunc);
    stat(List_ASTType);
    stat(List_ASTImport);
    stat(List_ASTScope);
    stat(Parser);

    fprintf(stderr, "Total nodes allocated %u B, used %u B (%.2f%%)\n",
        globalPool.cap, globalPool.used,
        globalPool.used * 100.0 / globalPool.cap);
}

#pragma mark - main
int main(int argc, char* argv[])
{
    if (argc == 1) {
        fprintf(stderr, "checkcc: no input files.\n");
        return 1;
    }
    bool printDiagnostics = (argc > 2 && *argv[2] == 'd') or false;

    // Stopwatch sw;
    ticks t0 = getticks();
    List(ASTModule) * modules;
    Parser* parser;
    if (access(argv[1], F_OK) == -1) {
        fprintf(stderr, "checkcc: file '%s' not found.\n", argv[1]);
        return 2;
    }
    parser = Parser_fromFile(argv[1], true);

    modules = parse(parser);

    if (parser->errCount or parser->warnCount) {
        //        fputs(equaltos, stderr);
        if (parser->errCount)
            fprintf(
                stderr, "\033[31m*** %d errors\033[0m\n", parser->errCount);
        if (parser->warnCount)
            fprintf(stderr, "\033[33m*** %d warnings\033[0m\n",
                parser->warnCount);
        //        fprintf(stderr, "    *Reading* the code helps,
        //        sometimes."); fputs(equaltos, stderr);
        return 1;
    };

    printf("#include \"checkstd.h\"\n");
    Parser_genc_open(parser);
    foreach (ASTModule*, mod, mods, modules)
        ASTModule_genc(mod,0);
    Parser_genc_close(parser);

    //    foreach (mod, mods, modules)
    //    gen(mod);

    if (printDiagnostics) {
        fprintf(stderr, "file %lu B\n", parser->end - parser->data);
        alloc_stat();
        fputs("total global counts:\n", stderr);
        fprintf(stderr, "calloc: %d\n", globalCallocCount);
        fprintf(stderr, "malloc: %d\n", globalMallocCount);
        fprintf(stderr, "strdup: %d\n", globalStrdupCount);
        fprintf(stderr, "realloc: %d\n", globalReallocCount);
        fprintf(stderr, "strlen: %d\n", globalStrlenCount);
        expralloc_stat();
    }

    if (printDiagnostics)
        fprintf(stderr, "time elapsed: %.3f\n",
            elapsed(getticks(), t0) / 1e6); // sw.print();

    return 0;
}
