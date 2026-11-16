#include "common.h"
#include "shared.h"
#include "vu0_macro.h"
#include "gs.h"
#include "main/tu207.h"
#include "main/xgl_2.h"

/* Wraps value once into lower..upper: one period (upper - lower + 1) down when above upper, one up when below lower. */
int wrapIntRange(int value, int lower, int upper)
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

/* One rectangle of a draw record: top, left, right, bottom (tu210.c's GsQueueRect). */
typedef struct GsQueueRect {
    int top;
    int left;
    int right;
    int bottom;
} GsQueueRect;

/*
 * One 0x7C-byte draw record (tu213.c's GsQueueContext; recordBanks holds three banks of ten). The type byte picks
 * the draw handler D_0063D758[type - 1], and that handler decides what the words after it hold: the type 1 handler
 * reads +0x04 as a stage, the type 2, 4, 8-14 and 20 handlers as the gsQueue frame (types 4 and 8-14 pass the
 * record to gsQueueBeginTriStrip or gsQueueBeginSprites), the type 3 handler as a ring step, the type 6 handler as
 * an alpha. Only these per-type arms are a union; color (+0x0C) and halveRgb (+0x18) are tu213.c's names, the
 * type 8-14 arms use the field names of tu208.c's and tu210.c's views of the record.
 */
typedef struct GsQueueContext {
    unsigned char type; /* +0x00: 0 unused, 1..21 picks the draw handler D_0063D758[type - 1] */
    unsigned char unmodeled_01[3];
    union {
        /* Any type: recordSetSlot copies the first ten words without looking at the type. */
        int params[30]; /* +0x04..+0x7B */
        /* Types 2 and 20 (recordDrawType2 fills one for func_0027A6F0): gsQueueBeginTriStrip passes frame to
           gsQueueSetFrame, tex0 to gsQueueSetTex0 and blendKind with color to gsQueueSetBlendColor; count is the
           number of passes, screenRect and textureRect the sprite. */
        struct {
            int frame;          /* +0x04 */
            int tex0;           /* +0x08 */
            int color;          /* +0x0C */
            int depth;          /* +0x10 */
            int count;          /* +0x14 */
            int halveRgb;       /* +0x18 */
            int blendKind;      /* +0x1C */
            int screenRect[4];  /* +0x20 */
            int textureRect[4]; /* +0x30 */
        } queue;
        /* Type 4 (recordDrawType4): after the gsQueue words of queue, the number of grid cells to set. */
        struct {
            unsigned char queueWords[0x20 - 0x04]; /* +0x04: frame..blendKind, see queue */
            int cellCount;                         /* +0x20 */
        } gridFill;
        /* Type 8 (recordDrawStrip; tu208.c's RecordGradient): two colours over a rectangle, along y when vertical. */
        struct {
            unsigned char queueWords[0x20 - 0x04]; /* +0x04: frame..blendKind, see queue */
            int color0;                            /* +0x20 */
            int color1;                            /* +0x24 */
            int top;                               /* +0x28 */
            int left;                              /* +0x2C */
            int right;                             /* +0x30 */
            int bottom;                            /* +0x34 */
            int vertical;                          /* +0x38 */
        } gradient;
        /* Type 9 (recordDrawCurve; tu208.c's RecordBatch): the graph's colour, baseline, time range, top, columns,
           rows and value ceiling. */
        struct {
            unsigned char queueWords[0x20 - 0x04]; /* +0x04: frame..blendKind, see queue */
            int color;                             /* +0x20 */
            int bottom;                            /* +0x24 */
            int startTime;                         /* +0x28 */
            int endTime;                           /* +0x2C */
            int top;                               /* +0x30 */
            int columns;                           /* +0x34 */
            int rows;                              /* +0x38 */
            unsigned char unmodeled_3c[0x5C - 0x3C];
            int maxValue;                          /* +0x5C */
        } graph;
        /* Type 10 (orbitDrawRing; tu210.c's GsQueueRing). */
        struct {
            unsigned char queueWords[0x20 - 0x04]; /* +0x04: frame..blendKind, see queue */
            int endColor;                          /* +0x20 */
            int centerX;                           /* +0x24 */
            int centerY;                           /* +0x28 */
            int radius;                            /* +0x2C */
            int innerRadius;                       /* +0x30 */
            int phaseSpeed;                        /* +0x34 */
            int segments;                          /* +0x38 */
            int amplitude;                         /* +0x3C */
        } ring;
        /* Type 11 (quadDrawPreset; tu210.c's GsQueuePresetQuad): a position preset and a texture rectangle. */
        struct {
            unsigned char queueWords[0x20 - 0x04]; /* +0x04: frame..blendKind, see queue */
            int positionPreset;                    /* +0x20 */
            unsigned char unmodeled_24[0x30 - 0x24];
            GsQueueRect texture;                   /* +0x30 */
        } presetQuad;
        /* Type 12 (quadDrawPresetUv; tu210.c's GsQueuePresetUvQuad): a screen rectangle and a texture preset. */
        struct {
            unsigned char queueWords[0x20 - 0x04]; /* +0x04: frame..blendKind, see queue */
            GsQueueRect screen;                    /* +0x20 */
            int texturePreset;                     /* +0x30 */
        } presetUvQuad;
        /* Type 13 (circleDrawSegments; tu210.c's GsQueueCircle). */
        struct {
            unsigned char queueWords[0x20 - 0x04]; /* +0x04: frame..blendKind, see queue */
            GsQueueRect screen;                    /* +0x20 */
            GsQueueRect texture;                   /* +0x30 */
            int radiusStep;                        /* +0x40 */
            int ringCount;                         /* +0x44 */
            int endColor;                          /* +0x48 */
            int startAngle;                        /* +0x4C */
            int segments;                          /* +0x50 */
            int centerDisabled;                    /* +0x54 */
        } circle;
        /* Type 14 (gridDrawCells; tu210.c's GsQueueGrid). */
        struct {
            unsigned char queueWords[0x20 - 0x04]; /* +0x04: frame..blendKind, see queue */
            int endColor;                          /* +0x20 */
            GsQueueRect screen;                    /* +0x24 */
            int columns;                           /* +0x34 */
            int rows;                              /* +0x38 */
            unsigned char unmodeled_3c[0x68 - 0x3C];
            int transformStart;                    /* +0x68 */
            int transformEnd;                      /* +0x6C */
        } grid;
        /* Type 1 (recordDrawType1): the stage (0..2) and alpha passed to recordStageColor/recordStageSize, and the Z of
           the full-screen sprite. */
        struct {
            int stage; /* +0x04 */
            int alpha; /* +0x08 */
            int depth; /* +0x0C */
        } fade;
        /* Type 3 (recordDrawType3): the ring radius step passed to recordAdvanceRingRadius, and the Z of the fan. */
        struct {
            int step;  /* +0x04 */
            int depth; /* +0x08 */
        } tunnel;
        /* Type 6 (recordDrawType6): the alpha of the RGBAQ word and the Z and fog word. */
        struct {
            int alpha; /* +0x04 */
            int z;     /* +0x08 */
        } warp;
    } u;
} GsQueueContext;

typedef struct RecordBank {
    GsQueueContext records[10];
} RecordBank;

