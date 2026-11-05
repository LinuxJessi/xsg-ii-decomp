#include "common.h"
#include "shared.h"

typedef struct XglPacket XglPacket;

extern void sceVif1PkRef(XglPacket *packet, const void *environment,
                         int count, int mode, int offset, int flags);

extern void sceVif1PkCnt(XglPacket *packet, int count);
extern XglPacket *xglPacketGetCurrent(void);
extern void xglSleep(void);

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

extern void xglFontPrintDirectOT(int ot, const char *text);

void sceVif1PkCnt(XglPacket *packet, int count);
void sceVif1PkAddDataN(XglPacket *packet, const void *data, int count);

/* The decoded jpeg is sent in two halves of HDD_TEXTURE_QWC quadwords. */
#define HDD_TEXTURE_QWC 0x7000

#define HDD_TEXTURE_HALF_SIZE (HDD_TEXTURE_QWC * 16)

struct HddInstallParams {
    u8 *image_data;
    u8 step;
    u8 unmodeled_05[3];
    u8 *transfer_buffer;
};

struct HddInstallRenderState {
    u8 unmodeled_00[0x20];
    u16 buffer_select;
};

extern struct HddInstallRenderState sRender;

/* The installer keeps one transfer-buffer pointer in its own BSS. */
extern struct HddInstallParams *HddInstallParam;

/* DMA/VIF tags followed by the GS register data sent by HddInstallSync. */
extern u64 TestEnv_0_00A830A0[12];

extern u64 TransEnv_1_00A83100[4];
extern u64 FlushEnv_2_00A83120[6];

/* Font control bytes passed to the existing font-script renderer. */
extern const char installScreenFontText[8];

void HddInstallSync(void)
{
    XglPacket *packet;
    u8 *transfer_buffer;

    packet = xglPacketGetCurrent();
    transfer_buffer = HddInstallParam->transfer_buffer;
    TestEnv_0_00A830A0[4] = ((u64)sRender.buffer_select << 37) | ((u64)0x8000 << 36);
    sceVif1PkCnt(packet, 0);
    sceVif1PkAddDataN(packet, TestEnv_0_00A830A0, 0x18);
    sceVif1PkRef(packet, TransEnv_1_00A83100, 2, 0, 0, 0);
    sceVif1PkRef(packet, transfer_buffer, HDD_TEXTURE_QWC, 0, 0x51007000, 0);
    sceVif1PkRef(packet, TransEnv_1_00A83100, 2, 0, 0, 0);
    sceVif1PkRef(packet, transfer_buffer + HDD_TEXTURE_HALF_SIZE, HDD_TEXTURE_QWC, 0, 0x51007000, 0);
    sceVif1PkRef(packet, FlushEnv_2_00A83120, 3, 0, 0, 0);
    xglSleep();
    xglFontPrintDirectOT(-1, installScreenFontText);
}

INCLUDE_ASM("asm/nonmatchings/ov02/tu002", func_00A81DB0);

INCLUDE_ASM("asm/nonmatchings/ov02/tu002", func_00A82208);

INCLUDE_ASM("asm/nonmatchings/ov02/tu002", func_00A82418);

INCLUDE_ASM("asm/nonmatchings/ov02/tu002", func_00A825E8);
