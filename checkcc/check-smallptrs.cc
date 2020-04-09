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
// errors are reported, run the token-based linter (formatter).
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

#include <cassert>
#include <cctype>
#include <climits>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include "cycle.h"

#define STEP 4

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

static int globalCallocCount = 0;
#define calloc(n, s)                                                           \
    calloc(n, s);                                                              \
    globalCallocCount++;
static int globalMallocCount = 0;
#define malloc(s)                                                              \
    malloc(s);                                                                 \
    globalMallocCount++;
static int globalStrdupCount = 0;
#define strdup(s)                                                              \
    strdup(s);                                                                 \
    globalStrdupCount++;
static int globalReallocCount = 0;
#define realloc(ptr, s)                                                        \
    realloc(ptr, s);                                                           \
    globalReallocCount++;
static int globalStrlenCount = 0;
#define strlen(s)                                                              \
    strlen(s);                                                                 \
    globalStrlenCount++;

#pragma mark - String Functions

char* str_noext(char* str)
{
    char* s = strdup(str);
    const size_t len = strlen(s);
    char* sc = s + len;
    while (sc > s and *sc != '.') sc--;
    if (sc >= s) *sc = '\0';
    return s;
}

char* str_base(char* str, char sep, size_t slen)
{
    if (!slen)
        return str; // you should pass in the len. len 0 is actually valid
                    // since basename for 'mod' is 'mod' itself, and this
                    // would have caused a call to strlen below. so len 0
                    // now means really just return what came in.
    char* s = str;
    //    if (!len) len = strlen(s);
    char* sc = s + slen;
    while (sc > s and sc[-1] != sep) sc--;
    if (sc >= s) s = sc;
    return s;
}

