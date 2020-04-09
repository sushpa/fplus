void ASTImport::gen(int level)
{
    printf("import %s%s%s%s\n", isPackage ? "@" : "", importFile,
        hasAlias ? " as " : "", hasAlias ? importFile + aliasOffset : "");
}
void ASTImport::genc(int level)
{
    str_tr_ip(importFile, '.', '_');
    printf("\n#include \"%s.h\"\n", importFile);
    if (hasAlias)
        printf("#define %s %s\n", importFile + aliasOffset, importFile);
    str_tr_ip(importFile, '_', '.');
}
void ASTImport::undefc()
{
    if (hasAlias) printf("#undef %s\n", importFile + aliasOffset);
}