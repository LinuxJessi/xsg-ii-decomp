#include "common.h"
#include "shared.h"
#include "main/tu168.h"
#include "main/tu170.h"
#include "main/tu187.h"

#include "quadword.h"
#include "vu0_macro.h"
/* Constants of the 32-bit LCG in the X1 port; the X2 code (sefRandf and its callers) steps the seed with 0xF82D and
 * adds 0x303B, so these three macros are not the X2 generator. They are unused in this unit. */
#define SEF_RAND_MULTIPLIER 0x41C64E6Du
#define SEF_RAND_INCREMENT 0x3039u
#define SEF_RAND_MASK 0x7FFF

void sefGetCubePosBtm(Vector4 *pos, const int *extent);
/* LCG state of sefRandf, stored at sefRandSeed. */
extern unsigned int sefRandSeed;
void MMathRandomSeed(int initArg);
/* Set to 1 once sefNoiseInit has filled the noise table. */
extern int sefNoiseReady;

/* 1024 uniform noise samples in [-0.5, 0.5) with a read index. sefNoiseNextVector reads three samples at a time and
 * can read the three floats after the 1024th, which sefNoiseInit also fills (1027 samples in all). */
typedef struct SefNoiseTable {
    f32 samples[1024];
    unsigned char unmodeled_1000[12];
    int nextIndex;
} SefNoiseTable;

extern SefNoiseTable sefNoiseTable;

/* Fills the noise table once with uniform samples in [-0.5, 0.5) and resets its read index to zero. */
void sefNoiseInit(void)
{
    f32 sample;
    f32 *cursor;
    int *indexSlot;
    int i;

    if (sefNoiseReady == 0) {
        sefNoiseReady = 1;
        MMathRandomSeed(0);
        i = 0x100C;
        indexSlot = (int *)((unsigned char *)&sefNoiseTable + i);
        *indexSlot = 0;
        cursor = (f32 *)&sefNoiseTable;
        for (i = 0; i < 1027; i++) {
            sample = MMathMakeRandom() - 0.5f;
            *cursor = sample;
            cursor++;
        }
    }
}

/* Random float in [0, 1): one LCG step, the high 15 bits scaled by 2^-15. */
INCLUDE_ASM("asm/main/nonmatchings/tu170", sefRandf);

/* Returns the next noise sample and advances the read index of the noise table. */
f32 sefNoiseNext(void)
{
    int index = sefNoiseTable.nextIndex;

    sefNoiseTable.nextIndex = (index + 1) & 0x3FF;
    return sefNoiseTable.samples[index];
}

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00261A38);

INCLUDE_ASM("asm/main/nonmatchings/tu170", sefRandAngle);

INCLUDE_ASM("asm/main/nonmatchings/tu170", sefRandVector);

INCLUDE_ASM("asm/main/nonmatchings/tu170", sefNoiseNextVector);

/* Writes the position transformed by the matrix into dest (the position's translation merged into the matrix). */
void sefMergeMatrixPos(Matrix4 *dest, Matrix4 *matrix, Vector4 *position)
{
    QuadwordMergeMatrixPosition(dest, matrix, position);
}

INCLUDE_ASM("asm/main/nonmatchings/tu170", sefBuildEffectMatrix);

INCLUDE_ASM("asm/main/nonmatchings/tu170", sefGetCameraPos);

/* VU0 product of a Vector4 and a column-major matrix: destination = matrix * vector. */
void sefApplyMatrixVector(Vector4 *destination, Matrix4 *matrix, Vector4 *vector)
{
    __asm__ __volatile__("lqc2 vf1, 0(%0)" : : "r"(vector) : "memory");
    __asm__ __volatile__("lqc2 vf2, 0(%0)" : : "r"(matrix) : "memory");
    __asm__ __volatile__("lqc2 vf3, 16(%0)" : : "r"(matrix) : "memory");
    __asm__ __volatile__("lqc2 vf4, 32(%0)" : : "r"(matrix) : "memory");
    __asm__ __volatile__("vmulax.xyzw ACC, vf2, vf1x" : : : "memory");
    __asm__ __volatile__("vmadday.xyzw ACC, vf3, vf1y" : : : "memory");
    __asm__ __volatile__("vmaddaz.xyzw ACC, vf4, vf1z" : : : "memory");
    __asm__ __volatile__("vmaddw.xyzw vf1, vf0, vf1w" : : : "memory");
    __asm__ __volatile__("sqc2 vf1, 0(%0)" : : "r"(destination) : "memory");
}

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00261D50);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00261E40);

