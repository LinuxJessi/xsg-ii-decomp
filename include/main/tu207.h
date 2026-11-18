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
 * One 0x7C-byte draw record (recordBanks in tu207.c holds three banks of ten), the context the gsQueue batch handlers
 * take (tu213.h's GsQueueContext describes the same record). The type byte picks the draw handler
 * recordDrawHandlers[type - 1], and that handler decides what the words after it hold: the handlers of types 4 and 8-20 read the gsQueue words
 * of `queue` (+0x04..+0x1F) and pass the record to a gsQueueBegin* handler, then read their own arm from +0x20 on.
 * The type 1 handler reads +0x04 as a stage, the type 3 handler as a ring step, the type 5 handler as a spread, the
 * type 6 handler as an alpha. The type 2 handler reads only tex0 (+0x08) and draws it through a type 20 record of its
 * own (frame 2). The per-type arms are partial: only the words the handlers read.
 */
typedef struct GsQueueContext {
    unsigned char type; /* +0x00: 0 unused, 1..21 picks the draw handler recordDrawHandlers[type - 1] */
    unsigned char unmodeled_01[3];
    union {
        /* Any type: recordSetSlot copies the first ten words without looking at the type. */
        int params[30]; /* +0x04..+0x7B */
        /* The gsQueue words every gsQueueBegin* handler reads (tu213.h's names); types 2 and 20 (spriteBatchDraw) add
           a screen and a texture rectangle. */
        struct {
            int frame;           /* +0x04: frame buffer value for gsQueueSetFrame */
            int tex0;            /* +0x08: TEX0 value for gsQueueSetTex0 */
            int color;           /* +0x0C: RGBA color */
            int z;               /* +0x10: depth shared by all vertices */
            int count;           /* +0x14: how many times the effect is queued */
            int textured;        /* +0x18: nonzero queues UV and TEX0 and halves R, G and B */
            int blend_kind;      /* +0x1C: kind for gsQueueSetBlendColor */
            GsQueueRect screen;  /* +0x20 */
            GsQueueRect texture; /* +0x30 */
        } queue;
        /* Type 4 (recordDrawType4): the number of grid cells to set. */
        struct {
            unsigned char queueWords[0x20 - 0x04]; /* +0x04: frame..blend_kind, see queue */
            int cellCount;                         /* +0x20 */
        } gridFill;
        /* Type 8 (recordDrawStrip): two colours over a rectangle, along y when vertical. */
        struct {
            unsigned char queueWords[0x20 - 0x04]; /* +0x04: frame..blend_kind, see queue */
            int color0;                            /* +0x20 */
            int color1;                            /* +0x24 */
            int top;                               /* +0x28 */
            int left;                              /* +0x2C */
            int right;                             /* +0x30 */
            int bottom;                            /* +0x34 */
            int vertical;                          /* +0x38 */
        } gradient;
        /* Type 9 (recordDrawTurnColumns): columns of rows quads between bottom and top, turned about the pivot by an
           angle swept across x, from startAngle at startX to endAngle at endX (the x, y and z angles in degrees). A
           column whose angle is below 0 is drawn flat, one past maxAngle is not drawn; the turned rows fade from
           white towards color by angle / maxAngle. */
        struct {
            unsigned char queueWords[0x20 - 0x04]; /* +0x04: frame..blend_kind, see queue */
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
            unsigned char queueWords[0x20 - 0x04]; /* +0x04: frame..blend_kind, see queue */
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
            unsigned char queueWords[0x20 - 0x04]; /* +0x04: frame..blend_kind, see queue */
            int positionPreset;                    /* +0x20 */
            unsigned char unmodeled_24[0x30 - 0x24];
            GsQueueRect texture;                   /* +0x30 */
        } presetQuad;
        /* Type 12 (quadDrawPresetUv): a screen rectangle and a texture preset. */
        struct {
            unsigned char queueWords[0x20 - 0x04]; /* +0x04: frame..blend_kind, see queue */
            GsQueueRect screen;                    /* +0x20 */
            int texturePreset;                     /* +0x30 */
        } presetUvQuad;
        /* Type 13 (circleDrawSegments, circleSpriteCorners): rings of sprites around the screen rectangle. */
        struct {
            unsigned char queueWords[0x20 - 0x04]; /* +0x04: frame..blend_kind, see queue */
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
           past maxAngle is not drawn; quadDrawTransformed turns a cell about its first corner plus the pivot. */
        struct {
            unsigned char queueWords[0x20 - 0x04]; /* +0x04: frame..blend_kind, see queue */
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
        /* Type 5 (recordDrawType5): how far the fan's edge points move out, and the Z and fog word. */
        struct {
            int spread; /* +0x04 */
            int z;      /* +0x08 */
        } fan;
        /* Type 6 (recordDrawType6): the alpha of the RGBAQ word and the Z and fog word. */
        struct {
            int alpha; /* +0x04 */
            int z;     /* +0x08 */
        } warp;
    } u;
} GsQueueContext;

void recordPauseUpdates(void);

#endif /* INCLUDE_MAIN_TU207_H */
