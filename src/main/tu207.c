#include "common.h"
#include "shared.h"
#include "vu0_macro.h"
#include "gs.h"
#include "main/tu207.h"

/* Wraps value once into lower..upper: one period (upper - lower + 1) down when above upper, one up when below lower. */
int func_00274808(int value, int lower, int upper)
{
    int period = upper - lower + 1;

    if (upper < value) {
        value -= period;
    }
    if (value < lower) {
        value += period;
    }
    return value;
}

/* Inclusive range test: lower <= value <= upper. */
int xglIsInRange(int value, int lower, int upper)
{
  return lower <= value && value <= upper;
}

typedef struct XglPacket XglPacket;

/*
 * One 0x7C-byte draw record (tu213.c's GsQueueContext; recordBanks holds three banks of ten). The kind byte picks
 * the draw handler D_0063D758[type - 1]; the words after it mean different things per kind, so they are a union:
 * the kinds drawn through the gsQueue emitters (tu213.c) share the queue header, whose color (+0x0C) and halveRgb
 * (+0x18) are tu213.c's names.
 */
typedef struct GsQueueContext {
    unsigned char type; /* +0x00: 0 unused, 1..21 picks the draw handler D_0063D758[type - 1] */
    unsigned char unmodeled_01[3];
    union {
        /* +0x04..+0x7C: the parameter words as func_00275710 sets them (the first ten). */
        int params[30];
        /* gsQueueBeginTriStrip passes frame to gsQueueSetFrame, tex0 to gsQueueSetTex0 and blendKind with color
           to gsQueueSetBlendColor; count is the number of passes. */
        struct {
            int frame;     /* +0x04 */
            int tex0;      /* +0x08 */
            int color;     /* +0x0C */
            int depth;     /* +0x10 */
            int count;     /* +0x14 */
            int halveRgb;  /* +0x18 */
            int blendKind; /* +0x1C */
            union {
                /* Kind 20 (func_0027A6F0): count sprites from screenRect with texture coordinates from textureRect. */
                struct {
                    int screenRect[4];  /* +0x20 */
                    int textureRect[4]; /* +0x30 */
                } sprite;
                /* Kind 4 (func_00276458): the number of grid cells to set. */
                int cellCount; /* +0x20 */
            } shape;
        } queue;
        /* Kind 1 (func_002759F8): the stage (0..2), the alpha and the depth of the full-screen sprite. */
        struct {
            int stage; /* +0x04 */
            int alpha; /* +0x08 */
            int depth; /* +0x0C */
        } fade;
        /* Kind 3 (func_00275F70): the ring radius step and the depth of the fan. */
        struct {
            int step;  /* +0x04 */
            int depth; /* +0x08 */
        } tunnel;
        /* Kinds 5 and 6 (func_00276838, func_00276D68): the alpha (kind 5: the spread) and the Z and fog word. */
        struct {
            int alpha; /* +0x04 */
            int z;     /* +0x08 */
        } warp;
    } u;
} GsQueueContext;

typedef struct RecordBank {
    GsQueueContext records[10];
} RecordBank;

/* Declared as bytes, as recordClearBank below reads it. func_00274850 and func_00275710 index it as bytes:
 * through (RecordBank *) gcc emits addu base,offset instead of offset,base and, in func_00275710, merges
 * the two record address computations the original keeps apart. */
extern unsigned char recordBanks[];

/* Local view of sRender (0x00645820; RenderFrameState in main/tu093.h does not model +0x3C): the
 * render callback, called with the frame's packet (DefocusRenderState.callback in game_defocus.c). */
typedef struct RecordRenderState {
    unsigned char unmodeled_00[0x3C];
    void (*callback)(XglPacket *packet);
} RecordRenderState;

extern RecordRenderState sRender;

void func_00275830(XglPacket *packet);

/* Installs func_00275830 as the render callback while any record is used, else removes it. The scan
 * covers 30 banks although recordBanks holds 3 (0xE88 bytes), as the original does. */
void func_00274850(void)
{
    int active;
    int slot;
    int bank;

    active = 0;
    for (slot = 0; slot < 10; slot++) {
        for (bank = 0; bank < 30; bank++) {
            if (recordBanks[bank * 0x4D8 + slot * 0x7C] != 0) {
                active = 1;
            }
        }
    }
    if (active == 0) {
        sRender.callback = 0;
    } else {
        sRender.callback = func_00275830;
    }
}

void recordClear(void *record);
/* Flag that suspends the per-frame record updates; 1 while suspended. */
extern int recordUpdatePaused;

/* Fills bank bankIndex (0..2) with the record preset kind (0..24): clears its ten records, writes the
 * preset's records (type and parameter words; the two-, three- and five-record presets fill records 0..4),
 * then updates the render callback. The screen is 512 x 448. */
