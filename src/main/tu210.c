#include "common.h"

#include "shared.h"

/* One rectangle of a draw record: top, left, right, bottom. */
typedef struct GsQueueRect {
    int top;
    int left;
    int right;
    int bottom;
} GsQueueRect;

/* The header every draw record starts with: the gsQueue functions read it, under the name tu213.c gives it (its
   partial view names color and halveRgb at the same offsets; tu207.c's record continues it). */
typedef struct GsQueueContext {
    unsigned char type;
    unsigned char unmodeled_01[3];
    int frame;
    int tex0;
    int color;
    int depth;                          /* 0x10: z of every vertex */
    int count;                          /* 0x14: how many times the primitive is drawn */
    int halveRgb;
    int blendKind;
} GsQueueContext;

/* Ring record (orbitDrawRing). */
typedef struct GsQueueRing {
    GsQueueContext header;
    int endColor;
    int centerX;
    int centerY;
    int radius;
    int innerRadius;
    int phaseSpeed;
    int segments;
    int amplitude;
} GsQueueRing;

/* Quad record with preset positions and a texture rectangle (quadDrawPreset). */
typedef struct GsQueuePresetQuad {
    GsQueueContext header;
    int positionPreset;
    unsigned char unmodeled_24[0x30 - 0x24];
    GsQueueRect texture;
} GsQueuePresetQuad;

/* Quad record with a screen rectangle and preset texture coordinates (quadDrawPresetUv). */
typedef struct GsQueuePresetUvQuad {
    GsQueueContext header;
    GsQueueRect screen;
    int texturePreset;
} GsQueuePresetUvQuad;

/* Circle-of-sprites record (circleDrawSegments, circleSpriteCorners). */
typedef struct GsQueueCircle {
    GsQueueContext header;
    GsQueueRect screen;
    GsQueueRect texture;
    int radiusStep;
    int ringCount;
    int endColor;
    int startAngle;
    int segments;
    int centerDisabled;
} GsQueueCircle;

/* Grid record (gridDrawCells). */
typedef struct GsQueueGrid {
    GsQueueContext header;
    int endColor;
    GsQueueRect screen;
    int columns;
    int rows;
    unsigned char unmodeled_3c[0x68 - 0x3C];
    int transformStart;
    int transformEnd;
} GsQueueGrid;

extern int D_0069DDFC; /* wave phase in degrees, advanced by phaseSpeed on every orbitDrawRing call */
extern float D_0069DE00; /* degrees to radians */

float sinf(float angle);
float cosf(float angle);
void gsQueueBeginTriStrip(void *context);
int setPointFromRecord(Vector4 *destination, int x, int y, const GsQueueContext *record);
void gsQueueAddColor(int packet, int color, GsQueueContext *context);
void gsQueueAddKickVertex(int packet, Vector4 *vector, void *context);
void gsQueueFlush(int packet);

/* Draws a ring as a triangle strip, count times: the outer edge waves with sin(phase) by amplitude around radius,
   the inner edge stays at innerRadius. When endColor is set, the outer vertices get color and the inner ones
   endColor. setPointFromRecord takes z from the record's depth. */
void orbitDrawRing(GsQueueRing *record, int packet) {
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
    count = record->header.count;
    color = record->header.color;
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
                               &record->header);
            setPointFromRecord(&points[1], innerRadius * cosf(radians) + centerX, innerRadius * sinf(radians) + centerY,
                               &record->header);
            if (endColor != 0) {
                gsQueueAddColor(packet, color, &record->header);
            }
            gsQueueAddKickVertex(packet, &points[0], record);
            if (endColor != 0) {
                gsQueueAddColor(packet, endColor, &record->header);
            }
            gsQueueAddKickVertex(packet, &points[1], record);
        }
    }
    gsQueueFlush(packet);
}

/* Stores a point: x and y from the arguments, z from the record's depth; returns that depth. */
int setPointFromRecord(Vector4 *destination, int x, int y, const GsQueueContext *record) {
    int depth;

    depth = record->depth;
    destination->x = (float) x;
    destination->y = (float) y;
    destination->z = (float) depth;
    return depth;
}

void func_0027A960(int preset, Vector4 *quad);
void gsQueueAddTexturedVertex(int packet, Vector4 *position, Vector4 *uv, void *context);
void gsQueueAddTexturedKickVertex(int packet, Vector4 *position, Vector4 *uv, void *context);

/* Draws one textured quad count times as a triangle strip: the positions are the preset quad of func_0027A960, the
   UVs come from the record's uv rectangle. */