/* Declared as bytes, as recordClearBank below reads it. recordUpdateRenderHook indexes it as bytes and recordSetSlot
 * steps to the bank as bytes: through ((RecordBank *) recordBanks)[bankIndex] gcc adds the base before the slot
 * offset (fd.sh recordSetSlot: 12/32 in place, 32/32 with the byte step to the bank). */
extern unsigned char recordBanks[];

/* Local view of sRender (0x00645820; RenderFrameState in main/tu093.h does not model +0x3C): the
 * render callback, called with the frame's packet (DefocusRenderState.callback in game_defocus.c). */
typedef struct RecordRenderState {
    unsigned char unmodeled_00[0x3C];
    void (*callback)(XglPacket *packet);
} RecordRenderState;

extern RecordRenderState sRender;

void recordRenderAll(XglPacket *packet);

/* Installs recordRenderAll as the render callback while any record is used, else removes it. The scan
 * covers 30 banks although recordBanks holds 3 (0xE88 bytes), as the original does. */
void recordUpdateRenderHook(void)
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
        sRender.callback = recordRenderAll;
    }
}

void recordClear(void *record);
/* Flag that suspends the per-frame record updates; 1 while suspended. A struct, not an int: as a scalar,
 * sched2 lifts recordSetPreset case 16's flag load above its params[0] reload (fd.sh 909/913); as a struct member
 * the load stays behind the record stores (913/913). */
typedef struct RecordUpdateFlag {
    int paused;
} RecordUpdateFlag;
extern RecordUpdateFlag recordUpdatePaused;

/*
 * Fills bank bankIndex (nothing happens unless it is below 3) with one of the record presets 0..24 (the second
 * argument, through jtbl_0068D630; any other value leaves the bank cleared): clears the bank's ten records, writes
 * the preset's records, then updates the render callback (recordUpdateRenderHook). Words no record arm names stay
 * params[] (all of types 7 and 16-19, the unmodeled ones of types 9 and 14).
 * Cases 14 and 15 declare their locals in the case block: at function scope they become global pseudos and $16
 * and $17 trade places from the prologue on (fd.sh 244/913).
 */
