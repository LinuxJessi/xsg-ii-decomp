#include "common.h"

#include "shared.h"

/* The draw records of this file share a header (color at 0xC, z at 0x10, repeat count at 0x14); the words from
   0x20 on depend on the primitive, so each drawing function below has its own view of them. */
typedef struct OrbitRecord {
    u8 unmodeled_00[0xC];
    int color;
    u8 unmodeled_10[0x14 - 0x10];
    int count;
    u8 unmodeled_18[0x20 - 0x18];
    int endColor;
    int centerX;
    int centerY;
    int radius;
    int innerRadius;
    int phaseSpeed;
    int segments;
    int amplitude;
} OrbitRecord;

struct IntegerRecord;

extern int D_0069DDFC; /* wave phase in degrees, advanced by phaseSpeed on every orbitDrawRing call */
extern float D_0069DE00; /* degrees to radians */

float sinf(float angle);
float cosf(float angle);
void gsQueueBeginTriStrip(void *context);
int setPointFromRecord(Vector4 *destination, int x, int y, const struct IntegerRecord *record);
void gsQueueAddColor(int packet, int color, void *context);
void gsQueueAddKickVertex(int packet, Vector4 *vector, void *context);
void gsQueueFlush(int packet);

/* Draws a ring as a triangle strip, count times: the outer edge waves with sin(phase) by amplitude around radius,
   the inner edge stays at innerRadius. When endColor is set, the outer vertices get color and the inner ones
   endColor. setPointFromRecord reads z from the same record through its IntegerRecord view. */
void orbitDrawRing(OrbitRecord *record, int packet) {
    Vector4 points[2];
    int pass;
    int color;
    int count;
    int angleStep;
    int endColor;
    int centerX;
    int centerY;
    int radius;
    int innerRadius;
    int amplitude;
    int phaseSpeed;
    int phase;
    int angle;
    float ringRadius;
    int wrappedAngle;
    float radians;

    phaseSpeed = record->phaseSpeed;
    count = record->count;
    color = record->color;
    endColor = record->endColor;
    centerX = record->centerX;
    centerY = record->centerY;
    radius = record->radius;
    innerRadius = record->innerRadius;
    amplitude = record->amplitude;
    angleStep = 365 / record->segments;
    D_0069DDFC += phaseSpeed;
    gsQueueBeginTriStrip(record);
    for (pass = 0; pass < count; pass++) {
        phase = D_0069DDFC;
        for (angle = 0; angle < 369; angle += angleStep) {
            wrappedAngle = (angle < 360) ? angle : 0;
            radians = wrappedAngle * D_0069DE00;
            ringRadius = (int) (radius + amplitude * sinf(phase * D_0069DE00));
            phase += 40;
            setPointFromRecord(&points[0], ringRadius * cosf(radians) + centerX, ringRadius * sinf(radians) + centerY,
                               (const struct IntegerRecord *) record);
            setPointFromRecord(&points[1], innerRadius * cosf(radians) + centerX, innerRadius * sinf(radians) + centerY,
                               (const struct IntegerRecord *) record);
            if (endColor != 0) {
                gsQueueAddColor(packet, color, record);
            }
            gsQueueAddKickVertex(packet, &points[0], record);
            if (endColor != 0) {
                gsQueueAddColor(packet, endColor, record);
            }
            gsQueueAddKickVertex(packet, &points[1], record);
        }
    }
    gsQueueFlush(packet);
}

#include "shared.h"

typedef struct IntegerRecord {
    u8 unmodeled_00[0x10];
    int value;
} IntegerRecord;

/* Stores a point: x and y from the arguments, z from the record value; returns that value. */
int setPointFromRecord(Vector4 *destination, int x, int y, const IntegerRecord *record) {
    int value;

    value = record->value;
    destination->x = (float) x;
    destination->y = (float) y;
    destination->z = (float) value;
    return value;
}

typedef struct TexturedQuadRecord {
    u8 unmodeled_00[0x10];
    int z;
    int count;
    u8 unmodeled_18[0x20 - 0x18];
    int positionPreset;
    u8 unmodeled_24[0x30 - 0x24];
    int uvTop;
    int uvLeft;
    int uvRight;
    int uvBottom;
} TexturedQuadRecord;

void func_0027A960(int preset, Vector4 *quad);
void gsQueueAddTexturedVertex(int packet, Vector4 *position, Vector4 *uv, void *context);
void gsQueueAddTexturedKickVertex(int packet, Vector4 *position, Vector4 *uv, void *context);

/* Draws one textured quad count times as a triangle strip: the positions are the preset quad of func_0027A960, the
   UVs come from the record's uv rectangle. */
