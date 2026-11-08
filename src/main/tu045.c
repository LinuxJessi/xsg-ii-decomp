#include "common.h"
#include "main/tu045.h"
#include "gs.h"

INCLUDE_ASM("asm/main/nonmatchings/tu045", xglVectorAddXYZ_001CCA30);

INCLUDE_ASM("asm/main/nonmatchings/tu045", xglVectorSubXYZ_001CCA68);

#include "shared.h"

typedef struct XglPacket XglPacket;

typedef int s32;

void xglMatrixUnit(void *object);
void func_001D4508(void *object);
extern s32 D_00989A00;
extern s32 D_00989A40;

void func_001CCAA0(void)
{
    xglMatrixUnit(&D_00989A00);
    func_001D4508(&D_00989A40);
}

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001CCAE8);

extern s32 pxyRotateFreeze;

/*
 * Sets the rotation freeze flag (proposed pxySetRotateFreeze): func_001CCFD4 adds no angle step while the flag is
 * nonzero. pxyFlush passes D_0096A2C4.
 */
void pxySetRotateFreeze(s32 freeze)
{
    pxyRotateFreeze = freeze;
}

s32 D_0069E11C = 0x75;

void func_001CCEB4(s32 value)
{
    D_0069E11C = value;
}

s32 pxyFlushDone = 0;

void pxySetFlushDone(void)
{
    pxyFlushDone = 1;
}

void pxyClearFlushDone(void)
{
    pxyFlushDone = 0;
}

extern s32 xglPacketMode3Flag;

void xglPacketSetMode3Flag(s32 value)
{
    xglPacketMode3Flag = value;
}

extern s32 xglPacketMode4Flag;

void xglPacketSetMode4Flag(s32 value)
{
    xglPacketMode4Flag = value;
}

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001CCF84);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001CCFD4);

void *xglStudioSelectGetActiveCamera(s32 index);
extern s32 xglStudioSelectedIndex;
extern void *xglStudioSelectedCamera;

/* Stores the studio index and caches that studio's active camera (xglStudioSelectedCamera). */
void xglStudioSelect(s32 index)
{
    xglStudioSelectedIndex = index;
    xglStudioSelectedCamera = xglStudioSelectGetActiveCamera(index);
}

extern s32 D_0069E0DC;

void func_001CD108(s32 value)
{
    D_0069E0DC = value;
}

extern s32 D_0069E0EC;
extern s32 D_0069E0F0;

void func_001CD134(void)
{
    D_0069E0EC = 0;
    D_0069E0F0 = -1;
}

extern s32 D_0069E0E8;

void func_001CD160(void)
{
    D_0069E0E8 = 0;
}

extern s32 xglPacketMode;

void xglPacketModeReset(void)
{
    xglPacketMode = -1;
}

typedef struct PxyPacket PxyPacket;

/* The model packet whose state pxyUploadPacketState last uploaded (0 after a reset). */
PxyPacket *D_0069E0F4 = 0;

void func_001CD1AC(void)
{
    D_0069E0F4 = 0;
}

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001CD1D0);

void func_001CEC98(void);

void func_001CD204(void)
{
    xglPacketModeReset();
    func_001CD134();
    func_001CD1AC();
    func_001CD160();
    func_001CEC98();
}

/* Scratchpad words that pxyUploadPacketState hands to the packet callees: the current model packet, and a cleared flag. */
#define SPR_CURRENT_PXY (*(PxyPacket **) 0x70000010)
#define SPR_PXY_FLAG (*(s32 *) 0x7000001C)

typedef unsigned int Quadword __attribute__((mode(TI)));

typedef struct PxyHeader {
    u8 unk00[8];
    u32 flags; /* +0x08 */
} PxyHeader;

typedef union PxyQword {
    u64 reg[2];
    Vector4 vec;
} PxyQword;

typedef struct PxyClipPlanes {
    Vector4 plane[2];  /* +0x00 */
    float depth;       /* +0x20 */
    float range;       /* +0x24 */
} PxyClipPlanes;

