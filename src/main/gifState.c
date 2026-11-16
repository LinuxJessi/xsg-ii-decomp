#include "common.h"
#include "shared.h"
#include "gs.h"
#include "quadword.h"
#include "main/gifState.h"
#include "main/tu051.h"
#include "main/nml_model_set.h"
#include "main/TiWater.h"
#include "main/tu026.h"
#include "main/tu148.h"
#include "main/tu255.h"
#include "main/tu256.h"
#include "main/xgl_2.h"
#include "main/xgl_render.h"
#include "main/sdv_033.h"
#include "main/tu254.h"
#include "main/xgl_studio.h"

typedef struct XglPacket XglPacket;

void gifStateReset(void);
void gifStateFlush(void);
void sceVif1PkCloseDirectHLCode(XglPacket *packet);
void sceVif1PkCnt(XglPacket *packet, int count);
void sceVif1PkOpenDirectHLCode(XglPacket *packet, int count);
void sceVif1PkAddDirectDataN(XglPacket *packet, const void *data, int qwc);
void gifStateSetupContext(XglPacket *packet, signed char alphaFix);

extern int gifStateOriginX;
extern int gifStateOriginY;

/* The 512x448 frame buffer in pixels. */
#define GIF_STATE_WIDTH 512
#define GIF_STATE_HEIGHT 448

#define NML_WINDOW_COUNT 4


INCLUDE_ASM("asm/main/nonmatchings/gifState", gifStateSetupContext);

INCLUDE_ASM("asm/main/nonmatchings/gifState", gifStateDrawRing);


extern GameWorkControl gameWork;
/* The smallest texture scale of the blur, 0.9 of the ratio. */
extern f32 gifBlurMinScale;
/* The texture scale of each edge point of the blur fan. */
extern f32 gifBlurScales[512];
/* The GIF tag template of the blur fan (the vertex count is or-ed in). */
extern u32 gifBlurFanTag[4];

#define GIF_GAME_PAUSED 0x1
/* The edge points are this far apart at least. */
#define GIF_BLUR_MIN_STEP 4

/* Draws the shown page again as a triangle fan from the effect centre to points around the screen edge, every
 * subWindow[0] pixels; each edge point samples the texture at a random scale between 0.9 and 1.0 of the ratio
 * towards the centre, so the picture blurs radially. The scales are drawn anew and the ratio lowered by its
 * step only while the game is not paused. Nothing is drawn while the ratio is 0. */
