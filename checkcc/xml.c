#include "fp_base.h"

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
    char* data,*end;
    char* pos;
 };

MKSTAT(XMLNode)
MKSTAT(XMLAttr)
MKSTAT(XMLParser)

XMLNode* XMLNode_new(const char* tag)
{
    XMLNode* ret = fp_new(XMLNode);
    ret->tag = tag;
    return ret;
}

XMLAttr* XMLAttr_new(const char* key, const char* value)
{
    XMLAttr* ret = fp_new(XMLAttr);
    ret->key = key;
    ret->val = value;
    return ret;
}

XMLParser* XMLParser_new(const char* str)
{
    XMLParser* par = fp_new(XMLParser);
    size_t len = strlen(str);
    par->data = pstrndup(str,len);
    par->pos = par->data;
    par->end=par->data+len;
    return par;
}

bool isAnyOf(char ch, char* chars)
{
    while (*chars)
        if (*chars++ == ch) return true;
    return false;
}

List(XMLAttr) * XMLParser_parseAttrs(XMLParser* parser)
{
    List(XMLAttr)* list = NULL;
    List(XMLAttr)** listp = &list;

    while (*parser->pos and *parser->pos != '>' and *parser->pos != '/') {
        const char* name = parser->pos;
        while (*parser->pos != '=') parser->pos++;
        *parser->pos++ = 0; // trample =

        char* markers;
        if (isAnyOf(*parser->pos, "'\"")) {
            markers = (*parser->pos == '"') ? "\"" : "'";
            parser->pos++;
        } else {
            markers = " >/\t\n";
        }

        const char* value = parser->pos;

        while (not isAnyOf(*parser->pos, markers)) parser->pos++;
        if (not isAnyOf(*parser->pos, "/>")) *parser->pos++ = 0; // trample the marker if " or ' or spaces
        // ^ DON't trample the markers / or > here, because in the case of e.g.
        // <meta name=content-type content=utf8/>
        // it will trample the / not allowing the calling parseTags to
        // detect that the tag is self-closing. Let the caller trample.

        while (isAnyOf(*parser->pos, " \t\n"))
            *parser->pos++=0; // skip and trample whitespace

        XMLAttr* attr = XMLAttr_new(name, value);
        listp = fp_PtrList_append(listp, attr);
    }
//    if (*parser->pos == '/') *parser->pos++ = 0;
//    assert(*parser->pos == '>');
//    *parser->pos++ = 0;
// don't consume ending />, leave it at that
    return list;
}

List(XMLNode) * XMLParser_parseTags(XMLParser* parser)
{
    List(XMLNode)* list = NULL;
    List(XMLNode)** listp = &list;

    while (parser->pos<parser->end) {
        switch (*parser->pos) {
        case ' ':
        case '\t':
        case '\n':
            parser->pos++;
            break;
        case '<': {
            parser->pos++;

            bool noChild = false;

            if (*parser->pos == '/') { // closing tag here means empty content
                parser->pos++; // note that will be past </
                return list; // null if no elements were found at this level
            } else if (not strncasecmp(parser->pos, "[CDATA[", 6)) { // cdata
            } else { // opening tag
                XMLNode* node = XMLNode_new(parser->pos);
                while (not isAnyOf(*parser->pos, " />\n\t")) parser->pos++;

                if (*parser->pos== ' ') {
                    *parser->pos++ = 0;
                    while(*parser->pos==' ') parser->pos++;
                    node->attributes = XMLParser_parseAttrs(parser);
                }
                while(*parser->pos==' ') parser->pos++;

                switch (*parser->pos) {
                case '/':
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
                        while (*parser->pos != '>')
                            parser->pos++; // seek to end of closing tag
                        *parser->pos++ = 0;

                        if (not *closingTag) {
                            printf("error: found end of file, expected </%s>\n",
                                     node->tag);
                            exit(1);
                        }
                        else if (strcasecmp(closingTag, node->tag)) {
                            printf("error: found </%s>, expected </%s>\n",
                                closingTag, node->tag);
                            exit(1);
                        }
                    }
                    break;
                default:
                    eprintf("oops1: unexpected '%c' (\"%.16s\"...)\n",*parser->pos,parser->pos);
                    break;
                }

                listp = fp_PtrList_append(listp, node);
            }
        }
        break;

        default:
            printf("oops2: unexpected '%c' (\"%.16s...\")\n", *parser->pos, parser->pos);
            parser->pos++;
        }
    }
    if (parser->pos < parser->end) printf("error: data unparsed\n");
    return list;
}

void XMLAttr_print(XMLAttr* attr, int indent)
{
    printf(" %s=\"%s\"", attr->key, attr->val);
}

void XMLNode_print(XMLNode* node, int indent);
void XMLNodeList_print(List(XMLNode) * nodeList, int indent)
{
    fp_foreach(XMLNode*, childNode, nodeList) XMLNode_print(childNode, indent);
}

const char* const spaces = "                                                 ";
void XMLNode_print(XMLNode* node, int indent)
{
    printf("%.*s<%s%s", indent, spaces, node->tag,
        node->attributes ? "" : node->children ? ">\n" : "/>\n");
    fp_foreach(XMLAttr*, attr, node->attributes) XMLAttr_print(attr, indent);
    if (node->attributes) printf("%s\n", node->children ? ">" : " />");
    XMLNodeList_print(node->children, indent + 2);
    if (node->children) printf("%.*s</%s>\n", indent, spaces, node->tag);
}

int main()
{
    char* xmlstr = "" //
                   "<xml>" //
                   "  <head>" //
                   "    <title attr=value attr2=\"value2 what>\">" //
                   "    </title>" //
                   "    <meta name=content-type content=utf-8/>" //
                   "  </head>" //
    "</xml>" ;//
                   "<foot>" //
                   "</foot>";
//     xmlstr = "<meta name=content-type content=utf-8/><meta name=keywords content='rail,train,goods'/>";
     xmlstr = "<a>";
//     xmlstr = "<a></a>";
    XMLParser* par = XMLParser_new(xmlstr);
    List(XMLNode)* parsed = XMLParser_parseTags(par);
    XMLNodeList_print(parsed, 0);
    return 0;
}
