#include "common.h"
#include "shared.h"
#include "main/get.h"
#include "main/tu051.h"
#include "main/xgl_2.h"

extern float sinf(float angle);
extern float cosf(float angle);
extern float atan2f(float y, float x);
int CheckPointLine(const Vector4 *first, const Vector4 *second,
                   const Vector4 *point);

/* GetBSplineLoop: DataSpline holds the weight table (byte offset) at 2048 phases per control-point period; the sum is divided by 6 * 4096. */
#define GET_BSPLINE_PHASES 2048
#define GET_BSPLINE_WEIGHTS_OFFSET 4304
#define GET_BSPLINE_WEIGHT_SCALE 24576.0f
/* Half and full turn of the angle values used by the Get_/unduCheck angle helpers. */
#define GET_PI 3.1415927f
#define GET_TWO_PI 6.2831855f

typedef struct ParabolaVec { float x, y, z, w; } ParabolaVec;

typedef struct Vector3 { float x, y, z; } Vector3;

float Get_Angle_Relative(const Vector4 *first, const Vector4 *second,
                         float reference);
/* Functions and data used by other translation units. */
extern unsigned short *DataSpline;
void Get_Point_By_AngleLength(const Vector4 *source, Vector4 *destination,
                              float angle, float length);
float Get_Distance(const Vector4 *first, const Vector4 *second);
int Check_CrossingOver(const Vector4 *first_start, const Vector4 *first_end,
                       const Vector4 *second_start, const Vector4 *second_end);
float Get_Angle(const Vector4 *first, const Vector4 *second);
void BSpline_Add(short frame, short count, short period,
                 Vector4 *destination, const Vector3 *source, short offset);
void Get_One_Step(float length, const Vector4 *source,
                  const Vector4 *destination, Vector4 *output);
float Get_Decimal_Surplus_for_Radius(float angle);
float Get_Distance3D(const Vector4 *first, const Vector4 *second);
float Get_Multi_Max_Under(float value, float step, float maximum);
void Get_MiddlePoint_Parabora(const ParabolaVec *first, const ParabolaVec *second,
                              int current, int total, float height,
                              ParabolaVec *destination);

float Get_Decimal_Surplus_for_Radius(float angle)
{
    if (angle <= 0.0f) {
        float negative_pi = -GET_PI;
        if (angle < negative_pi) {
            do {
                angle += GET_TWO_PI;
            } while (angle < negative_pi);
        }
    } else {
        while (GET_PI <= angle) {
            angle -= GET_TWO_PI;
        }
    }
    return angle;
}

#include "main/xgl_font.h"

/* Debug HUD: prints "%2d(%3d)" from the stick angle (PadData bytes 0x64/0x65 through atan2f). */
extern int padDebugSector;
extern int padDebugCount;
extern signed char PadData[];
extern const char padDebugDirectionFormat[];

int padDebugPrintDirection(int sector_count)
{
    float angle = GET_PI - atan2f(PadData[0x64], PadData[0x65]);
    float count = (float)sector_count;
    int sector = padDebugSector;

    if ((float)(sector + 1) * GET_TWO_PI / count <= angle
        && angle <= (float)(sector + 2) * GET_TWO_PI / count) {
        sector = (sector + 1) % (sector_count - 1);
        padDebugSector = sector;
        if (sector == 0)
            padDebugCount++;
    } else if (!((float)padDebugSector * GET_TWO_PI / count <= angle
                 && angle <= (float)(padDebugSector + 1) * GET_TWO_PI / count)) {
        sector = 0;
        padDebugSector = 0;
    }
    xglFontDebugPrintf(192, 10, padDebugDirectionFormat, padDebugSector, padDebugCount);
    return padDebugCount;
}

/* Debug move-square check: same strings and corner offsets as X1 Check_MoveSquare. */
extern const char moveSquarePlaceholderText[];
extern const char moveSquareForecastText[];
/* Sweeps from start toward end through the Undu query block; returns 1 when the sweep end is within 0.01 of end. */
int unduCheckSweep(int slotIndex, const Vector4 *from, const Vector4 *to, int map_block, int bits, Vector4 *hit, int attr_mask);
int uwamonoCrossPoint(const Vector4 *origin, const Vector4 *target, float radius, Vector4 *out);
void func_00202E60(const Vector4 *first, const Vector4 *second, int mode);

