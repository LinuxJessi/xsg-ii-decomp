#include "common.h"
#include "shared.h"

typedef struct XglPacket XglPacket;
typedef unsigned int XglQuadword __attribute__((mode(TI)));

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

/* GIF tag fields. */
#define GIF_TAG_EOP 0x8000

#define GIF_TAG_PRE 0x4000
#define GIF_TAG_PRIM_SHIFT 15
#define GIF_TAG_NREG_SHIFT 28
#define GIF_REG_RGBAQ 1
#define GIF_REG_XYZ2 5
#define GIF_REG_AD 14
#define GS_PRIM_SPRITE 6
#define SCE_GS_XYOFFSET_1 0x18
#define SCE_GS_XYOFFSET_2 0x19
#define SCE_GS_PRMODECONT 0x1A
#define SCE_GS_TEXA 0x3B
#define SCE_GS_SCISSOR_1 0x40
#define SCE_GS_SCISSOR_2 0x41
#define SCE_GS_DIMX 0x44
#define SCE_GS_DTHE 0x45
#define SCE_GS_COLCLAMP 0x46
#define SCE_GS_TEST_1 0x47
#define SCE_GS_TEST_2 0x48
#define SCE_GS_FRAME_1 0x4C
#define SCE_GS_FRAME_2 0x4D
#define SCE_GS_ZBUF_1 0x4E
#define SCE_GS_ZBUF_2 0x4F

/* GS alpha 0x80 is fully opaque. */
#define GS_ALPHA_OPAQUE 0x80

#define XGL_DMA_CHANNEL_GIF 2

void xglDmaDirectNormal(u32 channel, u32 address, u32 count);

extern void xglStudioInit(void);
extern void xglStudioMainCameraInit(void);

/* GIF_STAT and VIF1_STAT, polled by the VSync callback. */
#define GIF_STAT_ADDRESS 0x10003020

#define VIF1_STAT_ADDRESS 0x10003C00
#define GIF_STAT_APATH_MASK 0xC00 /* a GIF transfer path is active */
#define VIF1_STAT_VPS_MASK 3 /* the VIF1 pipeline is not idle */
#define RENDER_COUNT_RESO 6
#define GS_COORD_CENTER 2048 /* primitive coordinate system origin */

/* ScanLineInterpolate modes. */
#define GS_PMODE_EN2 2 /* PMODE.EN2: xglRenderDispOn/Off toggle the display output */

/* Vsyncs per frame: the render thread waits for two, and a third means the frame ran late. */
#define RENDER_VSYNC_PER_FRAME 2

#define GS_ZTST_GEQUAL 2
#define GS_TEST_ZTE_GEQUAL 0x50000
#define GS_TEST_CLEAR_DEPTH 0x00032001 /* alpha test never passes but writes Z only */
#define GS_TEST_CLEAR_FRAME 0x00030003

/* Default clear colour: black with GS alpha 0x80. */
#define RENDER_CLEAR_COLOR_DEFAULT 0x80000000

/* Clear sprite GIF tag: PRE with a sprite PRIM, and the register list
 * A+D (state), RGBAQ, XYZ2, XYZ2 (two corners), plus a closing A+D for the clear packet. */
#define RENDER_CLEAR_GIF_REGS (GIF_REG_AD | (GIF_REG_RGBAQ << 4) | (GIF_REG_XYZ2 << 8) | (GIF_REG_XYZ2 << 12))

#define RENDER_CLEAR_QWORDS 6

/*
 * GS clear-environment block (0x004A9080, size 0x60). xglRenderClearDepth
 * and xglRenderClearFrame each write only the 8-byte tag at +0x10 (a
 * GS-register-shaped value selecting what a queued clear targets);
 * xglRenderClearColor splits a packed 0xAABBGGRR colour into the four
 * one-byte-per-word fields at +0x20..+0x2c. Bytes no claimed function
 * touches stay unmodeled.
 */
typedef struct GsClearEnv {
    u32 gif_tag_low;
    u32 gif_tag_high;
    u32 gif_registers_low;
    u32 gif_registers_high;
    u64 target;
    u64 target_address;
    u32 color_r;
    u32 color_g;
    u32 color_b;
    u32 color_a;
    u32 left;
    u32 top;
    unsigned char unmodeled_38[0x08];
    u32 right;
    u32 bottom;
    unsigned char unmodeled_48[0x08];
    u64 restore_target;
    u64 restore_target_address;
} GsClearEnv;

