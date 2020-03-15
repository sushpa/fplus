
//=============================================================================
// ERROR REPORTING
//=============================================================================

#define ERR_MAX 20

void error__increment(parser_t* parser)
{
    parser->errCount++;
    if (parser->errCount >= ERR_MAX) {
        fprintf(stderr, "too many errors reported (%d), quitting\n", ERR_MAX);
        exit(1);
    }
}

void error_unexpectedToken(
    parser_t* parser, token_kind_e expected)
{
    fprintf(stderr, "./%s:%d:%d: expected '%s' found '%.*s'\n",
        parser->filename, parser->token.line, parser->token.col,
        token_repr(expected), parser->token.matchlen, parser->token.pos);
    error__increment(parser);
}

/*
#define error_report_unexpectedToken(a, b)                                 \
    error_report_unexpectedToken(a, b, __FILE__, __func__, __LINE__)
*/
