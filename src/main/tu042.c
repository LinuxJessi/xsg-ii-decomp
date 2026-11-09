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
