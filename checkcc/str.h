
//=============================================================================
// STRING
//=============================================================================

char* str_noext(const char* const str)
{
    char* s = strdup(str);
    const size_t len = strlen(s);
    char* sc = s + len;
    while (sc > s && *sc != '.')
        sc--;
    if (sc >= s) *sc = '\0';
    return s;
}

char* str_base(char* str)
{
    char* s = str;
    const size_t len = strlen(s);
    char* sc = s + len;
    while (sc > s && sc[-1] != '/')
        sc--;
    if (sc >= s) s = sc;
    return s;
}

char* str_dir(const char* const str)
{
    char* s = strdup(str);
    const size_t len = strlen(s);
    char* sc = s + len;
    while (sc > s && *sc != '/')
        sc--;
    if (sc >= s) *sc = '\0';
    return s;
}

char* str_upper(const char* const str)
{
    char* s = strdup(str);
    char* sc = s - 1;
    while (*++sc)
        if (*sc >= 'a' && *sc <= 'z') *sc -= 32;
    return s;
}

char* str_tr(const char* const str, const char oldc, const char newc)
{
    char* s = strdup(str);
    char* sc = s - 1;
    while (*++sc)
        if (*sc == oldc) *sc = newc;
    return s;
}
