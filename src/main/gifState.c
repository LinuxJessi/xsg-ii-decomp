#include "common.h"
#include "shared.h"
#include "gs.h"
#include "quadword.h"
#include "main/gifState.h"
#include "main/nml_model_set.h"
#include "main/TiWater.h"
#include "main/tu148.h"
#include "main/xgl_2.h"
#include "main/xgl_render.h"

void nmlModelUseSubWindow(unsigned int index, int enabled);

#define NML_WINDOW_COUNT 4

extern int g_aSubWindow[NML_WINDOW_COUNT];

INCLUDE_ASM("asm/main/nonmatchings/gifState", gifStateSetupContext);

INCLUDE_ASM("asm/main/nonmatchings/gifState", gifStateDrawRing);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_002424D8);

INCLUDE_ASM("asm/main/nonmatchings/gifState", gifStateDrawCameraQuad);

INCLUDE_ASM("asm/main/nonmatchings/gifState", gifStateDrawJitterGrid);

INCLUDE_ASM("asm/main/nonmatchings/gifState", gifStateDrawFadeQuad);

void func_002439C8(void) {
}

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_002439D0);

typedef struct XglPacket XglPacket;

void gifStateReset(void);
void gifStateFlush(void);
void sceVif1PkCloseDirectHLCode(XglPacket *packet);
void sceVif1PkCnt(XglPacket *packet, int count);
void sceVif1PkOpenDirectHLCode(XglPacket *packet, int count);

void func_00243B58(XglPacket *packet) {
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    gifStateReset();
    gifStateFlush();
    sceVif1PkCloseDirectHLCode(packet);
}

INCLUDE_ASM("asm/main/nonmatchings/gifState", gifStateDrawQuad);

/* The halfword fields of sRender read by the frame effect. */
typedef struct GifStateRender {
    unsigned char unmodeled_00[0x14];
    unsigned short displayBase;             /* 0x14: frame page shown, also the texture source */
    unsigned char unmodeled_16[4];
    unsigned short workBase;                /* 0x1A: frame page of the work buffer */
    unsigned char unmodeled_1c[4];
    unsigned short drawBase;                /* 0x20: frame page drawn this frame */
    unsigned short blendBase;               /* 0x22: frame page the effect blends back over the drawn one */
} GifStateRender;

extern GifStateRender sRender;
extern int gifStateOriginX;
extern int gifStateOriginY;
extern int gifStateBlurDivisor;                      /* blur divisor of the alpha (3) */
extern int gifStateBlurOffset;                      /* blur offset in pixels */

void gifStateAddReg(int gifAddress, u64 gifData);
void gifStateDrawQuad(XglPacket *packet, int width, int height, int offsetX, int offsetY, int alpha);

/* IEEE-754 1.0f as the raw Q word of RGBAQ. */
#define GIF_STATE_ONE_BITS 0x3F800000

/* The 512x448 frame buffer in pixels. */
#define GIF_STATE_WIDTH 512
#define GIF_STATE_HEIGHT 448

/* XYZ2 of a sprite corner at (x, y) pixels from the GS origin. */
#define GIF_STATE_XY(x, y) GS_SET_XYZ(((gifStateOriginX + (x)) << 4) - 8, ((gifStateOriginY + (y)) << 4) - 9, 0)

/* FRAME_1 of a 512-pixel-wide page with the alpha channel masked. */
#define GIF_STATE_FRAME(base) GS_SET_FRAME(base, 8, GS_PSMCT32, 0xFF000000)

/* TEX0_1 of a 512x512 PSMCT32 page used as a texture. */
#define GIF_STATE_TEX0(base) GS_SET_TEX0((base) << 5, 8, GS_PSMCT32, 9, 9, 1, 0, 0, 0, 0, 0, 0)

