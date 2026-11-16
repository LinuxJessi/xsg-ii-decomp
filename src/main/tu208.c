#include "common.h"
#include "shared.h"

/* Source record read by recordProjectQuad: an int value and an (x, y) screen offset added to corner 0. */
typedef struct RecordSource {
    unsigned char unmodeled_00[0x10];
    int value;
    unsigned char unmodeled_14[0x34];
    int offset_x;
    int offset_y;
} RecordSource;

/*
 * Batch header: the depth shared by the four corners and the number of repeats. recordDrawCurve reads
 * the graph fields from 0x20: colour, baseline, time range, top, columns, rows and value ceiling.
 */
typedef struct RecordBatch {
    unsigned char unmodeled_00[0x10];
    int z;
    int count;
    unsigned char unmodeled_18[0x20 - 0x18];
    int color;
    int bottom;
    int startTime;
    int endTime;
    int top;
    int columns;
    int rows;
    unsigned char unmodeled_3c[0x5c - 0x3c];
    int maxValue;
} RecordBatch;

/*
 * Gradient view of the record: two colours and a rectangle (top, left, right, bottom); vertical
 * selects whether the colour changes along y (non-zero) or along x.
 */
typedef struct RecordGradient {
    unsigned char unmodeled_00[0x10];
    int z;
    int count;
    unsigned char unmodeled_18[0x20 - 0x18];
    int color0;
    int color1;
    int top;
    int left;
    int right;
    int bottom;
    int vertical;
} RecordGradient;

/*
 * The draw record under the views this file reads; z and count are shared. Reading the gradient
 * through the union (alias set 0) keeps the original's field-load order around the corner stores.
 */
typedef union Record {
    RecordSource source;
    RecordBatch batch;
    RecordGradient gradient;
} Record;

void gsQueueBeginTriStrip(void *context);
void gsQueueAddColor(void *packet, int color, void *context);
void gsQueueAddVertex(void *context, Vector4 *vectors, void *batch);
void gsQueueAddKickVertex(void *context, Vector4 *vectors, void *batch);
void gsQueueFlush(void *packet);

/* Draws the record's rectangle as one two-colour strip, count times. */
void recordDrawStrip(Record *record, void *packet)
{
    Vector4 corners[4];
    int depth;
    int count;
    int color0;
    int color1;
    int top;
    int right;
    int bottom;
    int i;

    depth = record->gradient.z;
    count = record->gradient.count;
    color0 = record->gradient.color0;
    color1 = record->gradient.color1;
    top = record->gradient.top;
    right = record->gradient.right;
    bottom = record->gradient.bottom;
    corners[0].x = (float) record->gradient.left;
    corners[0].y = (float) top;
    if (record->gradient.vertical == 0)
    {
        corners[1].x = (float) record->gradient.left;
        corners[1].y = (float) bottom;
        corners[2].x = (float) right;
        corners[2].y = (float) top;
    }
    else
    {
        corners[1].x = (float) right;
        corners[1].y = (float) top;
        corners[2].x = (float) record->gradient.left;
        corners[2].y = (float) bottom;
    }
    corners[3].x = (float) right;
    corners[3].y = (float) bottom;
    corners[0].z = corners[1].z = corners[2].z = corners[3].z = (float) depth;
    gsQueueBeginTriStrip(record);
    for (i = 0; i < count; i++)
    {
        gsQueueAddColor(packet, color0, record);
        gsQueueAddVertex(packet, &corners[0], record);
        gsQueueAddVertex(packet, &corners[1], record);
        gsQueueAddColor(packet, color1, record);
        gsQueueAddKickVertex(packet, &corners[2], record);
        gsQueueAddKickVertex(packet, &corners[3], record);
    }
    gsQueueFlush(packet);
}

void *memset(void *destination, int value, unsigned int count);
typedef struct AnimSegment AnimSegment; /* defined in tu209.c */
int animSampleSegment(f32 *out, AnimSegment *seg, f32 position);
u64 xglColorLerp(u64 from, u64 to, f32 t);
void animOffsetPoints(Vector4 *points, f32 dx, f32 dy);
void recordProjectQuad(Vector4 *corners, RecordSource *source, void *params);
void recordDrawBatchQuad(void *context, Vector4 *corners, RecordBatch *batch);
void func_00277EA0(void *context, Vector4 *corners, RecordBatch *batch);

/*
 * Draws the record's curve as columns of quads: one column per time step, filled in rows up to the
 * sampled value. The record is passed on untyped: each helper reads its own partial view of it
 * (RecordSource, RecordBatch, AnimSegment in tu209.c).
 */
