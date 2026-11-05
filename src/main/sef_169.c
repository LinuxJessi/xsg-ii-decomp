#include "common.h"
#include "shared.h"

/* A 128-bit quadword (one lq/sq), as the VIF1 packet writers take it. */
typedef unsigned int Quadword __attribute__((mode(TI)));

typedef union SefQuadBlock {
    Quadword quad;
    unsigned int words[4];
} SefQuadBlock;

INCLUDE_ASM("asm/main/nonmatchings/sef_169", func_00261890);

void sefMemZero(void *dest, int size)
{
    SefQuadBlock *blocks = dest;
    int block_count = size >> 4;
    int word_count = (size & 0xf) >> 2;
    SefQuadBlock *tail = blocks + block_count;
    int i;

    for (i = 0; i < block_count; i++) {
        blocks[i].quad = 0;
    }
    for (i = 0; i < word_count; i++) {
        tail->words[i] = 0;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/sef_169", func_00261940);