void gifStateDrawRadialBlur(XglPacket *packet)
{
    u32 *cursor;
    u32 *tag = 0;
    int k;
    int count;
    int step;
    int pos;
    float scale = 0.0f;

    if (gifStatus.ratio == scale) {
        return;
    }
    gifBlurMinScale = gifStatus.ratio * 0.9f;
    if (!(gameWork.control & GIF_GAME_PAUSED)) {
        for (pos = 0; pos < 512; pos++) {
            gifBlurScales[pos] = (xglSRand() % 10U) * 0.01f + gifBlurMinScale;
        }
    }
    k = 0;
    count = 1;
    pos = 0;
    gifStateSetupContext(packet, gifStatus.effectAlpha * gifStatus.ratio);
    step = gifStatus.subWindow[0];
    if (step < GIF_BLUR_MIN_STEP) {
        step = GIF_BLUR_MIN_STEP;
    }
    cursor = (u32 *) 0x70000010;
    cursor[0] = gifStatus.centerX << 4;
    cursor[1] = gifStatus.centerY << 4;
    cursor[2] = 0;
    cursor[3] = 0;
    cursor[4] = ((gifStateOriginX + gifStatus.centerX) << 4) - 8;
    cursor[5] = ((gifStateOriginY + gifStatus.centerY) << 4) - 9;
    cursor[6] = gifStatus.depth;
    cursor[7] = 0;
    cursor = (u32 *) 0x70000030;
    do {
        scale = gifBlurScales[k++];
        cursor[0] = (gifStatus.centerX + (pos - gifStatus.centerX) * scale) * 16.0f;
        cursor[1] = (gifStatus.centerY + (0 - gifStatus.centerY) * scale) * 16.0f;
        cursor[2] = 0;
        cursor[3] = 0;
        count++;
        cursor[4] = ((gifStateOriginX + pos) << 4) - 8;
        pos += step;
        cursor[5] = ((gifStateOriginY + 0) << 4) - 9;
        cursor[6] = gifStatus.depth;
        cursor[7] = 0;
        cursor += 8;
    } while (pos < GIF_STATE_WIDTH);
    for (pos = 0; pos < GIF_STATE_HEIGHT; pos += step) {
        scale = gifBlurScales[k++];
        cursor[0] = (gifStatus.centerX + ((GIF_STATE_WIDTH - 1) - gifStatus.centerX) * scale) * 16.0f;
        cursor[1] = (gifStatus.centerY + (pos - gifStatus.centerY) * scale) * 16.0f;
        cursor[2] = 0;
        cursor[3] = 0;
        count++;
        cursor[4] = ((gifStateOriginX + (GIF_STATE_WIDTH - 1)) << 4) - 8;
        cursor[5] = ((gifStateOriginY + pos) << 4) - 9;
        cursor[6] = gifStatus.depth;
        cursor[7] = 0;
        cursor += 8;
    }
    for (pos = GIF_STATE_WIDTH - 1; pos >= 0; pos -= step) {
        scale = gifBlurScales[k++];
        cursor[0] = (gifStatus.centerX + (pos - gifStatus.centerX) * scale) * 16.0f;
        cursor[1] = (gifStatus.centerY + ((GIF_STATE_HEIGHT - 1) - gifStatus.centerY) * scale) * 16.0f;
        cursor[2] = 0;
        cursor[3] = 0;
        count++;
        cursor[4] = ((gifStateOriginX + pos) << 4) - 8;
        cursor[5] = ((gifStateOriginY + (GIF_STATE_HEIGHT - 1)) << 4) - 9;
        cursor[6] = gifStatus.depth;
        cursor[7] = 0;
        cursor += 8;
    }
    for (pos = GIF_STATE_HEIGHT - 1; pos >= 0; pos -= step) {
        scale = gifBlurScales[k++];
        cursor[0] = (gifStatus.centerX + (0 - gifStatus.centerX) * scale) * 16.0f;
        cursor[1] = (gifStatus.centerY + (pos - gifStatus.centerY) * scale) * 16.0f;
        cursor[2] = 0;
        cursor[3] = 0;
        count++;
        cursor[4] = ((gifStateOriginX + 0) << 4) - 8;
        cursor[5] = ((gifStateOriginY + pos) << 4) - 9;
        cursor[6] = gifStatus.depth;
        cursor[7] = 0;
        cursor += 8;
    }
    count++;
    tag = (u32 *) 0x70000000;
    tag[0] = gifBlurFanTag[0] | count;
    tag[1] = gifBlurFanTag[1];
    tag[2] = gifBlurFanTag[2];
    tag[3] = gifBlurFanTag[3];
    tag += 4;
    cursor[0] = tag[8];
    cursor[1] = tag[9];
    cursor[2] = tag[10];
    cursor[3] = tag[11];
    cursor[4] = tag[12];
    cursor[5] = tag[13];
    cursor[6] = tag[14];
    cursor[7] = tag[15];
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    sceVif1PkAddDirectDataN(packet, (void *) 0x70000000, count * 2 + 1);
    sceVif1PkCloseDirectHLCode(packet);
    if (!(gameWork.control & GIF_GAME_PAUSED)) {
        gifStatus.ratio -= gifStatus.ratioStep;
        if (gifStatus.ratio < 0.0f) {
            gifStatus.ratio = 0.0f;
        }
    }
}

/* The camera quad packet on the scratchpad: the GIF tag, then four vertices of UV and XYZ2. */
typedef struct GifCameraVertex {
    u32 uv[4];
    u32 xyz[4];
} GifCameraVertex;

typedef struct GifCameraPacket {
    u32 tag[4];
    GifCameraVertex vertex[4];
} GifCameraPacket;

#define GIF_CAMERA_PACKET ((GifCameraPacket *) 0x70000000)

/* The GIF tag template of the camera quad (the vertex count is or-ed in). */
extern u32 gifCameraQuadTag[4];