void func_002783D0(TexturedQuadRecord *record, int packet) {
    Vector4 uvs[4];
    Vector4 positions[4];
    int z;
    int top;
    int left;
    int right;
    int bottom;
    int count;
    int i;

    z = record->z;
    top = record->uvTop;
    left = record->uvLeft;
    right = record->uvRight;
    bottom = record->uvBottom;
    count = record->count;
    func_0027A960(record->positionPreset, positions);
    uvs[0].y = (float) top;
    uvs[0].x = (float) left;
    uvs[0].z = (float) z;
    uvs[1].x = (float) right;
    uvs[1].y = (float) top;
    uvs[1].z = (float) z;
    uvs[2].x = (float) left;
    uvs[2].y = (float) bottom;
    uvs[2].z = (float) z;
    uvs[3].x = (float) right;
    uvs[3].y = (float) bottom;
    uvs[3].z = (float) z;
    gsQueueBeginTriStrip(record);
    for (i = 0; i < count; i++) {
        gsQueueAddTexturedVertex(packet, &positions[0], &uvs[0], record);
        gsQueueAddTexturedVertex(packet, &positions[1], &uvs[1], record);
        gsQueueAddTexturedKickVertex(packet, &positions[2], &uvs[2], record);
        gsQueueAddTexturedKickVertex(packet, &positions[3], &uvs[3], record);
    }
    gsQueueFlush(packet);
}

typedef struct PresetUvQuadRecord {
    u8 unmodeled_00[0x10];
    int z;
    int count;
    u8 unmodeled_18[0x20 - 0x18];
    int top;
    int left;
    int right;
    int bottom;
    int uvPreset;
} PresetUvQuadRecord;

/* As func_002783D0 with the roles swapped: the positions come from the record's rectangle, the UVs are the preset
   quad of func_0027A960. */
void func_00278558(PresetUvQuadRecord *record, int packet) {
    Vector4 uvs[4];
    Vector4 positions[4];
    int z;
    int top;
    int left;
    int right;
    int bottom;
    int count;
    int i;

    z = record->z;
    top = record->top;
    left = record->left;
    right = record->right;
    bottom = record->bottom;
    count = record->count;
    func_0027A960(record->uvPreset, uvs);
    positions[0].y = (float) top;
    positions[0].x = (float) left;
    positions[0].z = (float) z;
    positions[1].x = (float) right;
    positions[1].y = (float) top;
    positions[1].z = (float) z;
    positions[2].x = (float) left;
    positions[2].y = (float) bottom;
    positions[2].z = (float) z;
    positions[3].x = (float) right;
    positions[3].y = (float) bottom;
    positions[3].z = (float) z;
    gsQueueBeginTriStrip(record);
    for (i = 0; i < count; i++) {
        gsQueueAddTexturedVertex(packet, &positions[0], &uvs[0], record);
        gsQueueAddTexturedVertex(packet, &positions[1], &uvs[1], record);
        gsQueueAddTexturedKickVertex(packet, &positions[2], &uvs[2], record);
        gsQueueAddTexturedKickVertex(packet, &positions[3], &uvs[3], record);
    }
    gsQueueFlush(packet);
}

typedef struct CircleRecord {
    u8 unmodeled_00[0xC];
    int color;
    int z;
    int count;
    u8 unmodeled_18[0x20 - 0x18];
    int top;
    int left;
    int right;
    int bottom;
    int uvTop;
    int uvLeft;
    int uvRight;
    int uvBottom;
    int radiusStep;
    int ringCount;
    int endColor;
    int startAngle;
    int segments;
    int centerDisabled;
} CircleRecord;

float xglCosDeg(int degrees);
float xglSinDeg(int degrees);
void gsQueueBeginSprites(void *context);
void func_002789C8(Vector4 *points, int x, int y, const CircleRecord *record);
long xglColorLerp(float fraction, int from, int to);

/*CIRCLE_BEGIN*/
/* Draws rings of sprites around the record rectangle, count times: segments sprites per ring, ring (ring + 1) *
   radiusStep away from the centre along directions spaced 360 / segments degrees from startAngle, each ring coloured
   from color towards endColor when endColor is set; a centre sprite comes first unless centerDisabled. Nothing is drawn
   with 15 segments or more (the direction table holds 15). */
void circleDrawSegments(CircleRecord *record, int packet) {
    float directions[15][2];
    Vector4 positions[2];
    Vector4 uvs[2];
    int color;
    int segments;
    int count;
    int uvTop;
    int uvLeft;
    int uvRight;
    int uvBottom;
    int radiusStep;
    int ringCount;
    int endColor;
    int startAngle;
    int centerDisabled;
    int angleStep;
    int i;
    int ring;
    int segment;
    float radius;

    angleStep = 0;
    color = record->color;
    segments = record->segments;
    count = record->count;
    uvTop = record->uvTop;
    uvLeft = record->uvLeft;
    uvRight = record->uvRight;
    uvBottom = record->uvBottom;
    radiusStep = record->radiusStep;
    ringCount = record->ringCount;
    endColor = record->endColor;
    startAngle = record->startAngle;
    centerDisabled = record->centerDisabled;
    if (segments < 15) {
        uvs[0].x = (float) uvLeft;
        uvs[0].y = (float) uvTop;
        uvs[1].x = (float) uvRight;
        uvs[1].y = (float) uvBottom;
        if (segments != 0) {
            angleStep = 360 / segments;
        }
        for (i = 0; i < segments; i++) {
            directions[i][0] = xglCosDeg(startAngle + i * angleStep);
            directions[i][1] = xglSinDeg(startAngle + i * angleStep);
        }
        gsQueueBeginSprites(record);
        for (i = 0; i < count; i++) {
            if (centerDisabled == 0) {
                func_002789C8(positions, 0, 0, record);
                gsQueueAddTexturedVertex(packet, &positions[0], &uvs[0], record);
                gsQueueAddTexturedKickVertex(packet, &positions[1], &uvs[1], record);
            }
            for (ring = 0; ring < ringCount; ring++) {
                if (endColor != 0) {
                    gsQueueAddColor(packet, xglColorLerp((float) ring / (float) ringCount, color, endColor), record);
                }
                for (segment = 0; segment < segments; segment++) {
                    radius = (float) (radiusStep * (ring + 1));
                    func_002789C8(positions, (int) (radius * directions[segment][0]),
                                  (int) (radius * directions[segment][1]), record);
                    gsQueueAddTexturedVertex(packet, &positions[0], &uvs[0], record);
                    gsQueueAddTexturedKickVertex(packet, &positions[1], &uvs[1], record);
                }
            }
        }
        gsQueueFlush(packet);
    }
}
/*CIRCLE_END*/

