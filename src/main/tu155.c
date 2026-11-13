#include "common.h"
#include "shared.h"
#include "main/sef.h"
#include "main/tu155.h"


/* Converts a direction into pitch (angles->x) and yaw (angles->y). */
void vu0DirectionToAngles(Vector4 *angles, Vector4 *direction, int unusedArg2, int unusedArg3) {
    float negatedY;
    float horizontalLength;
    float pitch;
    float yaw;

    negatedY = -direction->y;
    __asm__ __volatile__("lqc2 vf1, 0(%0)" : : "r"(direction) : "memory");
    __asm__ __volatile__("vmul.xz vf1, vf1, vf1" : : : "memory");
    __asm__ __volatile__("vaddz.x vf1, vf1, vf1z" : : : "memory");
    __asm__ __volatile__("vsqrt Q, vf1x" : : : "memory");
    __asm__ __volatile__("sqc2 vf0, 0(%0)" : : "r"(angles) : "memory");
    __asm__ __volatile__("vwaitq" : : : "memory");
    __asm__ __volatile__("vaddq.x vf1, vf0, Q" : : : "memory");
    __asm__ __volatile__("qmfc2.ni $8, vf1\n\tmtc1 $8, %0" : "=f"(horizontalLength) : : "$8", "memory");
    pitch = srsAtan2(negatedY, horizontalLength);
    angles->x = pitch;
    yaw = srsAtan2(direction->x, direction->z);
    angles->y = yaw;
}

/* Global flag word; sv_get_image_item_161 also reads it with masks. */
extern u32 svSceneFlags;

u32 func_00252E00(u32 mask) {
    return svSceneFlags & mask;
}

void func_00252E10(u32 mask) {
    svSceneFlags |= mask;
}

void func_00252E20(u32 mask) {
    svSceneFlags &= ~mask;
}

void func_00252E38(u32 mask) {
    svSceneFlags ^= mask;
}

/* The light set lightSetLogDump walks: directional lights, colours, shadow and point-light vectors. */
typedef struct LightSet {
    Vector4 direction[7];       /* +0x000 */
    Vector4 color[8];           /* +0x070 */
    Vector4 shadow[3];          /* +0x0F0 */
    Vector4 pointPosition[3];   /* +0x120 */
    Vector4 pointColor[3];      /* +0x150 */
} LightSet;

/* "sDir", "sCol", "sSha", "sPntP", "sPntC" */
extern const char lightLogDirectionLabel[];
extern const char lightLogColorLabel[];
extern const char lightLogShadowLabel[];
extern const char lightLogPointPositionLabel[];
extern const char lightLogPointColorLabel[];
void logNoOp(const void *data, const char *format, ...);

/* Logs every vector of the light set with its label and index (logNoOp does nothing in this build). */
void lightSetLogDump(LightSet *lights)
{
    int i;

    for (i = 0; i < 7; i++) {
        logNoOp(&lights->direction[i], lightLogDirectionLabel, i);
    }
    for (i = 0; i < 8; i++) {
        logNoOp(&lights->color[i], lightLogColorLabel, i);
    }
    for (i = 0; i < 3; i++) {
        logNoOp(&lights->shadow[i], lightLogShadowLabel, i);
    }
    for (i = 0; i < 3; i++) {
        logNoOp(&lights->pointPosition[i], lightLogPointPositionLabel, i);
    }
    for (i = 0; i < 3; i++) {
        logNoOp(&lights->pointColor[i], lightLogPointColorLabel, i);
    }
}
