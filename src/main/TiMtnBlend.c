#include "common.h"
#include "main/TiMtnBlend.h"
#include "main/tu026.h"
#include "shared.h"

/* TiMtnBlendChannel.flags: 0x2 the blend window runs, 0x4 selects the local slot (clear: global), 0x40 the blend request. */
#define TI_MTN_BLEND_RUNNING 0x2
#define TI_MTN_BLEND_LOCAL 0x4
#define TI_MTN_BLEND_REQUEST 0x40

/* Slots of TiMtnBlendContext.matArray: the matrix arrays a blend reads and writes. */
enum TiMtnBlendSlot {
    TI_MTN_BLEND_SLOT_LOCAL = 0,
    TI_MTN_BLEND_SLOT_GLOBAL = 1,
    TI_MTN_BLEND_SLOT_BLEND = 2
};

struct TiMtnBlendChannel {
    /* 0x0 */ int flags;
    /* 0x4 */ unsigned char unmodeled_0x4[4];
    /* 0x8 */ float duration;
    /* 0xC */ float blendSpan;
    /* 0x10 */ int elapsed;
    /* 0x14 */ unsigned char unmodeled_0x14[0x3C];
};

struct TiMtnBlendSet {
    /* 0x0 */ unsigned char unmodeled_0x0[0x10];
    /* 0x10 */ int count[4];
};

struct TiMtnBlendContext {
    /* 0x0 */ unsigned char unmodeled_0x0[0x10];
    /* 0x10 */ struct TiMtnBlendChannel channel[2];
    /* 0xB0 */ unsigned char unmodeled_0xB0[0x10];
    /* 0xC0 */ int enabledA;
    /* 0xC4 */ unsigned char unmodeled_0xC4[4];
    /* 0xC8 */ int enabledB;
    /* 0xCC */ unsigned char unmodeled_0xCC[4];
    /* 0xD0 */ struct TiMtnBlendSet *set;
    /* 0xD4 */ unsigned char unmodeled_0xD4[8];
    /* 0xDC */ Matrix4 *matArray[3]; /* 0xDC: local, 0xE0: global, 0xE4: blend; each holds the address of a 0x40-byte matrix array */
};

extern const char D_0068F868[];
extern const char D_0068F870[];
extern const char D_0068F888[];
extern const char D_0068F8A0[];
extern const char D_0068F8B8[];

/* Swaps the global and blend matrix arrays when flag 0x4 is clear. */
void tiMtnBlendSwapGlobal(struct TiMtnBlendContext *ctx, int *flags) {
    Matrix4 *swap;

    if (!(*flags & TI_MTN_BLEND_LOCAL)) {
        if (ctx == 0) {
            assertFailed(D_0068F868, D_0068F870, 14);
        }
        if (ctx->matArray[TI_MTN_BLEND_SLOT_GLOBAL] == 0) {
            assertFailed(D_0068F888, D_0068F870, 15);
        }
        if (ctx->matArray[TI_MTN_BLEND_SLOT_BLEND] == 0) {
            assertFailed(D_0068F8A0, D_0068F870, 16);
        }
        swap = ctx->matArray[TI_MTN_BLEND_SLOT_GLOBAL];
        ctx->matArray[TI_MTN_BLEND_SLOT_GLOBAL] = ctx->matArray[TI_MTN_BLEND_SLOT_BLEND];
        ctx->matArray[TI_MTN_BLEND_SLOT_BLEND] = swap;
    }
}

/* Swaps the local and blend matrix arrays when flag 0x4 is set. */
void tiMtnBlendSwapLocal(struct TiMtnBlendContext *ctx, int *flags) {
    Matrix4 *swap;

    if (*flags & TI_MTN_BLEND_LOCAL) {
        if (ctx == 0) {
            assertFailed(D_0068F868, D_0068F870, 34);
        }
        if (ctx->matArray[TI_MTN_BLEND_SLOT_LOCAL] == 0) {
            assertFailed(D_0068F8B8, D_0068F870, 35);
        }
        if (ctx->matArray[TI_MTN_BLEND_SLOT_BLEND] == 0) {
            assertFailed(D_0068F8A0, D_0068F870, 36);
        }
        swap = ctx->matArray[TI_MTN_BLEND_SLOT_LOCAL];
        ctx->matArray[TI_MTN_BLEND_SLOT_LOCAL] = ctx->matArray[TI_MTN_BLEND_SLOT_BLEND];
        ctx->matArray[TI_MTN_BLEND_SLOT_BLEND] = swap;
    }
}

