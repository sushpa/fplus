#include "jet_base.h"
#include "jet_sys_time.h"

typedef struct XMLAttr XMLAttr;
typedef struct XMLNode XMLNode;
typedef struct XMLParser XMLParser;

struct XMLAttr {
    const char* key;
    const char* val;
};

struct XMLNode {
    const char* tag;
    List(XMLAttr) * attributes;
    union {
        const char* text;
        List(XMLNode) * children;
    };
};

struct XMLParser {
    const char* filename;
    int line, col;
    char *data, *end;
    char* pos;
};

MKSTAT(XMLNode)
MKSTAT(XMLAttr)
MKSTAT(XMLParser)

static XMLNode* XMLNode_new(const char* tag)
{
    XMLNode* ret = jet_new(XMLNode);
    ret->tag = tag;
    return ret;
}

static XMLNode* XMLNode_newText(const char* text)
{
    XMLNode* ret = jet_new(XMLNode);
    ret->tag = NULL;
    ret->text = text;
    return ret;
}

static XMLAttr* XMLAttr_new(const char* key, const char* value)
{
    XMLAttr* ret = jet_new(XMLAttr);
    ret->key = key;
    ret->val = value;
    return ret;
}

static XMLParser* XMLParser_fromStringClone(const char* str)
{
    XMLParser* par = jet_new(XMLParser);
    size_t len = strlen(str);
    par->data = pstrndup(str, len);
    par->pos = par->data;
    par->end = par->data + len;
    return par;
}

static XMLParser* XMLParser_fromFile(char* filename)
{
    size_t flen = strlen(filename);

    struct stat sb;
    if (stat(filename, &sb) != 0) {
        eprintf("F+: file '%s' not found.\n", filename);
        return NULL;
    } else if (S_ISDIR(sb.st_mode)) {
        eprintf("F+: '%s' is a folder; only files are accepted.\n", filename);
        return NULL;
    } else if (access(filename, R_OK) == -1) {
        eprintf("F+: no permission to read file '%s'.\n", filename);
        return NULL;
    }

    FILE* file = fopen(filename, "r");
    assert(file);

    XMLParser* ret = jet_new(XMLParser);

    ret->filename = filename;
    // ret->noext = str_noext(filename);
    fseek(file, 0, SEEK_END);
    const size_t size = ftell(file);

    // if (size < FILE_SIZE_MAX) {
    ret->data = (char*)malloc(size);
    fseek(file, 0, SEEK_SET);
    if (fread(ret->data, size, 1, file) != 1) {
        eprintf("F+: the whole file '%s' could not be read.\n", filename);
        fclose(file);
        return NULL;
        // would leak if ret was malloc'd directly, but we have a pool
    }
    ret->end = ret->data + size;
    ret->pos = ret->data;

    fclose(file);
    return ret;
}

static bool isAnyOf(char ch, char* chars)
{
    while (*chars)
        if (*chars++ == ch) return true;
    return false;
}

static const char* findchars_fast(
    const char* buf, const char* buf_end, const char* chars, size_t chars_size)
{
    // *found = 0;
#if __SSE4_2__
    if (jet_likely(buf_end - buf >= 16)) {
        __m128i chars16 = _mm_loadu_si128((const __m128i*)chars);

        size_t left = (buf_end - buf) & ~15;
        do {
            __m128i b16 = _mm_loadu_si128((void*)buf);
            int r = _mm_cmpestri(chars16, chars_size, b16, 16,
                _SIDD_LEAST_SIGNIFICANT | _SIDD_CMP_EQUAL_ANY
                    | _SIDD_UBYTE_OPS);
            if (jet_unlikely(r != 16)) {
                buf += r;
                // *found = 1;
                break;
            }
            buf += 16;
            left -= 16;
        } while (jet_likely(left != 0));
    }
    return buf;
#else
    return buf + strcspn(buf, chars); // strpbrk(buf, chars);
#endif
}