/* Draws the shown page again as a 512x448 sprite, offset on the screen by the blur direction turned into
 * the active camera's view and scaled by the blur amount, with the effect alpha faded by the ratio; then
 * lowers the ratio by its step. Nothing is drawn once the ratio is down to 0. */
void gifStateDrawCameraQuad(XglPacket *packet)
{
    Vector4 scaled;
    Vector4 offset;
    Matrix matrix;
    int corners[4][2];
    XglCamera *camera = xglStudioSelectGetActiveCamera(0);
    u32 *cursor;
    u32 *gifTag = 0;
    int x;
    int y;

    if (gifStatus.ratio <= 0.0f) {
        return;
    }
    tiVu0LoadFixedMatrix();
    xglCurMulRotateY(camera->travel.rotation.y);
    xglCurMulRotateX(camera->travel.rotation.x);
    xglCurMulRotateZ(camera->travel.rotation.z);
    xglCurMatrixGet(matrix.rows);
    xglMatrixInverse(&matrix, &matrix);
    xglVectorScale(&scaled, &gifStatus.mulColor, gifStatus.blur);
    xglVectorMulMat(&offset, matrix.rows, &scaled);
    x = -offset.x;
    y = offset.y;
    corners[0][0] = x;
    corners[0][1] = y;
    corners[1][0] = x;
    corners[1][1] = y;
    corners[2][0] = x;
    corners[2][1] = y;
    corners[3][0] = x;
    corners[3][1] = y;
    gifStateSetupContext(packet, gifStatus.ratio * gifStatus.effectAlpha);
    cursor = GIF_CAMERA_PACKET->vertex[0].uv;
    cursor[0] = 0;
    cursor[1] = 0;
    cursor[2] = 0;
    cursor[3] = 0;
    cursor[4] = ((gifStateOriginX + corners[0][0]) << 4) - 8;
    cursor[5] = ((gifStateOriginY + corners[0][1]) << 4) - 9;
    cursor[6] = gifStatus.depth;
    cursor[7] = 0;
    cursor = GIF_CAMERA_PACKET->vertex[1].uv;
    cursor[0] = (GIF_STATE_WIDTH - 1) << 4;
    cursor[1] = 0;
    cursor[2] = 0;
    cursor[3] = 0;
    cursor[4] = ((gifStateOriginX + corners[1][0] + (GIF_STATE_WIDTH - 1)) << 4) - 8;
    cursor[5] = ((gifStateOriginY + corners[1][1]) << 4) - 9;
    cursor[6] = gifStatus.depth;
    cursor[7] = 0;
    cursor = GIF_CAMERA_PACKET->vertex[2].uv;
    cursor[0] = 0;
    cursor[1] = (GIF_STATE_HEIGHT - 1) << 4;
    cursor[2] = 0;
    cursor[3] = 0;
    cursor[4] = ((gifStateOriginX + corners[2][0]) << 4) - 8;
    cursor[5] = ((gifStateOriginY + corners[2][1] + (GIF_STATE_HEIGHT - 1)) << 4) - 9;
    cursor[6] = gifStatus.depth;
    cursor[7] = 0;
    cursor = GIF_CAMERA_PACKET->vertex[3].uv;
    cursor[0] = (GIF_STATE_WIDTH - 1) << 4;
    cursor[1] = (GIF_STATE_HEIGHT - 1) << 4;
    cursor[2] = 0;
    cursor[3] = 0;
    cursor[4] = ((gifStateOriginX + corners[3][0] + (GIF_STATE_WIDTH - 1)) << 4) - 8;
    cursor[5] = ((gifStateOriginY + corners[3][1] + (GIF_STATE_HEIGHT - 1)) << 4) - 9;
    cursor[6] = gifStatus.depth;
    cursor[7] = 0;
    gifTag = GIF_CAMERA_PACKET->tag;
    gifTag[0] = gifCameraQuadTag[0] | 4;
    gifTag[1] = gifCameraQuadTag[1];
    gifTag[2] = gifCameraQuadTag[2];
    gifTag[3] = gifCameraQuadTag[3];
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    sceVif1PkAddDirectDataN(packet, GIF_CAMERA_PACKET, 9);
    sceVif1PkCloseDirectHLCode(packet);
    gifStatus.ratio -= gifStatus.ratioStep;
}

