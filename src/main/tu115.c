#include "common.h"
#include "shared.h"
#include "gs.h"

typedef struct XglPacket XglPacket;

XglPacket *xglPacketGetCurrent(void);
void sceVif1PkCnt(XglPacket *packet, int value);
void sceVif1PkOpenDirectHLCode(XglPacket *packet, int value);
void sceVif1PkCloseDirectHLCode(XglPacket *packet);
void gifStateReset(void);
void gifStateAddReg(int gifAddress, u64 gifData);
void gifStateFlush(void);
void xglFontFlush(void);

/* Partial view of sRender: the work page at +0x1A and the draw page at +0x20. */
typedef struct RenderSpriteState {
    unsigned char unmodeled_00[0x1A];
    unsigned short workBase;
    unsigned char unmodeled_1c[4];
    unsigned short drawBase;
} RenderSpriteState;

extern RenderSpriteState sRender;
extern int renderOriginX;
extern int renderOriginY;

/* GS RGBAQ.Q of 1.0f (the bit pattern of the float) and the texture modulation value that leaves a texel unchanged (0x80). */
#define GS_Q_ONE 0x3F800000
#define GS_TEX_MODULATE_ONE 0x80

/* Draws a five-pass triangle strip of fillColour from (x0, y0) to (x1, y1), with transparent outer columns at both ends. */
void renderFadeBand(int x0, int y0, int x1, int y1, u32 fillColour) {
    XglPacket *packet;
    int pass;
    u64 fadeRgbaq;
    u64 fillRgbaq;
    u8 red;
    u8 green;
    u8 blue;
    u8 alpha;

    green = fillColour >> 8;
    blue = fillColour >> 16;
    alpha = fillColour >> 24;
    packet = xglPacketGetCurrent();
    xglFontFlush();
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    gifStateReset();
    pass = 0;
    x0 = x0 + x1;
    gifStateAddReg(GS_TEX1_1, GS_SET_TEX1(0, 0, 1, 1, 0, 0, 0));
    gifStateAddReg(GS_ZBUF_1, GS_SET_ZBUF(0, 0x31, 1));
    gifStateAddReg(GS_TEST_1, GS_TEST_ZTE_ALWAYS);
    gifStateAddReg(GS_CLAMP_1, GS_SET_CLAMP(0, 0, 0, 0, 0, 0));
    gifStateAddReg(GS_ALPHA_1, GS_SET_ALPHA(0, 2, 0, 1, 0));
    gifStateAddReg(GS_PRIM, GS_SET_PRIM(4, 1, 0, 0, 1, 0, 0, 0, 0));
    fadeRgbaq = GS_SET_RGBAQ(0, 0, 0, 0, GS_Q_ONE);
    x0 += (unsigned int)x0 >> 31;
    x0 >>= 1;
    red = fillColour;
    fillRgbaq = GS_SET_RGBAQ(red, green, blue, alpha, GS_Q_ONE);
    for (; pass < 5; pass++) {
        gifStateAddReg(GS_RGBAQ, fadeRgbaq);
        gifStateAddReg(GS_XYZF2, GS_SET_XYZ(((renderOriginX + x0) << 4) - 0x198, ((renderOriginY + y0) << 4) - 9, 0xFFFFFF));
        gifStateAddReg(GS_RGBAQ, fadeRgbaq);
        gifStateAddReg(GS_XYZF2, GS_SET_XYZ(((renderOriginX + x0) << 4) - 0x198, ((renderOriginY + y1) << 4) - 9, 0xFFFFFF));
        gifStateAddReg(GS_RGBAQ, fillRgbaq);
        gifStateAddReg(GS_XYZF2, GS_SET_XYZ(((renderOriginX + x0) << 4) - 0x38, ((renderOriginY + y0) << 4) - 9, 0xFFFFFF));
        gifStateAddReg(GS_RGBAQ, fillRgbaq);
        gifStateAddReg(GS_XYZF2, GS_SET_XYZ(((renderOriginX + x0) << 4) - 0x38, ((renderOriginY + y1) << 4) - 9, 0xFFFFFF));
        gifStateAddReg(GS_RGBAQ, fillRgbaq);
        gifStateAddReg(GS_XYZF2, GS_SET_XYZ(((renderOriginX + x0) << 4) - 8, ((renderOriginY + y0) << 4) - 9, 0xFFFFFF));
        gifStateAddReg(GS_RGBAQ, fillRgbaq);
        gifStateAddReg(GS_XYZF2, GS_SET_XYZ(((renderOriginX + x0) << 4) - 8, ((renderOriginY + y1) << 4) - 9, 0xFFFFFF));
        gifStateAddReg(GS_RGBAQ, fillRgbaq);
        gifStateAddReg(GS_XYZF2, GS_SET_XYZ(((renderOriginX + x0) << 4) + 0x28, ((renderOriginY + y0) << 4) - 9, 0xFFFFFF));
        gifStateAddReg(GS_RGBAQ, fillRgbaq);
        gifStateAddReg(GS_XYZF2, GS_SET_XYZ(((renderOriginX + x0) << 4) + 0x28, ((renderOriginY + y1) << 4) - 9, 0xFFFFFF));
        gifStateAddReg(GS_RGBAQ, fadeRgbaq);
        gifStateAddReg(GS_XYZF2, GS_SET_XYZ(((renderOriginX + x1) << 4) - 8, ((renderOriginY + y0) << 4) - 9, 0xFFFFFF));
        gifStateAddReg(GS_RGBAQ, fadeRgbaq);
        gifStateAddReg(GS_XYZF2, GS_SET_XYZ(((renderOriginX + x1) << 4) - 8, ((renderOriginY + y1) << 4) - 9, 0xFFFFFF));
    }
    gifStateFlush();
    sceVif1PkCloseDirectHLCode(packet);
}

