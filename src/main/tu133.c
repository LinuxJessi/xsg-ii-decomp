#include "common.h"

/* Swaps the two words the pointers address (the water sim swaps its height buffers this way). */
void tiWatSimSwapWords(int *first, int *second) {
    int firstValue;

    firstValue = *first;
    *first = *second;
    *second = firstValue;
}

INCLUDE_ASM("asm/main/nonmatchings/tu133", tiWatSimAdvance);

void tiWatSimAdvance(void);

/* Runs one water simulation step (tiWatSimAdvance). */
void tiWatSimUpdate(void) {
    tiWatSimAdvance();
}
