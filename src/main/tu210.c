#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tu210", orbitDrawRing);

#include "shared.h"

typedef struct IntegerRecord {
    u8 unmodeled_00[0x10];
    int value;
} IntegerRecord;

/* Stores a point: x and y from the arguments, z from the record value; returns that value. */
int setPointFromRecord(Vector4 *destination, int x, int y, const IntegerRecord *record) {
    int value;

    value = record->value;
    destination->x = (float) x;
    destination->y = (float) y;
    destination->z = (float) value;
    return value;
}

INCLUDE_ASM("asm/main/nonmatchings/tu210", func_002783D0);

INCLUDE_ASM("asm/main/nonmatchings/tu210", func_00278558);

INCLUDE_ASM("asm/main/nonmatchings/tu210", circleDrawSegments);

INCLUDE_ASM("asm/main/nonmatchings/tu210", func_002789C8);

INCLUDE_ASM("asm/main/nonmatchings/tu210", gridDrawCells);
