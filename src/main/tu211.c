#include "common.h"
#include "main/tu204.h"
#include "main/tu213.h"
#include "main/xgl_2.h"
#include "shared.h"
#include "vu0_macro.h"

/* Batch header: the depth shared by the four corners and the number of repeats. */
typedef struct QuadBatch {
    unsigned char unmodeled_00[0x10];
    int z;
    int count;
} QuadBatch;

/* Transformed batch: the repeat parameters of QuadBatch plus the screen offset at +0x60. */
typedef struct QuadTransform {
    unsigned char unmodeled_00[0x10];
    int value;
    int count;
    unsigned char unmodeled_18[0x48];
    int offset_x;
    int offset_y;
} QuadTransform;

void quadRotateCorner(Vector4 *destination, Vector4 *source, void *rotation, int value, int x, int y);
void randomPointInRect(Vector4 *position, Vector4 *uv, int x, int y, void *rect);
void randomRefreshSeed(void);

/* Transforms the four corners around corner 0 plus the screen offset and queues the quad count times. */
void quadDrawTransformed(int packet, Vector4 *corners, QuadTransform *transform, void *rotation)
{
    Vector4 quad[4];
    int x;
    int y;
    int value;
    int count;
    int remaining;

    x = (int) (corners[0].x + (float) transform->offset_x);
    y = (int) (corners[0].y + (float) transform->offset_y);
    value = transform->value;
    count = transform->count;
    quadRotateCorner(&quad[0], &corners[0], rotation, value, x, y);
    quadRotateCorner(&quad[1], &corners[1], rotation, value, x, y);
    quadRotateCorner(&quad[2], &corners[2], rotation, value, x, y);
    quadRotateCorner(&quad[3], &corners[3], rotation, value, x, y);
    if (count > 0)
    {
        remaining = count;
        do
        {
            remaining = remaining - 1;
            gsQueueAddVertex(packet, &quad[0].x, transform);
            gsQueueAddVertex(packet, &quad[1].x, transform);
            gsQueueAddKickVertex(packet, &quad[2].x, transform);
            gsQueueAddKickVertex(packet, &quad[3].x, transform);
        }
        while (remaining != 0);
    }
}

/* Copies the four corners, sets their depth from the batch and queues the quad count times. */

void quadDrawRepeated(int packet, Vector4 *corners, QuadBatch *batch)
{
    Vector4 quad[4];
    int z;
    int count;
    int remaining;

    z = batch->z;
    count = batch->count;
    quad[0] = corners[0];
    quad[1] = corners[1];
    quad[2] = corners[2];
    quad[3] = corners[3];
    quad[3].z = (float) z;
    quad[2].z = (float) z;
    quad[1].z = (float) z;
    quad[0].z = (float) z;
    if (count > 0)
    {
        remaining = count;
        do
        {
            remaining = remaining - 1;
            gsQueueAddVertex(packet, &quad[0].x, batch);
            gsQueueAddVertex(packet, &quad[1].x, batch);
            gsQueueAddKickVertex(packet, &quad[2].x, batch);
            gsQueueAddKickVertex(packet, &quad[3].x, batch);
        }
        while (remaining != 0);
    }
}

/* Colour ramp around a centre (partial layout, only the words pointFadeColorStep reads). */
typedef struct ArcParams {
    unsigned char unmodeled_00[0x3C];
    int centerX;
    int centerY;
    int radius;
    int fromA;
    int fromB;
    int fromC;
    int toA;
    int toB;
    int toC;
    int unmodeled_60[2];
    int limitA;
    int limitB;
    int invert;
} ArcParams;

f32 intLerp(f32 from, f32 to, f32 fraction);
void *memset(void *dest, int value, unsigned int count);

/* Scales a point's squared distance from the centre into a fraction, lerps three channels by it and
 * returns 0, 1 or 2 when the channels pass their limits (cleared on 0 and 2). */
