#include "common.h"
#include "main/TiXacAnimCurve.h"
#include "main/tu026.h"

/* Returns one float of a key header by its key type: types 0 and 1 read +4, 2 and 3 read +8, 5 reads +C, 4 and types above 5 give 0.0; asserts on a null pointer. */
INCLUDE_ASM("asm/main/nonmatchings/TiXacAnimCurve", xacGetKeyValue);

/* Evaluates a keyframe curve at curveParam: reads keys from *keyCursor, advances it, interpolates through func_00295610; asserts on an illegal curve type. */
INCLUDE_ASM("asm/main/nonmatchings/TiXacAnimCurve", xacEvalCurve);

/* Evaluates the nine channels of a joint transform (pTransJoint) into values[0..8]: each channel is a curve or a stored default, chosen by its flag bit. */
INCLUDE_ASM("asm/main/nonmatchings/TiXacAnimCurve", xacEvalTransJoint);

/* Evaluates nine keyed channels into values[0..8] from the key list at *keyCursor (flag bits 0x1 to 0x100); channel 8 defaults to 1.0; writes the advanced cursor back. */
INCLUDE_ASM("asm/main/nonmatchings/TiXacAnimCurve", xacEvalTransform);

/* Nine-channel transform reader for pTransLocator; same shape as the pTransJoint reader. */
INCLUDE_ASM("asm/main/nonmatchings/TiXacAnimCurve", xacEvalTransLocator);

/* Nine-channel transform reader for pTransFace; same shape as the pTransJoint reader. */
INCLUDE_ASM("asm/main/nonmatchings/TiXacAnimCurve", xacEvalTransFace);

/* Nine-channel transform reader for pTransDynamics (asserts at lines 555/556); its arguments come in another register order than func_002945C8. */
INCLUDE_ASM("asm/main/nonmatchings/TiXacAnimCurve", xacEvalTransDynamics);

/* Second pTransDynamics reader (asserts at lines 625/626); see xacEvalTransDynamics for the difference. */
INCLUDE_ASM("asm/main/nonmatchings/TiXacAnimCurve", func_002945C8);

#include "shared.h"

f32 xacEvalCurve(u8 **keyCursor, f32 curveParam);
extern char xacAssertFile[];
extern char D_0068F6D8[];
extern char D_0068F720[];

/* Reads six curve values from a joint header, starting 4 bytes in, and advances *ppJointHeader. */
u8 *xacEvalJointPose(f32 *values, u8 **ppJointHeader, f32 curveParam) {
    u8 *keyCursor;

    if (values == 0) {
        assertFailed(D_0068F6D8, xacAssertFile, 696);
    }
    if (ppJointHeader == 0) {
        assertFailed(D_0068F720, xacAssertFile, 697);
    }
    keyCursor = *ppJointHeader + 4;
    values[0] = xacEvalCurve(&keyCursor, curveParam);
    values[1] = xacEvalCurve(&keyCursor, curveParam);
    values[2] = xacEvalCurve(&keyCursor, curveParam);
    values[3] = xacEvalCurve(&keyCursor, curveParam);
    values[4] = xacEvalCurve(&keyCursor, curveParam);
    values[5] = xacEvalCurve(&keyCursor, curveParam);
    *ppJointHeader = keyCursor;
    return keyCursor;
}

/* Reads one curve value from a joint header, starting 4 bytes in, and advances *ppJointHeader. */
u8 *xacEvalJointValue(f32 *values, u8 **ppJointHeader, f32 curveParam) {
    u8 *keyCursor;

    if (values == 0) {
        assertFailed(D_0068F6D8, xacAssertFile, 722);
    }
    if (ppJointHeader == 0) {
        assertFailed(D_0068F720, xacAssertFile, 723);
    }
    keyCursor = *ppJointHeader + 4;
    values[0] = xacEvalCurve(&keyCursor, curveParam);
    *ppJointHeader = keyCursor;
    return keyCursor;
}

