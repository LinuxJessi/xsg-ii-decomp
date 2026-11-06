/*
 * OV01 original TU 27: 0x00a8f100..0x00a8f658 (12 functions)
 */
#include "common.h"

INCLUDE_ASM("asm/nonmatchings/ov01/tu027", slotRecordGet);

/*
 * Slot records: 0x40-byte records in slotRecordTable. Record k (k = slot index 0..15)
 * starts at D_61C3C0 + k * 0x40, so slotRecordTable is record 1. slotRecordGet takes
 * the slot index from the owner's work block (+0x62). The bit fields below are
 * read by the tu023 wrappers and by tu030; the bit numbers passed in are 1-based.
 *   +0x00 word total (capped at 99999999), +0x04 word counter, +0x08 and +0x0C
 *   words (capped at 99999), +0x20 code bits 1..59, +0x2C flag bits 1..104,
 *   +0x3C pair bits 1..16, +0x3E mark bits 1..21.
 */
extern unsigned char slotRecordTable[];

/* Record for the slot index given, or 0 when the index is 16 or more. */
void *slotRecordByIndex(int index) {
    if (index >= 0x10) {
        return 0;
    }
    return (void *) ((index << 6) + (slotRecordTable - 0x40));
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu027", slotFlagSet);

INCLUDE_ASM("asm/nonmatchings/ov01/tu027", slotFlagTest);

INCLUDE_ASM("asm/nonmatchings/ov01/tu027", slotFlagClear);

INCLUDE_ASM("asm/nonmatchings/ov01/tu027", slotCodeSet);

INCLUDE_ASM("asm/nonmatchings/ov01/tu027", slotCodeTest);

INCLUDE_ASM("asm/nonmatchings/ov01/tu027", func_00A8F3C0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu027", func_00A8F478);

INCLUDE_ASM("asm/nonmatchings/ov01/tu027", func_00A8F4E8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu027", func_00A8F560);

INCLUDE_ASM("asm/nonmatchings/ov01/tu027", func_00A8F5E0);
