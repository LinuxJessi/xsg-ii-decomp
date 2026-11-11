#include "common.h"
#include "main/sdv_033.h"
#include "shared.h"


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

void DrawImage_001EE618(XglQuadword *framebuffer);


extern DrawImageRenderState sRender;
extern void sceVif1PkAddDataN(XglPacket *packet, const void *data, int count);
extern u64 TestEnv_0_002BD4C0[12];
extern GameOverDmaPacket4 TransEnv_1_002BD520;
extern GameOverDmaPacket6 FlushEnv_2_002BD540;

void DrawImage_001EE618(XglQuadword *framebuffer)
{
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    TestEnv_0_002BD4C0[4] = ((u64)sRender.buffer_select << 0x25) | ((u64)0x8000 << 0x24);
    sceVif1PkCnt(packet, 0);
    sceVif1PkAddDataN(packet, TestEnv_0_002BD4C0, 0x18);
    sceVif1PkRef(packet, TransEnv_1_002BD520.words, 2, 0, 0, 0);
    sceVif1PkRef(packet, framebuffer, 0x7000, 0, 0x51007000, 0);
    sceVif1PkRef(packet, TransEnv_1_002BD520.words, 2, 0, 0, 0);
    sceVif1PkRef(packet, framebuffer + 0x7000, 0x7000, 0, 0x51007000, 0);
    sceVif1PkRef(packet, FlushEnv_2_002BD540.words, 3, 0, 0, 0);
}

/*
 * Display adjustment screen (tyaDisplaySetting in X1, src/main/tya_display_setting.c,
 * same image path and debug format strings): loads data\yajima\mnt_set.jpg into
 * the framebuffer, moves the picture with the pad and draws DrawImage_001EE618
 * each frame. The offset it applies is SaveData +0x32 (horizontal) and +0x34
 * (vertical), the same fields as X1.
 */
INCLUDE_ASM("asm/main/nonmatchings/game_over_065", tyaDisplaySetting);

/* GS DISPLAY register: data is the whole 64-bit value, the struct its fields. */
typedef union GsDisplay64 {
    u64 data;
    struct {
        unsigned int dx : 12;
        unsigned int dy : 11;
        unsigned int magh : 4;
        unsigned int magv : 2;
        unsigned int unmodeled_29 : 3;
        unsigned int dw : 12;
        unsigned int dh : 11;
        unsigned int unmodeled_55 : 9;
    };
} GsDisplay64;

/* +0x50 is the base dx/dy the offset is measured from (see tyaSetDisplayOffset). */
typedef struct TyaDisplayEnv {
    u8 unmodeled_00[0x18];
    GsDisplay64 display1; /* +0x18 */
    u8 unmodeled_20[0x30];
    GsDisplay64 base;      /* +0x50 */
} TyaDisplayEnv;

extern TyaDisplayEnv DispEnv;

/*
 * Applies the display offset to DispEnv: dx = the base dx + 5 * horizontal, dy =
 * the base dy + vertical (the mode-0 arithmetic of X1 tyaDisplaySetting).
 */
void tyaSetDisplayOffset(int horizontal, int vertical)
{
    DispEnv.display1.dx = DispEnv.base.dx + horizontal * 5;
    DispEnv.display1.dy = DispEnv.base.dy + vertical;
}
