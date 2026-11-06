#include "common.h"
#include "shared.h"
#include "main/gifState.h"
#include "main/nml_model_set.h"
#include "main/tu148.h"
#include "main/xgl_render.h"

void nmlModelUseSubWindow(unsigned int index, int enabled);

#define NML_WINDOW_COUNT 4

extern int g_aSubWindow[NML_WINDOW_COUNT];

INCLUDE_ASM("asm/main/nonmatchings/gifState", gifStateSetupContext);

INCLUDE_ASM("asm/main/nonmatchings/gifState", gifStateDrawRing);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_002424D8);

INCLUDE_ASM("asm/main/nonmatchings/gifState", gifStateDrawCameraQuad);

INCLUDE_ASM("asm/main/nonmatchings/gifState", gifStateDrawJitterGrid);

INCLUDE_ASM("asm/main/nonmatchings/gifState", gifStateDrawFadeQuad);

void func_002439C8(void) {
}

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_002439D0);

typedef struct XglPacket XglPacket;

void gifStateReset(void);
void gifStateFlush(void);
void sceVif1PkCloseDirectHLCode(XglPacket *packet);
void sceVif1PkCnt(XglPacket *packet, int count);
void sceVif1PkOpenDirectHLCode(XglPacket *packet, int count);

void func_00243B58(XglPacket *packet) {
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    gifStateReset();
    gifStateFlush();
    sceVif1PkCloseDirectHLCode(packet);
}

INCLUDE_ASM("asm/main/nonmatchings/gifState", gifStateDrawQuad);

INCLUDE_ASM("asm/main/nonmatchings/gifState", gifStateDrawFrame);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00244938);

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00245728);

INCLUDE_ASM("asm/main/nonmatchings/gifState", gifStateEffectFrame);

void xglRenderCopyDisp2Flip(void);
int sceGsSyncPath(int, int);

void func_00246AF8(void) {
    sceGsSyncPath(0, 0);
    xglRenderClearDepth();
    xglRenderCopyDisp2Flip();
}

/*
 * GIF state packet. gifStateNumPacket (numPacket) counts the register pairs queued in
 * gifStatePacket; entry 0 is the GIF tag word, the pairs follow in packed mode (A+D).
 * The assert text "numPacket < (STATEPACKET_MAX - 1)" names both.
 */
extern int gifStateNumPacket;
extern u32 gifStatePacket[4];

/* Empties the queue and writes the tag header: one register per loop, A+D descriptor. */
void gifStateReset(void) {
    gifStateNumPacket = 0;
    gifStatePacket[0] = 0x8000;
    gifStatePacket[1] = 0x10000000;
    gifStatePacket[2] = 0xE;
    gifStatePacket[3] = 0;
}

INCLUDE_ASM("asm/main/nonmatchings/gifState", gifStateAddReg);

INCLUDE_ASM("asm/main/nonmatchings/gifState", gifStateAddRawQword);

void sceVif1PkAddDirectDataN(XglPacket *packet, const void *data, int qwc);
XglPacket *xglPacketGetCurrent(void);

/* Sends the queued pairs as one direct VIF transfer of count+1 quadwords (tag + pairs). */
void gifStateFlush(void) {
    XglPacket *packet;
    int count;
    int tag;
    unsigned int byteCount;

    packet = xglPacketGetCurrent();
    count = gifStateNumPacket;
    tag = count | 0x8000;
    byteCount = (count * 0x10) + 0x10;
    if (count != 0) {
        gifStatePacket[0] = tag;
        sceVif1PkAddDirectDataN(packet, gifStatePacket, byteCount >> 4);
        gifStateNumPacket = 0;
    }
}

/* Queues one A+D pair: gifData goes to the GS register at address gifAddress. */
void gifStateAddReg(int gifAddress, u64 gifData);

/* GS TEXFLUSH (0x3F) with no data. */
void gifStateTexFlush(void) {
    gifStateAddReg(0x3F, 0);
}

/* GS ALPHA_1 (0x42): blend 0x64 with the fixed alpha in the upper word. */
void gifStateSetAlphaFix(int fixedAlpha) {
    gifStateAddReg(0x42, ((u64) fixedAlpha << 32) | 0x64);
}

