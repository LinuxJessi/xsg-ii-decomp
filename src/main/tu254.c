#include "common.h"
#include "main/tu254.h"

/*
 * Builds the per-node list of the package from its HRC data: tiMRPackageFillNodes
 * (the second argument is the output buffer, stored at +0xEC of the package).
 */
INCLUDE_ASM("asm/main/nonmatchings/tu254", tiMRPackageFillNodes);

#include "shared.h"
#include "vu0_macro.h"

/*
 * Only the two fields XenoActorConvertSourceRecord touches are modelled. The record is the one at XenoActor +0x650
 * (XenoActorApplySourceValue passes &actor->flags650): sourceValue is XenoActor source_value (+0x660).
 */
typedef struct XenoActorSourceRecord {
    unsigned char unmodeled_00[8];
    f32 sourceValueFloat;
    unsigned char unmodeled_0c[4];
    int sourceValue;
} XenoActorSourceRecord;

/* Converts the source value (int, +0x10) to the float at +8 of the record. */
void XenoActorConvertSourceRecord(XenoActorSourceRecord *record) {
    record->sourceValueFloat = (f32) record->sourceValue;
}

/* Copies one vector through VU0 register vf2. */
void xglVectorCopy_00292E28(Vector4 *destination, const Vector4 *source) {
    __asm__ __volatile__("lqc2 vf2, 0(%0)" : : "r"(source) : "memory");
    __asm__ __volatile__("sqc2 vf2, 0(%0)" : : "r"(destination) : "memory");
}

/* Adds the xyz parts of two vectors; w is taken from the first operand. */
void xglVectorAdd(Vector4 *destination, const Vector4 *first, const Vector4 *second) {
    __asm__ __volatile__("lqc2 vf2, 0(%0)" : : "r"(first) : "memory");
    __asm__ __volatile__("lqc2 vf3, 0(%0)" : : "r"(second) : "memory");
    __asm__ __volatile__("vadd.xyz vf4, vf2, vf3" : : : "memory");
    __asm__ __volatile__("vmove.w vf4, vf2" : : : "memory");
    __asm__ __volatile__("sqc2 vf4, 0(%0)" : : "r"(destination) : "memory");
}

/* Subtracts the xyz parts of two vectors; w is taken from the minuend. */
void xglVectorSubXYZKeepW(Vector4 *destination, const Vector4 *minuend, const Vector4 *subtrahend) {
    __asm__ __volatile__("lqc2 vf2, 0(%0)" : : "r"(minuend) : "memory");
    __asm__ __volatile__("lqc2 vf3, 0(%0)" : : "r"(subtrahend) : "memory");
    __asm__ __volatile__("vsub.xyz vf4, vf2, vf3" : : : "memory");
    __asm__ __volatile__("vmove.w vf4, vf2" : : : "memory");
    __asm__ __volatile__("sqc2 vf4, 0(%0)" : : "r"(destination) : "memory");
}

/* Component-wise product of two vectors. */
void xglVectorMul_00292E78(Vector4 *destination, const Vector4 *first, const Vector4 *second) {
    __asm__ __volatile__("lqc2 vf2, 0(%0)" : : "r"(first) : "memory");
    __asm__ __volatile__("lqc2 vf3, 0(%0)" : : "r"(second) : "memory");
    __asm__ __volatile__("vmul.xyzw vf4, vf2, vf3" : : : "memory");
    __asm__ __volatile__("sqc2 vf4, 0(%0)" : : "r"(destination) : "memory");
}

void xglVectorNormal(Vector4 *destination, const Vector4 *source) {
    __asm__ __volatile__("lqc2 vf2, 0(%0)" : : "r"(source) : "memory");
    __asm__ __volatile__("vmul.xyz vf3, vf2, vf2" : : : "memory");
    __asm__ __volatile__("vaddy.x vf3, vf3, vf3y" : : : "memory");
    __asm__ __volatile__("vaddz.x vf3, vf3, vf3z" : : : "memory");
    __asm__ __volatile__("vrsqrt Q, vf0w, vf3x" : : : "memory");
    __asm__ __volatile__("vwaitq" : : : "memory");
    __asm__ __volatile__("vmulq.xyz vf2, vf2, Q" : : : "memory");
    __asm__ __volatile__("sqc2 vf2, 0(%0)" : : "r"(destination) : "memory");
}

/* Copies four vectors (a 4x4 matrix) through vf2..vf5. */
void xglMatrixCopy_00292EB8(Vector4 *destination, const Vector4 *source) {
    __asm__ __volatile__("lqc2 vf2, 0(%0)" : : "r"(source) : "memory");
    __asm__ __volatile__("lqc2 vf3, 16(%0)" : : "r"(source) : "memory");
    __asm__ __volatile__("lqc2 vf4, 32(%0)" : : "r"(source) : "memory");
    __asm__ __volatile__("lqc2 vf5, 48(%0)" : : "r"(source) : "memory");
    __asm__ __volatile__("sqc2 vf2, 0(%0)" : : "r"(destination) : "memory");
    __asm__ __volatile__("sqc2 vf3, 16(%0)" : : "r"(destination) : "memory");
    __asm__ __volatile__("sqc2 vf4, 32(%0)" : : "r"(destination) : "memory");
    __asm__ __volatile__("sqc2 vf5, 48(%0)" : : "r"(destination) : "memory");
}

/* A four-row matrix in data; its rows load into the resident VU0 matrix. */
extern Vector4 D_00645FB0[4];

/* Loads the data matrix D_00645FB0 into the VU0 current matrix (vf27..vf30). */
void tiVu0LoadFixedMatrix(void) {
    Vu0SetCurrentMatrix(D_00645FB0);
}