int pointFadeColorStep(f32 *out, const ArcParams *params, f32 x, f32 y) {
    int centerY = params->centerY;
    int radius = params->radius;
    int centerX = params->centerX;
    int invert = params->invert;
    int fromB = params->fromB;
    int fromA = params->fromA;
    int fromC = params->fromC;
    int toA = params->toA;
    int toB = params->toB;
    int toC = params->toC;
    int limitA = params->limitA;
    int limitB = params->limitB;
    f32 dx = x - (f32) centerX;
    f32 dy = y - (f32) centerY;
    f32 fraction = (dx * dx + dy * dy) / (f32) (radius * radius);

    if (invert != 0) {
        fraction = 1.0f - fraction;
    }
    out[0] = intLerp((f32) fromA, (f32) toA, fraction);
    out[1] = intLerp((f32) fromB, (f32) toB, fraction);
    out[2] = intLerp((f32) fromC, (f32) toC, fraction);
    if (out[0] < (f32) limitA) {
        memset(out, 0, 16);
        return 0;
    }
    if ((f32) limitB < out[0]) {
        memset(out, 0, 16);
        out[0] = 90.0f;
        return 2;
    }
    return 1;
}

/* Linear interpolation from `from` to `to` by fraction; the same body as animLerp in tu209. */
f32 intLerp(f32 from, f32 to, f32 fraction) {
    return ((to - from) * fraction) + from;
}

extern f32 D_0069DE04;
void xglMatrixUnit(Matrix4 matrix);
void xglMatrixRotZ(Matrix4 destination, Matrix4 source, f32 angle);
void xglMatrixRotX(Matrix4 destination, Matrix4 source, f32 angle);
void xglMatrixMul(Matrix4 destination, Matrix4 first, Matrix4 second);

/* Rotates source by the three Euler angles (degrees) around the point (x, y) and sets the depth from value. */
void quadRotateCorner(Vector4 *destination, Vector4 *source, void *rotation, int value, int x, int y)
{
    AlignedVector4 offset;
    AlignedVector4 position;
    Matrix4 rotX;
    Matrix4 rotY;
    Matrix4 rotZ;
    Matrix4 rotXYZ;
    Matrix4 rotXY;
    AlignedVector4 rotated;
    AlignedVector4 result;
    f32 *angles;

    angles = (f32 *) rotation;
    offset.x = (f32) -x;
    offset.y = (f32) -y;
    offset.z = 0.0f;
    Vu0AddVectorXYZ(&position, source, &offset);
    xglMatrixUnit(rotX);
    xglMatrixUnit(rotY);
    xglMatrixUnit(rotZ);
    /* The original also initialises rotXY and rotXYZ to identity before xglMatrixMul overwrites them (five xglMatrixUnit calls). */
    xglMatrixUnit(rotXY);
    xglMatrixUnit(rotXYZ);
    xglMatrixRotX(rotX, rotX, angles[0] * D_0069DE04);
    xglMatrixRotY(rotY, rotY, angles[1] * D_0069DE04);
    xglMatrixRotZ(rotZ, rotZ, angles[2] * D_0069DE04);
    xglMatrixMul(rotXY, rotX, rotY);
    xglMatrixMul(rotXYZ, rotZ, rotXY);
    xglVectorMulMat(&rotated, rotXYZ, &position);
    Vu0SubVectorXYZ(&result, &rotated, &offset);
    *(AlignedVector4 *) destination = result;
    destination->z = (f32) value;
}

void recordSetAlphaFlags(int *record)
{
    int value;
    int extra;

    value = record[3];
    extra = record[2];
    if (record[1] != 0) {
        gsQueueSetAlphaFlags(value, extra, value);
        return;
    }
    gsQueueSetAlphaFlags(0, 0, value);
}

/* Per-quad parameters of quadDrawRotating (partial layout, only the words it reads). */
typedef struct QuadRotation {
    unsigned char unmodeled_00[0x10];
    int z;                      /* +0x10: depth of all four corners */
    int count;                  /* +0x14: how many times the quad is queued */
    unsigned char unmodeled_18[8];
    int top;                    /* +0x20 */
    int left;                   /* +0x24 */
    int right;                  /* +0x28 */
    int bottom;                 /* +0x2C */
    int v0;                     /* +0x30: top texture coordinate */
    int u0;                     /* +0x34: left texture coordinate */
    int u1;                     /* +0x38: right texture coordinate */
    int v1;                     /* +0x3C: bottom texture coordinate */
    int margin;                 /* +0x40: the quad is grown by this on every side */
    int wobbleX;                /* +0x44: amplitude of the cosine offset */
    int wobbleY;                /* +0x48: amplitude of the sine offset */
    int angle;                  /* +0x4C: rotation around the pivot (degrees) */
    int phaseStep;              /* +0x50: added to the global phase every call */
    int pivotX;                 /* +0x54 */
    int pivotY;                 /* +0x58 */
} QuadRotation;