/* Draws a width x height textured sprite of the work page at (x, y) into the draw page, blended with the fixed alpha. */
void renderSprite(int x, int y, int width, int height, int alpha) {
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    gifStateReset();
    gifStateAddReg(GS_TEXFLUSH, 0);
    gifStateAddReg(GS_ZBUF_1, GS_SET_ZBUF(0, 0x31, 0));
    gifStateAddReg(GS_FBA_1, 1);
    gifStateAddReg(GS_TEST_1, GS_TEST_ZTE_ALWAYS);
    gifStateAddReg(GS_CLAMP_1, GS_SET_CLAMP(0, 0, 0, 0, 0, 0));
    gifStateAddReg(GS_ALPHA_1, GS_SET_ALPHA(0, 0, 0, 0, 0x80));
    gifStateAddReg(GS_FRAME_1, GS_SET_FRAME(sRender.drawBase, 8, GS_PSMCT32, 0));
    gifStateAddReg(GS_TEX0_1, GS_SET_TEX0(sRender.workBase << 5, 8, GS_PSMCT32, 8, 8, 1, 0, 0, 0, 0, 0, 0));
    gifStateAddReg(GS_RGBAQ, GS_SET_RGBAQ(GS_TEX_MODULATE_ONE, GS_TEX_MODULATE_ONE, GS_TEX_MODULATE_ONE, GS_TEX_MODULATE_ONE, GS_Q_ONE));
    gifStateAddReg(GS_ALPHA_1, GS_SET_ALPHA(0, 1, 2, 1, alpha));
    gifStateAddReg(GS_PRIM, GS_PRIM_TEXTURED_SPRITE);
    gifStateAddReg(GS_UV, GS_SET_UV(0, 0));
    gifStateAddReg(GS_XYZF2, GS_SET_XYZ(((renderOriginX + x) << 4) - 8, ((renderOriginY + y) << 4) - 9, 0xFFFFFF));
    gifStateAddReg(GS_UV, GS_SET_UV(width << 4, height << 4));
    gifStateAddReg(GS_XYZF2, GS_SET_XYZ(((renderOriginX + x + width) << 4) - 8, ((renderOriginY + y + height) << 4) - 9, 0xFFFFFF));
    gifStateFlush();
    sceVif1PkCloseDirectHLCode(packet);
}

/* Draws a width x height textured sprite of the work page at (x, y) into the draw page, read through the CLUT at clutOffset. */
void renderSpriteClut(int x, int y, int width, int height, int clutOffset) {
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    gifStateReset();
    gifStateAddReg(GS_TEXFLUSH, 0);
    gifStateAddReg(GS_ZBUF_1, GS_SET_ZBUF(0, 0x31, 0));
    gifStateAddReg(GS_FBA_1, 0);
    gifStateAddReg(GS_TEST_1, GS_TEST_ZTE_ALWAYS);
    gifStateAddReg(GS_CLAMP_1, GS_SET_CLAMP(0, 0, 0, 0, 0, 0));
    gifStateAddReg(GS_FRAME_1, GS_SET_FRAME(sRender.drawBase, 8, GS_PSMCT32, 0));
    gifStateAddReg(GS_TEX0_1, GS_SET_TEX0(sRender.workBase << 5, 16, GS_PSMT8, 9, 9, 0, 0, clutOffset + 0x3800, 0, 0, 0, 1));
    gifStateAddReg(GS_TEX1_1, GS_SET_TEX1(0, 0, 0, 0, 0, 0, 0));
    gifStateAddReg(GS_RGBAQ, GS_SET_RGBAQ(GS_TEX_MODULATE_ONE, GS_TEX_MODULATE_ONE, GS_TEX_MODULATE_ONE, GS_TEX_MODULATE_ONE, GS_Q_ONE));
    gifStateAddReg(GS_ALPHA_1, GS_SET_ALPHA(0, 1, 1, 1, 0));
    gifStateAddReg(GS_PRIM, GS_PRIM_TEXTURED_SPRITE);
    gifStateAddReg(GS_UV, GS_SET_UV(0, 0));
    gifStateAddReg(GS_XYZF2, GS_SET_XYZ(((renderOriginX + x) << 4) - 8, ((renderOriginY + y) << 4) - 9, 0xFFFFFF));
    gifStateAddReg(GS_UV, GS_SET_UV(width << 4, height << 4));
    gifStateAddReg(GS_XYZF2, GS_SET_XYZ(((renderOriginX + x + width) << 4) - 8, ((renderOriginY + y + height) << 4) - 9, 0xFFFFFF));
    gifStateFlush();
    sceVif1PkCloseDirectHLCode(packet);
}