char* str_dir(char* str)
{
    char* s = strdup(str);
    const size_t len = strlen(s);
    char* sc = s + len;
    while (sc > s and *sc != '/') sc--;
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

void str_tr_ip(
    char* str, const char oldc, const char newc, const size_t length = 0)
{

    char* sc = str - 1;
    char* end = length ? str + length : (char*)0xFFFFFFFFFFFFFFFF;
    while (*++sc && sc < end)
        if (*sc == oldc) *sc = newc;
}

char* str_tr(char* str, const char oldc, const char newc)
{
    char* s = strdup(str);
    str_tr_ip(s, oldc, newc);
    return s;
}

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

#pragma mark - Stack


#pragma mark - Pool

// static size_t globalMemAllocBytes = 0;
// static size_t globalMemUsedBytes = 0;

template <class T, int elementsPerBlock = 512>
struct Pool {
    T* ref = NULL;
    uint32_t cap__unused = 0, total = 0;
//    Stack<T*> ptrs;

    uint32_t count = 0;/*
    T* alloc()
    {
        if (!ref or count >= elementsPerBlock) {
            if (ref) ptrs.push(ref);
            ref = (T*)malloc(elementsPerBlock * sizeof(T));
            count = 0;
            //            globalMemAllocBytes += elementsPerBlock *
            //            sizeof(T);
        }
        total++;
        //        globalMemUsedBytes += sizeof(T);
        return &ref[count++];
    }
    ~Pool()
    {
        for (int j = 0; j < count; j++) {
            T* obj = &(ref[j]);
            obj->~T();
        }
        free(ref);

        int k = 0;
        for (int i = 0; i < ptrs.count; i++) {
            for (int j = 0; j < elementsPerBlock; j++) {
                if (++k >= total) break;
                T* obj = &(ptrs[i][j]);
                obj->~T();
            }
            free(ptrs[i]);
        }
    }
*/
    void stat()
    {
        fprintf(stderr, "*** %-32s %4ld B x %5d = %7ld B \n",
            T::_typeName, sizeof(T), total, total * sizeof(T)
           /* ptrs.count + (ref ? 1 : 0)*/ );
    }
};

#define min(a, b) ((a) < (b)) ? (a) : (b)
#define max(a, b) ((a) > (b)) ? (a) : (b)
#define KB *1024
struct aptr {    uint32_t p:24, id: 8;
};

struct PoolB {
    char* base[32] /*= NULL*/; // this is NOT a string! char* used only for
                       // straightforward arithmetic
    uint32_t cap = 0, used = 0, total=0;
    int currid=-1;
    // to reset the pool for fresh use, just set used=0. This way you dont
    // have to call malloc/realloc again. For instance if you are done parsing
    // a file and want to load a next one, instead of freeing the pool and
    // reallocating a new one, just reset the existing pool.

    aptr salloc(size_t reqd) {
        char* get = alloc(reqd);
        return (aptr){ get - base[currid], currid };
    }
    char* alloc(size_t reqd)
    {
        char* ans = NULL;
        // This is a pool for single objects, not arrays or large strings.
        // dont ask for a big fat chunk larger than 16KB (or up to 256KB
        // depending on how much is already there) all at one time.
        size_t need = used + reqd;
        if (need > cap) {
            cap = max(reqd, cap > 64 KB ? 256 KB : 16 KB);
            base[++currid] = (char*)malloc( cap);
            used=0;
            assert(base[currid] != NULL);
        }
        ans = base[currid] + used;
        used += reqd;
        total+=reqd;
        return ans;
    }

     char* getPtr(aptr smallPtr)
    {
        // ptr is in steps of sizeof(void*), not in steps of any sizeof(T*)
//        if (smallPtr > used)
//            return NULL;
        return base[smallPtr.id] + smallPtr.p;

        // because ptr 0 is treated as invalid, return NULL here.
    }

//     uint32_t getSmallPtr(void* ptr) {
////        uint32_t ret= ;
////        if (ret > 500000)
////            return 0;
//         if (!ptr or (ptr < base or ptr > (base + used)))
//             return 0;
//         uint32_t uret = (uint32_t) ((char*)ptr - base);
//         if (uret>50000)
//             return uret;
//        return uret;
//    }

    ~PoolB() { free(base); }
};

PoolB gObjPool, gStrPool;

////typedef uint32_t aptr;

//// 32-bit pointer within a local pool

template <class T>
struct SmallPtr {
    aptr ptr;
    inline operator uint32_t&() { return ptr; };
    inline T* operator->() {
        return (T*)gObjPool.getPtr(ptr);
    }
    inline T* operator()() {
        return (T*)gObjPool.getPtr(ptr);
    }
//    inline SmallPtr<T>& operator=(const T* const p)
//    {
//        ptr = gObjPool.getSmallPtr((void*)p);
//        return *this;
//    }
};

struct SmallCharPtr {
    aptr ptr;
//    inline operator uint32_t&() { return ptr; };
    inline const char* operator()() const {
        return gStrPool.getPtr(ptr);
    }
//    inline SmallCharPtr& operator=(const char* const p)
//    {
//        ptr = gStrPool.getSmallPtr((void*)p);
//        return *this;
//    }
};


template <class T>
struct Ptr {
    T* ptr;
    inline operator T*() { return ptr; };
    inline T* operator()() {
        return ptr; //(T*)gObjPool.getPtr(ptr);

    }
    inline T* operator->() {
        return ptr; //(T*)gObjPool.getPtr(ptr);
        
    }
    inline Ptr<T>& operator=(const T* const p)
    {
        ptr = (T*)p; //gObjPool.getSmallPtr((void*)p);
        return *this;
    }
};

struct  CharPtr {
    char* ptr;
    inline operator char*() { return ptr; };
    inline const char* operator()() const {
        return ptr; //gStrPool.getPtr(ptr);
    }
    inline CharPtr& operator=(const char* const p)
    {
        ptr = (char*)p; //gStrPool.getSmallPtr((void*)p);
        return *this;
    }
};


#define SMALLPTRS

#ifndef SMALLPTRS

#define SmallPtr Ptr
#define SmallCharPtr CharPtr
#define GETSOBJPTR(x) x
#define GETSSTRPTR(x) x
#define nil(T) (Ptr<T>){NULL}

#else

#define GETSOBJPTR(x) gObjPool.getSmallPtr(x)
#define GETSSTRPTR(x) gStrPool.getSmallPtr(x)
#define nil(T) (SmallPtr<T>){0}

#endif

template <class T, int initialSize = 8>
class Stack {
    T* items = NULL;
    uint32_t cap = 0;
    
public:
    uint32_t count = 0;
    T& operator[](int index) { return items[index]; }
    ~Stack<T, initialSize>()
    {
        if (cap) free(items);
    }
    void push(T node)
    {
        if (count < cap) {
            items[count++] = node;
        } else {
            cap = cap ? 2 * cap : initialSize;
            items = (T*)realloc(items, sizeof(T) * cap);
            // !! realloc can NULL the ptr!
            items[count++] = node;
#ifndef SMALLPTRS
            for (int i = count; i < cap; i++) items[i] = NULL;
#endif
            
        }
    }
    
    T pop()
    {
        T ret;
#ifndef SMALLPTRS
        ret = NULL;
#endif
        if (count) {
            ret = items[count - 1];
#ifndef SMALLPTRS
            items[count - 1] = NULL;
#endif
            count--;
        } else {
            printf("error: pop from empty list\n");
        }
        return ret;
    }
    
    T top() {
//#ifndef SMALLPTRS
//        return count ? items[count - 1] : NULL;
//#else
        return items[count-1];
//#endif
    }
    
    bool empty() { return count == 0; }
};


// template <class T> PoolB * SmallPtr<T>::myPool = &gObjPool; // sane or
// insane default?
// static_assert(sizeof(SmallPtr<void>) == 4, ""); // for any type T actually

// for C
// #define SmallPtr<T> uint32_t // what did you expect?
// #define GetSmallPtr<ptr> (gObjPool.getSmallPtr<ptr>)
// #define GetPtr(smallPtr) (gObjPool.getPtr(smallPtr))
// #define GetPtrAs(T, smallPtr) ((T*)gObjPool.getPtr(smallPtr))

// #define SmallCharPtr uint32_t
// #define GetSmallCharPtr(ptr) (gStrPool.getSmallPtr<ptr>)
// #define GetCharPtr(smallPtr) (gStrPool.getPtr(smallPtr))

// TODO: change this to remove Pool<T, S>. keep count elsewhere
#define STHEAD_POOLB(T, s)                                                     \
    static Pool<T, s> pool;                                                    \
    static void* ___operator__new(size_t size)                                            \
    {                                                                          \
        pool.total++;                                                          \
        return gObjPool.alloc(size);                                           \
}      static SmallPtr<T> snew() {pool.total++;aptr p= gObjPool.salloc(sizeof(T));return (SmallPtr<T>){p}; } \
    static const char* _typeName;

// individual pools
#define __UNUSED__STHEAD_POOL(T, s)                                            \
    static Pool<T, s> pool;                                                    \
    void* operator new(size_t size) { return pool.alloc(); }                   \
    static const char* _typeName;

#define NAME_CLASS(T) const char* T::_typeName = #T;

#define STHEAD(T, s) STHEAD_POOLB(T, s) // common pool for all
//#define STHEAD(T, s) STHEAD_POOL(T, s) // own pool for each class

#pragma mark - List

template <class T>
struct PtrListItem {

    STHEAD(PtrListItem<T>, 512)

    T* item = NULL;

    PtrListItem<T>* next = NULL;

    PtrListItem<T>() {}
    PtrListItem<T>(T* item) { this->item = item; }

    void append(T* item)
    { // adds a single item, creating a wrapping list item holder.
        if (!this->item)
            this->item = item;
        else
            append(PtrListItem<T>(item));
    }

    void append(PtrListItem<T> listItem)
    {
        // adds a list item, including its next pointer, effectively
        // concatenating this with listItem
        auto li = new PtrListItem<T>;
        *li = listItem;
        PtrListItem<T>* last = this;
        while (last->next) last = last->next;
        last->next = li;
    }
};
template <class T>
Pool<PtrListItem<T> > PtrListItem<T>::pool;

template <class T>
struct SmallPtrListItem {

    STHEAD(SmallPtrListItem<T>, 512)

    SmallPtr<T> item = { 0 };

    SmallPtr<SmallPtrListItem<T> > next = { 0 };

    SmallPtrListItem<T>() {}
    SmallPtrListItem<T>(SmallPtr<T> item) { this->item = item; }

    void append(SmallPtr<T> item)
    { // adds a single item, creating a wrapping list item holder.
        if (!this->item)
            this->item = item;
        else
            append(SmallPtrListItem<T>(item));
    }

    void append(SmallPtrListItem<T> listItem)
    {
        // adds a list item passed by value, including its next pointer,
        // effectively concatenating this with listItem
        auto li = new SmallPtrListItem<T>;
        *li = listItem; //*GetPtr(listItem);
        SmallPtrListItem<T>* last = this;
        while (last->next) last = last->next();
        last->next = li;
    }

    // void append(SmallPtr<SmallPtrListItem<T>> listItem)
    // {
    //     // adds a list item passed by its small pointer, including its next
    //     pointer, effectively
    //     // concatenating this with listItem
    //     auto li = new SmallPtrListItem<T>;
    //     *li = *(listItem()); //*GetPtr(listItem);
    //     SmallPtrListItem<T>* last = this;
    //     while (last->next) last = last->next;
    //     last->next = li;
    // }
};
template <class T>
Pool<SmallPtrListItem<T> > SmallPtrListItem<T>::pool;

#define foreach(var, listp, listSrc)                                           \
    auto listp = &(listSrc);                                                   \
    for (auto var = listp->item; listp && (var = listp->item);                 \
         listp = listp->next())

#pragma mark - Token Kinds

enum TokenKind {
    TKNullChar,
    TKKeyword_cheater,
    TKKeyword_for,
    TKKeyword_while,
    TKKeyword_if,
    TKKeyword_end,
    TKKeyword_function,
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
    TKKeyword_base,
    TKKeyword_var,
    TKKeyword_let,
    TKKeyword_import,
    TKIdentifier,
    TKFunctionCall,
    TKSubscript,
    TKNumber,
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
};

// Return the repr of a token kind (for debug)
const char* TokenKind_repr(const TokenKind kind, bool spacing = true)
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
    case TKKeyword_base:
        return "base";
    case TKKeyword_var:
        return "var";
    case TKKeyword_let:
        return "let";
    case TKKeyword_import:
        return "import";
    case TKKeyword_return:
        return "return ";
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
        return " = ";
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
    }
    printf("unknown kind: %d\n", kind);
    return "(!unk)";
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

// Holds information about a syntax token.
struct Token {

    char* pos = NULL;
    uint32_t matchlen : 24;
    struct {
        bool skipWhiteSpace : 1,
            mergeArrayDims : 1, // merge [:,:,:] into one token
            noKeywosrdDetect : 1, // leave keywords as idents
            strictSpacing : 1; // missing spacing around operators etc. is a
                               // compile error YES YOU HEARD IT RIGHT
                               // but why need this AND skipWhiteSpace?
    } flags = { true, false, false };
    uint16_t line = 1;
    uint8_t col = 1;
    TokenKind kind : 8;

    Token()
        : kind(TKUnknown)
        , matchlen(0)
    {
    }
    const char* repr() { return TokenKind_repr(kind); }
    //    inline char* dup() const
    //    {
    //        return strndup(pos, matchlen);
    //    }

    // Advance the token position by one char.
    inline void advance1() { pos++; }

