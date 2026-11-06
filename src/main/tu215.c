#include "common.h"
#include "main/tu229.h"
#include "main/tu245.h"

/* msgWindowRunSlots: runs the window's five 0x98-byte slots (from +0x08); an active slot goes to msgSlotRun. */
INCLUDE_ASM("asm/main/nonmatchings/tu215", msgWindowRunSlots);

/* msgSlotRun: runs one slot's handler pair, chosen by the slot's type word at +0x1C (1 to 4). */
INCLUDE_ASM("asm/main/nonmatchings/tu215", msgSlotRun);


void talkEnd(void) {
    msgWindowUnloadBotanXtx();
    talkEndScript();
}