typedef union GsClearPacket {
    GsClearEnv value;
    XglQuadword quadwords[6];
} GsClearPacket;

/* GS PMODE register: display circuit enables and the alpha blend value. */
typedef struct GsPmode {
    unsigned int en1 : 1;
    unsigned int en2 : 1;
    unsigned int crtmd : 3;
    unsigned int mmod : 1;
    unsigned int amod : 1;
    unsigned int slbg : 1;
    unsigned int alp : 8;
    unsigned int unmodeled_16 : 16;
    unsigned int unmodeled_32 : 32;
} GsPmode;

typedef struct GsDispfb {
    unsigned int fbp : 9;
    unsigned int fbw : 6;
    unsigned int psm : 5;
    unsigned int unmodeled_20 : 12;
    unsigned int dbx : 11;
    unsigned int dby : 11;
    unsigned int unmodeled_54 : 10;
} GsDispfb;

typedef struct GsDisplay {
    unsigned int dx : 12;
    unsigned int dy : 11;
    unsigned int magh : 4;
    unsigned int magv : 2;
    unsigned int unmodeled_29 : 3;
    unsigned int dw : 12;
    unsigned int dh : 11;
    unsigned int unmodeled_55 : 9;
} GsDisplay;

typedef union GsDispfbRegister {
    u64 value;
    GsDispfb field;
} GsDispfbRegister;

typedef union GsDisplayRegister {
    u64 value;
    GsDisplay field;
} GsDisplayRegister;

/*
 * GS display-environment block (0x004A9000, size 0x50). xglRenderDispOff/On
 * only touch the 8-byte flag word at +0x00 (bit 0x2 gates display output).
 */
typedef struct GsDispEnv {
    union {
        u64 flags;
        GsPmode pmode;
    };
    unsigned char unmodeled_08[8];
    GsDispfbRegister dispfb1;          /* +0x10 */
    GsDisplayRegister display1;        /* +0x18 */
    unsigned char unmodeled_20[8];
    GsDispfbRegister dispfb2;          /* +0x28 */
    GsDisplayRegister display2;        /* +0x30 */
    u64 extbuf;                        /* +0x38 */
    u64 extdata;                       /* +0x40 */
    u64 extwrite;                      /* +0x48 */
} GsDispEnv;

/*
 * Packed GS registers of the draw-environment block below, laid out as
 * the GS manual defines them (bit positions from the first field).
 */
typedef struct GsFrame {
    u64 fbp : 9;
    u64 unmodeled_09 : 7;
    u64 fbw : 6;
    u64 unmodeled_22 : 2;
    u64 psm : 6;
    u64 unmodeled_30 : 2;
    u64 fbmsk : 32;
} GsFrame;

typedef struct GsZbuf {
    u64 zbp : 9;
    u64 unmodeled_09 : 15;
    u64 psm : 4;
    u64 unmodeled_28 : 4;
    u64 zmsk : 1;
    u64 unmodeled_33 : 31;
} GsZbuf;

typedef struct GsXyoffset {
    u64 ofx : 16;
    u64 unmodeled_16 : 16;
    u64 ofy : 16;
    u64 unmodeled_48 : 16;
} GsXyoffset;

typedef struct GsScissor {
    u64 scax0 : 11;
    u64 unmodeled_11 : 5;
    u64 scax1 : 11;
    u64 unmodeled_27 : 5;
    u64 scay0 : 11;
    u64 unmodeled_43 : 5;
    u64 scay1 : 11;
    u64 unmodeled_59 : 5;
} GsScissor;

typedef struct GsTest {
    u64 ate : 1;
    u64 atst : 3;
    u64 aref : 8;
    u64 afail : 2;
    u64 date : 1;
    u64 datm : 1;
    u64 zte : 1;
    u64 ztst : 2;
    u64 unmodeled_19 : 45;
} GsTest;

typedef struct GsTexa {
    u64 ta0 : 8;
    u64 unmodeled_08 : 7;
    u64 aem : 1;
    u64 unmodeled_16 : 16;
    u64 ta1 : 8;
    u64 unmodeled_40 : 24;
} GsTexa;

