#include "common.h"
#include "shared.h"

void xglDirectQueueBegin(void);
void xglDirectQueuePush(int, int);
void func_001D33D8(void);
extern u16 sRenderFrameBase;

/* grSetFrameBase: FRAME_1 (GS register 0x4C) with the frame base in sRenderFrameBase (sRenderFrameBase) and
   FBW 8, a 512-pixel-wide frame. */
void grSetFrameBase(void) {
    xglDirectQueueBegin();
    xglDirectQueuePush(0x4C, sRenderFrameBase | 0x80000);
    func_001D33D8();
}

/* grSetRenderFrame: FRAME_1 built from sRender: base at +0x20, FBW = width (+0x04) / 64, PSM at +0x02. */
INCLUDE_ASM("asm/main/nonmatchings/tu093", grSetRenderFrame);

void func_001CEC98(void);
void func_001D0D28(u16, int);

void grResetRenderState(void) {
    func_001CEC98();
    func_001D0D28(sRenderFrameBase, 0xF);
}

/* grDrawScreenSprite(expand, alpha): sets TEST_1, TEX0_1, CLAMP_1, TEX1_1, ALPHA_1 (alpha in its FIX byte),
   RGBAQ 0x80808080, PRIM 0x156, texture coordinates 1..511 by 1..447 texels and one 512x448 textured sprite. The
   corners move out by `expand` in 1/16 pixel. */
INCLUDE_ASM("asm/main/nonmatchings/tu093", grDrawScreenSprite);

/* grDrawScreenFade(fade): fade in -0x80..0x80. The sprite grows by 2^24 / (0x800 - fade) - 0x2000 and its alpha
   is (0x80 - |fade|)^2 >> 7, so it is unexpanded and opaque at 0 and vanishes at +-0x80. */
INCLUDE_ASM("asm/main/nonmatchings/tu093", grDrawScreenFade);

/* grCopyFrame(dstBase, srcBase): local-to-local transfer of 512x448 from the source frame base to the
   destination frame base (BITBLTBUF, TRXPOS 0, TRXREG, TRXDIR 2). */
INCLUDE_ASM("asm/main/nonmatchings/tu093", grCopyFrame);

/* grFillFrame: sets the frame base, then draws one flat 512x448 blended sprite (RGBAQ 0x80808080, PRIM 0x46)
   over the whole frame, and finally sets the render frame through grSetRenderFrame. */
INCLUDE_ASM("asm/main/nonmatchings/tu093", grFillFrame);

/* grResetScissor: SCISSOR_1 = (0, 0) to (511, 447), the whole 512x448 frame. */
INCLUDE_ASM("asm/main/nonmatchings/tu093", grResetScissor);

/* grSetScissor(x0, y0, x1, y1): SCISSOR_1 from the two corners, each ordered and clamped to the frame
   (0..511 and 0..447). */
INCLUDE_ASM("asm/main/nonmatchings/tu093", grSetScissor);

/* grAddScissorPacket(packet, rect): a VIF direct-data packet (GIF tag and A+D with SCISSOR_1) from the four ints
   of rect (x0 +0x0, y0 +0x4, x1 +0x8, y1 +0xC), ordered and clamped to sRender's width (+0x04) and height (+0x06);
   sent through sceVif1PkAddDirectDataN. */
INCLUDE_ASM("asm/main/nonmatchings/tu093", grAddScissorPacket);