extern f32 D_0069DE08;          /* wobble phase (degrees), advanced by every quadDrawRotating call */
extern f32 D_0069B56C;          /* degrees-to-radians factor used for the wobble phase */
f32 cosf(f32 x);
f32 sinf(f32 x);
void quadRotateAroundCenter(Vector4 *vectors, f32 angle, f32 centerX, f32 centerY);

/* Builds the wobbling, rotated quad from quad (the phase advances every call) and queues it count times. */
void quadDrawRotating(QuadRotation *quad, int packet)
{
    Vector4 corners[4];
    Vector4 uvs[4];
    int z;
    int count;
    int top;
    int left;
    int right;
    int bottom;
    int v0;
    int u0;
    int u1;
    int v1;
    int margin;
    int wobbleX;
    int wobbleY;
    int angle;
    int pivotX;
    int pivotY;
    f32 toRad = D_0069B56C;
    f32 amplitudeX;
    f32 amplitudeY;
    f32 offsetX;
    f32 offsetY;
    f32 leftX;
    f32 topY;
    f32 rightX;
    f32 bottomY;
    int remaining;
    wobbleX = quad->wobbleX;
    z = quad->z;
    count = quad->count;
    top = quad->top;
    left = quad->left;
    D_0069DE08 = D_0069DE08 + (f32) quad->phaseStep;
    right = quad->right;
    bottom = quad->bottom;
    v0 = quad->v0;
    u0 = quad->u0;
    u1 = quad->u1;
    v1 = quad->v1;
    margin = quad->margin;
    wobbleY = quad->wobbleY;
    angle = quad->angle;
    amplitudeX = (f32) wobbleX;
    pivotX = quad->pivotX;
    pivotY = quad->pivotY;
    offsetX = amplitudeX * cosf(D_0069DE08 * toRad);
    amplitudeY = (f32) wobbleY;
    offsetY = amplitudeY * sinf(D_0069DE08 * toRad);
    leftX = ((f32) left - (f32) margin) + offsetX;
    topY = ((f32) top - (f32) margin) + offsetY;
    rightX = ((f32) right + (f32) margin) + offsetX;
    bottomY = ((f32) bottom + (f32) margin) + offsetY;
    uvs[2].x = uvs[0].x = (f32) u0;
    uvs[1].y = uvs[0].y = (f32) v0;
    uvs[3].x = uvs[1].x = (f32) u1;
    uvs[3].y = uvs[2].y = (f32) v1;
    corners[2].x = corners[0].x = leftX;
    corners[1].y = corners[0].y = topY;
    corners[3].x = corners[1].x = rightX;
    corners[3].y = corners[2].y = bottomY;
    corners[3].z = corners[2].z = corners[1].z = corners[0].z = (f32) z;
    quadRotateAroundCenter(corners, (f32) angle, (f32) pivotX, (f32) pivotY);
    gsQueueBeginTriStrip((GsQueueContext *) quad);
    if (count > 0)
    {
        remaining = count;
        do
        {
            remaining = remaining - 1;
            gsQueueAddTexturedVertex(packet, (float *) &corners[0], (float *) &uvs[0], quad);
            gsQueueAddTexturedVertex(packet, (float *) &corners[1], (float *) &uvs[1], quad);
            gsQueueAddTexturedKickVertex(packet, (float *) &corners[2], (float *) &uvs[2], quad);
            gsQueueAddTexturedKickVertex(packet, (float *) &corners[3], (float *) &uvs[3], quad);
        }
        while (remaining != 0);
    }
    gsQueueFlush(packet);
}

extern f32 degToRad;

