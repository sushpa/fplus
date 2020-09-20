#include "fp_base.h"
#include "fp_sys_time.h"

typedef struct XMLAttr XMLAttr;
typedef struct XMLNode XMLNode;
typedef struct XMLParser XMLParser;

struct XMLAttr
{
    const char *key;
    const char *val;
};

struct XMLNode
{
    const char *tag;
    List(XMLAttr) * attributes;
    union
    {
        const char *text;
        List(XMLNode) * children;
    };
};

struct XMLParser
{
    const char *filename;
    int line, col;
    char *data, *end;
    char *pos;
};

MKSTAT(XMLNode)
MKSTAT(XMLAttr)
MKSTAT(XMLParser)

XMLNode *XMLNode_new(const char *tag)
{
    XMLNode *ret = fp_new(XMLNode);
    ret->tag = tag;
    return ret;
}

XMLNode *XMLNode_newText(const char *text)
{
    XMLNode *ret = fp_new(XMLNode);
    ret->tag = NULL;
    ret->text = text;
    return ret;
}

XMLAttr *XMLAttr_new(const char *key, const char *value)
{
    XMLAttr *ret = fp_new(XMLAttr);
    ret->key = key;
    ret->val = value;
    return ret;
}

XMLParser *XMLParser_fromStringClone(const char *str)
{
    XMLParser *par = fp_new(XMLParser);
    size_t len = strlen(str);
    par->data = pstrndup(str, len);
    par->pos = par->data;
    par->end = par->data + len;
    return par;
}

XMLParser *XMLParser_fromFile(char *filename)
{
    size_t flen = strlen(filename);

    // Error: the file might not end in .ch
    // if (not str_endswith(filename, flen, ".fp", 3)) {
    //     eprintf("F+: file '%s' invalid: name must end in '.fp'.\n", filename);
    //     return NULL;
    // }

    struct stat sb;
    // Error: the file might not exist
    if (stat(filename, &sb) != 0)
    {
        eprintf("F+: file '%s' not found.\n", filename);
        return NULL;
    }
    else if (S_ISDIR(sb.st_mode))
    {
        // Error: the "file" might really be a folder
        eprintf("F+: '%s' is a folder; only files are accepted.\n", filename);
        return NULL;
    }
    else if (access(filename, R_OK) == -1)
    {
        // Error: the user might not have read permissions for the file
        eprintf("F+: no permission to read file '%s'.\n", filename);
        return NULL;
    }

    FILE *file = fopen(filename, "r");
    assert(file);

    XMLParser *ret = fp_new(XMLParser);

    ret->filename = filename;
    // ret->noext = str_noext(filename);
    fseek(file, 0, SEEK_END);
    const size_t size = ftell(file);

    // if (size < FILE_SIZE_MAX) {
    ret->data = (char *)malloc(size);
    fseek(file, 0, SEEK_SET);
    if (fread(ret->data, size, 1, file) != 1)
    {
        eprintf("F+: the whole file '%s' could not be read.\n", filename);
        fclose(file);
        return NULL;
        // would leak if ret was malloc'd directly, but we have a pool
    }
    // ret->data[size - 1] = 0;
    // ret->data[size - 2] = 0;
    // ret->moduleName = str_tr(ret->noext, '/', '.');
    // ret->mangledName = str_tr(ret->noext, '/', '_');
    // ret->capsMangledName = str_upper(ret->mangledName);
    ret->end = ret->data + size;
    ret->pos = ret->data;
    // ret->token.skipWhiteSpace = skipws;
    // ret->token.mergeArrayDims = false;
    // ret->token.kind = tkUnknown;
    // ret->token.line = 1;
    // ret->token.col = 1;
    // ret->mode = PMGenC; // parse args to set this
    // ret->errCount = 0;
    // ret->warnCount = 0;
    // ret->errLimit = 50;
    // } else {
    //     eputs("Source files larger than 16MB are not allowed.\n");
    // }

    fclose(file);
    return ret;
}

bool isAnyOf(char ch, char *chars)
{
    while (*chars)
        if (*chars++ == ch)
            return true;
    return false;
}

