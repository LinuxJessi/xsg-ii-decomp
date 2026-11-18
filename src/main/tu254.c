#include "common.h"
#include "main/tu254.h"
#include "shared.h"
#include "quadword.h"

/* Partial view of TiHrcData: the dynamics count at +0x20 and the offset of the dynamics records from the HRC data at +0x34. */
typedef struct TiHrcDataView {
    unsigned char unmodeled_00[0x20];
    int dynamicsCount;
    unsigned char unmodeled_24[0x34 - 0x24];
    int dynamicsOffset;
} TiHrcDataView;

/* One dynamics record: 0xB0 bytes, then 16 bytes per vector counted by vectorCount. */
typedef struct TiHrcDynamicsRecord {
    unsigned char unmodeled_00[0x1C];
    int vectorCount[3];
    unsigned char unmodeled_28[0x80 - 0x28];
    Vector4 rows[3];          /* +0x80..+0xB0: copied into each node */
} TiHrcDynamicsRecord;

/* One node of the package buffer (64 bytes). */
typedef struct TiMRDynamicsNode {
    Vector4 rows[3];          /* +0x00..+0x30 */
    TiHrcDynamicsRecord *dynamics; /* +0x30 */
    float weight;             /* +0x34: set to 1.0f */
    unsigned char unmodeled_38[0x8];
} TiMRDynamicsNode;

/* Partial view of the package: the HRC data at +0xD0 and the node buffer at +0xEC. */
typedef struct TiMRPackageNodes {
    unsigned char unmodeled_00[0xD0];
    TiHrcDataView *hrcData;
    unsigned char unmodeled_d4[0x18];
    TiMRDynamicsNode *nodes;
} TiMRPackageNodes;

/* Builds the per-node list of the package from its HRC data (the second argument is the output buffer, stored at +0xEC). */
void tiMRPackageFillNodes(TiMRPackageNodes *pMRPackage, TiMRDynamicsNode *nodes) {
    TiHrcDataView *hrc;
    TiHrcDynamicsRecord *dynamics;
    TiMRDynamicsNode *node;
    unsigned int i;
    Vector4 *vectorBlock;

    pMRPackage->nodes = nodes;
    if (nodes != 0) {
        hrc = pMRPackage->hrcData;
        i = 0;
        dynamics = (TiHrcDynamicsRecord *) ((unsigned char *) hrc + hrc->dynamicsOffset);
        if (hrc->dynamicsCount != 0) {
            node = nodes;
            do {
                QuadwordCopy(&node->rows[1], &dynamics->rows[1]);
                QuadwordCopy(&node->rows[0], &dynamics->rows[0]);
                QuadwordCopy(&node->rows[2], &dynamics->rows[2]);
                i++;
                node->dynamics = dynamics;
                node->weight = 1.0f;
                node++;
                vectorBlock = (Vector4 *) (dynamics + 1);
                dynamics = (TiHrcDynamicsRecord *) (vectorBlock
                    + (dynamics->vectorCount[0] + dynamics->vectorCount[1] + dynamics->vectorCount[2]));
            } while (i < pMRPackage->hrcData->dynamicsCount);
        }
    }
}

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
extern Matrix4 D_00645FB0;

/* Loads the data matrix D_00645FB0 into the VU0 current matrix (vf27..vf30). */
void tiVu0LoadFixedMatrix(void) {
    Vu0SetCurrentMatrix(D_00645FB0);
}

Matrix4 D_00645FB0 = {
    { 1.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
};
