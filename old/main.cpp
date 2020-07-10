//
//  main.cpp
//  checkcc
//
//

#include <cstdio>
#include <cstdint>

#define RELF(s) (*s == '/' ? "" : "./"), s

#define fatal(str, ...)                                                    \
{                                                                      \
eprintf(str, __VA_ARGS__);                                         \
exit(1);                                                           \
}
class Errors;

enum ETokenKind {
TKKeyword_if
};

class TokenKind {
    ETokenKind kind;
    operator ETokenKind() {return kind;}
   const char* repr(bool spacing = true) {switch (kind) {}return "(err)";}
};


  struct ASTImport {
    char* importFile;
    uint32_t aliasOffset;
    bool isPackage, hasAlias;
}  ;

  struct ASTUnits {
    uint8_t powers[7], something;
    double factor, factors[7];
    char* label;
}  ;

  struct ASTTypeSpec {
    union {
          ASTType* type;
        char* name;
        ASTUnits* units;
    };
    uint32_t dims : 24, col : 8;
    uint16_t line;
    TypeTypes typeType : 8;
    CollectionTypes collectionType : 8;
}  ;

  struct ASTVar {
    ASTTypeSpec* typeSpec;
      ASTExpr* init;
    char* name;
    uint16_t line;
    uint8_t col;
    struct {
        bool used : 1, //
        changed : 1, //
        isLet : 1, //
        isVar : 1, //
        isTarget : 1, // x = f(x,y)
        printed : 1, // for checks, used to avoid printing this var more
                     // than once.
        escapesFunc : 1; // for func args, does it escape the func?
                         // (returned etc.)

    } flags;
}  ;

  struct ASTExpr {
    struct {
        uint16_t line;
             struct {
                uint16_t typeType : 5, isElementalOp : 1, canThrow : 1,
                opIsUnary : 1, collectionType : 6, mayNeedPromotion : 1,
                opIsRightAssociative : 1;
            };
         uint8_t opPrec;
        uint8_t col;
        TokenKind kind : 8;
    };
    struct ASTExpr* left;
    union {
        // union {
        char* string;
        double real;
        int64_t integer;
        uint64_t uinteger;
        //} value; // for terminals
        char* name; // for idents or unresolved call or subscript
          ASTFunc* func; // for functioncall
          ASTVar* var; // for array subscript, or a TKVarAssign
          ASTScope* body; // for if/for/while
          ASTExpr* right;
    };
    // TODO: the code motion routine should skip over exprs with
    // mayNeedPromotion=false this is set for exprs with func calls or array
    // filtering etc...
}  ;

  struct ASTScope {
    List<ASTExpr> * stmts;
    List<ASTVar> * locals;
      ASTScope* parent;
    // still space left
}  ;

  struct ASTType {
    ASTTypeSpec* super;
    char* name;
    // TODO: flags: hasUserInit : whether user has defined Type() ctor
    ASTScope* body;
}  ;

  struct ASTFunc {
    ASTScope* body;
    List<ASTVar> * args;
    ASTTypeSpec* returnType;
    char* name;
    char* selector;
    struct {
        uint16_t line;
        struct {
            uint16_t usesIO : 1, nothrow : 1, isRecursive : 1, usesNet : 1,
            usesGUI : 1, usesSerialisation : 1, isExported : 1,
            usesReflection : 1, nodispatch : 1, isStmt : 1,
            isDeclare : 1, isCalledFromWithinLoop : 1,
            isElementalFunc : 1;
        } flags;
        uint8_t argCount;
    };
}  ;

  struct ASTModule {
    List<ASTFunc> * funcs;
    List<ASTExpr> * exprs;
    List<ASTType> * types;
    List<ASTVar> * globals;
    List<ASTImport> * imports;
    List<ASTFunc> * tests;
    char* name;
    char* moduleName;
}  ;

class Parser {
    int errCount=0, errLimit = 20;
    void errorIncrement( )
    {
        if (++errCount >= errLimit)
            fatal("\ntoo many errors (%d), quitting\n", errLimit);
    }