typedef struct GsDimx {
    u64 dm00 : 3;
    u64 unmodeled_03 : 1;
    u64 dm01 : 3;
    u64 unmodeled_07 : 1;
    u64 dm02 : 3;
    u64 unmodeled_11 : 1;
    u64 dm03 : 3;
    u64 unmodeled_15 : 1;
    u64 dm10 : 3;
    u64 unmodeled_19 : 1;
    u64 dm11 : 3;
    u64 unmodeled_23 : 1;
    u64 dm12 : 3;
    u64 unmodeled_27 : 1;
    u64 dm13 : 3;
    u64 unmodeled_31 : 1;
    u64 dm20 : 3;
    u64 unmodeled_35 : 1;
    u64 dm21 : 3;
    u64 unmodeled_39 : 1;
    u64 dm22 : 3;
    u64 unmodeled_43 : 1;
    u64 dm23 : 3;
    u64 unmodeled_47 : 1;
    u64 dm30 : 3;
    u64 unmodeled_51 : 1;
    u64 dm31 : 3;
    u64 unmodeled_55 : 1;
    u64 dm32 : 3;
    u64 unmodeled_59 : 1;
    u64 dm33 : 3;
    u64 unmodeled_63 : 1;
} GsDimx;

typedef struct GsPrmodecont {
    u64 ac : 1;
    u64 unmodeled_01 : 63;
} GsPrmodecont;

typedef struct GsColclamp {
    u64 clamp : 1;
    u64 unmodeled_01 : 63;
} GsColclamp;

typedef struct GsDthe {
    u64 dthe : 1;
    u64 unmodeled_01 : 63;
} GsDthe;

typedef struct GsGifRegisters {
    u64 regs0 : 4;
    u64 unmodeled_04 : 60;
} GsGifRegisters;

typedef struct GsGifTagControl {
    u64 nloop : 15;
    u64 eop : 1;
    u64 unmodeled_16 : 30;
    u64 pre : 1;
    u64 prim : 11;
    u64 flg : 2;
    u64 nreg : 4;
} GsGifTagControl;

/*
 * GS draw-environment block (0x004A8F00, size 0x100): a GIF A+D packet of
 * fifteen register writes, each an 8-byte value followed by its register
 * address, as sceGsPutDrawEnv sends it.
 */
typedef struct GsDrawEnv {
    GsGifTagControl gif_tag;
    GsGifRegisters gif_registers;
    GsFrame frame1;
    u64 frame1_address;
    GsZbuf zbuf1;
    u64 zbuf1_address;
    GsXyoffset xyoffset1;
    u64 xyoffset1_address;
    GsScissor scissor1;
    u64 scissor1_address;
    GsTest test1;
    u64 test1_address;
    GsFrame frame2;
    u64 frame2_address;
    GsZbuf zbuf2;
    u64 zbuf2_address;
    GsXyoffset xyoffset2;
    u64 xyoffset2_address;
    GsScissor scissor2;
    u64 scissor2_address;
    GsTest test2;
    u64 test2_address;
    GsPrmodecont prmodecont;
    u64 prmodecont_address;
    GsTexa texa;
    u64 texa_address;
    GsColclamp colclamp;
    u64 colclamp_address;
    GsDthe dthe;
    u64 dthe_address;
    GsDimx dimx;
    u64 dimx_address;
} GsDrawEnv;

typedef union GsDrawPacket {
    GsDrawEnv value;
    XglQuadword quadwords[16];
} GsDrawPacket;

typedef void (*XglRenderFadeCallback)(XglPacket *packet);

/* One row of asDispReso: the display geometry sceGsSetDefDispEnv and the draw environment take. */
typedef struct XglRenderReso {
    short depth_psm;                   /* +0x00 */
    short frame_psm;                   /* +0x02 */
    short frame_width;                 /* +0x04 */
    short frame_height;                /* +0x06 */
} XglRenderReso;

