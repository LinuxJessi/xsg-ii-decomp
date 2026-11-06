#include "common.h"
#include "main/tu116.h"

/*
 * Core of the quest-flag setter (script native setQuestFlag, setQuestFlag):
 * stores value in the 36-entry byte table questFlags; values 2 and 3 also get
 * bit 0x10, which questFlagGet masks off again.
 */
INCLUDE_ASM("asm/main/nonmatchings/tu116", questFlagSet);

#include "shared.h"

extern u8 citizenFlags[];

/* Core of the citizen-flag setter (script native setCitizenFlag, setCitizenFlag): 50-entry byte table. */
void citizenFlagSet(u32 index, int value) {
    if (index < 0x32U) {
        citizenFlags[index] = (u8) value;
    }
}

extern u8 questFlags[];

/* Core of the quest-flag getter (script native getQuestFlag, getQuestFlag): 0 for an index outside the 36-entry table. */
int questFlagGet(u32 index) {
    if (!(index < 0x24U)) {
        return 0;
    }
    return questFlags[index] & 0xEF;
}