/* Blends two 4x4 matrices: dst = a * (1 - t) + b * t. The elements are addressed by byte offset: the typed [row][col] form changed the code. */
void tiMtnBlendLerpMatrix(float (*dst)[4], float (*a)[4], float (*b)[4], float t) {
    int byteOffset;
    int row;
    int col;
    float *dstElem;
    float *aElem;
    float *bElem;

    for (row = 0; row < 4; row++) {
        for (col = 0; col < 4; col++) {
            byteOffset = row * 16 + col * 4;
            dstElem = (float *)((char *)dst + byteOffset);
            aElem = (float *)((char *)a + byteOffset);
            bElem = (float *)((char *)b + byteOffset);
            *dstElem = *aElem * (1.0f - t) + *bElem * t;
        }
    }
}

extern const char D_0068F8D0[];
extern const char D_0068F8D8[];
extern const char D_0068F8E0[];
void xglMatrix2Quaternion(Vector4 *quaternion, Matrix4 *source);
void XrgQuaternionSlerp(Vector4 *result, Vector4 *from, Vector4 *to, float t);
void XrgQuaternionToMatrix(Matrix4 matrix, const Vector4 *quaternion);

/* Blends two matrices: the rotation by quaternion slerp, the translation by lerp. */
void tiMtnBlendMatrix(Matrix4 *dst, Matrix4 *matA, Matrix4 *matB, float t) {
    float oneMinusT = 1.0f - t;
    Vector4 startQuat;
    Vector4 endQuat;
    Vector4 blendQuat;
    float translation[3];
    float rowLength[3];
    int row;
    float x;
    float y;
    float z;
    float length;

    if (dst == 0) {
        assertFailed(D_0068F8D0, D_0068F870, 71);
    }
    if (matA == 0) {
        assertFailed(D_0068F8D8, D_0068F870, 72);
    }
    if (matB == 0) {
        assertFailed(D_0068F8E0, D_0068F870, 73);
    }

    translation[0] = (*matA)[3][0] * oneMinusT + (*matB)[3][0] * t;
    translation[1] = (*matA)[3][1] * oneMinusT + (*matB)[3][1] * t;
    translation[2] = (*matA)[3][2] * oneMinusT + (*matB)[3][2] * t;

    for (row = 0; row < 3; row++) {
        x = (*matA)[row][0] * oneMinusT + (*matB)[row][0] * t;
        y = (*matA)[row][1] * oneMinusT + (*matB)[row][1] * t;
        z = (*matA)[row][2] * oneMinusT + (*matB)[row][2] * t;
        length = __builtin_sqrtf(x * x + y * y + z * z);
        rowLength[row] = length;
        if (length <= 1.0e-6f) {
            return;
        }
    }

    xglMatrix2Quaternion(&startQuat, matA);
    xglMatrix2Quaternion(&endQuat, matB);
    XrgQuaternionSlerp(&blendQuat, &startQuat, &endQuat, t);
    XrgQuaternionToMatrix(*dst, &blendQuat);
    (*dst)[3][0] = translation[0];
    (*dst)[3][1] = translation[1];
    (*dst)[3][2] = translation[2];
}
void tiWordClearBits(int *word, int bits);

