#include "common.h"
#include "main/TiMtnBlend.h"
#include "main/tu026.h"

struct TiMtnBlendContext {
    /* 0x0 */ unsigned char unmodeled_0x0[0xDC];
    /* 0xDC */ int matArray[3]; /* 0xDC: local, 0xE0: global, 0xE4: blend; each holds the address of a 0x40-byte matrix array */
};

extern const char D_0068F868[];
extern const char D_0068F870[];
extern const char D_0068F888[];
extern const char D_0068F8A0[];
extern const char D_0068F8B8[];

/* Swaps the global and blend matrix arrays when flag 0x4 is clear. */
void tiMtnBlendSwapGlobal(struct TiMtnBlendContext *ctx, int *flags) {
    int swap;

    if (!(*flags & 4)) {
        if (ctx == 0) {
            assertFailed(D_0068F868, D_0068F870, 14);
        }
        if (ctx->matArray[1] == 0) {
            assertFailed(D_0068F888, D_0068F870, 15);
        }
        if (ctx->matArray[2] == 0) {
            assertFailed(D_0068F8A0, D_0068F870, 16);
        }
        swap = ctx->matArray[1];
        ctx->matArray[1] = ctx->matArray[2];
        ctx->matArray[2] = swap;
    }
}

/* Swaps the local and blend matrix arrays when flag 0x4 is set. */
void tiMtnBlendSwapLocal(struct TiMtnBlendContext *ctx, int *flags) {
    int swap;

    if (*flags & 4) {
        if (ctx == 0) {
            assertFailed(D_0068F868, D_0068F870, 34);
        }
        if (ctx->matArray[0] == 0) {
            assertFailed(D_0068F8B8, D_0068F870, 35);
        }
        if (ctx->matArray[2] == 0) {
            assertFailed(D_0068F8A0, D_0068F870, 36);
        }
        swap = ctx->matArray[0];
        ctx->matArray[0] = ctx->matArray[2];
        ctx->matArray[2] = swap;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/TiMtnBlend", tiMtnBlendLerpMatrix);

INCLUDE_ASM("asm/main/nonmatchings/TiMtnBlend", tiMtnBlendMatrix);

INCLUDE_ASM("asm/main/nonmatchings/TiMtnBlend", tiMtnBlendUpdateGlobal);

INCLUDE_ASM("asm/main/nonmatchings/TiMtnBlend", tiMtnBlendUpdateLocal);
