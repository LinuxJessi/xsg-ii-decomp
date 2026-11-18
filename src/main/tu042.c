#include "common.h"
#include "shared.h"
#include "gs.h"

extern u8 *pxyGlyphDigits[];
extern u8 *pxyGlyphUpper[];
extern u8 *pxyGlyphLower[];
extern u8 pxyGlyphUnderscore[];
extern u8 pxyGlyphPeriod[];
extern u8 pxyGlyphSlash[];
extern u8 pxyGlyphParenOpen[];
extern u8 pxyGlyphParenClose[];
extern u8 D_006458A4[];
extern u16 sRenderDepthBase;
extern u8 *xglDirectQueueBuffer;
extern int xglDirectQueueCount;
void xglDirectQueueBegin(void);
void xglDirectQueueSubmitRef(void);
void xglDirectQueueDefer(int, int);

u8 pxyDrawChar(u8 ch, int x, int y, float scale, u32 color, u32 z, int flush) {
    u8 advance = 0;
    u8 *glyph = 0;
    int code = ch;

    if (code >= 48 && code < 58) {
        glyph = pxyGlyphDigits[code - 48];
    } else if (code >= 65 && code < 91) {
        glyph = pxyGlyphUpper[code - 65];
    } else if (code >= 97 && code < 123) {
        glyph = pxyGlyphLower[code - 97];
    } else if (code == 46) {
        glyph = pxyGlyphPeriod;
    } else if (code == 47) {
        glyph = pxyGlyphSlash;
    } else if (code == 95) {
        glyph = pxyGlyphUnderscore;
    } else if (code == 40) {
        glyph = pxyGlyphParenOpen;
    } else if (code == 41) {
        glyph = pxyGlyphParenClose;
    } else if (code == 32) {
        return 7;
    }
    if (glyph == 0) {
        return advance;
    }
    advance = *glyph++;

    if (flush != 0) {
        u64 *cursor;
        u64 *packetStart;

        xglDirectQueueBegin();
        cursor = (u64 *)(xglDirectQueueBuffer + xglDirectQueueCount * 16);
        packetStart = cursor;
        cursor[0] = 0x50000;
        cursor[1] = 0x47;
        cursor += 2;
        GS_PUT(cursor, GS_ZBUF_1, GS_SET_ZBUF(sRenderDepthBase, 1, 0));
        GS_PUT(cursor, GS_RGBAQ, GS_SET_RGBAQ(color & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF, color >> 24, 0x3F800000));
        GS_PUT(cursor, GS_PRIM, 2);
        while (*glyph != 0xFF) {
            if (*glyph == 0xFE) {
                GS_PUT(cursor, GS_PRIM, 2);
            } else {
                GS_PUT(cursor, GS_XYZF2, GS_SET_XYZ(((int)((float)glyph[0] * scale) + x + 0x700) << 4, ((int)((float)glyph[1] * scale) + y + 0x720) << 4, z));
            }
            glyph += 2;
        }
        xglDirectQueueCount += ((u32)cursor - (u32)packetStart) >> 4;
        xglDirectQueueSubmitRef();
    } else {
        u64 *cursor;
        u64 *packetStart;

        xglDirectQueueBegin();
        cursor = (u64 *)(xglDirectQueueBuffer + xglDirectQueueCount * 16);
        packetStart = cursor;
        cursor[0] = 0x50000;
        cursor[1] = 0x47;
        cursor += 2;
        GS_PUT(cursor, GS_ZBUF_1, GS_SET_ZBUF(sRenderDepthBase, 1, 0));
        GS_PUT(cursor, GS_RGBAQ, GS_SET_RGBAQ(color & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF, color >> 24, 0x3F800000));
        GS_PUT(cursor, GS_PRIM, 2);
        while (*glyph != 0xFF) {
            if (*glyph == 0xFE) {
                GS_PUT(cursor, GS_PRIM, 2);
            } else {
                GS_PUT(cursor, GS_XYZF2, GS_SET_XYZ(((int)((float)glyph[0] * scale) + x + 0x700) << 4, ((int)((float)glyph[1] * scale) + y + 0x720) << 4, z));
            }
            glyph += 2;
        }
        xglDirectQueueCount += ((u32)cursor - (u32)packetStart) >> 4;
        xglDirectQueueDefer(0, 10);
    }
    return advance;
}

unsigned int strlen(const char *);

int pxyDrawString(const char *str, int x, int y, float scale, u32 color, u32 depth, int flush) {
    int i;
    int advanceTotal = 0;

    for (i = 0; i < strlen(str); i++) {
        advanceTotal += pxyDrawChar(str[i], (int)((float)x + (float)advanceTotal * scale), y, scale, color, depth, flush);
    }
    return advanceTotal;
}