void gifStateDrawBlurFrame(XglPacket *packet)
{
    int alpha = 0x28 / gifStateBlurDivisor;
    float alphaScale = alpha;
    int halfWidth = 256 / 2;
    int halfHeight = 224 / 2;

    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    gifStateReset();
    gifStateAddReg(GS_TEXFLUSH, 0);
    gifStateAddReg(GS_CLAMP_1, GS_SET_CLAMP(1, 1, 0, 0, 0, 0));
    gifStateAddReg(GS_ZBUF_1, GS_SET_ZBUF(0, 0x31, 1));
    gifStateAddReg(GS_FBA_1, 1);
    gifStateAddReg(GS_TEST_1, GS_SET_TEST(0, 0, 0, 0, 0, 0, 0, 1));
    gifStateAddReg(GS_ALPHA_1, GS_SET_ALPHA(0, 2, 2, 1, 0x7F));
    gifStateAddReg(GS_TEX1_1, GS_SET_TEX1(0, 0, 1, 1, 0, 0, 0));
    gifStateAddReg(GS_FRAME_1, GIF_STATE_FRAME(sRender.drawBase));
    gifStateAddReg(GS_TEX0_1, GIF_STATE_TEX0(sRender.displayBase));
    gifStateAddReg(GS_RGBAQ, GS_SET_RGBAQ(0x7F, 0x7F, 0x7F, 0x7F, GIF_STATE_ONE_BITS));
    gifStateAddReg(GS_PRIM, GS_SET_PRIM(6, 0, 1, 0, 1, 0, 1, 0, 0));
    gifStateAddReg(GS_UV, GS_SET_UV(0, 0));
    gifStateAddReg(GS_XYZF2, GIF_STATE_XY(0, 0));
    gifStateAddReg(GS_UV, GS_SET_UV(GIF_STATE_WIDTH << 4, GIF_STATE_HEIGHT << 4));
    gifStateAddReg(GS_XYZF2, GIF_STATE_XY(GIF_STATE_WIDTH, GIF_STATE_HEIGHT));
    gifStateFlush();
    sceVif1PkCloseDirectHLCode(packet);

    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    gifStateReset();
    gifStateAddReg(GS_TEXFLUSH, 0);
    gifStateAddReg(GS_FRAME_1, GIF_STATE_FRAME(sRender.workBase));
    gifStateAddReg(GS_TEX0_1, GIF_STATE_TEX0(sRender.displayBase));
    gifStateAddReg(GS_RGBAQ, GS_SET_RGBAQ(0x7F, 0x7F, 0x7F, 0x7F, GIF_STATE_ONE_BITS));
    gifStateAddReg(GS_PRIM, GS_SET_PRIM(6, 0, 1, 0, 0, 0, 1, 0, 0));
    gifStateAddReg(GS_UV, GS_SET_UV(0, 0));
    gifStateAddReg(GS_XYZF2, GIF_STATE_XY(0, 0));
    gifStateAddReg(GS_UV, GS_SET_UV(GIF_STATE_WIDTH << 4, GIF_STATE_HEIGHT << 4));
    gifStateAddReg(GS_XYZF2, GIF_STATE_XY(256, 224));
    gifStateFlush();
    sceVif1PkCloseDirectHLCode(packet);

    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    gifStateReset();
    gifStateAddReg(GS_ALPHA_1, GS_SET_ALPHA(0, 2, 2, 1, 0x28 / gifStateBlurDivisor));
    gifStateAddReg(GS_FRAME_1, GIF_STATE_FRAME(sRender.displayBase));
    gifStateAddReg(GS_TEX0_1, GIF_STATE_TEX0(sRender.workBase));
    gifStateAddReg(GS_PRIM, GS_SET_PRIM(6, 0, 0, 0, 0, 0, 0, 0, 0));
    gifStateAddReg(GS_RGBAQ, GS_SET_RGBAQ(0, 0, 0, 0, GIF_STATE_ONE_BITS));
    gifStateAddReg(GS_XYZF2, GIF_STATE_XY(0, 0));
    gifStateAddReg(GS_XYZF2, GIF_STATE_XY(276, 244));
    gifStateFlush();
    sceVif1PkCloseDirectHLCode(packet);

    gifStateDrawQuad(packet, 256, 224, gifStateBlurOffset, 0, alpha);
    gifStateDrawQuad(packet, 256, 224, -gifStateBlurOffset, 0, alpha);
    gifStateDrawQuad(packet, 256, 224, 0, gifStateBlurOffset, alpha);
    gifStateDrawQuad(packet, 256, 224, 0, -gifStateBlurOffset, alpha);
    gifStateDrawQuad(packet, 256, 224, gifStateBlurOffset, gifStateBlurOffset, alpha);
    gifStateDrawQuad(packet, 256, 224, -gifStateBlurOffset, gifStateBlurOffset, alpha);
    gifStateDrawQuad(packet, 256, 224, gifStateBlurOffset, -gifStateBlurOffset, alpha);
    gifStateDrawQuad(packet, 256, 224, -gifStateBlurOffset, -gifStateBlurOffset, alpha);

    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    gifStateReset();
    gifStateAddReg(GS_FRAME_1, GIF_STATE_FRAME(sRender.workBase));
    gifStateAddReg(GS_PRIM, GS_SET_PRIM(6, 0, 0, 0, 0, 0, 0, 0, 0));
    gifStateAddReg(GS_RGBAQ, GS_SET_RGBAQ(0, 0, 0, 0, GIF_STATE_ONE_BITS));
    gifStateAddReg(GS_XYZF2, GIF_STATE_XY(0, 0));
    gifStateAddReg(GS_XYZF2, GIF_STATE_XY(256, 224));
    gifStateFlush();
    sceVif1PkCloseDirectHLCode(packet);

    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    gifStateReset();
    gifStateAddReg(GS_TEXFLUSH, 0);
    gifStateAddReg(GS_ALPHA_1, GS_SET_ALPHA(0, 2, 2, 1, 0x80));
    gifStateAddReg(GS_FRAME_1, GIF_STATE_FRAME(sRender.workBase));
    gifStateAddReg(GS_TEX0_1, GIF_STATE_TEX0(sRender.displayBase));
    gifStateAddReg(GS_RGBAQ, GS_SET_RGBAQ(0x7F, 0x7F, 0x7F, 0x7F, GIF_STATE_ONE_BITS));
    gifStateAddReg(GS_PRIM, GS_SET_PRIM(6, 0, 1, 0, 0, 0, 1, 0, 0));
    gifStateAddReg(GS_UV, GS_SET_UV(0, 0));
    gifStateAddReg(GS_XYZF2, GIF_STATE_XY(0, 0));
    gifStateAddReg(GS_UV, GS_SET_UV(256 << 4, 224 << 4));
    gifStateAddReg(GS_XYZF2, GIF_STATE_XY(halfWidth, halfHeight));
    gifStateAddReg(GS_ALPHA_1, GS_SET_ALPHA(0, 2, 2, 1, 0x30 / gifStateBlurDivisor));
    gifStateFlush();
    sceVif1PkCloseDirectHLCode(packet);

    gifStateDrawQuad(packet, 128, 112, gifStateBlurOffset * 2, 0, (int) (alphaScale * 1.5f));
    gifStateDrawQuad(packet, 128, 112, -gifStateBlurOffset * 2, 0, (int) (alphaScale * 1.5f));
    gifStateDrawQuad(packet, 128, 112, 0, gifStateBlurOffset * 2, (int) (alphaScale * 1.5f));
    gifStateDrawQuad(packet, 128, 112, 0, -gifStateBlurOffset * 2, (int) (alphaScale * 1.5f));
    gifStateDrawQuad(packet, 128, 112, gifStateBlurOffset * 2, gifStateBlurOffset * 2, (int) (alphaScale * 1.5f));
    gifStateDrawQuad(packet, 128, 112, -gifStateBlurOffset * 2, gifStateBlurOffset * 2, (int) (alphaScale * 1.5f));
    gifStateDrawQuad(packet, 128, 112, gifStateBlurOffset * 2, -gifStateBlurOffset * 2, (int) (alphaScale * 1.5f));
    gifStateDrawQuad(packet, 128, 112, -gifStateBlurOffset * 2, -gifStateBlurOffset * 2, (int) (alphaScale * 1.5f));

    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    gifStateReset();
    gifStateAddReg(GS_FRAME_1, GIF_STATE_FRAME(sRender.workBase));
    gifStateAddReg(GS_PRIM, GS_SET_PRIM(6, 0, 0, 0, 0, 0, 0, 0, 0));
    gifStateAddReg(GS_RGBAQ, GS_SET_RGBAQ(0, 0, 0, 0, GIF_STATE_ONE_BITS));
    gifStateAddReg(GS_XYZF2, GIF_STATE_XY(0, 0));
    gifStateAddReg(GS_XYZF2, GIF_STATE_XY(halfWidth, halfHeight));
    gifStateFlush();
    sceVif1PkCloseDirectHLCode(packet);

    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    gifStateReset();
    gifStateAddReg(GS_TEXFLUSH, 0);
    gifStateAddReg(GS_ALPHA_1, GS_SET_ALPHA(0, 2, 2, 1, 0x80));
    gifStateAddReg(GS_FRAME_1, GIF_STATE_FRAME(sRender.workBase));
    gifStateAddReg(GS_TEX0_1, GIF_STATE_TEX0(sRender.displayBase));
    gifStateAddReg(GS_RGBAQ, GS_SET_RGBAQ(0x7F, 0x7F, 0x7F, 0x7F, GIF_STATE_ONE_BITS));
    gifStateAddReg(GS_PRIM, GS_SET_PRIM(6, 0, 1, 0, 0, 0, 1, 0, 0));
    gifStateAddReg(GS_UV, GS_SET_UV(0, 0));
    gifStateAddReg(GS_XYZF2, GIF_STATE_XY(0, 0));
    gifStateAddReg(GS_UV, GS_SET_UV(256 << 4, 224 << 4));
    gifStateAddReg(GS_XYZF2, GIF_STATE_XY(85, 74));
    gifStateAddReg(GS_ALPHA_1, GS_SET_ALPHA(0, 2, 2, 1, 0x30 / gifStateBlurDivisor));
    gifStateFlush();
    sceVif1PkCloseDirectHLCode(packet);

    gifStateDrawQuad(packet, 85, 74, gifStateBlurOffset * 2, gifStateBlurOffset * 2, (int) (alphaScale * 2.0f));
    gifStateDrawQuad(packet, 85, 74, -gifStateBlurOffset * 2, gifStateBlurOffset * 2, (int) (alphaScale * 2.0f));
    gifStateDrawQuad(packet, 85, 74, gifStateBlurOffset * 2, -gifStateBlurOffset * 2, (int) (alphaScale * 2.0f));
    gifStateDrawQuad(packet, 85, 74, -gifStateBlurOffset * 2, -gifStateBlurOffset * 2, (int) (alphaScale * 2.0f));

    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    gifStateReset();
    gifStateAddReg(GS_ALPHA_1, GS_SET_ALPHA(0, 2, 2, 1, 0x7F));
    gifStateAddReg(GS_FRAME_1, GIF_STATE_FRAME(sRender.drawBase));
    gifStateAddReg(GS_TEX0_1, GIF_STATE_TEX0(sRender.displayBase));
    gifStateAddReg(GS_RGBAQ, GS_SET_RGBAQ(0x7F, 0x7F, 0x7F, 0x7F, GIF_STATE_ONE_BITS));
    gifStateAddReg(GS_PRIM, GS_SET_PRIM(6, 0, 1, 0, 1, 0, 1, 0, 0));
    gifStateAddReg(GS_UV, GS_SET_UV(0, 0));
    gifStateAddReg(GS_XYZF2, GIF_STATE_XY(0, 0));
    gifStateAddReg(GS_UV, GS_SET_UV(256 << 4, 224 << 4));
    gifStateAddReg(GS_XYZF2, GIF_STATE_XY(GIF_STATE_WIDTH, GIF_STATE_HEIGHT));
    gifStateAddReg(GS_FRAME_1, GS_SET_FRAME(sRender.drawBase, 8, GS_PSMCT32, 0));
    gifStateFlush();
    sceVif1PkCloseDirectHLCode(packet);
}