static List(XMLAttr) * XMLParser_parseAttrs(XMLParser* parser)
{
    List(XMLAttr)* list = NULL;
    List(XMLAttr)** listp = &list;

    while (*parser->pos and *parser->pos != '>' and *parser->pos != '/'
        and *parser->pos != '?') {
        const char* name = parser->pos;
        while (*parser->pos != '=') parser->pos++;
        *parser->pos++ = 0; // trample =

        char* markers;
        if (isAnyOf(*parser->pos, "'\"")) {
            markers = (*parser->pos == '"') ? "\"" : "'";
            parser->pos++;
        } else {
            markers = " >/?\t\n";
        }

        const char* value = parser->pos;

        while (not isAnyOf(*parser->pos, markers)) parser->pos++;
        if (not isAnyOf(*parser->pos, "/?>"))
            *parser->pos++ = 0; // trample the marker if " or ' or spaces
        // ^ DON't trample the markers / or > here, because in the case of e.g.
        // <meta name=content-type content=utf8/>
        // it will trample the / not allowing the calling parseTags to
        // detect that the tag is self-closing. Let the caller trample.

        while (isAnyOf(*parser->pos, " \t\n"))
            *parser->pos++ = 0; // skip and trample whitespace

        XMLAttr* attr = XMLAttr_new(name, value);
        listp = jet_PtrList_append(listp, attr);
    }
    //    if (*parser->pos == '/') *parser->pos++ = 0;
    //    assert(*parser->pos == '>');
    //    *parser->pos++ = 0;
    // don't consume ending />, leave it at that
    return list;
}

static List(XMLNode) * XMLParser_parseTags(XMLParser* parser)
{
    List(XMLNode)* list = NULL;
    List(XMLNode)** listp = &list;

    while (parser->pos < parser->end) {
        switch (*parser->pos) {
        case ' ':
        case '\t':
        case '\n':
            parser->pos++;
            break;
        case '<': {
            *parser->pos++ = 0;

            bool noChild = false;

            if (*parser->pos == '/') { // closing tag here means empty content
                parser->pos++; // note that will be past </
                return list; // null if no elements were found at this level
            } else if (not strncasecmp(parser->pos, "[CDATA[", 6)) { // cdata
            } else { // opening tag
                XMLNode* node = XMLNode_new(parser->pos);
                while (not isAnyOf(*parser->pos, " />\n\t"))
                    parser->pos++; // SSE?

                if (*parser->pos == ' ') {
                    *parser->pos++ = 0;
                    while (*parser->pos == ' ') parser->pos++;
                    node->attributes = XMLParser_parseAttrs(parser);
                }
                while (*parser->pos == ' ') parser->pos++;

                switch (*parser->pos) {
                case '/':
                case '?':
                    *parser->pos++ = 0;
                    assert(*parser->pos == '>');
                    noChild = true;
                //  fall through
                case '>':
                    // tag has ended. parse children
                    *parser->pos++ = 0;
                    if (not noChild) {
                        node->children = XMLParser_parseTags(parser);

                        char* closingTag = parser->pos;
                        while (*parser->pos != '>') // SSE?
                            parser->pos++; // seek to end of closing tag
                        *parser->pos++ = 0;

#ifndef FP_XML_SKIP_CLOSING_CHECKS // this is about 10% runtime for a large file
                        if (not *closingTag) {
                            printf("error: found end of file, expected </%s>\n",
                                node->tag);
                            exit(1);
                        } else if (strcasecmp(closingTag, node->tag)) {
                            printf("error: found </%s>, expected </%s>\n",
                                closingTag, node->tag);
                            exit(1);
                        }
#endif
                    }
                    break;
                default:
                    eprintf("oops1: unexpected '%c' (\"%.16s\"...)\n",
                        *parser->pos, parser->pos);
                    break;
                }

                listp = jet_PtrList_append(listp, node);
            }
        } break;

        default:

        {
            char* text = parser->pos;
            // printf("oops2: unexpected '%c' (\"%.16s...\")\n", *parser->pos,
            // parser->pos);
            while (*parser->pos != '<' and parser->pos < parser->end)
                parser->pos++;
            // parser->pos = findchars_fast(parser->pos, parser->end, "<", 1);
            // relying on the </ detector state to trample the <
            XMLNode* textNode = XMLNode_newText(text);
            listp = jet_PtrList_append(listp, textNode);
        }
        }
    }
    if (parser->pos < parser->end) printf("error: data unparsed\n");
    return list;
}

static void XMLAttr_print(XMLAttr* attr, int indent)
{
    printf(" %s=\"%s\"", attr->key, attr->val);
}