/* One row of asVramBase: the GS frame-buffer pages of a resolution. */
typedef struct XglRenderVram {
    unsigned short depth_base;         /* +0x08 */
    unsigned short unmodeled_0a[3];
    u16 width;                         /* +0x10 */
    u16 height;                        /* +0x12 */
    unsigned short flip_base;          /* +0x14 */
    unsigned short unmodeled_16[5];
} XglRenderVram;

typedef union XglRenderResoSlot {
    XglRenderReso row;
    short value[4];
} XglRenderResoSlot;

typedef union XglRenderVramSlot {
    XglRenderVram row;
    unsigned short value[12];
} XglRenderVramSlot;

typedef struct XglRenderState {
    XglRenderResoSlot reso;            /* +0x00 */
    XglRenderVramSlot vram;            /* +0x08 */
    u16 display_buffer_base;            /* +0x20 */
    u16 draw_buffer_base;               /* +0x22 */
    XglRenderFadeCallback final_packet_callback[8]; /* +0x24 */
    XglRenderFadeCallback fade_callback; /* +0x44 */
    u32 render_status;                  /* +0x48 */
    u32 frame_delta;                    /* +0x4c */
    u32 frame_count;                    /* +0x50 */
    u32 frame_status;                   /* +0x54 */
    unsigned char scene_disabled;       /* +0x58 */
    unsigned char unmodeled_59[3];
} XglRenderState;

extern GsDrawPacket DrawEnv;
extern GsClearPacket ClearEnv;
extern GsDispEnv DispEnv;
extern XglRenderState sRender;
extern void xglRenderSetReso(int index);
extern void SyncDCache(void *start, void *end);
extern int xglRenderVSyncCallback(void);
extern void sceGsSetDefDispEnv(GsDispEnv *env, int psm, int width, int height, int dx, int dy);
extern void sceGsPutDrawEnv(GsDrawEnv *env);

/* The GS VSync interrupt callback increments this counter while
 * xglRenderSyncMove polls it on the render thread. */
extern volatile u32 VSyncCount;

extern XglRenderReso asDispReso[RENDER_COUNT_RESO];
extern XglRenderVram asVramBase[RENDER_COUNT_RESO];

int xglRenderVSyncCallback(void);

void xglRenderDispEnvInit(void);
void xglRenderClearEnvInit(void);

INCLUDE_ASM("asm/main/nonmatchings/xgl_render", func_001B2DE4);

INCLUDE_ASM("asm/main/nonmatchings/xgl_render", func_001B2E08);

INCLUDE_ASM("asm/main/nonmatchings/xgl_render", func_001B2E30);

INCLUDE_ASM("asm/main/nonmatchings/xgl_render", func_001B2FD0);

INCLUDE_ASM("asm/main/nonmatchings/xgl_render", func_001B3228);

INCLUDE_ASM("asm/main/nonmatchings/xgl_render", func_001B3480);

INCLUDE_ASM("asm/main/nonmatchings/xgl_render", func_001B36D8);

INCLUDE_ASM("asm/main/nonmatchings/xgl_render", xglRenderVSyncCallback);