int unduCheckMoveSquare(const Vector4 *first, const Vector4 *second,
                        Vector4 *destination, int map_index, float radius)
{
    Vector4 first_intersection;
    Vector4 first_crossing;
    Vector4 second_intersection;
    Vector4 second_crossing;
    Vector4 third_intersection;
    Vector4 third_crossing;
    Vector4 fourth_intersection;
    Vector4 fourth_crossing;
    Vector4 first_corner;
    Vector4 second_corner;
    union {
        int debug_color[4];
        float debug_color_alias[4];
    } dbg;
    int crossed;

    dbg.debug_color[0] = 0;
    dbg.debug_color[1] = 0;
    dbg.debug_color[2] = 255;
    crossed = 0;
    dbg.debug_color[3] = 128;
    first_corner.x = first->x;
    first_corner.y = second->y;
    first_corner.z = second->z;
    second_corner.x = second->x;
    second_corner.y = second->y;
    second_corner.z = first->z;
    {
        float adjusted_x;
        float z_delta;
        if ((first->x - second->x) < 0.0f) {
            adjusted_x = first->x + -0.1f;
        } else {
            adjusted_x = 0.1f + first->x;
        }
        z_delta = first->z - second->z;
        first_corner.x = adjusted_x;
        {
            float corner_z = second_corner.z;
            float adjusted_z;
            if (z_delta < 0.0f) {
                adjusted_z = corner_z + -0.1f;
            } else {
                adjusted_z = corner_z + 0.1f;
            }
            second_corner.z = adjusted_z;
        }
    }

    unduCheckSweep(-1, second, &first_corner, map_index, -1,
                   &first_intersection, 0);
    unduCheckSweep(-1, second, &second_corner, map_index, -1,
                   &second_intersection, 0);
    uwamonoCrossPoint(second, &first_intersection, radius, &first_crossing);
    uwamonoCrossPoint(second, &second_intersection, radius, &second_crossing);
    first_crossing.w = 0.1f;
    second_crossing.w = 0.1f;
    first_intersection = first_crossing;
    second_intersection = second_crossing;

    first_corner.x = second->x;
    first_corner.y = first->y;
    first_corner.z = first->z;
    second_corner.x = first->x;
    second_corner.y = first->y;
    second_corner.z = second->z;
    first_corner.x = (second->x - first->x) < 0.0f
        ? first_corner.x + -0.1f : first_corner.x + 0.1f;
    second_corner.z += (second->z - first->z) < 0.0f
        ? -0.1f : 0.1f;

    unduCheckSweep(-1, first, &first_corner, map_index, -1,
                   &third_intersection, 0);
    unduCheckSweep(-1, first, &second_corner, map_index, -1,
                   &fourth_intersection, 0);
    uwamonoCrossPoint(first, &third_intersection, radius, &third_crossing);
    uwamonoCrossPoint(first, &fourth_intersection, radius, &fourth_crossing);
    third_crossing.w = 0.1f;
    fourth_crossing.w = 0.1f;
    third_intersection = third_crossing;
    fourth_intersection = fourth_crossing;

    xglFontDebugPrintf(8, 100, moveSquarePlaceholderText);
    if (Check_CrossingOver(second, &first_intersection, first,
                           &fourth_intersection) == 1) {
        *destination = first_intersection;
        destination->z = second->z;
        crossed = 1;
        xglFontDebugPrintf(24, 100, moveSquareForecastText);
    }
    if (Check_CrossingOver(second, &second_intersection, first,
                           &third_intersection) == 1) {
        *destination = second_intersection;
        destination->x = second->x;
        crossed = 1;
        xglFontDebugPrintf(24, 100, moveSquareForecastText);
    }

    dbg.debug_color[0] = 255;
    dbg.debug_color[1] = 0;
    dbg.debug_color[2] = 0;
    dbg.debug_color[3] = 128;
    func_00202E60(second, &first_intersection, 1);
    func_00202E60(second, &second_intersection, 1);
    func_00202E60(first, &third_intersection, 1);
    func_00202E60(first, &fourth_intersection, 1);
    return crossed;
}

void Get_Point_By_AngleLength(const Vector4 *source, Vector4 *destination,
                              float angle, float length)
{
    destination->x = source->x + sinf(angle) * length;
    destination->y = source->y;
    destination->z = source->z + cosf(angle) * length;
    destination->w = source->w;
}

#include "main/tu051.h"


