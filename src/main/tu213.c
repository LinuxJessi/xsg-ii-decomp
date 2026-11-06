#include "common.h"
#include "main/tu213.h"

/* The direct GS queue: each gsQueue emitter appends one qword (GS register address in the high word, data in the
   low word) at gsQueueBase[gsQueueCount]. gsQueueFlush sends the queued qwords through the VIF1 direct-HL path. */

INCLUDE_ASM("asm/main/nonmatchings/tu213", func_0027A960);

/* gsQueueBeginTriStrip: starts a batch in the scratchpad packet (GIF tag, PRIM, state) and stores itself as the begin handler. */
INCLUDE_ASM("asm/main/nonmatchings/tu213", gsQueueBeginTriStrip);

/* gsQueueAddPrimTriStrip: PRIM for a triangle strip, textured when the argument is nonzero. */
INCLUDE_ASM("asm/main/nonmatchings/tu213", gsQueueAddPrimTriStrip);

/* gsQueueBeginTriFan: as gsQueueBeginTriStrip with PRIM type 5 (triangle fan). */
INCLUDE_ASM("asm/main/nonmatchings/tu213", gsQueueBeginTriFan);

/* gsQueueBeginLines: as gsQueueBeginTriStrip with PRIM type 1 (line). */
INCLUDE_ASM("asm/main/nonmatchings/tu213", gsQueueBeginLines);

/* gsQueueBeginSprites: as gsQueueBeginTriStrip with PRIM type 6 (sprite). */
INCLUDE_ASM("asm/main/nonmatchings/tu213", gsQueueBeginSprites);

/* gsQueueAddTexturedQuad: four vertices, UV and XYZ qwords; the second row carries the kick. */
INCLUDE_ASM("asm/main/nonmatchings/tu213", gsQueueAddTexturedQuad);

/* gsQueueAddTriangle: three vertices, the third is the kick; UV first when gsQueueTextured is set. */
INCLUDE_ASM("asm/main/nonmatchings/tu213", gsQueueAddTriangle);

/* gsQueueBeginPoints: as gsQueueBeginTriStrip with PRIM type 0 (point). */
INCLUDE_ASM("asm/main/nonmatchings/tu213", gsQueueBeginPoints);

/* gsQueueAddTexturedKickVertex: one kick vertex, UV first when textured, flush at 901 qwords (0x385). */
INCLUDE_ASM("asm/main/nonmatchings/tu213", gsQueueAddTexturedKickVertex);

/* gsQueueAddTexturedVertex: one vertex without kick, UV first when textured. */
INCLUDE_ASM("asm/main/nonmatchings/tu213", gsQueueAddTexturedVertex);

/* gsQueueAddKickVertex: one untextured kick vertex; flushes and restarts the batch when the queue is full. */
INCLUDE_ASM("asm/main/nonmatchings/tu213", gsQueueAddKickVertex);

/* gsQueueAddVertex: one untextured vertex without kick. */
INCLUDE_ASM("asm/main/nonmatchings/tu213", gsQueueAddVertex);

typedef signed char s8;

typedef int s32;

/* Partial layout: only the words these callers read. Field 0x0C is the RGBA color, field 0x18 the halving flag. */
typedef struct GsQueueContext {
    unsigned char unmodeled_00[0xC];
    s32 color;
    unsigned char unmodeled_10[8];
    s32 halveRgb;
} GsQueueContext;

void gsQueueRestart(void *context);
void gsQueueFlush(int packet);
int gsQueueAddRgbaq(s32 color, s32 halveRgb);
extern int gsQueueCount;

/* gsQueueAddColor: appends an RGBAQ color; at 901 qwords it flushes and restarts the batch. */
void gsQueueAddColor(s32 packet, s32 color, GsQueueContext *context) {
    gsQueueAddRgbaq(color, context->halveRgb);
    if (gsQueueCount >= 0x385) {
        gsQueueFlush(packet);
        gsQueueRestart(context);
    }
}