INCLUDE_ASM("asm/main/nonmatchings/gifState", gifStateDrawJitterGrid);


/* The GIF tag template of the fade quad (the vertex count is or-ed in). */
extern u32 gifFadeQuadTag[4];

/* Draws the shown page again as a 512x448 sprite whose corners move by the sub-window offsets scaled by
 * the ratio, with the effect alpha faded by the ratio; then lowers the ratio by its step. Nothing is
 * drawn once the ratio is down to 0. */
void gifStateDrawFadeQuad(XglPacket *packet)
{
    u32 *cursor;
    u32 *gifTag = 0;
    int count;

    if (gifStatus.ratio <= 0.0f) {
        return;
    }
    gifStateSetupContext(packet, gifStatus.ratio * gifStatus.effectAlpha);
    cursor = (u32 *) 0x70000010;
    count = 0;
    cursor[0] = 0;
    cursor[1] = 0;
    cursor[2] = 0;
    cursor[3] = 0;
    cursor[4] = (gifStateOriginX + gifStatus.subWindow[0] * gifStatus.ratio) * 16.0f - 8.0f;
    cursor[5] = (gifStateOriginY + gifStatus.subWindow[1] * gifStatus.ratio) * 16.0f - 9.0f;
    cursor[6] = gifStatus.depth;
    cursor[7] = 0;
    cursor += 8;
    count++;
    cursor[0] = (GIF_STATE_WIDTH - 1) << 4;
    cursor[1] = 0;
    cursor[2] = 0;
    cursor[3] = 0;
    cursor[4] = (gifStateOriginX + (GIF_STATE_WIDTH - 1) + gifStatus.subWindow[0] * gifStatus.ratio) * 16.0f - 8.0f;
    cursor[5] = (gifStateOriginY + gifStatus.subWindow[1] * gifStatus.ratio) * 16.0f - 9.0f;
    cursor[6] = gifStatus.depth;
    cursor[7] = 0;
    cursor += 8;
    count++;
    cursor[0] = 0;
    cursor[1] = (GIF_STATE_HEIGHT - 1) << 4;
    cursor[2] = 0;
    cursor[3] = 0;
    cursor[4] = (gifStateOriginX + gifStatus.subWindow[2] * gifStatus.ratio) * 16.0f - 8.0f;
    cursor[5] = (gifStateOriginY + (GIF_STATE_HEIGHT - 1) + gifStatus.subWindow[3] * gifStatus.ratio) * 16.0f - 9.0f;
    cursor[6] = gifStatus.depth;
    cursor[7] = 0;
    cursor += 8;
    count++;
    cursor[0] = (GIF_STATE_WIDTH - 1) << 4;
    cursor[1] = (GIF_STATE_HEIGHT - 1) << 4;
    cursor[2] = 0;
    cursor[3] = 0;
    cursor[4] = (gifStateOriginX + (GIF_STATE_WIDTH - 1) + gifStatus.subWindow[2] * gifStatus.ratio) * 16.0f - 8.0f;
    cursor[5] = (gifStateOriginY + (GIF_STATE_HEIGHT - 1) + gifStatus.subWindow[3] * gifStatus.ratio) * 16.0f - 9.0f;
    cursor[6] = gifStatus.depth;
    cursor[7] = 0;
    count++;
    gifTag = (u32 *) 0x70000000;
    gifTag[0] = gifFadeQuadTag[0] | count;
    gifTag[1] = gifFadeQuadTag[1];
    gifTag[2] = gifFadeQuadTag[2];
    gifTag[3] = gifFadeQuadTag[3];
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    sceVif1PkAddDirectDataN(packet, gifTag, count * 2 + 1);
    sceVif1PkCloseDirectHLCode(packet);
    gifStatus.ratio -= gifStatus.ratioStep;
}

void func_002439C8(void) {
}

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
extern int gifStateBlurDivisor;                      /* blur divisor of the alpha (3) */
extern int gifStateBlurOffset;                      /* blur offset in pixels */