/* Returns 1.0 when the 1-D spans [0x14, 0x14 + 0x584) of uwamonoRecords entries a and b overlap, else 0.0. */
float uwamonoRecordsOverlap(int a, int b)
{
    float firstEnd = uwamonoRecords[a].position.y + uwamonoRecords[a].extent.y;
    float secondEnd = uwamonoRecords[b].position.y + uwamonoRecords[b].extent.y;

    if (firstEnd < uwamonoRecords[b].position.y) {
        return 0.0f;
    }
    if (secondEnd < uwamonoRecords[a].position.y) {
        return 0.0f;
    }
    return 1.0f;
}

void Get_MiddlePoint(const Vector4 *start, const Vector4 *following,
                     short point_index, short point_count, Vector4 *result)
{
    if (point_index == point_count) {
        result->x = following->x;
        result->y = following->y;
        result->z = following->z;
        result->w = 1.0f;
    } else {
        result->x = start->x + (following->x - start->x) * (float)point_index
                    / (float)point_count;
        result->y = start->y + (following->y - start->y) * (float)point_index
                    / (float)point_count;
        result->z = start->z + (following->z - start->z) * (float)point_index
                    / (float)point_count;
        result->w = 1.0f;
    }
}

/* Weighted blend of two points: the running weight sums select the sample at which the blend of
   from and to is taken; the output point has w = 1. */
void Get_MiddlePoint_Curve(const Vector4 *start, const Vector4 *following, short point_index, short point_count,
                           const float *curve_weights, const float *curve_values, short sample_count, Vector4 *result)
{
    int span_start = 0;
    float sum_product = 0.0f;
    float part_product = 0.0f;
    float sum_weight = 0.0f;
    float part_weight = 0.0f;
    float prev_ratio = 0.0f;
    float blend = 0.0f;
    float ratio;
    int k;
    int sample;
    int total_weight;
    int reached_from = 0;

    if (point_index == 0) {
        result->x = start->x;
        result->y = start->y;
        result->z = start->z;
        result->w = 1.0f;
        return;
    }
    if (point_index == point_count) {
        result->x = following->x;
        result->y = following->y;
        result->z = following->z;
        result->w = 1.0f;
        return;
    }
    for (k = 0; k < sample_count; k++) {
        sum_weight += curve_weights[k];
        sum_product += curve_weights[k] * curve_values[k];
    }
    for (k = 0; k < sample_count; k++) {
        total_weight = (int)sum_weight;
        part_weight += curve_weights[k];
        part_product += curve_weights[k] * curve_values[k];
        sample = span_start;
        ratio = part_product / sum_product;
        /* Steps sample over the samples of this span. The blend and the point_index test sit in the
           loop condition (the original's shape: its first test is copied ahead of the loop with
           sample == span_start, and the span limit is recomputed on every step). */
        while (sample < (point_count * (int)part_weight) / total_weight
               && (blend = prev_ratio
                           + (ratio - prev_ratio) * (float)(sample - span_start)
                             / ((float)point_count * part_weight / sum_weight - (float)span_start),
                   sample != point_index || (reached_from = 1, 0)))
            sample++;
        if (reached_from == 1)
            break;
        prev_ratio = ratio;
        span_start = sample;
    }
    result->x = start->x + (following->x - start->x) * blend;
    result->y = start->y + (following->y - start->y) * blend;
    result->z = start->z + (following->z - start->z) * blend;
    result->w = 1.0f;
}

void Get_MiddlePoint_Parabora(const ParabolaVec *first, const ParabolaVec *second,
                              int current, int total, float height,
                              ParabolaVec *destination)
{
    int current_value = (short)current;
    int total_value = (short)total;
    int midpoint = (int)((short)total +
                         ((unsigned int)(total << 16) >> 31)) >> 1;
    int delta = midpoint - current_value;
    float adjusted_height = height * (float)(delta * delta)
                           / (float)(midpoint * midpoint);
    height -= adjusted_height;

    if (current_value == total_value) {
        destination->x = second->x;
        destination->y = second->y;
        destination->z = second->z;
        destination->w = second->w;
    } else {
        destination->x = first->x + (second->x - first->x)
                       * (float)current_value / (float)total_value;
        destination->y = first->y + (second->y - first->y)
                       * (float)current_value / (float)total_value + height;
        destination->z = first->z + (second->z - first->z)
                       * (float)current_value / (float)total_value;
        destination->w = first->w + (second->w - first->w)
                       * (float)current_value / (float)total_value;
    }
}