List(XMLAttr) * XMLParser_parseAttrs(XMLParser *parser)
{
    List(XMLAttr) *list = NULL;
    List(XMLAttr) **listp = &list;

    while (*parser->pos and *parser->pos != '>' and *parser->pos != '/' and *parser->pos != '?')
    {
        const char *name = parser->pos;
        while (*parser->pos != '=')
            parser->pos++;
        *parser->pos++ = 0; // trample =

        char *markers;
        if (isAnyOf(*parser->pos, "'\""))
        {
            markers = (*parser->pos == '"') ? "\"" : "'";
            parser->pos++;
        }
        else
        {
            markers = " >/?\t\n";
        }

        const char *value = parser->pos;

        while (not isAnyOf(*parser->pos, markers))
            parser->pos++;
        if (not isAnyOf(*parser->pos, "/?>"))
            *parser->pos++ = 0; // trample the marker if " or ' or spaces
        // ^ DON't trample the markers / or > here, because in the case of e.g.
        // <meta name=content-type content=utf8/>
        // it will trample the / not allowing the calling parseTags to
        // detect that the tag is self-closing. Let the caller trample.

        while (isAnyOf(*parser->pos, " \t\n"))
            *parser->pos++ = 0; // skip and trample whitespace

        XMLAttr *attr = XMLAttr_new(name, value);
        listp = fp_PtrList_append(listp, attr);
    }
    //    if (*parser->pos == '/') *parser->pos++ = 0;
    //    assert(*parser->pos == '>');
    //    *parser->pos++ = 0;
    // don't consume ending />, leave it at that
    return list;
}

List(XMLNode) * XMLParser_parseTags(XMLParser *parser)
{
    List(XMLNode) *list = NULL;
    List(XMLNode) **listp = &list;

    while (parser->pos < parser->end)
    {
        switch (*parser->pos)
        {
        case ' ':
        case '\t':
        case '\n':
            parser->pos++;
            break;
        case '<':
        {
            *parser->pos++ = 0;

            bool noChild = false;

            if (*parser->pos == '/')
            {                  // closing tag here means empty content
                parser->pos++; // note that will be past </
                return list;   // null if no elements were found at this level
            }
            else if (not strncasecmp(parser->pos, "[CDATA[", 6))
            { // cdata
            }
            else
            { // opening tag
                XMLNode *node = XMLNode_new(parser->pos);
                while (not isAnyOf(*parser->pos, " />\n\t"))
                    parser->pos++;

                if (*parser->pos == ' ')
                {
                    *parser->pos++ = 0;
                    while (*parser->pos == ' ')
                        parser->pos++;
                    node->attributes = XMLParser_parseAttrs(parser);
                }
                while (*parser->pos == ' ')
                    parser->pos++;

                switch (*parser->pos)
                {
                case '/':
                case '?':
                    *parser->pos++ = 0;
                    assert(*parser->pos == '>');
                    noChild = true;
                //  fall through
                case '>':
                    // tag has ended. parse children
                    *parser->pos++ = 0;
                    if (not noChild)
                    {
                        node->children = XMLParser_parseTags(parser);

                        char *closingTag = parser->pos;
                        while (*parser->pos != '>')
                            parser->pos++; // seek to end of closing tag
                        *parser->pos++ = 0;

                        if (not *closingTag)
                        {
                            printf("error: found end of file, expected </%s>\n",
                                   node->tag);
                            exit(1);
                        }
                        else if (strcasecmp(closingTag, node->tag))
                        {
                            printf("error: found </%s>, expected </%s>\n",
                                   closingTag, node->tag);
                            exit(1);
                        }
                    }
                    break;
                default:
                    eprintf("oops1: unexpected '%c' (\"%.16s\"...)\n", *parser->pos, parser->pos);
                    break;
                }

                listp = fp_PtrList_append(listp, node);
            }
        }
        break;

        default:

        {
            char *text = parser->pos;
            //printf("oops2: unexpected '%c' (\"%.16s...\")\n", *parser->pos, parser->pos);
            while (*parser->pos != '<' and parser->pos < parser->end)
                parser->pos++;
            // relying on the </ detector state to trample the <
            XMLNode *textNode = XMLNode_newText(text);
            listp = fp_PtrList_append(listp, textNode);
        }
        }
    }
    if (parser->pos < parser->end)
        printf("error: data unparsed\n");
    return list;
}

void XMLAttr_print(XMLAttr *attr, int indent)
{
    printf(" %s=\"%s\"", attr->key, attr->val);
}

void XMLNode_print(XMLNode *node, int indent);
void XMLNodeList_print(List(XMLNode) * nodeList, int indent)
{
    fp_foreach(XMLNode *, childNode, nodeList) XMLNode_print(childNode, indent);
}

const char *const spaces = "                                                 ";
void XMLNode_print(XMLNode *node, int indent)
{
    if (node->tag)
    {
        printf("%.*s<%s%s", indent, spaces, node->tag,
               node->attributes ? "" : node->children ? ">\n" : "/>\n");
        fp_foreach(XMLAttr *, attr, node->attributes) XMLAttr_print(attr, indent);
        if (node->attributes)
            printf("%s\n", node->children ? ">" : " />");
        XMLNodeList_print(node->children, indent + 2);
        if (node->children)
            printf("%.*s</%s>\n", indent, spaces, node->tag);
    }
    else
    {
        printf("%.*s%s\n", indent, spaces, node->text);
    }
}