void tiMtnBlendUpdateGlobal(struct TiMtnBlendContext *ctx) {
    float progress;
    float remaining;

    if (ctx == 0) {
        assertFailed(D_0068F868, D_0068F870, 121);
    }
    if (ctx->enabledA != 0) {
        unsigned int item;
        int matrixIndex;

        if ((ctx->channel[0].flags & TI_MTN_BLEND_REQUEST) == 0) {
            tiWordClearBits(&ctx->channel[0].flags, TI_MTN_BLEND_RUNNING);
        } else if ((*(long long *)&ctx->channel[0].flags & (TI_MTN_BLEND_RUNNING | TI_MTN_BLEND_LOCAL)) == TI_MTN_BLEND_RUNNING) {
            remaining = ctx->channel[0].duration - (float)ctx->channel[0].elapsed;
            if (ctx->channel[0].blendSpan == 0.0f || ctx->channel[0].blendSpan <= remaining || remaining < 0.0f) {
                tiWordClearBits(&ctx->channel[0].flags, TI_MTN_BLEND_RUNNING);
                tiWordClearBits(&ctx->channel[0].flags, TI_MTN_BLEND_REQUEST);
                return;
            }
            if (ctx->matArray[TI_MTN_BLEND_SLOT_BLEND] == 0) {
                assertFailed(D_0068F8A0, D_0068F870, 145);
            }
            if (ctx->matArray[TI_MTN_BLEND_SLOT_GLOBAL] == 0) {
                assertFailed(D_0068F888, D_0068F870, 146);
            }
            progress = remaining / ctx->channel[0].blendSpan;
            matrixIndex = 0;
            if (progress < 0.0f) {
                progress = 0.0f;
            } else if (1.0f < progress) {
                progress = 1.0f;
            }
            for (item = 0; item < ctx->set->count[0]; item++) {
                tiMtnBlendMatrix(ctx->matArray[TI_MTN_BLEND_SLOT_GLOBAL] + matrixIndex, ctx->matArray[TI_MTN_BLEND_SLOT_BLEND] + matrixIndex,
                                 ctx->matArray[TI_MTN_BLEND_SLOT_GLOBAL] + matrixIndex, progress);
                matrixIndex++;
            }
            for (item = 0; item < ctx->set->count[1]; item++) {
                tiMtnBlendMatrix(ctx->matArray[TI_MTN_BLEND_SLOT_GLOBAL] + matrixIndex, ctx->matArray[TI_MTN_BLEND_SLOT_BLEND] + matrixIndex,
                                 ctx->matArray[TI_MTN_BLEND_SLOT_GLOBAL] + matrixIndex, progress);
                matrixIndex++;
            }
            for (item = 0; item < ctx->set->count[2]; item++) {
                tiMtnBlendMatrix(ctx->matArray[TI_MTN_BLEND_SLOT_GLOBAL] + matrixIndex, ctx->matArray[TI_MTN_BLEND_SLOT_BLEND] + matrixIndex,
                                 ctx->matArray[TI_MTN_BLEND_SLOT_GLOBAL] + matrixIndex, progress);
                matrixIndex++;
            }
        }
    }
    if (ctx->enabledB != 0) {
        unsigned int item;
        int matrixIndex;

        if ((ctx->channel[1].flags & TI_MTN_BLEND_REQUEST) == 0) {
            tiWordClearBits(&ctx->channel[1].flags, TI_MTN_BLEND_RUNNING);
        } else if ((*(long long *)&ctx->channel[1].flags & (TI_MTN_BLEND_RUNNING | TI_MTN_BLEND_LOCAL)) == TI_MTN_BLEND_RUNNING) {
            remaining = ctx->channel[1].duration - (float)ctx->channel[1].elapsed;
            if (ctx->channel[1].blendSpan == 0.0f || ctx->channel[1].blendSpan <= remaining || remaining < 0.0f) {
                tiWordClearBits(&ctx->channel[1].flags, TI_MTN_BLEND_RUNNING);
                tiWordClearBits(&ctx->channel[1].flags, TI_MTN_BLEND_REQUEST);
                return;
            }
            if (ctx->matArray[TI_MTN_BLEND_SLOT_BLEND] == 0) {
                assertFailed(D_0068F8A0, D_0068F870, 191);
            }
            if (ctx->matArray[TI_MTN_BLEND_SLOT_GLOBAL] == 0) {
                assertFailed(D_0068F888, D_0068F870, 192);
            }
            progress = remaining / ctx->channel[1].blendSpan;
            matrixIndex = ctx->set->count[0] + ctx->set->count[1] + ctx->set->count[2];
            if (progress < 0.0f) {
                progress = 0.0f;
            } else if (1.0f < progress) {
                progress = 1.0f;
            }
            for (item = 0; item < ctx->set->count[3]; item++) {
                tiMtnBlendMatrix(ctx->matArray[TI_MTN_BLEND_SLOT_GLOBAL] + matrixIndex, ctx->matArray[TI_MTN_BLEND_SLOT_BLEND] + matrixIndex,
                                 ctx->matArray[TI_MTN_BLEND_SLOT_GLOBAL] + matrixIndex, progress);
                matrixIndex++;
            }
        }
    }
}