INCLUDE_ASM("asm/main/nonmatchings/tu207", func_002748C8);

/* Sets record slotIndex of bank bankIndex to type with its ten parameters, then updates the render callback. */
void func_00275710(int bankIndex, int slotIndex, int type, int *params)
{
    int i;

    recordBanks[slotIndex * 0x7C + bankIndex * 0x4D8] = type;
    for (i = 0; i < 10; i++) {
        ((RecordBank *)recordBanks)[bankIndex].records[slotIndex].u.params[i] = params[i];
    }
    func_00274850();
}

/* Clears the ten 0x7C-byte records of bank bankIndex (banks are 0x4D8 bytes apart). */
void recordClearBank(int bankIndex)
{
    unsigned char *record;
    int slot;

    record = recordBanks + bankIndex * 0x4D8;
    for (slot = 0; slot < 10; slot++) {
        recordClear(record);
        record += 0x7C;
    }
}

/* Clears the first three banks' records. */
void func_002757F0(void)
{
    int bankIndex;

    bankIndex = 0;
    do {
        recordClearBank(bankIndex);
        bankIndex += 1;
    } while (bankIndex < 3);
}

void sceVif1PkCnt(XglPacket *packet, int count);
void sceVif1PkOpenDirectHLCode(XglPacket *packet, int mode);
void sceVif1PkAddDirectDataN(XglPacket *packet, const void *data, int qwc);
void sceVif1PkCloseDirectHLCode(XglPacket *packet);
int func_00275968(void);
/* GIF data sent before (9 quadwords) and after (3 quadwords) the records are drawn. */
extern const unsigned char D_0063D6C8[];
extern const unsigned char D_0063D7B0[];
/* Draw handlers of record types 1..21. */
extern void (*D_0063D758[])(GsQueueContext *record, XglPacket *packet);

/* Draws every used record of the three banks between two fixed GIF blocks, then clears the banks and
 * removes itself as the render callback. */
void func_00275830(XglPacket *packet)
{
    RecordBank *bank;
    GsQueueContext *record;
    int bankIndex;
    int slot;
    int type;

    bank = (RecordBank *)recordBanks;
    if (func_00275968() == 0) {
        return;
    }
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    sceVif1PkAddDirectDataN(packet, D_0063D6C8, 9);
    sceVif1PkCloseDirectHLCode(packet);
    for (bankIndex = 0; bankIndex < 3; bankIndex++) {
        record = bank->records;
        for (slot = 0; slot < 10; slot++) {
            type = record->type;
            /* if (type < 22 && type != 0) gives beq with the slot filled from the call instead of beql */
            switch (type) {
            case 1 ... 21:
                D_0063D758[type - 1](record, packet);
                break;
            }
            record++;
        }
        bank++;
    }
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    sceVif1PkAddDirectDataN(packet, D_0063D7B0, 3);
    sceVif1PkCloseDirectHLCode(packet);
    func_002757F0();
    recordUpdatePaused = 0;
    sRender.callback = 0;
}

/* True if any of the 30 records (3 banks of 10) has a nonzero first byte. */
int func_00275968(void)
{
    unsigned char *bank;
    unsigned char *record;
    int bankIndex;
    int slot;

    bank = recordBanks;
    for (bankIndex = 0; bankIndex < 3; bankIndex++) {
        for (slot = 0, record = bank; slot < 10; slot++) {
            if (*record != 0) {
                return 1;
            }
            record += 0x7C;
        }
        bank += 0x4D8;
    }
    return 0;
}

extern void *memset(void *destination, int value, unsigned int count);

/* Clears one 0x7C-byte record. */
void recordClear(void *record)
{
  memset(record, 0, 0x7C);
}

/* Sets recordUpdatePaused, the flag that suspends the per-frame record updates. */
void recordPauseUpdates(void)
{
  recordUpdatePaused = 1;
}

/* The EE scratchpad, where the packet is built before it is sent. */
#define SCRATCHPAD_BASE 0x70000000

/* One A+D qword of a GS packet: the register value, then the register address. A union as tu229.c's TalkGsPair:
   with a plain struct gcc moves func_00275F70's point reads above the packet stores. */
typedef union GsAdPair {
    struct {
        u64 data;
        u64 reg;
    } ad;
} GsAdPair;

XglPacket *xglPacketGetCurrent(void);
u64 func_002745A8(int buffer, int flag);
u64 func_00274650(int buffer, int flag);
int func_002746F8(int blend);
void recordStepAngle(int stage);
int func_00275CF0(int mode, int fallback);
int func_00275D30(int mode, int alpha);
int func_00275E08(void);

/* Draw handler of record kind 1 (D_0063D758[0]): one textured sprite over the 512x448 screen, inset by the
   stage's size, in the stage's color; at stage 2 the sprite is drawn a second time in the pulsing color. */
