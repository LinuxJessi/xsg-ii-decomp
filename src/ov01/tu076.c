/*
 * OV01 original TU 76: 0x00aab010..0x00aab888 (6 functions)
 */
#include "common.h"
#include "shared.h"

void *memset(void *, int, unsigned int);

/* Clears the 0xB0-byte record, then stores value in its first word. */
void ov01RecordInit(int *record, int value)
{
    memset(record, 0, 0xB0);
    *record = value;
}

/* Key choice of MMathCalcHermiteKey, tail-calling MMathCalcHermitePrm (proposed MMathCalcHermiteKeyPrm). */
INCLUDE_ASM("asm/nonmatchings/ov01/tu076", MMathCalcHermiteKeyPrm);

/*
 * Argument order as this TU passes it: the parameter comes last.  The
 * definition in main/tu188 takes the parameter second, so the two
 * declarations do not agree.
 */
void MMathCalcHermite(float *, void *, void *, void *, void *, f32);

/* Hermite sample at key record `index` of the table at base (proposed MMathCalcHermiteKey). */
void MMathCalcHermiteKey(float *destination, char *base, int index, f32 parameter)
{
    char *record;

    record = base + (index * 0x10);
    MMathCalcHermite(destination, base + 0x90, base + 0xA0, record + 0x10, record + 0x20, parameter);
}

/* GS packet builders on an MGsPacket (MEf family); roles not fixed, left unnamed. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu076", func_00AAB0E0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu076", func_00AAB368);

INCLUDE_ASM("asm/nonmatchings/ov01/tu076", func_00AAB5C8);
