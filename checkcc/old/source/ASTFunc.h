void ASTFunc::gen(int level)
{
    printf("function %s(", name);

    foreach (arg, args, this->args) {
        arg->gen(level);
        printf(args->next ? ", " : "");
    }
    printf(")");

    if (returnType) {
        printf(" as ");
        returnType->gen(level);
    }
    puts("");

    body->gen(level + STEP);

    puts("end function\n");
}

const char* ASTFunc::getDefaultValueForType(ASTTypeSpec* type)
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

void ASTFunc::genc(int level)
{

    if (flags.isDeclare) return;

    printf("\n// ------------------------ %s \n", name);
    printf("#define DEFAULT_VALUE %s\n", getDefaultValueForType(returnType));
    if (!flags.exported) printf("static ");
    if (returnType) {
        returnType->genc(level);
    } else
        printf("void");
    str_tr_ip(name, '.', '_');
    printf(" %s", name);
    str_tr_ip(name, '_', '.');
    // TODO: here add the type of the first arg, unless it is a method
    // of a type
    if (this->args.next) { // means at least 2 args
        foreach (arg, nargs, *(this->args.next)) { // start from the 2nd
            printf("_%s", arg->name);
        }
    }
    printf("(");
    foreach (arg, args, this->args) {
        arg->genc(level, true);
        printf(args->next ? ", " : "");
    }

    printf("\n#ifdef DEBUG\n"
           "    %c const char* callsite_ "
           "\n#endif\n",
        ((this->args.item ? ',' : ' ')));

    // TODO: if (flags.throws) printf("const char** _err_");
    puts(") {");
    printf("#ifdef DEBUG\n"
           "    static const char* sig_ = \"");
    printf("%s%s(", flags.isStmt ? "" : "function ", name);
    foreach (arg, chargs, this->args) {
        arg->gen(level);
        printf(chargs->next ? ", " : ")");
    }
    if (returnType) {
        printf(" as ");
        returnType->gen(level);
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

    body->genc(level + STEP);

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

void ASTFunc::genh(int level)
{
    if (flags.isDeclare) return;
    if (!flags.exported) printf("static ");
    if (returnType) {
        returnType->genc(level);
    } else
        printf("void");
    str_tr_ip(name, '.', '_');
    printf(" %s", name);
    str_tr_ip(name, '_', '.');
    if (this->args.next) { // means at least 2 args
        foreach (arg, nargs, *(this->args.next)) {
            printf("_%s", arg->name);
        }
    }
    printf("(");

    foreach (arg, args, this->args) {
        arg->genc(level, true);
        printf(args->next ? ", " : "");
    }
    printf("\n#ifdef DEBUG\n    %c const char* callsite_ "
           "\n#endif\n",
        ((this->args.item) ? ',' : ' '));
    puts(");\n");
}