    // Peek at the char after the current (complete) token
    char peekCharAfter()
    {
        char* s = pos + matchlen;
        if (flags.skipWhiteSpace)
            while (*s == ' ') s++;
        return *s;
    }

#define Token_compareKeyword(tok)                                              \
    if (sizeof(#tok) - 1 == l and not strncmp(#tok, s, l)) {                   \
        kind = TKKeyword_##tok;                                                \
        return;                                                                \
    }

    // Check if an (ident) token matches a keyword and return its type
    // accordingly.
    void tryKeywordMatch()
    {
        if (/*flags.noKeywordDetect or */ kind != TKIdentifier) return;

        const char* s = pos;
        const int l = matchlen;

        Token_compareKeyword(and)
        Token_compareKeyword(cheater)
        Token_compareKeyword(for)
        Token_compareKeyword(do)
        Token_compareKeyword(while)
        Token_compareKeyword(if)
        Token_compareKeyword(then)
        Token_compareKeyword(end)
        Token_compareKeyword(function)
        Token_compareKeyword(test)
        Token_compareKeyword(not)
        Token_compareKeyword(and)
        Token_compareKeyword(or)
        Token_compareKeyword(in)
        Token_compareKeyword(else)
        Token_compareKeyword(type)
//        Token_compareKeyword(check)
        Token_compareKeyword(base)
        Token_compareKeyword(var)
        Token_compareKeyword(let)
        Token_compareKeyword(import)
        Token_compareKeyword(return)
        Token_compareKeyword(as)
        //        Token_compareKeyword(print);
        // return TKIdentifier;
    }

    // Get the type based on the char at the current position.
    // inline TokenKind getTypeAtCurrentPos() { return getType(0); }

    // Get the type based on the char after the current position (don't
    // conflate this with char* Token_peekCharAfter(token))
    // inline TokenKind getTypeAtNextChar() { return getType(1); }
    //    inline TokenKind peek_prevchar() { return getType(-1); }

    // Get the token kind based only on the char at the current position (or
    // an offset).
    TokenKind getType(const size_t offset = 0)
    {
        const char c = pos[offset];
        const char cn = c ? pos[1 + offset] : 0;
        TokenKind ret = (TokenKind)TokenKindTable[c];
        switch (c) {
        case '<':
            switch (cn) {
            case '=':
                return TKOpLE;
            case '>':
                return TKOpNE; // NE variants are !=, <>, =/ (default)
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
            case '/':
                return TKOpNE;
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
        default:
            return ret;
        }
    }

    void detect()
    {
        TokenKind tt = getType();
        TokenKind tt_ret = TKUnknown; // = tt;
        static TokenKind tt_last
            = TKUnknown; // the previous token that was found
        static TokenKind tt_lastNonSpace
            = TKUnknown; // the last non-space token found
        TokenKind tmp;
        char* start = pos;
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
                advance1();
                tt = getType();
                if (tt == TKNullChar or tt == tmp) {
                    advance1();
                    break;
                }
                if (tt == TKBackslash)
                    if (getType(1) == tmp) { // why if?
                        advance1();
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
            //            tt=peek_prevchar();

            if (tt_last == TKOneSpace) // if prev char was a space return
                                       // this as a run of spaces
                while (tt != TKNullChar) {
                    // here we dont want to consume the end char, so break
                    // before
                    tt = getType(1);
                    advance1();
                    if (tt != TKSpaces) break;
                }
            else
                advance1();
            // else its a single space
            tt_ret = TKSpaces;
            break;

        case TKOpComma:
        case TKOpSemiColon:
            //        line continuation tokens
            tt_ret = tt;

            while (tt != TKNullChar) {
                tt = getType(1);
                advance1();
                // line number should be incremented for line continuations
                if (tt == TKSpaces) {
                    found_spc++;
                }
                if (tt == TKExclamation) {
                    found_cmt = true;
                }
                if (tt == TKNewline) {
                    line++;
                    col = -found_spc - 1; // account for extra spaces
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
            //        case TKOpAssign:
            //        case TKParenOpen:
            //        case TKArrayOpen:
            //        case TKBraceOpen:
            //            while (tt != TKNullChar) {
            //                tt = getTypeAtNextChar();
            ////                advance1();
            //                // line number should be incremented for line
            //                continuations if (tt == TKSpaces) {
            //                    found_spc++;
            //                }
            //                if (tt == TKExclamation) {
            //                    found_cmt = true;
            //                }
            //                if (tt == TKNewline) {
            //                    line++;
            //                    col = -found_spc - 1; // account for extra
            //                    spaces
            //                                          // after , and for
            //                                          nl itself
            //                    found_spc = 0;
            //                }
            //                if (found_cmt and tt != TKNewline) {
            //                    found_spc++;
            //                    continue;
            //                }
            //                if (tt != TKSpaces and tt != TKNewline) break;
            //                advance1();
            //            }
            //            if (tt_ret!=TKArrayOpen) break;

        case TKArrayOpen:
            // mergearraydims should be set only when reading func args
            if (not flags.mergeArrayDims) goto defaultToken;

            while (tt != TKNullChar) {
                tt = getType(1);
                advance1();
                if (tt != TKOpColon and tt != TKOpComma) break;
            }
            tt = getType();
            if (tt != TKArrayClose) {
                fprintf(
                    stderr, "expected a ']', found a '%c'. now what?\n", *pos);
            }
            advance1();
            tt_ret = TKArrayDims;
            break;

        case TKAlphabet:
        case TKPeriod:
        case TKUnderscore:
            while (tt != TKNullChar) {
                tt = getType(1);
                advance1();
                if (tt != TKAlphabet //
                    and tt != TKDigit //
                    and tt != TKUnderscore and tt != TKPeriod)
                    break; /// validate in parser not here
            }
            tt_ret = TKIdentifier;
            break;

        case TKHash: // TKExclamation:
            while (tt != TKNullChar) {
                tt = getType(1);
                advance1();
                if (tt == TKNewline) break;
            }
            tt_ret = TKLineComment;
            break;

        case TKPipe:
            while (tt != TKNullChar) {
                tt = getType(1);
                advance1();
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
                tt = getType(1);
                // numbers such as 1234500.00 are allowed
                // very crude, error-checking is parser's job
                //            if (tt==TKDigit //
                //                or tt==TKPlus //
                //                or tt==TKMinus //
                //                or tt==TKPeriod //
                //                or *token.pos == 'e' //
                //                or *token.pos == 'E' //
                //                or *token.pos == 'd'
                //                or *token.pos == 'D')
                advance1();

                if (*pos == 'e' //
                    or *pos == 'E' //
                    or *pos == 'd'
                    or *pos == 'D') { // will all be changed to e btw
                    found_e = true;
                    continue;
                }
                if (found_e) {
                    //}and (*token.pos == '-' or *token.pos ==
                    //'+'))
                    //{
                    found_e = false;
                    continue;
                }
                if (tt == TKPeriod) {
                    found_dot = true;
                    continue;
                }
                if (found_dot and tt == TKPeriod) tt_ret = TKMultiDotNumber;

                if (tt != TKDigit and tt != TKPeriod and *pos != 'i') break;
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
            advance1();
            break;

        case TKOpNotResults:
            // 3-char tokens
            advance1();
        case TKOpEQ:
        case TKOpGE:
        case TKOpLE:
        case TKOpNE:
        case TKOpResults:
        case TKBackslash:
            // 2-char tokens
            advance1();
        default:
        defaultToken:
            tt_ret = tt;
            advance1();
            break;
        }

        matchlen = (uint32_t)(pos - start);
        pos = start;
        kind = tt_ret;

        if (kind == TKIdentifier) tryKeywordMatch();

        if (kind == TKSpaces and matchlen == 1) kind = TKOneSpace;

        tt_last = kind;
        if (tt_last != TKOneSpace and tt_last != TKSpaces)
            tt_lastNonSpace = tt_last;
    }

    // Advance to the next token (skip whitespace if `skipws` is set).
    void advance()
    {
        switch (kind) {
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
        case TKKeyword_base:
        case TKKeyword_var:
        case TKKeyword_let:
        case TKKeyword_import:
        case TKUnknown: // bcz start of the file is this
            break;
        default:
            *pos = 0; // trample it so that idents etc. can be assigned
                      // in-situ
        }

        pos += matchlen;
        col += matchlen;
        matchlen = 0;
        detect();

        if (kind == TKNewline) {
            line++;
            col = 0; // position of the nl itself is 0
        }
        if (flags.skipWhiteSpace
            and (kind == TKSpaces
                or (flags.strictSpacing and kind == TKOneSpace)))
            advance();
    }
};

const char* const spaces = //
    "                                                                     ";
const char* const dashes = //
    "\n_________________________________________________________________\n";
const char* const equaltos = //
    "\n=================================================================\n";

#pragma mark - AST Import

struct ASTImport { // canbe 8
    STHEAD(ASTImport, 32)

    SmallCharPtr importFile, alias;

    // generally this is not changed (the alias), at least not by the
    // linter. So we can only store the offset from importFile. In general
    // it would be nice to allocate the file contents buffer with a little
    // extra space and use that as a string pool. This way we can make new
    // strings while still referring to them using a uint32.
    bool isPackage = false, hasAlias = false;

    void gen(int level)
    {
        printf("import %s%s%s%s\n", isPackage ? "@" : "", importFile(),
            hasAlias ? " as " : "", hasAlias ? alias() : "");
    }
};

#pragma mark - AST Units

struct ASTUnits {

    STHEAD(ASTUnits, 32)

    uint8_t powers[7], something;
    double factor, factors[7]; // maybe getrid of factors[7]
    char* label;
    void gen(int level) { printf("|%s", label); }
};

struct ASTTypeSpec;
struct ASTType;
struct ASTFunc;
struct ASTScope;
struct ASTExpr;
struct ASTVar;

#pragma mark - AST TypeSpec

struct ASTTypeSpec {

    STHEAD(ASTTypeSpec, 128)

    union {
        SmallPtr<ASTType> type = { 0 };
        SmallCharPtr name;
        SmallPtr<ASTUnits> units;
        // you know that if this is set, then type can only be Number anyway
    };

    uint32_t dims = 0;
    enum TypeSpecStatus {
        TSUnresolved, // name ptr is set
        TSResolved, // type ptr is set and points to the type
        TSDimensionedNumber // type can only be Number, units ptr is set
                            // if more (predefined) number types can use
                            // units, add them here
    };
    TypeSpecStatus status = TSUnresolved;
    // if false, name is set, else type is set
#define DEREFAS(T, p) ((T*)DEREF(p))
    void gen(int level = 0)
    {
        if (status == TSUnresolved) printf("%s", name());
        //        if (status==TSResolved) printf("%s", type->name);
        if (dims) printf("%s", "[]" /*dimsGenStr(dims)*/);
        if (status == TSDimensionedNumber) units->gen(level);
    }
    /*
        const char* dimsGenStr(int32_t dims)
        {
            switch (dims) {
            case 0:
                return "";
            case 1:
                return "[]";
            case 2:
                return "[:,:]";
            case 3:
                return "[:,:,:]";
            case 4:
                return "[:,:,:,:]";
            case 5:
                return "[:,:,:,:,:]";
            case 6:
                return "[:,:,:,:,:,:]";
            default:
                int32_t i;
                int32_t sz = 2 + dims + (dims ? (dims - 1) : 0) + 1;
                char* str = (char*)malloc(sz * sizeof(char));
                str[sz * 0] = '[';
                str[sz - 1] = 0;
                for (i = 0; i < dims; i++) {
                    str[i * 2 + 1] = ':';
                    str[i * 2 + 2] = ',';
                }
                str[sz - 2] = ']';
                return str;
            }
        } */
};

#pragma mark - AST Var
static uint32_t exprsAllocHistogram[128];

void expralloc_stat()
{
    int iexpr, sum = 0, thres = 0;
    for (int i = 0; i < 128; i++) sum += exprsAllocHistogram[i];
    thres = sum / 20;
    fprintf(
        stderr, "Expr alloc distribution (>5%%):\n  Kind        #      %%\n");
    for (int i = 0; i < 128; i++) {
        if ((iexpr = exprsAllocHistogram[i]) > thres)
            fprintf(stderr, "  %-8s %4d %6.2f\n",
                TokenKind_repr((TokenKind)i, false), iexpr,
                iexpr * 100.0 / sum);
    }
}

struct ASTVar {

    STHEAD(ASTVar, 128)

    SmallPtr<ASTTypeSpec> typeSpec={0};
    SmallPtr<ASTExpr> init={0};
    SmallCharPtr name={0}; // = NULL;

    struct {
        bool unused : 1, //
            unset : 1, //
            isLet : 1, //
            isVar : 1, //
            isTarget : 1;
        /* x = f(x,y) */ //
    } flags = {0,0,0,0,0};

    void gen(int level = 0);
};

struct ASTExpr {

    STHEAD(ASTExpr, 1024)

    struct {
        uint16_t line = 0;
        union {
            struct {
                bool opIsUnary, opIsRightAssociative;
            };
            uint16_t strLength;
        }; // for literals, the string length
        uint8_t opPrecedence=0;
        uint8_t col = 0;
        TokenKind kind : 8; // TokenKind must be updated to add
                            // TKFuncCallResolved TKSubscriptResolved etc.
                            // to indicate a resolved identifier. while you
                            // are at it add TKNumberAsString etc. then
                            // instead of name you will use the corr.
                            // object. OR instead of extra enum values add a
                            // bool bit resolved
    };

    //    SmallPtr<ASTExpr> left ; //= NULL;

    union {
        union {
            char* string; // use only for temporary ops on strings, then set label
            double real;
            int64_t integer;
            uint64_t uinteger;
        } value; // for terminals
        struct {
            SmallPtr<ASTExpr> left;
            union {
                SmallPtr<ASTExpr> right;
                SmallCharPtr label; // name // for unresolved functioncall or subscript, or
                                    // identifiers. will take 8 bytes, so no point reducing
                SmallPtr<ASTFunc> func; // for functioncall
                SmallPtr<ASTVar> var; // for array subscript, or to refer to a variable
                SmallPtr<ASTScope> body; // for if/for/while
                };
        };
        /* struct {uint32_t left, right}; // for 32-bit ptrs to local pool
         */
    };

    ASTExpr() {}
    ASTExpr(const Token* token)
    {
        kind = token->kind;
        line = token->line;
        col = token->col;

        opPrecedence = TokenKind_getPrecedence(kind);
        if (opPrecedence) {
            opIsRightAssociative = TokenKind_isRightAssociative(kind);
            opIsUnary = TokenKind_isUnary(kind);
        }

        exprsAllocHistogram[kind]++;
        
        value.integer=0;

        switch (kind) {
        case TKIdentifier:
        case TKString:
        case TKRegex:
        case TKNumber:
        case TKMultiDotNumber:
        case TKLineComment: // Comments go in the AST like regular stmts
            strLength = (uint16_t)token->matchlen;
                value.string = token->pos;
            break;
        default:;
        }
        // the '!' will be trampled
        if (kind == TKLineComment) value.string++;
        // turn all 1.0234[DdE]+01 into 1.0234e+01.
        if (kind == TKNumber) {
            str_tr_ip(value.string, 'd', 'e', strLength);
            str_tr_ip(value.string, 'D', 'e', strLength);
            str_tr_ip(value.string, 'E', 'e', strLength);
        }
        if (value.string) label = value.string;
    }

    void gen(int level = 0, bool spacing = true);
};

void ASTVar::gen(int level)
{
    printf("%.*s%s%s", level, spaces,
        flags.isVar ? "var " : flags.isLet ? "let " : "", name());
    if (typeSpec) {
        printf(" as ");
        typeSpec->gen(level + STEP);
    } else
        printf(" as UnknownType");
    if (init) {
        printf(" = ");
        init->gen(0);
    }
}

#pragma mark - AST Scope

struct ASTScope {

    STHEAD(ASTScope, 32)

    SmallPtrListItem<ASTExpr> stmts;
    SmallPtrListItem<ASTVar> locals;
    SmallPtr<ASTScope> parent={0}; // = NULL;
    void gen(int level);
};

void ASTScope::gen(int level)
{
    foreach (local, locals, this->locals) {
        local->gen(level);
        puts("");
    }
    foreach (stmt, stmts, this->stmts) {
        stmt->gen(level);
        puts("");
    }
}

#pragma mark - AST Expr

void ASTExpr::gen(int level, bool spacing)
{
    // generally an expr is not split over several lines (but maybe in
    // rare cases). so level is not passed on to recursive calls.
    printf("%.*s", level, spaces);

    switch (kind) {
    case TKIdentifier:
    case TKNumber:
    case TKMultiDotNumber:
    case TKRegex:
    case TKInline:
    case TKString:
        printf("%.*s", strLength, label());
        break;

    case TKLineComment:
        printf("%s%.*s", TokenKind_repr(TKLineComment, *label() != ' '),
            strLength, label());
        break;

    case TKFunctionCall:
        printf("%.*s(", strLength, label());
        if (left) left->gen(0, false);
        printf(")");
        break;

    case TKSubscript:
        printf("%.*s[", strLength, label());
        if (left) left->gen(0, false);
        printf("]");
        break;

    case TKKeyword_for:
    case TKKeyword_if:
    case TKKeyword_while:
        printf("%s ", TokenKind_repr(kind));
        if (left) left->gen(0);
        puts("");
        if (body) body->gen(level + STEP);
        printf("%.*send %s", level, spaces, TokenKind_repr(kind));
        break;

    default:
        if (not opPrecedence) break;
        // not an operator, but this should be error if you reach here
        bool leftBr = left and left->opPrecedence
            and left->opPrecedence < this->opPrecedence;
        bool rightBr = right and right->opPrecedence
            and right->kind != TKKeyword_return // found in 'or return'
            and right->opPrecedence < this->opPrecedence;

        if (kind == TKOpColon) {
            // expressions like arr[a:x-3:2] should become
            // arr[a:(x-3):2]
            // or list literals [8, 9, 6, 77, sin(c)]
            if (left) switch (left->kind) {
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
            if (right) switch (right->kind) {
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

        if (false and kind == TKKeyword_return and right) {
            switch (right->kind) {
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

        if (kind == TKPower and not spacing) putc('(', stdout);

        char lpo = leftBr and left->kind == TKOpColon ? '[' : '(';
        char lpc = leftBr and left->kind == TKOpColon ? ']' : ')';
        if (leftBr) putc(lpo, stdout);
        if (left) left->gen(0, spacing and !leftBr and kind != TKOpColon);
        if (leftBr) putc(lpc, stdout);

        printf("%s", TokenKind_repr(kind, spacing));

        char rpo = rightBr and right->kind == TKOpColon ? '[' : '(';
        char rpc = rightBr and right->kind == TKOpColon ? ']' : ')';
        if (rightBr) putc(rpo, stdout);
        if (right) right->gen(0, spacing and !rightBr and kind != TKOpColon);
        if (rightBr) putc(rpc, stdout);

        if (kind == TKPower and not spacing) putc(')', stdout);
        if (kind == TKArrayOpen) putc(']', stdout);
    }
}

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
    inline uintptr_t getPtr() { return (uiptr & 0xFFFFFFFFFFFFFFF8); }
    uint8_t kind() { return typ; } // this only takes values 0-7, 3 bits
    //--
    ASTVar* var() { return (ASTVar*)getPtr(); }
    ASTExpr* expr() { return (ASTExpr*)getPtr(); }
};

#pragma mark - AST Type

struct ASTType {

    STHEAD(ASTType, 32)

    SmallPtrListItem<ASTVar> vars;
    SmallPtr<ASTTypeSpec> super={0};
    SmallCharPtr name={0};

    SmallPtrListItem<ASTExpr> checks; // invariants

    SmallPtrListItem<ASTVar> params; // params of this type

    void gen(int level = 0)
    {
        printf("type %s\n", name());

        if (super) {
            printf("    base ");
            super->gen(level);
            puts("");
        }

        foreach (var, vars, this->vars) {
            if (!var) continue;
            var->gen(level + STEP);
            puts("");
        }

        foreach (check, checks, this->checks) {
            if (!check) continue;
            check->gen(level + STEP);
            puts("");
        }

        puts("end type\n");
    }
};

#pragma mark - AST Func

struct ASTFunc {

    STHEAD(ASTFunc, 64)

    SmallPtr<ASTScope> body={0};
    SmallPtrListItem<ASTVar> args;
    SmallPtr<ASTTypeSpec> returnType={0};
    SmallCharPtr mangledName={0};
    SmallCharPtr owner={0}; // if method of a type
    SmallCharPtr name={0};
    struct {
        uint16_t line;
        struct {
            uint16_t prints : 1, throws : 1, recurs : 1, net : 1, gui : 1,
                file : 1, refl : 1, nodispatch : 1;
        } flags;
        uint8_t col;
    };

    void gen(int level = 0)
    {
        printf("function %s(", name());

        foreach (arg, args, this->args) {
            arg->gen(level);
            printf(args->next ? ", " : ")");
        }

        if (returnType) {
            printf(" as ");
            returnType->gen(level);
        }
        puts("");

        body->gen(level + STEP);

        puts("end function\n");
    }
};

#pragma mark - AST Module

struct ASTModule {

    STHEAD(ASTModule, 16)

    SmallPtrListItem<ASTFunc> funcs;
    SmallPtrListItem<ASTExpr> exprs;
    SmallPtrListItem<ASTType> types;
    SmallPtrListItem<ASTVar> globals;
    SmallPtrListItem<ASTImport> imports;
    SmallPtrListItem<ASTFunc> tests;

    char* name;
    char* moduleName = NULL; // mod.submod.xyz.mycode
    char* mangledName = NULL; // mod_submod_xyz_mycode
    char* capsMangledName = NULL; // MOD_SUBMOD_XYZ_MYCODE

    void gen(int level = 0)
    {
        printf("! module %s\n", name);

        foreach (import, imports, this->imports)
            import->gen(level);

        puts("");

        foreach (type, types, this->types)
            type->gen(level);

        foreach (func, funcs, this->funcs)
            func->gen(level);
    }
};

#pragma mark - Parser

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
        = true; // set to false when compiling, set to true when linting
    char* noext = NULL;
    Token token; // current
    SmallPtrListItem<ASTModule> modules; // module node of the AST
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
//        free(moduleName);
        free(mangledName);
        free(capsMangledName);
        free(dirname);
    }
    ~Parser() { fini(); }
    uint32_t errCount = 0, warnCount = 0;
    Parser(char* filename, bool skipws = true)
    {
        static const char* _funcSig
            = "%s:%d: Parser::Parser(filename = \"%s\", skipws = %s)";
        // the file and line should be of the call site, not the definition!
        // ie every func must return errcode in check generated source. if
        // err code is unhandled then backtrace. func prints its string?
        // caller needs to supply file,line as arg then. or else caller
        // prints, needs str ptr then. all str ptrs could be made const
        // static globals.

        static const auto FILE_SIZE_MAX = 1 << 24;
        FILE* file = fopen(filename, "r");
        size_t flen = strlen(filename);
        fprintf(stdout, "compiling %s\n", filename);
        this->filename = filename;
        noext = str_noext(filename);
        fseek(file, 0, SEEK_END);
        const size_t size = ftell(file) + 2;
        // 2 null chars, so we can always lookahead
        if (size < FILE_SIZE_MAX) {
            // data must be the first thing to be allocated in the gStrPool.
            data = gStrPool.alloc(size); //(char*)malloc(size);
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

        //_printbt:
        //    fprintf(stderr, _funcSig, __LINE__, filename, skipws);
    }

#pragma mark - Error Reporting

    static const auto errLimit = 10;
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
        fputs(dashes, stderr);
        fprintf(stderr,
            "(%d) error: %s at %s:%d:%d\n      expected '%s' found '%s'\n",
            errCount + 1, funcName, filename, token.line, token.col,
            TokenKind_repr(expected), token.repr());
        errorIncrement();
    }

#define errorParsingExpr() errorParsingExpr_(__func__)
    void errorParsingExpr_(const char* funcName)
    {
        fputs(dashes, stderr);
        fprintf(stderr, "(%d) error: %s at %s:%d\n      failed to parse expr",
            errCount + 1, funcName, filename,
            token.line - 1); // parseExpr will move to next line
        errorIncrement();
    }

#define errorUnexpectedToken() errorUnexpectedToken_(__func__)
    void errorUnexpectedToken_(const char* funcName)
    {
        fputs(dashes, stderr);
        fprintf(stderr,
            "(%d) error: %s at %s:%d:%d\n      unexpected token '%.*s'\n",
            errCount + 1, funcName, filename, token.line, token.col,
            token.matchlen, token.pos);
        errorIncrement();
    }

#define errorUnexpectedExpr(e) errorUnexpectedExpr_(e, __func__)
    void errorUnexpectedExpr_(const ASTExpr* expr, const char* funcName)
    {
        fputs(dashes, stderr);
        fprintf(stderr,
            "(%d) error: %s at %s:%d:%d\n      unexpected expr '%.*s'",
            errCount + 1, funcName, filename, expr->line, expr->col,
            expr->opPrecedence ? 100 : expr->strLength,
            expr->opPrecedence ? TokenKind_repr(expr->kind) : expr->label());
        errorIncrement();
    }

#pragma mark -

    SmallPtr<ASTExpr> exprFromCurrentToken()
    {
        auto ret = ASTExpr::snew();

//        SmallPtr<ASTExpr> ret = { GETSOBJPTR(expr) };
        token.advance();
        return ret;
    }

    SmallPtr<ASTExpr> next_token_node(
        TokenKind expected, const bool ignore_error)
    {
        if (token.kind == expected) {
            return exprFromCurrentToken();
        } else {
            if (not ignore_error) errorExpectedToken(expected);
            return (SmallPtr<ASTExpr>) { 0 };
        }
    }
    // these should all be part of Token_ when converted back to C
    // in the match case, token should be advanced on error
    SmallPtr<ASTExpr> match(TokenKind expected)
    {
        return next_token_node(expected, false);
    }

    // this returns the match node or null
    SmallPtr<ASTExpr> trymatch(TokenKind expected)
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

    SmallCharPtr parseIdent()
    {
        if (token.kind != TKIdentifier) errorExpectedToken(TKIdentifier);
        char* p = token.pos;
        token.advance();
        return (SmallCharPtr) {  GETSSTRPTR(p) };
    }

#pragma mark -
    SmallPtr<ASTExpr> parseExpr()
    {
        // there are 2 steps to this madness.
        // 1. parse a sequence of tokens into RPN using shunting-yard.
        // 2. walk the rpn stack as a sequence and copy it into a result
        // stack, collapsing the stack when you find nonterminals (ops, func
        // calls, array index, ...)
        SmallPtr<ASTExpr> ret ;
        // we could make this static and set len to 0 upon func exit
        static Stack<SmallPtr<ASTExpr>, 32> rpn, ops, result;
        int prec_top = 0;
        SmallPtr<ASTExpr> p = {0};
        TokenKind revBrkt = TKUnknown;

        // ******* STEP 1 CONVERT TOKENS INTO RPN

        while (token.kind != TKNullChar and token.kind != TKNewline
            and token.kind != TKLineComment) { // build RPN

            // you have to ensure that ops have a space around them, etc.
            // so don't just skip the one spaces like you do now.
            if (token.kind == TKOneSpace) token.advance();

            SmallPtr<ASTExpr> expr = (SmallPtr<ASTExpr>) {GETSOBJPTR(new ASTExpr(&token))}; // dont advance yet
            int prec = expr->opPrecedence;
            bool rassoc = expr->opIsRightAssociative;
            char lookAheadChar = token.peekCharAfter();

            switch (expr->kind) {
            case TKIdentifier:
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
                }
                break;
            case TKParenOpen:
                ops.push(expr);
                if (not ops.empty() and ops.top()->kind == TKFunctionCall)
                    rpn.push(expr);
                // instead of marking with (, could consider pushing a NULL.
                // (not for func calls & array indexes -- for grouping)
                break;

            case TKArrayOpen:
                ops.push(expr);
                if (not ops.empty() and ops.top()->kind == TKSubscript)
                    rpn.push(expr);
                break;

            case TKParenClose:
            case TKArrayClose:
            case TKBraceClose:

                revBrkt = TokenKind_reverseBracket(expr->kind);
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
                    rpn.push(nil(ASTExpr));
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

                            rpn.push(nil(ASTExpr)); // indicates empty operand
                    }
                    while (not ops.empty()) {
                        auto tp = ops.top();
                        prec_top = tp ? tp->opPrecedence : 0;
//                        prec_top = ops.top() ? ops.top()->opPrecedence : 0;
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

                        if (!p->opIsUnary and p->kind != TKFunctionCall
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
                        rpn.push(nil(ASTExpr));

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
                errorUnexpectedExpr(rpn.top()());
                goto error;
            }

            if (!p->opIsUnary
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

        SmallPtr<ASTExpr> arg;
        for (int i = 0; i < rpn.count; i++) {
            if (!(p = rpn[i])) goto justpush;
            switch (p->kind) {
            case TKFunctionCall:
            case TKSubscript:
                if (result.count > 0) {
                    arg = result.pop();
                    p->left = arg; //(SmallPtr<ASTExpr>) { GETSOBJPTR(arg) };
//                    if (p->left.ptr>50000)
//                    {
//                        printf("");
//                    }
                }
            case TKNumber:
            case TKString:
            case TKRegex:
            case TKInline:
            case TKUnits:
            case TKMultiDotNumber:
            case TKIdentifier:
            case TKParenOpen:
                // case TKArrayOpen:
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

                    p->right = result.pop(); //(SmallPtr<ASTExpr>) { GETSOBJPTR(
//                    result.pop()) };
                // GETSOBJPTR(result.pop());

                if (not p->opIsUnary) {
                    if (result.empty()) {
                        errorParsingExpr();
                        goto error;
                    }
                    p->left = result.pop(); // (SmallPtr<ASTExpr>) { GETSOBJPTR(
//                        result.pop()) };
//                    if (p->left.ptr>50000)
//                    {
//                        printf("");
//                    }
                    //= GETSOBJPTR(result.pop());
                }
            }
        justpush:
            result.push(p);
        }
        if (result.count != 1) {
            errorParsingExpr();
            goto error;
        }

        // for next invocation just set count to 0, allocation will be
        // reused
        ops.count = 0;
        rpn.count = 0;
        result.count = 0;
        ret = result[0];// (SmallPtr<ASTExpr>) { GETSOBJPTR(result[0]) };
//        if (ret.ptr>50000)
//            printf("");
        return ret;
        // GETSOBJPTR(result[0]);

    error:

        while (token.kind != TKNewline and token.pos < this->end)
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
                                         : rpn[i]->label());
        if (p)
            printf("\n      p: %.*s ", p->opPrecedence ? 100 : p->strLength,
                p->opPrecedence ? TokenKind_repr(p->kind) : p->label());

        if (rpn.count or ops.count) puts("");

        ops.count = 0; // "reset" stacks
        rpn.count = 0;
        result.count = 0;
        return (SmallPtr<ASTExpr>) { 0 };
    }

#pragma mark -
    SmallPtr<ASTTypeSpec> parseTypeSpec()
    { // must have ident(U), then may have "[:,:]" i.e. '[\]\[\:, ]+' , then
      // may have units. note: after ident may have params <T, S>
        token.flags.mergeArrayDims = true;

        ASTTypeSpec* typeSpec = new ASTTypeSpec;

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
        return (SmallPtr<ASTTypeSpec>) { GETSOBJPTR(typeSpec) };
    }

    SmallPtrListItem<ASTVar> parseArgs()
    {
        token.flags.mergeArrayDims = true;
        discard(TKParenOpen);

        SmallPtrListItem<ASTVar> args;
        SmallPtr<ASTVar> arg;
        do {
            arg = parseVar();
            args.append(arg);
        } while (ignore(TKOpComma));

        discard(TKParenClose);

        token.flags.mergeArrayDims = false;
        return args;
    }

#pragma mark -
    SmallPtr<ASTVar> parseVar()
    {
        auto var_ = new ASTVar;
        SmallPtr<ASTVar> var;
        var = var_;
        var->flags.isVar = (token.kind == TKKeyword_var);
        var->flags.isLet = (token.kind == TKKeyword_let);

        if (var->flags.isVar) discard(TKKeyword_var);
        if (var->flags.isLet) discard(TKKeyword_var);
        if (var->flags.isVar or var->flags.isLet) discard(TKOneSpace);

        var->name = parseIdent();

        if (ignore(TKOneSpace) and ignore(TKKeyword_as)) {
            discard(TKOneSpace);
            var->typeSpec = parseTypeSpec();
        }

        ignore(TKOneSpace);
        if (ignore(TKOpAssign)) var->init = parseExpr();

        return var; //(SmallPtr<ASTVar>) { GETSOBJPTR(var) };
    }

#pragma mark -
    SmallPtr<ASTScope> parseScope()
    {
        auto scope_ = new ASTScope;
        SmallPtr<ASTScope> scope ;
        scope = scope_;
        SmallPtr<ASTVar> var;
        SmallPtr<ASTExpr> expr;
        TokenKind tt = TKUnknown;
        // ALWAYS INITIALIZE ENUMS TO AN OUTOFBAND
        // VALUE ("UNKNOWN")! Always have an
        // outofband enum member unknown (=-1);

        // don't conflate this with the while in parse(): it checks against
        // file end, this checks against the keyword 'end'.
        while (token.kind != TKKeyword_end) {

            switch (token.kind) {

            case TKNullChar:
                errorExpectedToken(TKUnknown);
                goto exitloop;

            case TKKeyword_var:
                var = parseVar();
                if (!var) continue;
                scope->locals.append(var);
                break; // the below works but skipping for now
                //                expr = new ASTExpr; // skip comments when
                //                not linting expr->kind = TKOpAssign;
                //                expr->opPrecedence =
                //                TokenKind_getPrecedence(TKOpAssign);
                //                expr->left = new ASTExpr;
                //                expr->left->kind = TKIdentifier;
                //                expr->left->name = var->name;
                //                expr->left->strLength =
                //                strlen(var->name); expr->right =
                //                var->init; scope->stmts.append(expr);
                break;

            case TKKeyword_if:
            case TKKeyword_for:
            case TKKeyword_while:
                tt = token.kind;
                expr = match(tt); // will advance
                expr->left = parseExpr();
                if ((expr->body = parseScope()))
                    expr->body->parent = scope;
                discard(TKKeyword_end);
                discard(TKOneSpace);
                discard(tt);
                scope->stmts.append(expr);
                break;

            case TKNewline:
            case TKOneSpace: // found at beginning of line
                break;

            case TKLineComment:
                // do this only when linting! parser should have a flag
                if (generateCommentExprs) {
                    expr = (SmallPtr<ASTExpr>){GETSOBJPTR(new ASTExpr(&token))};
                    scope->stmts.append(expr);
                }
                break;

            default:
                scope->stmts.append(parseExpr());
                break;
            }
            token.advance();
        }
    exitloop:
        return scope; //(SmallPtr<ASTScope>) { GETSOBJPTR(scope) };
    }

#pragma mark -
    SmallPtrListItem<ASTVar> parseParams()
    {
        discard(TKOpLT);
        SmallPtrListItem<ASTVar> params;
        SmallPtr<ASTVar> param;
        do {
            param = (SmallPtr<ASTVar>) { GETSOBJPTR(new ASTVar) };
            param->name = parseIdent();
            if (ignore(TKKeyword_as)) param->typeSpec = parseTypeSpec();
            if (ignore(TKOpAssign)) param->init = parseExpr();
            params.append(param);
        } while (ignore(TKOpComma));

        discard(TKOpGT);
        return params;
    }

#pragma mark -
    SmallPtr<ASTFunc> parseFunc()
    {
        SmallPtr<ASTFunc> ret;

        discard(TKKeyword_function);
        discard(TKOneSpace);
        auto _func = new ASTFunc;
        auto func = (SmallPtr<ASTFunc>){GETSOBJPTR(_func)};

        func->name = parseIdent();
        func->args = parseArgs();
        if (ignore(TKOneSpace) and ignore(TKKeyword_as)) {
            discard(TKOneSpace);
            func->returnType = parseTypeSpec();
        }

        discard(TKNewline);

        func->body = parseScope();

        discard(TKKeyword_end);
        discard(TKOneSpace);
        discard(TKKeyword_function);

        ret = func;
        return ret;
    }

#pragma mark -
    SmallPtr<ASTFunc> parseTest() { return (SmallPtr<ASTFunc>) { 0 }; }

#pragma mark -
    SmallPtr<ASTUnits> parseUnits() { return (SmallPtr<ASTUnits>) { 0 }; }

#pragma mark -
    SmallPtr<ASTType> parseType()
    {
//        auto type_ = ASTType;
        auto type = ASTType::snew() ; //(SmallPtr<ASTType>){GETSOBJPTR(type_)};
        
        SmallPtr<ASTExpr> expr;
        SmallPtr<ASTVar> var;

        discard(TKKeyword_type);
        discard(TKOneSpace);
        type->name = parseIdent();
        if (matches(TKOpLT)) type->params = parseParams();

        while (token.kind != TKKeyword_end) {
            switch (token.kind) {
            case TKNullChar:
                errorUnexpectedToken();
                goto exitloop;

            case TKKeyword_var:
                var = parseVar();
                if (!var) continue;
                type->vars.append(var);
                break;

            case TKKeyword_base:
                discard(TKKeyword_base);
                discard(TKOneSpace);
                type->super = parseTypeSpec();
                break;

            case TKNewline:
            case TKOneSpace:
                token.advance();
                break;
            case TKLineComment:
                if (generateCommentExprs) {
                    expr = //(SmallPtr<ASTExpr>) { GETSOBJPTR(
                        new ASTExpr(&token); //)
                    //  }; // exprFromCurrentToken();
                    type->checks.append(expr);
                }
                token.advance();
                break;
            default:
                // general exprs are not allowed. Report the error as
                // unexpected token (first token on the line), then seek to
                // the next newline.
                errorUnexpectedToken();
                while (token.kind != TKNewline) token.advance();
                break;
            }
        }
    exitloop:

        discard(TKKeyword_end);
        discard(TKOneSpace);
        discard(TKKeyword_type);

        return type; //(SmallPtr<ASTType>) { GETSOBJPTR(type) };
    }

#pragma mark -
    SmallPtr<ASTImport> parseImport()
    {
//        SmallPtr<ASTImport> import ;
        auto import =  ASTImport::snew();
        char* tmp;
//        import = import_;
        discard(TKKeyword_import);
        discard(TKOneSpace);

        import->isPackage = ignore(TKAt);
        import->importFile = parseIdent();
        size_t len = token.pos - import->importFile();
        ignore(TKOneSpace);
        if (ignore(TKKeyword_as)) {

            ignore(TKOneSpace);
            import->hasAlias = true;
            import->alias = parseIdent();
            //            if (tmp)
            //                import->aliasOffset = (uint32_t)(tmp -
            //                import->importFile);

        } else {
            // TODO: set the alias using str_base
            //     import->aliasOffset
            //         = (uint32_t)(str_base(import->importFile, '.', len)
            //             - import->importFile);
        }

        ignore(TKOneSpace);

        if (token.kind != TKLineComment and token.kind != TKNewline)
            errorUnexpectedToken();
        while (token.kind != TKLineComment and token.kind != TKNewline)
            token.advance();
        return import ; //(SmallPtr<ASTImport>) { GETSOBJPTR(import) };
    }

#pragma mark -
    SmallPtrListItem<ASTModule> parse()
    {
//        SmallPtr<ASTModule> root;
        auto root =  ASTModule::snew();
//        root = sroot;
        root->name = moduleName;
        const bool onlyPrintTokens = false;
        token.advance(); // maybe put this in parser ctor
        SmallPtr<ASTImport> import;

        // The take away is (for C gen):
        // Every caller who calls List->append() should keep a local List*
        // to follow the list top as items are appended. Each actual append
        // call must be followed by an update of this pointer to its own
        // ->next. Append should be called on the last item of the list, not
        // the first. (it will work but seek through the whole list every
        // time).

        SmallPtrListItem<ASTFunc>* funcsTop = &root->funcs;
        SmallPtrListItem<ASTImport>* importsTop = &root->imports;
        SmallPtrListItem<ASTType>* typesTop = &root->types;
        SmallPtrListItem<ASTFunc>* testsTop = &root->tests;
        SmallPtrListItem<ASTVar>* globalsTop = &root->globals;

        while (token.kind != TKNullChar) {
            if (onlyPrintTokens) {
                printf("%s %2d %3d %3d %-6s\t%.*s\n", basename, token.line,
                    token.col, token.matchlen, TokenKind_repr(token.kind),
                    token.kind == TKNewline ? 0 : token.matchlen, token.pos);
                token.advance();
                continue;
            }
            switch (token.kind) {
            case TKKeyword_function:
                funcsTop->append(parseFunc());
                if (funcsTop->next) funcsTop = funcsTop->next();
                break;
            case TKKeyword_type:
                typesTop->append(parseType());
                if (typesTop->next) typesTop = typesTop->next();
                break;
            case TKKeyword_import:
                import = parseImport();
                if (import) {
                    importsTop->append(import);
                    if (importsTop->next) importsTop = importsTop->next();
                    //                    auto subParser = new
                    //                    Parser(import->importFile);
                    //                    List<ASTModule*> subMods =
                    //                    subParser->parse();
                    //                    modules.append(subMods);
                }
                break;
            case TKKeyword_test:
                testsTop->append(parseTest());
                if (testsTop->next) testsTop = testsTop->next();
                break;
            case TKKeyword_var:
            case TKKeyword_let:
                globalsTop->append(parseVar());
                if (globalsTop->next) globalsTop = globalsTop->next();
                break;
            case TKNewline:
            case TKLineComment:
            case TKOneSpace:
                token.advance();
                break;
            default:
                printf("other token: %s at %d:%d len %d\n", token.repr(),
                    token.line, token.col, token.matchlen);
                token.advance();
            }
        }
        // also keep modulesTop
        // ret = modules;
        modules.append(root);
        return modules;
    }
};

Pool<ASTTypeSpec, 128> ASTTypeSpec::pool;
Pool<ASTVar, 128> ASTVar::pool;
Pool<ASTImport, 32> ASTImport::pool;
Pool<ASTModule, 16> ASTModule::pool;
Pool<ASTFunc, 64> ASTFunc::pool;
Pool<ASTType, 32> ASTType::pool;
Pool<ASTExpr, 1024> ASTExpr::pool;
Pool<ASTScope, 32> ASTScope::pool;
Pool<Parser, 16> Parser::pool;

NAME_CLASS(Parser)
NAME_CLASS(ASTVar)
NAME_CLASS(ASTTypeSpec)
NAME_CLASS(ASTModule)
NAME_CLASS(ASTFunc)
NAME_CLASS(ASTType)
NAME_CLASS(ASTImport)
NAME_CLASS(ASTScope)
NAME_CLASS(ASTUnits)
NAME_CLASS(ASTExpr)
template <>
NAME_CLASS(SmallPtrListItem<ASTExpr>)
template <>
NAME_CLASS(SmallPtrListItem<ASTFunc>)
template <>
NAME_CLASS(SmallPtrListItem<ASTModule>)
template <>
NAME_CLASS(SmallPtrListItem<ASTType>)
template <>
NAME_CLASS(SmallPtrListItem<ASTVar>)
template <>
NAME_CLASS(SmallPtrListItem<ASTScope>)
template <>
NAME_CLASS(SmallPtrListItem<ASTImport>)

void alloc_stat()
{
    ASTImport::pool.stat();
    ASTExpr::pool.stat();
    ASTVar::pool.stat();
    ASTType::pool.stat();
    ASTScope::pool.stat();
    ASTTypeSpec::pool.stat();
    ASTFunc::pool.stat();
    ASTModule::pool.stat();
    SmallPtrListItem<ASTExpr>::pool.stat();
    SmallPtrListItem<ASTVar>::pool.stat();
    SmallPtrListItem<ASTModule>::pool.stat();
    SmallPtrListItem<ASTFunc>::pool.stat();
    SmallPtrListItem<ASTType>::pool.stat();
    SmallPtrListItem<ASTImport>::pool.stat();
    SmallPtrListItem<ASTScope>::pool.stat();
    Parser::pool.stat();

    fprintf(stderr, "Total nodes allocated %u B, used %u B (%.2f%%)\n",
        gObjPool.cap, gObjPool.used, gObjPool.used * 100.0 / gObjPool.cap);
}

#pragma mark - main
int main(int argc, char* argv[])
{
    if (argc == 1) return 1;
    bool printDiagnostics = (argc > 2 && *argv[2] == 'd') or true;

    Stopwatch sw;

    auto parser = new Parser(argv[1]);

    SmallPtrListItem<ASTModule> modules = parser->parse();

    if (parser->errCount or parser->warnCount) {
        fputs(equaltos, stderr);
        if (parser->errCount)
            fprintf(stderr, "*** %d errors\n", parser->errCount);
        if (parser->warnCount)
            fprintf(stderr, "*** %d warnings\n", parser->warnCount);
        fprintf(stderr, "    *Reading* the code helps, sometimes.");
        fputs(equaltos, stderr);
        return 1;
    };

    foreach (mod, mods, modules)
        mod->gen();

    if (printDiagnostics) {
        fprintf(stderr, "file %lu B\n", parser->dataSize());
        alloc_stat();
        fputs("total global counts:\n", stderr);
        fprintf(stderr, "calloc: %d\n", globalCallocCount);
        fprintf(stderr, "malloc: %d\n", globalMallocCount);
        fprintf(stderr, "strdup: %d\n", globalStrdupCount);
        fprintf(stderr, "realloc: %d\n", globalReallocCount);
        fprintf(stderr, "strlen: %d\n", globalStrlenCount);
        expralloc_stat();
    }

    //    fprintf(stderr, "ticks: %f\n", t.lap());
    sw.print();

//    SmallPtr<ASTImport> vp = 128;
//    ASTImport* va = vp;
#define GET(T, addr) (T*)poolb.deref(addr)
    //    auto v = GET(ASTImport, 128);
    //    const int s = sizeof(SmallPtr<ASTImport>);

    //    free(parser);
//    parser->fini();

    return 0;
}