void func_002759F8(GsQueueContext *record)
{
    int stage;
    int alpha;
    int z;
    unsigned int color;
    int size;
    XglPacket *packet;
    GsAdPair *pairs;
    int pairCount;

    stage = record->u.fade.stage;
    alpha = record->u.fade.alpha;
    z = record->u.fade.depth;
    recordStepAngle(stage);
    color = func_00275D30(stage, alpha);
    size = func_00275CF0(stage, alpha);
    packet = xglPacketGetCurrent();
    pairs = (GsAdPair *) SCRATCHPAD_BASE;
    pairCount = 0;
    /* GIF tag (one A+D register, EOP); the loop count is added once the packet is complete. */
    pairs[pairCount].ad.reg = GIF_AD;
    pairs[pairCount].ad.data = ((u64) 0x10000000 << 32) | 0x8000;
    pairCount++;
    pairs[pairCount].ad.reg = GS_TEXFLUSH;
    pairs[pairCount].ad.data = 0;
    pairCount++;
    pairs[pairCount].ad.reg = GS_TEST_1;
    pairs[pairCount].ad.data = GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 2);
    pairCount++;
    pairs[pairCount].ad.reg = GS_TEX1_1;
    pairs[pairCount].ad.data = 0;
    pairCount++;
    pairs[pairCount].ad.reg = GS_TEXA;
    pairs[pairCount].ad.data = 0;
    pairCount++;
    pairs[pairCount].ad.reg = GS_ALPHA_1;
    pairs[pairCount].ad.data = func_002746F8(0);
    pairCount++;
    pairs[pairCount].ad.reg = GS_TEX0_1;
    pairs[pairCount].ad.data = func_002745A8(1, 0);
    pairCount++;
    pairs[pairCount].ad.reg = GS_FRAME_1;
    pairs[pairCount].ad.data = func_00274650(0, 0);
    pairCount++;
    pairs[pairCount].ad.reg = GS_PRIM;
    pairs[pairCount].ad.data = GS_SET_PRIM(GS_PRIM_SPRITE, 1, 1, 0, 1, 0, 1, 0, 0);
    pairCount++;
    pairs[pairCount].ad.reg = GS_RGBAQ;
    pairs[pairCount].ad.data = color;
    pairCount++;
    /* Corners in 12.4 fixed point: (0x7000, 0x7200) is the screen's top left around the GS origin 2048,
       (0x9000, 0x8E00) its bottom right. */
    pairs[pairCount].ad.reg = GS_UV;
    pairs[pairCount].ad.data = GS_SET_UV(size << 4, size << 4);
    pairCount++;
    pairs[pairCount].ad.reg = GS_XYZF2;
    pairs[pairCount].ad.data = GS_SET_XYZ(0x7000, 0x7200, z);
    pairCount++;
    pairs[pairCount].ad.reg = GS_UV;
    pairs[pairCount].ad.data = GS_SET_UV((0x200 - size) << 4, (0x1C0 - size) << 4);
    pairCount++;
    pairs[pairCount].ad.reg = GS_XYZF2;
    pairs[pairCount].ad.data = GS_SET_XYZ(0x9000, 0x8E00, z);
    pairCount++;
    if (stage == 2) {
        color = func_00275E08();
        pairs[pairCount].ad.reg = GS_RGBAQ;
        pairs[pairCount].ad.data = color;
        pairCount++;
        pairs[pairCount].ad.reg = GS_UV;
        pairs[pairCount].ad.data = GS_SET_UV(size << 4, size << 4);
        pairCount++;
        pairs[pairCount].ad.reg = GS_XYZF2;
        pairs[pairCount].ad.data = GS_SET_XYZ(0x7000, 0x7200, z);
        pairCount++;
        pairs[pairCount].ad.reg = GS_UV;
        pairs[pairCount].ad.data = GS_SET_UV((0x200 - size) << 4, (0x1C0 - size) << 4);
        pairCount++;
        pairs[pairCount].ad.reg = GS_XYZF2;
        pairs[pairCount].ad.data = GS_SET_XYZ(0x9000, 0x8E00, z);
        pairCount++;
    }
    pairs[0].ad.data += pairCount - 1;
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    sceVif1PkAddDirectDataN(packet, pairs, pairCount);
    sceVif1PkCloseDirectHLCode(packet);
}

/* Mode selector: mode 0 and 2 return fallback, mode 1 returns 5, anything else returns 0. */
int func_00275CF0(int mode, int fallback)
{
    switch (mode) {
    case 0:
    case 2:
        return fallback;
    case 1:
        return 5;
    default:
        return 0;
    }
}

extern int recordAngle;
float sinf(float angle);

