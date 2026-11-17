#ifndef INCLUDE_MAIN_TU213_H
#define INCLUDE_MAIN_TU213_H

#include "main/sdv_033.h"
#include "shared.h"

/* Functions of src/main/tu213.c that other files call. */

/* The effect record handed to the gsQueue batch handlers. The words up to +0x1F are the same for
   every effect; from +0x20 on the layout depends on the effect kind, one union member per kind
   (partial layouts: only the words the effect code reads). */
typedef struct GsQueueContext {
    unsigned char unmodeled_00[4];
    int frame;                             /* +0x04: frame buffer value for gsQueueSetFrame */
    int tex0;                              /* +0x08: TEX0 value for gsQueueSetTex0 */
    int color;                             /* +0x0C: RGBA color */
    int z;                                 /* +0x10: depth shared by all vertices */
    int count;                             /* +0x14: how many times the effect is queued */
    int textured;                          /* +0x18: nonzero queues UV and TEX0 and halves R, G and B */
    int blend_kind;                        /* +0x1C: kind for gsQueueSetBlendColor */
    union {
        /* Sprite / quad: screen and texture rectangle. */
        struct {
            int top;                       /* +0x20 */
            int left;                      /* +0x24 */
            int right;                     /* +0x28 */
            int bottom;                    /* +0x2C */
            int uv_top;                    /* +0x30 */
            int uv_left;                   /* +0x34 */
            int uv_right;                  /* +0x38 */
            int uv_bottom;                 /* +0x3C */
            int margin;                    /* +0x40: the quad is grown by this on every side */
            int wobble_x;                  /* +0x44: amplitude of the cosine offset */
            int wobble_y;                  /* +0x48: amplitude of the sine offset */
            int angle;                     /* +0x4C: rotation around the pivot (degrees) */
            int phase_step;                /* +0x50: added to the global phase every call */
            int pivot_x;                   /* +0x54 */
            int pivot_y;                   /* +0x58 */
        } quad;
        /* Randomly displaced grid. */
        struct {
            int top;                       /* +0x20 */
            int left;                      /* +0x24 */
            int right;                     /* +0x28 */
            int bottom;                    /* +0x2C */
            unsigned char unmodeled_30[0x10];
            int divisions;                 /* +0x40: grid cells per side */
        } scatter;
        /* Transformed quad: screen offset. */
        struct {
            unsigned char unmodeled_20[0x40];
            int offset_x;                  /* +0x60 */
            int offset_y;                  /* +0x64 */
        } transform;
        /* Colour ramp around a centre. */
        struct {
            unsigned char unmodeled_20[0x1C];
            int center_x;                  /* +0x3C */
            int center_y;                  /* +0x40 */
            int radius;                    /* +0x44 */
            int from_a;                    /* +0x48 */
            int from_b;                    /* +0x4C */
            int from_c;                    /* +0x50 */
            int to_a;                      /* +0x54 */
            int to_b;                      /* +0x58 */
            int to_c;                      /* +0x5C */
            int unmodeled_60[2];
            int limit_a;                   /* +0x68 */
            int limit_b;                   /* +0x6C */
            int invert;                    /* +0x70 */
        } arc;
        /* Random point displacement. */
        struct {
            unsigned char unmodeled_20[0x10];
            int count_x;                   /* +0x30: horizontal displacement range, none when <= 0 */
            int offset_x;                  /* +0x34: added to the horizontal displacement */
            int count_y;                   /* +0x38: vertical displacement range, none when <= 0 */
            int offset_y;                  /* +0x3C: added to the vertical displacement */
        } point;
        /* Ring-wave grid. */
        struct {
            int color_b;                   /* +0x20: second fade color, 0 for none */
            int top;                       /* +0x24 */
            int left;                      /* +0x28 */
            int right;                     /* +0x2C */
            int bottom;                    /* +0x30 */
            int center_x;                  /* +0x34 */
            int center_y;                  /* +0x38 */
            int inner_radius;              /* +0x3C */
            int outer_radius;              /* +0x40 */
            int phase_scale;               /* +0x44 */
            int amplitude;                 /* +0x48 */
            int angle_step;                /* +0x4C: added to the global wave angle per call */
            int divisions;                 /* +0x50: grid cells per side */
        } ring;
        /* Orbiting grid. */
        struct {
            int color_b;                   /* +0x20: second fade color, 0 for none */
            int top;                       /* +0x24 */
            int left;                      /* +0x28 */
            int right;                     /* +0x2C */
            int bottom;                    /* +0x30 */
            int radius_x;                  /* +0x34 */
            unsigned int speed_y;          /* +0x38 */
            int phase_step_a;              /* +0x3C: added to the global phase per call */
            int radius_y;                  /* +0x40 */
            unsigned int speed_x;          /* +0x44 */
            int phase_step_b;              /* +0x48: added to the global phase per call */
            int divisions;                 /* +0x4C: grid cells per side */
        } grid;
        /* Fade lengths read by gsQueueFadeToColor / gsQueueFadeToColorPhases. */
        struct {
            unsigned char unmodeled_20[0x14];
            int phase1_frames;             /* +0x34: first phase length; with phase2_frames the divisor of the fade */
            unsigned char unmodeled_38[8];
            int phase2_frames;             /* +0x40 */
            unsigned char unmodeled_44[4];
            int fade_frames;               /* +0x48: fade length, the divisor of the fade */
        } fade;
    } kind;
} GsQueueContext;

void gsQueueSetAlphaFlags(int texAlphaExpand, int alphaTest, int unusedArg);
void gsQueueAddColor(XglPacket *packet, int color, GsQueueContext *context);
void gsQueueAddContextColor(XglPacket *packet, GsQueueContext *context);

/* Batch begin handlers: start a GIF batch of the given primitive type. */
void gsQueueBeginTriStrip(GsQueueContext *context);
void gsQueueBeginSprites(GsQueueContext *context);
/* Restart re-runs the stored begin handler; flush sends the queued qwords. */
void gsQueueRestart(GsQueueContext *context);
void gsQueueFlush(XglPacket *packet);
/* Vertex queueing; the Kick forms also flush and restart a full queue. The
   non-kick forms ignore their first argument and the trailing
   batch/context argument, which the original callers pass but never read. */
void gsQueueAddKickVertex(XglPacket *packet, float *pos, GsQueueContext *context);
void gsQueueAddVertex(XglPacket *unused, float *pos, GsQueueContext *batch);
void gsQueueAddTexturedKickVertex(XglPacket *packet, float *pos, float *uv, GsQueueContext *context);
void gsQueueAddTexturedVertex(XglPacket *unused, float *pos, float *uv, GsQueueContext *context);
void gsQueueAddTriangle(XglPacket *packet, DoublewordVector4 *pos, Vector4 *uv, GsQueueContext *context);

#endif /* INCLUDE_MAIN_TU213_H */
