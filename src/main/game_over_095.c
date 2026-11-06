#include "common.h"
#include "shared.h"
#include "main/xgl_cd.h"

typedef struct XglPacket XglPacket;

extern void sceVif1PkRef(XglPacket *packet, const void *environment,
                         int count, int mode, int offset, int flags);
extern void sceVif1PkCnt(XglPacket *packet, int count);
extern XglPacket *xglPacketGetCurrent(void);

typedef unsigned int XglQuadword __attribute__((mode(TI)));

/* DrawBack reads the framebuffer-page halfword at sRender +0x14. */
typedef struct {
    u8 unmodeled_00[0x10];
    u16 width;
    u16 height;
    u16 framebuffer_page;
    u8 unmodeled_16[0x0a];
    u16 buffer_select;
} DrawImageRenderState;

typedef union GameOverDmaPacket4 {
    u64 words[4];
    XglQuadword quadwords[2];
} GameOverDmaPacket4;

typedef union GameOverDmaPacket6 {
    u64 words[6];
    XglQuadword quadwords[3];
} GameOverDmaPacket6;

void DrawImage_0020CAB0(void *framebuffer);

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

extern DrawImageRenderState sRender;
extern void sceVif1PkAddDataN(XglPacket *packet, const void *data, int count);
extern u64 TestEnv_0_003E7E30[12];
extern GameOverDmaPacket4 TransEnv_1_003E7E90;
extern GameOverDmaPacket6 FlushEnv_2_003E7EB0;

INCLUDE_ASM("asm/main/nonmatchings/game_over_095", bgLayerReady);

INCLUDE_ASM("asm/main/nonmatchings/game_over_095", gameOverDrawOverlay);

void *memset(void *dest, int value, unsigned int count);
void xglJpegDecode(void *request[]);
extern u8 *WorkEnd;

/*
 * Reads the file at `path` into the scratch area 0xE0000 bytes past the
 * destination (the buffer, or WorkEnd when none is given), hands the read
 * target and the destination to xglJpegDecode and returns the destination.
 */
u8 *gameOverLoadFile(const char *path, u8 *buffer)
{
    u8 *base;
    u8 *read_target;
    void *request[8];

    base = WorkEnd;
    if (buffer != 0) {
        base = buffer;
    }
    read_target = base + 0xE0000;
    xglCdReadFile(path, read_target, 0, 1);
    memset(request, 0, sizeof(request));
    request[0] = read_target;
    request[1] = base;
    xglJpegDecode(request);
    return base;
}

void DrawImage_0020CAB0(void *framebuffer)
{
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    TestEnv_0_003E7E30[4] = ((u64)sRender.buffer_select << 0x25) | ((u64)0x8000 << 0x24);
    sceVif1PkCnt(packet, 0);
    sceVif1PkAddDataN(packet, TestEnv_0_003E7E30, 0x18);
    sceVif1PkRef(packet, TransEnv_1_003E7E90.words, 2, 0, 0, 0);
    sceVif1PkRef(packet, framebuffer, 0x7000, 0, 0x51007000, 0);
    sceVif1PkRef(packet, TransEnv_1_003E7E90.words, 2, 0, 0, 0);
    sceVif1PkRef(packet, (u8 *)framebuffer + 0x70000, 0x7000, 0, 0x51007000, 0);
    sceVif1PkRef(packet, FlushEnv_2_003E7EB0.words, 3, 0, 0, 0);
}

INCLUDE_ASM("asm/main/nonmatchings/game_over_095", gameOverDrawSprite);