extern UnduParam unduTest;
extern u64 eneTerritoryID[];
int HitCheckMapUnitPosSize(const Vector4 *position, float radius);

/* Wall probes around a point: one probe per step on the ring of the radius; when steps <= 0 a random
   start index is probed alone and the flags are cleared first. */
void Check_Wall(const Vector4 *start, char *flags, short steps, int unused,
                int unused_arg, int territory_index, float radius, float span)
{
    short first_index;
    short index;
    short end_index;

    first_index = 0;
    end_index = steps;
    if (steps <= 0) {
        first_index = xglSRand() % (steps < 0 ? -steps : steps);
        end_index = first_index + 1;
        for (index = 0; index < (steps < 0 ? -steps : steps); index++)
            flags[index] = 0;
    }
    {
        Vector4 probe;
        float angle;

        probe.w = 0.0f;
        for (index = first_index; index < end_index; index++) {
            angle = (float)index * GET_TWO_PI / (float)steps;
            probe.x = start->x + sinf(angle) * radius;
            probe.y = start->y;
            probe.z = start->z + cosf(angle) * radius;
            flags[index] = 0;
            if (unduGetHeightAttr(&probe, unused, 0, &unduTest) == 0.0f) {
                flags[index] = 1;
            } else if (HitCheckMapUnitPosSize(&probe, span) != -1) {
                flags[index] = 1;
            } else if (territory_index != 0 && (unsigned int)unduTest.attribute != eneTerritoryID[territory_index]) {
                flags[index] = 1;
            }
        }
    }
}

void Get_One_Step(float length, const Vector4 *source,
                  const Vector4 *destination, Vector4 *output)
{
    float x_step = destination->x - source->x;
    float z_step = destination->z - source->z;
    float inverse_distance = 1.0f / Get_Distance(source, destination);

    output->x = x_step * inverse_distance * length;
    output->y = 0.0f;
    output->z = z_step * inverse_distance * length;
    output->w = 1.0f;
}

/* Straight-line probe: Check_Undu sweep first, then per-step territory, wall and actor tests (X1 Check_Straight flow). */
int uwamonoFindReadyInReach(const Vector4 *position, int ready);

short unduCheckStraight(const Vector4 *first, const Vector4 *second, short steps,
                      Vector4 *destination, int map_index, int ready,
                      short territory_index, float radius)
{
    DoublewordAlignedVector4 cur;
    DoublewordAlignedVector4 prev;
    DoublewordAlignedVector4 sweep_hit;
    short step;

    prev = *first;
    if (unduCheckSweep(-1, first, second, map_index, -1, &sweep_hit, 0) == 0) {
        *destination = sweep_hit;
        return 0;
    }
    step = 0;
    if (steps >= 0) {
        for (; step <= steps; step++) {
            cur.x = first->x + (second->x - first->x) * (float)step / (float)steps;
            cur.y = first->y + (second->y - first->y) * (float)step / (float)steps;
            cur.z = first->z + (second->z - first->z) * (float)step / (float)steps;
            if ((territory_index != 0 && unduGetAttr(&cur, map_index, 0) != eneTerritoryID[territory_index])
                || HitCheckMapUnitPosSize(&cur, radius) != -1
                || uwamonoFindReadyInReach(&cur, ready) == 1) {
                *destination = prev;
                return step - 1;
            }
            prev = cur;
        }
    }
    *destination = *second;
    return steps;
}

extern UnduParam D_003E7BB8;
void mapProbeInit(UnduParam *request);
int mapFindBlockEntry(int block, int unused);
float mapMoveProbe(const Vector4 *from, const Vector4 *delta, UnduParam *request);
int HitCheckMapUnitPos(const Vector4 *pos);