/* A screen position of the break effect, in pixels. */
typedef struct GifBreakXY {
    f32 x;
    f32 y;
} GifBreakXY;

#define GIF_SCREEN_RIGHT 511.0f
#define GIF_SCREEN_BOTTOM 447.0f

#define GIF_SET_POINT(point, px, py) ((point)[0].f = (px), (point)[1].f = (py))
#define GIF_SET_VERTEX(vertex, source) ((vertex)[0].f = (source)[0].f, (vertex)[1].f = (source)[1].f)

/* Breaks the screen into 24 shards around the impact triangle `points`: the triangle's top,
 * left, bottom and right corners are joined to the screen corners, giving eight triangles, and
 * each of them is split at its centroid into three shards. */
void gifStateBuildBreakShards(GifBreakShard *shards, GifBreakPoint *points)
{
    GifQuad wedge[8][3];
    GifBreakXY center;
    int i;
    int top;
    int left;
    int bottom;
    int right;
    int otherPoint;
    f32 angle;

    top = 0;
    otherPoint = 0;
    for (i = 0; i < 3; i++) {
        if (points[i].y < points[top].y) {
            top = i;
        }
    }
    left = 0;
    for (i = 0; i < 3; i++) {
        if (points[i].x < points[left].x) {
            left = i;
        }
    }
    bottom = 0;
    for (i = 0; i < 3; i++) {
        if (points[bottom].y < points[i].y) {
            bottom = i;
        }
    }
    right = 0;
    for (i = 0; i < 3; i++) {
        if (points[right].x < points[i].x) {
            right = i;
        }
    }

    if ((top == left && bottom == right) || (top == right && bottom == left)) {
        if ((top == 0 && bottom == 1) || (top == 1 && bottom == 0)) {
            otherPoint = 2;
        }
        if ((top == 1 && bottom == 2) || (top == 2 && bottom == 1)) {
            otherPoint = 0;
        }
        if ((top == 2 && bottom == 0) || (top == 0 && bottom == 2)) {
            otherPoint = 1;
        }
    }

    GIF_SET_POINT(wedge[0][0], points[top].x, points[top].y);
    GIF_SET_POINT(wedge[0][1], 0.0f, 0.0f);
    GIF_SET_POINT(wedge[0][2], GIF_SCREEN_RIGHT, 0.0f);
    GIF_SET_POINT(wedge[1][0], points[left].x, points[left].y);
    GIF_SET_POINT(wedge[1][1], 0.0f, 0.0f);
    GIF_SET_POINT(wedge[1][2], 0.0f, GIF_SCREEN_BOTTOM);
    GIF_SET_POINT(wedge[2][0], points[bottom].x, points[bottom].y);
    GIF_SET_POINT(wedge[2][1], 0.0f, GIF_SCREEN_BOTTOM);
    GIF_SET_POINT(wedge[2][2], GIF_SCREEN_RIGHT, GIF_SCREEN_BOTTOM);
    GIF_SET_POINT(wedge[3][0], points[right].x, points[right].y);
    GIF_SET_POINT(wedge[3][1], GIF_SCREEN_RIGHT, 0.0f);
    GIF_SET_POINT(wedge[3][2], GIF_SCREEN_RIGHT, GIF_SCREEN_BOTTOM);

    if (left == top && right != bottom) {
        GIF_SET_POINT(wedge[4][0], points[right].x, points[right].y);
        GIF_SET_POINT(wedge[4][1], points[top].x, points[top].y);
        GIF_SET_POINT(wedge[4][2], GIF_SCREEN_RIGHT, 0.0f);
        GIF_SET_POINT(wedge[5][0], points[bottom].x, points[bottom].y);
        GIF_SET_POINT(wedge[5][1], points[top].x, points[top].y);
        GIF_SET_POINT(wedge[5][2], 0.0f, GIF_SCREEN_BOTTOM);
        GIF_SET_POINT(wedge[6][0], points[right].x, points[right].y);
        GIF_SET_POINT(wedge[6][1], points[bottom].x, points[bottom].y);
        GIF_SET_POINT(wedge[6][2], GIF_SCREEN_RIGHT, GIF_SCREEN_BOTTOM);
    } else if (left == bottom && right != top) {
        GIF_SET_POINT(wedge[4][0], points[top].x, points[top].y);
        GIF_SET_POINT(wedge[4][1], points[bottom].x, points[bottom].y);
        GIF_SET_POINT(wedge[4][2], 0.0f, 0.0f);
        GIF_SET_POINT(wedge[5][0], points[bottom].x, points[bottom].y);
        GIF_SET_POINT(wedge[5][1], points[right].x, points[right].y);
        GIF_SET_POINT(wedge[5][2], GIF_SCREEN_RIGHT, GIF_SCREEN_BOTTOM);
        GIF_SET_POINT(wedge[6][0], points[right].x, points[right].y);
        GIF_SET_POINT(wedge[6][1], points[top].x, points[top].y);
        GIF_SET_POINT(wedge[6][2], GIF_SCREEN_RIGHT, 0.0f);
    } else if (left == top && right == bottom) {
        angle = tiWaterVectorAngle(points[left].x - points[bottom].x, points[left].y - points[bottom].y);
        if (tiWaterVectorAngle(points[otherPoint].x - points[bottom].x, points[otherPoint].y - points[bottom].y) < angle) {
            GIF_SET_POINT(wedge[4][0], points[bottom].x, points[bottom].y);
            GIF_SET_POINT(wedge[4][1], points[left].x, points[left].y);
            GIF_SET_POINT(wedge[4][2], GIF_SCREEN_RIGHT, 0.0f);
            GIF_SET_POINT(wedge[5][0], points[left].x, points[left].y);
            GIF_SET_POINT(wedge[5][1], points[otherPoint].x, points[otherPoint].y);
            GIF_SET_POINT(wedge[5][2], 0.0f, GIF_SCREEN_BOTTOM);
            GIF_SET_POINT(wedge[6][0], points[otherPoint].x, points[otherPoint].y);
            GIF_SET_POINT(wedge[6][1], points[bottom].x, points[bottom].y);
            GIF_SET_POINT(wedge[6][2], 0.0f, GIF_SCREEN_BOTTOM);
        } else {
            GIF_SET_POINT(wedge[4][0], points[bottom].x, points[bottom].y);
            GIF_SET_POINT(wedge[4][1], points[otherPoint].x, points[otherPoint].y);
            GIF_SET_POINT(wedge[4][2], GIF_SCREEN_RIGHT, 0.0f);
            GIF_SET_POINT(wedge[5][0], points[otherPoint].x, points[otherPoint].y);
            GIF_SET_POINT(wedge[5][1], points[left].x, points[left].y);
            GIF_SET_POINT(wedge[5][2], GIF_SCREEN_RIGHT, 0.0f);
            GIF_SET_POINT(wedge[6][0], points[left].x, points[left].y);
            GIF_SET_POINT(wedge[6][1], points[bottom].x, points[bottom].y);
            GIF_SET_POINT(wedge[6][2], 0.0f, GIF_SCREEN_BOTTOM);
        }
    } else if (left == bottom && right == top) {
        angle = tiWaterVectorAngle(points[right].x - points[bottom].x, points[right].y - points[bottom].y);
        if (angle < tiWaterVectorAngle(points[otherPoint].x - points[bottom].x, points[otherPoint].y - points[bottom].y)) {
            GIF_SET_POINT(wedge[4][0], points[right].x, points[right].y);
            GIF_SET_POINT(wedge[4][1], points[bottom].x, points[bottom].y);
            GIF_SET_POINT(wedge[4][2], 0.0f, 0.0f);
            GIF_SET_POINT(wedge[5][0], points[bottom].x, points[bottom].y);
            GIF_SET_POINT(wedge[5][1], points[otherPoint].x, points[otherPoint].y);
            GIF_SET_POINT(wedge[5][2], GIF_SCREEN_RIGHT, GIF_SCREEN_BOTTOM);
            GIF_SET_POINT(wedge[6][0], points[otherPoint].x, points[otherPoint].y);
            GIF_SET_POINT(wedge[6][1], points[right].x, points[right].y);
            GIF_SET_POINT(wedge[6][2], GIF_SCREEN_RIGHT, GIF_SCREEN_BOTTOM);
        } else {
            GIF_SET_POINT(wedge[4][0], points[right].x, points[right].y);
            GIF_SET_POINT(wedge[4][1], points[otherPoint].x, points[otherPoint].y);
            GIF_SET_POINT(wedge[4][2], 0.0f, 0.0f);
            GIF_SET_POINT(wedge[5][0], points[otherPoint].x, points[otherPoint].y);
            GIF_SET_POINT(wedge[5][1], points[bottom].x, points[bottom].y);
            GIF_SET_POINT(wedge[5][2], 0.0f, 0.0f);
            GIF_SET_POINT(wedge[6][0], points[bottom].x, points[bottom].y);
            GIF_SET_POINT(wedge[6][1], points[right].x, points[right].y);
            GIF_SET_POINT(wedge[6][2], GIF_SCREEN_RIGHT, GIF_SCREEN_BOTTOM);
        }
    } else if (right == bottom) {
        GIF_SET_POINT(wedge[4][0], points[bottom].x, points[bottom].y);
        GIF_SET_POINT(wedge[4][1], points[top].x, points[top].y);
        GIF_SET_POINT(wedge[4][2], GIF_SCREEN_RIGHT, 0.0f);
        GIF_SET_POINT(wedge[5][0], points[top].x, points[top].y);
        GIF_SET_POINT(wedge[5][1], points[left].x, points[left].y);
        GIF_SET_POINT(wedge[5][2], 0.0f, 0.0f);
        GIF_SET_POINT(wedge[6][0], points[left].x, points[left].y);
        GIF_SET_POINT(wedge[6][1], points[bottom].x, points[bottom].y);
        GIF_SET_POINT(wedge[6][2], 0.0f, GIF_SCREEN_BOTTOM);
    } else if (right == top) {
        GIF_SET_POINT(wedge[4][0], points[right].x, points[right].y);
        GIF_SET_POINT(wedge[4][1], points[left].x, points[left].y);
        GIF_SET_POINT(wedge[4][2], 0.0f, 0.0f);
        GIF_SET_POINT(wedge[5][0], points[bottom].x, points[bottom].y);
        GIF_SET_POINT(wedge[5][1], points[left].x, points[left].y);
        GIF_SET_POINT(wedge[5][2], 0.0f, GIF_SCREEN_BOTTOM);
        GIF_SET_POINT(wedge[6][0], points[right].x, points[right].y);
        GIF_SET_POINT(wedge[6][1], points[bottom].x, points[bottom].y);
        GIF_SET_POINT(wedge[6][2], GIF_SCREEN_RIGHT, GIF_SCREEN_BOTTOM);
    }

    GIF_SET_POINT(wedge[7][0], points[0].x, points[0].y);
    GIF_SET_POINT(wedge[7][1], points[1].x, points[1].y);
    GIF_SET_POINT(wedge[7][2], points[2].x, points[2].y);

    for (i = 0; i < 8; i++) {
        center = (GifBreakXY) {
            (wedge[i][0][0].f + wedge[i][1][0].f + wedge[i][2][0].f) / 3.0f,
            (wedge[i][0][1].f + wedge[i][1][1].f + wedge[i][2][1].f) / 3.0f
        };
        GIF_SET_POINT(shards[i * 3].vertex[0], center.x, center.y);
        GIF_SET_VERTEX(shards[i * 3].vertex[1], wedge[i][0]);
        GIF_SET_VERTEX(shards[i * 3].vertex[2], wedge[i][1]);
        GIF_SET_POINT(shards[i * 3 + 1].vertex[0], center.x, center.y);
        GIF_SET_VERTEX(shards[i * 3 + 1].vertex[1], wedge[i][1]);
        GIF_SET_VERTEX(shards[i * 3 + 1].vertex[2], wedge[i][2]);
        GIF_SET_POINT(shards[i * 3 + 2].vertex[0], center.x, center.y);
        GIF_SET_VERTEX(shards[i * 3 + 2].vertex[1], wedge[i][2]);
        GIF_SET_VERTEX(shards[i * 3 + 2].vertex[2], wedge[i][0]);
    }

    for (i = 0; i < 24; i++) {
        shards[i].center[0].f = (shards[i].vertex[0][0].f + shards[i].vertex[1][0].f + shards[i].vertex[2][0].f) / 3.0f;
        shards[i].center[1].f = (shards[i].vertex[0][1].f + shards[i].vertex[1][1].f + shards[i].vertex[2][1].f) / 3.0f;
        shards[i].center[2].f = 0.0f;
        shards[i].center[3].f = 1.0f;
        shards[i].vertex[0][2].f = 0.0f;
        shards[i].vertex[0][3].f = 1.0f;
        shards[i].vertex[1][2].f = 0.0f;
        shards[i].vertex[1][3].f = 1.0f;
        shards[i].vertex[2][2].f = 0.0f;
        shards[i].vertex[2][3].f = 1.0f;
        shards[i].vertex[0][0].f = (int) (shards[i].vertex[0][0].f - shards[i].center[0].f);
        shards[i].vertex[0][1].f = (int) (shards[i].vertex[0][1].f - shards[i].center[1].f);
        shards[i].vertex[1][0].f = (int) (shards[i].vertex[1][0].f - shards[i].center[0].f);
        shards[i].vertex[1][1].f = (int) (shards[i].vertex[1][1].f - shards[i].center[1].f);
        shards[i].vertex[2][0].f = (int) (shards[i].vertex[2][0].f - shards[i].center[0].f);
        shards[i].vertex[2][1].f = (int) (shards[i].vertex[2][1].f - shards[i].center[1].f);
        {
            int sign[2] = { 1, -1 };

            shards[i].spin[0].f = (xglSRand() % 20 + 30) * sign[xglSRand() & 1] * 3.141592 / 180.0;
            shards[i].spin[1].f = (xglSRand() % 20 + 30) * sign[xglSRand() & 1] * 3.141592 / 180.0;
        }
        shards[i].spin[2].f = 0.0f;
        shards[i].spin[3].f = 1.0f;
        if (shards[i].center[0].f >= 256.0f) {
            shards[i].speed.f = ((512.0f - shards[i].center[0].f) / 256.0f * 30.0f + 5.0f) * 0.01f;
        } else {
            shards[i].speed.f = (shards[i].center[0].f / 256.0f * 30.0f + 5.0f) * 0.01f;
        }
        shards[i].fall.f = 0.0f;
    }

    for (i = 0; i < 24; i++) {
        QuadwordCopy(shards[i].vertex[3], shards[i].vertex[0]);
        QuadwordCopy(shards[i].vertex[4], shards[i].vertex[1]);
        QuadwordCopy(shards[i].vertex[5], shards[i].vertex[2]);
        shards[i].vertex[3][2].f -= 10.0f;
        shards[i].vertex[4][2].f -= 10.0f;
        shards[i].vertex[5][2].f -= 10.0f;
        QuadwordCopy(shards[i].vertex[6], shards[i].vertex[0]);
        QuadwordCopy(shards[i].vertex[7], shards[i].vertex[3]);
        QuadwordCopy(shards[i].vertex[8], shards[i].vertex[4]);
        QuadwordCopy(shards[i].vertex[9], shards[i].vertex[0]);
        QuadwordCopy(shards[i].vertex[10], shards[i].vertex[4]);
        QuadwordCopy(shards[i].vertex[11], shards[i].vertex[1]);
        QuadwordCopy(shards[i].vertex[12], shards[i].vertex[0]);
        QuadwordCopy(shards[i].vertex[13], shards[i].vertex[3]);
        QuadwordCopy(shards[i].vertex[14], shards[i].vertex[5]);
        QuadwordCopy(shards[i].vertex[15], shards[i].vertex[0]);
        QuadwordCopy(shards[i].vertex[16], shards[i].vertex[5]);
        QuadwordCopy(shards[i].vertex[17], shards[i].vertex[2]);
        QuadwordCopy(shards[i].vertex[18], shards[i].vertex[1]);
        QuadwordCopy(shards[i].vertex[19], shards[i].vertex[2]);
        QuadwordCopy(shards[i].vertex[20], shards[i].vertex[4]);
        QuadwordCopy(shards[i].vertex[21], shards[i].vertex[2]);
        QuadwordCopy(shards[i].vertex[22], shards[i].vertex[4]);
        QuadwordCopy(shards[i].vertex[23], shards[i].vertex[5]);
    }
}

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00245728);

