#include "common.h"
#include "main/tu204.h"
#include "main/tu213.h"
#include "main/xgl_2.h"
#include "shared.h"
#include "vu0_macro.h"

/* The queue context read through a float-aliased view (the original scheduling of ringWavePoint depends on that). */
typedef union EffectParams {
    GsQueueContext context;
    f32 words[24];
} EffectParams;

void quadRotateCorner(Vector4 *destination, Vector4 *source, const float *rotation, int value, int x, int y);
void randomPointInRect(Vector4 *position, Vector4 *uv, int x, int y, GsQueueContext *rect);
void randomRefreshSeed(void);

/* Transforms the four corners around corner 0 plus the screen offset and queues the quad count times. */
void quadDrawTransformed(XglPacket *packet, Vector4 *corners, GsQueueContext *transform, const float *rotation)
{
    Vector4 quad[4];
    int x;
    int y;
    int value;
    int count;
    int remaining;

    x = (int) (corners[0].x + (float) transform->kind.transform.offset_x);
    y = (int) (corners[0].y + (float) transform->kind.transform.offset_y);
    value = transform->z;
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

void quadDrawRepeated(XglPacket *packet, Vector4 *corners, GsQueueContext *batch)
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

f32 intLerp(f32 from, f32 to, f32 fraction);
void *memset(void *dest, int value, unsigned int count);

/* Scales a point's squared distance from the centre into a fraction, lerps three channels by it and
 * returns 0, 1 or 2 when the channels pass their limits (cleared on 0 and 2). */
int pointFadeColorStep(f32 *out, const GsQueueContext *params, f32 x, f32 y) {
    int centerY = params->kind.arc.center_y;
    int radius = params->kind.arc.radius;
    int centerX = params->kind.arc.center_x;
    int invert = params->kind.arc.invert;
    int fromB = params->kind.arc.from_b;
    int fromA = params->kind.arc.from_a;
    int fromC = params->kind.arc.from_c;
    int toA = params->kind.arc.to_a;
    int toB = params->kind.arc.to_b;
    int toC = params->kind.arc.to_c;
    int limitA = params->kind.arc.limit_a;
    int limitB = params->kind.arc.limit_b;
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

extern f32 degreesToRadians;
void xglMatrixUnit(Matrix4 matrix);
void xglMatrixRotZ(Matrix4 destination, Matrix4 source, f32 angle);
void xglMatrixRotX(Matrix4 destination, Matrix4 source, f32 angle);
void xglMatrixMul(Matrix4 destination, Matrix4 first, Matrix4 second);

/* Rotates source by the three Euler angles (degrees) around the point (x, y) and sets the depth from value. */
void quadRotateCorner(Vector4 *destination, Vector4 *source, const float *rotation, int value, int x, int y)
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
    xglMatrixRotX(rotX, rotX, angles[0] * degreesToRadians);
    xglMatrixRotY(rotY, rotY, angles[1] * degreesToRadians);
    xglMatrixRotZ(rotZ, rotZ, angles[2] * degreesToRadians);
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

extern f32 quadWobblePhase;          /* wobble phase (degrees), advanced by every quadDrawRotating call */
extern f32 kDegToRadQuad;          /* degrees-to-radians factor used for the wobble phase */
f32 cosf(f32 x);
f32 sinf(f32 x);
void quadRotateAroundCenter(Vector4 *vectors, f32 angle, f32 centerX, f32 centerY);

/* Builds the wobbling, rotated quad from quad (the phase advances every call) and queues it count times. */
void quadDrawRotating(GsQueueContext *quad, XglPacket *packet)
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
    f32 toRad = kDegToRadQuad;
    f32 amplitudeX;
    f32 amplitudeY;
    f32 offsetX;
    f32 offsetY;
    f32 leftX;
    f32 topY;
    f32 rightX;
    f32 bottomY;
    int remaining;
    wobbleX = quad->kind.quad.wobble_x;
    z = quad->z;
    count = quad->count;
    top = quad->kind.quad.top;
    left = quad->kind.quad.left;
    quadWobblePhase = quadWobblePhase + (f32) quad->kind.quad.phase_step;
    right = quad->kind.quad.right;
    bottom = quad->kind.quad.bottom;
    v0 = quad->kind.quad.uv_top;
    u0 = quad->kind.quad.uv_left;
    u1 = quad->kind.quad.uv_right;
    v1 = quad->kind.quad.uv_bottom;
    margin = quad->kind.quad.margin;
    wobbleY = quad->kind.quad.wobble_y;
    angle = quad->kind.quad.angle;
    amplitudeX = (f32) wobbleX;
    pivotX = quad->kind.quad.pivot_x;
    pivotY = quad->kind.quad.pivot_y;
    offsetX = amplitudeX * cosf(quadWobblePhase * toRad);
    amplitudeY = (f32) wobbleY;
    offsetY = amplitudeY * sinf(quadWobblePhase * toRad);
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
    gsQueueBeginTriStrip(quad);
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

/* Queues the grid of randomly displaced points repeat times as triangle strips: row by row, each column adds the points of two grid rows (the first column without kick).
   points[0] holds the current grid row and points[1] the next one; the next row is reused as the current row of the following pass. */
void pointGridDraw(GsQueueContext *scatter, XglPacket *packet)
{
    Vector4 points[2][30];
    Vector4 currentUv;
    Vector4 nextUv;
    int column;
    int remaining;
    int divisions = scatter->kind.scatter.divisions;
    int repeat = scatter->count;
    int top = scatter->kind.scatter.top;
    int left = scatter->kind.scatter.left;
    int right = scatter->kind.scatter.right;
    int bottom = scatter->kind.scatter.bottom;
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
                gsQueueBeginTriStrip(scatter);
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

/* Writes the grid point (x, y) displaced by a seeded random amount below countX/countY plus the offsets,
   with the block depth as z, and stores the undisplaced point as its uv. */
void randomPointInRect(Vector4 *position, Vector4 *uv, int x, int y, GsQueueContext *rect)
{
    int depth = rect->z;
    int countX = rect->kind.point.count_x;
    int offsetX = rect->kind.point.offset_x;
    int countY = rect->kind.point.count_y;
    int offsetY = rect->kind.point.offset_y;
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

f32 quadOrbitPoint(Vector4 *position, Vector4 *center, int centerX, int centerY, GsQueueContext *orbit);
f32 angleWrap360(f32 angle);
void gsQueueFadeToColorPhases(XglPacket *packet, int colorB, GsQueueContext *context, f32 t);

/* Queues the orbiting grid repeat times as triangle strips: row by row, each column adds the points of two grid rows (the first column without kick). */
void quadOrbitGridDraw(GsQueueContext *grid, XglPacket *packet)
{
    Vector4 point[2];
    Vector4 center[2];
    int remaining;
    int divisions = grid->kind.grid.divisions;
    int repeat = grid->count;
    int colorB = grid->kind.grid.color_b;
    int top = grid->kind.grid.top;
    int left = grid->kind.grid.left;
    int right = grid->kind.grid.right;
    int bottom = grid->kind.grid.bottom;
    int phaseStepA = grid->kind.grid.phase_step_a;
    int phaseStepB = grid->kind.grid.phase_step_b;
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
                gsQueueBeginTriStrip(grid);
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
                                gsQueueFadeToColorPhases(packet, colorB, grid, height0);
                            }
                            gsQueueAddTexturedVertex(packet, (float *) &point[0], (float *) &center[0], grid);
                            if (colorB != 0)
                            {
                                gsQueueFadeToColorPhases(packet, colorB, grid, height1);
                            }
                            gsQueueAddTexturedVertex(packet, (float *) &point[1], (float *) &center[1], grid);
                        }
                        else
                        {
                            if (colorB != 0)
                            {
                                gsQueueFadeToColorPhases(packet, colorB, grid, height0);
                            }
                            gsQueueAddTexturedKickVertex(packet, (float *) &point[0], (float *) &center[0], grid);
                            if (colorB != 0)
                            {
                                gsQueueFadeToColorPhases(packet, colorB, grid, height1);
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
f32 quadOrbitPoint(Vector4 *position, Vector4 *center, int centerX, int centerY, GsQueueContext *orbit)
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

    height = orbit->z;
    top = orbit->kind.grid.top;
    left = orbit->kind.grid.left;
    radiusX = orbit->kind.grid.radius_x;
    speedY = (f32) (orbit->kind.grid.speed_y * 0.1);
    radiusY = orbit->kind.grid.radius_y;
    speedX = (f32) (orbit->kind.grid.speed_x * 0.1);
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
void gsQueueFadeToColorPhases(XglPacket *packet, int colorB, GsQueueContext *context, f32 t) {
    int from = context->color;
    f32 divisor = (f32) (context->kind.fade.phase1_frames + context->kind.fade.phase2_frames);
    if (t > 0.0f) {
        int color = xglColorLerp(from, colorB, __builtin_fabsf(t / divisor));
        gsQueueAddColor(packet, color, context);
        return;
    }
    gsQueueAddContextColor(packet, context);
}

extern int D_0069DE1C;
int angleAdvance(int angle, int delta);
f32 ringWavePoint(Vector4 *wave, Vector4 *flat, int x, int y, GsQueueContext *ring);
void gsQueueFadeToColor(XglPacket *packet, int colorB, GsQueueContext *context, f32 t);

/* Draws the ring-wave grid repeat times: one triangle strip per row, each column adding the points of two grid rows (the first column without kick). */
void ringWaveGridDraw(GsQueueContext *grid, XglPacket *packet)
{
    Vector4 position[2];
    Vector4 flat[2];
    int pass;
    int divisions = grid->kind.ring.divisions;
    int repeat = grid->count;
    int colorB = grid->kind.ring.color_b;
    int top = grid->kind.ring.top;
    int left = grid->kind.ring.left;
    int right = grid->kind.ring.right;
    int bottom = grid->kind.ring.bottom;
    int angleStep = grid->kind.ring.angle_step;
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
                    gsQueueBeginTriStrip(grid);
                    for (column = 0; column <= divisions; column++)
                    {
                        height0 = ringWavePoint(&position[0], &flat[0], left + column * stepX, top + row * stepY, grid);
                        height1 = ringWavePoint(&position[1], &flat[1], left + column * stepX, top + (row + 1) * stepY, grid);
                        if (column == 0)
                        {
                            if (colorB != 0)
                            {
                                gsQueueFadeToColor(packet, colorB, grid, height0);
                            }
                            gsQueueAddTexturedVertex(packet, (float *) &position[0], (float *) &flat[0], grid);
                            if (colorB != 0)
                            {
                                gsQueueFadeToColor(packet, colorB, grid, height1);
                            }
                            gsQueueAddTexturedVertex(packet, (float *) &position[1], (float *) &flat[1], grid);
                        }
                        else
                        {
                            if (colorB != 0)
                            {
                                gsQueueFadeToColor(packet, colorB, grid, height0);
                            }
                            gsQueueAddTexturedKickVertex(packet, (float *) &position[0], (float *) &flat[0], grid);
                            if (colorB != 0)
                            {
                                gsQueueFadeToColor(packet, colorB, grid, height1);
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
f32 ringWavePoint(Vector4 *wave, Vector4 *flat, int x, int y, GsQueueContext *ring)
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
    innerRadius = params->context.kind.ring.inner_radius;
    outerRadius = ring->kind.ring.outer_radius;
    centerX = params->context.kind.ring.center_x;
    centerY = params->context.kind.ring.center_y;
    z = params->context.z;
    phaseScale = params->context.kind.ring.phase_scale;
    amplitude = params->context.kind.ring.amplitude;
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
void gsQueueFadeToColor(XglPacket *packet, int colorB, GsQueueContext *context, f32 t) {
    int from = context->color;
    int divisor = context->kind.fade.fade_frames;
    if (t > 0.0f) {
        int color = xglColorLerp(from, colorB, __builtin_fabsf(t / (f32) divisor));
        gsQueueAddColor(packet, color, context);
        return;
    }
    gsQueueAddContextColor(packet, context);
}
