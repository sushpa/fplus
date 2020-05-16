#include <stdio.h>
#include <string.h>

static const char* const words[] = { "function", "test", "returns", "type",
    "var", "let", "return", "as", "and", "check", "extends", "public",
    "private", "print", "describe", "json", "repr" };
static const size_t nWords = 1; // sizeof(words) / sizeof(words[0]);

static const char* const testWords[]
    = { "westermere", "roxisd", "test", "testa", "John", "function" };
static const size_t nTestWords = sizeof(testWords) / sizeof(testWords[0]);

static int hasWord(const char* const word)
{
    for (int iw = 0; iw < nWords; iw++)
        if (!strcmp(words[iw], word)) return 1;
    return 0;
}

#define THE_FUNCNAME hasWord_fast
#define THE_CODE(x) return 1;
#define THE_ERRHANDLER(x) return 0;
#include "genrec.h"

int main(int argc, char* argv[])
{
    int sum = 0;
    const int NREP = 100000000;
    switch (*argv[1]) {
    case 's':
        for (int i = 0; i < NREP; i++) {
            sum += hasWord(testWords[i % nTestWords]);
        }
        break;
    case 'f':
        for (int i = 0; i < NREP; i++) {
            sum += hasWord_fast(testWords[i % nTestWords]);
        }
        break;
    }
    printf("%d %.1f%%\n", sum, sum * 100.0 / NREP);
}