/* Rotates four vectors by angle (degrees) around the point (centerX, centerY). */
void quadRotateAroundCenter(Vector4 *vectors, f32 angle, f32 centerX, f32 centerY)
{
    AlignedVector4 center;
    AlignedVector4 offset;
    AlignedVector4 rotated;
    Matrix4 rotation;
    int i;

    center.x = centerX;
    center.y = centerY;
    center.z = 0.0f;
    xglMatrixUnit(rotation);
    xglMatrixRotZ(rotation, rotation, angle * degToRad);
    for (i = 0; i < 4; i++) {
        Vu0SubVector(&offset, vectors, &center);
        xglVectorMulMat(&rotated, rotation, &offset);
        __asm__ __volatile__("lqc2 vf20, 0(%0)" : : "r"(&rotated) : "memory");
        __asm__ __volatile__("lqc2 vf21, 0(%0)" : : "r"(&center) : "memory");
        __asm__ __volatile__("vadd.xyzw vf20, vf20, vf21" : : : "memory");
        __asm__ __volatile__("sqc2 vf20, 0(%0)" : : "r"(vectors) : "memory");
        vectors++;
    }
}

/* Bounds of the randomly displaced grid of pointGridDraw (partial layout, only the words it reads). */
typedef struct QuadScatter {
    unsigned char unmodeled_00[0x14];
    int repeat;                 /* +0x14: how many times the grid is drawn */
    unsigned char unmodeled_18[8];
    int top;                    /* +0x20 */
    int left;                   /* +0x24 */
    int right;                  /* +0x28 */
    int bottom;                 /* +0x2C */
    unsigned char unmodeled_30[0x10];
    int divisions;              /* +0x40: grid cells per side */
} QuadScatter;

/* Queues the grid of randomly displaced points repeat times as triangle strips: row by row, each column adds the points of two grid rows (the first column without kick).
   points[0] holds the current grid row and points[1] the next one; the next row is reused as the current row of the following pass. */
void pointGridDraw(QuadScatter *scatter, int packet)
{
    Vector4 points[2][30];
    Vector4 currentUv;
    Vector4 nextUv;
    int column;
    int remaining;
    int divisions = scatter->divisions;
    int repeat = scatter->repeat;
    int top = scatter->top;
    int left = scatter->left;
    int right = scatter->right;
    int bottom = scatter->bottom;
    int stepX;
    int stepY;
    int row;

    if (divisions > 0)
    {
        randomRefreshSeed();
        stepY = (bottom - top) / divisions;
        stepX = (right - left) / divisions;
        if (repeat > 0)
        {
            remaining = repeat;
            do
            {
                gsQueueBeginTriStrip((GsQueueContext *) scatter);
                for (row = 0; row < divisions; row++)
                {
                    for (column = 0; column <= divisions; column++)
                    {
                        if (row == 0)
                        {
                            randomPointInRect(&points[0][column], &currentUv, left + column * stepX, top, scatter);
                        }
                        else
                        {
                            points[0][column] = points[1][column];
                            currentUv.x = (f32) (left + column * stepX);
                            currentUv.y = (f32) (top + row * stepY);
                        }
                        randomPointInRect(&points[1][column], &nextUv, left + column * stepX, top + (row + 1) * stepY, scatter);
                        if (column == 0)
                        {
                            gsQueueAddTexturedVertex(packet, (float *) &points[0][0], (float *) &currentUv, scatter);
                            gsQueueAddTexturedVertex(packet, (float *) &points[1][0], (float *) &nextUv, scatter);
                        }
                        else
                        {
                            gsQueueAddTexturedKickVertex(packet, (float *) &points[0][column], (float *) &currentUv, scatter);
                            gsQueueAddTexturedKickVertex(packet, (float *) &points[1][column], (float *) &nextUv, scatter);
                        }
                    }
                }
                gsQueueFlush(packet);
                remaining = remaining - 1;
            }
            while (remaining != 0);
        }
    }
}

int func_00279AB8(f32 *values, int limit);
int randomSeededIndex(f32 *values, int limit);

