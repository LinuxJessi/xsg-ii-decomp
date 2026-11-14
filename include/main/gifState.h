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
    int i;                    /* GifBreakShard.fall: the frame count of the fall */
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
    GifQuad angle;            /* +0x320: the rotation of the last frame */
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
    int fbp;                    /* +0x14: gifStateFbp is this word */
    int blur;                   /* +0x18: gifStateBlur is this word */
    int depth;                  /* +0x1C: the Z of the camera quad (func_00246DD8 sets it) */
    f32 ratio;
    f32 ratioStep;
    int subWindow[4];           /* +0x28: g_aSubWindow; word 0 is the flag gifStateEffectFrame tests,
                                 * gifStateDrawFadeQuad moves its corners by all four */
    u8 unmodeled_38[0x50 - 0x38];
    Vector4 mulColor;           /* +0x50: the model multiply colour nmlModelSetMulColor sets;
                                 * gifStateDrawCameraQuad scales it by blur into its offset */
} GifStatusBlock;

extern GifStatusBlock gifStatus;

void func_00246AF8(void);
void func_00246DD8(int value);
void gifStateClearModeValue(void);
void gifStateSuppressDrawHook(void);
void nmlModelEnableSubWindow0(void);
void nmlModelUseSubWindow(unsigned int index, int enabled);
void gifStateSetMode(int mode);
void gifStateSetDecrease(f32 decrease);
void gifStateSetCenter(int centerX, int centerY);
void gifStateSetEffectAlpha(int alphaType, int alpha);
void gifStateSetFbp(int fbp);
void gifStateSetBlur(int blur);
void gifStateBuildBreakShards(GifBreakShard *shards, GifBreakPoint *points);
void func_00245728(GifBreakShard *shard, f32 ratio);

#endif /* INCLUDE_MAIN_GIFSTATE_H */
