#include "common.h"
#include "main/tu206.h"
#include "main/tu229.h"
#include "main/tu242.h"

INCLUDE_ASM("asm/main/nonmatchings/tu227", interpUpdate);

INCLUDE_ASM("asm/main/nonmatchings/tu227", func_0027E2D0);

INCLUDE_ASM("asm/main/nonmatchings/tu227", func_0027E3C0);

#include "shared.h"

/*
 * The interpolation block at +0x28 of the object. Its members are the
 * sub-object the functions of this TU address through one pointer; the
 * gaps stay unmodeled.
 */
typedef struct InterpBlock {
    int handle;
    u8 unmodeled_04[0x08];
    int pending;
    u8 unmodeled_10[0x04];
    int cursor;
    int value;
    int base;
    int delta;
    int angle;
    int angle_step;
    int range_first;
    u8 unmodeled_30[0x04];
    int range_last;
} InterpBlock;

typedef struct InterpObject {
    int flags;
    u8 unmodeled_04[0x18];
    int state;
    u8 unmodeled_20[0x08];
    InterpBlock interp;
} InterpObject;

extern float cosf(float x);
extern float mathDegToRad;
int func_00286F08(int delta, int base);

void interpRewind(InterpObject *object)
{
    if (object->state == 1) {
        textSetOutputCursor(object->interp.handle);
        object->flags = ((object->flags & ~0x4) & ~((u32) 0x8)) | 0x2;
    }
}

void interpSetPending(InterpObject *object, int value)
{
    if (object->state == 1) {
        object->interp.pending = value;
    }
}

void interpAdvance(InterpObject *object, int advance)
{
    InterpBlock *interp;
    int old_value;
    int cursor;
    int next_cursor;
    int base;

    interp = &object->interp;
    if (object->state == 1) {
        object->flags |= 0x20;
        old_value = interp->value;
        cursor = interp->cursor + advance;
        interp->delta = old_value;
        interp->cursor = cursor;
        next_cursor = xglClampInt(cursor, 1, interp->range_last - interp->range_first + 1);
        interp->cursor = next_cursor;
        base = interpCursorFrame(next_cursor);
        interp->angle = 0;
        interp->base = base;
        interp->angle_step = func_00286F08(interp->delta, base);
    }
}

int interpStepCosine(InterpObject *object)
{
    InterpBlock *interp;
    int angle;
    int base;
    int half_sum;
    int midpoint;
    float span;

    interp = &object->interp;
    angle = interp->angle + interp->angle_step;
    interp->angle = angle;
    if (angle >= 0xB4) {
        interp->value = interp->base;
        return 1;
    }
    base = interp->base;
    half_sum = base + interp->delta;
    midpoint = half_sum >> 1;
    span = (float) (midpoint - base);
    interp->value = midpoint + (int) (span * cosf((float) angle * mathDegToRad));
    return 0;
}
