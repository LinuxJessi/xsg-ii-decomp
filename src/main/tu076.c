#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tu076", ladderInitTable);

INCLUDE_ASM("asm/main/nonmatchings/tu076", ladderFindSlot);

#include "shared.h"

/* One 208-byte record of the 16-slot ladder table at ladderSlots; its first halfword is -1 when the slot is unused. */
typedef struct LadderSlot {
    short id;
    unsigned char unmodeled_02[0xce];
} LadderSlot;

#define LADDER_SLOT_COUNT 16

extern LadderSlot ladderSlots[LADDER_SLOT_COUNT];

/* Returns how many slots, counted from slot 0, are in use before the first unused one. */
int ladderCountUsedPrefix(void)
{
    int count;
    LadderSlot *slot;

    count = 0;
    if (ladderSlots[0].id != -1) {
        slot = ladderSlots;
        for (;;) {
            count += 1;
            slot += 1;
            if (count < LADDER_SLOT_COUNT && slot->id != -1) {
                continue;
            }
            break;
        }
    }
    return count;
}

INCLUDE_ASM("asm/main/nonmatchings/tu076", ladder_001F9A68);

INCLUDE_ASM("asm/main/nonmatchings/tu076", ladder_001F9B90);