extern int gifStateEffectHook;

#define GIF_BREAK_SHARDS 24
#define GIF_BREAK_VERTICES 24

extern GifBreakPoint gifBreakPoints[3];
extern GifBreakShard gifBreakShards[GIF_BREAK_SHARDS];
extern char D_006315C8[8];
extern u32 gifBreakGifTagTemplate[4];

void *memset(void *dst, int value, unsigned int size);
void sceVif1PkAddDirectDataN(XglPacket *packet, const void *data, int qwc);

/* Scratchpad RAM staging area of the shard packet. */
#define GIF_SCRATCHPAD 0x70000000

#define GIF_X(x) ((gifStateOriginX + (x)) * 16 - 8)
#define GIF_Y(y) ((gifStateOriginY + (y)) * 16 - 9)

void gifStateEffectFrame(XglPacket *packet)
{
    int i;
    int j;
    int vertexCount;
    u32 *cursor;
    u32 *gifTag = (void *) GIF_SCRATCHPAD;
    u32 finalAlpha;
    int faceIndex;

    /* ratio runs from 1.0 down to 0: the shards are built at 1.0 and the screen is cleared below 0.5 */
    if (gifStatus.ratio <= 0.0f) {
        gifStateEffectHook = 0;
        return;
    }
    if (gifStatus.ratio == 1.0f || gifStatus.ratio < 0.5f) {
        if (gifStatus.ratio == 1.0f) {
            xglRenderClearDepth();
            gifBreakPoints[0].x = xglSRand() % 200 + 150;
            gifBreakPoints[0].y = xglSRand() % 80 + 70;
            gifBreakPoints[0].z = 0;
            gifBreakPoints[1].x = xglSRand() % 100 + 50;
            gifBreakPoints[1].y = xglSRand() % 100 + 250;
            gifBreakPoints[1].z = 0;
            gifBreakPoints[2].x = xglSRand() % 100 + 350;
            gifBreakPoints[2].y = xglSRand() % 100 + 250;
            gifBreakPoints[2].z = 0;
            memset(gifBreakShards, 0, sizeof(gifBreakShards));
            gifStateBuildBreakShards(gifBreakShards, gifBreakPoints);
        } else {
            xglRenderClearFrame();
            sceVif1PkCnt(packet, 0);
            sceVif1PkOpenDirectHLCode(packet, 0);
            gifStateReset();
            gifStateAddReg(GS_TEXFLUSH, 0);
            gifStateAddReg(GS_ZBUF_1, GS_SET_ZBUF(0, 0x31, 1));
            gifStateAddReg(GS_TEST_1, 0);
            gifStateAddReg(GS_CLAMP_1, GS_SET_CLAMP(1, 1, 0, 0, 0, 0));
            gifStateAddReg(GS_FRAME_1, GS_SET_FRAME(sRender.displayBase, 8, GS_PSMCT32, 0));
            gifStateAddReg(GS_TEX0_1, GIF_STATE_TEX0(sRender.drawBase));
            gifStateAddReg(GS_RGBAQ, GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, GIF_STATE_ONE_BITS));
            gifStateAddReg(GS_PRIM, GS_SET_PRIM(6, 0, 1, 0, 0, 0, 1, 0, 0));
            gifStateAddReg(GS_UV, 0);
            gifStateAddReg(GS_XYZF2, GS_SET_XYZ(GIF_X(0), GIF_Y(0), 0xFFFFFF));
            gifStateAddReg(GS_UV, GS_SET_UV(0x2000, 0x1C00));
            gifStateAddReg(GS_XYZF2, GS_SET_XYZ(GIF_X(GIF_STATE_WIDTH), GIF_Y(GIF_STATE_HEIGHT), 0xFFFFFF));
            gifStateFlush();
            sceVif1PkCloseDirectHLCode(packet);
        }
    }

    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    gifStateReset();
    gifStateAddReg(GS_TEXFLUSH, 0);
    gifStateAddReg(GS_ZBUF_1, GS_SET_ZBUF(0, 0x31, 0));
    gifStateAddReg(GS_TEST_1, 0);
    gifStateAddReg(GS_CLAMP_1, GS_SET_CLAMP(1, 1, 0, 0, 0, 0));
    gifStateAddReg(GS_FRAME_1, GS_SET_FRAME(sRender.drawBase, 8, GS_PSMCT32, 0));
    gifStateAddReg(GS_RGBAQ, GS_SET_RGBAQ(0x28, 0x30, 0x78, 0, GIF_STATE_ONE_BITS));
    gifStateAddReg(GS_PRIM, GS_SET_PRIM(6, 0, 0, 0, 0, 0, 0, 0, 0));
    gifStateAddReg(GS_XYZF2, GS_SET_XYZ(GIF_X(0), GIF_Y(0), 0));
    gifStateAddReg(GS_XYZF2, GS_SET_XYZ(GIF_X(GIF_STATE_WIDTH), GIF_Y(GIF_STATE_HEIGHT), 0));
    gifStateFlush();
    sceVif1PkCloseDirectHLCode(packet);

    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    gifStateReset();
    gifStateAddReg(GS_TEXFLUSH, 0);
    gifStateAddReg(GS_FBA_1, 1);
    gifStateAddReg(GS_TEST_1, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 2));
    gifStateAddReg(GS_CLAMP_1, GS_SET_CLAMP(1, 1, 0, 0, 0, 0));
    gifStateAddReg(GS_FRAME_1, GS_SET_FRAME(sRender.drawBase, 8, GS_PSMCT32, 0));
    gifStateAddReg(GS_TEX0_1, GIF_STATE_TEX0(sRender.displayBase));
    gifStateAddReg(GS_RGBAQ, GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, GIF_STATE_ONE_BITS));
    {
        u32 address = (u32) &gifStatus;
        /* the offset is always 0; it keeps a second register copy of the address as the original */
        GifStatusBlock *status = (GifStatusBlock *) (address + ((address & 0xFF) >> 8));
        switch (status->effectAlphaType) {
        case 0:
            gifStateAddReg(GS_ALPHA_1, GS_SET_ALPHA(0, 1, 2, 1, status->effectAlpha));
            break;
        case 1:
            gifStateAddReg(GS_ALPHA_1, GS_SET_ALPHA(0, 2, 2, 1, status->effectAlpha));
            break;
        case 2:
            gifStateAddReg(GS_ALPHA_1, GS_SET_ALPHA(2, 0, 2, 1, gifStatus.effectAlpha));
            break;
        default:
            gifStateAddReg(GS_ALPHA_1, GS_SET_ALPHA(0, 1, 2, 1, gifStatus.effectAlpha));
            break;
        }
    }
    gifStateAddReg(GS_TEX1_1, 0);
    gifStateAddReg(GS_TEXFLUSH, 0);
    gifStateFlush();
    sceVif1PkCloseDirectHLCode(packet);

    for (i = 0; i < GIF_BREAK_SHARDS; i++) {
        cursor = (void *) (GIF_SCRATCHPAD + 0x10);
        vertexCount = 0;
        func_00245728(&gifBreakShards[i], gifStatus.ratio);
        if (gifBreakShards[i].projected[0][0].f < 0.0f && gifBreakShards[i].projected[1][0].f < 0.0f && gifBreakShards[i].projected[2][0].f < 0.0f) {
            continue;
        }
        if (gifBreakShards[i].projected[0][0].f >= 512.0f && gifBreakShards[i].projected[1][0].f >= 512.0f
            && gifBreakShards[i].projected[2][0].f >= 512.0f) {
            continue;
        }
        for (j = 0; j < GIF_BREAK_VERTICES; j++) {
            faceIndex = j / 3;
            cursor[0] = 0x80;
            cursor[1] = 0x80;
            cursor[2] = D_006315C8[faceIndex];
            cursor[3] = 0x80;
            cursor[4] = (gifBreakShards[i].vertex[j][0].f + gifBreakShards[i].center[0].f) * 16.0f;
            cursor[5] = (gifBreakShards[i].vertex[j][1].f + gifBreakShards[i].center[1].f) * 16.0f;
            cursor[6] = 0;
            cursor[7] = 0;
            cursor[8] = (gifStateOriginX + gifBreakShards[i].projected[j][0].f) * 16.0f - 8.0f;
            cursor[9] = (gifStateOriginY + gifBreakShards[i].projected[j][1].f) * 16.0f - 9.0f;
            cursor[10] = (u32) gifBreakShards[i].projected[j][2].f + 0xFF0000;
            cursor[11] = 0;
            cursor += 12;
            vertexCount++;
        }
        gifTag = (void *) GIF_SCRATCHPAD;
        gifTag[0] = gifBreakGifTagTemplate[0] | vertexCount;
        gifTag[1] = gifBreakGifTagTemplate[1];
        gifTag[2] = gifBreakGifTagTemplate[2];
        gifTag[3] = gifBreakGifTagTemplate[3];
        sceVif1PkCnt(packet, 0);
        sceVif1PkOpenDirectHLCode(packet, 0);
        sceVif1PkAddDirectDataN(packet, (void *) GIF_SCRATCHPAD, vertexCount * 3 + 1);
        sceVif1PkCloseDirectHLCode(packet);
    }

    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    gifStateReset();
    gifStateAddReg(GS_TEXFLUSH, 0);
    gifStateAddReg(GS_ZBUF_1, GS_SET_ZBUF(0, 0x31, 1));
    gifStateAddReg(GS_TEST_1, 0);
    gifStateAddReg(GS_CLAMP_1, GS_SET_CLAMP(1, 1, 0, 0, 0, 0));
    gifStateAddReg(GS_FRAME_1, GS_SET_FRAME(sRender.workBase, 8, GS_PSMCT32, 0));
    gifStateAddReg(GS_RGBAQ, GS_SET_RGBAQ(0, 0, 0, 0, GIF_STATE_ONE_BITS));
    gifStateAddReg(GS_PRIM, GS_SET_PRIM(6, 0, 0, 0, 0, 0, 0, 0, 0));
    gifStateAddReg(GS_XYZF2, GS_SET_XYZ(GIF_X(0), GIF_Y(0), 0xFFFFFF));
    gifStateAddReg(GS_XYZF2, GS_SET_XYZ(GIF_X(351), GIF_Y(153), 0xFFFFFF));
    gifStateFlush();
    sceVif1PkCloseDirectHLCode(packet);

    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    gifStateReset();
    gifStateAddReg(GS_TEXFLUSH, 0);
    gifStateAddReg(GS_FBA_1, 0);
    gifStateAddReg(GS_TEST_1, GS_SET_TEST(1, 3, 1, 0, 0, 0, 0, 0));
    gifStateAddReg(GS_CLAMP_1, GS_SET_CLAMP(GS_REGION_CLAMP, GS_REGION_CLAMP, 0, 511, 0, 447));
    gifStateAddReg(GS_TEX1_1, GS_SET_TEX1(0, 0, 1, 1, 0, 0, 0));
    gifStateAddReg(GS_FRAME_1, GS_SET_FRAME(sRender.workBase, 8, GS_PSMCT32, 0));
    gifStateAddReg(GS_TEX0_1, GIF_STATE_TEX0(sRender.drawBase));
    gifStateAddReg(GS_ALPHA_1, GS_SET_ALPHA(0, 2, 2, 1, 0x80));
    gifStateAddReg(GS_RGBAQ, GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, GIF_STATE_ONE_BITS));
    gifStateAddReg(GS_PRIM, GS_SET_PRIM(6, 0, 1, 0, 0, 0, 1, 0, 0));
    gifStateAddReg(GS_UV, 0);
    gifStateAddReg(GS_XYZF2, GS_SET_XYZ(GIF_X(0), GIF_Y(0), 0xFFFFFF));
    gifStateAddReg(GS_UV, GS_SET_UV(0x2000, 0x1C00));
    gifStateAddReg(GS_XYZF2, GS_SET_XYZ(GIF_X(175), GIF_Y(153), 0xFFFFFF));
    gifStateAddReg(GS_TEXFLUSH, 0);
    gifStateFlush();
    sceVif1PkCloseDirectHLCode(packet);

    for (i = 1; i < 9; i++) {
        int offset[4][2] = { { 1, 1 }, { 1, -1 }, { -1, 1 }, { -1, -1 } };

        sceVif1PkCnt(packet, 0);
        sceVif1PkOpenDirectHLCode(packet, 0);
        gifStateReset();
        gifStateAddReg(GS_TEST_1, GS_SET_TEST(1, 3, 0, 0, 0, 0, 0, 0));
        gifStateAddReg(GS_ALPHA_1, GS_SET_ALPHA(0, 2, 2, 1, 0x10));
        for (j = 0; j < 4; j++) {
            gifStateAddReg(GS_PRIM, GS_SET_PRIM(6, 0, 1, 0, 1, 0, 1, 0, 0));
            gifStateAddReg(GS_UV, 0);
            gifStateAddReg(GS_XYZF2, GS_SET_XYZ(GIF_X(offset[j][0] * i), GIF_Y(offset[j][1] * i), 0xFFFFFF));
            gifStateAddReg(GS_UV, GS_SET_UV(0x2000, 0x1C00));
            gifStateAddReg(GS_XYZF2, GS_SET_XYZ(GIF_X(offset[j][0] * i + 175), GIF_Y(offset[j][1] * i + 153), 0xFFFFFF));
        }
        gifStateFlush();
        sceVif1PkCloseDirectHLCode(packet);
    }

    finalAlpha = 0x40;
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    gifStateReset();
    gifStateAddReg(GS_TEXFLUSH, 0);
    gifStateAddReg(GS_TEST_1, 0);
    gifStateAddReg(GS_CLAMP_1, GS_SET_CLAMP(1, 1, 0, 0, 0, 0));
    gifStateAddReg(GS_ALPHA_1, GS_SET_ALPHA(0, 2, 2, 1, 0x80));
    gifStateAddReg(GS_FRAME_1, GS_SET_FRAME(sRender.drawBase, 8, GS_PSMCT32, 0));
    gifStateAddReg(GS_TEX0_1, GIF_STATE_TEX0(sRender.workBase));
    gifStateAddReg(GS_RGBAQ, GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, GIF_STATE_ONE_BITS));
    gifStateAddReg(GS_PRIM, GS_SET_PRIM(6, 0, 1, 0, 1, 0, 1, 0, 0));
    gifStateAddReg(GS_UV, GS_SET_UV(0x10, 0x10));
    gifStateAddReg(GS_XYZF2, GS_SET_XYZ(GIF_X(0), GIF_Y(0), 0xFFFFFF));
    gifStateAddReg(GS_UV, GS_SET_UV(0xAE0, 0x980));
    gifStateAddReg(GS_XYZF2, GS_SET_XYZ(GIF_X(GIF_STATE_WIDTH), GIF_Y(GIF_STATE_HEIGHT), 0xFFFFFF));
    gifStateAddReg(GS_TEXFLUSH, 1);
    gifStateFlush();
    sceVif1PkCloseDirectHLCode(packet);

    /* in the second half of the effect the final blend fades in with the ratio */
    if (!(gifStatus.ratio > 0.5)) {
        finalAlpha = gifStatus.ratio * 64.0f * 2.0f;
    }
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    gifStateReset();
    gifStateAddReg(GS_TEXFLUSH, 0);
    gifStateAddReg(GS_ALPHA_1, GS_SET_ALPHA(0, 1, 2, 1, 0x40));
    gifStateAddReg(GS_FRAME_1, GS_SET_FRAME(sRender.drawBase, 8, GS_PSMCT32, 0));
    gifStateAddReg(GS_TEX0_1, GIF_STATE_TEX0(sRender.blendBase));
    gifStateAddReg(GS_RGBAQ, GS_SET_RGBAQ(0x80, 0x80, 0x80, 0, GIF_STATE_ONE_BITS) | ((u64) finalAlpha << 24));
    gifStateAddReg(GS_PRIM, GS_SET_PRIM(6, 0, 1, 0, 1, 0, 1, 0, 0));
    gifStateAddReg(GS_UV, 0x40);
    gifStateAddReg(GS_XYZF2, GS_SET_XYZ(GIF_X(0), GIF_Y(0), 0xFFFFFF));
    gifStateAddReg(GS_UV, GS_SET_UV(0x1FC0, 0x1C00));
    gifStateAddReg(GS_XYZF2, GS_SET_XYZ(GIF_X(GIF_STATE_WIDTH), GIF_Y(GIF_STATE_HEIGHT), 0xFFFFFF));
    gifStateFlush();
    sceVif1PkCloseDirectHLCode(packet);

    if (0.5f < gifStatus.ratio || gifStatus.fadeOut != 0) {
        gifStatus.ratio -= gifStatus.ratioStep;
    }
}