/* Draws a width x height textured sprite of the work page at (x, y) into the draw page, read through the CLUT at clutOffset, with the alpha in the vertex colour. */
void renderSpriteClutAlpha(int x, int y, int width, int height, int clutOffset, int alpha) {
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    gifStateReset();
    gifStateAddReg(GS_TEXFLUSH, 0);
    gifStateAddReg(GS_ZBUF_1, GS_SET_ZBUF(0, 0x31, 0));
    gifStateAddReg(GS_FBA_1, 0);
    gifStateAddReg(GS_TEST_1, GS_TEST_ZTE_ALWAYS);
    gifStateAddReg(GS_CLAMP_1, GS_SET_CLAMP(0, 0, 0, 0, 0, 0));
    gifStateAddReg(GS_FRAME_1, GS_SET_FRAME(sRender.drawBase, 8, GS_PSMCT32, 0xFFFFFF));
    gifStateAddReg(GS_TEX0_1, GS_SET_TEX0(sRender.workBase << 5, 16, GS_PSMT8, 10, 8, 1, 0, clutOffset + 0x3800, 0, 0, 0, 1));
    gifStateAddReg(GS_TEX1_1, GS_SET_TEX1(0, 0, 0, 0, 0, 0, 0));
    gifStateAddReg(GS_RGBAQ, GS_SET_RGBAQ(GS_TEX_MODULATE_ONE, GS_TEX_MODULATE_ONE, GS_TEX_MODULATE_ONE, alpha, GS_Q_ONE));
    gifStateAddReg(GS_ALPHA_1, GS_SET_ALPHA(0, 1, 0, 1, 0));
    gifStateAddReg(GS_PRIM, GS_PRIM_TEXTURED_SPRITE);
    gifStateAddReg(GS_UV, GS_SET_UV(0, 0));
    gifStateAddReg(GS_XYZF2, GS_SET_XYZ(((renderOriginX + x) << 4) - 8, ((renderOriginY + y) << 4) - 9, 0xFFFFFF));
    gifStateAddReg(GS_UV, GS_SET_UV(width << 4, height << 4));
    gifStateAddReg(GS_XYZF2, GS_SET_XYZ(((renderOriginX + x + width) << 4) - 8, ((renderOriginY + y + height) << 4) - 9, 0xFFFFFF));
    gifStateFlush();
    sceVif1PkCloseDirectHLCode(packet);
}

unsigned char yg2PathImageFade = 0;
int renderOriginX = 1792;
int renderOriginY = 0x720;
unsigned char D_0069D7F0[8] = { 0 };
unsigned char D_0069D7F8[8] = "pBuf";
unsigned char D_0069D800[8] = "Sex:%s\n";
unsigned char D_0069D808[8] = "Age:%s\n";
unsigned char D_0069D810[8] = "pPxy";
unsigned char D_0069D818[8] = "pTxy";
unsigned char D_0069D820[8] = "pXhr";
unsigned char D_0069D828[8] = "pXap";
int D_0069D830 = (int)0x80000000;
int D_0069D834 = 131071;
unsigned char D_0069D838[8] = "Subject";
unsigned char D_0069D840[8] = "Message";
unsigned char D_0069D848[8] = "Client";
unsigned char D_0069D850[8] = "Data";
unsigned char D_0069D858[8] = "Profile";
unsigned char D_0069D860[8] = "Name:";
unsigned char D_0069D868[8] = "Sex:";
unsigned char D_0069D870[8] = "Age:";
unsigned char D_0069D878[8] = "Hobby:";
char D_0069D880[8] = "\xd0\xb0h";
