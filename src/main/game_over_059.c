#include "common.h"
#include "main/sdv_033.h"
#include "shared.h"
#include "main/control_entry.h"
#include "main/xenoActor.h"
#include "main/xgl_render.h"


extern void sceVif1PkRef(XglPacket *packet, const void *environment,
                         int count, int mode, int offset, int flags);
extern void sceVif1PkCnt(XglPacket *packet, int count);
extern XglPacket *xglPacketGetCurrent(void);

typedef unsigned int XglQuadword __attribute__((mode(TI)));

/* DrawBack reads the framebuffer-page halfword at sRender +0x14. */
typedef struct {
    u8 unmodeled_00[0x10];
    u16 width;
    u16 height;
    u16 framebuffer_page;
    u8 unmodeled_16[0x0a];
    u16 buffer_select;
} DrawImageRenderState;

typedef union GameOverDmaPacket4 {
    u64 words[4];
    XglQuadword quadwords[2];
} GameOverDmaPacket4;

typedef union GameOverDmaPacket6 {
    u64 words[6];
    XglQuadword quadwords[3];
} GameOverDmaPacket6;

typedef union GameOverDmaPacket20 {
    u64 words[20];
    XglQuadword quadwords[10];
} GameOverDmaPacket20;

void DrawImage(void *framebuffer);
void DrawBack(int alpha);
void redraw_frame(void *framebuffer);
extern void FlushCache(int mode);


extern DrawImageRenderState sRender;
extern void sceVif1PkAddDataN(XglPacket *packet, const void *data, int count);
extern u64 TestEnv_0_00369CC0[12];
extern GameOverDmaPacket4 TransEnv_1_00369D20;
extern GameOverDmaPacket6 FlushEnv_2_00369D40;
extern GameOverDmaPacket20 TestEnv_3_002BCED0;
int *xglStudioGetCamera2(int);
void uwamonoResetRecords(int, int);
void func_0023AD18(int);
void fadeReset(void);

void GameSceneReset(void *framebuffer) {
    xglStudioGetCamera2(0)[1] = 1;
    xglRenderClearDepth();
    fadeReset();
    xglRenderGlobalFadeReset();
    XenoActorPoolInitDefault();
    uwamonoResetRecords(-1, 1);
    redraw_frame(framebuffer);
    func_0023AD18(0);
}

INCLUDE_ASM("asm/main/nonmatchings/game_over_059", DrawImage);

INCLUDE_ASM("asm/main/nonmatchings/game_over_059", GameSynopsisDrawPanel);

void DrawBack(int alpha)
{
    u64 nonnegative_alpha;
    u64 gs_frame_address_bits;

    nonnegative_alpha = alpha < 0 ? 0 : alpha;
    gs_frame_address_bits = 0xc800;
    TestEnv_3_002BCED0.words[6] =
        (0x24120000 | (sRender.framebuffer_page << 5)) |
        (gs_frame_address_bits << 0x13);
    TestEnv_3_002BCED0.words[8] = (nonnegative_alpha << 32) | 100;
    FlushCache(0);
    sceVif1PkRef(xglPacketGetCurrent(), TestEnv_3_002BCED0.words, 10, 0, 0, 0);
}

INCLUDE_ASM("asm/main/nonmatchings/game_over_059", GameCopyFrame);

void redraw_frame(void *framebuffer) {
    xglSleep();
    DrawImage(framebuffer);
    xglSleep();
    DrawImage(framebuffer);
    xglSleep();
}

INCLUDE_ASM("asm/main/nonmatchings/game_over_059", GameEnd);

INCLUDE_ASM("asm/main/nonmatchings/game_over_059", GameOver);

INCLUDE_ASM("asm/main/nonmatchings/game_over_059", Intermission);

/* Rewrites each '1' or '2' in the disc message text to the disc number digit. */
void GameDiskSetDiscDigit(u8 *text, int digit) {
    u8 *cursor;
    u8 ch;

    cursor = text;
    ch = *cursor;
    if (ch != 0) {
        do {
            if (ch == '1' || ch == '2') {
                *cursor = digit + '0';
            }
            cursor += 1;
            ch = *cursor;
        } while (ch != 0);
    }
}

INCLUDE_ASM("asm/main/nonmatchings/game_over_059", GameDiskChange);

INCLUDE_ASM("asm/main/nonmatchings/game_over_059", GameSynopsis);

INCLUDE_ASM("asm/main/nonmatchings/game_over_059", func_001E95C8);