void xglRenderSyncInit(void)
{
    sceGsSyncVCallback(xglRenderVSyncCallback);
    VSyncCount = 0;
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_render", func_001B3A4C);

static void xglRenderDrawEnvInit(void)
{
    DrawEnv.value.gif_tag.nloop = 15;
    DrawEnv.value.gif_tag.eop = 1;
    DrawEnv.value.gif_tag.pre = 0;
    DrawEnv.value.gif_tag.prim = 0;
    DrawEnv.value.gif_tag.flg = 0;
    DrawEnv.value.gif_tag.nreg = 1;
    DrawEnv.value.gif_registers.regs0 = GIF_REG_AD;
    DrawEnv.value.frame1.fbp = sRender.vram.row.height;
    DrawEnv.value.frame1.fbw = sRender.reso.row.frame_width / 64;
    DrawEnv.value.frame1.psm = sRender.reso.row.frame_psm;
    DrawEnv.value.frame1.fbmsk = 0;
    DrawEnv.value.frame1_address = SCE_GS_FRAME_1;
    DrawEnv.value.zbuf1.zbp = sRender.vram.row.depth_base;
    DrawEnv.value.zbuf1.psm = sRender.reso.row.depth_psm;
    DrawEnv.value.zbuf1.zmsk = 0;
    DrawEnv.value.zbuf1_address = SCE_GS_ZBUF_1;
    DrawEnv.value.xyoffset1.ofx = (GS_COORD_CENTER - (sRender.reso.row.frame_width >> 1)) << 4;
    DrawEnv.value.xyoffset1.ofy = (GS_COORD_CENTER - (sRender.reso.row.frame_height >> 1)) << 4;
    DrawEnv.value.xyoffset1_address = SCE_GS_XYOFFSET_1;
    DrawEnv.value.scissor1.scax0 = 0;
    DrawEnv.value.scissor1.scax1 = sRender.reso.row.frame_width - 1;
    DrawEnv.value.scissor1.scay0 = 0;
    DrawEnv.value.scissor1.scay1 = sRender.reso.row.frame_height - 1;
    DrawEnv.value.scissor1_address = SCE_GS_SCISSOR_1;
    DrawEnv.value.test1.ate = 0;
    DrawEnv.value.test1.atst = 0;
    DrawEnv.value.test1.aref = 0;
    DrawEnv.value.test1.afail = 0;
    DrawEnv.value.test1.date = 0;
    DrawEnv.value.test1.datm = 0;
    DrawEnv.value.test1.zte = 1;
    DrawEnv.value.test1.ztst = GS_ZTST_GEQUAL;
    DrawEnv.value.test1_address = SCE_GS_TEST_1;
    DrawEnv.value.frame2.fbp = sRender.vram.row.height;
    DrawEnv.value.frame2.fbw = sRender.reso.row.frame_width / 64;
    DrawEnv.value.frame2.psm = sRender.reso.row.frame_psm;
    DrawEnv.value.frame2.fbmsk = 0;
    DrawEnv.value.frame2_address = SCE_GS_FRAME_2;
    DrawEnv.value.zbuf2.zbp = sRender.vram.row.depth_base;
    DrawEnv.value.zbuf2.psm = sRender.reso.row.depth_psm;
    DrawEnv.value.zbuf2.zmsk = 0;
    DrawEnv.value.zbuf2_address = SCE_GS_ZBUF_2;
    DrawEnv.value.xyoffset2.ofx = (GS_COORD_CENTER - (sRender.reso.row.frame_width >> 1)) << 4;
    DrawEnv.value.xyoffset2.ofy = (GS_COORD_CENTER - (sRender.reso.row.frame_height >> 1)) << 4;
    DrawEnv.value.xyoffset2_address = SCE_GS_XYOFFSET_2;
    DrawEnv.value.scissor2.scax0 = 0;
    DrawEnv.value.scissor2.scax1 = sRender.reso.row.frame_width - 1;
    DrawEnv.value.scissor2.scay0 = 0;
    DrawEnv.value.scissor2.scay1 = sRender.reso.row.frame_height - 1;
    DrawEnv.value.scissor2_address = SCE_GS_SCISSOR_2;
    DrawEnv.value.test2.ate = 0;
    DrawEnv.value.test2.atst = 0;
    DrawEnv.value.test2.aref = 0;
    DrawEnv.value.test2.afail = 0;
    DrawEnv.value.test2.date = 0;
    DrawEnv.value.test2.datm = 0;
    DrawEnv.value.test2.zte = 1;
    DrawEnv.value.test2.ztst = GS_ZTST_GEQUAL;
    DrawEnv.value.test2_address = SCE_GS_TEST_2;
    DrawEnv.value.prmodecont.ac = 1;
    DrawEnv.value.prmodecont_address = SCE_GS_PRMODECONT;
    DrawEnv.value.texa.ta0 = GS_ALPHA_OPAQUE;
    DrawEnv.value.texa.aem = 0;
    DrawEnv.value.texa.ta1 = GS_ALPHA_OPAQUE;
    DrawEnv.value.texa_address = SCE_GS_TEXA;
    DrawEnv.value.colclamp.clamp = 1;
    DrawEnv.value.colclamp_address = SCE_GS_COLCLAMP;
    DrawEnv.value.dthe.dthe = 0;
    DrawEnv.value.dthe_address = SCE_GS_DTHE;
    DrawEnv.value.dimx.dm00 = 4;
    DrawEnv.value.dimx.dm01 = 2;
    DrawEnv.value.dimx.dm02 = 5;
    DrawEnv.value.dimx.dm03 = 3;
    DrawEnv.value.dimx.dm10 = 0;
    DrawEnv.value.dimx.dm11 = 6;
    DrawEnv.value.dimx.dm12 = 1;
    DrawEnv.value.dimx.dm13 = 7;
    DrawEnv.value.dimx.dm20 = 5;
    DrawEnv.value.dimx.dm21 = 3;
    DrawEnv.value.dimx.dm22 = 4;
    DrawEnv.value.dimx.dm23 = 2;
    DrawEnv.value.dimx.dm30 = 1;
    DrawEnv.value.dimx.dm31 = 7;
    DrawEnv.value.dimx.dm32 = 0;
    DrawEnv.value.dimx.dm33 = 6;
    DrawEnv.value.dimx_address = SCE_GS_DIMX;
}

void xglRenderDrawEnvMove(void)
{
    DrawEnv.value.frame1.fbp = sRender.vram.row.height;
    DrawEnv.value.frame2.fbp = sRender.vram.row.height;
    DrawEnv.value.zbuf1.zbp = sRender.vram.row.depth_base;
    DrawEnv.value.zbuf2.zbp = sRender.vram.row.depth_base;
    SyncDCache(&DrawEnv.value, &DrawEnv.value + 1);
    sceGsPutDrawEnv(&DrawEnv.value);
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_render", xglRenderDispEnvInit);

INCLUDE_ASM("asm/main/nonmatchings/xgl_render", func_001B4A38);

void xglRenderClearDepth(void)
{
    ClearEnv.value.target = GS_TEST_CLEAR_DEPTH;
}

void xglRenderClearFrame(void)
{
    ClearEnv.value.target = GS_TEST_CLEAR_FRAME;
}

void xglRenderClearColor(u32 color)
{
    ClearEnv.value.color_r = color & 0xff;
    ClearEnv.value.color_g = (color >> 8) & 0xff;
    ClearEnv.value.color_b = (color >> 16) & 0xff;
    ClearEnv.value.color_a = (color >> 24) & 0xff;
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_render", xglRenderClearEnvInit);

void xglRenderClearEnvMove(void)
{
    SyncDCache(&ClearEnv.value, &ClearEnv.value + 1);
    xglDmaDirectNormal(XGL_DMA_CHANNEL_GIF, (u32) &ClearEnv.value, RENDER_CLEAR_QWORDS);
}

void xglRenderSetReso(int resolution)
{
    sRender.reso.row = asDispReso[resolution];
    sRender.vram.row = asVramBase[resolution];
    sRender.display_buffer_base = sRender.vram.row.width;
    sRender.draw_buffer_base = sRender.vram.row.height;
    xglRenderDrawEnvInit();
    xglRenderDispEnvInit();
    xglRenderClearEnvInit();
    xglStudioInit();
    xglStudioMainCameraInit();
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_render", func_001B5018);

void xglRenderDispOff(void)
{
    DispEnv.flags &= ~(u64) GS_PMODE_EN2;
}

void xglRenderDispOn(void)
{
    DispEnv.flags |= GS_PMODE_EN2;
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_render", func_001B5114);

INCLUDE_ASM("asm/main/nonmatchings/xgl_render", func_001B513C);

INCLUDE_ASM("asm/main/nonmatchings/xgl_render", func_001B5160);

INCLUDE_ASM("asm/main/nonmatchings/xgl_render", func_001B529C);

INCLUDE_ASM("asm/main/nonmatchings/xgl_render", func_001B534C);

INCLUDE_ASM("asm/main/nonmatchings/xgl_render", func_001B5378);

INCLUDE_ASM("asm/main/nonmatchings/xgl_render", func_001B539C);

INCLUDE_ASM("asm/main/nonmatchings/xgl_render", func_001B53C8);

INCLUDE_ASM("asm/main/nonmatchings/xgl_render", func_001B5624);

INCLUDE_ASM("asm/main/nonmatchings/xgl_render", func_001B5C68);

INCLUDE_ASM("asm/main/nonmatchings/xgl_render", func_001B5E08);

INCLUDE_ASM("asm/main/nonmatchings/xgl_render", func_001B5E40);
