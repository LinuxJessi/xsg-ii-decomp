#include "common.h"
#include "main/tu207.h"

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_00274808);

/* Inclusive range test: lower <= value <= upper. */
int xglIsInRange(int value, int lower, int upper)
{
  return lower <= value && value <= upper;
}

struct XglPacket;

/* One 0x7C-byte record of recordBanks; a bank is ten of them (0x4D8 bytes). */
typedef struct Record {
    unsigned char type; /* +0x00: 0 unused, 1..21 picks the draw handler D_0063D758[type - 1] */
    unsigned char unmodeled_01[3];
    int params[30];     /* +0x04..+0x7C: parameter words of the type; func_00275710 sets the first ten */
} Record;

typedef struct RecordBank {
    Record records[10];
} RecordBank;

/* Declared as bytes, as recordClearBank below reads it. func_00274850 and func_00275710 index it as bytes:
 * through (RecordBank *) gcc emits addu base,offset instead of offset,base and, in func_00275710, merges
 * the two record address computations the original keeps apart. */
extern unsigned char recordBanks[];

/* Local view of sRender (0x00645820; RenderFrameState in main/tu093.h does not model +0x3C): the
 * render callback, called with the frame's packet (DefocusRenderState.callback in game_defocus.c). */
typedef struct RecordRenderState {
    unsigned char unmodeled_00[0x3C];
    void (*callback)(struct XglPacket *packet);
} RecordRenderState;

extern RecordRenderState sRender;

void func_00275830(struct XglPacket *packet);

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
        ((RecordBank *)recordBanks)[bankIndex].records[slotIndex].params[i] = params[i];
    }
    func_00274850();
}

void recordClear(void *record);
extern unsigned char recordBanks[];

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

void sceVif1PkCnt(struct XglPacket *, int);
void sceVif1PkOpenDirectHLCode(struct XglPacket *, int);
void sceVif1PkAddDirectDataN(struct XglPacket *, const void *, int);
void sceVif1PkCloseDirectHLCode(struct XglPacket *);
int func_00275968(void);
extern int recordUpdatePaused;
/* GIF data sent before (9 quadwords) and after (3 quadwords) the records are drawn. */
extern const unsigned char D_0063D6C8[];
extern const unsigned char D_0063D7B0[];
/* Draw handlers of record types 1..21. */
extern void (*D_0063D758[])(Record *record, struct XglPacket *packet);

/* Draws every used record of the three banks between two fixed GIF blocks, then clears the banks and
 * removes itself as the render callback. */