void xglRenderCopyDisp2Flip(void);
int sceGsSyncPath(int, int);

void func_00246AF8(void) {
    sceGsSyncPath(0, 0);
    xglRenderClearDepth();
    xglRenderCopyDisp2Flip();
}

/*
 * GIF state packet. gifStateNumPacket (numPacket) counts the register pairs queued in
 * gifStatePacket; entry 0 is the GIF tag word, the pairs follow in packed mode (A+D).
 * The assert text "numPacket < (STATEPACKET_MAX - 1)" names both.
 */
extern int gifStateNumPacket;
extern u32 gifStatePacket[4];

/* GIF tag words: end-of-packet flag in the low word, one register descriptor (NREG = 1) in word 1. */
#define GIF_TAG_EOP 0x8000
#define GIF_TAG_NREG_1 0x10000000

/* Empties the queue and writes the tag header: one register per loop, A+D descriptor. */
void gifStateReset(void) {
    gifStateNumPacket = 0;
    gifStatePacket[0] = GIF_TAG_EOP;
    gifStatePacket[1] = GIF_TAG_NREG_1;
    gifStatePacket[2] = GIF_AD;
    gifStatePacket[3] = 0;
}

INCLUDE_ASM("asm/main/nonmatchings/gifState", gifStateAddReg);

