#include "common.h"

/* Partial layout: only the fields these three functions touch are modeled. */
struct SlotBlock {
    int flags;
    unsigned char unmodeled_04[0x18];
    int state;
    unsigned char unmodeled_20[0x40];
    int slots[11];
};

void slotBlockSetSlot(struct SlotBlock *block, int index, int value) {
    if (block->state == 1 && (unsigned int) (index - 1) < 10) {
        block->slots[index] = value;
    }
}

void slotBlockSetFirstSlot(struct SlotBlock *block, int value) {
    if (block->state == 1) {
        block->slots[0] = value;
    }
}

void slotBlockSetFlag04(struct SlotBlock *block) {
    if (block->state == 1) {
        block->flags = (block->flags & ~2) | 4;
    }
}
