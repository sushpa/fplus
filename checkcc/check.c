#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define true 1
#define false 0
typedef char bool_t;

#include "str.h"
#include "token.h"
#include "node.h"
#include "context.h"
#include "errors.h"
#include "alloc.h"
#include "parser.h"

// MAIN

int main(int argc, char* argv[])
{
    if (argc == 1) return 1;
    print_sizes();
    Parser* parser = Parser_new(argv[1], skipws);
    Parser_parse(parser);
    if (!parser->errCount) print_tree(parser->modules, 0);
    alloc_stat();
    return 0;
}

// linter needs to :
// 1. format spacing line breaks etc.
// 2. replace E in floats with e
// 3. rearrange comments
// 4. replace end with end if/for/etc.
