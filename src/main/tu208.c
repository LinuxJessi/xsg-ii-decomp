#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tu208", func_00277720);

INCLUDE_ASM("asm/main/nonmatchings/tu208", func_002778D0);

#include "shared.h"

/* Source record read by recordProjectQuad: an int value and an (x, y) screen offset added to corner 0. */
typedef struct RecordSource {
    unsigned char unmodeled_00[0x10];
    int value;
    unsigned char unmodeled_14[0x34];
    int offset_x;
    int offset_y;
} RecordSource;

/* Batch header: the depth shared by the four corners and the number of repeats. */
typedef struct RecordBatch {
    unsigned char unmodeled_00[0x10];
    int z;
    int count;
} RecordBatch;

void func_00277C28(Vector4 *vectors, void *params, int value, int x, int y);

/* Passes the four corners to func_00277C28 with one value and an origin: corner 0 plus the source offset. */
void recordProjectQuad(Vector4 *corners, RecordSource *source, void *params)
{
    int value;
    int originX;
    int originY;

    originX = (int) (corners[0].x + (float) source->offset_x);
    value = source->value;
    originY = (int) (corners[0].y + (float) source->offset_y);
    func_00277C28(&corners[0], params, value, originX, originY);
    func_00277C28(&corners[1], params, value, originX, originY);
    func_00277C28(&corners[2], params, value, originX, originY);
    func_00277C28(&corners[3], params, value, originX, originY);
}

INCLUDE_ASM("asm/main/nonmatchings/tu208", func_00277C28);

void gsQueueAddKickVertex(void *context, Vector4 *vectors, void *batch);
void gsQueueAddVertex(void *context, Vector4 *vectors, void *batch);

/* Sets the depth of the four corners from the batch and runs the batch count times over them. */
void recordDrawBatchQuad(void *context, Vector4 *corners, RecordBatch *batch)
{
    float z;
    int count;
    int remaining;
    Vector4 *corner1;
    Vector4 *corner2;
    Vector4 *corner3;

    corner1 = &corners[1];
    corner2 = &corners[2];
    corner3 = &corners[3];
    z = (float) batch->z;
    count = batch->count;
    corner3->z = z;
    corner2->z = z;
    corner1->z = z;
    corners->z = z;
    if (count > 0)
    {
        remaining = count;
        do
        {
            remaining = remaining - 1;
            gsQueueAddVertex(context, corners, batch);
            gsQueueAddVertex(context, corner1, batch);
            gsQueueAddKickVertex(context, corner2, batch);
            gsQueueAddKickVertex(context, corner3, batch);
        }
        while (remaining != 0);
    }
}

void func_00277EA0(void *context, Vector4 *corners, RecordBatch *batch)
{
    float z;
    int count;
    int remaining;
    Vector4 *corner1;
    Vector4 *corner2;
    Vector4 *corner3;

    corner1 = &corners[1];
    corner2 = &corners[2];
    corner3 = &corners[3];
    z = (float) batch->z;
    count = batch->count;
    corner3->z = z;
    corner2->z = z;
    corner1->z = z;
    corners->z = z;
    if (count > 0)
    {
        remaining = count;
        do
        {
            remaining = remaining - 1;
            gsQueueAddVertex(context, corners, batch);
            gsQueueAddVertex(context, corner1, batch);
            gsQueueAddKickVertex(context, corner2, batch);
            gsQueueAddKickVertex(context, corner3, batch);
        }
        while (remaining != 0);
    }
}