int Check_Straight_ID(const Vector4 *start, const Vector4 *end, short steps, Vector4 *result, int block)
{
    short index;
    Vector4 pos;
    int entry;
    int last_step;
    int table_block;
    Vector4 cur;
    int found;

    D_003E7BB8.header = mapFindBlockEntry(block, -1);
    index = 0;
    cur.x = start->x;
    cur.y = start->y;
    cur.z = start->z;
    last_step = steps;
    if (last_step >= 0) {
        do {
            pos.x = start->x + (end->x - start->x) * (float)index / steps;
            pos.y = start->y + (end->y - start->y) * (float)index / steps;
            pos.z = start->z + (end->z - start->z) * (float)index / steps;
            if (mapMoveProbe(&pos, 0, &D_003E7BB8) == -1000.0f || HitCheckMapUnitPos(&pos) != -1) {
                result->x = cur.x;
                result->y = cur.y;
                result->z = cur.z;
                return (short)(index - 1);
            }
            index++;
            cur.x = pos.x;
            cur.y = pos.y;
            cur.z = pos.z;
        } while (index <= last_step);
    }
    result->x = end->x;
    result->y = end->y;
    result->z = end->z;
    return steps;
}

float Get_Distance(const Vector4 *first, const Vector4 *second)
{
    float first_x = first->x;
    float first_z = first->z;

    return __builtin_sqrtf((second->x - first_x) * (second->x - first_x)
                           + (second->z - first_z) * (second->z - first_z));
}

float Get_Distance3D(const Vector4 *first, const Vector4 *second)
{
    float first_x = first->x;
    float first_y = first->y;
    float first_z = first->z;

    return __builtin_sqrtf((second->x - first_x) * (second->x - first_x)
                           + (second->y - first_y) * (second->y - first_y)
                           + (second->z - first_z) * (second->z - first_z));
}

float Get_Angle(const Vector4 *first, const Vector4 *second)
{
    return atan2f(second->x - first->x, second->z - first->z);
}

/* Three Get_Decimal_Surplus_for_Radius normalisations and the range test of X1 Check_Angle, after a span shortcut. */
int unduCheckAngle(float angle, float from, float to)
{
    double span = to - from;
    int inside;

    if (span < 0.0)
        span = -span;
    if (GET_TWO_PI <= span)
        return 1;

    from = Get_Decimal_Surplus_for_Radius(from);
    to = Get_Decimal_Surplus_for_Radius(to);
    angle = Get_Decimal_Surplus_for_Radius(angle);
    if (from <= to) {
        if (from <= angle)
            return angle <= to;
        return 0;
    }
    if (from <= angle)
        return 1;
    inside = 0;
    if (angle <= to)
        inside = 1;
    return inside;
}

float Get_Angle_Relative(const Vector4 *first, const Vector4 *second,
                         float reference)
{
    float relative = Get_Angle(first, second) - reference;

    if (relative < -GET_PI)
        relative += GET_TWO_PI;
    if (GET_PI < relative)
        relative -= GET_TWO_PI;
    return relative;
}

extern float fanMinDistance;
extern float fanHeightTolerance;
extern float fanMaxHeightDelta;

/* True when the origin sees the midpoint of each edge of the quad (corner0..corner3) without that
   ray crossing another edge: the four midpoints are tested against the three other edges. */
int uwamonoQuadEdgesVisible(const Vector4 *origin, const Vector4 *corner0, const Vector4 *corner1,
                  const Vector4 *corner2, const Vector4 *corner3)
{
    Vector4 edge_midpoints[4];

    Get_MiddlePoint(corner0, corner1, 1, 2, &edge_midpoints[0]);
    Get_MiddlePoint(corner1, corner2, 1, 2, &edge_midpoints[1]);
    Get_MiddlePoint(corner2, corner3, 1, 2, &edge_midpoints[2]);
    Get_MiddlePoint(corner3, corner0, 1, 2, &edge_midpoints[3]);

    if (Check_CrossingOver(origin, &edge_midpoints[0], corner1, corner2) == 1)
        return 0;
    if (Check_CrossingOver(origin, &edge_midpoints[0], corner2, corner3) == 1)
        return 0;
    if (Check_CrossingOver(origin, &edge_midpoints[0], corner3, corner0) == 1)
        return 0;
    if (Check_CrossingOver(origin, &edge_midpoints[1], corner2, corner3) == 1)
        return 0;
    if (Check_CrossingOver(origin, &edge_midpoints[1], corner3, corner0) == 1)
        return 0;
    if (Check_CrossingOver(origin, &edge_midpoints[1], corner0, corner1) == 1)
        return 0;
    if (Check_CrossingOver(origin, &edge_midpoints[2], corner3, corner0) == 1)
        return 0;
    if (Check_CrossingOver(origin, &edge_midpoints[2], corner0, corner1) == 1)
        return 0;
    if (Check_CrossingOver(origin, &edge_midpoints[2], corner1, corner2) == 1)
        return 0;
    if (Check_CrossingOver(origin, &edge_midpoints[3], corner0, corner1) == 1)
        return 0;
    if (Check_CrossingOver(origin, &edge_midpoints[3], corner1, corner2) == 1)
        return 0;
    return Check_CrossingOver(origin, &edge_midpoints[3], corner2, corner3) != 1;
}