static void XMLNode_print(XMLNode* node, int indent);
static void XMLNodeList_print(List(XMLNode) * nodeList, int indent)
{
    jet_foreach(XMLNode*, childNode, nodeList) XMLNode_print(childNode, indent);
}

static const char* const spaces
    = "                                                 ";
static void XMLNode_print(XMLNode* node, int indent)
{
    if (node->tag) {
        printf("%.*s<%s%s", indent, spaces, node->tag,
            node->attributes ? "" : node->children ? ">\n" : "/>\n");
        jet_foreach(XMLAttr*, attr, node->attributes)
            XMLAttr_print(attr, indent);
        if (node->attributes) printf("%s\n", node->children ? ">" : " />");
        XMLNodeList_print(node->children, indent + 2);
        if (node->children) printf("%.*s</%s>\n", indent, spaces, node->tag);
    } else {
        printf("%.*s%s\n", indent, spaces, node->text);
    }
}

int main(int argc, char* argv[])
{
    char* xmlstr = "" //
                   "<xml>" //
                   "  <head>" //
                   "    <title attr=value attr2=\"value2 what>\">" //
                   "    </title>" //
                   "    <meta name=content-type content=utf-8/>" //
                   "  </head>" //
                   "</xml>"; //
    "<foot>" //
    "</foot>";
    //     xmlstr = "<meta name=content-type content=utf-8/><meta name=keywords
    //     content='rail,train,goods'/>"; xmlstr = "<a>"; xmlstr = "<a></a>";

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
    if (argc < 2) {
        printf("usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    jet_sys_time_Time t0; // = jet_sys_time_getTime();
    XMLParser* par = XMLParser_fromFile(argv[1]);
    double tms; // = jet_sys_time_clockSpanMicro(t0) / 1.0e3;
    // eprintf("\e[1mread time:\e[0m %.1f ms (%.2f GB/s)\n", tms,
    //     1 / ((tms / 1e3) * 1e9 / (par->end - par->data))); // sw.print();

    t0 = jet_sys_time_getTime();
    List(XMLNode)* parsed = XMLParser_parseTags(par);
    if (argc > 2 && *argv[2] == 'd') XMLNodeList_print(parsed, 0);

    tms = jet_sys_time_clockSpanMicro(t0) / 1.0e3;

    eputs("-------------------------------------------------------"
          "\n");
    allocstat(XMLAttr);
    allocstat(XMLParser);
    allocstat(XMLNode);
    allocstat(jet_PtrList);
    eputs("-------------------------------------------------------"
          "\n");
    eprintf("*** Total size of nodes                     = %7d B\n",
        jet_gPool->usedTotal);
    eprintf("*** Space allocated for nodes               = %7d B\n",
        jet_gPool->capTotal);
    eprintf("*** Node space utilisation                  = %7.1f %%\n",
        jet_gPool->usedTotal * 100.0 / jet_gPool->capTotal);
    eputs("-------------------------------------------------------"
          "\n");
    eprintf("*** File size                               = %7lu B\n",
        par->end - par->data);
    eprintf("*** Node size to file size ratio            = %7.1f x\n",
        jet_gPool->usedTotal * 1.0 / (par->end - par->data));
    eputs("-------------------------------------------------------"
          "\n");
    eprintf("*** Space used for strings                  = %7u B\n",
        jet_sPool->usedTotal);
    eprintf("*** Allocated for strings                   = %7u B\n",
        jet_sPool->capTotal);
    eprintf("*** Space utilisation                       = %7.1f %%\n",
        jet_sPool->usedTotal * 100.0 / jet_sPool->capTotal);
    eputs("-------------------------------------------------------"
          "\n");
    eputs("\e[1mMemory-related calls\e[0m\n");
    eprintf("  calloc: %-7d | malloc: %-7d | realloc: %-7d\n",
        jet_globals__callocCount, jet_globals__mallocCount,
        jet_globals__reallocCount);
    eprintf("  strlen: %-7d | strdup: %-7d |\n", jet_globals__strlenCount,
        jet_globals__strdupCount);

    eprintf("\e[1mTime elapsed:\e[0m %.1f ms (%.2f GB/s)\n", tms,
        1 / ((tms / 1e3) * 1e9 / (par->end - par->data))); // sw.print();

    return 0;
}
