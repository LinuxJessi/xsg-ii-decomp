#include "common.h"
#include "main/tu102.h"

/* The request block shared with fontObjDraw (packet builder, main/tu097); the same
   object is viewed as int[8] in tu101 and as Tu104Request in tu104.h. Layouts disagree. */
typedef struct PacketRequest {
    int command;
    int second;
    int third;
    unsigned char unmodeled_0c[8];
    int fourth;
    int fifth;
    int flags;
} PacketRequest;

extern PacketRequest packetRequest;
int fontObjDraw(PacketRequest *block);

/* Like packetSubmitRequest with two more operands; bit 3 of flags set, bit 5 cleared. */
void packetSubmitRequestEx(int command, int second, int third, int fourth, int fifth, int flags)
{
    packetRequest.command = command;
    packetRequest.second = second;
    packetRequest.third = third;
    packetRequest.fourth = fourth;
    packetRequest.fifth = fifth;
    packetRequest.flags = (flags | 8) & ~0x20;
    fontObjDraw(&packetRequest);
}

/* Text run: maps each character to a glyph kind (letters, digits, symbols) and submits one request per glyph, advancing x by the glyph width. */
INCLUDE_ASM("asm/main/nonmatchings/tu102", textDrawGlyphString);

/* Text run drawn at (x + 1, y + 3) after setting a state word with zero colour bytes; the previous state is restored. */
INCLUDE_ASM("asm/main/nonmatchings/tu102", textDrawShadow);

/* Text run with the second glyph bank (kinds 0x7A to 0x7F). */
INCLUDE_ASM("asm/main/nonmatchings/tu102", textDrawStringAlt);

/* Shadowed text run over the second glyph bank. */
INCLUDE_ASM("asm/main/nonmatchings/tu102", textDrawShadowAlt);

/* Text run with a leading mode word (0x10034 or 0x74 in the request flags). */
INCLUDE_ASM("asm/main/nonmatchings/tu102", func_0021AFE8);

/* Formats an integer in decimal into a static buffer with width and zero-pad flag; returns the buffer. */
INCLUDE_ASM("asm/main/nonmatchings/tu102", textFormatDecimal);

/* Decimal number at (x, y): width 1 to 9, 9-pixel character spacing, space padding. */
INCLUDE_ASM("asm/main/nonmatchings/tu102", textDrawDecimal);

/* textDrawDecimalAlt variant: second glyph bank, width up to 10, 7-pixel spacing. */
INCLUDE_ASM("asm/main/nonmatchings/tu102", textDrawDecimalAlt);

/* textDrawDecimal with zero padding. */
INCLUDE_ASM("asm/main/nonmatchings/tu102", textDrawDecimalZero);

/* Decimal number through the shadowed text run. */
INCLUDE_ASM("asm/main/nonmatchings/tu102", textDrawDecimalShadow);

/* textDrawDecimalAlt through the shadowed second-bank run. */
INCLUDE_ASM("asm/main/nonmatchings/tu102", textDrawDecimalAltShadow);

/* textDrawDecimalShadow with zero padding. */
INCLUDE_ASM("asm/main/nonmatchings/tu102", textDrawDecimalShadowZero);

/* Signed decimal (leading '+' or '-') through the shadowed text run. */
INCLUDE_ASM("asm/main/nonmatchings/tu102", textDrawSignedDecimalShadow);