int Check_CrossingOver(const Vector4 *first_start, const Vector4 *first_end,
                       const Vector4 *second_start, const Vector4 *second_end)
{
    int first_start_side = CheckPointLine(first_start, first_end, second_start);
    int first_end_side = CheckPointLine(first_start, first_end, second_end);

    if (first_start_side != first_end_side) {
        int second_start_side = CheckPointLine(second_start, second_end, first_start);
        int second_end_side = CheckPointLine(second_start, second_end, first_end);

        if (second_start_side != second_end_side)
            return 1;
    }
    return 0;
}

/* Field-of-view test of a target from origin: range, angle sector, map sweep and height limits (X1 Check_InsideFan). */
int unduCheckInsideFan(int slotIndex, const Vector4 *origin, float centerAngle,
                       float range, const Vector4 *target, int mask,
                       float fanWidth, float radius)
{
    Vector4 hit;
    int inside;

    if (range < Get_Distance3D(origin, target))
        return 0;
    if (!unduCheckAngle(Get_Angle(origin, target), centerAngle - fanWidth * 0.5f,
                        centerAngle + fanWidth * 0.5f))
        return 0;
    if (mask == -1 || slotIndex == -1)
        return 1;
    if (uwamonoCrossPoint(origin, target, radius, &hit) == 1)
        return 0;
    inside = unduCheckSweep(slotIndex, origin, target, mask, -1, &hit, 0);
    if (inside == 0)
        return inside;
    if (fanMinDistance <= Get_Distance(target, &hit)
        || fanHeightTolerance <= __builtin_fabsf(target->y - hit.y))
        return 0;
    if (fanMaxHeightDelta < __builtin_fabsf(origin->y - target->y))
        return 0;
    return 1;
}

/* Angle cursor that moves toward a target by the shorter way round, divided by speed (same body as X1). */
float cursolGetByReduceSpeedAngleLoop(float current, float target, float speed)
{
    float delta;

    if (current == target)
        return current;

    delta = target - current;
    if (__builtin_fabsf(delta) < GET_PI) {
        current += delta / speed;
    } else if (delta < -GET_PI) {
        current += (delta + GET_TWO_PI) / speed;
    } else {
        current -= (GET_TWO_PI - delta) / speed;
    }
    if (current <= -GET_PI)
        current += GET_TWO_PI;
    if (GET_PI <= current)
        current -= GET_TWO_PI;
    return current;
}

extern signed char pxyTickScaleDouble;

/* Nearest reachable ground point among eight points at radius 2 around the origin (X1 Get_EnemyIDPos flow). */
void unduGetEnemyIDPos(const Vector4 *origin, Vector4 *destination)
{
    Vector4 ring;
    Vector4 hit;
    Vector4 cross;
    int probeRange[4];
    float nearest = 1000.0f;
    float probeHeight;
    short i;

    *destination = *origin;
    probeRange[0] = 255;
    probeRange[1] = 127;
    probeRange[3] = 127;
    probeRange[2] = 0;
    mapProbeInit(&D_003E7BB8);
    D_003E7BB8.query_flags = 0;
    D_003E7BB8.attr_mask = 3;
    D_003E7BB8.header = mapFindBlockEntry(2048, 0x8000);
    probeHeight = mapMoveProbe(origin, 0, &D_003E7BB8);
    if (probeHeight != -1000.0f) {
        destination->x = origin->x;
        destination->y = origin->y;
        destination->z = origin->z;
    } else {
        for (i = 0; i < 8; i++) {
            do {
            float angle = (float) i * GET_TWO_PI * 0.125f;
            float distance;

            ring.x = origin->x + sinf(angle) * 2.0f;
            ring.y = origin->y;
            ring.z = origin->z + cosf(angle) * 2.0f;
            ring.w = 1.0f;
            unduCheckSweep(-1, &ring, origin, 2048, -1, &hit, 0);
            uwamonoCrossPoint(&ring, origin, 1.0f, &cross);
            if (pxyTickScaleDouble != 0)
                func_00202E60(&ring, &hit, 3);
            distance = Get_Distance(origin, &hit);
            if (distance != 0.0f && distance < nearest) {
                nearest = distance;
                destination->x = hit.x;
                destination->y = hit.y;
                destination->z = hit.z;
            }
            } while (0);
        }
    }
    destination->w = 1.0f;
}