void func_00275830(struct XglPacket *packet)
{
    RecordBank *bank;
    Record *record;
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

/* Flag that suspends the per-frame record updates; 1 while suspended. */
extern int recordUpdatePaused;

void recordPauseUpdates(void)
{
  recordUpdatePaused = 1;
}

#include "gs.h"

/* The EE scratchpad, where the packet is built before it is sent. */
#define SCRATCHPAD_BASE 0x70000000

typedef struct XglPacket XglPacket;

/* One A+D qword of a GS packet: the register value, then the register address. A union as tu229.c's TalkGsPair:
   with a plain struct gcc moves func_00275F70's point reads above the packet stores. */
typedef union GsAdPair {
    struct {
        u64 data;
        u64 reg;
    } ad;
} GsAdPair;

XglPacket *xglPacketGetCurrent(void);
void sceVif1PkCnt(XglPacket *packet, int count);
void sceVif1PkOpenDirectHLCode(XglPacket *packet, int mode);
void sceVif1PkAddDirectDataN(XglPacket *packet, const void *data, int qwc);
void sceVif1PkCloseDirectHLCode(XglPacket *packet);
u64 func_002745A8(int buffer, int flag);
u64 func_00274650(int buffer, int flag);
int func_002746F8(int blend);
void recordStepAngle(int stage);
int func_00275CF0(int mode, int fallback);
int func_00275D30(int mode, int alpha);
int func_00275E08(void);

/* Draw handler of record type 0 (D_0063D758[0]): one textured sprite over the 512x448 screen, inset by the
   stage's size, in the stage's color; at stage 2 the sprite is drawn a second time in the pulsing color. record[1]
   is the stage, record[2] the alpha, record[3] the depth. */
void func_002759F8(int *record)
{
    int stage;
    int alpha;
    int z;
    unsigned int color;
    int size;
    XglPacket *packet;
    GsAdPair *pairs;
    int pairCount;

    stage = record[1];
    alpha = record[2];
    z = record[3];
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

int func_00274808(int, int, int);
extern int recordAngle;
float sinf(float angle);

/* Sprite color of stage mode: grey 0x808080 under alpha at stage 1, grey at half alpha at stage 0, at stage 2 the
   alpha 80 + 5 * sin(recordAngle degrees) put into 0..127 by func_00274808; 0 for any other stage. */
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
    if (recordUpdatePaused == 0 && stage == 2) {
        recordAngle += 0x5C;
        recordAngle = func_00274808(recordAngle, 0, 0x168);
    }
}

/* One 0x7C-byte draw record as the gsQueue functions read it (tu213.c models the color and halveRgb words of
   the same record as GsQueueContext): gsQueueBeginTriStrip passes frame to gsQueueSetFrame, tex0 to
   gsQueueSetTex0 and blendKind with color to gsQueueSetBlendColor; func_0027A6F0 draws count sprites at depth
   from screenRect with texture coordinates from textureRect. */
typedef struct GsQueueContext {
    unsigned char type;
    unsigned char unmodeled_01[3];
    int frame;
    int tex0;
    int color;
    int depth;
    int count;
    int halveRgb;
    int blendKind;
    int screenRect[4];
    int textureRect[4];
    unsigned char unmodeled_40[0x7C - 0x40];
} GsQueueContext;

void func_0027A6F0(GsQueueContext *context, int packet);

/* Draw handler of record type 1 (D_0063D758[1]): draws the texture record[2] over the whole 512x448 screen
   through a type 20 sprite record. */
void func_00275EE8(int *record, int packet)
{
    GsQueueContext sprite;

    sprite.type = 0x14;
    sprite.frame = 2;
    sprite.tex0 = record[2];
    sprite.color = 0x80FFFFFF;
    sprite.depth = 0x7FFFFFFF;
    sprite.count = 1;
    sprite.halveRgb = 1;
    sprite.blendKind = 0;
    sprite.screenRect[0] = 0;
    sprite.screenRect[1] = 0;
    sprite.screenRect[2] = 0x200;
    sprite.screenRect[3] = 0x1C0;
    sprite.textureRect[0] = 0;
    sprite.textureRect[1] = 0;
    sprite.textureRect[2] = 0x200;
    sprite.textureRect[3] = 0x1C0;
    func_0027A6F0(&sprite, packet);
}

void func_00276400(int step);
void func_00276350(float *point, float *clampedPoint, float angle, float radius);
/* libgcc float to 64-bit integer conversion (in the libgcc block between __moddi3 and fptodp). */
long func_002A6A90(float value);
extern float D_0069DDD8;
extern int D_0069DDDC;

/* Draw handler of record type 2 (D_0063D758[2]): a triangle fan from the screen centre through octagonal rings of
   radius D_0069DDDC, D_0069DDDC + 25, ... below 620 (func_00276400 advances D_0069DDDC by record[1]); each ring
   vertex takes its texture coordinates from the point clamped to radius 224. record[2] is the depth. */
void func_00275F70(int *record)
{
    int z;
    XglPacket *packet;
    GsAdPair *pairs;
    int pairCount;
    int radius;
    int side;
    float point[2];
    float clampedPoint[2];

    z = record[2];
    func_00276400(record[1]);
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

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_00276400);

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_00276458);

extern int recordGridWidth;
extern int recordGridHeight;
extern unsigned char recordGridCells[];

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

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_00276838);

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_00276D68);

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_00277100);

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_00277168);

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_002772A8);

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

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_00277610);
