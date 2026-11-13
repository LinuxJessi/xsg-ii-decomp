#include "common.h"

char *func_0024F418(char *text, int lines);
int func_0024F480(const char *pattern, const char *text);

/* Walks the text line by line from `start` and returns the first line the pattern matches (func_0024F480 is 0 on a match). */
char *func_0024F3B8(char *start, const char *pattern)
{
    char *position;

    position = start;
    while (func_0024F480(pattern, position) != 0) {
        position = func_0024F418(position, 1);
    }
    return position;
}

/* Moves past the next newline, then past spaces and tabs; returns the new cursor. */
char *func_0024F418(char *text, int lines)
{
    while (*text != '\n') {
        text++;
    }
    text++;
    while (*text == ' ' || *text == '\t') {
        text++;
    }
    return text;
}

/* Compares the text with the pattern up to the pattern's newline; a '%' in the pattern ends the
 * comparison as a match. 0 on a match, nonzero otherwise. */
int func_0024F480(const char *pattern, const char *text)
{
    int c;

    for (; *pattern != '\n'; pattern++) {
        c = *pattern;
        if (c != *text++) {
            return *pattern != '%';
        }
    }
    return *pattern != *text;
}