void recordSetPreset(int bankIndex, int preset, int depth, int *params)
{
    int slot;
    GsQueueContext *records;
    GsQueueContext *record;

    records = ((RecordBank *)recordBanks)[bankIndex].records;
    if (bankIndex >= 3) {
        return;
    }
    record = records;
    for (slot = 0; slot < 10; slot++) {
        recordClear(record);
        record++;
    }
    switch (preset) {
    case 1:
        records[0].type = 4;
        records[0].u.queue.frame = 0;
        records[0].u.queue.tex0 = 2;
        records[0].u.queue.color = params[0];
        records[0].u.queue.depth = depth;
        records[0].u.queue.count = 1;
        records[0].u.queue.halveRgb = 1;
        records[0].u.queue.blendKind = 0;
        records[0].u.gridFill.cellCount = params[1];
        break;
    case 2:
        records[0].type = 11;
        records[0].u.queue.frame = 2;
        records[0].u.queue.tex0 = 0;
        records[0].u.queue.color = 0x80FFFFFF;
        records[0].u.queue.depth = 0x7FFFFFFF;
        records[0].u.queue.count = 1;
        records[0].u.queue.halveRgb = 1;
        records[0].u.queue.blendKind = 0;
        records[0].u.presetQuad.positionPreset = 0;
        records[0].u.presetQuad.texture.top = 0;
        records[0].u.presetQuad.texture.left = 0;
        records[0].u.presetQuad.texture.right = 512;
        records[0].u.presetQuad.texture.bottom = 448;
        records[1].type = 12;
        records[1].u.queue.frame = 0;
        records[1].u.queue.tex0 = 2;
        records[1].u.queue.color = 0x80FFFFFF;
        records[1].u.queue.depth = depth;
        records[1].u.queue.count = 1;
        records[1].u.queue.halveRgb = 1;
        records[1].u.queue.blendKind = 0;
        records[1].u.presetUvQuad.screen.top = 0;
        records[1].u.presetUvQuad.screen.left = 0;
        records[1].u.presetUvQuad.screen.right = 512;
        records[1].u.presetUvQuad.screen.bottom = 448;
        records[1].u.presetUvQuad.texturePreset = 0;
        break;
    case 3:
        records[0].type = 2;
        records[0].u.queue.tex0 = 0;
        records[1].type = 17;
        records[1].u.queue.frame = 0;
        records[1].u.queue.tex0 = 2;
        records[1].u.queue.color = params[0];
        records[1].u.queue.depth = depth;
        records[1].u.queue.count = 1;
        records[1].u.queue.halveRgb = 1;
        records[1].u.queue.blendKind = params[1];
        records[1].u.params[7] = -params[3];
        records[1].u.params[8] = -params[2];
        records[1].u.params[9] = params[2] + 512;
        records[1].u.params[10] = params[3] + 448;
        records[1].u.params[11] = params[2];
        records[1].u.params[12] = -(params[2] >> 1);
        records[1].u.params[13] = params[3];
        records[1].u.params[14] = -(params[3] >> 1);
        records[1].u.params[15] = 8;
        break;
    case 4:
        records[0].type = 2;
        records[0].u.queue.tex0 = 0;
        records[1].type = 18;
        records[1].u.queue.frame = 0;
        records[1].u.queue.tex0 = 2;
        records[1].u.queue.color = params[0];
        records[1].u.queue.depth = depth;
        records[1].u.queue.count = 1;
        records[1].u.queue.halveRgb = 1;
        records[1].u.queue.blendKind = params[1];
        records[1].u.params[7] = params[5];
        records[1].u.params[8] = -params[3];
        records[1].u.params[9] = -params[4];
        records[1].u.params[10] = params[4] + 512;
        records[1].u.params[11] = params[3] + 448;
        records[1].u.params[15] = params[3];
        records[1].u.params[13] = 20;
        records[1].u.params[14] = params[2];
        records[1].u.params[12] = params[4];
        records[1].u.params[16] = 20;
        records[1].u.params[17] = params[2];
        records[1].u.params[18] = 7;
        break;
    case 5:
        records[0].type = 2;
        records[0].u.queue.tex0 = 0;
        records[1].type = 19;
        records[1].u.queue.frame = 0;
        records[1].u.queue.tex0 = 2;
        records[1].u.queue.color = params[0];
        records[1].u.queue.depth = depth;
        records[1].u.queue.count = 1;
        records[1].u.queue.halveRgb = 1;
        records[1].u.queue.blendKind = params[1];
        records[1].u.params[7] = params[4];
        records[1].u.params[8] = -7;
        records[1].u.params[9] = -7;
        records[1].u.params[10] = 519;
        records[1].u.params[11] = 455;
        records[1].u.params[12] = 256;
        records[1].u.params[13] = 224;
        records[1].u.params[14] = 0;
        records[1].u.params[15] = 360;
        records[1].u.params[16] = params[2];
        records[1].u.params[17] = 6;
        records[1].u.params[18] = params[3];
        records[1].u.params[19] = params[5];
        break;
    case 6:
        records[0].type = 19;
        records[0].u.queue.frame = 0;
        records[0].u.queue.tex0 = 1;
        records[0].u.queue.color = 0x10FFFFFF;
        records[0].u.queue.depth = depth;
        records[0].u.queue.count = 1;
        records[0].u.queue.halveRgb = 0;
        records[0].u.queue.blendKind = 0;
        records[0].u.params[7] = 0x80FFFFFF;
        records[0].u.params[8] = -7;
        records[0].u.params[9] = -7;
        records[0].u.params[10] = 519;
        records[0].u.params[11] = 455;
        records[0].u.params[12] = 256;
        records[0].u.params[13] = 224;
        records[0].u.params[14] = 0;
        records[0].u.params[15] = (params[0] << 6);
        records[0].u.params[16] = 2;
        records[0].u.params[17] = 12;
        records[0].u.params[18] = params[1];
        records[0].u.params[19] = 14;
        break;
    case 7:
        records[0].type = 20;
        records[0].u.queue.frame = 2;
        records[0].u.queue.tex0 = 0;
        records[0].u.queue.color = 0x80FFFFFF;
        records[0].u.queue.depth = 0xFFFFFFF;
        records[0].u.queue.count = 1;
        records[0].u.queue.halveRgb = 1;
        records[0].u.queue.blendKind = 0;
        records[0].u.queue.screenRect[0] = 0;
        records[0].u.queue.screenRect[1] = 0;
        records[0].u.queue.screenRect[2] = 512;
        records[0].u.queue.screenRect[3] = 448;
        records[0].u.queue.textureRect[0] = 0;
        records[0].u.queue.textureRect[1] = 0;
        records[0].u.queue.textureRect[2] = 512;
        records[0].u.queue.textureRect[3] = 448;
        records[1].type = 20;
        records[1].u.queue.frame = 2;
        records[1].u.queue.tex0 = 0;
        records[1].u.queue.color = params[0];
        records[1].u.queue.depth = 0xFFFFFFF;
        records[1].u.queue.count = 1;
        records[1].u.queue.halveRgb = 0;
        records[1].u.queue.blendKind = 2;
        records[1].u.queue.screenRect[0] = 0;
        records[1].u.queue.screenRect[1] = 0;
        records[1].u.queue.screenRect[2] = 512;
        records[1].u.queue.screenRect[3] = 448;
        records[2].type = 13;
        records[2].u.queue.frame = 0;
        records[2].u.queue.tex0 = 2;
        records[2].u.queue.color = 0x40FFFFFF;
        records[2].u.queue.depth = depth;
        records[2].u.queue.count = 1;
        records[2].u.queue.halveRgb = 1;
        records[2].u.queue.blendKind = 1;
        records[2].u.circle.screen.top = 0;
        records[2].u.circle.screen.left = 0;
        records[2].u.circle.screen.right = 512;
        records[2].u.circle.screen.bottom = 448;
        records[2].u.circle.texture.top = 0;
        records[2].u.circle.texture.left = 0;
        records[2].u.circle.texture.right = 512;
        records[2].u.circle.texture.bottom = 448;
        records[2].u.circle.radiusStep = 4;
        records[2].u.circle.ringCount = params[2];
        records[2].u.circle.endColor = (params[0] & 0xFFFFFF);
        records[2].u.circle.startAngle = params[3];
        records[2].u.circle.segments = params[1];
        records[2].u.circle.centerDisabled = 0;
        break;
    case 8:
        records[0].type = 2;
        records[0].u.queue.tex0 = 0;
        records[1].type = 20;
        records[1].u.queue.frame = 2;
        records[1].u.queue.tex0 = 0;
        records[1].u.queue.color =
            (((((128 - params[1]) << 16) | (128 - params[1])) | ((128 - params[1]) << 8)) | 0x80000000);
        records[1].u.queue.depth = 0xFFFFFFF;
        records[1].u.queue.count = 1;
        records[1].u.queue.halveRgb = 0;
        records[1].u.queue.blendKind = 2;
        records[1].u.queue.screenRect[0] = 0;
        records[1].u.queue.screenRect[1] = 0;
        records[1].u.queue.screenRect[2] = 512;
        records[1].u.queue.screenRect[3] = 448;
        records[2].type = 20;
        records[2].u.queue.frame = 0;
        records[2].u.queue.tex0 = 0;
        records[2].u.queue.color = ((((params[2] << 16) | params[2]) | (params[2] << 8)) | 0x80000000);
        records[2].u.queue.depth = 0xFFFFFFF;
        records[2].u.queue.count = 1;
        records[2].u.queue.halveRgb = 0;
        records[2].u.queue.blendKind = 2;
        records[2].u.queue.screenRect[0] = 0;
        records[2].u.queue.screenRect[1] = 0;
        records[2].u.queue.screenRect[2] = 512;
        records[2].u.queue.screenRect[3] = 448;
        records[3].type = 20;
        records[3].u.queue.frame = 0;
        records[3].u.queue.tex0 = 2;
        records[3].u.queue.color = 0x80FFFFFF;
        records[3].u.queue.depth = 0xFFFFFFF;
        records[3].u.queue.count = 1;
        records[3].u.queue.halveRgb = 1;
        records[3].u.queue.blendKind = 1;
        records[3].u.queue.screenRect[0] = 0;
        records[3].u.queue.screenRect[1] = 0;
        records[3].u.queue.screenRect[2] = 512;
        records[3].u.queue.screenRect[3] = 448;
        records[3].u.queue.textureRect[0] = 0;
        records[3].u.queue.textureRect[1] = 0;
        records[3].u.queue.textureRect[2] = 512;
        records[3].u.queue.textureRect[3] = 448;
        break;
    case 9:
        records[0].type = 2;
        records[0].u.queue.tex0 = 0;
        records[1].type = 20;
        records[1].u.queue.frame = 2;
        records[1].u.queue.tex0 = 2;
        records[1].u.queue.color = 0x80808080;
        records[1].u.queue.depth = 0xFFFFFFF;
        records[1].u.queue.count = params[1];
        records[1].u.queue.halveRgb = 1;
        records[1].u.queue.blendKind = 2;
        records[1].u.queue.screenRect[0] = 0;
        records[1].u.queue.screenRect[1] = 0;
        records[1].u.queue.screenRect[2] = 512;
        records[1].u.queue.screenRect[3] = 448;
        records[1].u.queue.textureRect[0] = 0;
        records[1].u.queue.textureRect[1] = 0;
        records[1].u.queue.textureRect[2] = 512;
        records[1].u.queue.textureRect[3] = 448;
        records[2].type = 20;
        records[2].u.queue.frame = 2;
        records[2].u.queue.tex0 = 2;
        records[2].u.queue.color = 0x80808080;
        records[2].u.queue.depth = 0xFFFFFFF;
        records[2].u.queue.count = params[1];
        records[2].u.queue.halveRgb = 1;
        records[2].u.queue.blendKind = 1;
        records[2].u.queue.screenRect[0] = 0;
        records[2].u.queue.screenRect[1] = 0;
        records[2].u.queue.screenRect[2] = 512;
        records[2].u.queue.screenRect[3] = 448;
        records[2].u.queue.textureRect[0] = 0;
        records[2].u.queue.textureRect[1] = 0;
        records[2].u.queue.textureRect[2] = 512;
        records[2].u.queue.textureRect[3] = 448;
        records[3].type = 20;
        records[3].u.queue.frame = 0;
        records[3].u.queue.tex0 = 2;
        records[3].u.queue.color = params[0];
        records[3].u.queue.depth = depth;
        records[3].u.queue.count = 1;
        records[3].u.queue.halveRgb = 1;
        records[3].u.queue.blendKind = 0;
        records[3].u.queue.screenRect[0] = 0;
        records[3].u.queue.screenRect[1] = 0;
        records[3].u.queue.screenRect[2] = 512;
        records[3].u.queue.screenRect[3] = 448;
        records[3].u.queue.textureRect[0] = 0;
        records[3].u.queue.textureRect[1] = 0;
        records[3].u.queue.textureRect[2] = 512;
        records[3].u.queue.textureRect[3] = 448;
        break;
    case 10:
        records[0].type = 20;
        records[0].u.queue.frame = 0;
        records[0].u.queue.tex0 = 2;
        records[0].u.queue.color = params[0];
        records[0].u.queue.depth = depth;
        records[0].u.queue.count = 1;
        records[0].u.queue.halveRgb = params[1];
        records[0].u.queue.blendKind = 0;
        records[0].u.queue.screenRect[0] = 0;
        records[0].u.queue.screenRect[1] = 0;
        records[0].u.queue.screenRect[2] = 512;
        records[0].u.queue.screenRect[3] = 448;
        records[0].u.queue.textureRect[0] = 0;
        records[0].u.queue.textureRect[1] = 0;
        records[0].u.queue.textureRect[2] = 512;
        records[0].u.queue.textureRect[3] = 448;
        break;
    case 11:
        records[0].type = 20;
        records[0].u.queue.frame = 2;
        records[0].u.queue.tex0 = 0;
        records[0].u.queue.color = 0x7FFFFFFF;
        records[0].u.queue.depth = 0xFFFFFFF;
        records[0].u.queue.count = 1;
        records[0].u.queue.halveRgb = 1;
        records[0].u.queue.blendKind = 0;
        records[0].u.queue.screenRect[0] = 0;
        records[0].u.queue.screenRect[1] = 0;
        records[0].u.queue.screenRect[2] = 512;
        records[0].u.queue.screenRect[3] = 448;
        records[0].u.queue.textureRect[0] = 0;
        records[0].u.queue.textureRect[1] = 0;
        records[0].u.queue.textureRect[2] = 512;
        records[0].u.queue.textureRect[3] = 448;
        records[1].type = 20;
        records[1].u.queue.frame = 0;
        records[1].u.queue.tex0 = 2;
        records[1].u.queue.color = params[0];
        records[1].u.queue.depth = depth;
        records[1].u.queue.count = 1;
        records[1].u.queue.halveRgb = 1;
        records[1].u.queue.blendKind = 0;
        records[1].u.queue.screenRect[0] = params[1];
        records[1].u.queue.screenRect[1] = params[1];
        records[1].u.queue.screenRect[2] = params[1] + 512;
        records[1].u.queue.screenRect[3] = params[1] + 448;
        records[1].u.queue.textureRect[0] = 0;
        records[1].u.queue.textureRect[1] = 0;
        records[1].u.queue.textureRect[2] = 512;
        records[1].u.queue.textureRect[3] = 448;
        break;
    case 12:
        records[0].type = 20;
        records[0].u.queue.frame = 0;
        records[0].u.queue.tex0 = 1;
        records[0].u.queue.color = params[0];
        records[0].u.queue.depth = depth;
        records[0].u.queue.count = 1;
        records[0].u.queue.halveRgb = 1;
        records[0].u.queue.blendKind = 0;
        records[0].u.queue.screenRect[0] = 0;
        records[0].u.queue.screenRect[1] = 0;
        records[0].u.queue.screenRect[2] = 512;
        records[0].u.queue.screenRect[3] = 448;
        records[0].u.queue.textureRect[0] = 0;
        records[0].u.queue.textureRect[1] = 0;
        records[0].u.queue.textureRect[2] = 512;
        records[0].u.queue.textureRect[3] = 448;
        break;
    case 13:
        records[0].type = 8;
        records[0].u.queue.frame = 0;
        records[0].u.queue.tex0 = 0;
        records[0].u.queue.color = 0;
        records[0].u.queue.depth = depth;
        records[0].u.queue.count = 1;
        records[0].u.queue.halveRgb = 0;
        records[0].u.queue.blendKind = 0;
        records[0].u.gradient.color0 = ((params[1] << 24) | params[0]);
        records[0].u.gradient.color1 = ((params[2] << 24) | params[0]);
        records[0].u.gradient.top = 0;
        records[0].u.gradient.left = 0;
        records[0].u.gradient.right = 512;
        records[0].u.gradient.bottom = 448;
        records[0].u.gradient.vertical = params[3];
        break;
    case 14: {
        int columns;
        int rows;
        int bottom;
        int top;
        int start;
        int end;
        int halfWidth;
        int halfHeight;

        columns = 9;
        rows = 6;
        bottom = -3;
        top = 451;
        start = -6;
        end = 515;
        halfWidth = (end - start) / columns / 2;
        halfHeight = (top - bottom) / rows / 2;
        records[0].type = 9;
        records[0].u.queue.frame = 0;
        records[0].u.queue.tex0 = 2;
        records[0].u.queue.depth = depth;
        records[0].u.queue.count = 1;
        records[0].u.queue.halveRgb = params[4];
        records[0].u.queue.blendKind = 0;
        records[0].u.graph.bottom = bottom;
        records[0].u.graph.top = top;
        records[0].u.graph.columns = columns;
        records[0].u.graph.rows = rows;
        records[0].u.params[18] = halfHeight;
        records[0].u.graph.maxValue = 90;
        switch (params[1]) {
        case 1:
            records[0].u.queue.color = params[0];
            records[0].u.graph.color = params[3];
            records[0].u.graph.startTime = start;
            records[0].u.graph.endTime = end;
            records[0].u.params[17] = halfWidth;
            records[0].u.params[14] = params[2];
            records[0].u.params[15] = params[2];
            records[0].u.params[16] = params[2];
            records[0].u.params[19] = params[2] - 180;
            records[0].u.params[20] = params[2] - 180;
            records[0].u.params[21] = params[2] - 180;
            break;
        case 2:
            records[0].u.queue.color = params[0];
            records[0].u.graph.color = params[3];
            records[0].u.graph.startTime = end;
            records[0].u.graph.endTime = start;
            records[0].u.params[17] = -halfWidth;
            records[0].u.params[14] = params[2];
            records[0].u.params[15] = params[2];
            records[0].u.params[16] = params[2];
            records[0].u.params[19] = params[2] - 180;
            records[0].u.params[20] = params[2] - 180;
            records[0].u.params[21] = params[2] - 180;
            break;
        }
        break;
    }
    case 15: {
        int columns;
        int rows;
        int top;
        int left;
        int right;
        int bottom;
        int halfWidth;
        int halfHeight;
        int angle;

        columns = 9;
        rows = 6;
        top = 0;
        left = 0;
        right = 512;
        bottom = 448;
        halfWidth = (right - left) / columns / 2;
        halfHeight = (bottom - top) / rows / 2;
        angle = params[2];
        records[0].type = 14;
        records[0].u.queue.frame = 0;
        records[0].u.queue.tex0 = 2;
        records[0].u.queue.color = params[0];
        records[0].u.queue.depth = depth;
        records[0].u.queue.count = 1;
        records[0].u.queue.halveRgb = params[4];
        records[0].u.queue.blendKind = 0;
        records[0].u.grid.endColor = params[3];
        records[0].u.grid.screen.top = top;
        records[0].u.grid.screen.left = left;
        records[0].u.grid.screen.right = right;
        records[0].u.grid.screen.bottom = bottom;
        records[0].u.grid.columns = columns;
        records[0].u.grid.rows = rows;
        records[0].u.params[14] = 228;
        records[0].u.params[15] = 187;
        records[0].u.params[16] = 350;
        records[0].u.params[17] = angle;
        records[0].u.params[18] = angle;
        records[0].u.params[19] = angle;
        records[0].u.params[20] = angle - 180;
        records[0].u.params[21] = angle - 180;
        records[0].u.params[22] = angle - 180;
        records[0].u.params[23] = halfWidth;
        records[0].u.params[24] = halfHeight;
        records[0].u.grid.transformStart = 0;
        records[0].u.grid.transformEnd = 90;
        records[0].u.params[27] = params[1];
        break;
    }
    case 16: {
        int phaseSpeed;

        phaseSpeed = (recordUpdatePaused.paused == 0) ? 184 : 180;
        records[0].type = 10;
        records[0].u.queue.frame = 0;
        records[0].u.queue.tex0 = 0;
        records[0].u.queue.color = (params[0] & 0xFFFFFF);
        records[0].u.queue.depth = depth;
        records[0].u.queue.count = 1;
        records[0].u.queue.halveRgb = 0;
        records[0].u.queue.blendKind = 0;
        records[0].u.ring.endColor = params[0];
        records[0].u.ring.centerX = 256;
        records[0].u.ring.centerY = 224;
        records[0].u.ring.radius = params[1];
        records[0].u.ring.innerRadius = 350;
        records[0].u.ring.phaseSpeed = phaseSpeed;
        records[0].u.ring.segments = 35;
        records[0].u.ring.amplitude = params[2];
        records[1].type = 10;
        records[1].u.queue.frame = 0;
        records[1].u.queue.tex0 = 0;
        records[1].u.queue.color = (params[0] & 0xFFFFFF);
        records[1].u.queue.depth = depth;
        records[1].u.queue.count = 1;
        records[1].u.queue.halveRgb = 0;
        records[1].u.queue.blendKind = 0;
        records[1].u.ring.endColor = params[0];
        records[1].u.ring.centerX = 256;
        records[1].u.ring.centerY = 224;
        records[1].u.ring.radius = params[1];
        records[1].u.ring.innerRadius = 350;
        records[1].u.ring.phaseSpeed = phaseSpeed;
        records[1].u.ring.segments = 35;
        records[1].u.ring.amplitude = params[2];
        break;
    }
    case 17:
        records[0].type = 16;
        records[0].u.queue.frame = 0;
        records[0].u.queue.tex0 = 1;
        records[0].u.queue.color = params[0];
        records[0].u.queue.depth = depth;
        records[0].u.queue.count = 1;
        records[0].u.queue.halveRgb = 1;
        records[0].u.queue.blendKind = 0;
        records[0].u.params[7] = 0;
        records[0].u.params[8] = 0;
        records[0].u.params[9] = 512;
        records[0].u.params[10] = 448;
        records[0].u.params[11] = 0;
        records[0].u.params[12] = 0;
        records[0].u.params[13] = 512;
        records[0].u.params[14] = 448;
        records[0].u.params[15] = params[1];
        records[0].u.params[16] = params[2];
        records[0].u.params[17] = params[3];
        records[0].u.params[18] = params[4];
        records[0].u.params[19] = params[5];
        records[0].u.params[20] = 256;
        records[0].u.params[21] = 124;
        break;
    case 18:
        records[0].type = 20;
        records[0].u.queue.frame = 2;
        records[0].u.queue.tex0 = 0;
        records[0].u.queue.color = 0x7FFFFFFF;
        records[0].u.queue.depth = 0x7FFFFFFF;
        records[0].u.queue.count = 1;
        records[0].u.queue.halveRgb = 1;
        records[0].u.queue.blendKind = 0;
        records[0].u.queue.screenRect[0] = 0;
        records[0].u.queue.screenRect[1] = 0;
        records[0].u.queue.screenRect[2] = 512;
        records[0].u.queue.screenRect[3] = 448;
        records[0].u.queue.textureRect[0] = 0;
        records[0].u.queue.textureRect[1] = 0;
        records[0].u.queue.textureRect[2] = 512;
        records[0].u.queue.textureRect[3] = 448;
        records[1].type = 20;
        records[1].u.queue.frame = 2;
        records[1].u.queue.tex0 = 0;
        records[1].u.queue.color = 0;
        records[1].u.queue.depth = depth;
        records[1].u.queue.count = 1;
        records[1].u.queue.halveRgb = 0;
        records[1].u.queue.blendKind = 0;
        records[1].u.queue.screenRect[0] = 0;
        records[1].u.queue.screenRect[1] = 0;
        records[1].u.queue.screenRect[2] = 512;
        records[1].u.queue.screenRect[3] = 448;
        records[2].type = 15;
        records[2].u.queue.frame = 1;
        records[2].u.queue.tex0 = 1;
        records[2].u.queue.color = 0;
        records[3].type = 20;
        records[3].u.queue.frame = 0;
        records[3].u.queue.tex0 = 2;
        records[3].u.queue.color = params[0];
        records[3].u.queue.depth = 0x7FFFFFFF;
        records[3].u.queue.count = 1;
        records[3].u.queue.halveRgb = 1;
        records[3].u.queue.blendKind = 0;
        records[3].u.queue.screenRect[0] = params[1];
        records[3].u.queue.screenRect[1] = params[1];
        records[3].u.queue.screenRect[2] = params[1] + 512;
        records[3].u.queue.screenRect[3] = params[1] + 448;
        records[3].u.queue.textureRect[0] = 0;
        records[3].u.queue.textureRect[1] = 0;
        records[3].u.queue.textureRect[2] = 512;
        records[3].u.queue.textureRect[3] = 448;
        records[4].type = 15;
        records[4].u.queue.frame = 0;
        break;
    case 20:
        records[0].type = 1;
        records[0].u.fade.stage = params[0];
        records[0].u.fade.alpha = params[1];
        records[0].u.fade.depth = depth;
        break;
    case 22:
        records[0].type = 2;
        records[0].u.queue.tex0 = 1;
        records[1].type = 5;
        records[1].u.warp.alpha = params[0];
        records[1].u.warp.z = depth;
        break;
    case 21:
        records[0].type = 6;
        records[0].u.warp.alpha = params[0];
        records[0].u.warp.z = depth;
        break;
    case 19:
        records[0].type = 3;
        records[0].u.tunnel.step = params[0];
        records[0].u.tunnel.depth = depth;
        break;
    case 23:
        records[0].type = 7;
        records[0].u.params[0] = params[0];
        records[0].u.params[1] = depth;
        break;
    case 0:
    case 24:
        break;
    }
    recordUpdateRenderHook();
}