/* Sprite color of stage mode: grey 0x808080 under alpha at stage 1, the fixed 0x40808080 (alpha 0x40, whatever alpha
   is) at stage 0, at stage 2 the alpha 80 + 5 * sin(recordAngle degrees) put into 0..127 by func_00274808; 0 for any
   other stage. */
int func_00275D30(int mode, int alpha)
{
    switch (mode) {
    case 1:
        return (alpha << 24) | 0x808080;
    case 0:
        return 0x40808080;
    case 2:
        alpha = func_00274808((int) (sinf(recordAngle * 3.141592 / 180.0) * 5.0f + 80.0f), 0, 0x7F);
        return (alpha << 24) | 0x808080;
    default:
        return 0;
    }
}

float cosf(float angle);

/* Pulsing color: grey 0x808080 under the alpha 80 + 5 * cos(recordAngle degrees), put into 0..127 by
   func_00274808. */
int func_00275E08(void)
{
    int alpha;

    alpha = func_00274808((int) (cosf(recordAngle * 3.141592 / 180.0) * 5.0f + 80.0f), 0, 0x7F);
    return (alpha << 24) | 0x808080;
}

/* At stage 2, while updates are not suspended, steps the angle recordAngle by 92 and wraps it into 0..360. */
void recordStepAngle(int stage)
{
    /* Block-scope declaration: a call through the file-scope one, after the definition above, is marked
       nothrow and turns the original bnel at 0x275EB4 into bne; this call has no such mark. */
    int func_00274808(int value, int lower, int upper);

    if (recordUpdatePaused == 0 && stage == 2) {
        recordAngle += 0x5C;
        recordAngle = func_00274808(recordAngle, 0, 0x168);
    }
}

void func_0027A6F0(GsQueueContext *context, int packet);

/* Draw handler of record kind 2 (D_0063D758[1]): draws the record's texture (its tex0 word) over the whole 512x448
   screen through a kind 20 sprite record. */
void func_00275EE8(GsQueueContext *record, int packet)
{
    GsQueueContext sprite;

    sprite.type = 0x14;
    sprite.u.queue.frame = 2;
    sprite.u.queue.tex0 = record->u.queue.tex0;
    sprite.u.queue.color = 0x80FFFFFF;
    sprite.u.queue.depth = 0x7FFFFFFF;
    sprite.u.queue.count = 1;
    sprite.u.queue.halveRgb = 1;
    sprite.u.queue.blendKind = 0;
    sprite.u.queue.shape.sprite.screenRect[0] = 0;
    sprite.u.queue.shape.sprite.screenRect[1] = 0;
    sprite.u.queue.shape.sprite.screenRect[2] = 0x200;
    sprite.u.queue.shape.sprite.screenRect[3] = 0x1C0;
    sprite.u.queue.shape.sprite.textureRect[0] = 0;
    sprite.u.queue.shape.sprite.textureRect[1] = 0;
    sprite.u.queue.shape.sprite.textureRect[2] = 0x200;
    sprite.u.queue.shape.sprite.textureRect[3] = 0x1C0;
    func_0027A6F0(&sprite, packet);
}

void func_00276400(int delta);
void func_00276350(float *point, float *clampedPoint, float angle, float radius);
/* libgcc __fixunssfdi, float to unsigned 64-bit integer (negative values give 0), in the libgcc block between
   __moddi3 and fptodp. */
u64 func_002A6A90(float value);
extern float D_0069DDD8;
extern int D_0069DDDC;

/* Draw handler of record kind 3 (D_0063D758[2]): a triangle fan from the screen centre through octagonal rings of
   radius D_0069DDDC, D_0069DDDC + 25, ... below 620 (func_00276400 advances D_0069DDDC by the record's step); each
   ring vertex takes its texture coordinates from the point clamped to radius 224, at the record's depth. */
