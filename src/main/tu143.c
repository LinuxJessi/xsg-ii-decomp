#include "common.h"

int func_0024F418(int, int);
int func_0024F480(int, int);

/* Walks the text line by line from `start` and returns the first line the pattern matches (func_0024F480 is 0 on a match). */
int func_0024F3B8(int start, int pattern)
{
    int position;

    position = start;
    while (func_0024F480(pattern, position) != 0) {
        position = func_0024F418(position, 1);
    }
    return position;
}

/* Moves past the next newline, then past spaces and tabs; returns the new cursor. */
INCLUDE_ASM("asm/main/nonmatchings/tu143", func_0024F418);

/* Compares the text at a1 with the pattern at a0 up to a newline or a '%' wildcard: 0 on a match, nonzero otherwise. */
INCLUDE_ASM("asm/main/nonmatchings/tu143", func_0024F480);