/* Random-point words of the effect parameter block (the same block pointGridDraw reads as QuadScatter). */
typedef struct PointRect {
    unsigned char unmodeled_00[0x10];
    int depth;                  /* +0x10: z of every point */
    unsigned char unmodeled_14[0x1C];
    int countX;                 /* +0x30: horizontal displacement range, none when <= 0 */
    int offsetX;                /* +0x34: added to the horizontal displacement */
    int countY;                 /* +0x38: vertical displacement range, none when <= 0 */
    int offsetY;                /* +0x3C: added to the vertical displacement */
} PointRect;

/* Ring-wave grid effect words of the queue context, read by ringWaveGridDraw and ringWavePoint (partial layout; the fields are used only here). */
typedef struct RingWave {
    unsigned char unmodeled_00[0x10];
    int z;                      /* +0x10: depth of every vertex */
    int repeat;                 /* +0x14: how many times the grid is drawn */
    unsigned char unmodeled_18[8];
    int colorB;                 /* +0x20: second fade color, 0 for none */
    int top;                    /* +0x24 */
    int left;                   /* +0x28 */
    int right;                  /* +0x2C */
    int bottom;                 /* +0x30 */
    int centerX;                /* +0x34 */
    int centerY;                /* +0x38 */
    int innerRadius;            /* +0x3C */
    int outerRadius;            /* +0x40 */
    int phaseScale;             /* +0x44 */
    int amplitude;              /* +0x48 */
    int angleStep;              /* +0x4C: added to the global wave angle D_0069DE1C per call */
    int divisions;              /* +0x50: grid cells per side */
} RingWave;

/* The parameter block holds int or float words depending on the effect; reading it through this
   union keeps the loads aliased with float data (the original scheduling depends on that). */
typedef union EffectParams {
    PointRect point;
    RingWave ring;
    f32 words[21];
} EffectParams;

/* Writes the grid point (x, y) displaced by a seeded random amount below countX/countY plus the offsets,
   with the block depth as z, and stores the undisplaced point as its uv. */
void randomPointInRect(Vector4 *position, Vector4 *uv, int x, int y, void *rect)
{
    EffectParams *params = rect;
    int depth = params->point.depth;
    int countX = params->point.countX;
    int offsetX = params->point.offsetX;
    int countY = params->point.countY;
    int offsetY = params->point.offsetY;
    int displaceX;
    int displaceY;

    if (countX > 0)
    {
        displaceX = (unsigned int) func_00279AB8(&uv->x, countX) % countX + offsetX;
    }
    else
    {
        displaceX = offsetX;
    }
    if (countY > 0)
    {
        displaceY = (unsigned int) randomSeededIndex(&uv->x, countY) % countY + offsetY;
    }
    else
    {
        displaceY = offsetY;
    }
    position->x = (f32) (x + displaceX);
    position->y = (f32) (y + displaceY);
    position->z = (f32) depth;
    uv->x = (f32) x;
    uv->y = (f32) y;
}

extern int recordUpdatePaused;
extern int randomSeed;

/* Same body as randomSeededIndex below; the original binary contains two copies. */
int func_00279AB8(f32 *values, int limit) {
    if (recordUpdatePaused == 0) {
        return xglSRand();
    }
    return (int) (((values[1] * 20.0f) / values[0]) + (f32) randomSeed) % limit;
}

/* Seeded random index below limit (or the game RNG when the seed flag is clear). */
int randomSeededIndex(f32 *values, int limit) {
    if (recordUpdatePaused == 0) {
        return xglSRand();
    }
    return (int) (((values[0] * 20.0f) / values[1]) + (f32) randomSeed) % limit;
}

void randomRefreshSeed(void) {
    if (recordUpdatePaused == 0) {
        randomSeed = xglSRand();
    }
}

extern f32 D_0069DE10;
extern f32 D_0069DE14;
extern f32 D_0069DE18;

/* Grid-effect words of the queue context (GsQueueContext) read by quadOrbitPoint and ringWaveGridDraw (partial layout; the fields are used only here). */
typedef struct QuadGrid {
    unsigned char unmodeled_00[0x10];
    int height;                 /* +0x10 */
    int repeat;                 /* +0x14: how many times the grid is drawn */
    unsigned char unmodeled_18[8];
    int colorB;                 /* +0x20: second fade color, 0 for none */
    int top;                    /* +0x24 */
    int left;                   /* +0x28 */
    int right;                  /* +0x2C */
    int bottom;                 /* +0x30 */
    int radiusX;                /* +0x34 */
    unsigned int speedY;        /* +0x38 */
    int phaseStepA;             /* +0x3C: added to the global phase D_0069DE10 per call */
    int radiusY;                /* +0x40 */
    unsigned int speedX;        /* +0x44 */
    int phaseStepB;             /* +0x48: added to the global phase D_0069DE14 per call */
    int divisions;              /* +0x4C: grid cells per side */
} QuadGrid;