void tiMtnBlendUpdateLocal(struct TiMtnBlendContext *ctx) {
    float progress;
    float remaining;

    if (ctx == 0) {
        assertFailed(D_0068F868, D_0068F870, 217);
    }
    if (ctx->enabledA != 0) {
        unsigned int item;
        int matrixIndex;

        if ((ctx->channel[0].flags & TI_MTN_BLEND_REQUEST) == 0) {
            tiWordClearBits(&ctx->channel[0].flags, TI_MTN_BLEND_RUNNING);
        } else if ((*(long long *)&ctx->channel[0].flags & (TI_MTN_BLEND_RUNNING | TI_MTN_BLEND_LOCAL)) == (TI_MTN_BLEND_RUNNING | TI_MTN_BLEND_LOCAL)) {
            remaining = ctx->channel[0].duration - (float)ctx->channel[0].elapsed;
            if (ctx->channel[0].blendSpan == 0.0f || ctx->channel[0].blendSpan <= remaining || remaining < 0.0f) {
                tiWordClearBits(&ctx->channel[0].flags, TI_MTN_BLEND_RUNNING);
                tiWordClearBits(&ctx->channel[0].flags, TI_MTN_BLEND_REQUEST);
                return;
            }
            if (ctx->matArray[TI_MTN_BLEND_SLOT_BLEND] == 0) {
                assertFailed(D_0068F8A0, D_0068F870, 241);
            }
            if (ctx->matArray[TI_MTN_BLEND_SLOT_LOCAL] == 0) {
                assertFailed(D_0068F8B8, D_0068F870, 242);
            }
            progress = remaining / ctx->channel[0].blendSpan;
            matrixIndex = 0;
            if (progress < 0.0f) {
                progress = 0.0f;
            } else if (1.0f < progress) {
                progress = 1.0f;
            }
            for (item = 0; item < ctx->set->count[0]; item++) {
                tiMtnBlendMatrix(ctx->matArray[TI_MTN_BLEND_SLOT_LOCAL] + matrixIndex, ctx->matArray[TI_MTN_BLEND_SLOT_BLEND] + matrixIndex,
                                 ctx->matArray[TI_MTN_BLEND_SLOT_LOCAL] + matrixIndex, progress);
                matrixIndex++;
            }
            for (item = 0; item < ctx->set->count[1]; item++) {
                tiMtnBlendMatrix(ctx->matArray[TI_MTN_BLEND_SLOT_LOCAL] + matrixIndex, ctx->matArray[TI_MTN_BLEND_SLOT_BLEND] + matrixIndex,
                                 ctx->matArray[TI_MTN_BLEND_SLOT_LOCAL] + matrixIndex, progress);
                matrixIndex++;
            }
            for (item = 0; item < ctx->set->count[2]; item++) {
                tiMtnBlendMatrix(ctx->matArray[TI_MTN_BLEND_SLOT_LOCAL] + matrixIndex, ctx->matArray[TI_MTN_BLEND_SLOT_BLEND] + matrixIndex,
                                 ctx->matArray[TI_MTN_BLEND_SLOT_LOCAL] + matrixIndex, progress);
                matrixIndex++;
            }
        }
    }
    if (ctx->enabledB != 0) {
        unsigned int item;
        int matrixIndex;

        if ((ctx->channel[1].flags & TI_MTN_BLEND_REQUEST) == 0) {
            tiWordClearBits(&ctx->channel[1].flags, TI_MTN_BLEND_RUNNING);
        } else if ((*(long long *)&ctx->channel[1].flags & (TI_MTN_BLEND_RUNNING | TI_MTN_BLEND_LOCAL)) == (TI_MTN_BLEND_RUNNING | TI_MTN_BLEND_LOCAL)) {
            remaining = ctx->channel[1].duration - (float)ctx->channel[1].elapsed;
            if (ctx->channel[1].blendSpan == 0.0f || ctx->channel[1].blendSpan <= remaining || remaining < 0.0f) {
                tiWordClearBits(&ctx->channel[1].flags, TI_MTN_BLEND_RUNNING);
                tiWordClearBits(&ctx->channel[1].flags, TI_MTN_BLEND_REQUEST);
                return;
            }
            if (ctx->matArray[TI_MTN_BLEND_SLOT_BLEND] == 0) {
                assertFailed(D_0068F8A0, D_0068F870, 287);
            }
            if (ctx->matArray[TI_MTN_BLEND_SLOT_LOCAL] == 0) {
                assertFailed(D_0068F8B8, D_0068F870, 288);
            }
            progress = remaining / ctx->channel[1].blendSpan;
            matrixIndex = ctx->set->count[0] + ctx->set->count[1] + ctx->set->count[2];
            if (progress < 0.0f) {
                progress = 0.0f;
            } else if (1.0f < progress) {
                progress = 1.0f;
            }
            for (item = 0; item < ctx->set->count[3]; item++) {
                tiMtnBlendMatrix(ctx->matArray[TI_MTN_BLEND_SLOT_LOCAL] + matrixIndex, ctx->matArray[TI_MTN_BLEND_SLOT_BLEND] + matrixIndex,
                                 ctx->matArray[TI_MTN_BLEND_SLOT_LOCAL] + matrixIndex, progress);
                matrixIndex++;
            }
        }
    }
}

/* Assert strings: each one is shared by several routines, so each is one named array. */
const char D_0068F868[] = "pPack";
const char D_0068F870[] = "../TiMtnBlend.euc.c";
const char D_0068F888[] = "pPack->pGlobalMat";
const char D_0068F8A0[] = "pPack->pBlendMat";
const char D_0068F8B8[] = "pPack->pLocalMat";
const char D_0068F8D0[] = "pDest";
const char D_0068F8D8[] = "pMat1";
const char D_0068F8E0[] = "pMat2";