/* Sets record slotIndex of bank bankIndex to type with its ten parameters, then updates the render callback. */
void recordSetSlot(int bankIndex, int slotIndex, int type, int *params)
{
    int i;

    ((GsQueueContext *)(recordBanks + bankIndex * sizeof(RecordBank)))[slotIndex].type = type;
    for (i = 0; i < 10; i++) {
        ((RecordBank *)recordBanks)[bankIndex].records[slotIndex].u.params[i] = params[i];
    }
    recordUpdateRenderHook();
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
void recordClearBanks(void)
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
int recordAnyUsed(void);
/* GIF data sent before (9 quadwords) and after (3 quadwords) the records are drawn. */
extern const unsigned char D_0063D6C8[];
extern const unsigned char D_0063D7B0[];
/* Draw handlers of record types 1..21. */
extern void (*D_0063D758[])(GsQueueContext *record, XglPacket *packet);

/* Calls the draw handler of every record of type 1..21 in the three banks between two fixed GIF blocks, then
 * clears the banks and removes itself as the render callback. */
void recordRenderAll(XglPacket *packet)
{
    RecordBank *bank;
    GsQueueContext *record;
    int bankIndex;
    int slot;
    int type;

    bank = (RecordBank *)recordBanks;
    if (recordAnyUsed() == 0) {
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
    recordClearBanks();
    recordUpdatePaused.paused = 0;
    sRender.callback = 0;
}

/* True if any of the 30 records (3 banks of 10) has a nonzero first byte. */
int recordAnyUsed(void)
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
  recordUpdatePaused.paused = 1;
}

/* The EE scratchpad, where the packet is built before it is sent. */
#define SCRATCHPAD_BASE 0x70000000

/* One A+D qword of a GS packet: the register value, then the register address. A union as tu229.c's TalkGsPair:
   with a plain struct gcc moves recordDrawType3's point reads above the packet stores. */
typedef union GsAdPair {
    struct {
        u64 data;
        u64 reg;
    } ad;
} GsAdPair;

XglPacket *xglPacketGetCurrent(void);
u64 gsMakeTex0(int buffer, int flag);
u64 gsMakeFrame(int buffer, int flag);
int gsMakeAlpha(int blend);
void recordStepAngle(int stage);
int recordStageSize(int mode, int fallback);
int recordStageColor(int mode, int alpha);
int recordPulseColor(void);

/* Draw handler of record type 1 (D_0063D758[0]): one textured sprite over the 512x448 screen, inset by the
   stage's size, in the stage's color; at stage 2 the sprite is drawn a second time in the pulsing color. */
void recordDrawType1(GsQueueContext *record)
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
    color = recordStageColor(stage, alpha);
    size = recordStageSize(stage, alpha);
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
    pairs[pairCount].ad.data = gsMakeAlpha(0);
    pairCount++;
    pairs[pairCount].ad.reg = GS_TEX0_1;
    pairs[pairCount].ad.data = gsMakeTex0(1, 0);
    pairCount++;
    pairs[pairCount].ad.reg = GS_FRAME_1;
    pairs[pairCount].ad.data = gsMakeFrame(0, 0);
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
        color = recordPulseColor();
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
int recordStageSize(int mode, int fallback)
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
   is) at stage 0, at stage 2 the alpha 80 + 5 * sin(recordAngle degrees) put into 0..127 by wrapIntRange; 0 for any
   other stage. */
