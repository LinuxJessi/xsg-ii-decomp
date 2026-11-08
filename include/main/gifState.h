#ifndef INCLUDE_MAIN_GIFSTATE_H
#define INCLUDE_MAIN_GIFSTATE_H

#include "shared.h"

/* Types and functions of src/main/gifState.c that other files use. */

/* One impact point of the break effect (integer screen position). */
typedef struct GifBreakPoint {
    int x;
    int y;
    int z;
    int w;
} GifBreakPoint;

/* A float lane that GS packets also read as a raw word. */
typedef union GifFloat {
    f32 f;
    u32 u;
} GifFloat;

/* One qword of four GifFloat lanes (x, y, z, w). */
typedef GifFloat GifQuad[4];

/* One shard: a triangular prism (front face, back face 10 units behind, three sides as six
 * triangles) around its centre, with its spin and fall speed. */
typedef struct GifBreakShard {
    GifQuad vertex[24];       /* +0x000 */
    GifQuad projected[24];    /* +0x180 */
    GifQuad center;           /* +0x300 */
    GifQuad spin;             /* +0x310 */
    u8 unmodeled_320[0x10];
    GifFloat speed;           /* +0x330 */
    GifFloat fall;            /* +0x334 */
    u8 unmodeled_338[0x8];
} GifBreakShard;

/*
 * One GIF status block. The centre pair (+0x04/+0x08) is written by gifStateSetCenter and the
 * effect alpha pair (+0x0C/+0x10) by gifStateSetEffectAlpha; the state word and the ratio are read
 * by gifStateEffectReady.
 */
typedef struct GifStatusBlock {
    int state;
    int centerX;
    int centerY;
    int effectAlphaType;
    int effectAlpha;
    u8 unmodeled_14[0xC];
    f32 ratio;
    f32 ratioStep;
    int fadeOut;
} GifStatusBlock;

extern GifStatusBlock gifStatus;

void func_00246AF8(void);
void func_00246DD8(int value);
void gifStateClearModeValue(void);
void gifStateSuppressDrawHook(void);
void nmlModelEnableSubWindow0(void);
void gifStateSetMode(int mode);
void gifStateSetDecrease(f32 decrease);
void gifStateSetCenter(int centerX, int centerY);
void gifStateSetEffectAlpha(int alphaType, int alpha);
void gifStateSetFbp(int fbp);
void gifStateSetBlur(int blur);
void gifStateBuildBreakShards(GifBreakShard *shards, GifBreakPoint *points);
void func_00245728(GifBreakShard *shard, f32 ratio);

#endif /* INCLUDE_MAIN_GIFSTATE_H */