/* IEEE-754 1.0f as the raw Q word of RGBAQ. */
#define GIF_STATE_ONE_BITS 0x3F800000

/* XYZ2 of a sprite corner at (x, y) pixels from the GS origin. */
#define GIF_STATE_XY(x, y) GS_SET_XYZ(((gifStateOriginX + (x)) << 4) - 8, ((gifStateOriginY + (y)) << 4) - 9, 0)

/* FRAME_1 of a 512-pixel-wide page with the alpha channel masked. */
#define GIF_STATE_FRAME(base) GS_SET_FRAME(base, 8, GS_PSMCT32, 0xFF000000)

/* TEX0_1 of a 512x512 PSMCT32 page used as a texture. */
#define GIF_STATE_TEX0(base) GS_SET_TEX0((base) << 5, 8, GS_PSMCT32, 9, 9, 1, 0, 0, 0, 0, 0, 0)

/* Size in pixels of the area gifStateCopyDisplayToWork copies. */
extern int gifCopyWidth;
extern int gifCopyHeight;

/* Copies the gifCopyWidth x gifCopyHeight pixels at the origin of the shown page into the work page. */
void gifStateCopyDisplayToWork(XglPacket *packet)
{
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    gifStateReset();
    gifStateAddReg(GS_FRAME_1, GIF_STATE_FRAME(sRender.workBase));
    gifStateAddReg(GS_TEX0_1, GIF_STATE_TEX0(sRender.displayBase));
    gifStateAddReg(GS_RGBAQ, GS_SET_RGBAQ(0x7F, 0x7F, 0x7F, 0x7F, GIF_STATE_ONE_BITS));
    gifStateAddReg(GS_PRIM, GS_SET_PRIM(6, 0, 1, 0, 0, 0, 1, 0, 0));
    gifStateAddReg(GS_UV, GS_SET_UV(0, 0));
    gifStateAddReg(GS_XYZF2, GIF_STATE_XY(0, 0));
    gifStateAddReg(GS_UV, GS_SET_UV(gifCopyWidth << 4, gifCopyHeight << 4));
    gifStateAddReg(GS_XYZF2, GIF_STATE_XY(gifCopyWidth, gifCopyHeight));
    gifStateFlush();
    sceVif1PkCloseDirectHLCode(packet);
}

void func_00243B58(XglPacket *packet) {
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    gifStateReset();
    gifStateFlush();
    sceVif1PkCloseDirectHLCode(packet);
}

/* Draws the work page as a textured sprite of width x height texels at (offsetX, offsetY), 256 x 224
 * pixels in size, blended with the fixed alpha into the shown page. */
void gifStateDrawQuad(XglPacket *packet, unsigned int width, unsigned int height, int offsetX, int offsetY, int alpha)
{
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    gifStateReset();
    gifStateAddReg(GS_TEXFLUSH, 0);
    gifStateAddReg(GS_ALPHA_1, GS_SET_ALPHA(0, 2, 2, 1, alpha));
    gifStateAddReg(GS_FRAME_1, GS_SET_FRAME(sRender.displayBase, 8, GS_PSMCT32, 0));
    gifStateAddReg(GS_TEX0_1, GIF_STATE_TEX0(sRender.workBase));
    gifStateAddReg(GS_RGBAQ, GS_SET_RGBAQ(0x7F, 0x7F, 0x7F, 0x7F, GIF_STATE_ONE_BITS));
    gifStateAddReg(GS_PRIM, GS_SET_PRIM(6, 0, 1, 0, 1, 0, 1, 0, 0));
    gifStateAddReg(GS_UV, GS_SET_UV(0, 0));
    gifStateAddReg(GS_XYZF2, GIF_STATE_XY(offsetX, offsetY));
    gifStateAddReg(GS_UV, GS_SET_UV(width << 4, height << 4));
    gifStateAddReg(GS_XYZF2, GIF_STATE_XY(offsetX + 256, offsetY + 224));
    gifStateFlush();
    sceVif1PkCloseDirectHLCode(packet);
}

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

#define GIF_BREAK_VERTICES 24

