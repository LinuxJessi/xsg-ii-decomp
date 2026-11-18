#include "common.h"
#include "main/tu026.h"
#include "main/tu254.h"
#include "shared.h"

/* Status threshold below which the point entries are indexed by key (else searched by key). */
#define TI_DYNAMICS_INDEXED_THRESHOLD 1.1f

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

/* Status block at MRPack +0xd8: a float threshold, an entry count and the entry array after a 16-byte header. */
typedef struct {
    u8 unmodeled_00[4];
    float threshold;    /* +0x04: below 1.1f the entries are indexed, else searched by key */
    u32 entryCount;     /* +0x08 */
    u8 unmodeled_0c[4];
} DynamicsCollStatus;

/* 28-byte entry of the indexed list. */
typedef struct {
    int matrixIndex;
    float x;
    float y;
    float z;
    u8 unmodeled_10[12];
} DynamicsCollPointEntry;

/* 32-byte entry of the keyed list. */
typedef struct {
    int key;
    int matrixIndex;
    float x;
    float y;
    float z;
    u8 unmodeled_14[12];
} DynamicsCollKeyedEntry;

/* local view: pDynamicsColl (+0xd0), pStatus (+0xd8) and pGlobalMat (+0xe0). The shared TiMRPackage in
 * include/main/TiMRPackage.h types these offsets as TiVersionedBlock * and int, so this view keeps its own pointer types. */
typedef struct {
    u8 unmodeled_00[0xd0];
    DynamicsColl *pDynamicsColl;
    u8 unmodeled_d4[4];
    DynamicsCollStatus *pStatus;
    u8 unmodeled_dc[4];
    Matrix4 *pGlobalMat;
} MRPack;

void xglVectorMulMat(Vector4 *destination, Matrix4 matrix, const Vector4 *vector);
void xglMatrixInverse(Matrix4 *destination, Matrix4 *matrix);
void xglCurMatrixGet(f32 matrix[4][4]);
void xglCurMatrixSet(f32 matrix[4][4]);
void xglCurApplyVector(Vector4 *destination, const Vector4 *vector);

/* Writes the position of the entry for key into dst with w = 1. Below a threshold of 1.1f the entries are indexed
 * directly by key; otherwise the first keyed entry with that key is used. With apply set, dst is then transformed
 * by the global matrix that the entry names. */
int tiMRPackageGetNodePoint(MRPack *pMRPack, u32 key, Vector4 *dst, int apply)
{
    DynamicsCollStatus *pStatus;
    DynamicsCollPointEntry *indexedEntries;
    DynamicsCollKeyedEntry *keyedEntries;
    DynamicsCollKeyedEntry *found;
    u32 entryIndex;

    pStatus = pMRPack->pStatus;
    if (pStatus == 0) {
        return 0;
    }
    if (pStatus->threshold < TI_DYNAMICS_INDEXED_THRESHOLD) {
        indexedEntries = (DynamicsCollPointEntry *) (pStatus + 1);
        if (key >= pStatus->entryCount) {
            return 0;
        }
        dst->x = indexedEntries[key].x;
        dst->y = indexedEntries[key].y;
        dst->z = indexedEntries[key].z;
        dst->w = 1.0f;
        if (apply != 0) {
            xglCurMatrixSet(pMRPack->pGlobalMat[indexedEntries[key].matrixIndex]);
            xglCurApplyVector(dst, dst);
            return 1;
        }
        return 1;
    } else {
        keyedEntries = (DynamicsCollKeyedEntry *) (pStatus + 1);
        found = 0;
        for (entryIndex = 0; entryIndex < pMRPack->pStatus->entryCount; entryIndex++) {
            if (keyedEntries[entryIndex].key == key) {
                found = &keyedEntries[entryIndex];
                break;
            }
        }
        if (found == 0) {
            return 0;
        }
        dst->x = found->x;
        dst->y = found->y;
        dst->z = found->z;
        dst->w = 1.0f;
        if (apply != 0) {
            xglCurMatrixSet(pMRPack->pGlobalMat[found->matrixIndex]);
            xglCurApplyVector(dst, dst);
        }
        return 1;
    }
}

/* Transforms each vector entry from src into dst with its global matrix. */
void tiDynamicsCollTransformVectors(MRPack *pMRPack)
{
    DynamicsCollVectorEntry *entry;
    u32 entryIndex;

    entry = (DynamicsCollVectorEntry *) ((u8 *) pMRPack->pDynamicsColl
                                         + pMRPack->pDynamicsColl->vectorEntriesOffset);
    if (pMRPack->pGlobalMat == 0) {
        assertFailed("pMRPack->pGlobalMat", "../TiDynamicsColl.euc.c", 17);
    }
    entryIndex = 0;
    if (pMRPack->pDynamicsColl->vectorEntryCount != 0) {
        do {
            /* entry advances only for the entries that are transformed (original shape). */
            if (entry->matrixIndex != -1) {
                xglVectorMulMat(&entry->dst, pMRPack->pGlobalMat[entry->matrixIndex], &entry->src);
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
                xglCurMatrixGet(entry->matrix);
            } else {
                xglMatrixInverse(&entry->matrix, pMRPack->pGlobalMat + entry->matrixIndex);
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
                xglCurMatrixGet(entry->matrix);
            } else {
                xglMatrixInverse(&entry->matrix, pMRPack->pGlobalMat + entry->matrixIndex);
            }
            entryIndex++;
            entry++;
        } while (entryIndex < pMRPack->pDynamicsColl->wideMatrixEntryCount);
    }
}
