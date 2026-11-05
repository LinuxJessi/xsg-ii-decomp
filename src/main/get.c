#include "common.h"
#include "shared.h"

extern float sinf(float angle);
extern float cosf(float angle);
extern float atan2f(float y, float x);

int Get_Rnd(int min, int max);

int CheckPointLine(const Vector4 *first, const Vector4 *second,
                   const Vector4 *point);

unsigned short xglSRand(void);

/* GetBSplineLoop: DataSpline holds the weight table (byte offset) at 2048 phases per control-point period; the sum is divided by 6 * 4096. */
#define GET_BSPLINE_PHASES 2048

#define GET_BSPLINE_WEIGHTS_OFFSET 4304
#define GET_BSPLINE_WEIGHT_SCALE 24576.0f

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

void Get_MiddlePoint(const float *start, const float *following,
                     short point_index, short point_count, float *result);

float Get_Multi_Max_Under(float value, float step, float maximum);

void Get_MiddlePoint_Parabora(const ParabolaVec *first, const ParabolaVec *second,
                              int current, int total, float height,
                              ParabolaVec *destination);

extern unsigned short * DataSpline;

float Get_Decimal_Surplus_for_Radius(float angle)
{
    if (angle <= 0.0f) {
        float negative_pi = -3.1415927f;
        if (angle < negative_pi) {
            do {
                angle += 6.2831855f;
            } while (angle < negative_pi);
        }
    } else {
        while (3.1415927f <= angle) {
            angle -= 6.2831855f;
        }
    }
    return angle;
}

INCLUDE_ASM("asm/main/nonmatchings/get", func_00208C88);

INCLUDE_ASM("asm/main/nonmatchings/get", func_00208E10);

void Get_Point_By_AngleLength(const Vector4 *source, Vector4 *destination,
                              float angle, float length)
{
    destination->x = source->x + sinf(angle) * length;
    destination->y = source->y;
    destination->z = source->z + cosf(angle) * length;
    destination->w = source->w;
}

INCLUDE_ASM("asm/main/nonmatchings/get", func_00209300);

void Get_MiddlePoint(const float *start, const float *following,
                     short point_index, short point_count, float *result)
{
    if (point_index == point_count) {
        result[0] = following[0];
        result[1] = following[1];
        result[2] = following[2];
        result[3] = 1.0f;
    } else {
        result[0] = start[0] + (following[0] - start[0]) * (float)point_index
                    / (float)point_count;
        result[1] = start[1] + (following[1] - start[1]) * (float)point_index
                    / (float)point_count;
        result[2] = start[2] + (following[2] - start[2]) * (float)point_index
                    / (float)point_count;
        result[3] = 1.0f;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/get", func_00209458);

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

INCLUDE_ASM("asm/main/nonmatchings/get", Check_Wall);

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

INCLUDE_ASM("asm/main/nonmatchings/get", func_00209AC0);

INCLUDE_ASM("asm/main/nonmatchings/get", Check_Straight_ID);

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

INCLUDE_ASM("asm/main/nonmatchings/get", func_00209FD8);

float Get_Angle_Relative(const Vector4 *first, const Vector4 *second,
                         float reference)
{
    float relative = Get_Angle(first, second) - reference;

    if (relative < -3.1415927f)
        relative += 6.2831855f;
    if (3.1415927f < relative)
        relative -= 6.2831855f;
    return relative;
}

INCLUDE_ASM("asm/main/nonmatchings/get", func_0020A158);

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

INCLUDE_ASM("asm/main/nonmatchings/get", func_0020A408);

INCLUDE_ASM("asm/main/nonmatchings/get", func_0020A588);

INCLUDE_ASM("asm/main/nonmatchings/get", func_0020A658);

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

INCLUDE_ASM("asm/main/nonmatchings/get", func_0020A910);

INCLUDE_ASM("asm/main/nonmatchings/get", func_0020A968);

INCLUDE_ASM("asm/main/nonmatchings/get", func_0020AA48);

INCLUDE_ASM("asm/main/nonmatchings/get", func_0020AAA8);

INCLUDE_ASM("asm/main/nonmatchings/get", func_0020AB30);

int Get_Rnd(int min, int max)
{
    return min + xglSRand() % (max - min + 1);
}

INCLUDE_ASM("asm/main/nonmatchings/get", func_0020AD48);

INCLUDE_ASM("asm/main/nonmatchings/get", func_0020AD90);

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

INCLUDE_ASM("asm/main/nonmatchings/get", func_0020B008);