int pxyDrawStringShadow(const char *str, int x, int y, float scale, u32 color, u32 depth, int flush) {
    int i;
    int advanceTotal = 0;

    for (i = 0; i < strlen(str); i++) {
        advanceTotal += pxyDrawChar(str[i], (int)((float)(x + 1) + (float)advanceTotal * scale), y + 1, scale, 0, depth, flush);
    }
    advanceTotal = 0;
    for (i = 0; i < strlen(str); i++) {
        advanceTotal += pxyDrawChar(str[i], (int)((float)x + (float)advanceTotal * scale), y, scale, color, depth, flush);
    }
    return advanceTotal;
}

/* pxyTextColor and pxyTextScale are read by pxyPrintf, which passes them on to pxyDrawString. */
int pxyTextColor = -1;
float pxyTextScale = 1.0f;
int pxyTextDepth = 0xFFFFFF;

void pxySetTextStyle(int color, float scale, int depth) {
    pxyTextColor = color;
    pxyTextScale = scale;
    pxyTextDepth = depth;
}

typedef char *va_list;

extern int vsprintf(char *buffer, const char *format, va_list args);

void pxyPrintf(int x, int y, const char *format, ...) {
    char buffer[512];
    va_list args;

    args = (va_list)__builtin_next_arg(format) - (8 - 3) * 8;
    vsprintf(buffer, format, args);
    pxyDrawString(buffer, x, y, pxyTextScale, pxyTextColor, pxyTextDepth, 1);
}

