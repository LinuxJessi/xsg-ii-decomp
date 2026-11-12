#include "common.h"
#include "main/sdv_033.h"
#include "shared.h"
#include "main/xgl_cd.h"
#include "main/tu094.h"
#include "gs.h"

/* Offset of the scratch area past the destination in gameOverLoadFile. */
#define GAMEOVER_SCRATCH_OFFSET 0xE0000
/* Q = 1.0f as raw bits. */
#define GS_Q_ONE 0x3F800000
/* Added to every sprite vertex (1/16 pixel units). */
#define GS_SPRITE_ORIGIN_X 0x7000
#define GS_SPRITE_ORIGIN_Y 0x71C0

extern void sceVif1PkRef(XglPacket *packet, const void *environment,
                         int count, int mode, int offset, int flags);
extern void sceVif1PkCnt(XglPacket *packet, int count);
extern XglPacket *xglPacketGetCurrent(void);

typedef unsigned int XglQuadword __attribute__((mode(TI)));

/* local view of sRender: width/height at +0x10/+0x12 and buffer_select at +0x20 disagree with main/tu093.h's frame fields. */
typedef struct {
    u8 unmodeled_00[0x08];
    u16 depth_base;
    u8 unmodeled_0a[0x06];
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

void DrawImage_0020CAB0(u8 *framebuffer);
extern DrawImageRenderState sRender;
extern void sceVif1PkAddDataN(XglPacket *packet, const void *data, int count);
extern u64 TestEnv_0_003E7E30[12];
extern GameOverDmaPacket4 TransEnv_1_003E7E90;
extern GameOverDmaPacket6 FlushEnv_2_003E7EB0;
extern u8 bgLayerState0[4];

int bgLayerReady(int index)
{
    unsigned int done_flag;

    done_flag = bgLayerState0[index] & BG_LAYER_DONE;
    return done_flag != 0;
}

void xglDirectQueueBegin(void);
void xglDirectQueuePush(u64 address, u64 data);
void xglDirectQueueSubmitRef(void);

/* Draws the full-screen fade quad (the 512x448 sprite at the origin); the low byte of fade_value becomes the RGBAQ alpha. */
void gameOverDrawOverlay(u64 fade_value)
{
    xglDirectQueueBegin();
    xglDirectQueuePush(GS_TEST_1, GS_TEST_ZTE_ALWAYS);
    xglDirectQueuePush(GS_ALPHA_1, (128ULL << 32) | 0x44);
    xglDirectQueuePush(GS_RGBAQ, ((fade_value << 32) >> 8) | (0xFE00ULL << 46));
    xglDirectQueuePush(GS_PRIM, GS_PRIM_SPRITE_ABE);
    xglDirectQueuePush(GS_XYZ2, GS_SET_XYZ(0x7000, 0x7200, 0));
    xglDirectQueuePush(GS_XYZ2, GS_SET_XYZ(0x9000, 0x8E00, 0));
    xglDirectQueueSubmitRef();
}

void *memset(void *dest, int value, unsigned int count);
int xglJpegDecode(void *request[]);
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
    read_target = base + GAMEOVER_SCRATCH_OFFSET;
    xglCdReadFile(path, read_target, 0, 1);
    memset(request, 0, sizeof(request));
    request[0] = read_target;
    request[1] = base;
    xglJpegDecode(request);
    return base;
}

void DrawImage_0020CAB0(u8 *framebuffer)
{
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    TestEnv_0_003E7E30[4] = ((u64)sRender.buffer_select << 0x25) | ((u64)0x8000 << 0x24);
    sceVif1PkCnt(packet, 0);
    sceVif1PkAddDataN(packet, TestEnv_0_003E7E30, 0x18);
    sceVif1PkRef(packet, TransEnv_1_003E7E90.words, 2, 0, 0, 0);
    sceVif1PkRef(packet, framebuffer, 0x7000, 0, 0x51007000, 0);
    sceVif1PkRef(packet, TransEnv_1_003E7E90.words, 2, 0, 0, 0);
    sceVif1PkRef(packet, framebuffer + 0x70000, 0x7000, 0, 0x51007000, 0);
    sceVif1PkRef(packet, FlushEnv_2_003E7EB0.words, 3, 0, 0, 0);
}

void xglDirectQueueDefer(int flag, int key);

/* Draws a translucent sprite between the corners (x0, y0) and (x1, y1) in pixels, queued with a deferred submit. */
void gameOverDrawSprite(int x0, int y0, int x1, int y1)
{
    xglDirectQueueBegin();
    xglDirectQueuePush(GS_TEST_1, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 2));
    xglDirectQueuePush(GS_ZBUF_1, GS_SET_ZBUF(sRender.depth_base, 1, 1));
    xglDirectQueuePush(GS_RGBAQ, GS_SET_RGBAQ(0, 0x80, 0, 0x80, GS_Q_ONE));
    xglDirectQueuePush(GS_ALPHA_1, GS_SET_ALPHA(0, 1, 2, 1, 0x40));
    xglDirectQueuePush(GS_PRIM, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 0, 0, 1, 0, 0, 0, 0));
    xglDirectQueuePush(GS_XYZF2, GS_SET_XYZ((x0 << 4) + GS_SPRITE_ORIGIN_X, (y0 << 4) + GS_SPRITE_ORIGIN_Y, 0));
    xglDirectQueuePush(GS_XYZF2, GS_SET_XYZ((x1 << 4) + GS_SPRITE_ORIGIN_X, (y1 << 4) + GS_SPRITE_ORIGIN_Y, 0));
    xglDirectQueueDefer(0, 4);
}
