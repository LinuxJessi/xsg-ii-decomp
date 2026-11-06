#include "common.h"
#define SEF_GROUP_ENTRY_COUNT 8

/*
 * One group timer, 0x20 bytes, eight of them in sefGroupTable. sefRegisterGroupCenter fills it for a tracked group
 * (the centre of the group's objects, the owner id and the key that name the group) with 600 frames on the timer;
 * sefTickGroupTimers counts `remaining` down; sefGetGroupCenter reads the centre back by owner and key.
 */
typedef struct SefGroupEntry {
    float center[4];               /* +0x00: x, y, z, w of the group centre */
    int owner;                     /* +0x10: owner id compared by sefGetGroupCenter and sefRegisterGroupCenter */
    short remaining;               /* +0x14: frames left; zero or less means the entry is free */
    unsigned short key;            /* +0x16: second id of the group, compared as an unsigned short */
    unsigned char unmodeled_18[8]; /* +0x18..0x1F: not read by the code in this unit */
} SefGroupEntry;

extern SefGroupEntry sefGroupTable[SEF_GROUP_ENTRY_COUNT];

/* Counts every group timer down by one frame, stopping at zero. */
void sefTickGroupTimers(void)
{
    SefGroupEntry *entry;
    int index;

    entry = sefGroupTable;
    for (index = SEF_GROUP_ENTRY_COUNT - 1; index >= 0; index--) {
        if (entry->remaining > 0) {
            entry->remaining = entry->remaining - 1;
        }
        entry++;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu171", sefGetGroupCenter);

INCLUDE_ASM("asm/main/nonmatchings/tu171", sefRegisterGroupCenter);