INCLUDE_ASM("asm/main/nonmatchings/gifState", gifStateSetScissor);

/* GS ZBUF_1 (0x4E) with a fixed format word; 0x31 in the PSM field is the 24-bit Z format code. */
void gifStateSetZBufFormat(void) {
    gifStateAddReg(0x4E, 0x31000000);
}

INCLUDE_ASM("asm/main/nonmatchings/gifState", nmlModelSetMulColor);

/*
 * One GIF status block. The centre pair (+0x04/+0x08) is written by gifStateSetCenter and the
 * effect alpha pair (+0x0C/+0x10) by gifStateSetEffectAlpha; the state word and the ratio are read
 * by gifStateEffectReady.
 */
typedef struct GifStatusBlock {
    int state;
    int centerX;
    int centerY;
    int effectAlphaType;
    int effectAlpha;
    u8 unmodeled_14[0xC];
    f32 ratio;
} GifStatusBlock;

extern GifStatusBlock gifStatus;

/* Sets the alpha type and the alpha value of the effect (gifStatus +0x0C/+0x10). */
void gifStateSetEffectAlpha(int alphaType, int alpha) {
    gifStatus.effectAlphaType = alphaType;
    gifStatus.effectAlpha = alpha;
}

extern f32 gifStateDecrease;

/* Stores the decrease value of the effect; the reader of gifStateDecrease is not located yet. */
void gifStateSetDecrease(f32 decrease) {
    gifStateDecrease = decrease;
}

/* Sets the centre of the effect (gifStatus +0x04/+0x08). */
void gifStateSetCenter(int centerX, int centerY) {
    gifStatus.centerX = centerX;
    gifStatus.centerY = centerY;
}

extern int D_0063165C;

void func_00246DD8(int value) {
    D_0063165C = value;
}

extern int gifStateFbp;

/* Stores the frame buffer base of the effect. */
void gifStateSetFbp(int fbp) {
    gifStateFbp = fbp;
}

extern int gifStateBlur;

/* Stores the blur amount of the effect. */
void gifStateSetBlur(int blur) {
    gifStateBlur = blur;
}

void nmlModelUseSubWindow(unsigned int index, int enabled) {
    if (index < NML_WINDOW_COUNT) {
        g_aSubWindow[index] = enabled;
    }
}

extern f32 nmlModelSubWindowValue[];

void nmlModelSetSubWindowValue(u32 index, f32 value) {
    if (index < 4U) {
        nmlModelSubWindowValue[index] = value;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/gifState", gifStateSetMode);

extern int D_0064586C;

void gifStateClearModeValue(void) {
    D_0064586C = 0;
}


void func_00246EB0(void) {
    func_001BA6F4();
    func_00246AF8();
    msgStateReset();
}

INCLUDE_ASM("asm/main/nonmatchings/gifState", gifStateEnableEffect);

void nmlModelEnableSubWindow0(void) {
    nmlModelUseSubWindow(0U, 1);
}

extern int gifStateEffectHook;

int gifStateEffectReady(void) {
    if (gifStateEffectHook != 0 && gifStatus.state == 5) {
        if (!(0.5f <= gifStatus.ratio)) {
            return 1;
        }
        return 0;
    }
    return 1;
}

extern int D_00631660;

void gifStateDisableEffect(void) {
    D_00631660 = 0;
    gifStateEffectHook = 0;
}

INCLUDE_ASM("asm/main/nonmatchings/gifState", func_00246FF8);

extern int gifStateDrawHookSuppressed;

void gifStateSuppressDrawHook(void) {
    gifStateDrawHookSuppressed = 1;
}

extern void (*gifStateDrawHook)(void);
extern void gifStateDrawFrame(void);

void gifStateUpdateDrawHook(void) {
    if (func_001BBC74() != 0 && gifStateDrawHookSuppressed == 0) {
        gifStateDrawHook = gifStateDrawFrame;
        return;
    }
    gifStateDrawHookSuppressed = 0;
    gifStateDrawHook = 0;
}