struct PxyPacket {
    u32 renderStatus;       /* +0x00 */
    u8 unk04[0x10 - 0x4];
    PxyHeader *header;      /* +0x10 */
    u8 unk14[0x70 - 0x14];
    float unk70[4];         /* +0x70 */
    u8 unk80[0x84 - 0x80];
    u32 unk84;              /* +0x84 */
    u8 unk88[0xA4 - 0x88];
    s16 planeCount;         /* +0xA4 */
    u8 unkA6[2];
    PxyClipPlanes *planes;  /* +0xA8 */
    u8 unkAC[0x1A0 - 0xAC];
    float unk1A0[4];        /* +0x1A0 */
    u8 unk1B0[0x1E0 - 0x1B0];
    float unk1E0[8][4];     /* +0x1E0 */
    float unk260[4][4];     /* +0x260 */
};

typedef union GsDisplay64 {
    u64 data;
    struct {
        unsigned int dx : 12;
        unsigned int dy : 11;
        unsigned int magh : 4;
        unsigned int magv : 2;
        unsigned int unmodeled_29 : 3;
        unsigned int dw : 12;
        unsigned int dh : 11;
        unsigned int unmodeled_55 : 9;
    };
} GsDisplay64;

typedef struct DispEnvView {
    u8 unmodeled_00[0x18];
    GsDisplay64 display1; /* +0x18 */
} DispEnvView;

extern DispEnvView DispEnv;
extern s16 cameraScreenWidth;
extern s16 cameraScreenHeight;
extern u16 D_00645828;
s32 D_0069E0F8 = 0;
extern s32 D_0069E0FC;

XglPacket *xglPacketGetCurrent(void);
void sceVif1PkCnt(XglPacket *packet, int count);
void func_0012B6E0(XglPacket *packet, int address, int mode, int a, int b);
void func_0012B770(XglPacket *packet);
void func_0012B9F8(XglPacket *packet, Quadword data);
void func_0012BCE8(XglPacket *packet, void *data, int count);
void func_001CDC40(void);
void func_001CE7C4(s32 mode);
void func_001CE6A4(void);

/* GS TEST_1 with only the depth test enabled (ZTE = 1) at depth-test method `ztst`. */
#define TEST_ZONLY(ztst) GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, ztst)
/* Same, plus the destination alpha test (DATE, DATM = 1). */
#define TEST_ZDEST(date, ztst) GS_SET_TEST(0, 0, 0, 0, date, 1, 1, ztst)