/* The effect context returned by sefGetRequestBuffer. +0x318 is the kind-1 slot read by sefGetLinkedFlag; +0x31C..+0x330
 * hold the other kind slots (kinds 2, 5, 3, 4, 6 and the default) that the kind helpers select between. */
typedef struct SefLinkedContext {
    unsigned char unmodeled_000[0x318];
    int objectOffset;
} SefLinkedContext;

/* One effect object record, 0x1670 bytes. Its per-index flag bytes start at +0x163C; bit 0 is the in-use bit that
 * the kind and slot helpers return. */
typedef struct SefObject {
    unsigned char unmodeled_000[0x163C];
    unsigned char flags;
} SefObject;

/* Returns the effect context of the running sef task. */

/* Returns bit 0 of the flags of the object linked to `instance` through the context, or 0 without a link. */
int sefGetLinkedFlag(unsigned char *instance)
{
    SefLinkedContext *context = (SefLinkedContext *) sefGetRequestBuffer();
    int offset = context->objectOffset;

    if (offset == 0) {
        return 0;
    }
    return ((SefObject *) (instance + offset))->flags & 1;
}

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00261EF0);

INCLUDE_ASM("asm/main/nonmatchings/tu170", sefGetKindVector);

INCLUDE_ASM("asm/main/nonmatchings/tu170", sefGetKindMatrix);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00262358);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_002624B0);

INCLUDE_ASM("asm/main/nonmatchings/tu170", sefGetPoint);

INCLUDE_ASM("asm/main/nonmatchings/tu170", sefGetPartMatrix);

INCLUDE_ASM("asm/main/nonmatchings/tu170", sefGetPosition);

INCLUDE_ASM("asm/main/nonmatchings/tu170", sefGetCirclePos);

INCLUDE_ASM("asm/main/nonmatchings/tu170", sefGetSpherePos);

INCLUDE_ASM("asm/main/nonmatchings/tu170", sefGetUnitDirection);

void sefNoiseNextVector(Vector4 *pos);

/* Random point of the cube [-0.5, 0.5) x [0, 1) x [-0.5, 0.5) scaled by extent: the bottom-aligned cube. */
void sefGetCubePosBtm(Vector4 *pos, const int *extent) {
    f32 shiftedY;

    sefNoiseNextVector(pos);
    shiftedY = pos->y + 0.5f;
    pos->y = shiftedY;
    Vu0SefMulVectorXYZ(pos, pos, extent);
}

/* The top-aligned cube: sefGetCubePosBtm with y negated. */
void sefGetCubePosTop(Vector4 *pos, const int *extent)
{
    sefGetCubePosBtm(pos, extent);
    pos->y = -pos->y;
}

/* Random point of the centred cube [-0.5, 0.5)^3 scaled by the extent vector. */
void sefGetCubePosCenter(Vector4 *destination, Vector4 *scale)
{
    sefNoiseNextVector(destination);
    Vu0SefMulVectorXYZ(destination, destination, scale);
}

INCLUDE_ASM("asm/main/nonmatchings/tu170", sefApplyPartTransform);

INCLUDE_ASM("asm/main/nonmatchings/tu170", sefGetShapePos);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_002634C0);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00263550);

INCLUDE_ASM("asm/main/nonmatchings/tu170", sefResetObjects);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00263640);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_00263720);

INCLUDE_ASM("asm/main/nonmatchings/tu170", func_002639A0);