/* Reads nine curve values from a joint header, starting 4 bytes in, and advances *ppJointHeader. */
u8 *xacReadJointCurves9(f32 *values, u8 **ppJointHeader, f32 curveParam) {
    u8 *keyCursor;

    if (values == 0) {
        assertFailed(D_0068F6D8, xacAssertFile, 741);
    }
    if (ppJointHeader == 0) {
        assertFailed(D_0068F720, xacAssertFile, 742);
    }
    keyCursor = *ppJointHeader + 4;
    values[0] = xacEvalCurve(&keyCursor, curveParam);
    values[1] = xacEvalCurve(&keyCursor, curveParam);
    values[2] = xacEvalCurve(&keyCursor, curveParam);
    values[3] = xacEvalCurve(&keyCursor, curveParam);
    values[4] = xacEvalCurve(&keyCursor, curveParam);
    values[5] = xacEvalCurve(&keyCursor, curveParam);
    values[6] = xacEvalCurve(&keyCursor, curveParam);
    values[7] = xacEvalCurve(&keyCursor, curveParam);
    values[8] = xacEvalCurve(&keyCursor, curveParam);
    *ppJointHeader = keyCursor;
    return keyCursor;
}

/* Reads nine curve values from a joint header (pJointHeader, assert lines 772/773). */
INCLUDE_ASM("asm/main/nonmatchings/TiXacAnimCurve", func_00294AE0);

/* Reads twelve curve values from a joint header (ppJointHeader, assert lines 801/802). */
INCLUDE_ASM("asm/main/nonmatchings/TiXacAnimCurve", func_00294BF0);

/* Reads twelve curve values from a joint header (ppJointHeader, assert lines 840/841). */
INCLUDE_ASM("asm/main/nonmatchings/TiXacAnimCurve", func_00294D50);

/* Reads twelve curve values from a joint header (ppJointHeader, assert lines 879/880). */
INCLUDE_ASM("asm/main/nonmatchings/TiXacAnimCurve", func_00294EB0);

/* Reads fifteen curve values from a joint header, starting 4 bytes in, and advances *ppJointHeader. */
u8 *xacReadJointCurves15(f32 *values, u8 **ppJointHeader, f32 curveParam) {
    u8 *keyCursor;

    if (values == 0) {
        assertFailed(D_0068F6D8, xacAssertFile, 924);
    }
    if (ppJointHeader == 0) {
        assertFailed(D_0068F720, xacAssertFile, 925);
    }
    keyCursor = *ppJointHeader + 4;
    values[0] = xacEvalCurve(&keyCursor, curveParam);
    values[1] = xacEvalCurve(&keyCursor, curveParam);
    values[2] = xacEvalCurve(&keyCursor, curveParam);
    values[3] = xacEvalCurve(&keyCursor, curveParam);
    values[4] = xacEvalCurve(&keyCursor, curveParam);
    values[5] = xacEvalCurve(&keyCursor, curveParam);
    values[6] = xacEvalCurve(&keyCursor, curveParam);
    values[7] = xacEvalCurve(&keyCursor, curveParam);
    values[8] = xacEvalCurve(&keyCursor, curveParam);
    values[9] = xacEvalCurve(&keyCursor, curveParam);
    values[10] = xacEvalCurve(&keyCursor, curveParam);
    values[11] = xacEvalCurve(&keyCursor, curveParam);
    values[12] = xacEvalCurve(&keyCursor, curveParam);
    values[13] = xacEvalCurve(&keyCursor, curveParam);
    values[14] = xacEvalCurve(&keyCursor, curveParam);
    *ppJointHeader = keyCursor;
    return keyCursor;
}

/* Scans a key table (count word, then key times) for the key segment around curveParam; writes a 0/1 flag to its first argument and returns the key pointer. */
INCLUDE_ASM("asm/main/nonmatchings/TiXacAnimCurve", xacFindCurveKey);