int recordStageColor(int mode, int alpha)
{
    switch (mode) {
    case 1:
        return (alpha << 24) | 0x808080;
    case 0:
        return 0x40808080;
    case 2:
        alpha = wrapIntRange((int) (sinf(recordAngle * 3.141592 / 180.0) * 5.0f + 80.0f), 0, 0x7F);
        return (alpha << 24) | 0x808080;
    default:
        return 0;
    }
}

float cosf(float angle);

/* Pulsing color: grey 0x808080 under the alpha 80 + 5 * cos(recordAngle degrees), put into 0..127 by
   wrapIntRange. */
int recordPulseColor(void)
{
    int alpha;

    alpha = wrapIntRange((int) (cosf(recordAngle * 3.141592 / 180.0) * 5.0f + 80.0f), 0, 0x7F);
    return (alpha << 24) | 0x808080;
}

/* At stage 2, while updates are not suspended, steps the angle recordAngle by 92 and wraps it into 0..360. */
void recordStepAngle(int stage)
{
    /* Block-scope declaration, a codegen device: with only the definition above in scope, fd.sh recordStepAngle
       gives bne $4,$2 where the original has bnel at 0x275EB4 (17/18 in place); with it, 18/18. */
    int wrapIntRange(int value, int lower, int upper);

    if (recordUpdatePaused.paused == 0 && stage == 2) {
        recordAngle += 0x5C;
        recordAngle = wrapIntRange(recordAngle, 0, 0x168);
    }
}

