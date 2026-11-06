#include "common.h"
#include "main/tu101.h"

/* Font draw state word; the draw call in textDrawSegment takes it as its third argument. */
extern int D_0069D580;

void func_0021A520(void)
{
    D_0069D580 = 0xFFFFF0;
}

/* One text segment: optional colour and alpha control tags, then the draw call. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", textDrawSegment);

/* Multi-line text: a backslash-n escape starts a new line, textCursorY is the y cursor. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", textDrawLines);

int fontObjDraw(int *block);
/* Shared request block that fontObjDraw (packet builder) reads. */
extern int packetRequest[8];

/* Fills the request's command word, two operands and flags (bits 3 and 5 cleared), then submits it. */
void packetSubmitRequest(int command, int second, int third, int flags)
{
    packetRequest[0] = command;
    packetRequest[1] = second;
    packetRequest[2] = third;
    packetRequest[7] = flags & ~0x28;
    fontObjDraw(packetRequest);
}
