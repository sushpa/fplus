
void ASTScope::gen(int level)
{
    foreach (stmt, stmts, this->stmts) {
        stmt->gen(level, true, false);
        puts("");
    }
}

ASTVar* ASTScope::getVar(const char* name)
{
    // stupid linear search, no dictionary yet
    foreach (local, locals, this->locals) {
        if (!strcmp(name, local->name)) return local;
    }
    // in principle this will walk up the scopes, including the
    // function's "own" scope (which has locals = func's args). It
    // doesn't yet reach up to module scope, because there isn't yet a
    // "module" scope.
    if (parent) return parent->getVar(name);
    return NULL;
}

#define genLineNumbers 0
void ASTScope::genc(int level)
{
    foreach (local, locals, this->locals) {
        local->genc(level);
        puts(";");
    } // these will be declared at top and defined within the expr list
    foreach (stmt, stmts, this->stmts) {
        if (stmt->kind == TKLineComment) continue;
        if (genLineNumbers) printf("#line %d\n", stmt->line);
        stmt->genc(level, true, false, false);
        puts(";");
        puts("    if (_err_ == ERROR_TRACE) goto backtrace;");
    }
}