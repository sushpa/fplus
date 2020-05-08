#include "checkstd.h"
#ifndef HAVE_SIMPTEST1
#define HAVE_SIMPTEST1

#define THISMODULE simptest1
#define THISFILE "simptest1.ch"

typedef struct Expr* Expr;
struct Expr;
static Expr Expr_alloc_();
static Expr Expr_init_(Expr this);
Expr Expr_new_(
#ifdef DEBUG
    const char* callsite_
#endif
);

DECL_json_wrap_(Expr)
// DECL_json_file(Expr)
#define Expr_json(x)                                                       \
    {                                                                      \
        printf("\"%s\": ", #x);                                            \
        Expr_json_wrap_(x);                                                \
    }

    static void Expr_json_(const Expr this, int nspc);
typedef struct Another* Another;
struct Another;
static Another Another_alloc_();
static Another Another_init_(Another this);
Another Another_new_(
#ifdef DEBUG
    const char* callsite_
#endif
);

DECL_json_wrap_(Another)
// DECL_json_file(Another)
#define Another_json(x)                                                    \
    {                                                                      \
        printf("\"%s\": ", #x);                                            \
        Another_json_wrap_(x);                                             \
    }

    static void Another_json_(const Another this, int nspc);
typedef struct Other* Other;
struct Other;
static Other Other_alloc_();
static Other Other_init_(Other this);
Other Other_new_(
#ifdef DEBUG
    const char* callsite_
#endif
);

DECL_json_wrap_(Other)
// DECL_json_file(Other)
#define Other_json(x)                                                      \
    {                                                                      \
        printf("\"%s\": ", #x);                                            \
        Other_json_wrap_(x);                                               \
    }

    static void Other_json_(const Other this, int nspc);
typedef struct Point* Point;
struct Point;
static Point Point_alloc_();
static Point Point_init_(Point this);
Point Point_new_(
#ifdef DEBUG
    const char* callsite_
#endif
);

DECL_json_wrap_(Point)
// DECL_json_file(Point)
#define Point_json(x)                                                      \
    {                                                                      \
        printf("\"%s\": ", #x);                                            \
        Point_json_wrap_(x);                                               \
    }

    static void Point_json_(const Point this, int nspc);
static Number Number_fxfunc(const Number x
#ifdef DEBUG
    ,
    const char* callsite_
#endif
);

static Point Number_Point(const Number x
#ifdef DEBUG
    ,
    const char* callsite_
#endif
);

static Number Strings_main(const Strings args
#ifdef DEBUG
    ,
    const char* callsite_
#endif
);

static void funky(
#ifdef DEBUG
    const char* callsite_
#endif
);

static void joyce(
#ifdef DEBUG
    const char* callsite_
#endif
);

#define FIELDS_Expr                                                        \
    Number meg;                                                            \
    Boolean bx;                                                            \
    Expr f;

struct Expr {
    FIELDS_Expr
};

static const char* Expr_name_ = "Expr";
static Expr Expr_alloc_()
{
    return _Pool_alloc_(&gPool_, sizeof(struct Expr));
}
static Expr Expr_init_(Expr this)
{
#define meg this->meg
#define bx this->bx
#define f this->f
    meg = 33.2;
    bx = 4 < 5;
    f = Expr_new_(
#ifdef DEBUG
        "./" THISFILE ":33:13:\e[0m Expr()"
#endif
    );
#undef meg
#undef bx
#undef f
    return this;
}
Expr Expr_new_(
#ifdef DEBUG
    const char* callsite_
#endif
)
{
#define DEFAULT_VALUE NULL
#ifdef DEBUG
    static const char* sig_ = "Expr()";
#endif
#ifdef DEBUG
#define MYSTACKUSAGE (48 + 6 * sizeof(void*) + sizeof(char*))
#else
#define MYSTACKUSAGE (48 + 6 * sizeof(void*))
#endif
#ifndef NOSTACKCHECK
    STACKDEPTH_UP
    // printf("%8lu %8lu\n",_scUsage_, _scSize_);
    if (_scUsage_ >= _scSize_) {
#ifdef DEBUG
        _scPrintAbove_ = _scDepth_ - _btLimit_;
        printf("\e[31mfatal: stack overflow at call depth %lu.\n    in "
               "%s\e[0m\n",
            _scDepth_, sig_);
        printf("\e[90mBacktrace (innermost first):\n");
        if (_scDepth_ > 2 * _btLimit_)
            printf("    limited to %d outer and %d inner entries.\n",
                _btLimit_, _btLimit_);
        printf("[%lu] \e[36m%s\n", _scDepth_, callsite_);
#else
        printf("\e[31mfatal: stack overflow at call depth %lu.\e[0m\n",
            _scDepth_);
#endif
        DONE
    }
#endif

    Expr ret = Expr_alloc_();
    Expr_init_(ret);
    if (_err_ == ERROR_TRACE) goto backtrace;
    return ret; // ------------ error handling
    return DEFAULT_VALUE;
    assert(0);
error:
#ifdef DEBUG
    eprintf("error: %s\n", _err_);
#endif
backtrace:
#ifdef DEBUG
    if (_scDepth_ <= _btLimit_ || _scDepth_ > _scPrintAbove_)
        printf("\e[90m[%lu] \e[36m%s\n", _scDepth_, callsite_);
    else if (_scDepth_ == _scPrintAbove_)
        printf("\e[90m... truncated ...\e[0m\n");
#endif
done:
#ifndef NOSTACKCHECK
    STACKDEPTH_DOWN
#endif
    return DEFAULT_VALUE;
#undef DEFAULT_VALUE
#undef MYSTACKUSAGE
}
#define Expr_print_(p) Expr_print__(p, STR(p))
void Expr_print__(Expr this, const char* name)
{
    printf("<Expr '%s' at %p\n>", name, this);
}

static void Expr_json_(const Expr this, int nspc)
{
    printf("{\n");
    printf("%.*s\"meg\": ", nspc + 4, _spaces_);
    Number_json_(this->meg, nspc + 4);
    printf(",\n");
    printf("%.*s\"bx\": ", nspc + 4, _spaces_);
    Boolean_json_(this->bx, nspc + 4);
    printf(",\n");
    printf("%.*s\"f\": ", nspc + 4, _spaces_);
    Expr_json_(this->f, nspc + 4);
    printf("\n");
    printf("%.*s}", nspc, _spaces_);
}
MAKE_json_wrap_(Expr)
// MAKE_json_file(Expr)
#define FIELDS_Another                                                     \
    Number g;                                                              \
    Expr exp;

    struct Another {
    FIELDS_Another
};

static const char* Another_name_ = "Another";
static Another Another_alloc_()
{
    return _Pool_alloc_(&gPool_, sizeof(struct Another));
}
static Another Another_init_(Another this)
{
#define g this->g
#define exp this->exp
    g = 12;
    exp = Expr_new_(
#ifdef DEBUG
        "./" THISFILE ":44:15:\e[0m Expr()"
#endif
    );
#undef g
#undef exp
    return this;
}
Another Another_new_(
#ifdef DEBUG
    const char* callsite_
#endif
)
{
#define DEFAULT_VALUE NULL
#ifdef DEBUG
    static const char* sig_ = "Another()";
#endif
#ifdef DEBUG
#define MYSTACKUSAGE (48 + 6 * sizeof(void*) + sizeof(char*))
#else
#define MYSTACKUSAGE (48 + 6 * sizeof(void*))
#endif
#ifndef NOSTACKCHECK
    STACKDEPTH_UP
    // printf("%8lu %8lu\n",_scUsage_, _scSize_);
    if (_scUsage_ >= _scSize_) {
#ifdef DEBUG
        _scPrintAbove_ = _scDepth_ - _btLimit_;
        printf("\e[31mfatal: stack overflow at call depth %lu.\n    in "
               "%s\e[0m\n",
            _scDepth_, sig_);
        printf("\e[90mBacktrace (innermost first):\n");
        if (_scDepth_ > 2 * _btLimit_)
            printf("    limited to %d outer and %d inner entries.\n",
                _btLimit_, _btLimit_);
        printf("[%lu] \e[36m%s\n", _scDepth_, callsite_);
#else
        printf("\e[31mfatal: stack overflow at call depth %lu.\e[0m\n",
            _scDepth_);
#endif
        DONE
    }
#endif

    Another ret = Another_alloc_();
    Another_init_(ret);
    if (_err_ == ERROR_TRACE) goto backtrace;
    return ret; // ------------ error handling
    return DEFAULT_VALUE;
    assert(0);
error:
#ifdef DEBUG
    eprintf("error: %s\n", _err_);
#endif
backtrace:
#ifdef DEBUG
    if (_scDepth_ <= _btLimit_ || _scDepth_ > _scPrintAbove_)
        printf("\e[90m[%lu] \e[36m%s\n", _scDepth_, callsite_);
    else if (_scDepth_ == _scPrintAbove_)
        printf("\e[90m... truncated ...\e[0m\n");
#endif
done:
#ifndef NOSTACKCHECK
    STACKDEPTH_DOWN
#endif
    return DEFAULT_VALUE;
#undef DEFAULT_VALUE
#undef MYSTACKUSAGE
}
#define Another_print_(p) Another_print__(p, STR(p))
void Another_print__(Another this, const char* name)
{
    printf("<Another '%s' at %p\n>", name, this);
}

static void Another_json_(const Another this, int nspc)
{
    printf("{\n");
    printf("%.*s\"g\": ", nspc + 4, _spaces_);
    Number_json_(this->g, nspc + 4);
    printf(",\n");
    printf("%.*s\"exp\": ", nspc + 4, _spaces_);
    Expr_json_(this->exp, nspc + 4);
    printf("\n");
    printf("%.*s}", nspc, _spaces_);
}
MAKE_json_wrap_(Another)
// MAKE_json_file(Another)
#define FIELDS_Other                                                       \
    Number m;                                                              \
    Another we;

    struct Other {
    FIELDS_Other
};

static const char* Other_name_ = "Other";
static Other Other_alloc_()
{
    return _Pool_alloc_(&gPool_, sizeof(struct Other));
}
static Other Other_init_(Other this)
{
#define m this->m
#define we this->we
    m = 43;
    we = Another_new_(
#ifdef DEBUG
        "./" THISFILE ":49:14:\e[0m Another()"
#endif
    );
#undef m
#undef we
    return this;
}
Other Other_new_(
#ifdef DEBUG
    const char* callsite_
#endif
)
{
#define DEFAULT_VALUE NULL
#ifdef DEBUG
    static const char* sig_ = "Other()";
#endif
#ifdef DEBUG
#define MYSTACKUSAGE (48 + 6 * sizeof(void*) + sizeof(char*))
#else
#define MYSTACKUSAGE (48 + 6 * sizeof(void*))
#endif
#ifndef NOSTACKCHECK
    STACKDEPTH_UP
    // printf("%8lu %8lu\n",_scUsage_, _scSize_);
    if (_scUsage_ >= _scSize_) {
#ifdef DEBUG
        _scPrintAbove_ = _scDepth_ - _btLimit_;
        printf("\e[31mfatal: stack overflow at call depth %lu.\n    in "
               "%s\e[0m\n",
            _scDepth_, sig_);
        printf("\e[90mBacktrace (innermost first):\n");
        if (_scDepth_ > 2 * _btLimit_)
            printf("    limited to %d outer and %d inner entries.\n",
                _btLimit_, _btLimit_);
        printf("[%lu] \e[36m%s\n", _scDepth_, callsite_);
#else
        printf("\e[31mfatal: stack overflow at call depth %lu.\e[0m\n",
            _scDepth_);
#endif
        DONE
    }
#endif

    Other ret = Other_alloc_();
    Other_init_(ret);
    if (_err_ == ERROR_TRACE) goto backtrace;
    return ret; // ------------ error handling
    return DEFAULT_VALUE;
    assert(0);
error:
#ifdef DEBUG
    eprintf("error: %s\n", _err_);
#endif
backtrace:
#ifdef DEBUG
    if (_scDepth_ <= _btLimit_ || _scDepth_ > _scPrintAbove_)
        printf("\e[90m[%lu] \e[36m%s\n", _scDepth_, callsite_);
    else if (_scDepth_ == _scPrintAbove_)
        printf("\e[90m... truncated ...\e[0m\n");
#endif
done:
#ifndef NOSTACKCHECK
    STACKDEPTH_DOWN
#endif
    return DEFAULT_VALUE;
#undef DEFAULT_VALUE
#undef MYSTACKUSAGE
}
#define Other_print_(p) Other_print__(p, STR(p))
void Other_print__(Other this, const char* name)
{
    printf("<Other '%s' at %p\n>", name, this);
}

static void Other_json_(const Other this, int nspc)
{
    printf("{\n");
    printf("%.*s\"m\": ", nspc + 4, _spaces_);
    Number_json_(this->m, nspc + 4);
    printf(",\n");
    printf("%.*s\"we\": ", nspc + 4, _spaces_);
    Another_json_(this->we, nspc + 4);
    printf("\n");
    printf("%.*s}", nspc, _spaces_);
}
MAKE_json_wrap_(Other)
// MAKE_json_file(Other)
#define FIELDS_Point                                                       \
    Number x;                                                              \
    Number y;                                                              \
    Other o;                                                               \
    Number z;                                                              \
    String cstr;

    struct Point {
    FIELDS_Point
};

static const char* Point_name_ = "Point";
static Point Point_alloc_()
{
    return _Pool_alloc_(&gPool_, sizeof(struct Point));
}
static Point Point_init_(Point this)
{
#define x this->x
#define y this->y
#define o this->o
#define z this->z
#define cstr this->cstr
    x = Number_fxfunc(3
#ifdef DEBUG
        ,
        "./" THISFILE ":63:13:\e[0m fxfunc(3)"
#endif
    );
    y = 69.6723;
    o = Other_new_(
#ifdef DEBUG
        "./" THISFILE ":65:13:\e[0m Other()"
#endif
    );
    z = y + 5.6 * x + o->we->g;
    cstr = "xyz";
#undef x
#undef y
#undef o
#undef z
#undef cstr
    return this;
}
Point Point_new_(
#ifdef DEBUG
    const char* callsite_
#endif
)
{
#define DEFAULT_VALUE NULL
#ifdef DEBUG
    static const char* sig_ = "Point()";
#endif
#ifdef DEBUG
#define MYSTACKUSAGE (48 + 6 * sizeof(void*) + sizeof(char*))
#else
#define MYSTACKUSAGE (48 + 6 * sizeof(void*))
#endif
#ifndef NOSTACKCHECK
    STACKDEPTH_UP
    // printf("%8lu %8lu\n",_scUsage_, _scSize_);
    if (_scUsage_ >= _scSize_) {
#ifdef DEBUG
        _scPrintAbove_ = _scDepth_ - _btLimit_;
        printf("\e[31mfatal: stack overflow at call depth %lu.\n    in "
               "%s\e[0m\n",
            _scDepth_, sig_);
        printf("\e[90mBacktrace (innermost first):\n");
        if (_scDepth_ > 2 * _btLimit_)
            printf("    limited to %d outer and %d inner entries.\n",
                _btLimit_, _btLimit_);
        printf("[%lu] \e[36m%s\n", _scDepth_, callsite_);
#else
        printf("\e[31mfatal: stack overflow at call depth %lu.\e[0m\n",
            _scDepth_);
#endif
        DONE
    }
#endif

    Point ret = Point_alloc_();
    Point_init_(ret);
    if (_err_ == ERROR_TRACE) goto backtrace;
    return ret; // ------------ error handling
    return DEFAULT_VALUE;
    assert(0);
error:
#ifdef DEBUG
    eprintf("error: %s\n", _err_);
#endif
backtrace:
#ifdef DEBUG
    if (_scDepth_ <= _btLimit_ || _scDepth_ > _scPrintAbove_)
        printf("\e[90m[%lu] \e[36m%s\n", _scDepth_, callsite_);
    else if (_scDepth_ == _scPrintAbove_)
        printf("\e[90m... truncated ...\e[0m\n");
#endif
done:
#ifndef NOSTACKCHECK
    STACKDEPTH_DOWN
#endif
    return DEFAULT_VALUE;
#undef DEFAULT_VALUE
#undef MYSTACKUSAGE
}
#define Point_print_(p) Point_print__(p, STR(p))
void Point_print__(Point this, const char* name)
{
    printf("<Point '%s' at %p\n>", name, this);
}

static void Point_json_(const Point this, int nspc)
{
    printf("{\n");
    printf("%.*s\"x\": ", nspc + 4, _spaces_);
    Number_json_(this->x, nspc + 4);
    printf(",\n");
    printf("%.*s\"y\": ", nspc + 4, _spaces_);
    Number_json_(this->y, nspc + 4);
    printf(",\n");
    printf("%.*s\"o\": ", nspc + 4, _spaces_);
    Other_json_(this->o, nspc + 4);
    printf(",\n");
    printf("%.*s\"z\": ", nspc + 4, _spaces_);
    Number_json_(this->z, nspc + 4);
    printf(",\n");
    printf("%.*s\"cstr\": ", nspc + 4, _spaces_);
    String_json_(this->cstr, nspc + 4);
    printf("\n");
    printf("%.*s}", nspc, _spaces_);
}
MAKE_json_wrap_(Point)
// MAKE_json_file(Point)
#ifdef DEBUG
#define MYSTACKUSAGE (8 + 6 * sizeof(void*) + sizeof(char*))
#else
#define MYSTACKUSAGE (8 + 6 * sizeof(void*))
#endif
#define DEFAULT_VALUE 0
    static Number Number_fxfunc(const Number x
#ifdef DEBUG
        ,
        const char* callsite_
#endif
    )
{
#ifdef DEBUG
    static const char* sig_ = "fxfunc(x as Number) returns Number";
#endif
#ifndef NOSTACKCHECK
    STACKDEPTH_UP
    // printf("%8lu %8lu\n",_scUsage_, _scSize_);
    if (_scUsage_ >= _scSize_) {
#ifdef DEBUG
        _scPrintAbove_ = _scDepth_ - _btLimit_;
        printf("\e[31mfatal: stack overflow at call depth %lu.\n    in "
               "%s\e[0m\n",
            _scDepth_, sig_);
        printf("\e[90mBacktrace (innermost first):\n");
        if (_scDepth_ > 2 * _btLimit_)
            printf("    limited to %d outer and %d inner entries.\n",
                _btLimit_, _btLimit_);
        printf("[%lu] \e[36m%s\n", _scDepth_, callsite_);
#else
        printf("\e[31mfatal: stack overflow at call depth %lu.\e[0m\n",
            _scDepth_);
#endif
        DONE
    }
#endif

    {
        _err_ = NULL;
#ifndef NOSTACKCHECK
        STACKDEPTH_DOWN
#endif
        return x * 1.5;
    }

    // ------------ error handling
    return DEFAULT_VALUE;
    assert(0);
error:
#ifdef DEBUG
    eprintf("error: %s\n", _err_);
#endif
backtrace:
#ifdef DEBUG
    if (_scDepth_ <= _btLimit_ || _scDepth_ > _scPrintAbove_)
        printf("\e[90m[%lu] \e[36m%s\n", _scDepth_, callsite_);
    else if (_scDepth_ == _scPrintAbove_)
        printf("\e[90m... truncated ...\e[0m\n");
#endif
done:
#ifndef NOSTACKCHECK
    STACKDEPTH_DOWN
#endif
    return DEFAULT_VALUE;
}
#undef DEFAULT_VALUE
#undef MYSTACKUSAGE
#ifdef DEBUG
#define MYSTACKUSAGE (16 + 6 * sizeof(void*) + sizeof(char*))
#else
#define MYSTACKUSAGE (16 + 6 * sizeof(void*))
#endif
#define DEFAULT_VALUE 0
static Point Number_Point(const Number x
#ifdef DEBUG
    ,
    const char* callsite_
#endif
)
{
#ifdef DEBUG
    static const char* sig_ = "function Point(x as Number) returns Point";
#endif
#ifndef NOSTACKCHECK
    STACKDEPTH_UP
    // printf("%8lu %8lu\n",_scUsage_, _scSize_);
    if (_scUsage_ >= _scSize_) {
#ifdef DEBUG
        _scPrintAbove_ = _scDepth_ - _btLimit_;
        printf("\e[31mfatal: stack overflow at call depth %lu.\n    in "
               "%s\e[0m\n",
            _scDepth_, sig_);
        printf("\e[90mBacktrace (innermost first):\n");
        if (_scDepth_ > 2 * _btLimit_)
            printf("    limited to %d outer and %d inner entries.\n",
                _btLimit_, _btLimit_);
        printf("[%lu] \e[36m%s\n", _scDepth_, callsite_);
#else
        printf("\e[31mfatal: stack overflow at call depth %lu.\e[0m\n",
            _scDepth_);
#endif
        DONE
    }
#endif

    Point p;
    p = Point_new_(
#ifdef DEBUG
        "./" THISFILE ":79:13:\e[0m Point()"
#endif
    );
    if (_err_ == ERROR_TRACE) goto backtrace;
    p->y = x;
    Number_describe(x);
    if (_err_ == ERROR_TRACE) goto backtrace;
    {
        _err_ = NULL;
#ifndef NOSTACKCHECK
        STACKDEPTH_DOWN
#endif
        return p;
    }

    // ------------ error handling
    return DEFAULT_VALUE;
    assert(0);
error:
#ifdef DEBUG
    eprintf("error: %s\n", _err_);
#endif
backtrace:
#ifdef DEBUG
    if (_scDepth_ <= _btLimit_ || _scDepth_ > _scPrintAbove_)
        printf("\e[90m[%lu] \e[36m%s\n", _scDepth_, callsite_);
    else if (_scDepth_ == _scPrintAbove_)
        printf("\e[90m... truncated ...\e[0m\n");
#endif
done:
#ifndef NOSTACKCHECK
    STACKDEPTH_DOWN
#endif
    return DEFAULT_VALUE;
}
#undef DEFAULT_VALUE
#undef MYSTACKUSAGE
#ifdef DEBUG
#define MYSTACKUSAGE (36 + 6 * sizeof(void*) + sizeof(char*))
#else
#define MYSTACKUSAGE (36 + 6 * sizeof(void*))
#endif
#define DEFAULT_VALUE 0
static Number Strings_main(const Strings args
#ifdef DEBUG
    ,
    const char* callsite_
#endif
)
{
#ifdef DEBUG
    static const char* sig_
        = "function main(args as Strings) returns Number";
#endif
#ifndef NOSTACKCHECK
    STACKDEPTH_UP
    // printf("%8lu %8lu\n",_scUsage_, _scSize_);
    if (_scUsage_ >= _scSize_) {
#ifdef DEBUG
        _scPrintAbove_ = _scDepth_ - _btLimit_;
        printf("\e[31mfatal: stack overflow at call depth %lu.\n    in "
               "%s\e[0m\n",
            _scDepth_, sig_);
        printf("\e[90mBacktrace (innermost first):\n");
        if (_scDepth_ > 2 * _btLimit_)
            printf("    limited to %d outer and %d inner entries.\n",
                _btLimit_, _btLimit_);
        printf("[%lu] \e[36m%s\n", _scDepth_, callsite_);
#else
        printf("\e[31mfatal: stack overflow at call depth %lu.\e[0m\n",
            _scDepth_);
#endif
        DONE
    }
#endif

    Point po;
    Point pcx;
    Boolean mk;
    Other sd;
    po = Point_new_(
#ifdef DEBUG
        "./" THISFILE ":87:14:\e[0m Point()"
#endif
    );
    if (_err_ == ERROR_TRACE) goto backtrace;
    pcx = Number_Point(78
#ifdef DEBUG
        ,
        "./" THISFILE ":88:15:\e[0m Point(78)"
#endif
    );
    if (_err_ == ERROR_TRACE) goto backtrace;
    ;
    Point_json(po);
    if (_err_ == ERROR_TRACE) goto backtrace;
    Point_json(pcx);
    if (_err_ == ERROR_TRACE) goto backtrace;
    ;
    mk = Number_cmp3way_LE_LT(3, 6, 5);
    Boolean_json(mk);
    if (_err_ == ERROR_TRACE) goto backtrace;
    sd = po->o;
    Other_json(sd);
    if (_err_ == ERROR_TRACE) goto backtrace;
    Boolean_describe(po->o->we->exp->bx);
    if (_err_ == ERROR_TRACE) goto backtrace;
    Other_json(po->o);
    if (_err_ == ERROR_TRACE) goto backtrace;
    Boolean_describe(mk);
    if (_err_ == ERROR_TRACE) goto backtrace;
    funky(
#ifdef DEBUG
        "./" THISFILE ":100:5:\e[0m funky()"
#endif
    );
    if (_err_ == ERROR_TRACE) goto backtrace;
    // ------------ error handling
    return DEFAULT_VALUE;
    assert(0);
error:
#ifdef DEBUG
    eprintf("error: %s\n", _err_);
#endif
backtrace:
#ifdef DEBUG
    if (_scDepth_ <= _btLimit_ || _scDepth_ > _scPrintAbove_)
        printf("\e[90m[%lu] \e[36m%s\n", _scDepth_, callsite_);
    else if (_scDepth_ == _scPrintAbove_)
        printf("\e[90m... truncated ...\e[0m\n");
#endif
done:
#ifndef NOSTACKCHECK
    STACKDEPTH_DOWN
#endif
    return DEFAULT_VALUE;
}
#undef DEFAULT_VALUE
#undef MYSTACKUSAGE
#ifdef DEBUG
#define MYSTACKUSAGE (0 + 6 * sizeof(void*) + sizeof(char*))
#else
#define MYSTACKUSAGE (0 + 6 * sizeof(void*))
#endif
#define DEFAULT_VALUE
static void funky(
#ifdef DEBUG
    const char* callsite_
#endif
)
{
#ifdef DEBUG
    static const char* sig_ = "function funky(";
#endif
#ifndef NOSTACKCHECK
    STACKDEPTH_UP
    // printf("%8lu %8lu\n",_scUsage_, _scSize_);
    if (_scUsage_ >= _scSize_) {
#ifdef DEBUG
        _scPrintAbove_ = _scDepth_ - _btLimit_;
        printf("\e[31mfatal: stack overflow at call depth %lu.\n    in "
               "%s\e[0m\n",
            _scDepth_, sig_);
        printf("\e[90mBacktrace (innermost first):\n");
        if (_scDepth_ > 2 * _btLimit_)
            printf("    limited to %d outer and %d inner entries.\n",
                _btLimit_, _btLimit_);
        printf("[%lu] \e[36m%s\n", _scDepth_, callsite_);
#else
        printf("\e[31mfatal: stack overflow at call depth %lu.\e[0m\n",
            _scDepth_);
#endif
        DONE
    }
#endif

    joyce(
#ifdef DEBUG
        "./" THISFILE ":137:5:\e[0m joyce()"
#endif
    );
    if (_err_ == ERROR_TRACE) goto backtrace;
    // ------------ error handling
    return DEFAULT_VALUE;
    assert(0);
error:
#ifdef DEBUG
    eprintf("error: %s\n", _err_);
#endif
backtrace:
#ifdef DEBUG
    if (_scDepth_ <= _btLimit_ || _scDepth_ > _scPrintAbove_)
        printf("\e[90m[%lu] \e[36m%s\n", _scDepth_, callsite_);
    else if (_scDepth_ == _scPrintAbove_)
        printf("\e[90m... truncated ...\e[0m\n");
#endif
done:
#ifndef NOSTACKCHECK
    STACKDEPTH_DOWN
#endif
    return DEFAULT_VALUE;
}
#undef DEFAULT_VALUE
#undef MYSTACKUSAGE
#ifdef DEBUG
#define MYSTACKUSAGE (32 + 6 * sizeof(void*) + sizeof(char*))
#else
#define MYSTACKUSAGE (32 + 6 * sizeof(void*))
#endif
#define DEFAULT_VALUE
static void joyce(
#ifdef DEBUG
    const char* callsite_
#endif
)
{
#ifdef DEBUG
    static const char* sig_ = "function joyce(";
#endif
#ifndef NOSTACKCHECK
    STACKDEPTH_UP
    // printf("%8lu %8lu\n",_scUsage_, _scSize_);
    if (_scUsage_ >= _scSize_) {
#ifdef DEBUG
        _scPrintAbove_ = _scDepth_ - _btLimit_;
        printf("\e[31mfatal: stack overflow at call depth %lu.\n    in "
               "%s\e[0m\n",
            _scDepth_, sig_);
        printf("\e[90mBacktrace (innermost first):\n");
        if (_scDepth_ > 2 * _btLimit_)
            printf("    limited to %d outer and %d inner entries.\n",
                _btLimit_, _btLimit_);
        printf("[%lu] \e[36m%s\n", _scDepth_, callsite_);
#else
        printf("\e[31mfatal: stack overflow at call depth %lu.\e[0m\n",
            _scDepth_);
#endif
        DONE
    }
#endif

    String name;
    String nam2;
    Number x;
    Number joyce;
    name = "BhAbru";
    nam2 = "Bhabru";
    x = Number_fxfunc(3 + 5 + 4
#ifdef DEBUG
        ,
        "./" THISFILE ":143:13:\e[0m fxfunc(3+5+4)"
#endif
    );
    if (_err_ == ERROR_TRACE) goto backtrace;
    Number_describe(x);
    if (_err_ == ERROR_TRACE) goto backtrace;
    Number_describe(x + 5);
    if (_err_ == ERROR_TRACE) goto backtrace;
    ;
    if (_err_ == ERROR_TRACE) goto backtrace;
    joyce = 55.3;
    Number_describe(joyce);
    if (_err_ == ERROR_TRACE) goto backtrace;
    ;
    {
        String _lhs = name;
        String _rhs = nam2;
        if (not(str_cmp(==, name, nam2))) {
            printf("\n\n[31mruntime error:[0m check failed at "
                   "[36m./%s:152:11:[0m\n    %s\n\n",
                THISFILE, "name == nam2");
#ifdef DEBUG
            printf("[90mHere's some help:[0m\n");
            printf("    name = \"%s\"\n", name);
            printf("    nam2 = \"%s\"\n", nam2);
            printf("\n");
            printf("\e[90mBacktrace (innermost first):\n");
            if (_scDepth_ > 2 * _btLimit_)
                printf("    limited to %d outer and %d inner entries.\n",
                    _btLimit_, _btLimit_);
            BACKTRACE

#else
            eputs("(run in debug mode to get more info)\n");
            exit(1);
#endif
        }
    };
    // ------------ error handling
    return DEFAULT_VALUE;
    assert(0);
error:
#ifdef DEBUG
    eprintf("error: %s\n", _err_);
#endif
backtrace:
#ifdef DEBUG
    if (_scDepth_ <= _btLimit_ || _scDepth_ > _scPrintAbove_)
        printf("\e[90m[%lu] \e[36m%s\n", _scDepth_, callsite_);
    else if (_scDepth_ == _scPrintAbove_)
        printf("\e[90m... truncated ...\e[0m\n");
#endif
done:
#ifndef NOSTACKCHECK
    STACKDEPTH_DOWN
#endif
    return DEFAULT_VALUE;
}
#undef DEFAULT_VALUE
#undef MYSTACKUSAGE
#undef THISMODULE
#undef THISFILE
#endif // HAVE_SIMPTEST1