/* Projection depth of the shards: a vertex at z is scaled by (z + depth) / depth. */
#define GIF_BREAK_DEPTH 300.0f
/* The part of the break (1 - ratio) over which the shards spin up. */
#define GIF_BREAK_SPIN_UP 0.05f
/* How fast the spin grows once a shard falls. */
#define GIF_BREAK_SPIN_GROWTH 8.0f
/* Above this ratio the falling shards speed up; below it they come back. */
#define GIF_BREAK_TURN_RATIO 0.5f

/* Moves one shard for the break ratio (1 down to 0). The shard spins up over the first part of the
 * break and then keeps its angle; once 1 - ratio passes its speed it falls: the spin grows and the
 * fall count goes up each frame while the ratio is above one half, below it the angle is reset and
 * the count goes down again while sub-window 0 is on. Its vertices are rotated (X, then Y), projected
 * and moved to its centre, sideways away from the screen middle by twice the squared fall count. */
void gifBreakShardUpdate(GifBreakShard *shard, f32 ratio)
{
    Matrix4 matrix;
    Vector4 angle;
    float progress = 1.0f - ratio;
    float depth;
    int i;

    if (shard->speed.f < progress) {
        if (GIF_BREAK_TURN_RATIO < ratio) {
            float scale = (progress - shard->speed.f) * GIF_BREAK_SPIN_GROWTH + 1.0f;

            shard->angle[3].f = 1.0f;
            angle.w = 1.0f;
            shard->fall.i++;
            angle.x = shard->angle[0].f = shard->spin[0].f * scale;
            angle.y = shard->angle[1].f = shard->spin[1].f * scale;
            angle.z = shard->angle[2].f = shard->spin[2].f * scale;
        } else {
            if (shard->fall.i > 0 && gifStatus.subWindow[0] != 0) {
                shard->fall.i--;
            }
            angle.x = 0.0f;
            angle.y = 0.0f;
            angle.z = 0.0f;
        }
    } else if (GIF_BREAK_SPIN_UP < progress) {
        angle.x = shard->angle[0].f;
        angle.y = shard->angle[1].f;
        angle.z = shard->angle[2].f;
        angle.w = shard->angle[3].f;
    } else {
        float scale = progress / GIF_BREAK_SPIN_UP;

        shard->angle[3].f = 1.0f;
        angle.w = 1.0f;
        angle.x = shard->angle[0].f = shard->spin[0].f * scale;
        angle.y = shard->angle[1].f = shard->spin[1].f * scale;
        angle.z = shard->angle[2].f = shard->spin[2].f * scale;
    }
    xglCurSetRotateX(angle.x);
    xglCurMulRotateY(angle.y);
    xglCurMatrixGet(matrix);
    for (i = 0; i < GIF_BREAK_VERTICES; i++) {
        /* A GifQuad has the layout of a Vector4. */
        xglCurApplyVector((Vector4 *) shard->projected[i], (Vector4 *) shard->vertex[i]);
        depth = (shard->projected[i][2].f + GIF_BREAK_DEPTH) / GIF_BREAK_DEPTH;
        shard->projected[i][0].f *= depth;
        shard->projected[i][1].f *= depth;
    }
    if (shard->fall.i != 0) {
        if (GIF_STATE_WIDTH / 2 <= shard->center[0].f) {
            for (i = 0; i < GIF_BREAK_VERTICES; i++) {
                shard->projected[i][0].f += shard->center[0].f + (float) (shard->fall.i * shard->fall.i) * 2;
                shard->projected[i][1].f += shard->center[1].f;
            }
        } else {
            for (i = 0; i < GIF_BREAK_VERTICES; i++) {
                shard->projected[i][0].f += shard->center[0].f - (float) (shard->fall.i * shard->fall.i) * 2;
                shard->projected[i][1].f += shard->center[1].f;
            }
        }
    } else {
        for (i = 0; i < GIF_BREAK_VERTICES; i++) {
            shard->projected[i][0].f += shard->center[0].f;
            shard->projected[i][1].f += shard->center[1].f;
        }
    }
}

extern void (*gifStateEffectHook)(XglPacket *packet);

#define GIF_BREAK_SHARDS 24

extern GifBreakPoint gifBreakPoints[3];
extern GifBreakShard gifBreakShards[GIF_BREAK_SHARDS];
extern char D_006315C8[8];
extern u32 gifBreakGifTagTemplate[4];