/* Sets the two corner points of a sprite: the record rectangle shifted by (x, y), z from the record. */
void func_002789C8(Vector4 *points, int x, int y, const CircleRecord *record) {
    int top;
    int left;
    int right;
    int bottom;
    int z;

    bottom = record->bottom;
    top = record->top;
    left = record->left;
    right = record->right;
    z = record->z;
    points[0].x = (float) (left + x);
    points[0].y = (float) (top + y);
    points[1].x = (float) (right + x);
    points[1].y = (float) (bottom + y);
    points[0].z = (float) z;
    points[1].z = (float) z;
}

typedef struct GridRecord {
    u8 unmodeled_00[0xC];
    int color;
    u8 unmodeled_10[0x20 - 0x10];
    int endColor;
    int top;
    int left;
    int right;
    int bottom;
    int columns;
    int rows;
    u8 unmodeled_3C[0x68 - 0x3C];
    int transformStart;
    int transformEnd;
} GridRecord;

/* Results of func_00278FC8 for one grid cell. */
enum {
    GRID_CELL_FLAT,
    GRID_CELL_TRANSFORMED,
    GRID_CELL_HIDDEN
};

int func_00278FC8(Vector4 *transform, float x, float y, GridRecord *record);
void quadDrawRepeated(int packet, Vector4 *quad, void *context);
void quadDrawTransformed(int packet, Vector4 *quad, void *context, Vector4 *transform);

/* Draws the record rectangle as rows x columns cells. func_00278FC8 fills a transform for each cell from its corner:
   below transformStart the cell is drawn flat, past transformEnd it is skipped, in between it is drawn transformed;
   endColor, when set, colours the cells (lerped by transform.x for transformed ones). */
void gridDrawCells(GridRecord *record, int packet) {
    Vector4 quad[4];
    Vector4 transform;
    int color;
    int columns;
    int endColor;
    int top;
    int left;
    int right;
    int bottom;
    int rows;
    int transformStart;
    int transformEnd;
    int cellWidth;
    int cellHeight;
    int row;
    int column;
    int clip;

    color = record->color;
    columns = record->columns;
    endColor = record->endColor;
    top = record->top;
    left = record->left;
    right = record->right;
    bottom = record->bottom;
    rows = record->rows;
    transformStart = record->transformStart;
    transformEnd = record->transformEnd;
    if (columns > 0 && rows > 0) {
        cellWidth = (right - left) / columns;
        cellHeight = (bottom - top) / rows;
        gsQueueBeginTriStrip(record);
        for (row = 0; row < rows; row++) {
            for (column = 0; column < columns; column++) {
                quad[0].x = (float) (column * cellWidth + left);
                quad[0].y = (float) (row * cellHeight + top);
                quad[0].z = 0.0f;
                quad[1].x = (float) ((column + 1) * cellWidth + left);
                quad[1].y = (float) (row * cellHeight + top);
                quad[1].z = 0.0f;
                quad[2].x = (float) (column * cellWidth + left);
                quad[2].y = (float) ((row + 1) * cellHeight + top);
                quad[2].z = 0.0f;
                quad[3].x = (float) ((column + 1) * cellWidth + left);
                quad[3].y = (float) ((row + 1) * cellHeight + top);
                quad[3].z = 0.0f;
                clip = func_00278FC8(&transform, quad[0].x, quad[0].y, record);
                if (clip == GRID_CELL_HIDDEN) {
                    continue;
                }
                switch (clip) {
                case GRID_CELL_FLAT:
                    if (endColor != 0) {
                        gsQueueAddColor(packet, color, record);
                    }
                    quadDrawRepeated(packet, quad, record);
                    break;
                case GRID_CELL_TRANSFORMED:
                    if (endColor != 0) {
                        gsQueueAddColor(packet,
                                        xglColorLerp(transform.x / (float) (transformEnd - transformStart),
                                                     color, endColor),
                                        record);
                    }
                    quadDrawTransformed(packet, quad, record, &transform);
                    break;
                }
            }
        }
        gsQueueFlush(packet);
    }
}