void pxyUploadPacketState(s32 mode, PxyPacket *model)
{
    XglPacket *vif;

    vif = xglPacketGetCurrent();
    D_0069E0F8 = mode;
    D_0069E0FC = 0;
    if (model->renderStatus & 0x20000000) {
        D_0069E0FC = 4;
    } else if (model->renderStatus & 0x10000000) {
        D_0069E0FC = 3;
    } else if (model->renderStatus & 0x1000000) {
        D_0069E0FC = 5;
    } else if (mode & 8) {
        D_0069E0FC = 2;
    } else if (mode & 0x1000) {
        D_0069E0FC = 6;
    }
    sceVif1PkCnt(vif, 0);
    if (D_0069E0F4 != model) {
        if (model->renderStatus & 1) {
            func_0012B6E0(vif, 0x39C, 0x6C, 1, 1);
            func_0012B9F8(vif, *(Quadword *) model->unk1A0);
            func_0012B770(vif);
        }
        if (model->header->flags & 4) {
            func_0012B6E0(vif, 0x370, 0x6C, 1, 1);
            func_0012BCE8(vif, model->unk1E0, 8);
            func_0012B770(vif);
        }
        if (model->header->flags & 0x10) {
            func_0012B6E0(vif, 0x37C, 0x6C, 1, 1);
            func_0012BCE8(vif, model->unk260, 4);
            func_0012B770(vif);
        }
        if (model->renderStatus & 0x80) {
            func_0012B6E0(vif, 0x3DB, 0x6C, 1, 1);
            func_0012B9F8(vif, *(Quadword *) model->unk70);
            func_0012B770(vif);
        }
        {
        AlignedVector4 viewConstants = { 1.0f, 1.0f, 1.0f, 0.0f };
        PxyQword gsWords[6];

        func_0012B6E0(vif, 0x39D, 0x6C, 1, 1);
        func_0012B9F8(vif, *(Quadword *) &viewConstants);
        func_0012B770(vif);
        viewConstants.x = 4096.0f / (cameraScreenWidth + 4.0f);
        viewConstants.y = (4096.0f / (cameraScreenHeight + 4.0f))
                    / (12.272727f / (54.0f / (DispEnv.display1.magh + 1)));
        viewConstants.z = 1.001f;
        func_0012B6E0(vif, 0x3E7, 0x6C, 1, 1);
        func_0012B9F8(vif, *(Quadword *) &viewConstants);
        func_0012B770(vif);
        if (model->planes != 0
            && ((model->renderStatus & 0x2000 && model->renderStatus & 4) || model->renderStatus & 0x4000
                || model->renderStatus & 0x8000)) {
            Vector4 *row;
            Vector4 *plane;

            row = &gsWords[0].vec;
            plane = model->planes->plane;
            row[0].w = model->planes->plane[0].x;
            row[1].w = model->planes->plane[0].y;
            row[2].w = model->planes->plane[0].z;
            row[3].w = model->planes->plane[0].w;
            row[4].w = model->planes->depth;
            row[5].w = model->planes->range;
            row[5].z = 1.0f / model->planes->range;
            if (model->planeCount == 2) {
                plane = &model->planes->plane[1];
            }
            row[0].z = plane->x;
            row[1].z = plane->y;
            row[2].z = plane->z;
            row[3].z = plane->w;
            func_0012B6E0(vif, 0x352, 0x6C, 1, 1);
            func_0012BCE8(vif, gsWords, 6);
            func_0012B770(vif);
        }
        SPR_CURRENT_PXY = model;
        SPR_PXY_FLAG = 0;
        func_001CDC40();
        func_001CE7C4(mode);
        func_001CE6A4();
        {
            s32 date;
            s32 ztst;

            date = 1;
            ztst = 2;
            if (mode & 8) {
                date = 0;
            }
            if (model->unk84 & 0x200) {
                ztst = 1;
            }
            if (model->renderStatus & 0x10000000) {
                gsWords[0].reg[0] = TEST_ZONLY(1);
                gsWords[1].reg[0] = TEST_ZONLY(1);
                gsWords[2].reg[0] = TEST_ZONLY(1);
                gsWords[3].reg[0] = TEST_ZONLY(1);
                gsWords[4].reg[0] = GS_SET_ZBUF(D_00645828, 1, 1);
                gsWords[5].reg[0] = GS_SET_ZBUF(D_00645828, 1, 1);
            } else if (model->renderStatus & 0x1000000) {
                gsWords[0].reg[0] = TEST_ZONLY(ztst);
                gsWords[1].reg[0] = TEST_ZONLY(ztst);
                gsWords[2].reg[0] = TEST_ZONLY(ztst);
                gsWords[3].reg[0] = TEST_ZONLY(ztst);
                gsWords[4].reg[0] = GS_SET_ZBUF(D_00645828, 1, 1);
                gsWords[5].reg[0] = GS_SET_ZBUF(D_00645828, 1, 1);
            } else {
                if (mode & 8) {
                    gsWords[1].reg[0] = GS_SET_TEST(1, 5, D_0069E11C, 0, 0, 0, 1, ztst);
                    gsWords[3].reg[0] = GS_SET_TEST(1, 5, D_0069E11C, 0, date, 1, 1, 1);
                    gsWords[5].reg[0] = GS_SET_ZBUF(D_00645828, 1, 0);
                } else if (mode & 4) {
                    gsWords[1].reg[0] = TEST_ZONLY(ztst);
                    gsWords[3].reg[0] = TEST_ZDEST(date, 1);
                    gsWords[5].reg[0] = GS_SET_ZBUF(D_00645828, 1, 1);
                } else if (D_0069E0DC == 1) {
                    gsWords[1].reg[0] = GS_SET_TEST(1, 2, D_0069E11C, 0, 0, 0, 1, ztst);
                    gsWords[3].reg[0] = GS_SET_TEST(1, 2, D_0069E11C, 0, date, 1, 1, 1);
                    gsWords[5].reg[0] = GS_SET_ZBUF(D_00645828, 1, 1);
                } else {
                    gsWords[1].reg[0] = GS_SET_TEST(1, 5, D_0069E11C, 0, 0, 0, 1, ztst);
                    gsWords[3].reg[0] = GS_SET_TEST(1, 5, D_0069E11C, 0, date, 1, 1, 1);
                    gsWords[5].reg[0] = GS_SET_ZBUF(D_00645828, 1, 0);
                }
                gsWords[0].reg[0] = TEST_ZONLY(ztst);
                gsWords[2].reg[0] = TEST_ZDEST(date, ztst);
                if (D_0069E0DC == 1 && !(D_0069E0F8 & 8)) {
                    gsWords[4].reg[0] = GS_SET_ZBUF(D_00645828, 1, 1);
                } else {
                    gsWords[4].reg[0] = GS_SET_ZBUF(D_00645828, 1, 0);
                }
            }
            gsWords[0].reg[1] = GS_TEST_1;
            gsWords[1].reg[1] = GS_TEST_1;
            gsWords[2].reg[1] = GS_TEST_1;
            gsWords[3].reg[1] = GS_TEST_1;
            gsWords[4].reg[1] = GS_ZBUF_1;
            gsWords[5].reg[1] = GS_ZBUF_1;
            func_0012B6E0(vif, 0x3DC, 0x6C, 1, 1);
            func_0012BCE8(vif, gsWords, 6);
            func_0012B770(vif);
        }
        }
        D_0069E0F4 = model;
    }
    D_0069E11C = 0x75;
}

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001CDC40);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001CE4C4);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001CE6A4);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001CE7C4);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001CEC08);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001CEC98);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001CF468);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001CF520);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001CF644);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001CF8FC);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001CFAA8);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001CFC74);