INCLUDE_ASM("asm/main/nonmatchings/gifState", gifStateAddRawQword);

void sceVif1PkAddDirectDataN(XglPacket *packet, const void *data, int qwc);
XglPacket *xglPacketGetCurrent(void);

/* Sends the queued pairs as one direct VIF transfer of count+1 quadwords (tag + pairs). */
void gifStateFlush(void) {
    XglPacket *packet;
    int count;
    int tag;
    unsigned int byteCount;

    packet = xglPacketGetCurrent();
    count = gifStateNumPacket;
    tag = count | GIF_TAG_EOP;
    byteCount = (count * 0x10) + 0x10;
    if (count != 0) {
        gifStatePacket[0] = tag;
        sceVif1PkAddDirectDataN(packet, gifStatePacket, byteCount >> 4);
        gifStateNumPacket = 0;
    }
}

/* Queues one A+D pair: gifData goes to the GS register at address gifAddress. */
void gifStateAddReg(int gifAddress, u64 gifData);

/* GS TEXFLUSH with no data. */
void gifStateTexFlush(void) {
    gifStateAddReg(GS_TEXFLUSH, 0);
}

/* GS ALPHA_1: (Cs - Cd) * FIX + Cd with a fixed alpha. */
void gifStateSetAlphaFix(int fixedAlpha) {
    gifStateAddReg(GS_ALPHA_1, GS_SET_ALPHA(0, 1, 2, 1, fixedAlpha));
}

