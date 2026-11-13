#include "common.h"
#include "shared.h"
#include "main/tu100.h"
#include "main/ysegf.h"
#include "main/tu147.h"
#include "main/xgl_flags.h"

typedef struct XglPacket XglPacket;

/* EE scratchpad RAM. */
#define SCRATCHPAD_BASE 0x70000000

extern void sceVif1PkCnt(XglPacket *packet, int count);
extern void sceVif1PkOpenDirectHLCode(XglPacket *packet, int mode);
extern void sceVif1PkCloseDirectHLCode(XglPacket *packet);
extern XglPacket *xglPacketGetCurrent(void);

/* DrawBack reads the framebuffer-page halfword at sRender +0x14. */
typedef struct {
    u8 unmodeled_00[0x10];
    u16 width;
    u16 height;
    u16 framebuffer_page;
    u8 unmodeled_16[0x0a];
    u16 buffer_select;
} DrawImageRenderState;

int copyframe(void);

/*
 * The VIF1 direct-mode packet builder object. xglPacketInit constructs two
 * of them back to back inside asPacketSource, 0x28 bytes apart (main
 * 0x0022c458-0x0022c4b4 writes both entries' +0x20 and +0x24 at that
 * stride), so sizeof(XglPacket) is 0x28. Only the members the functions of
 * this TU touch are named; everything the sceVif1Pk* library alone reads or
 * writes stays unmodeled.
 */
struct XglPacket {
    /* xglFontReloadTexture decrements this forward cursor by four words. */
    u32 *write_cursor;
    /*
     * +0x04: the address xglPacketSend hands to xglDmaDirectSrcChain as the
     * DMA source-chain start whenever a packet is pending (main
     * 0x0022c520-0x0022c548). No function of this TU writes it -- neither
     * xglPacketInit nor xglPacketMove touches +0x04 -- so it is set by the
     * sceVif1Pk* library: sceVif1PkInit (main 0x0020ac68) stores its base
     * argument at +0x00 and +0x04, and xglPacketInit passes it each entry's
     * buffer base (0x00c00000 and 0x00e00000).
     */
    u32 start;
    /*
 * +0x08 and +0x0c: xglPrimAddGifTagDirect (main 0x00222c00 lw v0,8(s0);
 * main 0x00222c0c lw v0,0xc(s0)) tests both for non-zero to decide
 * whether a direct-mode block the sceVif1Pk* library opened is still
 * open, so it appends to that block instead of resetting/opening/
 * closing a new one (compare SGsSendPacket, src/main/sv_get_image_
 * item.c, which always resets/opens/closes without this check).
 * Neither word is written by any function of this TU, so both are set
 * by the sceVif1Pk* library.
 */
    u32 directCode;
    u32 *directState;
    unsigned char unmodeled_10[0x10];
    /*
     * +0x20: the packet's fixed top-of-buffer address. xglPacketInit sets it
     * once per entry after both sceVif1PkInit calls (main 0x0022c49c for the
     * first entry, value 0x00e00000; main 0x0022c4a0 for the second, value
     * 0x01000000): the end of the buffer whose base went to sceVif1PkInit.
     */
    u32 limit;
    /*
     * +0x24: the write cursor. xglPacketInit seeds it equal to `limit` (main
     * 0x0022c48c/0x0022c490) and xglPacketMove reloads `limit` into it at
     * the start of every frame (main 0x0022c4e4/0x0022c4f0), so writers that
     * fill the packet backward from the top always restart from the same
     * address. main/tu107's nmlPacketSetAttributeData and
     * nmlPacketSetAttributeAlloc16N read it, decrement it by the size being
     * requested and store it back, then use the new value as the memcpy
     * destination or return it as the allocation (main 0x00238e28-0x00238e3c
     * and 0x00238fe0-0x00238ff4),
     * which is why the member is byte-addressed instead of scaled.
     */
    u8 *cursor;
};

/* Screen height in lines (copyframe's source rectangle). */
#define GAME_OVER_SCREEN_HEIGHT 448

extern DrawImageRenderState sRender;
extern void sceVif1PkAddDirectDataN(XglPacket *packet, const void *data, int count);

int func_00A80798(void) {
    return 0;
}

int copyframe(void)
{
    XglPacket *packet;
    u64 *scratch;

    packet = xglPacketGetCurrent();
    scratch = (u64 *)SCRATCHPAD_BASE;
    scratch[0] = 0;
    scratch[1] = 0x51000009;
    scratch[2] = ((u64)0x10000000 << 32) | 0x8008;
    scratch[3] = 14;
    scratch[4] = (0x80000 | (sRender.width << 5)) |
                 ((u64)sRender.height << 37) | ((u64)0x8000 << 36);
    scratch[5] = 80;
    scratch[6] = 0;
    scratch[7] = 81;
    scratch[8] = ((u64)GAME_OVER_SCREEN_HEIGHT << 32) | 0x200;
    scratch[9] = 82;
    scratch[10] = 2;
    scratch[11] = 83;
    scratch[12] = 0;
    scratch[13] = 63;
    scratch[14] = (0x80000 | (sRender.width << 5)) |
                  ((u64)sRender.framebuffer_page << 37) | ((u64)0x8000 << 36);
    scratch[15] = 80;
    scratch[16] = 2;
    scratch[17] = 83;
    scratch[18] = 0;
    scratch[19] = 63;
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    sceVif1PkAddDirectDataN(packet, scratch, 10);
    sceVif1PkCloseDirectHLCode(packet);
}

void func_00A808C8(void) {

}

/* Starts the title music (proposed titleStartBgm, low confidence): opens the M-01.adx file. */
INCLUDE_ASM("asm/nonmatchings/ov02/tu001", func_00A808D0);

INCLUDE_ASM("asm/nonmatchings/ov02/tu001", func_00A80930);

/* Title scene step (proposed titleRunScene, low confidence). */
INCLUDE_ASM("asm/nonmatchings/ov02/tu001", func_00A80A28);

/* Title per-frame draw callback (proposed titleDrawFrame). */
INCLUDE_ASM("asm/nonmatchings/ov02/tu001", titleDrawFrame);

/* Title fade transition (proposed titleFadeTransition). */
INCLUDE_ASM("asm/nonmatchings/ov02/tu001", titleFadeTransition);

/* The title screen (proposed Title). */
INCLUDE_ASM("asm/nonmatchings/ov02/tu001", Title);

void *memset(void *, int, unsigned int);
extern u8 D_2B5D90[0x1B30];

void titleResetState(void) {
    memset(D_2B5D90, 0, 0x1B30);
    xglFlagsClear();
    menuSceneReset();
    yg2PathResetAll();
    msgBalanceReset();
}