float Get_Multi_Max_Under(float value, float step, float maximum)
{
    if (value < maximum) {
        maximum -= step;
        while (value <= maximum)
            value += step;
    } else {
        maximum -= step;
        while (!(value <= maximum))
            value -= step;
    }
    return value;
}

/* Height at a position under a query mode, with the parameter block the caller supplies. */
float unduGetHeightAttr(const Vector4 *position, int unused, int attr_mask, UnduParam *request)
{
    mapProbeInit(request);
    request->query_flags = 0;
    request->attr_mask = (short) attr_mask;
    return mapMoveProbe(position, 0, request);
}

typedef struct UwamonoSlotEntry {
    struct XenoActor *actor;
    unsigned char unmodeled_04[0x870 - 0x04];
} UwamonoSlotEntry;

extern UwamonoSlotEntry uwamonoSlotTable[];

float unduGetSlotHeight(int map_index, const Vector4 *position, int unused, int map_key, int attr_mask)
{
    Vector4 origin = *position;
    struct XenoActor *actor;

    mapProbeInit(&unduTest);
    unduTest.attr_mask = (short) (attr_mask | 0x4303);
    unduTest.header = mapFindBlockEntry(0x100, map_key);
    actor = uwamonoSlotTable[map_index].actor;
    if (map_index != -1 && actor != 0)
        unduTest.probeEnabled = actor->probeEnabled;
    return mapMoveProbe(&origin, 0, &unduTest);
}

/* Attribute at a position through the shared query block, default header (no map index is used). */
int unduGetAttr(const Vector4 *position, int unused, int attr_mask)
{
    mapProbeInit(&unduTest);
    unduTest.query_flags = 0;
    unduTest.attr_mask = (short) attr_mask;
    mapMoveProbe(position, 0, &unduTest);
    return unduTest.attribute;
}

/* Attribute at a position with the header of a map index; 0x100 keeps the header UnduParamInit set. */
int unduGetMapAttr(const Vector4 *position, int map_key, int attr_mask)
{
    mapProbeInit(&unduTest);
    unduTest.attr_mask = (short) attr_mask;
    unduTest.query_flags = 0;
    if (map_key != 0x100) {
        unduTest.header = mapFindBlockEntry(map_key, -1);
    }
    mapMoveProbe(position, 0, &unduTest);
    return unduTest.attribute;
}

extern float unduDistanceTolerance;

/* Sweeps from start toward end through the Undu query block; returns 1 when the sweep end is within 0.01 of end. */
int unduCheckSweep(int slotIndex, const Vector4 *from, const Vector4 *to, int map_block, int bits, Vector4 *hit, int attr_mask)
{
    Vector4 delta;

    mapProbeInit(&unduTest);
    unduTest.query_flags = 0;
    unduTest.attr_mask = (short) attr_mask;
    if (slotIndex != -1) {
        unduTest.scale = uwamonoScaleRecords[slotIndex].scale;
    } else {
        unduTest.scale = 0.0f;
    }
    unduTest.header = mapFindBlockEntry(map_block, bits);
    if (slotIndex != -1) {
        struct XenoActor *actor = uwamonoSlotTable[slotIndex].actor;
        unsigned char enabled = 1;

        if (actor != 0)
            enabled = actor->probeEnabled;
        unduTest.probeEnabled = enabled;
    } else {
        unduTest.probeEnabled = 1;
    }
    delta.x = to->x - from->x;
    delta.y = to->y - from->y;
    delta.z = to->z - from->z;
    delta.w = 1.0f;
    *hit = *from;
    mapMoveProbe(hit, 0, &unduTest);
    mapMoveProbe(hit, &delta, &unduTest);
    return Get_Distance3D(to, hit) < unduDistanceTolerance;
}

int Get_Rnd(int min, int max)
{
    return min + xglSRand() % (max - min + 1);
}

f32 xglFRand();

int Get_RndFloatRange(float minimum, float maximum)
{
    return (int) (minimum + (xglFRand() * (maximum - minimum)));
}

