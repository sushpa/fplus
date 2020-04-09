ASTType* ASTModule::getType(const char* name)
{
    // the type may be "mm.XYZType" in which case you should look in
    // module mm instead. actually the caller should have bothered about
    // that.
    foreach (type, types, this->types) {
        if (!strcmp(type->name, name)) return type;
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
bool ASTModule::hasImportAlias(const char* alias)
{
    foreach (imp, imps, this->imports) {
        if (!strcmp(imp->importFile + imp->aliasOffset, alias)) return true;
    }
    return false;
}

ASTFunc* ASTModule::getFunc(const char* name)
{
    // figure out how to deal with overloads. or set a selector field in
    // each astfunc.
    foreach (func, funcs, this->funcs) {
        if (!strcmp(func->name, name)) return func;
    }
    // again no looking anywhere else. If the name is of the form
    // "mm.func" you should have bothered to look in mm instead.
    return NULL;
}

void ASTModule::gen(int level)
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

void ASTModule::genc(int level)
{
    foreach (import, imports, this->imports)
        import->genc(level);

    puts("");

    foreach (type, types, this->types)
        type->genh(level);

    foreach (func, funcs, this->funcs)
        func->genh(level);

    foreach (type, mtypes, this->types)
        type->genc(level);

    foreach (func, mfuncs, this->funcs)
        func->genc(level);

    foreach (simport, simports, this->imports)
        simport->undefc();
}