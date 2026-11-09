#include "common.h"
#include "main/sdv_033.h"
#include "shared.h"
#include "main/xgl_cd.h"


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

void DrawImage_0020CAB0(void *framebuffer);


extern DrawImageRenderState sRender;
extern void sceVif1PkAddDataN(XglPacket *packet, const void *data, int count);
extern u64 TestEnv_0_003E7E30[12];
extern GameOverDmaPacket4 TransEnv_1_003E7E90;
extern GameOverDmaPacket6 FlushEnv_2_003E7EB0;

INCLUDE_ASM("asm/main/nonmatchings/game_over_095", bgLayerReady);

INCLUDE_ASM("asm/main/nonmatchings/game_over_095", gameOverDrawOverlay);

void *memset(void *dest, int value, unsigned int count);
void xglJpegDecode(void *request[]);
extern u8 *WorkEnd;

/*
 * Reads the file at `path` into the scratch area 0xE0000 bytes past the
 * destination (the buffer, or WorkEnd when none is given), hands the read
 * target and the destination to xglJpegDecode and returns the destination.
 */
u8 *gameOverLoadFile(const char *path, u8 *buffer)
{
    u8 *base;
    u8 *read_target;
    void *request[8];

    base = WorkEnd;
    if (buffer != 0) {
        base = buffer;
    }
    read_target = base + 0xE0000;
    xglCdReadFile(path, read_target, 0, 1);
    memset(request, 0, sizeof(request));
    request[0] = read_target;
    request[1] = base;
    xglJpegDecode(request);
    return base;
}

void DrawImage_0020CAB0(void *framebuffer)
{
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    TestEnv_0_003E7E30[4] = ((u64)sRender.buffer_select << 0x25) | ((u64)0x8000 << 0x24);
    sceVif1PkCnt(packet, 0);
    sceVif1PkAddDataN(packet, TestEnv_0_003E7E30, 0x18);
    sceVif1PkRef(packet, TransEnv_1_003E7E90.words, 2, 0, 0, 0);
    sceVif1PkRef(packet, framebuffer, 0x7000, 0, 0x51007000, 0);
    sceVif1PkRef(packet, TransEnv_1_003E7E90.words, 2, 0, 0, 0);
    sceVif1PkRef(packet, (u8 *)framebuffer + 0x70000, 0x7000, 0, 0x51007000, 0);
    sceVif1PkRef(packet, FlushEnv_2_003E7EB0.words, 3, 0, 0, 0);
}

INCLUDE_ASM("asm/main/nonmatchings/game_over_095", gameOverDrawSprite);
