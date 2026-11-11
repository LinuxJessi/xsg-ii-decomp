#include "common.h"
#include "main/get.h"

/* Script native "atan": two centi-unit arguments, atan2f of them scaled by the float at D_0069B00C. */
extern float sinf(float angle);
extern float cosf(float angle);
extern float atan2f(float y, float x);
extern float D_0069B00C;
extern float D_0069B010;
extern float D_0069B014;
extern float D_0069B018;
extern float D_0069B01C;
int func_A9F0A8(void);
void func_A9CC70(int context, int value);
int func_A9F0C0(int context, int value);

int mathAtan(int context)
{
    float centiScale;
    float y;
    int x;

    if (func_A9F0A8() != 2) {
        func_A9CC70(context, 2);
        return 0;
    }
    centiScale = 100.0f;
    y = (float)func_A9F0C0(context, 1) / centiScale;
    x = func_A9F0C0(context, 2);
    return (int)(atan2f(y, (float)x / centiScale) * D_0069B00C);
}

/* Script native "math.sin": one argument in centi-degrees; returns sin(angle) * D_0069B014 (10000). */
int math_001FC398(int context)
{
    int angle_centideg;

    if (func_A9F0A8() != 1) {
        func_A9CC70(context, 2);
        return 0;
    }
    angle_centideg = func_A9F0C0(context, 1);
    return (int)(sinf((((float)angle_centideg / 100.0f) / 180.0f) * D_0069B010) * D_0069B014);
}

/* Script native "math.cos": one argument in centi-degrees; returns cos(angle) * D_0069B01C (10000). */
int math_001FC438(int context)
{
    int angle_centideg;

    if (func_A9F0A8() != 1) {
        func_A9CC70(context, 2);
        return 0;
    }
    angle_centideg = func_A9F0C0(context, 1);
    return (int)(cosf((((float)angle_centideg / 100.0f) / 180.0f) * D_0069B018) * D_0069B01C);
}

/* Script native "math.random": two arguments, min and max, passed to Get_Rnd. */
int math_001FC4D8(int context)
{
    int min;
    int max;

    if (func_A9F0A8() != 2) {
        func_A9CC70(context, 2);
        return 0;
    }
    min = func_A9F0C0(context, 1);
    max = func_A9F0C0(context, 2);
    return Get_Rnd(min, max);
}
