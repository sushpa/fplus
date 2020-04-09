ASTVar* ASTType::getVar(const char* name)
{
    // stupid linear search, no dictionary yet
    foreach (var, vars, this->vars) {
        if (!strcmp(name, var->name)) return var;
    }
    if (super and super->typeType == TYObject) return super->type->getVar(name);
    return NULL;
}

void ASTType::gen(int level)
{
    printf("type %s", name);
    if (params.item) {
        ; // TODO: figure out what to do about params
    }
    if (super) {
        printf(" extends ");
        super->gen(level);
    }
    puts("");

    // TODO: rename to exprs or body. this contains vars also
    foreach (check, checks, this->checks) {
        if (!check) continue;
        check->gen(level + STEP, true, false);
        puts("");
    }

    puts("end type\n");
}

void ASTType::genc(int level)
{
    printf("#define FIELDS_%s \\\n", name);
    foreach (var, fvars, this->vars) {
        if (!var) continue;
        var->genc(level + STEP);
        printf("; \\\n");
    }
    printf("\n\nstruct %s {\n", name);

    if (super) {
        printf("    FIELDS_");
        super->genc(level);
        printf("\n");
    }

    printf("    FIELDS_%s\n};\n\n", name);
    printf("static const char* %s_name_ = \"%s\";\n", name, name);
    printf("static %s %s_alloc_() {\n    return _Pool_alloc_(&gPool_, "
           "sizeof(struct %s));\n}\n",
        name, name, name);
    printf("static %s %s_init_(%s self) {\n", name, name, name);
    // TODO: rename this->checks to this->exprs or this->body
    foreach (var, vars, this->vars) {
        printf("#define %s self->%s\n", var->name, var->name);
    }
    foreach (check, checks, this->checks) {
        if (!check or check->kind != TKVarAssign or !check->var->init) continue;
        check->genc(level + STEP, true, false, false);
        puts(";");
    }
    foreach (var, mvars, this->vars) {
        printf("#undef %s \n", var->name);
    }

    printf("    return self;\n}\n");
    printf("%s %s_new_() {\n    return "
           "%s_init_(%s_alloc_());\n}\n",
        name, name, name, name);
    printf("#define %s_print_ %s_print__(p, STR(p))\n", name, name);
    printf("%s %s_print__(%s self, const char* name) {\n    printf(\"<%s "
           "'%%s' at %%p>\",name,self);\n}\n",
        name, name, name, name);

    puts("");
}

void ASTType::genh(int level)
{
    printf("typedef struct %s* %s; struct %s;\n", name, name, name);
}