void *memset(void *dst, int value, unsigned int size);

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
        gifBreakShardUpdate(&gifBreakShards[i], gifStatus.ratio);
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

    if (0.5f < gifStatus.ratio || gifStatus.subWindow[0] != 0) {
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

/* One quadword of gifStatePacket: a data/address pair (A+D) or four raw words (the tag). */
typedef union GifStateQword {
    u64 pair[2];                /* A+D: the data, then the register address */
    u32 word[4];
} GifStateQword;

extern GifStateQword gifStatePacket[];

/* GIF tag words: end-of-packet flag in the low word, one register descriptor (NREG = 1) in word 1. */
#define GIF_TAG_EOP 0x8000
#define GIF_TAG_NREG_1 0x10000000

/* Empties the queue and writes the tag header: one register per loop, A+D descriptor. */
void gifStateReset(void) {
    gifStateNumPacket = 0;
    gifStatePacket[0].word[0] = GIF_TAG_EOP;
    gifStatePacket[0].word[1] = GIF_TAG_NREG_1;
    gifStatePacket[0].word[2] = GIF_AD;
    gifStatePacket[0].word[3] = 0;
}

/* The assert text and the file name of the packet queue asserts. */
extern const char gifStatePacketAssert[];
extern const char gifStateSourceFile[];

/* The queue size the assert names: numPacket < (STATEPACKET_MAX - 1). */
#define STATEPACKET_MAX 0x80

/* Queues the A+D pair (gifAddress, gifData) as the next quadword of gifStatePacket (asserts while the
 * packet is not full). */
void gifStateAddReg(int gifAddress, u64 gifData)
{
    if ((unsigned int) gifStateNumPacket >= STATEPACKET_MAX - 1) {
        assertFailed(gifStatePacketAssert, gifStateSourceFile, 37);
    }
    gifStateNumPacket++;
    gifStatePacket[gifStateNumPacket].pair[0] = gifData;
    gifStatePacket[gifStateNumPacket].pair[1] = gifAddress;
}

/* Queues four raw words as one quadword of gifStatePacket (asserts while the packet is not full). */
void gifStateAddRawQword(u32 word0, u32 word1, u32 word2, u32 word3)
{
    if ((unsigned int) gifStateNumPacket >= STATEPACKET_MAX - 1) {
        assertFailed(gifStatePacketAssert, gifStateSourceFile, 50);
    }
    gifStateNumPacket++;
    gifStatePacket[gifStateNumPacket].word[0] = word0;
    gifStatePacket[gifStateNumPacket].word[1] = word1;
    gifStatePacket[gifStateNumPacket].word[2] = word2;
    gifStatePacket[gifStateNumPacket].word[3] = word3;
}

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
        gifStatePacket[0].word[0] = tag;
        sceVif1PkAddDirectDataN(packet, gifStatePacket, byteCount >> 4);
        gifStateNumPacket = 0;
    }
}

/* GS TEXFLUSH with no data. */
void gifStateTexFlush(void) {
    gifStateAddReg(GS_TEXFLUSH, 0);
}

/* GS ALPHA_1: (Cs - Cd) * FIX + Cd with a fixed alpha. */
void gifStateSetAlphaFix(int fixedAlpha) {
    gifStateAddReg(GS_ALPHA_1, GS_SET_ALPHA(0, 1, 2, 1, fixedAlpha));
}

/* Sets the drawing scissor rectangle (SCISSOR_1). */
void gifStateSetScissor(int minX, int minY, int maxX, int maxY)
{
    gifStateAddReg(GS_SCISSOR_1, GS_SET_SCISSOR(minX, maxX, minY, maxY));
}

/* GS ZBUF_1 at page 0 with the Z format code 0x31 in the PSM field. */
void gifStateSetZBufFormat(void) {
    gifStateAddReg(GS_ZBUF_1, GS_SET_ZBUF(0, 0x31, 0));
}

/* Sets the model multiply colour (a quadword copy). */
void nmlModelSetMulColor(Vector4 *color)
{
    QuadwordCopy(&gifStatus.mulColor, color);
}