void recordDrawCurve(void *record, void *packet)
{
    RecordBatch *graph;
    Vector4 corners[4];
    f32 sample[4];
    int color;
    int top;
    int maxValue;
    int rowHeight;
    int startTime;
    int endTime;
    int bottom;
    int columns;
    int rows;
    int columnWidth;
    int column;
    int next;
    int state;
    int row;
    f32 columnLeft;
    f32 columnRight;
    f32 rowTop;

    graph = record;
    color = graph->color;
    bottom = graph->bottom;
    startTime = graph->startTime;
    endTime = graph->endTime;
    top = graph->top;
    columns = graph->columns;
    rows = graph->rows;
    maxValue = graph->maxValue;
    memset(corners, 0, sizeof(corners));
    if (columns <= 0 || rows <= 0)
    {
        return;
    }
    columnWidth = (endTime - startTime) / columns;
    rowHeight = (top - bottom) / rows;
    gsQueueBeginTriStrip(record);
    for (column = 0; column < columns; column = next)
    {
        next = column + 1;
        columnRight = (f32) (next * columnWidth + startTime);
        state = animSampleSegment(sample, record, columnRight);
        if (state == 2)
        {
            continue;
        }
        columnLeft = (f32) (column * columnWidth + startTime);
        corners[0].x = columnLeft;
        corners[0].y = (f32) bottom;
        corners[0].z = 0.0f;
        corners[1].x = columnRight;
        corners[1].y = (f32) bottom;
        corners[1].z = 0.0f;
        if (state == 0)
        {
            corners[2].x = columnLeft;
            corners[2].y = (f32) top;
            corners[2].z = 0.0f;
            corners[3].x = columnRight;
            corners[3].y = (f32) top;
            corners[3].z = 0.0f;
            func_00277EA0(packet, corners, record);
        }
        else
        {
            gsQueueAddColor(packet, xglColorLerp(0x80FFFFFF, color, sample[0] / (f32) maxValue), record);
            rowTop = (f32) (bottom + rowHeight);
            corners[2].x = columnLeft;
            corners[2].y = rowTop;
            corners[2].z = 0.0f;
            corners[3].x = columnRight;
            corners[3].y = rowTop;
            corners[3].z = 0.0f;
            recordProjectQuad(corners, record, sample);
            for (row = 0; row < rows; row++)
            {
                recordDrawBatchQuad(packet, corners, record);
                animOffsetPoints(corners, 0.0f, (f32) rowHeight);
            }
        }
    }
    gsQueueFlush(packet);
}

void recordRotateCorner(Vector4 *vectors, void *params, int value, int x, int y);

/* Passes the four corners to recordRotateCorner with one value and an origin: corner 0 plus the source offset. */
void recordProjectQuad(Vector4 *corners, RecordSource *source, void *params)
{
    int value;
    int originX;
    int originY;

    originX = (int) (corners[0].x + (float) source->offset_x);
    value = source->value;
    originY = (int) (corners[0].y + (float) source->offset_y);
    recordRotateCorner(&corners[0], params, value, originX, originY);
    recordRotateCorner(&corners[1], params, value, originX, originY);
    recordRotateCorner(&corners[2], params, value, originX, originY);
    recordRotateCorner(&corners[3], params, value, originX, originY);
}

#include "vu0_macro.h"

extern f32 D_0069DDF8;
void xglMatrixUnit(Matrix4 matrix);
void xglMatrixRotX(Matrix4 destination, Matrix4 source, f32 angle);
void xglMatrixRotY(Matrix4 destination, Matrix4 source, f32 angle);
void xglMatrixRotZ(Matrix4 destination, Matrix4 source, f32 angle);
void xglMatrixMul(void *destination, void *left, void *right);
void xglVectorMulMat(Vector4 *destination, Matrix4 matrix, const Vector4 *vector);

/* Rotates one corner by the (x, y, z) angles in degrees of params around the point (x, y); value is not read. */
void recordRotateCorner(Vector4 *vectors, void *params, int value, int x, int y)
{
    AlignedVector4 offset;
    AlignedVector4 centered;
    Matrix4 rotationX;
    Matrix4 rotationY;
    Matrix4 rotationZ;
    Matrix4 rotation;
    Matrix4 rotationXY;
    AlignedVector4 rotated;
    AlignedVector4 result;
    AlignedVector4 position;
    Vector4 *angles;

    angles = params;
    position = *vectors;
    offset.x = (f32) -x;
    offset.y = (f32) -y;
    offset.z = 0.0f;
    Vu0AddVectorXYZ(&centered, &position, &offset);
    xglMatrixUnit(rotationX);
    xglMatrixUnit(rotationY);
    xglMatrixUnit(rotationZ);
    xglMatrixUnit(rotationXY);
    xglMatrixUnit(rotation);
    xglMatrixRotX(rotationX, rotationX, angles->x * D_0069DDF8);
    xglMatrixRotY(rotationY, rotationY, angles->y * D_0069DDF8);
    xglMatrixRotZ(rotationZ, rotationZ, angles->z * D_0069DDF8);
    xglMatrixMul(rotationXY, rotationX, rotationY);
    xglMatrixMul(rotation, rotationZ, rotationXY);
    xglVectorMulMat(&rotated, rotation, &centered);
    Vu0SubVectorXYZ(&result, &rotated, &offset);
    *vectors = result;
}

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