f32 quadOrbitPoint(Vector4 *position, Vector4 *center, int centerX, int centerY, QuadGrid *orbit);
f32 angleWrap360(f32 angle);
void gsQueueFadeToColorPhases(int packet, int colorB, GsQueueContext *context, f32 t);

/* Queues the orbiting grid repeat times as triangle strips: row by row, each column adds the points of two grid rows (the first column without kick). */
void quadOrbitGridDraw(QuadGrid *grid, int packet)
{
    Vector4 point[2];
    Vector4 center[2];
    int remaining;
    int divisions = grid->divisions;
    int repeat = grid->repeat;
    int colorB = grid->colorB;
    int top = grid->top;
    int left = grid->left;
    int right = grid->right;
    int bottom = grid->bottom;
    int phaseStepA = grid->phaseStepA;
    int phaseStepB = grid->phaseStepB;
    int stepX;
    int stepY;
    int row;
    int column;
    f32 height0;
    f32 height1;

    if (divisions > 0)
    {
        stepY = (bottom - top) / divisions;
        stepX = (right - left) / divisions;
        if (recordUpdatePaused == 0)
        {
            D_0069DE10 = D_0069DE10 + (f32) phaseStepA;
            D_0069DE10 = angleWrap360(D_0069DE10);
            D_0069DE14 = D_0069DE14 + (f32) phaseStepB;
            D_0069DE14 = angleWrap360(D_0069DE14);
        }
        if (repeat > 0)
        {
            remaining = repeat;
            do
            {
                gsQueueBeginTriStrip((GsQueueContext *) grid);
                for (row = 0; row < divisions; row++)
                {
                    for (column = 0; column <= divisions; column++)
                    {
                        height0 = quadOrbitPoint(&point[0], &center[0], left + column * stepX, top + row * stepY, grid);
                        height1 = quadOrbitPoint(&point[1], &center[1], left + column * stepX, top + (row + 1) * stepY, grid);
                        if (column == 0)
                        {
                            if (colorB != 0)
                            {
                                gsQueueFadeToColorPhases(packet, colorB, (GsQueueContext *) grid, height0);
                            }
                            gsQueueAddTexturedVertex(packet, (float *) &point[0], (float *) &center[0], grid);
                            if (colorB != 0)
                            {
                                gsQueueFadeToColorPhases(packet, colorB, (GsQueueContext *) grid, height1);
                            }
                            gsQueueAddTexturedVertex(packet, (float *) &point[1], (float *) &center[1], grid);
                        }
                        else
                        {
                            if (colorB != 0)
                            {
                                gsQueueFadeToColorPhases(packet, colorB, (GsQueueContext *) grid, height0);
                            }
                            gsQueueAddTexturedKickVertex(packet, (float *) &point[0], (float *) &center[0], grid);
                            if (colorB != 0)
                            {
                                gsQueueFadeToColorPhases(packet, colorB, (GsQueueContext *) grid, height1);
                            }
                            gsQueueAddTexturedKickVertex(packet, (float *) &point[1], (float *) &center[1], grid);
                        }
                    }
                }
                gsQueueFlush(packet);
                remaining = remaining - 1;
            }
            while (remaining != 0);
        }
    }
}