int main(int argc, char *argv[])
{
    char *xmlstr = ""                                              //
                   "<xml>"                                         //
                   "  <head>"                                      //
                   "    <title attr=value attr2=\"value2 what>\">" //
                   "    </title>"                                  //
                   "    <meta name=content-type content=utf-8/>"   //
                   "  </head>"                                     //
                   "</xml>";                                       //
    "<foot>"                                                       //
    "</foot>";
    //     xmlstr = "<meta name=content-type content=utf-8/><meta name=keywords content='rail,train,goods'/>";
    //     xmlstr = "<a>";
    //     xmlstr = "<a></a>";

    xmlstr = "<?xml version='1.0'?>"
             "<Tests xmlns='http://www.adatum.com'>"
             "<Test TestId='0001' TestType='CMD'>"
             "<Name>Convert number to string</Name>"
             "<CommandLine>Examp1.EXE</CommandLine>"
             "<Input>1</Input>"
             "<Output>One</Output>"
             "</Test>"
             "<Test TestId='0002' TestType='CMD'>"
             "<Name>Find succeeding characters</Name>"
             "<CommandLine>Examp2.EXE</CommandLine>"
             "<Input>abc</Input>"
             "<Output>def</Output>"
             "</Test>"
             "<Test TestId='0003' TestType='GUI'>"
             "<Name>Convert multiple numbers to strings</Name>"
             "<CommandLine>Examp2.EXE /Verbose</CommandLine>"
             "<Input>123</Input>"
             "<Output>One Two Three</Output>"
             "</Test>"
             "<Test TestId='0004' TestType='GUI'>"
             "<Name>Find correlated key</Name>"
             "<CommandLine>Examp3.EXE</CommandLine>"
             "<Input>a1</Input>"
             "<Output>b1</Output>"
             "</Test>"
             "<Test TestId='0005' TestType='GUI'>"
             "<Name>Count characters</Name>"
             "<CommandLine>FinalExamp.EXE</CommandLine>"
             "<Input>This is a test</Input>"
             "<Output>14</Output>"
             "</Test>"
             "<Test TestId='0006' TestType='GUI'>"
             "<Name>Another Test</Name>"
             "<CommandLine>Examp2.EXE</CommandLine>"
             "<Input>Test Input</Input>"
             "<Output>10</Output>"
             "</Test>"
             "</Tests>";

    // XMLParser *par = XMLParser_fromStringClone(xmlstr);
    if (argc < 2)
    {
        printf("usage: %s <filename>\n", argv[0]);
        exit(1);
    }
    XMLParser *par = XMLParser_fromFile(argv[1]);
    fp_sys_time_Time t0 = fp_sys_time_getTime();
    List(XMLNode) *parsed = XMLParser_parseTags(par);
    XMLNodeList_print(parsed, 0);

    double tms = fp_sys_time_clockSpanMicro(t0) / 1.0e3;

    eputs("-------------------------------------------------------"
          "\n");
    allocstat(XMLAttr);
    allocstat(XMLParser);
    allocstat(XMLNode);
    allocstat(fp_PtrList);
    eputs("-------------------------------------------------------"
          "\n");
    eprintf("*** Total size of nodes                     = %7d B\n",
            fp_gPool->usedTotal);
    eprintf("*** Space allocated for nodes               = %7d B\n",
            fp_gPool->capTotal);
    eprintf("*** Node space utilisation                  = %7.1f %%\n",
            fp_gPool->usedTotal * 100.0 / fp_gPool->capTotal);
    eputs("-------------------------------------------------------"
          "\n");
    eprintf("*** File size                               = %7lu B\n",
            par->end - par->data);
    eprintf("*** Node size to file size ratio            = %7.1f x\n",
            fp_gPool->usedTotal * 1.0 / (par->end - par->data));
    eputs("-------------------------------------------------------"
          "\n");
    eprintf("*** Space used for strings                  = %7u B\n",
            fp_sPool->usedTotal);
    eprintf("*** Allocated for strings                   = %7u B\n",
            fp_sPool->capTotal);
    eprintf("*** Space utilisation                       = %7.1f %%\n",
            fp_sPool->usedTotal * 100.0 / fp_sPool->capTotal);
    eputs("-------------------------------------------------------"
          "\n");
    eputs("\e[1mMemory-related calls\e[0m\n");
    eprintf("  calloc: %-7d | malloc: %-7d | realloc: %-7d\n",
            fp_globals__callocCount, fp_globals__mallocCount,
            fp_globals__reallocCount);
    eprintf("  strlen: %-7d | strdup: %-7d |\n", fp_globals__strlenCount,
            fp_globals__strdupCount);

    eprintf("\e[1mTime elapsed:\e[0m %.1f ms (%.1f ms / 32kB)\n", tms,
            tms * 32768.0 / (par->end - par->data)); // sw.print();

    return 0;
}