INCLUDE_ASM("asm/main/nonmatchings/tu045", xglDirectQueueFlush);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001D00D0);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001D0234);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001D04E4);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001D0698);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001D0798);

void func_0012B9A0(XglPacket *packet, u32 word);

/* Sets the current packet's count to 0 (sceVif1PkCnt), then adds VIF FLUSHE with the interrupt bit (0x91000000). */
void xglPacketAddFlushEIrq(void)
{
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    sceVif1PkCnt(packet, 0);
    func_0012B9A0(packet, 0x91000000);
}

/* Sets the current packet's count to 0 (sceVif1PkCnt), then adds VIF FLUSHE (0x11000000). */
void xglPacketAddFlushE(void)
{
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    sceVif1PkCnt(packet, 0);
    func_0012B9A0(packet, 0x11000000);
}

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001D091C);

extern void sceVif1PkRef(XglPacket *packet, const void *environment,
                         int count, int mode, int offset, int flags);

void xglPacketAddRef(const void *environment, int count)
{
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    sceVif1PkRef(packet, environment, count, 0, 0, 0);
}

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001D09D8);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001D0AE0);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001D0D28);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001D1140);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001D15BC);

/* One quadword of the direct GIF queue: an A+D register write (data, then register address). */
typedef struct XglDirectQueueEntry {
    u64 data;
    u64 address;
} XglDirectQueueEntry;

extern XglDirectQueueEntry *xglDirectQueueBuffer;
extern int xglDirectQueueCount;
extern s16 sRenderPsm;
void xglDirectQueueBegin(void);
void func_001D00D0(void);

/* The fields of a model packet (PxyPacket, src/main/nml_model_set.c) that pxyDrawGlow reads. */
typedef struct PxyGlowPacket {
    u32 renderStatus;
    int camera;
    int drawMode; /* +0x08 */
    u8 unmodeled_0c[0x4C - 0x0C];
    float glowScale; /* +0x4c */
    u8 unmodeled_50[0x58 - 0x50];
    float glowIntensity; /* +0x58 */
    u8 unmodeled_5c[0x180 - 0x5C];
    int cameraLevel[4]; /* +0x180: per studio camera, out of 4080 */
} PxyGlowPacket;

