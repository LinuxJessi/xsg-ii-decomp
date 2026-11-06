#include "common.h"
#include "main/tu045.h"

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

extern s32 D_0069E11C;

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

extern s32 D_0069E0F4;

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

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001CD254);

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
void sceVif1PkCnt(XglPacket *packet, int count);
XglPacket *xglPacketGetCurrent(void);

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

INCLUDE_ASM("asm/main/nonmatchings/tu045", func_001D19A0);

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
