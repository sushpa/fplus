
//=============================================================================
// PARSER CONTEXT
//=============================================================================

typedef struct parser_t {
    char* filename; // mod/submod/xyz/mycode.ch
    char* moduleName; // mod.submod.xyz.mycode
    char* mangledName; // mod_submod_xyz_mycode
    char* capsMangledName; // MOD_SUBMOD_XYZ_MYCODE
    char* basename; // mycode
    char* dirname; // mod/submod/xyz
    char *data, *end;
    token_t token; // current
    node_t* modules; // module node of the AST
    node_stack_t scopes; // a stack that keeps track of scope nesting
    // parse_state_e state; just read scopes[0] and see if its type or func
    uint8_t errCount;
} parser_t;

#define FILE_SIZE_MAX 1 << 24
parser_t* parser_new(char* filename, bool_t skipws)
{
    FILE* file = fopen(filename, "r");
    fprintf(stdout, "compiling %s\n", filename);
    char* noext = str_noext(filename);
    fseek(file, 0, SEEK_END);
    const size_t size = ftell(file) + 1;
    parser_t* parser = NULL;
    if (size < FILE_SIZE_MAX) { // 16MB max
        char* data = malloc(size);
        fseek(file, 0, SEEK_SET);
        fread(data, size, 1, file);
        parser = malloc(sizeof(parser_t));
        *parser = (parser_t){ //
            .filename = filename,
            .moduleName = str_tr(noext, '/', '.'),
            .mangledName = str_tr(noext, '/', '_'),
            .capsMangledName = str_upper(str_tr(noext, '/', '_')),
            .basename = str_base(noext),
            .dirname = str_dir(noext),
            .data = data,
            .end = data + size,
            .token = { //
                .pos = data,
                .kind = token_kind_unknown,
                .line = 1,
                .col = 0,
                .flags = { //
                    .skipws = skipws //
                } //
            }
        };
    }
    fclose(file);
    return parser;
}