void func_00275F70(GsQueueContext *record)
{
    int z;
    XglPacket *packet;
    GsAdPair *pairs;
    int pairCount;
    int radius;
    int side;
    float point[2];
    float clampedPoint[2];

    z = record->u.tunnel.depth;
    func_00276400(record->u.tunnel.step);
    packet = xglPacketGetCurrent();
    pairs = (GsAdPair *) SCRATCHPAD_BASE;
    pairCount = 0;
    /* GIF tag (one A+D register, EOP); the loop count is added once the packet is complete. */
    pairs[pairCount].ad.reg = GIF_AD;
    pairs[pairCount].ad.data = ((u64) 0x10000000 << 32) | 0x8000;
    pairCount++;
    pairs[pairCount].ad.reg = GS_TEXFLUSH;
    pairs[pairCount].ad.data = 0;
    pairCount++;
    pairs[pairCount].ad.reg = GS_TEST_1;
    pairs[pairCount].ad.data = GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 2);
    pairCount++;
    pairs[pairCount].ad.reg = GS_TEX1_1;
    pairs[pairCount].ad.data = 0;
    pairCount++;
    pairs[pairCount].ad.reg = GS_TEXA;
    pairs[pairCount].ad.data = 0;
    pairCount++;
    pairs[pairCount].ad.reg = GS_ALPHA_1;
    pairs[pairCount].ad.data = func_002746F8(0);
    pairCount++;
    pairs[pairCount].ad.reg = GS_TEX0_1;
    pairs[pairCount].ad.data = func_002745A8(1, 0);
    pairCount++;
    pairs[pairCount].ad.reg = GS_FRAME_1;
    pairs[pairCount].ad.data = func_00274650(0, 0);
    pairCount++;
    pairs[pairCount].ad.reg = GS_PRIM;
    pairs[pairCount].ad.data = GS_SET_PRIM(5, 1, 1, 0, 1, 0, 1, 0, 0);
    pairCount++;
    pairs[pairCount].ad.reg = GS_RGBAQ;
    pairs[pairCount].ad.data = GS_SET_RGBAQ(0x80, 0x80, 0x80, 0, 0);
    pairCount++;
    pairs[pairCount].ad.reg = GS_UV;
    pairs[pairCount].ad.data = GS_SET_UV(256 << 4, 224 << 4);
    pairCount++;
    pairs[pairCount].ad.reg = GS_XYZF2;
    pairs[pairCount].ad.data = GS_SET_XYZ(0x8000, 0x8000, z);
    pairCount++;
    pairs[pairCount].ad.reg = GS_RGBAQ;
    pairs[pairCount].ad.data = GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x23, 0);
    pairCount++;
    /* point is in screen pixels: + (2048 - 256, 2048 - 224) puts it around the GS origin, in 12.4 fixed point.
       The first vertex of a ring goes to the XYZ3 register, without a drawing kick; D_0069DDD8 converts degrees
       to radians. */
    for (radius = D_0069DDDC; radius < 620; radius += 25) {
        func_00276350(point, clampedPoint, 0.0f, radius);
        pairs[pairCount].ad.reg = GS_UV;
        pairs[pairCount].ad.data = GS_SET_UV(func_002A6A90(clampedPoint[0] * 16.0f),
                                             func_002A6A90(clampedPoint[1] * 16.0f));
        pairCount++;
        pairs[pairCount].ad.reg = GS_XYZ3;
        pairs[pairCount].ad.data = GS_SET_XYZ((int) ((point[0] + 1792.0f) * 16.0f),
                                              (int) ((point[1] + 1824.0f) * 16.0f), z);
        pairCount++;
        for (side = 1; side <= 8; side++) {
            func_00276350(point, clampedPoint, side * 45 * D_0069DDD8, radius);
            pairs[pairCount].ad.reg = GS_UV;
            pairs[pairCount].ad.data = GS_SET_UV(func_002A6A90(clampedPoint[0] * 16.0f),
                                                 func_002A6A90(clampedPoint[1] * 16.0f));
            pairCount++;
            pairs[pairCount].ad.reg = GS_XYZF2;
            pairs[pairCount].ad.data = GS_SET_XYZ((int) ((point[0] + 1792.0f) * 16.0f),
                                                  (int) ((point[1] + 1824.0f) * 16.0f), z);
            pairCount++;
        }
    }
    pairs[0].ad.data += pairCount - 1;
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    sceVif1PkAddDirectDataN(packet, pairs, pairCount);
    sceVif1PkCloseDirectHLCode(packet);
}

/* point: the screen position at angle (radians) and radius around the screen centre (256, 224); clampedPoint: the
   same with the radius clamped to 224. */
void func_00276350(float *point, float *clampedPoint, float angle, float radius)
{
    float cosine;
    float sine;

    cosine = cosf(angle);
    sine = sinf(angle);
    point[0] = radius * cosine + 256.0f;
    point[1] = radius * sine + 224.0f;
    if (radius > 224.0f) {
        radius = 224.0f;
    }
    clampedPoint[0] = radius * cosine + 256.0f;
    clampedPoint[1] = radius * sine + 224.0f;
}

/* While updates are not suspended, advances D_0069DDDC by delta and steps it back below 406 in steps of 25. */
void func_00276400(int delta)
{
    if (recordUpdatePaused == 0) {
        D_0069DDDC += delta;
        while (D_0069DDDC >= 0x196) {
            D_0069DDDC -= 0x19;
        }
    }
}

/* Row stride of the cell grid (50 x 50 flags); recordGridReset sets the width and height in use. */
#define RECORD_GRID_SIDE 50

extern int recordGridWidth;
extern int recordGridHeight;
/* signed char: the original reads the flags with lb (unsigned char gave lbu). */
extern signed char recordGridCells[];