void func_0027A6F0(GsQueueContext *context, int packet);

/* Draw handler of record type 2 (D_0063D758[1]): draws the record's texture (its tex0 word) over the whole 512x448
   screen through a type 20 sprite record. */
void recordDrawType2(GsQueueContext *record, int packet)
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
    sprite.u.queue.screenRect[0] = 0;
    sprite.u.queue.screenRect[1] = 0;
    sprite.u.queue.screenRect[2] = 0x200;
    sprite.u.queue.screenRect[3] = 0x1C0;
    sprite.u.queue.textureRect[0] = 0;
    sprite.u.queue.textureRect[1] = 0;
    sprite.u.queue.textureRect[2] = 0x200;
    sprite.u.queue.textureRect[3] = 0x1C0;
    func_0027A6F0(&sprite, packet);
}

void recordAdvanceRingRadius(int step);
void recordRingPoint(float *point, float *clampedPoint, float angle, float radius);
/* libgcc __fixunssfdi, float to unsigned 64-bit integer (negative values give 0), in the libgcc block between
   __moddi3 and fptodp. */
u64 func_002A6A90(float value);
extern float D_0069DDD8;
extern int D_0069DDDC;

/* Draw handler of record type 3 (D_0063D758[2]): a triangle fan from the screen centre through octagonal rings of
   radius D_0069DDDC, D_0069DDDC + 25, ... below 620 (recordAdvanceRingRadius advances D_0069DDDC by the record's
   step); each ring vertex takes its texture coordinates from the point clamped to radius 224, at the record's depth. */
