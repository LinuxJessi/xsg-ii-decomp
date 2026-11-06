#include "common.h"
#include "shared.h"
#include "main/sef_169.h"
#include "main/tu172.h"

extern unsigned char sefEffectSlots[];

/* Clears the whole effect slot table sefEffectSlots: 32 records of 0x40 bytes. */
void sefResetSlotTable(void) {
    sefMemZero(sefEffectSlots, 0x800);
}

INCLUDE_ASM("asm/main/nonmatchings/tu172", sefSpawnEffect);

INCLUDE_ASM("asm/main/nonmatchings/tu172", sefUpdateEffectSlots);

void sefSpawnEffect(void);

/* Forwards to sefSpawnEffect with the caller's arguments unchanged (tail call in the binary). */
void sefSpawnEffectById(void) {
    sefSpawnEffect();
}

INCLUDE_ASM("asm/main/nonmatchings/tu172", sefSpawnEffectByName);