void gsQueueBeginSprites(GsQueueContext *context);
void gsQueueAddTexturedVertex(int packet, Vector4 *position, Vector4 *texcoord, GsQueueContext *context);
void gsQueueAddTexturedKickVertex(int packet, Vector4 *position, Vector4 *texcoord, GsQueueContext *context);
void gsQueueFlush(int packet);
unsigned short xglSRand(void);

/*
 * Draw handler of record kind 4 (D_0063D758[3]): sets the record's cellCount random cells of the grid (clearing
 * random ones when more are set), then draws one sprite per set cell on the 512 x 448 screen, count times.
 */
INCLUDE_ASM("asm/main/nonmatchings/tu207", func_00276458);

/* Clears the 0x9C4-byte cell grid and sets its width and height (cells across and down). */
void recordGridReset(int width, int height)
{
    memset(recordGridCells, 0, 0x9C4);
    recordGridWidth = width;
    recordGridHeight = height;
}

/* Number of cells in the grid: width * height. */
int recordGridCellCount(void)
{
  return recordGridWidth * recordGridHeight;
}

void func_00277100(void);
void func_00277168(Vector4 *result, int x, int y);

/*
 * Draw handler of record kind 5 (D_0063D758[4]): draws the screen texture as a triangle fan around the screen centre (256, 224): eight corner and edge points
 * pushed outwards by the record's alpha word (spread), its half and its eighth, written twice around and closed
 * at the first point. Z and fog take the record's z word.
 */
INCLUDE_ASM("asm/main/nonmatchings/tu207", func_00276838);

/*
 * Draw handler of record kind 6 (D_0063D758[5]): draws the 512 x 448 screen texture as one triangle fan of its four
 * corners, each corner moved by func_00277168. XYZF2 takes the record's z as the whole upper word (Z and fog).
 */
void func_00276D68(GsQueueContext *record)
{
    Vector4 point;
    GsAdPair *pairs;
    XglPacket *packet;
    int alpha;
    int z;
    int pairCount;

    alpha = record->u.warp.alpha;
    z = record->u.warp.z;
    func_00277100();
    packet = xglPacketGetCurrent();
    pairs = (GsAdPair *) SCRATCHPAD_BASE;
    pairCount = 0;
    pairs[pairCount].ad.reg = GIF_AD;
    pairs[pairCount].ad.data = ((u64)0x10000000 << 32) | 0x8000;
    pairCount++;
    pairs[pairCount].ad.reg = GS_TEXFLUSH;
    pairs[pairCount].ad.data = 0;
    pairCount++;
    pairs[pairCount].ad.reg = GS_TEST_1;
    pairs[pairCount].ad.data = GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 2);
    pairCount++;
    pairs[pairCount].ad.reg = GS_TEX1_1;
    pairs[pairCount].ad.data = 0;
    pairCount++;
    pairs[pairCount].ad.reg = GS_TEXA;
    pairs[pairCount].ad.data = 0;
    pairCount++;
    pairs[pairCount].ad.reg = GS_ALPHA_1;
    pairs[pairCount].ad.data = func_002746F8(0);
    pairCount++;
    pairs[pairCount].ad.reg = GS_TEX0_1;
    pairs[pairCount].ad.data = func_002745A8(1, 0);
    pairCount++;
    pairs[pairCount].ad.reg = GS_FRAME_1;
    pairs[pairCount].ad.data = func_00274650(0, 0);
    pairCount++;
    pairs[pairCount].ad.data = 0x15D;
    pairs[pairCount].ad.reg = GS_PRIM;
    pairCount++;
    /* Grey 0x808080 with the record's alpha. */
    pairs[pairCount].ad.reg = GS_RGBAQ;
    pairs[pairCount].ad.data = (unsigned int)(alpha << 24 | 0x808080);
    pairCount++;
    func_00277168(&point, 0, 0);
    pairs[pairCount].ad.reg = GS_UV;
    pairs[pairCount].ad.data = GS_SET_UV(0, 0);
    pairCount++;
    pairs[pairCount].ad.reg = GS_XYZF2;
    pairs[pairCount].ad.data = GS_SET_XYZ((int)((point.x + 1792.0f) * 16.0f), (int)((point.y + 1824.0f) * 16.0f), z);
    pairCount++;
    func_00277168(&point, 512, 0);
    pairs[pairCount].ad.reg = GS_UV;
    pairs[pairCount].ad.data = GS_SET_UV(512 * 16, 0);
    pairCount++;
    pairs[pairCount].ad.reg = GS_XYZF2;
    pairs[pairCount].ad.data = GS_SET_XYZ((int)((point.x + 1792.0f) * 16.0f), (int)((point.y + 1824.0f) * 16.0f), z);
    pairCount++;
    func_00277168(&point, 512, 448);
    pairs[pairCount].ad.reg = GS_UV;
    pairs[pairCount].ad.data = GS_SET_UV(512 * 16, 448 * 16);
    pairCount++;
    pairs[pairCount].ad.reg = GS_XYZF2;
    pairs[pairCount].ad.data = GS_SET_XYZ((int)((point.x + 1792.0f) * 16.0f), (int)((point.y + 1824.0f) * 16.0f), z);
    pairCount++;
    func_00277168(&point, 0, 448);
    pairs[pairCount].ad.reg = GS_UV;
    pairs[pairCount].ad.data = GS_SET_UV(0, 448 * 16);
    pairCount++;
    pairs[pairCount].ad.reg = GS_XYZF2;
    pairs[pairCount].ad.data = GS_SET_XYZ((int)((point.x + 1792.0f) * 16.0f), (int)((point.y + 1824.0f) * 16.0f), z);
    pairCount++;
    pairs[0].ad.data += pairCount - 1;
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    sceVif1PkAddDirectDataN(packet, pairs, pairCount);
    sceVif1PkCloseDirectHLCode(packet);
}

