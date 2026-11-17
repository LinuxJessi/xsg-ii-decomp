#include "common.h"
#include "shared.h"
#include "main/tu256.h"
#include "main/xenoActor.h"

void xglCurSetTranslate(const Vector4 *vector);
void xglCurMulScaleXYZ(const Vector4 *scale);
/* Header form (tu255.h: void xglCurApplyVector(Vector4 *, const Vector4 *)) does not compile
   here: "void value not ignored" where the result is returned (the original returns the
   callee result), so int is kept. */
int xglCurApplyVector(Vector4 *, const Vector4 *);

/* Partial view of struct XenoActor (full definition in src/main/xenoActor.c): only the
 * draw flags, the focus pair, the flag word of the channel block at +0x650 and the slot at +0x79C are used here.
 * XenoActorSetChannelFlag80 runs on the XenoActor that unitCreateSide allocates (stored at unit +0x1D0). */
struct XenoActor {
    unsigned char unmodeled_000[0xBC];
    unsigned int drawFlags;                 /* 0x0BC */
    unsigned char unmodeled_0c0[0x140 - 0xC0];
    float focusX;                           /* 0x140 */
    float focusY;                           /* 0x144 */
    unsigned char unmodeled_148[0x650 - 0x148];
    int channelFlags;                       /* 0x650: flag word of the channel block */
    unsigned char unmodeled_654[0x710 - 0x654];
    XenoHrc *table;                         /* 0x710 */
    unsigned char unmodeled_714[0x79c - 0x714];
    int state;                              /* 0x79C: written here; XenoActorDrawMotion reads it and clears it */
};

#define XENO_CHANNEL_BIT80 0x80  /* channel flag bit set here; its role is not recovered */
#define CFE_DIR_COUNT 9          /* directions 0..8 index cfeDirOffsetTable */

extern float cfeDirOffsetTable[][2];

/* Damps the actor's focus pair toward the table offset of direction dir (0..8); the focus-locked draw flag is set unless both results are zero. */
void cfeDampDirOffset(struct XenoActor *actor, int dir) {
    if (dir < CFE_DIR_COUNT) {
        actor->focusX = actor->focusX * 0.9f + cfeDirOffsetTable[dir][0] * 0.1f;
        actor->focusY = actor->focusY * 0.9f + cfeDirOffsetTable[dir][1] * 0.1f;
        if (actor->focusX != 0.0f || actor->focusY != 0.0f) {
            actor->drawFlags |= XENO_DRAW_FOCUS_LOCKED;
        } else {
            actor->drawFlags &= ~XENO_DRAW_FOCUS_LOCKED;
        }
    }
}

/* Builds the current matrix from the table's translation, rotations Z/Y/X and scale, then applies it to the point stored in out. */
int XenoActorGetHrcPoint(struct XenoActor *actor, Vector4 *out) {
    XenoHrc *table = actor->table;
    XenoTransform *transform;

    if (table == 0) {
        return 0;
    }
    transform = &table->transform;
    xglCurSetTranslate((const Vector4 *) transform->translation);
    xglCurMulRotateZ(transform->rotationZ);
    xglCurMulRotateY(transform->rotationY);
    xglCurMulRotateX(transform->rotationX);
    xglCurMulScaleXYZ((const Vector4 *) transform->scale);
    out->x = transform->point[0];
    out->y = transform->point[1];
    out->z = transform->point[2];
    out->w = 1.0f;
    return xglCurApplyVector(out, out);
}

/* Sets XENO_CHANNEL_BIT80 of the channel block flag word. */
void XenoActorSetChannelFlag80(struct XenoActor *actor) {
    actor->channelFlags |= XENO_CHANNEL_BIT80;
}

/* Stores the state word that XenoActorDrawMotion reads and clears. */
void XenoActorSetState(struct XenoActor *actor, int state) {
    actor->state = state;
}