INCLUDE_ASM("asm/main/nonmatchings/gifState", gifStateSetScissor);

/* GS ZBUF_1 at page 0 with the Z format code 0x31 in the PSM field. */
void gifStateSetZBufFormat(void) {
    gifStateAddReg(GS_ZBUF_1, GS_SET_ZBUF(0, 0x31, 0));
}

INCLUDE_ASM("asm/main/nonmatchings/gifState", nmlModelSetMulColor);


/* Sets the alpha type and the alpha value of the effect (gifStatus +0x0C/+0x10). */
void gifStateSetEffectAlpha(int alphaType, int alpha) {
    gifStatus.effectAlphaType = alphaType;
    gifStatus.effectAlpha = alpha;
}

extern f32 gifStateDecrease;

/* Stores the decrease value of the effect; the reader of gifStateDecrease is not located yet. */
void gifStateSetDecrease(f32 decrease) {
    gifStateDecrease = decrease;
}

/* Sets the centre of the effect (gifStatus +0x04/+0x08). */
void gifStateSetCenter(int centerX, int centerY) {
    gifStatus.centerX = centerX;
    gifStatus.centerY = centerY;
}

extern int D_0063165C;

void func_00246DD8(int value) {
    D_0063165C = value;
}

extern int gifStateFbp;