extern float D_0069DDE8;
extern float D_0069DDEC;

/* Integrates D_0069DDE8 by D_0069DDEC and stops the motion once it crosses -1.0f. */
void func_00277100(void)
{
    float velocity;
    float next;

    if (recordUpdatePaused != 0) {
        return;
    }

    velocity = D_0069DDEC;
    next = D_0069DDE8 + velocity;
    D_0069DDE8 = next;
    if (velocity > 0.0f) {
        if (next > -1.0f) {
            D_0069DDEC = 0.0f;
        }
    } else {
        if (next < -1.0f) {
            D_0069DDEC = 0.0f;
        }
    }
}

void xglMatrixUnit(Matrix4 matrix);
void xglMatrixRotZ(Matrix4 destination, Matrix4 source, f32 angle);
void xglMatrixScale(Matrix4 destination, Matrix4 source, const Vector4 *scale);
void xglVectorMulMat(Vector4 *destination, Matrix4 matrix, const Vector4 *vector);
extern AlignedVector4 D_0068D6C0;
extern AlignedVector4 D_0068D6D0;
extern float D_0069DDF0;

/* Rotates the point (x, y) about D_0068D6C0 by D_0069DDE8 * D_0069DDF0, scales it by D_0068D6D0 and stores the
 * resulting x and y in result. */
void func_00277168(Vector4 *result, int x, int y)
{
    AlignedVector4 point = { x, y, 0.0f, 0.0f };
    AlignedVector4 center = D_0068D6C0;
    AlignedVector4 scale = D_0068D6D0;
    AlignedVector4 offset;
    Matrix4 rotation;
    Matrix4 scaling;
    float angle;

    angle = D_0069DDE8 * D_0069DDF0;
    xglMatrixUnit(rotation);
    xglMatrixUnit(scaling);
    xglMatrixRotZ(rotation, rotation, angle);
    xglMatrixScale(scaling, scaling, &scale);
    Vu0SubVector(&offset, &point, &center);
    xglVectorMulMat(&offset, rotation, &offset);
    xglVectorMulMat(&offset, scaling, &offset);
    __asm__ __volatile__("lqc2 vf20, 0(%0)" : : "r"(&offset) : "memory");
    __asm__ __volatile__("lqc2 vf21, 0(%0)" : : "r"(&center) : "memory");
    __asm__ __volatile__("vadd.xyzw vf20, vf20, vf21" : : : "memory");
    __asm__ __volatile__("sqc2 vf20, 0(%0)" : : "r"(&offset) : "memory");
    result->x = offset.x;
    result->y = offset.y;
}

/* Points on the ring: 0..360 degrees in 18-degree steps. */
#define RING_STEPS 21
#define RING_STEP_DEGREES 18

void func_002775E8(void);
int func_002775F0(void);
int recordDefaultColor(void);
void func_00277610(float angle, Vector4 *screenPoints, Vector4 *texturePoints);
extern float D_0069DDF4;

/*
 * Draw handler of record kind 7 (D_0063D758[6]): draws a textured ring as one triangle strip: for every step an inner vertex (func_002775F0's colour) and an
 * outer one (recordDefaultColor), Z and fog 0x7FFFFFFF.
 */
