#include "common.h"
#include "main/tu026.h"
#include "main/tu254.h"

INCLUDE_ASM("asm/main/nonmatchings/TiDynamicsColl", func_0029A980);

#include "shared.h"

typedef float Matrix4[4][4];

/* 48-byte entry: a vector transformed by a global matrix. A matrix index of -1
 * marks an entry that the transform loop skips. The sphere code in the
 * neighbouring unit (asm) reads dst as the centre, +0x4 as the radius and
 * bit 0 of +0x8 as a disable flag. */
typedef struct {
    int matrixIndex;
    u8 unmodeled_04[12];
    Vector4 src;
    Vector4 dst;
} DynamicsCollVectorEntry;

/* 208-byte entry: a matrix built from a global matrix. */
typedef struct {
    int matrixIndex;
    u8 unmodeled_04[140];
    Matrix4 matrix;
} DynamicsCollMatrixEntry;

/* 224-byte entry, same role as DynamicsCollMatrixEntry with a wider stride. */
typedef struct {
    int matrixIndex;
    u8 unmodeled_04[156];
    Matrix4 matrix;
} DynamicsCollWideMatrixEntry;

/* Sub-object of the MRPack; the three entry lists sit at offsets it stores. */
typedef struct {
    u8 unmodeled_00[0x3c];
    u32 vectorEntryCount;
    u32 matrixEntryCount;
    u32 wideMatrixEntryCount;
    u8 unmodeled_48[4];
    int vectorEntriesOffset;
    int matrixEntriesOffset;
    int wideMatrixEntriesOffset;
} DynamicsColl;

/* pDynamicsColl (+0xd0) and pGlobalMat (+0xe0). TiMRPackage.c declares int
 * settings at the same offsets; the two views are not reconciled. */
typedef struct {
    u8 unmodeled_00[0xd0];
    DynamicsColl *pDynamicsColl;
    u8 unmodeled_d4[0xc];
    Matrix4 *pGlobalMat;
} MRPack;

void xglVectorMulMat(Vector4 *destination, Matrix4 matrix, const Vector4 *vector);
void func_001A9B50(Matrix4 *destination, Matrix4 *matrix);
void xglCurMatrixGet(f32 matrix[4][4]);
/* assert expression "pMRPack->pGlobalMat" */
extern unsigned char tiDynamicsCollAssertExpr[];
/* assert file "../TiDynamicsColl.euc.c" */
extern unsigned char tiDynamicsCollSourceFile[];

/* Transforms each vector entry from src into dst with its global matrix. */
void tiDynamicsCollTransformVectors(MRPack *pMRPack)
{
    DynamicsCollVectorEntry *entry;
    u32 entryIndex;

    entry = (DynamicsCollVectorEntry *) ((u8 *) pMRPack->pDynamicsColl
                                         + pMRPack->pDynamicsColl->vectorEntriesOffset);
    if (pMRPack->pGlobalMat == 0) {
        assertFailed(tiDynamicsCollAssertExpr, tiDynamicsCollSourceFile, 17);
    }
    entryIndex = 0;
    if (pMRPack->pDynamicsColl->vectorEntryCount != 0) {
        do {
            /* entry advances only for the entries that are transformed (original shape). */
            if (entry->matrixIndex != -1) {
                xglVectorMulMat(&entry->dst, pMRPack->pGlobalMat + entry->matrixIndex, &entry->src);
                entry++;
            }
            entryIndex++;
        } while (entryIndex < pMRPack->pDynamicsColl->vectorEntryCount);
    }
}

/* Fills each matrix entry from its global matrix; index -1 runs the VU0 store
 * path (tiVu0LoadFixedMatrix, xglCurMatrixGet) instead. */
void tiDynamicsCollBuildMatrices(MRPack *pMRPack)
{
    DynamicsColl *pDynamicsColl;
    DynamicsCollMatrixEntry *entry;
    u32 entryIndex;

    entryIndex = 0;
    pDynamicsColl = pMRPack->pDynamicsColl;
    entry = (DynamicsCollMatrixEntry *) ((u8 *) pDynamicsColl + pDynamicsColl->matrixEntriesOffset);
    if (pDynamicsColl->matrixEntryCount != 0) {
        do {
            if (entry->matrixIndex == -1) {
                tiVu0LoadFixedMatrix();
                xglCurMatrixGet(&entry->matrix);
            } else {
                func_001A9B50(&entry->matrix, pMRPack->pGlobalMat + entry->matrixIndex);
            }
            entryIndex++;
            entry++;
        } while (entryIndex < pMRPack->pDynamicsColl->matrixEntryCount);
    }
}

/* Same as the matrix entry pass, for the wide entry list. */
void tiDynamicsCollBuildWideMatrices(MRPack *pMRPack)
{
    DynamicsColl *pDynamicsColl;
    DynamicsCollWideMatrixEntry *entry;
    u32 entryIndex;

    entryIndex = 0;
    pDynamicsColl = pMRPack->pDynamicsColl;
    entry = (DynamicsCollWideMatrixEntry *) ((u8 *) pDynamicsColl + pDynamicsColl->wideMatrixEntriesOffset);
    if (pDynamicsColl->wideMatrixEntryCount != 0) {
        do {
            if (entry->matrixIndex == -1) {
                tiVu0LoadFixedMatrix();
                xglCurMatrixGet(&entry->matrix);
            } else {
                func_001A9B50(&entry->matrix, pMRPack->pGlobalMat + entry->matrixIndex);
            }
            entryIndex++;
            entry++;
        } while (entryIndex < pMRPack->pDynamicsColl->wideMatrixEntryCount);
    }
}
