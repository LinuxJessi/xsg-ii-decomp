#ifndef INCLUDE_MAIN_TU213_H
#define INCLUDE_MAIN_TU213_H

#include "shared.h"

/* Functions of src/main/tu213.c that other files call. */

/* Partial layout of the queue context: only the words the callers read. */
typedef struct GsQueueContext {
    unsigned char unmodeled_00[4];
    int frame;                             /* +0x04: frame buffer value for gsQueueSetFrame */
    int tex0;                              /* +0x08: TEX0 value for gsQueueSetTex0 */
    int color;                             /* +0x0C: RGBA color */
    unsigned char unmodeled_10[8];
    int halveRgb;                          /* +0x18: nonzero halves R, G and B */
    int blendKind;                         /* +0x1C: kind for gsQueueSetBlendColor */
    unsigned char unmodeled_20[0x14];
    int phase1_frames;                     /* +0x34: first phase length; with phase2_frames the divisor of the fade (gsQueueFadeToColorPhases) */
    unsigned char unmodeled_38[8];
    int phase2_frames;                     /* +0x40 */
    unsigned char unmodeled_44[4];
    int fade_frames;                       /* +0x48: fade length, the divisor of the fade (gsQueueFadeToColor) */
} GsQueueContext;

void gsQueueSetAlphaFlags(int texAlphaExpand, int alphaTest, int unusedArg);
void gsQueueAddColor(int packet, int color, GsQueueContext *context);
void gsQueueAddContextColor(int packet, GsQueueContext *context);

/* Batch begin handlers: start a GIF batch of the given primitive type. */
void gsQueueBeginTriStrip(GsQueueContext *context);
void gsQueueBeginSprites(GsQueueContext *context);
/* Restart re-runs the stored begin handler; flush sends the queued qwords. */
void gsQueueRestart(void *context);
void gsQueueFlush(int packet);
/* Vertex queueing; the Kick forms also flush and restart a full queue. The
   non-kick forms ignore their first argument and the trailing
   batch/context argument, which the original callers pass but never read. */
void gsQueueAddKickVertex(int packet, float *pos, void *context);
void gsQueueAddVertex(int unused, float *pos, void *batch);
void gsQueueAddTexturedKickVertex(int packet, float *pos, float *uv, void *context);
void gsQueueAddTexturedVertex(int unused, float *pos, float *uv, void *context);
void gsQueueAddTriangle(int packet, DoublewordVector4 *pos, Vector4 *uv, void *context);

#endif /* INCLUDE_MAIN_TU213_H */