void vectorCopyPoint(Vector4 *destination, const Vector4 *source)
{
    float z;

    destination->x = source->x;
    destination->y = source->y;
    z = source->z;
    destination->w = 1.0f;
    destination->z = z;
}

void BSpline_Add(short frame, short count, short period,
                 Vector4 *destination, const Vector3 *source, short offset)
{
    unsigned short index;

    index = (frame / period + offset) % count;
    destination[index].x = source->x;
    destination[index].y = source->y;
    destination[index].z = source->z;
    destination[index].w = 1.0f;
}

void GetBSplineLoop(short frame, short count, int period_value,
                    Vector4 *points, Vector4 *destination)
{
    short period = period_value;
    unsigned short index = (frame / period) % count;
    unsigned short phase = (frame % period) * (GET_BSPLINE_PHASES / period);
    float first_weight = DataSpline[(phase + GET_BSPLINE_WEIGHTS_OFFSET) / 2];
    float second_weight = DataSpline[(phase + GET_BSPLINE_WEIGHTS_OFFSET + 2) / 2];
    float third_weight = DataSpline[(phase + GET_BSPLINE_WEIGHTS_OFFSET + 4) / 2];
    float fourth_weight = DataSpline[(phase + GET_BSPLINE_WEIGHTS_OFFSET + 6) / 2];
    unsigned short second_index = (index + 1) % count;
    unsigned short third_index = (index + 2) % count;
    unsigned short fourth_index = (index + 3) % count;

    destination->x = (first_weight * points[index].x + second_weight * points[second_index].x
                      + third_weight * points[third_index].x + fourth_weight * points[fourth_index].x)
                     / GET_BSPLINE_WEIGHT_SCALE;
    destination->y = (first_weight * points[index].y + second_weight * points[second_index].y
                      + third_weight * points[third_index].y + fourth_weight * points[fourth_index].y)
                     / GET_BSPLINE_WEIGHT_SCALE;
    destination->z = (first_weight * points[index].z + second_weight * points[second_index].z
                      + third_weight * points[third_index].z + fourth_weight * points[fourth_index].z)
                     / GET_BSPLINE_WEIGHT_SCALE;
}


/* Advances a per-actor spline cursor (frame at +0x58, segment steps at +0x5C) and interpolates the current segment (X1 GetBSplineLoopDummy). */
typedef struct BsplineCursor {
    unsigned char unmodeled_00[0x3C];
    float speed;                        /* +0x3C: divides the segment length into steps */
    unsigned char unmodeled_40[0x18];
    short frame;                        /* +0x58 */
    unsigned char unmodeled_5A[0x2];
    short steps;                        /* +0x5C */
} BsplineCursor;

/* One 0x870-byte uwamonoStateSlots entry: the state block pointer at +0x00 (init_uwamono_sys.c UwamonoStateSlot). */
typedef struct BsplineSlot {
    BsplineCursor *state;
    unsigned char unmodeled_04[0x870 - 0x04];
} BsplineSlot;

extern BsplineSlot uwamonoStateSlots[];

void bsplineGetLoopDummy(short frame, short count, short period, Vector4 *points, Vector4 *destination, int slot)
{
    BsplineCursor *cursor;
    int segment;
    unsigned short phase;
    unsigned short base;
    int next;
    int steps;
    int span;
    DoublewordVector4 *out = (DoublewordVector4 *) destination;

    cursor = uwamonoStateSlots[slot].state;
    segment = frame / period;
    phase = (unsigned short) (frame % period);
    base = (unsigned short) (segment % count);
    next = base + 1;
    steps = (int) (Get_Distance(&points[base], &points[next % count]) / cursor->speed);
    cursor->steps = (short) steps;
    if (phase >= cursor->steps - 1) {
        cursor->frame = (short) ((unsigned short) cursor->frame - cursor->frame % 128 + 128);
    } else {
        cursor->frame = (short) (cursor->frame + 1);
    }
    span = period * count;
    cursor->frame = cursor->frame % span;
    if (cursor->steps == 0) {
        destination->x = points[base].x;
        destination->y = points[base].y;
        destination->z = points[base].z;
    } else {
        out->vector.x = points[base].x + (points[next % count].x - points[base].x) * phase / cursor->steps;
        out->vector.y = points[base].y + (points[next % count].y - points[base].y) * phase / cursor->steps;
        out->vector.z = points[base].z + (points[next % count].z - points[base].z) * phase / cursor->steps;
    }
    destination->w = 1.0f;
}
