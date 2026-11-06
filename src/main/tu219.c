#include "common.h"
/* Grid step and origin used by msgWindowRunCurrentCell (the 20, 30 and 64 of its
 * argument arithmetic). */
#define DISPATCH_STEP_X 20
#define DISPATCH_STEP_Y 30
#define DISPATCH_ORIGIN 64

typedef struct DispatchBody DispatchBody;

typedef struct DispatchSlot DispatchSlot;

/* Body of a dispatch slot, at slot +0x08.  These three functions touch only
 * +0x00 to +0x1C of the 0x90-byte body; the rest is not used here. */
struct DispatchBody {
    int active;                 /* +0x00: nonzero when the body is live */
    int anchor_x;               /* +0x04: passed as a callee argument */
    int anchor_y;               /* +0x08: passed as a callee argument */
    unsigned char unmodeled_0C[4];
    int extent_x;               /* +0x10: passed as a callee argument */
    int extent_y;               /* +0x14: passed as a callee argument */
    unsigned char unmodeled_18[4];
    int kind;                   /* +0x1C: selects the callee */
    unsigned char unmodeled_20[0x70];
};

/* One 0x98-byte record.  The word at +0x00 of slot 0 selects the record
 * whose body these functions use; a negative value selects nothing. */
struct DispatchSlot {
    int current;                  /* +0x00 */
    unsigned char unmodeled_04[4];
    DispatchBody body;          /* +0x08 */
};

void func_0027E048(void *body, int first, int second, int third, int fourth);
void func_00284338(DispatchBody *body, int first, int second, int third, int fourth);
void timecodeTickIfActive(void *body, int first, int second, int third, int fourth);

void msgWindowRunCurrentAt(DispatchSlot *slots, int x, int y)
{
    int current;
    DispatchBody *body;

    current = slots[0].current;
    if (current < 0) {
        return;
    }
    body = &slots[current].body;
    if (body->active == 0) {
        return;
    }
    switch (body->kind) {
    case 1:
        func_0027E048(body, x, y, body->extent_x, body->extent_y);
        break;

    case 2:
        func_00284338(body, x, y, body->extent_x, body->extent_y);
        break;

    case 3:
        timecodeTickIfActive(body, x, y, body->extent_x, body->extent_y);
        break;
    }
}

void msgWindowRunCurrentAnchored(DispatchSlot *slots, int x, int y)
{
    int current;
    DispatchBody *body;

    current = slots[0].current;
    if (current < 0) {
        return;
    }
    body = &slots[current].body;
    if (body->active == 0) {
        return;
    }
    switch (body->kind) {
    case 1:
        func_0027E048(body, body->anchor_x, body->anchor_y, x, y);
        break;

    case 2:
        func_00284338(body, body->anchor_x, body->anchor_y, x, y);
        break;

    case 3:
        timecodeTickIfActive(body, body->anchor_x, body->anchor_y, x, y);
        break;
    }
}

void msgWindowRunCurrentCell(DispatchSlot *slots, int x, int y)
{
    int current;
    DispatchBody *body;

    current = slots[0].current;
    if (current >= 0) {
        body = &slots[current].body;
        func_0027E048(body, body->anchor_x, body->anchor_y,
                      x * DISPATCH_STEP_X + DISPATCH_ORIGIN,
                      y * DISPATCH_STEP_Y + DISPATCH_ORIGIN);
    }
}
