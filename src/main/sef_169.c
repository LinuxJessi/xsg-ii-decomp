#include "common.h"
#include "shared.h"
#include "main/sef_169.h"

/* A 128-bit quadword (one lq/sq), as the VIF1 packet writers take it. */
typedef unsigned int Quadword __attribute__((mode(TI)));

typedef union SefQuadBlock {
    Quadword quad;
    unsigned int words[4];
} SefQuadBlock;

/* Stores value into each of the size / 4 words starting at dest. */
void sefMemFillWords(int *dest, int value, int size) {
    int *word;
    int count;

    word = dest;
    count = size >> 2;
    if (count > 0) {
        do {
            count -= 1;
            *word = value;
            word += 1;
        } while (count != 0);
    }
}

void sefMemZero(void *dest, int size)
{
    SefQuadBlock *blocks = dest;
    int block_count = size >> 4;
    int word_count = (size & 0xf) >> 2;
    SefQuadBlock *tailBlock = blocks + block_count;
    int i;

    for (i = 0; i < block_count; i++) {
        blocks[i].quad = 0;
    }
    for (i = 0; i < word_count; i++) {
        tailBlock->words[i] = 0;
    }
}

extern void *memset(void *destination, int value, unsigned int count);

/* Clears size bytes at dest with memset. */
void sefMemClear(void *dest, unsigned int size) {
    memset(dest, 0, size);
}
