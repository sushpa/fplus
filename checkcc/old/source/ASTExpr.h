
ASTExpr::ASTExpr() {}
ASTExpr::ASTExpr(const Token* token)
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

    switch (kind) {
    case TKIdentifier:
    case TKString:
    case TKRegex:
    case TKInline:
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
}
Value ASTExpr::eval()
{
    // TODO: value is a union of whatever
    Value v;
    v.d = 0;
    return v;
}
TypeTypes ASTExpr::evalType()
{
    // TODO:
    // this func should return union {ASTType*; TypeTypes;} with the
    // TypeTypes value reduced by 1 (since it cannot be unresolved and
    // since 0 LSB would mean valid pointer (to an ASTType)).
    // this func does all type checking too btw and reports errors.
    return TYBool;
}

void ASTExpr::gen(int level, bool spacing, bool escapeStrings)
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
        printf("%.*s", strLength, value.string);
        break;

    case TKString:
        printf(escapeStrings ? "\\%.*s\\\"" : "%.*s\"", strLength - 1,
            value.string);
        break;

    case TKLineComment:
        printf("%s%.*s", TokenKind_repr(TKLineComment, *value.string != ' '),
            strLength, value.string);
        break;

    case TKFunctionCall:
        printf("%.*s(", strLength, name);
        if (left) left->gen(0, false, escapeStrings);
        printf(")");
        break;

    case TKSubscript:
        printf("%.*s[", strLength, name);
        if (left) left->gen(0, false, escapeStrings);
        printf("]");
        break;

    case TKVarAssign:
        // var x as XYZ = abc... -> becomes an ASTVar and an ASTExpr
        // (to keep location). Send it to ASTVar::gen.
        assert(var != NULL);
        var->gen();
        break;

    case TKKeyword_for:
    case TKKeyword_if:
    case TKKeyword_while:
        printf("%s ", TokenKind_repr(kind));
        if (left) left->gen(0, true, escapeStrings);
        puts("");
        if (body) body->gen(level + STEP); //, true, escapeStrings);
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
        if (left)
            left->gen(
                0, spacing and !leftBr and kind != TKOpColon, escapeStrings);
        if (leftBr) putc(lpc, stdout);

        printf("%s", TokenKind_repr(kind, spacing));

        char rpo = rightBr and right->kind == TKOpColon ? '[' : '(';
        char rpc = rightBr and right->kind == TKOpColon ? ']' : ')';
        if (rightBr) putc(rpo, stdout);
        if (right)
            right->gen(
                0, spacing and !rightBr and kind != TKOpColon, escapeStrings);
        if (rightBr) putc(rpc, stdout);

        if (kind == TKPower and not spacing) putc(')', stdout);
        if (kind == TKArrayOpen) putc(']', stdout);
    }
}
void ASTExpr::catarglabels()
{
    switch (this->kind) {
    case TKOpComma:
        this->left->catarglabels();
        this->right->catarglabels();
        break;
    case TKOpAssign:
        printf("_%s", this->left->name);
        break;
    default:
        break;
    }
}
void ASTExpr::genc(int level, bool spacing, bool inFuncArgs, bool escapeStrings)
{
    // generally an expr is not split over several lines (but maybe in
    // rare cases). so level is not passed on to recursive calls.

    printf("%.*s", level, spaces);
    switch (kind) {
    case TKNumber:
    case TKMultiDotNumber:
        printf("%.*s", strLength, value.string);
        break;

    case TKString:
        printf(escapeStrings ? "\\%.*s\\\"" : "%.*s\"", strLength - 1,
            value.string);
        break;

    case TKIdentifier:
    case TKIdentifierResolved:
        // convert a.b.c.d to DEREF3(a,b,c,d), a.b to DEREF(a,b) etc.
        {
            char* tmp = (kind == TKIdentifierResolved) ? var->name : name;
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
        value.string[0] = '"';
        value.string[strLength - 1] = '"';
        printf("%.*s", strLength, value.string);
        value.string[0] = '\'';
        value.string[strLength - 1] = '\'';
        break;

    case TKInline:
        value.string[0] = '"';
        value.string[strLength - 1] = '"';
        printf("mkRe_(%.*s)", strLength, value.string);
        value.string[0] = '`';
        value.string[strLength - 1] = '`';
        break;

    case TKLineComment:
        // TODO: skip  comments in generated code
        printf("// %.*s", strLength, value.string);
        break;

    case TKFunctionCall:
    case TKFunctionCallResolved: {
        char* tmp = (kind == TKFunctionCallResolved) ? func->name : name;
        str_tr_ip(tmp, '.', '_'); // this should have been done in a previous
                                  // stage prepc() or lower()
        printf("%s", tmp);
        if (*tmp >= 'A' and *tmp <= 'Z' and not strchr(tmp, '_'))
            printf("_new_"); // MyType() generates MyType_new_()
                             // TODO: if constructors for MyType are defined,
                             // they should
        // generate both a _init_arg1_arg2 function AND a corresponding
        // _new_arg1_arg2 func.
        if (left) left->catarglabels();
        str_tr_ip(tmp, '_',
            '.'); // this won't be needed, prepc will do the "mangling"
        printf("(");

        if (left) left->genc(0, false, true, escapeStrings);

        if (strcmp(tmp, "print")) {
            // more generally this IF is for those funcs that are standard
            // and dont need any instrumentation
            printf("\n#ifdef DEBUG\n"
                   "      %c THISFILE \":%d:\\033[0m\\n     -> ",
                left ? ',' : ' ', line);
            this->gen(0, false, true);
            printf("\"\n"
                   "#endif\n        ");
            // printf("_err_"); // temporary -- this
            // should be if the function throws. resolve the func first
        }
        printf(")");
        break;
    }

    case TKSubscriptResolved:
    case TKSubscript: // TODO
                      // TODO: lookup the var, its typespec, then its dims. then
                      // slice
        // here should be slice1D slice2D etc.
        {
            char* tmp = (kind == TKSubscriptResolved) ? var->name : name;
            printf("slice(%s, {", tmp);
            if (left) left->genc(0, false, inFuncArgs, escapeStrings);
            printf("})");
            break;
        }
    case TKOpAssign:
        if (!inFuncArgs) {
            left->genc(0, spacing, inFuncArgs, escapeStrings);
            printf("%s", TokenKind_repr(TKOpAssign, spacing));
        }
        right->genc(0, spacing, inFuncArgs, escapeStrings);
        // check various types of lhs  here, eg arr[9:87] = 0,
        // map["uuyt"]="hello" etc.
        break;

    case TKOpColon: // convert 3:4:5 to range(...)
                    // must do bounds check first!
        printf("%s(", left->kind != TKOpColon ? "range_to" : "range_to_by");
        if (left->kind == TKOpColon) {
            left->kind = TKOpComma;
            left->genc(0, false, inFuncArgs, escapeStrings);
            left->kind = TKOpColon;
        } else
            left->genc(0, false, inFuncArgs, escapeStrings);
        printf(", ");
        right->genc(0, false, inFuncArgs, escapeStrings);
        printf(")");
        break;

    case TKVarAssign: // basically a TKOpAssign corresponding to a local var
                      // var x as XYZ = abc... -> becomes an ASTVar and an
                      // ASTExpr (to keep location). Send it to ASTVar::gen.
        if (var->init != NULL) {
            printf("%s = ", var->name);
            var->init->genc(0, true, inFuncArgs, escapeStrings);
        }
        break;

    case TKKeyword_for:
    case TKKeyword_if:
    case TKKeyword_while:
        if (kind == TKKeyword_for)
            printf("FOR(");
        else
            printf("%s (", TokenKind_repr(kind));
        if (kind == TKKeyword_for) left->kind = TKOpComma;
        if (left) left->genc(0, spacing, inFuncArgs, escapeStrings);
        if (kind == TKKeyword_for) left->kind = TKOpAssign;
        puts(") {");
        if (body) body->genc(level + STEP);
        printf("%.*s}", level, spaces);
        break;

    case TKPower:
        printf("pow(");
        left->genc(0, false, inFuncArgs, escapeStrings);
        printf(",");
        right->genc(0, false, inFuncArgs, escapeStrings);
        printf(")");
        break;

    case TKKeyword_return:
        printf("{_err_ = NULL; _scDepth_--; return ");
        right->genc(0, spacing, inFuncArgs, escapeStrings);
        printf(";}\n");
        break;

    default:
        if (not opPrecedence) break;
        // not an operator, but this should be error if you reach here
        bool leftBr = left and left->opPrecedence
            and left->opPrecedence < this->opPrecedence;
        bool rightBr = right and right->opPrecedence
            and right->kind != TKKeyword_return // found in 'or return'
            and right->opPrecedence < this->opPrecedence;

        char lpo = '(';
        char lpc = ')';
        if (leftBr) putc(lpo, stdout);
        if (left)
            left->genc(0, spacing and !leftBr and kind != TKOpColon, inFuncArgs,
                escapeStrings);
        if (leftBr) putc(lpc, stdout);

        if (kind == TKArrayOpen)
            putc('{', stdout);
        else
            printf("%s", TokenKind_repr(kind, spacing));

        char rpo = '(';
        char rpc = ')';
        if (rightBr) putc(rpo, stdout);
        if (right)
            right->genc(0, spacing and !rightBr and kind != TKOpColon,
                inFuncArgs, escapeStrings);
        if (rightBr) putc(rpc, stdout);

        if (kind == TKArrayOpen) putc('}', stdout);
    }
}