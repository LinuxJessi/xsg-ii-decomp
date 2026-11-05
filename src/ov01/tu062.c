#include "common.h"
#include "shared.h"

/*
 * grGpInit's argument, restated from src/ov01/gr_gp_init.c (the owner keeps
 * it TU-local; header proposal in the PR): a data pointer and a length. A
 * local packet must be the whole record, because grGpInit writes both words.
 */
typedef struct GrPacket {
    void *data;
    int count;
} GrPacket;

typedef struct MenuRGBA {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} MenuRGBA;

/* The sprite colour: menuStatSpriteRGB copies a caller's MenuRGBA into it
 * byte-wise (lwl/lwr, swl/swr), while menuStatSprite/menuStatSprite2 copy the
 * whole word from an aligned local (lw/sw), so the field is word-aligned. */
typedef union MenuColor {
    unsigned int word;
    MenuRGBA rgba;
} MenuColor;

/* grPutSpr's sprite record: two corner vertices, a colour and the texel rectangle. */
typedef struct GrSprite {
    float x0, y0, z0, w0; /* +0x00 */
    float x1, y1, z1, w1; /* +0x10 */
    MenuColor color;      /* +0x20 */
    short u0;             /* +0x24 */
    short u1;             /* +0x26 */
    short v0;             /* +0x28 */
    short v1;             /* +0x2A */
} GrSprite;

#define SPRITE_Z 16777212.0f

extern void grPutSpr(GrPacket *packet, GrSprite *sprite, int flags);

INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA70E8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA7150);

INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA71E0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA7240);

INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA72A0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA7360);

INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA73C8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA73F8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA74D0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA7568);

INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA76F8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA7750);

INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA7770);

INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA7900);

INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA7998);

INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA79B0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA7A48);

INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA7A60);

INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA7C50);

INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA7C70);

INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA7D00);

INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA7EC8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA7F30);

INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA7FE0);

void menuStatSpriteRGB(GrPacket *packet, int x, int y, int width, int height, int srcX, int srcY, const MenuRGBA *rgba) {
    GrSprite sprite;

    sprite.x0 = x;
    sprite.y0 = y;
    sprite.z0 = SPRITE_Z;
    sprite.x1 = x + width;
    sprite.y1 = y + height;
    sprite.z1 = SPRITE_Z;
    sprite.u0 = srcX;
    sprite.u1 = srcX + width + 1;
    sprite.v0 = srcY;
    sprite.v1 = srcY + height + 1;
    sprite.color.rgba = *rgba;
    grPutSpr(packet, &sprite, 0);
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA8148);
