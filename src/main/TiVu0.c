#include "common.h"
#include "main/tu026.h"

extern char D_0068F5F0[];
extern char tiVu0AssertFile[];

/* Unconditional assert(0) at TiVu0.euc.c line 739: a fail-only path with no role shown. */
void func_002933D0(void)
{
    assertFailed(D_0068F5F0, tiVu0AssertFile, 739);
}

/* Unconditional assert(0) at TiVu0.euc.c line 758: a fail-only path with no role shown. */
void func_002933F8(void)
{
    assertFailed(D_0068F5F0, tiVu0AssertFile, 758);
}

#include "shared.h"
#include "main/TiXacAnimCurve.h"

extern char D_0068F610[];
extern char xacAssertFile[];

extern f32 xacKeyU16ScaleA;

/* Reads one curve key's value by key type: 0 gives 0.0, 2 a float at +4, 3 a u16 at +6 times 1e-4, 5 a float at +8; other types assert.
 * The assert file names TiXacAnimCurve.euc.c, so this reader belongs to that family. */
f32 xacGetKeyPayloadA(TiXacKey *key) {
    int keyType;

    keyType = key->typeFlags & XAC_KEY_TYPE_MASK;
    switch (keyType) {
    case 0:
        return 0.0f;
    case 3:
        return (f32) key->payload.halfwords[1] * xacKeyU16ScaleA;
    case 5:
        return key->payload.value[1];
    case 2:
        return key->payload.value[0];
    default:
        assertFailed(D_0068F610, xacAssertFile, 32);
        return 0.0f;
    }
}

extern f32 xacKeyU16ScaleB;

/* Same type switch as xacGetKeyPayloadA with another key layout: 3 reads a u16 at +4 times 1e-4, 2 and 5 read a float at +4. */
f32 xacGetKeyPayloadB(TiXacKey *key) {
    int keyType;

    keyType = key->typeFlags & XAC_KEY_TYPE_MASK;
    switch (keyType) {
    case 0:
        return 0.0f;
    case 3:
        return (f32) key->payload.halfwords[0] * xacKeyU16ScaleB;
    case 5:
        return key->payload.value[0];
    case 2:
        return key->payload.value[0];
    default:
        assertFailed(D_0068F610, xacAssertFile, 67);
        return 0.0f;
    }
}