/* Stores the frame buffer base of the effect. */
void gifStateSetFbp(int fbp) {
    gifStateFbp = fbp;
}

extern int gifStateBlur;

/* Stores the blur amount of the effect. */
void gifStateSetBlur(int blur) {
    gifStateBlur = blur;
}

void nmlModelUseSubWindow(unsigned int index, int enabled) {
    if (index < NML_WINDOW_COUNT) {
        g_aSubWindow[index] = enabled;
    }
}

extern f32 nmlModelSubWindowValue[];

void nmlModelSetSubWindowValue(u32 index, f32 value) {
    if (index < 4U) {
        nmlModelSubWindowValue[index] = value;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/gifState", gifStateSetMode);

extern int D_0064586C;

void gifStateClearModeValue(void) {
    D_0064586C = 0;
}


void func_00246EB0(void) {
    func_001BA6F4();
    func_00246AF8();
    msgStateReset();
}

INCLUDE_ASM("asm/main/nonmatchings/gifState", gifStateEnableEffect);

void nmlModelEnableSubWindow0(void) {
    nmlModelUseSubWindow(0U, 1);
}

extern int gifStateEffectHook;

int gifStateEffectReady(void) {
    if (gifStateEffectHook != 0 && gifStatus.state == 5) {
        if (!(0.5f <= gifStatus.ratio)) {
            return 1;
        }
        return 0;
    }
    return 1;
}

extern int D_00631660;

void gifStateDisableEffect(void) {
    D_00631660 = 0;
    gifStateEffectHook = 0;
}

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00246FF8);

extern int gifStateDrawHookSuppressed;

void gifStateSuppressDrawHook(void) {
    gifStateDrawHookSuppressed = 1;
}

extern void (*gifStateDrawHook)(XglPacket *packet);

void gifStateUpdateDrawHook(void) {
    if (func_001BBC74() != 0 && gifStateDrawHookSuppressed == 0) {
        gifStateDrawHook = gifStateDrawBlurFrame;
        return;
    }
    gifStateDrawHookSuppressed = 0;
    gifStateDrawHook = 0;
}