void recordDrawType3(GsQueueContext *record)
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
    recordAdvanceRingRadius(record->u.tunnel.step);
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
    pairs[pairCount].ad.data = gsMakeAlpha(0);
    pairCount++;
    pairs[pairCount].ad.reg = GS_TEX0_1;
    pairs[pairCount].ad.data = gsMakeTex0(1, 0);
    pairCount++;
    pairs[pairCount].ad.reg = GS_FRAME_1;
    pairs[pairCount].ad.data = gsMakeFrame(0, 0);
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
        recordRingPoint(point, clampedPoint, 0.0f, radius);
        pairs[pairCount].ad.reg = GS_UV;
        pairs[pairCount].ad.data = GS_SET_UV(func_002A6A90(clampedPoint[0] * 16.0f),
                                             func_002A6A90(clampedPoint[1] * 16.0f));
        pairCount++;
        pairs[pairCount].ad.reg = GS_XYZ3;
        pairs[pairCount].ad.data = GS_SET_XYZ((int) ((point[0] + 1792.0f) * 16.0f),
                                              (int) ((point[1] + 1824.0f) * 16.0f), z);
        pairCount++;
        for (side = 1; side <= 8; side++) {
            recordRingPoint(point, clampedPoint, side * 45 * D_0069DDD8, radius);
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
void recordRingPoint(float *point, float *clampedPoint, float angle, float radius)
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

/* While updates are not suspended, advances D_0069DDDC by step and steps it back below 406 in steps of 25. */
void recordAdvanceRingRadius(int step)
{
    if (recordUpdatePaused.paused == 0) {
        D_0069DDDC += step;
        while (D_0069DDDC >= 0x196) {
            D_0069DDDC -= 0x19;
        }
    }
}

/* Row length of the cell grid: 50 x 50 flags (0x9C4 bytes, cleared by recordGridReset), of which recordGridWidth
 * x recordGridHeight are in use; recordDrawType4 indexes a cell as column + row * 50. */
#define RECORD_GRID_SIDE 50

extern int recordGridWidth;
extern int recordGridHeight;
/* signed char: recordDrawType4 reads the flags with lb (fd.sh with unsigned char: lbu $2,0($4) at its first flag
 * read, 221/225). */
extern signed char recordGridCells[];

void gsQueueBeginSprites(void *context);
void gsQueueAddTexturedVertex(int packet, Vector4 *position, Vector4 *uv, void *context);
void gsQueueAddTexturedKickVertex(int packet, Vector4 *position, Vector4 *uv, void *context);
void gsQueueFlush(int packet);

/*
 * Draw handler of record type 4 (D_0063D758[3]): sets random free cells of the grid until cellCount (clamped to
 * 0..width * height) are set, or clears random set ones down to it, then draws each set cell as one sprite of
 * 512 / width x 448 / height pixels at the record's depth, count times. Reading count before depth gives the
 * original's depth reload register (fd.sh: depth first 216/225, count first 225/225); each random loop declares
 * its own cell, randomColumn and randomRow (at function scope fd.sh gives 33/225).
 */
void recordDrawType4(GsQueueContext *record, int packet)
{
    Vector4 position;
    Vector4 kickPosition;
    int pass;
    int depth;
    int cellHeight;
    int cellWidth;
    int cellTotal;
    int cellCount;
    int count;
    int filled;
    int row;
    int column;
    int toSet;
    int toClear;

    cellTotal = recordGridHeight * recordGridWidth;
    cellCount = record->u.gridFill.cellCount;
    count = record->u.queue.count;
    depth = record->u.queue.depth;
    if (cellCount < 0) {
        cellCount = 0;
    }
    if (cellCount > cellTotal) {
        cellCount = cellTotal;
    }
    filled = 0;
    for (row = 0; row < recordGridHeight; row++) {
        for (column = 0; column < recordGridWidth; column++) {
            if (recordGridCells[column + row * RECORD_GRID_SIDE] == 1) {
                filled++;
            }
        }
    }
    toSet = cellCount - filled;
    toClear = filled - cellCount;
    for (column = 0; column < toSet; column++) {
        signed char *cell;
        int randomColumn;
        int randomRow;

        do {
            randomColumn = xglSRand() % recordGridWidth;
            randomRow = xglSRand() % recordGridHeight;
            cell = &recordGridCells[randomColumn + randomRow * RECORD_GRID_SIDE];
        } while (*cell == 1);
        *cell = 1;
    }
    for (column = 0; column < toClear; column++) {
        signed char *cell;
        int randomColumn;
        int randomRow;

        do {
            randomColumn = xglSRand() % recordGridWidth;
            randomRow = xglSRand() % recordGridHeight;
            cell = &recordGridCells[randomColumn + randomRow * RECORD_GRID_SIDE];
        } while (*cell == 0);
        *cell = 0;
    }
    cellWidth = 512 / recordGridWidth;
    cellHeight = 448 / recordGridHeight;
    gsQueueBeginSprites(record);
    for (pass = 0; pass < count; pass++) {
        for (row = 0; row < recordGridHeight; row++) {
            for (column = 0; column < recordGridWidth; column++) {
                if (recordGridCells[column + row * RECORD_GRID_SIDE] != 0) {
                    position.x = cellWidth * column;
                    position.y = cellHeight * row;
                    position.z = depth;
                    kickPosition.x = cellWidth * (column + 1);
                    kickPosition.y = cellHeight * (row + 1);
                    kickPosition.z = depth;
                    gsQueueAddTexturedVertex(packet, &position, &position, record);
                    gsQueueAddTexturedKickVertex(packet, &kickPosition, &kickPosition, record);
                }
            }
        }
    }
    gsQueueFlush(packet);
}

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

void recordStepWarp(void);
void recordWarpCorner(Vector4 *result, int x, int y);

/* XYZ of a point in screen pixels: + (2048 - 256, 2048 - 224) puts it around the GS origin, in 12.4 fixed point. */
#define RECORD_XYZ(x, y, z) GS_SET_XYZ((int) (((x) + 1792.0f) * 16.0f), (int) (((y) + 1824.0f) * 16.0f), z)

/*
 * Draw handler of record type 5 (D_0063D758[4]): draws the screen texture as a triangle fan around the screen
 * centre (256, 224). The four edge midpoints move out by an eighth of the record's alpha word (spread), each corner
 * by spread along one axis and half of it along the other, a quarter turn further at each corner. The fan goes
 * around twice and closes at its first point. Z and fog take the record's z word.
 */
void recordDrawType5(GsQueueContext *record)
{
    GsAdPair *pairs;
    XglPacket *packet;
    int spread;
    int z;
    int eighth;
    int pairCount;
    int lap;

    spread = record->u.warp.alpha;
    z = record->u.warp.z;
    packet = xglPacketGetCurrent();
    eighth = spread >> 3;
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
    pairs[pairCount].ad.data = gsMakeAlpha(0);
    pairCount++;
    pairs[pairCount].ad.reg = GS_TEX0_1;
    pairs[pairCount].ad.data = gsMakeTex0(2, 0);
    pairCount++;
    pairs[pairCount].ad.reg = GS_FRAME_1;
    pairs[pairCount].ad.data = gsMakeFrame(0, 0);
    pairCount++;
    pairs[pairCount].ad.data = 0x15D;
    pairs[pairCount].ad.reg = GS_PRIM;
    pairCount++;
    pairs[pairCount].ad.reg = GS_RGBAQ;
    pairs[pairCount].ad.data = 0x40808080;
    pairCount++;
    pairs[pairCount].ad.reg = GS_UV;
    pairs[pairCount].ad.data = GS_SET_UV(256 * 16, 224 * 16);
    pairCount++;
    pairs[pairCount].ad.reg = GS_XYZF2;
    pairs[pairCount].ad.data = RECORD_XYZ(256, 224, z);
    pairCount++;
    for (lap = 0; lap < 2; lap++) {
        /* Declared in the loop: at function scope gcc leaves the 512 + half chain in the loop (fd.sh 16/331). */
        int half = spread >> 1;

        pairs[pairCount].ad.reg = GS_UV;
        pairs[pairCount].ad.data = GS_SET_UV(512 * 16, 224 * 16);
        pairCount++;
        pairs[pairCount].ad.reg = GS_XYZF2;
        pairs[pairCount].ad.data = RECORD_XYZ(512 + eighth, 224, z);
        pairCount++;
        pairs[pairCount].ad.reg = GS_UV;
        pairs[pairCount].ad.data = GS_SET_UV(512 * 16, 448 * 16);
        pairCount++;
        pairs[pairCount].ad.reg = GS_XYZF2;
        pairs[pairCount].ad.data = RECORD_XYZ(512 + spread, 448 + half, z);
        pairCount++;
        pairs[pairCount].ad.reg = GS_UV;
        pairs[pairCount].ad.data = GS_SET_UV(256 * 16, 448 * 16);
        pairCount++;
        pairs[pairCount].ad.reg = GS_XYZF2;
        pairs[pairCount].ad.data = RECORD_XYZ(256, 448 + eighth, z);
        pairCount++;
        pairs[pairCount].ad.reg = GS_UV;
        pairs[pairCount].ad.data = GS_SET_UV(0, 448 * 16);
        pairCount++;
        pairs[pairCount].ad.reg = GS_XYZF2;
        pairs[pairCount].ad.data = RECORD_XYZ(-half, 448 + spread, z);
        pairCount++;
        pairs[pairCount].ad.reg = GS_UV;
        pairs[pairCount].ad.data = GS_SET_UV(0, 224 * 16);
        pairCount++;
        pairs[pairCount].ad.reg = GS_XYZF2;
        pairs[pairCount].ad.data = RECORD_XYZ(-eighth, 224, z);
        pairCount++;
        pairs[pairCount].ad.reg = GS_UV;
        pairs[pairCount].ad.data = GS_SET_UV(0, 0);
        pairCount++;
        pairs[pairCount].ad.reg = GS_XYZF2;
        pairs[pairCount].ad.data = RECORD_XYZ(-spread, -half, z);
        pairCount++;
        pairs[pairCount].ad.reg = GS_UV;
        pairs[pairCount].ad.data = GS_SET_UV(256 * 16, 0);
        pairCount++;
        pairs[pairCount].ad.reg = GS_XYZF2;
        pairs[pairCount].ad.data = RECORD_XYZ(256, -eighth, z);
        pairCount++;
        pairs[pairCount].ad.reg = GS_UV;
        pairs[pairCount].ad.data = GS_SET_UV(512 * 16, 0);
        pairCount++;
        pairs[pairCount].ad.reg = GS_XYZF2;
        pairs[pairCount].ad.data = RECORD_XYZ(512 + half, -spread, z);
        pairCount++;
    }
    pairs[pairCount].ad.reg = GS_UV;
    pairs[pairCount].ad.data = GS_SET_UV(512 * 16, 224 * 16);
    pairCount++;
    pairs[pairCount].ad.reg = GS_XYZF2;
    pairs[pairCount].ad.data = RECORD_XYZ(512 + eighth, 224, z);
    pairCount++;
    pairs[0].ad.data += pairCount - 1;
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    sceVif1PkAddDirectDataN(packet, pairs, pairCount);
    sceVif1PkCloseDirectHLCode(packet);
}

/*
 * Draw handler of record type 6 (D_0063D758[5]): draws the 512 x 448 screen texture as one triangle fan of its four
 * corners, each corner moved by recordWarpCorner. XYZF2 takes the record's z as the whole upper word (Z and fog).
 */
void recordDrawType6(GsQueueContext *record)
{
    Vector4 point;
    GsAdPair *pairs;
    XglPacket *packet;
    int alpha;
    int z;
    int pairCount;

    alpha = record->u.warp.alpha;
    z = record->u.warp.z;
    recordStepWarp();
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
    pairs[pairCount].ad.data = gsMakeAlpha(0);
    pairCount++;
    pairs[pairCount].ad.reg = GS_TEX0_1;
    pairs[pairCount].ad.data = gsMakeTex0(1, 0);
    pairCount++;
    pairs[pairCount].ad.reg = GS_FRAME_1;
    pairs[pairCount].ad.data = gsMakeFrame(0, 0);
    pairCount++;
    pairs[pairCount].ad.data = 0x15D;
    pairs[pairCount].ad.reg = GS_PRIM;
    pairCount++;
    /* Grey 0x808080 with the record's alpha. */
    pairs[pairCount].ad.reg = GS_RGBAQ;
    pairs[pairCount].ad.data = (unsigned int)(alpha << 24 | 0x808080);
    pairCount++;
    recordWarpCorner(&point, 0, 0);
    pairs[pairCount].ad.reg = GS_UV;
    pairs[pairCount].ad.data = GS_SET_UV(0, 0);
    pairCount++;
    pairs[pairCount].ad.reg = GS_XYZF2;
    pairs[pairCount].ad.data = GS_SET_XYZ((int)((point.x + 1792.0f) * 16.0f), (int)((point.y + 1824.0f) * 16.0f), z);
    pairCount++;
    recordWarpCorner(&point, 512, 0);
    pairs[pairCount].ad.reg = GS_UV;
    pairs[pairCount].ad.data = GS_SET_UV(512 * 16, 0);
    pairCount++;
    pairs[pairCount].ad.reg = GS_XYZF2;
    pairs[pairCount].ad.data = GS_SET_XYZ((int)((point.x + 1792.0f) * 16.0f), (int)((point.y + 1824.0f) * 16.0f), z);
    pairCount++;
    recordWarpCorner(&point, 512, 448);
    pairs[pairCount].ad.reg = GS_UV;
    pairs[pairCount].ad.data = GS_SET_UV(512 * 16, 448 * 16);
    pairCount++;
    pairs[pairCount].ad.reg = GS_XYZF2;
    pairs[pairCount].ad.data = GS_SET_XYZ((int)((point.x + 1792.0f) * 16.0f), (int)((point.y + 1824.0f) * 16.0f), z);
    pairCount++;
    recordWarpCorner(&point, 0, 448);
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
void recordStepWarp(void)
{
    float velocity;
    float next;

    if (recordUpdatePaused.paused != 0) {
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
extern AlignedVector4 D_0068D6C0;
extern AlignedVector4 D_0068D6D0;
extern float D_0069DDF0;

/* Rotates the point (x, y) about D_0068D6C0 by D_0069DDE8 * D_0069DDF0, scales it by D_0068D6D0 and stores the
 * resulting x and y in result. */
void recordWarpCorner(Vector4 *result, int x, int y)
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
void recordRingStep(float angle, Vector4 *screenPoints, Vector4 *texturePoints);
extern float D_0069DDF4;

/*
 * Draw handler of record type 7 (D_0063D758[6]): draws a textured ring as one triangle strip: for every step an
 * inner vertex (func_002775F0's colour) and an outer one (recordDefaultColor), Z and fog 0x7FFFFFFF.
 */
void recordDrawType7(void)
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
    pairs[pairCount].ad.data = gsMakeAlpha(0);
    pairCount++;
    pairs[pairCount].ad.reg = GS_TEX0_1;
    pairs[pairCount].ad.data = gsMakeTex0(1, 0);
    pairCount++;
    pairs[pairCount].ad.reg = GS_FRAME_1;
    pairs[pairCount].ad.data = gsMakeFrame(0, 0);
    pairCount++;
    pairs[pairCount].ad.data = 0x15C;
    pairs[pairCount].ad.reg = GS_PRIM;
    pairCount++;
    degrees = 0;
    for (step = 0; step < RING_STEPS; step++) {
        recordRingStep((float)degrees * D_0069DDF4, &screenPoints, &texturePoints);
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
void recordRingStep(float angle, Vector4 *screenPoints, Vector4 *texturePoints)
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
