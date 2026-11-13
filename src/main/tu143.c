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

/* Compares the text at a1 with the pattern at a0 up to a newline or a '%' wildcard: 0 on a match, nonzero otherwise. */
INCLUDE_ASM("asm/main/nonmatchings/tu143", func_0024F480);
