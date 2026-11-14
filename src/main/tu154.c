#include "common.h"
#include "shared.h"
#include "vu0_macro.h"
#include "main/m_math_188.h"

/* A key of a Hermite track: the point, and the key time in the low half of w. A time of
 * MMATH_HERMITE_END ends the track. */
typedef union MMathHermiteKey {
    HermiteVector point;
    struct {
        unsigned char unmodeled_00[0xC];
        unsigned short time;
    } key;
} MMathHermiteKey;

#define MMATH_HERMITE_END 0x400

/* The point of the Hermite track `keys` at `time`: the origin without keys, the first or last key's
 * point outside the keys (w = 1), else the Catmull-Rom curve between the two keys around `time`. */
void MMathEvalHermiteKeys(Vector4 *result, MMathHermiteKey *keys, float time)
{
    HermiteVector tangent[2];
    int frame;
    int first;
    int count;
    int index;
    int next;
    int after;
    int previous;
    float parameter;
    MMathHermiteKey *start = keys;

    frame = time;
    first = -1;
    index = 0;
    count = 0;
    for (; keys[index].key.time != MMATH_HERMITE_END; index++) {
        if (frame < keys[index].key.time) {
            if (first < 0) {
                first = index - 1;
            }
        }
        count++;
    }
    if (count <= 0) {
        Vu0SetOrigin(result);
        return;
    }
    if (count == 1) {
        Vu0MMathCopyPoint(result, start);
        return;
    }
    if (frame < keys[0].key.time) {
        count = 1;
    }
    if (first < 0 || first == count - 1) {
        Vu0MMathCopyPoint(result, &keys[count - 1]);
        return;
    }
    next = first + 1;
    previous = first - (first != 0);
    after = first < count - 2 ? first + 2 : next;
    parameter = (time - keys[first].key.time) / (float) (keys[next].key.time - keys[first].key.time);
    MMathCalcHermitePrm(&tangent[0], &tangent[1], &keys[previous].point, &keys[first].point,
                        &keys[next].point, &keys[after].point);
    MMathCalcHermite(&result->x, parameter, &tangent[0], &tangent[1], &keys[first].point, &keys[next].point);
    result->w = 1.0f;
}

void func_00252838(void)
{
}

Vector4 *MMathVectorCrossProduct(Vector4 *destination, const Vector4 *first, const Vector4 *second);
float MMathVectorDotProduct(const Vector4 *first, const Vector4 *second);

/* 1 when origin lies inside the triangle first, second, third (seen along the triangle's normal): the normals
 * of the three edge triangles around origin all point the same way. */
int func_00252840(const Vector4 *first, const Vector4 *second, const Vector4 *third, const Vector4 *origin)
{
    AlignedVector4 toFirst;
    AlignedVector4 toSecond;
    AlignedVector4 toThird;
    AlignedVector4 normal0;
    AlignedVector4 normal1;
    AlignedVector4 normal2;
    int inside;

    __asm__ __volatile__(
        "lqc2 vf1, 0(%0)\n\t"
        "lqc2 vf2, 0(%1)\n\t"
        "vsub.xyz vf1, vf1, vf2\n\t"
        "sqc2 vf1, 0(%2)"
        : : "r"(first), "r"(origin), "r"(&toFirst) : "memory");
    __asm__ __volatile__(
        "lqc2 vf1, 0(%0)\n\t"
        "lqc2 vf2, 0(%1)\n\t"
        "vsub.xyz vf1, vf1, vf2\n\t"
        "sqc2 vf1, 0(%2)"
        : : "r"(second), "r"(origin), "r"(&toSecond) : "memory");
    __asm__ __volatile__(
        "lqc2 vf1, 0(%0)\n\t"
        "lqc2 vf2, 0(%1)\n\t"
        "vsub.xyz vf1, vf1, vf2\n\t"
        "sqc2 vf1, 0(%2)"
        : : "r"(third), "r"(origin), "r"(&toThird) : "memory");
    MMathVectorCrossProduct(&normal0, &toFirst, &toThird);
    MMathVectorCrossProduct(&normal1, &toSecond, &toFirst);
    MMathVectorCrossProduct(&normal2, &toThird, &toSecond);
    inside = 0;
    if (MMathVectorDotProduct(&normal0, &normal1) >= 0.0f && MMathVectorDotProduct(&normal0, &normal2) >= 0.0f) {
        inside = 1;
    }
    return inside;
}

float MMathCalcDistXZ(int, int);

/* Returns 1 when the XZ distance between the points at +0x10 of the two entries is within limit, 0 otherwise. */
int MMathIsWithinDistXZ(int firstEntry, int secondEntry, float limit)
{
    if (MMathCalcDistXZ(firstEntry + 0x10, secondEntry + 0x10) <= limit) {
        return 1;
    }
    return 0;
}

INCLUDE_ASM("asm/main/nonmatchings/tu154", func_00252980);

INCLUDE_ASM("asm/main/nonmatchings/tu154", func_00252AB8);

int func_00252980(int firstEntry, int secondEntry, float limit);
int func_00252AB8(int firstEntry, int secondEntry, int kind);

/* Tests two entries by kind: 1 with func_00252980 at half the limit, 2 and 3 by their XZ distance, 4 to 7
 * with func_00252AB8. 1 when the test passes; 0 for a missing entry or another kind. */
int func_00252CD8(int kind, int firstEntry, int secondEntry, float limit)
{
    int result = 0;

    if (firstEntry == 0 || secondEntry == 0) {
        return 0;
    }
    switch (kind) {
    case 1:
        result = func_00252980(firstEntry, secondEntry, limit * 0.5f);
        break;
    case 2:
    case 3:
        result = MMathIsWithinDistXZ(firstEntry, secondEntry, limit);
        break;
    case 4:
    case 5:
    case 6:
    case 7:
        result = func_00252AB8(firstEntry, secondEntry, kind);
        break;
    }
    return result != 0;
}
