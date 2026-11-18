#ifndef INCLUDE_MAIN_TU207_H
#define INCLUDE_MAIN_TU207_H

#include "main/sdv_033.h"

/* The draw record of src/main/tu207.c and the functions other files call. */

/* One rectangle of a draw record: top, left, right, bottom. */
typedef struct GsQueueRect {
    int top;
    int left;
    int right;
    int bottom;
} GsQueueRect;

/*
 * One 0x7C-byte draw record (recordBanks in tu207.c holds three banks of ten). The type byte picks the draw handler
 * D_0063D758[type - 1], and that handler decides what the words after it hold: the type 1 handler reads +0x04 as a
 * stage, the type 2, 4, 8-14 and 20 handlers as the gsQueue frame (types 4 and 8-14 pass the record to
 * gsQueueBeginTriStrip or gsQueueBeginSprites), the type 3 handler as a ring step, the type 6 handler as an alpha.
 * Only these per-type arms are a union; color (+0x0C) and halveRgb (+0x18) are tu213.c's names.
 */
typedef struct GsQueueContext {
    unsigned char type; /* +0x00: 0 unused, 1..21 picks the draw handler D_0063D758[type - 1] */
    unsigned char unmodeled_01[3];
    union {
        /* Any type: recordSetSlot copies the first ten words without looking at the type. */
        int params[30]; /* +0x04..+0x7B */
        /* Types 2 and 20 (recordDrawType2 fills one for spriteBatchDraw): gsQueueBeginTriStrip passes frame to
           gsQueueSetFrame, tex0 to gsQueueSetTex0 and blendKind with color to gsQueueSetBlendColor; count is the
           number of passes, screenRect and textureRect the sprite. */
        struct {
            int frame;          /* +0x04 */
            int tex0;           /* +0x08 */
            int color;          /* +0x0C */
            int depth;          /* +0x10: z of every vertex */
            int count;          /* +0x14: how many times the primitive is drawn */
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
        /* Type 8 (recordDrawStrip): two colours over a rectangle, along y when vertical. */
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
        /* Type 9 (recordDrawTurnColumns): columns of rows quads between bottom and top, turned about the pivot by an angle
           swept across x, from startAngle at startX to endAngle at endX (the x, y and z angles in degrees). A
           column whose angle is below 0 is drawn flat, one past maxAngle is not drawn; the turned rows fade from
           white towards color by angle / maxAngle. */
        struct {
            unsigned char queueWords[0x20 - 0x04]; /* +0x04: frame..blendKind, see queue */
            int color;                             /* +0x20 */
            int bottom;                            /* +0x24 */
            int startX;                            /* +0x28 */
            int endX;                              /* +0x2C */
            int top;                               /* +0x30 */
            int columns;                           /* +0x34 */
            int rows;                              /* +0x38 */
            int endAngle[3];                       /* +0x3C */
            int pivotX;                            /* +0x48: from a column's bottom left corner */
            int pivotY;                            /* +0x4C */
            int startAngle[3];                     /* +0x50 */
            int maxAngle;                          /* +0x5C */
        } turn;
        /* Type 10 (orbitDrawRing): a ring whose outer edge waves by amplitude around radius. */
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
        /* Type 11 (quadDrawPreset): a position preset and a texture rectangle. */
        struct {
            unsigned char queueWords[0x20 - 0x04]; /* +0x04: frame..blendKind, see queue */
            int positionPreset;                    /* +0x20 */
            unsigned char unmodeled_24[0x30 - 0x24];
            GsQueueRect texture;                   /* +0x30 */
        } presetQuad;
        /* Type 12 (quadDrawPresetUv): a screen rectangle and a texture preset. */
        struct {
            unsigned char queueWords[0x20 - 0x04]; /* +0x04: frame..blendKind, see queue */
            GsQueueRect screen;                    /* +0x20 */
            int texturePreset;                     /* +0x30 */
        } presetUvQuad;
        /* Type 13 (circleDrawSegments, circleSpriteCorners): rings of sprites around the screen rectangle. */
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
        /* Type 14 (gridDrawCells): the screen rectangle as rows x columns cells, each turned about the pivot by an
           angle that pointFadeColorStep (tu211.c) lerps from startAngle at the centre to endAngle at radius by the
           squared distance (reversed when invert is set). A cell whose angle is below minAngle is drawn flat, one
           past maxAngle is not drawn. */
        struct {
            unsigned char queueWords[0x20 - 0x04]; /* +0x04: frame..blendKind, see queue */
            int endColor;                          /* +0x20 */
            GsQueueRect screen;                    /* +0x24 */
            int columns;                           /* +0x34 */
            int rows;                              /* +0x38 */
            int centerX;                           /* +0x3C */
            int centerY;                           /* +0x40 */
            int radius;                            /* +0x44 */
            int startAngle[3];                     /* +0x48 */
            int endAngle[3];                       /* +0x54 */
            int pivotX;                            /* +0x60: from a cell's top left corner */
            int pivotY;                            /* +0x64 */
            int minAngle;                          /* +0x68 */
            int maxAngle;                          /* +0x6C */
            int invert;                            /* +0x70 */
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
        /* Types 5 and 6 (recordDrawType5, recordDrawType6): the alpha of the RGBAQ word (type 5: the spread of the
           fan) and the Z and fog word. */
        struct {
            int alpha; /* +0x04 */
            int z;     /* +0x08 */
        } warp;
    } u;
} GsQueueContext;

void recordPauseUpdates(void);

#endif /* INCLUDE_MAIN_TU207_H */
