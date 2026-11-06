#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tu259", func_00295260);

#include "shared.h"

f32 func_00293420(int);
f32 xacGetKeyValue(void);

/* Reads two key values of one key header: the first through xacGetKeyValue, the second through func_00293420. */
void xacGetKeyPair(int keyHeader, f32 *first, f32 *second)
{
    *first = xacGetKeyValue();
    *second = func_00293420(keyHeader);
}

f32 func_002934C0(int);

void func_00295438(int keyHeader, f32 *first, f32 *second)
{
    *first = xacGetKeyValue();
    *second = func_002934C0(keyHeader);
}

INCLUDE_ASM("asm/main/nonmatchings/tu259", func_00295488);

/* One curve key. origin is the key's start time; the time scale tiXacAnimCurveTimeScale converts (time - origin). */
typedef struct CurveRecord {
    unsigned char unmodeled_00[2];
    short origin;
} CurveRecord;

extern f32 tiXacAnimCurveTimeScale;

/* Cubic segment of one key: coeffs[0] is the leading term, coeffs[3] the constant (Horner form). */
f32 xacEvalCubicSegment(const CurveRecord *record, const f32 *coeffs, f32 value)
{
    f32 scaled;

    scaled = (value - (f32) record->origin) * tiXacAnimCurveTimeScale;
    return scaled * (scaled * (scaled * coeffs[0] + coeffs[1]) + coeffs[2]) + coeffs[3];
}

INCLUDE_ASM("asm/main/nonmatchings/tu259", func_00295610);