/* Glyph stroke data of the lower-case and digit-tail blocks, with the lower-case pointer table. */
u8 pxyGlyphUnderscore[] = {
    7, 0, 7, 6, 7, 0, 7, 255
};
u8 pxyGlyphPeriod[16] = {
    7, 2, 6, 3, 6, 3, 7, 2,
    7, 2, 6, 255, 0, 0, 0, 0
};
u8 pxyGlyphSlash[] = {
    7, 5, 1, 1, 7, 5, 1, 255
};
u8 pxyGlyphParenOpen[16] = {
    7, 4, 1, 3, 2, 3, 6, 4,
    7, 3, 6, 255, 0, 0, 0, 0
};
u8 pxyGlyphParenClose[16] = {
    7, 2, 1, 3, 2, 3, 6, 2,
    7, 3, 6, 255, 0, 0, 0, 0
};
u8 D_006459E0[24] = {
    7, 1, 7, 1, 3, 3, 1, 5,
    3, 5, 7, 5, 5, 1, 5, 5,
    5, 255, 0, 0, 0, 0, 0, 0
};
u8 D_006459F8[32] = {
    7, 1, 7, 1, 1, 4, 1, 5,
    2, 5, 3, 4, 4, 1, 4, 4,
    4, 5, 5, 5, 6, 4, 7, 1,
    7, 255, 0, 0, 0, 0, 0, 0
};
u8 D_00645A18[24] = {
    7, 5, 2, 4, 1, 2, 1, 1,
    2, 1, 6, 2, 7, 4, 7, 5,
    6, 4, 7, 255, 0, 0, 0, 0
};
u8 D_00645A30[24] = {
    7, 1, 7, 1, 1, 4, 1, 5,
    2, 5, 6, 4, 7, 1, 7, 4,
    7, 255, 0, 0, 0, 0, 0, 0
};
u8 D_00645A48[24] = {
    7, 5, 1, 1, 1, 1, 7, 5,
    7, 1, 7, 254, 254, 1, 4, 4,
    4, 1, 4, 255, 0, 0, 0, 0
};
u8 D_00645A60[16] = {
    7, 5, 1, 1, 1, 1, 7, 1,
    4, 4, 4, 1, 4, 255, 0, 0
};
u8 D_00645A70[24] = {
    7, 5, 2, 4, 1, 2, 1, 1,
    2, 1, 6, 2, 7, 4, 7, 5,
    6, 5, 4, 4, 4, 5, 4, 255
};
u8 D_00645A88[16] = {
    7, 1, 1, 1, 7, 1, 4, 5,
    4, 5, 1, 5, 7, 5, 1, 255
};
u8 D_00645A98[16] = {
    7, 2, 1, 4, 1, 3, 1, 3,
    7, 4, 7, 2, 7, 4, 7, 255
};
u8 D_00645AA8[16] = {
    7, 3, 1, 5, 1, 4, 1, 4,
    6, 3, 7, 2, 7, 1, 6, 255
};
u8 D_00645AB8[24] = {
    7, 1, 1, 1, 7, 1, 4, 2,
    4, 5, 1, 2, 4, 5, 7, 2,
    4, 255, 0, 0, 0, 0, 0, 0
};
u8 D_00645AD0[16] = {
    7, 1, 1, 1, 7, 5, 7, 1,
    7, 255, 0, 0, 0, 0, 0, 0
};
u8 D_00645AE0[16] = {
    7, 1, 7, 1, 1, 3, 7, 5,
    1, 5, 7, 5, 1, 255, 0, 0
};
u8 D_00645AF0[16] = {
    7, 1, 7, 1, 1, 5, 7, 5,
    1, 5, 7, 255, 0, 0, 0, 0
};
u8 D_00645B00[24] = {
    7, 1, 2, 1, 6, 2, 7, 4,
    7, 5, 6, 5, 2, 4, 1, 2,
    1, 1, 2, 255, 0, 0, 0, 0
};
u8 D_00645B18[24] = {
    7, 1, 7, 1, 1, 4, 1, 5,
    2, 5, 3, 4, 4, 1, 4, 4,
    4, 255, 0, 0, 0, 0, 0, 0
};
u8 D_00645B30[32] = {
    7, 1, 2, 1, 6, 2, 7, 4,
    7, 5, 6, 5, 2, 4, 1, 2,
    1, 1, 2, 254, 254, 3, 5, 5,
    7, 3, 5, 255, 0, 0, 0, 0
};
u8 D_00645B50[32] = {
    7, 1, 7, 1, 1, 4, 1, 5,
    2, 5, 3, 4, 4, 1, 4, 4,
    4, 254, 254, 2, 4, 5, 7, 2,
    4, 255, 0, 0, 0, 0, 0, 0
};
u8 D_00645B70[32] = {
    7, 5, 2, 4, 1, 2, 1, 1,
    2, 1, 3, 2, 4, 4, 4, 5,
    5, 5, 6, 4, 7, 2, 7, 1,
    6, 2, 7, 255, 0, 0, 0, 0
};
u8 D_00645B90[16] = {
    7, 1, 1, 5, 1, 3, 1, 3,
    7, 3, 1, 255, 0, 0, 0, 0
};
u8 D_00645BA0[16] = {
    7, 1, 1, 1, 6, 2, 7, 4,
    7, 5, 6, 5, 1, 5, 6, 255
};
u8 D_00645BB0[16] = {
    7, 1, 1, 3, 7, 5, 1, 3,
    7, 255, 0, 0, 0, 0, 0, 0
};
u8 D_00645BC0[16] = {
    7, 1, 1, 2, 7, 3, 1, 4,
    7, 5, 1, 4, 7, 255, 0, 0
};
u8 D_00645BD0[16] = {
    7, 1, 1, 5, 7, 1, 1, 254,
    254, 1, 7, 5, 1, 1, 7, 255
};
u8 D_00645BE0[16] = {
    7, 1, 1, 3, 4, 3, 7, 3,
    4, 5, 1, 3, 4, 255, 0, 0
};
u8 D_00645BF0[16] = {
    7, 1, 1, 5, 1, 1, 7, 5,
    7, 1, 7, 255, 0, 0, 0, 0
};
u8 *pxyGlyphDigits[10] = {
    D_006458A4 + 4, D_006458A4 + 36, D_006458A4 + 52, D_006458A4 + 76,
    D_006458A4 + 108, D_006458A4 + 124, D_006458A4 + 148, D_006458A4 + 172,
    D_006458A4 + 188, D_006458A4 + 228
};
u8 *pxyGlyphUpper[26] = {
    D_006459E0, D_006459F8, D_00645A18, D_00645A30,
    D_00645A48, D_00645A60, D_00645A70, D_00645A88,
    D_00645A98, D_00645AA8, D_00645AB8, D_00645AD0,
    D_00645AE0, D_00645AF0, D_00645B00, D_00645B18,
    D_00645B30, D_00645B50, D_00645B70, D_00645B90,
    D_00645BA0, D_00645BB0, D_00645BC0, D_00645BD0,
    D_00645BE0, D_00645BF0
};
u8 D_00645C90[24] = { 7, 2, 2, 4, 2, 5, 3, 5, 7, 2, 7, 1, 6, 1, 5, 2, 4, 5, 4, 255, 0, 0, 0, 0 };
u8 D_00645CA8[16] = { 7, 1, 1, 1, 7, 4, 7, 5, 6, 5, 5, 4, 4, 1, 4, 255 };
u8 D_00645CB8[24] = { 7, 5, 4, 4, 3, 2, 3, 1, 4, 1, 6, 2, 7, 4, 7, 5, 6, 4, 7, 255, 0, 0, 0, 0 };
u8 D_00645CD0[16] = { 7, 5, 1, 5, 7, 2, 7, 1, 6, 1, 5, 2, 4, 5, 4, 255 };
u8 D_00645CE0[24] = { 7, 1, 5, 5, 5, 5, 4, 4, 3, 2, 3, 1, 4, 1, 6, 2, 7, 5, 7, 2, 7, 255, 0, 0 };
u8 D_00645CF8[24] = { 7, 5, 1, 4, 1, 3, 2, 3, 7, 3, 3, 1, 3, 5, 3, 4, 3, 255, 0, 0, 0, 0, 0, 0 };
u8 D_00645D10[24] = { 7, 2, 8, 4, 8, 5, 7, 5, 4, 4, 3, 2, 3, 1, 4, 1, 5, 2, 6, 5, 6, 255, 0, 0 };
u8 D_00645D28[16] = { 7, 1, 0, 1, 7, 1, 3, 4, 3, 5, 4, 5, 7, 5, 4, 255 };
u8 D_00645D38[16] = { 7, 3, 7, 3, 4, 3, 7, 254, 254, 3, 2, 3, 1, 255, 0, 0 };
u8 D_00645D48[24] = { 7, 1, 7, 2, 7, 3, 6, 3, 4, 3, 6, 254, 254, 3, 2, 3, 1, 255, 0, 0, 0, 0, 0, 0 };
u8 D_00645D60[32] = { 7, 1, 1, 1, 7, 1, 5, 2, 5, 3, 4, 4, 4, 5, 3, 4, 4, 254, 254, 2, 5, 3, 6, 4, 6, 5, 7, 4, 6, 255, 0, 0 };
u8 D_00645D80[16] = { 7, 3, 1, 3, 6, 4, 7, 3, 6, 255, 0, 0, 0, 0, 0, 0 };
u8 D_00645D90[32] = { 7, 1, 7, 1, 3, 2, 3, 3, 4, 4, 3, 5, 4, 5, 7, 5, 6, 254, 254, 3, 4, 3, 7, 3, 4, 255, 0, 0, 0, 0, 0, 0 };
u8 D_00645DB0[16] = { 7, 1, 7, 1, 3, 4, 3, 5, 4, 5, 7, 5, 4, 255, 0, 0 };
u8 D_00645DC0[24] = { 7, 1, 6, 2, 7, 4, 7, 5, 6, 5, 4, 4, 3, 2, 3, 1, 4, 1, 6, 255, 0, 0, 0, 0 };
u8 D_00645DD8[16] = { 7, 1, 8, 1, 3, 4, 3, 5, 4, 5, 5, 4, 6, 1, 6, 255 };
u8 D_00645DE8[16] = { 7, 5, 8, 5, 3, 2, 3, 1, 4, 1, 5, 2, 6, 5, 6, 255 };
u8 D_00645DF8[16] = { 7, 1, 3, 1, 7, 1, 5, 3, 3, 5, 3, 3, 3, 255, 0, 0 };
u8 D_00645E08[24] = { 7, 5, 3, 2, 3, 1, 4, 2, 5, 4, 5, 5, 6, 4, 7, 1, 7, 4, 7, 255, 0, 0, 0, 0 };
u8 D_00645E20[16] = { 7, 3, 1, 3, 7, 3, 3, 1, 3, 5, 3, 1, 3, 255, 0, 0 };
u8 D_00645E30[16] = { 7, 1, 3, 1, 6, 2, 7, 5, 7, 5, 3, 5, 7, 255, 0, 0 };
u8 D_00645E40[16] = { 7, 1, 3, 1, 5, 3, 7, 5, 5, 5, 3, 5, 5, 255, 0, 0 };
u8 D_00645E50[32] = { 7, 1, 3, 1, 6, 2, 7, 3, 6, 4, 7, 5, 6, 5, 3, 5, 4, 254, 254, 3, 3, 3, 6, 3, 3, 255, 0, 0, 0, 0, 0, 0 };
u8 D_00645E70[16] = { 7, 1, 3, 5, 7, 1, 3, 254, 254, 1, 7, 5, 3, 1, 7, 255 };
u8 D_00645E80[16] = { 7, 1, 3, 3, 7, 5, 3, 3, 7, 2, 8, 1, 8, 2, 8, 255 };
u8 D_00645E90[16] = { 7, 1, 3, 5, 3, 4, 4, 1, 7, 5, 7, 1, 7, 255, 0, 0 };
u8 *pxyGlyphLower[28] = { D_00645C90, D_00645CA8, D_00645CB8, D_00645CD0, D_00645CE0, D_00645CF8, D_00645D10, D_00645D28, D_00645D38, D_00645D48, D_00645D60, D_00645D80, D_00645D90, D_00645DB0, D_00645DC0, D_00645DD8, D_00645DE8, D_00645DF8, D_00645E08, D_00645E20, D_00645E30, D_00645E40, D_00645E50, D_00645E70, D_00645E80, D_00645E90, 0, 0 };