/* gsQueueAddContextColor: as gsQueueAddColor, the color is taken from the context. */
void gsQueueAddContextColor(s32 packet, GsQueueContext *context) {
    gsQueueAddRgbaq(context->color, context->halveRgb);
    if (gsQueueCount >= 0x385) {
        gsQueueFlush(packet);
        gsQueueRestart(context);
    }
}

extern void (*gsQueueBeginHandler)(void *context);

/* gsQueueRestart: calls the batch-begin handler stored in gsQueueBeginHandler. */
void gsQueueRestart(void *context) {
    gsQueueBeginHandler(context);
}

int sceVif1PkAddDirectDataN();
int sceVif1PkCloseDirectHLCode();
int sceVif1PkCnt();
int sceVif1PkOpenDirectHLCode();
extern long long *gsQueueBase;

/* gsQueueFlush: the first qword counts the queued qwords, then the packet is sent in direct-HL mode. */
void gsQueueFlush(int packet) {
    *gsQueueBase += gsQueueCount - 1;
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    sceVif1PkAddDirectDataN(packet, gsQueueBase, gsQueueCount);
    sceVif1PkCloseDirectHLCode(packet);
}

/* gsQueueSetFrame: appends the FRAME_1 register (0x4C). */
INCLUDE_ASM("asm/main/nonmatchings/tu213", gsQueueSetFrame);

/* gsQueueSetTex0: appends the TEX0_1 register (0x06). */
INCLUDE_ASM("asm/main/nonmatchings/tu213", gsQueueSetTex0);

/* gsQueueSetBlendColor: appends ALPHA_1 for the blend kind, then gsQueueAddRgbaq. */
INCLUDE_ASM("asm/main/nonmatchings/tu213", gsQueueSetBlendColor);

/* gsQueueAddRgbaq: appends an RGBAQ qword; the flag halves R, G and B. */
INCLUDE_ASM("asm/main/nonmatchings/tu213", gsQueueAddRgbaq);

extern int gsQueueAlphaTest;
extern int gsQueueTexAlphaExpand;

/* gsQueueSetAlphaFlags: stores the texture-alpha expand flag and the alpha-test flag. */
void gsQueueSetAlphaFlags(int texAlphaExpand, int alphaTest, int unusedArg) {
    gsQueueTexAlphaExpand = texAlphaExpand;
    gsQueueAlphaTest = alphaTest;
}

/* gsQueueSetTest: appends TEST_1 (0x47), with alpha test when gsQueueAlphaTest is 1. */
INCLUDE_ASM("asm/main/nonmatchings/tu213", gsQueueSetTest);

/* gsQueueSetClamp: appends CLAMP_1 (0x08). */
INCLUDE_ASM("asm/main/nonmatchings/tu213", gsQueueSetClamp);

int func_002745A8(int value, int flag);

/* gsQueueTex0Value: the TEX0_1 data for a texture word, with the flag from gsQueueAlphaTest. */
int gsQueueTex0Value(int value)
{
  int flag = 0;
  if ((gsQueueAlphaTest == flag) || (gsQueueAlphaTest != 1))
  {
    return func_002745A8(value, flag);
  }
  return func_002745A8(value, 1);
}

/* gsQueueSetTexA: appends TEXA (0x3B); bit 15 is set when gsQueueTexAlphaExpand is set. */
INCLUDE_ASM("asm/main/nonmatchings/tu213", gsQueueSetTexA);

/* gsQueueFlushTexture: appends TEXFLUSH (0x3F). */
INCLUDE_ASM("asm/main/nonmatchings/tu213", gsQueueFlushTexture);

/* gsQueueSetTexFilter: appends TEX1_1 (0x14), MMAG and MMIN from the argument. */
INCLUDE_ASM("asm/main/nonmatchings/tu213", gsQueueSetTexFilter);

INCLUDE_ASM("asm/main/nonmatchings/tu213", func_0027BFD0);

int func_002757F0(void);

void func_0027C7B8(void) {
    func_002757F0();
}