    void errorExpectedToken(const TokenKind expected)
    {
        eprintf("\n(%d) \e[31merror:\e[0m at %s%s:%d:%d\n"
                "      expected '%s' found '%s'\n",
                errCount + 1, RELF(filename), token.line,
                token.col, expected.repr(false),
                TokenKind_repr(token.kind, false));
        errorIncrement();
    }

    void errorParsingExpr( )
    {
        eprintf("\n(%d) \e[31merror:\e[0m at %s%s:%d/%d\n"
                "      failed to parse expr, giving up\n",
                errCount + 1, RELF(filename), token.line - 1,
                token.line);
        errorIncrement();
    }

    void errorInvalidIdent( )
    {
        eprintf("\n(%d) \e[31merror:\e[0m invalid name '%.*s' at "
                "%s%s:%d:%d\n",
                errCount + 1, token.matchlen, token.pos,
                RELF(filename), token.line, token.col);
        errorIncrement();
    }

    void errorInvalidTypeMember( )
    {
        eprintf("\n(%d) \e[31merror:\e[0m invalid member at %s%s:%d\n",
                errCount + 1, RELF(filename), token.line - 1);
        errorIncrement();
    }

    void errorUnrecognizedVar(const ASTExpr&   expr)
    {
        eprintf("\n(%d) \e[31merror:\e[0m unknown variable "
                "\e[34m%s\e[0m at %s%s:%d:%d\n",
                errCount + 1, expr.string, RELF(filename), expr.line,
                expr.col);
        errorIncrement();
    }

    void warnUnusedArg(const  ASTVar&   var)
    {
        eprintf("\n(%d) \e[33mwarning:\e[0m unused argument "
                "\e[34m%s\e[0m at %s%s:%d:%d\n",
                ++warnCount, var.name, RELF(filename), var.line,
                var.col); // every var has init!! and every
                           // var is indented 4 spc
                           // ;-)
                           // errorIncrement();
    }
    void warnUnusedVar(const ASTVar &  var)
    {
        eprintf("\n(%d) \e[33mwarning:\e[0m unused variable "
                "\e[34m%s\e[0m at %s%s:%d:%d\n",
                ++warnCount, var.name, RELF(filename), var.line,
                var.col);
    }
    void errorDuplicateVar(    const ASTVar &var,const ASTVar & orig)
    {
        eprintf("\n(%d) \e[31merror:\e[0m duplicate variable "
                "\e[34m%s\e[0m at %s%s:%d:%d\n   "
                "          already declared at %s%s:%d:%d\n",
                errCount + 1, var.name, RELF(filename), var.line,
                var.col, RELF(filename), orig.line,
                orig.col);
        errorIncrement();
    }