/* GS primitive coordinates are 12.4 fixed point around the 2048 origin; Q = 1.0f as raw bits. */
#define GS_ORIGIN 0x800
#define GS_Q_ONE 0x3F800000
/* The glow passes copy the 0xFF-pixel square that starts one pixel after the origin. */
#define GS_ORIGIN_1 (GS_ORIGIN + 1)
#define GS_LAST (GS_ORIGIN + 0xFF)
/* XYZ2 vertex at (x, y) in pixels. */
#define XY(x, y) GS_SET_XYZ((x) << 4, (y) << 4, 0)
/* Half the screen size. */
#define SCREEN_HALF_WIDTH ((s16) (cameraScreenWidth / 2))
#define SCREEN_HALF_HEIGHT ((s16) (cameraScreenHeight / 2))
#define GLOW_LEVEL(packet) \
    ((int) ((packet)->glowIntensity * 128.0f * (packet)->glowScale * \
            (float) (packet)->cameraLevel[xglStudioSelectedIndex] / 4080.0f))

/*
 * Draws the glow of `packet` into the frame buffer at frameBase: a sprite in the glow colour, three
 * passes that copy the buffer onto itself shifted by one texel, and a line-strip border.
 */
void pxyDrawGlow(u32 frameBase, PxyGlowPacket *packet)
{
    int i;
    u64 *p;
    u64 *q;

    xglDirectQueueBegin();
    p = (u64 *) &xglDirectQueueBuffer[xglDirectQueueCount];
    q = p;
    GS_PUT(p, GS_TEST_1, GS_SET_TEST(1, 0, 0, 1, 1, 0, 1, 1));
    if (packet->drawMode == 4) {
        GS_PUT(p, GS_RGBAQ,
               GS_SET_RGBAQ(GLOW_LEVEL(packet), GLOW_LEVEL(packet), GLOW_LEVEL(packet), 0x80, GS_Q_ONE));
    } else {
        GS_PUT(p, GS_RGBAQ, GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80 - GLOW_LEVEL(packet), GS_Q_ONE));
    }
    GS_PUT(p, GS_PRIM, 6);
    GS_PUT(p, GS_XYZF2, XY(GS_ORIGIN - SCREEN_HALF_WIDTH, GS_ORIGIN - SCREEN_HALF_HEIGHT));
    GS_PUT(p, GS_XYZF2, XY(GS_ORIGIN, GS_LAST - SCREEN_HALF_HEIGHT));
    GS_PUT(p, GS_FRAME_1, GS_SET_FRAME(frameBase, (s16) (cameraScreenWidth / 64), sRenderPsm, 0));
    GS_PUT(p, GS_TEX0_1, GS_SET_TEX0(frameBase << 5, 8, 0, 9, 9, 1, 0, 0, 0, 0, 0, 1));
    GS_PUT(p, GS_TEX1_1, GS_SET_TEX1(0, 0, 1, 1, 0, 0, 0));
    GS_PUT(p, GS_TEST_1, GS_SET_TEST(1, 0, 0, 1, 0, 0, 1, 1));
    GS_PUT(p, GS_CLAMP_1, 0);
    GS_PUT(p, GS_RGBAQ, GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, GS_Q_ONE));
    xglDirectQueueCount += ((u32) p - (u32) q) >> 4;
    func_001D00D0();

    for (i = 0; i < 3; i++) {
        xglDirectQueueBegin();
        q = (u64 *) &xglDirectQueueBuffer[xglDirectQueueCount];
        p = q;
        GS_PUT(q, GS_TEXFLUSH, 0);
        GS_PUT(q, GS_PRIM, GS_SET_PRIM(6, 0, 1, 0, 0, 0, 1, 0, 0));
        GS_PUT(q, GS_UV, GS_SET_UV(0x10, 0x10));
        GS_PUT(q, GS_XYZF2, XY(GS_ORIGIN_1, GS_ORIGIN_1 - SCREEN_HALF_HEIGHT));
        GS_PUT(q, GS_UV, GS_SET_UV(0xFF0, 0xFF0));
        GS_PUT(q, GS_XYZF2, XY(GS_LAST, GS_LAST - SCREEN_HALF_HEIGHT));
        GS_PUT(q, GS_TEXFLUSH, 0);
        GS_PUT(q, GS_PRIM, GS_SET_PRIM(6, 0, 1, 0, 0, 0, 1, 0, 0));
        GS_PUT(q, GS_UV, GS_SET_UV((SCREEN_HALF_WIDTH + 1) << 4, 0x10));
        GS_PUT(q, GS_XYZF2, XY(GS_ORIGIN_1 - SCREEN_HALF_WIDTH, GS_ORIGIN_1 - SCREEN_HALF_HEIGHT));
        GS_PUT(q, GS_UV, GS_SET_UV((SCREEN_HALF_WIDTH + 0xFF) << 4, 0xFF0));
        GS_PUT(q, GS_XYZF2, XY(GS_LAST - SCREEN_HALF_WIDTH, GS_LAST - SCREEN_HALF_HEIGHT));
        xglDirectQueueCount += ((u32) q - (u32) p) >> 4;
        func_001D00D0();
    }

    xglDirectQueueBegin();
    q = (u64 *) &xglDirectQueueBuffer[xglDirectQueueCount];
    p = q;
    GS_PUT(q, GS_TEXFLUSH, 0);
    GS_PUT(q, GS_TEST_1, GS_SET_TEST(1, 0, 0, 1, 0, 0, 1, 1));
    GS_PUT(q, GS_RGBAQ, GS_SET_RGBAQ(0, 0, 0, 0x80, GS_Q_ONE));
    GS_PUT(q, GS_PRIM, 2);
    GS_PUT(q, GS_XYZF2, XY(GS_LAST - SCREEN_HALF_WIDTH, GS_ORIGIN - SCREEN_HALF_HEIGHT));
    GS_PUT(q, GS_XYZF2, XY(GS_ORIGIN - SCREEN_HALF_WIDTH, GS_ORIGIN - SCREEN_HALF_HEIGHT));
    GS_PUT(q, GS_XYZF2, XY(GS_ORIGIN - SCREEN_HALF_WIDTH, GS_LAST - SCREEN_HALF_HEIGHT));
    GS_PUT(q, GS_XYZF2, XY(GS_LAST - SCREEN_HALF_WIDTH, GS_LAST - SCREEN_HALF_HEIGHT));
    GS_PUT(q, GS_XYZF2, XY(GS_LAST - SCREEN_HALF_WIDTH, GS_ORIGIN - SCREEN_HALF_HEIGHT));
    xglDirectQueueCount += ((u32) q - (u32) p) >> 4;
    func_001D00D0();
}

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001D2678);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001D27D4);

