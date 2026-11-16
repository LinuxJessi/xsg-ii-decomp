#include "common.h"

#include "main/tu213.h"
#include "shared.h"

/* Sprite batch header (the queue context gsQueueBeginSprites receives); only the
   words this unit reads. The two rectangles are inferred from which word lands in
   which corner component. */
typedef struct SpriteBatch {
    unsigned char unmodeled_00[0x10];
    int z;                                 /* +0x10: depth shared by both corners */
    int count;                             /* +0x14: number of times the sprite is queued */
    unsigned char unmodeled_18[0x08];
    int top;                               /* +0x20: screen rectangle */
    int left;
    int right;
    int bottom;
    int uvTop;                             /* +0x30: texture rectangle */
    int uvLeft;
    int uvRight;
    int uvBottom;
} SpriteBatch;

/* Queues the batch's textured sprite count times: the top-left and bottom-right
   corners with their texture coordinates, then flushes the packet. The corner
   vectors are written through the DoublewordVector4 union (the original's stores
   alias the batch reads, which only a union access reproduces). */
void spriteBatchDraw(SpriteBatch *batch, int packet) {
    DoublewordVector4 position[2];
    DoublewordVector4 uv[2];
    int z = batch->z;
    int count = batch->count;
    int top = batch->top;
    int left = batch->left;
    int right = batch->right;
    int bottom = batch->bottom;
    int uvTop = batch->uvTop;
    int uvLeft = batch->uvLeft;
    int uvRight = batch->uvRight;
    int uvBottom = batch->uvBottom;
    int i;

    position[0].vector.x = left;
    position[0].vector.y = top;
    uv[0].vector.x = uvLeft;
    uv[0].vector.y = uvTop;
    position[1].vector.x = right;
    uv[1].vector.x = uvRight;
    position[1].vector.y = bottom;
    uv[1].vector.y = uvBottom;
    position[0].vector.z = z;
    position[1].vector.z = z;
    gsQueueBeginSprites((GsQueueContext *) batch);
    for (i = 0; i < count; i++) {
        gsQueueAddTexturedVertex(packet, &position[0].vector.x, &uv[0].vector.x, batch);
        gsQueueAddTexturedKickVertex(packet, &position[1].vector.x, &uv[1].vector.x, batch);
    }
    gsQueueFlush(packet);
}

/* Returns at once with no effect. No caller inside this unit. */
void recordDrawType21(void) {
}

#define FRAME_WIDTH 0x200
#define FRAME_HEIGHT 0x1C0

/* Fills a clip rectangle (y, x, width, height) with the given size clamped to
   the frame, and stores the remaining frame margin in a second record. */
void clipRectFill(long long *clip, long long *margin, int width, int height) {
    clip[0] = 0;
    clip[1] = 0;
    clip[2] = width;
    clip[3] = height;
    if (clip[2] > FRAME_WIDTH) {
        clip[2] = FRAME_WIDTH;
    }
    if (clip[3] > FRAME_HEIGHT) {
        clip[3] = FRAME_HEIGHT;
    }
    margin[0] = FRAME_HEIGHT - clip[3];
    margin[1] = FRAME_WIDTH - clip[2];
    margin[2] = FRAME_WIDTH;
    margin[3] = FRAME_HEIGHT;
}

#define ABS(v) ((v) >= 0 ? (v) : -(v))

/* Splits a signed (x, y) offset between two clip records: the record that
   receives the offset gets its magnitude, the other keeps the full span. */
void func_0027A8B8(long long *a, long long *b, int x, int y) {
    int mag;

    if (x < 0) {
        mag = ABS(x);
        a[1] = 0;
        a[2] = FRAME_WIDTH;
        b[1] = mag;
        b[2] = FRAME_WIDTH - mag;
    } else {
        mag = x;
        a[1] = mag;
        a[2] = FRAME_WIDTH - mag;
        b[2] = FRAME_WIDTH;
        b[1] = 0;
    }

    if (y < 0) {
        mag = ABS(y);
        a[0] = 0;
        a[3] = FRAME_HEIGHT;
        b[0] = mag;
        b[3] = FRAME_HEIGHT - mag;
    } else {
        mag = y;
        a[0] = mag;
        a[3] = FRAME_HEIGHT - mag;
        b[3] = FRAME_HEIGHT;
        b[0] = 0;
    }
}

