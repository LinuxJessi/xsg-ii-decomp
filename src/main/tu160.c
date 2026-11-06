#include "common.h"
#include "main/sv_get_image_item_159.h"

INCLUDE_ASM("asm/main/nonmatchings/tu160", srsFindFreeEffectSlot);

INCLUDE_ASM("asm/main/nonmatchings/tu160", srsFreeEffectSlot);

int srsFreeEffectSlot(unsigned int index);

/* Unloads effect slot `index`: deletes its image list (index + 9), then frees the slot. */
void srsUnloadEffectSlot(unsigned int index)
{
    if (index < 0x30U) {
        svDeleteImageMapper(index + 9);
        srsFreeEffectSlot(index);
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu160", srsEvictEffectSlot);

INCLUDE_ASM("asm/main/nonmatchings/tu160", srsEvictEffectSlotsForSize);

INCLUDE_ASM("asm/main/nonmatchings/tu160", srsIsEffectKeyListed);

INCLUDE_ASM("asm/main/nonmatchings/tu160", srsReleaseOtherOwnerEffectSlots);

INCLUDE_ASM("asm/main/nonmatchings/tu160", srsLoadEffectSlot);