/* Sets the alpha type and the alpha value of the effect (gifStatus +0x0C/+0x10). */
void gifStateSetEffectAlpha(int alphaType, int alpha) {
    gifStatus.effectAlphaType = alphaType;
    gifStatus.effectAlpha = alpha;
}

/* Sets the ratio step of the effect: the draw functions lower the ratio by it each frame. */
void gifStateSetDecrease(f32 decrease) {
    gifStatus.ratioStep = decrease;
}

/* Sets the centre of the effect (gifStatus +0x04/+0x08). */
void gifStateSetCenter(int centerX, int centerY) {
    gifStatus.centerX = centerX;
    gifStatus.centerY = centerY;
}

void func_00246DD8(int value) {
    gifStatus.depth = value;
}

/* Stores the frame buffer base of the effect. */
void gifStateSetFbp(int fbp) {
    gifStatus.fbp = fbp;
}

/* Stores the blur amount of the effect. */
void gifStateSetBlur(int blur) {
    gifStatus.blur = blur;
}

void nmlModelUseSubWindow(unsigned int index, int enabled) {
    if (index < NML_WINDOW_COUNT) {
        gifStatus.subWindow[index] = enabled;
    }
}

extern f32 nmlModelSubWindowValue[];

void nmlModelSetSubWindowValue(u32 index, f32 value) {
    if (index < 4U) {
        nmlModelSubWindowValue[index] = value;
    }
}

/* The draw function of each effect mode. */
extern GifEffectDraw gifStateEffectDraws[];

/* Selects one of the seven GIF effects: its draw function becomes the effect hook, the state word and a
 * full ratio. */
void gifStateSetMode(int mode)
{
    if (mode < GIF_EFFECT_COUNT) {
        gifStatus.state = mode;
        gifStateModeHook = gifStateEffectDraws[mode];
        gifStatus.ratio = 1.0f;
    }
}

void gifStateClearModeValue(void) {
    gifStateModeHook = 0;
}


void func_00246EB0(void) {
    func_001BA6F4();
    func_00246AF8();
    msgStateReset();
}

/* Starts the screen effect: full effect alpha, the decrease rate, frame buffer 1, the sub window off, the
 * effect frame hook, state GIF_EFFECT_FRAME and ratio 1, then shows panel entry 0x151 in mode 0x4E (tu148's
 * func_00251B18 shows the same entry in mode 8; what the two numbers select is not recovered). */
void gifStateEnableEffect(void)
{
    func_001BA6F4();
    gifStateSetEffectAlpha(0, 0x80);
    gifStateSetDecrease(0.01334f);
    gifStateSetFbp(1);
    nmlModelUseSubWindow(0, 0);
    gifStatus.state = GIF_EFFECT_FRAME;
    gifStatus.ratio = 1.0f;
    gifStateEffectHook = gifStateEffectFrame;
    msgPanelShow(0x151, 0x4E);
}

void nmlModelEnableSubWindow0(void) {
    nmlModelUseSubWindow(0U, 1);
}


int gifStateEffectReady(void) {
    if (gifStateEffectHook != 0 && gifStatus.state == GIF_EFFECT_FRAME) {
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

/* The colour of the white screen effect (RGB). */
#define GIF_STATE_WHITE 0xFFFFFF

/* Starts the white screen effect for an actor and sets up its move state (XenoActorUpdateMoveState, state 3). */
void gifStateStartWhiteShake(struct XenoActor *actor)
{
    func_00246DD8(GIF_STATE_WHITE);
    gifStateSetCenter(0x100, 0xE0);
    gifStateSetEffectAlpha(0, 100);
    gifStateSetDecrease(0.006f);
    gifStateSetFbp(1);
    gifStateSetBlur(1);
    nmlModelUseSubWindow(0, 0);
    nmlModelUseSubWindow(1, 0);
    gifStateSetMode(GIF_EFFECT_RADIAL_BLUR);
    actor->moveAngle = -0.6f;
    actor->shadowValue = 3.019999f;
    actor->moveVelocity = 0.6f;
    actor->shadowCount = 4;
    actor->shadowValue3 = 1.0f;
    actor->moveState = 3;
}

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