    void errorUnrecognizedFunc(   const ASTExpr& expr, const char& selector)
    {
        eprintf("\n(%d) \e[31merror:\e[0m can't resolve call to "
                "\e[34m%s\e[0m at %s%s:%d:%d\n"
                "        selector is \e[34m%s\e[0m\n",
                errCount + 1, expr.string, RELF(filename), expr.line,
                expr.col, selector);
        errorIncrement();
    }
    void errorArgsCountMismatch( const ASTExpr& expr)
    {
        assert(expr.kind == tkCallRes);
        eprintf("\n(%d) \e[31merror:\e[0m arg count mismatch for "
                "\e[34m%s\e[0m at %s%s:%d:%d\n"
                "          have %d args, need %d, func defined at %s%s:%d\n",
                errCount + 1, expr.func.name, RELF(filename),
                expr.line, expr.col,  expr.left.countCommaList,
                expr.func.argCount, RELF(filename), expr.func.line);
        errorIncrement();
    }
    void errorIndexDimsMismatch( const ASTExpr&  expr)
    {
        assert(expr.kind == tkSubscriptResolved);
        int reqdDims = expr.var.typeSpec.dims;
        if (not reqdDims)
            eprintf("\n(%d) \e[31merror:\e[0m not an array: "
                    "\e[34m%s\e[0m at %s%s:%d:%d\n"
                    "          indexing a non-array with %d dims, var defined "
                    "at %s%s:%d\n",
                    errCount + 1, expr.var.name, RELF(filename),
                    expr.line, expr.col, ASTExpr_countCommaList(expr.left),
                    RELF(filename), expr.var.typeSpec.line);
        else
            eprintf(
                    "(%d) \e[31merror:\e[0m index dims mismatch for "
                    "\e[34m%s\e[0m at %s%s:%d:%d\n"
                    "          have %d indexes, need %d, var defined at %s%s:%d\n",
                    errCount + 1, expr.var.name, RELF(filename),
                    expr.line, expr.col, ASTExpr_countCommaList(expr.left),
                    reqdDims, RELF(filename), expr.var.typeSpec.line);
        errorIncrement();
    }
    void errorMissingInit( const ASTExpr&  expr)
    {
        assert(expr.kind == TKVarAssign);
        eprintf("\n(%d) \e[31merror:\e[0m missing initializer for "
                "\e[34m%s\e[0m at %s%s:%d-%d\n",
                errCount + 1, expr.var.name, RELF(filename),
                expr.line - 1, expr.line);
        errorIncrement();
    }

    void errorUnrecognizedType( const ASTTypeSpec& typeSpec)
    {
        eprintf("\n(%d) \e[31merror:\e[0m unknown typespec \e[33m%s\e[0m "
                "at %s%s:%d:%d\n",
                errCount + 1, typeSpec.name, RELF(filename),
                typeSpec.line, typeSpec.col);
        errorIncrement();
    }

    void errorTypeMismatchBinOp( const ASTExpr& expr)
    {
        eprintf("\n(%d) \e[31merror:\e[0m type mismatch for operands of '"
                "\e[34m%s\e[0m' at %s%s:%d:%d\n",
                errCount + 1, TokenKind_repr(expr.kind, false),
                RELF(filename), expr.line, expr.col);
        errorIncrement();
    }

    void errorReadOnlyVar( const ASTExpr& expr)
    {
        eprintf("\n(%d) \e[31merror:\e[0m mutating read-only variable '"
                "\e[34m%s\e[0m' at %s%s:%d:%d\n",
                errCount + 1, expr.var.name, RELF(filename),
                expr.line, expr.col);
        errorIncrement();
    }
    void errorInvalidTypeForOp( const ASTExpr& expr)
    {
        eprintf("\n(%d) \e[31merror:\e[0m invalid types for operator '"
                "\e[34m%s\e[0m' at %s%s:%d:%d\n",
                errCount + 1, TokenKind_repr(expr.kind, false),
                RELF(filename), expr.line, expr.col);
        errorIncrement();
    }
    void errorArgTypeMismatch(  const ASTExpr& expr,const ASTVar& var)
    {
        eprintf("\n(%d) \e[31merror:\e[0m type mismatch for argument '"
                "\e[34m%s\e[0m' at %s%s:%d:%d\n",
                errCount + 1, var.name, RELF(filename), expr.line,
                expr.col);
        errorIncrement();
    }

    void errorUnexpectedToken()
    {
        eprintf("\n(%d) \e[31merror:\e[0m at %s%s:%d:%d\n      unexpected "
                "token '%.*s'\n",
                errCount + 1, RELF(filename), token.line,
                token.col, token.matchlen, token.pos);
        errorIncrement();
    }

    void errorUnexpectedExpr( const ASTExpr& expr)
    {
        eprintf("\n(%d) \e[31merror:\e[0m at %s%s:%d:%d\n"
                "      unexpected expr '%s'",
                errCount + 1, RELF(filename), expr.line, expr.col,
                expr.opPrec ? TokenKind_repr(expr.kind, false) : expr.name);
        errorIncrement();
    }
};
