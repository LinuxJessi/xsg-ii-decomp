#include "common.h"
#include "shared.h"
#include "main/tu206.h"

struct XglPacket;
struct XglPacket *xglPacketGetCurrent(void);
void sceVif1PkCnt(struct XglPacket *, int);
void sceVif1PkOpenDirectHLCode(struct XglPacket *, int);
void sceVif1PkAddDirectDataN(struct XglPacket *, const void *, int);
void sceVif1PkCloseDirectHLCode(struct XglPacket *);
extern struct XglPacket *xglScratchPacket;
extern long long *xglScratchQuads;
extern int xglScratchQuadCount;

/* Starts a scratchpad quadword batch (proposed name xglPacketScratchBegin): the current packet goes to
 * xglScratchPacket, the batch base is the EE scratchpad at 0x70000000 and the count is reset to zero. */
void xglPacketScratchBegin(void)
{
    xglScratchPacket = xglPacketGetCurrent();
    xglScratchQuads = (long long *)0x70000000;
    xglScratchQuadCount = 0;
}

/* Appends one 128-bit quadword (high, low) to the batch (proposed name xglPacketScratchAddQuad). */
void xglPacketScratchAddQuad(long long high, long long low)
{
    int newCount;
    long long *slot;

    newCount = xglScratchQuadCount + 1;
    slot = (long long *)(xglScratchQuadCount * 16 + (int)xglScratchQuads);
    slot[1] = high;
    slot[0] = low;
    xglScratchQuadCount = newCount;
}

/* Adds count - 1 to the first doubleword of the batch, the tag written at its head (proposed name
 * xglPacketScratchPatchTag). */
void xglPacketScratchPatchTag(void)
{
    *xglScratchQuads += xglScratchQuadCount - 1;
}

/* Appends the batch to the current packet as a VIF1 direct HL packet (proposed name
 * xglPacketScratchAddDirect). */
void xglPacketScratchAddDirect(void)
{
    sceVif1PkCnt(xglScratchPacket, 0);
    sceVif1PkOpenDirectHLCode(xglScratchPacket, 0);
    sceVif1PkAddDirectDataN(xglScratchPacket, xglScratchQuads, xglScratchQuadCount);
    sceVif1PkCloseDirectHLCode(xglScratchPacket);
}

/* Caps value at upper, then raises it to lower (proposed name xglClampInt). */
int xglClampInt(int value, int lower, int upper)
{
    int clamped = value;

    if (upper < value) {
        clamped = upper;
    }
    return (clamped < lower) ? lower : clamped;
}
