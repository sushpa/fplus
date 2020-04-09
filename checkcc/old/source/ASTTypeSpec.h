void ASTTypeSpec::genc(int level, bool isconst)
{
    if (isconst) printf("const ");
    if (dims) printf("Array%dD(", dims);

    switch (typeType) {
    case TYObject:
        printf("%s", type->name);
        break;
    case TYUnresolved:
        printf("%s", name);
        break;
    default:
        printf("%s", TypeType_nativeName(typeType));
        break;
    }

    // if (isconst) printf(" const"); // only if a ptr type
    if (dims) printf("%s", ")");
    //        if (status == TSDimensionedNumber) {
    //            units->genc(level);
    //        }
}

void ASTTypeSpec::gen(int level)
{
    // if (status == TSUnresolved)
    printf("%s", name);
    //        if (status==TSResolved) printf("%s", type->name);
    if (dims) printf("%s", "[]" /*dimsGenStr(dims)*/);
    // if (status == TSDimensionedNumber) units->gen(level);
}