/* Evaluates the orbit around (centerX, centerY): writes the position and height to position and the center to center. */
f32 quadOrbitPoint(Vector4 *position, Vector4 *center, int centerX, int centerY, QuadGrid *orbit)
{
    int height;
    int top;
    int left;
    int radiusX;
    int radiusY;
    f32 speedY;
    f32 speedX;
    f32 radiusXf;
    f32 radiusYf;
    f32 centerXf;
    f32 centerYf;
    int stepX;
    int stepY;
    f32 angleX;
    f32 angleY;
    f32 px;
    f32 py;
    f32 sumX;
    f32 sumY;

    height = orbit->height;
    top = orbit->top;
    left = orbit->left;
    radiusX = orbit->radiusX;
    speedY = (f32) (orbit->speedY * 0.1);
    radiusY = orbit->radiusY;
    speedX = (f32) (orbit->speedX * 0.1);
    radiusYf = (f32) radiusY;
    radiusXf = (f32) radiusX;
    centerXf = (f32) centerX;
    centerYf = (f32) centerY;
    stepY = (int) (D_0069DE10 + speedY * (f32) (centerX - left));
    stepX = (int) (D_0069DE14 + speedX * (f32) (centerY - top));
    angleX = (f32) stepX;
    angleY = (f32) stepY;
    position->x = centerXf + radiusXf * cosf(angleX * D_0069DE18);
    position->y = centerYf + radiusYf * cosf(angleY * D_0069DE18);
    position->z = (f32) height;
    center->x = centerXf;
    center->y = centerYf;
    sumY = radiusYf * cosf(angleY * D_0069DE18);
    sumX = radiusXf * cosf(angleX * D_0069DE18);
    return sumY + sumX;
}

/* Wraps an angle in degrees into the range 0 to 360. */
f32 angleWrap360(f32 angle) {
    if (angle > 360.0f) {
        angle -= 360.0f;
    }
    if (angle < 0.0f) {
        angle += 360.0f;
    }
    return angle;
}

/* Fades from the context color toward colorB by |t| / (phase1 + phase2) frames, then queues it. */
void gsQueueFadeToColorPhases(int packet, int colorB, GsQueueContext *context, f32 t) {
    int from = context->color;
    f32 divisor = (f32) (context->phase1_frames + context->phase2_frames);
    if (t > 0.0f) {
        int color = xglColorLerp(from, colorB, __builtin_fabsf(t / divisor));
        gsQueueAddColor(packet, color, context);
        return;
    }
    gsQueueAddContextColor(packet, context);
}

extern int D_0069DE1C;
int angleAdvance(int angle, int delta);
f32 ringWavePoint(Vector4 *wave, Vector4 *flat, int x, int y, RingWave *ring);
void gsQueueFadeToColor(int packet, int colorB, GsQueueContext *context, f32 t);

/* Draws the ring-wave grid repeat times: one triangle strip per row, each column adding the points of two grid rows (the first column without kick). */
void ringWaveGridDraw(RingWave *grid, int packet)
{
    Vector4 position[2];
    Vector4 flat[2];
    int pass;
    int divisions = grid->divisions;
    int repeat = grid->repeat;
    int colorB = grid->colorB;
    int top = grid->top;
    int left = grid->left;
    int right = grid->right;
    int bottom = grid->bottom;
    int angleStep = grid->angleStep;
    int stepX;
    int stepY;
    int row;
    int column;
    f32 height0;
    f32 height1;

    if (divisions > 0)
    {
        if (recordUpdatePaused == 0)
        {
            D_0069DE1C = angleAdvance(D_0069DE1C, angleStep);
        }
        stepY = (bottom - top) / divisions;
        stepX = (right - left) / divisions;
        if (repeat > 0)
        {
            pass = repeat;
            do
            {
                for (row = 0; row < divisions; row++)
                {
                    gsQueueBeginTriStrip((GsQueueContext *) grid);
                    for (column = 0; column <= divisions; column++)
                    {
                        height0 = ringWavePoint(&position[0], &flat[0], left + column * stepX, top + row * stepY, grid);
                        height1 = ringWavePoint(&position[1], &flat[1], left + column * stepX, top + (row + 1) * stepY, grid);
                        if (column == 0)
                        {
                            if (colorB != 0)
                            {
                                gsQueueFadeToColor(packet, colorB, (GsQueueContext *) grid, height0);
                            }
                            gsQueueAddTexturedVertex(packet, (float *) &position[0], (float *) &flat[0], grid);
                            if (colorB != 0)
                            {
                                gsQueueFadeToColor(packet, colorB, (GsQueueContext *) grid, height1);
                            }
                            gsQueueAddTexturedVertex(packet, (float *) &position[1], (float *) &flat[1], grid);
                        }
                        else
                        {
                            if (colorB != 0)
                            {
                                gsQueueFadeToColor(packet, colorB, (GsQueueContext *) grid, height0);
                            }
                            gsQueueAddTexturedKickVertex(packet, (float *) &position[0], (float *) &flat[0], grid);
                            if (colorB != 0)
                            {
                                gsQueueFadeToColor(packet, colorB, (GsQueueContext *) grid, height1);
                            }
                            gsQueueAddTexturedKickVertex(packet, (float *) &position[1], (float *) &flat[1], grid);
                        }
                    }
                    gsQueueFlush(packet);
                }
                pass = pass - 1;
            }
            while (pass != 0);
        }
    }
}