INCLUDE_ASM("asm/main/nonmatchings/tu045", xglPacketAppendData);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001D29AC);

INCLUDE_ASM("asm/main/nonmatchings/tu045", xglPacketAppendMatrices);

INCLUDE_ASM("asm/main/nonmatchings/tu045", xglPacketAppendVectors);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001D2B20);

/*
 * Rounds a requested size up to the next power of two: the floor is 2 and
 * any size above 0x800 returns 1.
 */
int xglRoundUpPow2(int size)
{
    size = size;
    if (size < 3)
    {
        return 2;
    }
    if (size < 5)
    {
        return 4;
    }
    if (size < 9)
    {
        return 8;
    }
    if (size < 0x11)
    {
        return 0x10;
    }
    if (size < 0x21)
    {
        return 0x20;
    }
    if (size < 0x41)
    {
        return 0x40;
    }
    if (size < 0x81)
    {
        return 0x80;
    }
    if (size < 0x101)
    {
        return 0x100;
    }
    if (size < 0x201)
    {
        return 0x200;
    }
    if (size < 0x401)
    {
        return 0x400;
    }
    if (size < 0x801)
    {
        return 0x800;
    }
    return 1;
}

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001D2D34);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001D2E90);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001D300C);

INCLUDE_ASM("asm/main/nonmatchings/tu045", xglDirectQueueBegin);

INCLUDE_ASM("asm/main/nonmatchings/tu045", xglDirectQueuePush);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001D32A4);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001D32FC);

extern s32 xglDirectQueueRefOffset;

void xglDirectQueueSetRefOffset(s32 value)
{
    xglDirectQueueRefOffset = value;
}

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001D33D8);

extern s32 pxySubmitCount;

void pxyResetSubmitCount(void)
{
    pxySubmitCount = 0;
}

s32 pxyGetSubmitCount(void)
{
    return pxySubmitCount;
}

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001D34C4);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001D35FC);

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001D3744);