void func_002772A8(void)
{
    Vector4 screenPoints;
    Vector4 texturePoints;
    GsAdPair *pairs;
    XglPacket *packet;
    unsigned int innerColor;
    unsigned int outerColor;
    int pairCount;
    int step;
    int degrees;

    func_002775E8();
    innerColor = func_002775F0();
    outerColor = recordDefaultColor();
    packet = xglPacketGetCurrent();
    pairs = (GsAdPair *) SCRATCHPAD_BASE;
    pairCount = 0;
    pairs[pairCount].ad.reg = GIF_AD;
    pairs[pairCount].ad.data = ((u64)0x10000000 << 32) | 0x8000;
    pairCount++;
    pairs[pairCount].ad.reg = GS_TEXFLUSH;
    pairs[pairCount].ad.data = 0;
    pairCount++;
    pairs[pairCount].ad.reg = GS_TEST_1;
    pairs[pairCount].ad.data = GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 2);
    pairCount++;
    pairs[pairCount].ad.reg = GS_TEX1_1;
    pairs[pairCount].ad.data = 0;
    pairCount++;
    pairs[pairCount].ad.reg = GS_TEXA;
    pairs[pairCount].ad.data = 0;
    pairCount++;
    pairs[pairCount].ad.reg = GS_ALPHA_1;
    pairs[pairCount].ad.data = func_002746F8(0);
    pairCount++;
    pairs[pairCount].ad.reg = GS_TEX0_1;
    pairs[pairCount].ad.data = func_002745A8(1, 0);
    pairCount++;
    pairs[pairCount].ad.reg = GS_FRAME_1;
    pairs[pairCount].ad.data = func_00274650(0, 0);
    pairCount++;
    pairs[pairCount].ad.data = 0x15C;
    pairs[pairCount].ad.reg = GS_PRIM;
    pairCount++;
    degrees = 0;
    for (step = 0; step < RING_STEPS; step++) {
        func_00277610((float)degrees * D_0069DDF4, &screenPoints, &texturePoints);
        pairs[pairCount].ad.reg = GS_RGBAQ;
        pairs[pairCount].ad.data = innerColor;
        pairCount++;
        pairs[pairCount].ad.reg = GS_UV;
        pairs[pairCount].ad.data = GS_SET_UV(func_002A6A90(texturePoints.x * 16.0f),
                                             func_002A6A90(texturePoints.y * 16.0f));
        pairCount++;
        pairs[pairCount].ad.reg = GS_XYZF2;
        pairs[pairCount].ad.data = GS_SET_XYZ((int)((screenPoints.x + 1792.0f) * 16.0f),
                                              (int)((screenPoints.y + 1824.0f) * 16.0f), 0x7FFFFFFF);
        pairCount++;
        pairs[pairCount].ad.data = outerColor;
        pairs[pairCount].ad.reg = GS_RGBAQ;
        pairCount++;
        pairs[pairCount].ad.reg = GS_UV;
        pairs[pairCount].ad.data = GS_SET_UV(func_002A6A90(texturePoints.z * 16.0f),
                                             func_002A6A90(texturePoints.w * 16.0f));
        pairCount++;
        pairs[pairCount].ad.reg = GS_XYZF2;
        pairs[pairCount].ad.data = GS_SET_XYZ((int)((screenPoints.z + 1792.0f) * 16.0f),
                                              (int)((screenPoints.w + 1824.0f) * 16.0f), 0x7FFFFFFF);
        pairCount++;
        degrees += RING_STEP_DEGREES;
    }
    pairs[0].ad.data += pairCount - 1;
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    sceVif1PkAddDirectDataN(packet, pairs, pairCount);
    sceVif1PkCloseDirectHLCode(packet);
}

void func_002775E8(void)
{
}

int func_002775F0(void)
{
  return 0x40808080;
}

int recordDefaultColor(void)
{
  return 0x40808080;
}

/*
 * One step of the ring for angle: texturePoints gets the texture coordinates around (256, 224), radius 150 in x/y
 * and 300 in z/w; screenPoints the screen positions, radius 150 in x/y and 350 in z/w.
 */
void func_00277610(float angle, Vector4 *screenPoints, Vector4 *texturePoints)
{
    float textureSin;
    float textureCos;
    float screenSin;
    float screenCos;
    float radius;

    /* Set before the first sinf: the shared 150.0f then stays in one callee-saved register. */
    radius = 150.0f;
    textureSin = sinf(angle);
    textureCos = cosf(angle);
    texturePoints->x = textureCos * radius + 256.0f;
    texturePoints->y = textureSin * radius + 224.0f;
    texturePoints->z = textureCos * 300.0f + 256.0f;
    texturePoints->w = textureSin * 300.0f + 224.0f;
    /* The screen points use the same angle (an offset of 0.0f). */
    angle += 0.0f;
    screenSin = sinf(angle);
    screenCos = cosf(angle);
    screenPoints->x = screenCos * radius + 256.0f;
    screenPoints->y = screenSin * radius + 224.0f;
    screenPoints->z = screenCos * 350.0f + 256.0f;
    screenPoints->w = screenSin * 350.0f + 224.0f;
}