f32 func_0027A620(int sum, f32 angle);

int angleAdvance(int angle, int delta) {
    int sum;

    sum = angle + delta;
    return (int) func_0027A620(sum, (f32) sum);
}

extern f32 D_0069DE20;          /* degrees to radians */

/* Computes the ring-wave point for grid position (x, y): outside the ring between innerRadius and outerRadius
   (or for an empty ring) the point stays flat; inside it, y is lifted by amplitude * sin(global angle + phase),
   the phase growing from 0 at the inner edge to 180 * phaseScale at the outer edge. Writes the waved point to
   wave and the flat point to flat, and returns the lift. */
f32 ringWavePoint(Vector4 *wave, Vector4 *flat, int x, int y, RingWave *ring)
{
    EffectParams *params = (EffectParams *) ring;
    f32 offset;
    int centerX;
    int centerY;
    int innerRadius;
    int outerRadius;
    int phaseScale;
    int z;
    int amplitude;
    int innerSq;
    int outerSq;
    int deltaX;
    int deltaY;
    int squareX;
    int squareY;
    int distanceSq;

    /* The outer radius is read as a plain int word; the other words go through the float-aliased view. */
    outerRadius = ring->outerRadius;
    innerRadius = params->ring.innerRadius;
    centerX = params->ring.centerX;
    centerY = params->ring.centerY;
    z = params->ring.z;
    phaseScale = params->ring.phaseScale;
    amplitude = params->ring.amplitude;
    innerSq = innerRadius * innerRadius;
    outerSq = outerRadius * outerRadius;
    if (innerRadius == outerRadius)
    {
        return 0.0f;
    }
    deltaX = x - centerX;
    squareX = deltaX * deltaX;
    deltaY = y - centerY;
    squareY = deltaY * deltaY;
    distanceSq = squareX + squareY;
    if (distanceSq < innerSq || outerSq < distanceSq)
    {
        wave->x = (f32) x;
        wave->y = (f32) y;
        wave->z = (f32) z;
        flat->x = (f32) x;
        flat->y = (f32) y;
        return 0.0f;
    }
    {
        f32 flatX = (f32) x;
        f32 height = (f32) amplitude;
        f32 phase = ((f32) (distanceSq - innerSq) / (f32) (outerSq - innerSq)) * 180.0f * (f32) phaseScale;

        wave->x = flatX;
        offset = height * sinf(((f32) D_0069DE1C + phase) * D_0069DE20);
        wave->y = (f32) y - offset;
        wave->z = (f32) z;
        flat->x = flatX;
        flat->y = (f32) y;
    }
    return offset;
}

/* Same body as angleWrap360 above; the original binary holds a second copy that takes (and ignores) a leading int. */
f32 func_0027A620(int sum, f32 angle) {
    if (angle > 360.0f) {
        angle -= 360.0f;
    }
    if (angle < 0.0f) {
        angle += 360.0f;
    }
    return angle;
}

/* Fades from the context color toward colorB by |t| / fade_frames, then queues it. */
void gsQueueFadeToColor(int packet, int colorB, GsQueueContext *context, f32 t) {
    int from = context->color;
    int divisor = context->fade_frames;
    if (t > 0.0f) {
        int color = xglColorLerp(from, colorB, __builtin_fabsf(t / (f32) divisor));
        gsQueueAddColor(packet, color, context);
        return;
    }
    gsQueueAddContextColor(packet, context);
}