void quadDrawPreset(GsQueuePresetQuad *record, int packet) {
    Vector4 uvs[4];
    Vector4 positions[4];
    int z;
    int top;
    int left;
    int right;
    int bottom;
    int count;
    int i;

    z = record->header.depth;
    top = record->texture.top;
    left = record->texture.left;
    right = record->texture.right;
    bottom = record->texture.bottom;
    count = record->header.count;
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

/* As quadDrawPreset with the roles swapped: the positions come from the record's rectangle, the UVs are the preset
   quad of func_0027A960. */
void quadDrawPresetUv(GsQueuePresetUvQuad *record, int packet) {
    Vector4 uvs[4];
    Vector4 positions[4];
    int z;
    int top;
    int left;
    int right;
    int bottom;
    int count;
    int i;

    z = record->header.depth;
    top = record->screen.top;
    left = record->screen.left;
    right = record->screen.right;
    bottom = record->screen.bottom;
    count = record->header.count;
    func_0027A960(record->texturePreset, uvs);
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

float xglCosDeg(int degrees);
float xglSinDeg(int degrees);
void gsQueueBeginSprites(void *context);
void circleSpriteCorners(Vector4 *points, int x, int y, const GsQueueCircle *record);
u64 xglColorLerp(u64 from, u64 to, float t);

/* Draws rings of sprites around the record rectangle, count times: segments sprites per ring, ring (ring + 1) *
   radiusStep away from the centre along directions spaced 360 / segments degrees from startAngle, each ring coloured
   from color towards endColor when endColor is set; a centre sprite comes first unless centerDisabled. Nothing is drawn
   with 15 segments or more (the direction table holds 15). */
void circleDrawSegments(GsQueueCircle *record, int packet) {
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
    color = record->header.color;
    segments = record->segments;
    count = record->header.count;
    uvTop = record->texture.top;
    uvLeft = record->texture.left;
    uvRight = record->texture.right;
    uvBottom = record->texture.bottom;
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
                circleSpriteCorners(positions, 0, 0, record);
                gsQueueAddTexturedVertex(packet, &positions[0], &uvs[0], record);
                gsQueueAddTexturedKickVertex(packet, &positions[1], &uvs[1], record);
            }
            for (ring = 0; ring < ringCount; ring++) {
                if (endColor != 0) {
                    gsQueueAddColor(packet, xglColorLerp(color, endColor, (float) ring / (float) ringCount),
                                    &record->header);
                }
                for (segment = 0; segment < segments; segment++) {
                    radius = (float) (radiusStep * (ring + 1));
                    circleSpriteCorners(positions, (int) (radius * directions[segment][0]),
                                  (int) (radius * directions[segment][1]), record);
                    gsQueueAddTexturedVertex(packet, &positions[0], &uvs[0], record);
                    gsQueueAddTexturedKickVertex(packet, &positions[1], &uvs[1], record);
                }
            }
        }
        gsQueueFlush(packet);
    }
}

/* Sets the two corner points of a sprite: the record rectangle shifted by (x, y), z from the record. */
void circleSpriteCorners(Vector4 *points, int x, int y, const GsQueueCircle *record) {
    int top;
    int left;
    int right;
    int bottom;
    int z;

    bottom = record->screen.bottom;
    top = record->screen.top;
    left = record->screen.left;
    right = record->screen.right;
    z = record->header.depth;
    points[0].x = (float) (left + x);
    points[0].y = (float) (top + y);
    points[1].x = (float) (right + x);
    points[1].y = (float) (bottom + y);
    points[0].z = (float) z;
    points[1].z = (float) z;
}

/* Results of func_00278FC8 for one grid cell. */
enum {
    GRID_CELL_FLAT,
    GRID_CELL_TRANSFORMED,
    GRID_CELL_HIDDEN
};

int func_00278FC8(Vector4 *transform, float x, float y, GsQueueGrid *record);
void quadDrawRepeated(int packet, Vector4 *quad, void *context);
void quadDrawTransformed(int packet, Vector4 *quad, void *context, Vector4 *transform);

/* Draws the record rectangle as rows x columns cells. For each cell corner func_00278FC8 (still asm: it loads
   transformStart and transformEnd at +0x68/+0x6C, compares with c.lt.s and returns 0, 1 or 2) fills a transform;
   the cell is then drawn flat, drawn transformed or skipped. endColor, when set, colours the cells (lerped by
   transform.x for transformed ones). */
void gridDrawCells(GsQueueGrid *record, int packet) {
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

    color = record->header.color;
    columns = record->columns;
    endColor = record->endColor;
    top = record->screen.top;
    left = record->screen.left;
    right = record->screen.right;
    bottom = record->screen.bottom;
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
                        gsQueueAddColor(packet, color, &record->header);
                    }
                    quadDrawRepeated(packet, quad, record);
                    break;
                case GRID_CELL_TRANSFORMED:
                    if (endColor != 0) {
                        gsQueueAddColor(packet,
                                        xglColorLerp(color, endColor,
                                                     transform.x / (float) (transformEnd - transformStart)),
                                        &record->header);
                    }
                    quadDrawTransformed(packet, quad, record, &transform);
                    break;
                }
            }
        }
        gsQueueFlush(packet);
    }
}
