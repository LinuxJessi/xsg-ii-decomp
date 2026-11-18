/*
 * OV01 original TU 7: 0x00a814a8..0x00a818e8 (6 functions)
 */
#include "common.h"
#include "shared.h"

#define SLIDE_ANGLE_FULL_TURN 6.2831855f /* 2*pi, the wrap point of SlideBlock.angle */
#define SLIDE_ARRIVE_EPSILON 1e-6f       /* an angle or gap this small counts as arrived */

f32 slideHorizontalDistance();

/*
 * Slide block: the 0x80-byte subobject the slide steps work on. The callers pass it by address
 * (menu panels at MenuState +0x10, the ov01 tu050 subobject, the tu016 record pairLow at +0x150).
 * position moves toward goal by step each frame; angle moves toward goalAngle by angleRate.
 */
typedef struct SlideBlock {
    DoublewordAlignedVector4 position;  /* 0x00: x, y, z of the moved point */
    u8 unmodeled_10[0x14];
    f32 angle;                          /* 0x24: current angle, radians */
    u8 unmodeled_28[0x18];
    f32 step[3];                        /* 0x40: per-frame step, written by slideSetStep */
    u8 unmodeled_4C[0x14];
    DoublewordAlignedVector4 goal;      /* 0x60: destination; x and z are used by slideLimitGoalXZ */
    f32 goalAngle;                      /* 0x70: target angle */
    f32 angleRate;                      /* 0x74: radians per call, read by slideAngleStep */
} SlideBlock;

/* slideSetStep: per-frame step from position to goal over `frames`; axes within 0.5 of goal snap onto it. */
void slideSetStep(SlideBlock *block, int frames) {
    f32 snapRadius = 0.5f;
    f32 dyAfterX;
    f32 dy;
    f32 dz;
    f32 goalX = block->goal.x;
    f32 goalY = block->goal.y;
    f32 dx = goalX - block->position.x;
    dy = goalY - block->position.y;

    block->step[0] = dx / frames;
    block->step[1] = dy / frames;
    block->step[2] = (block->goal.z - block->position.z) / frames;

    if ((dx < 0.0f ? -dx : dx) < snapRadius) {
        block->position.x = goalX;
    }
    dyAfterX = goalY - block->position.y;
    if ((dyAfterX < 0.0f ? -dyAfterX : dyAfterX) < snapRadius) {
        block->position.y = goalY;
    }
    dz = block->goal.z - block->position.z;
    if ((dz < 0.0f ? -dz : dz) < snapRadius) {
        block->position.z = block->goal.z;
    }
}

/* slideHorizontalDistance: XZ distance from position to goal (no y term). */
f32 slideHorizontalDistance(block) SlideBlock *block; {
    f32 dx;
    f32 dz;

    dx = block->goal.x - block->position.x;
    if (dx < 0.0f) {
        dx = -dx;
    }
    dz = block->goal.z - block->position.z;
    if (dz < 0.0f) {
        dz = -dz;
    }
    return __builtin_sqrtf(dx * dx + dz * dz);
}

/* slideLimitGoalXZ: pulls goal toward position so its XZ distance is `numerator`. */
void slideLimitGoalXZ(SlideBlock *block, f32 numerator) {
    f32 fraction;
    f32 positionZ;

    fraction = numerator / slideHorizontalDistance();
    block->goal.x = block->position.x + (block->goal.x - block->position.x) * fraction;
    positionZ = block->position.z;
    block->goal.z = positionZ + (block->goal.z - positionZ) * fraction;
}

/* slideSnapPosToGoal: the position jumps to the goal (16 bytes copied). */
void slideSnapPosToGoal(SlideBlock *block) {
    block->position = block->goal;
}

/* slideAngleStep: steps angle toward goalAngle by angleRate, wrapping at 2*pi; snaps and returns 1 on arrival. */
int slideAngleStep(SlideBlock *block) {
    f32 angle;
    f32 offset;      /* a scratch: the current angle, then its gap to goalAngle, then the stepped angle */
    f32 rate;
    f32 goalDistance;
    int arrived;

    arrived = 0;
    offset = block->angle;
    angle = offset;
    if (offset == block->goalAngle) {
        block->angle = block->goalAngle;
        arrived = 1;
    } else {
        offset = offset - block->goalAngle;
        if ((offset < 0.0f ? (offset = -offset) : offset) <= SLIDE_ARRIVE_EPSILON) {
            block->angle = block->goalAngle;
            return 1;
        }
        rate = block->angleRate;
        block->angle = angle + rate;
        if (0.0f < rate && SLIDE_ANGLE_FULL_TURN <= block->angle) {
            block->angle = block->angle - SLIDE_ANGLE_FULL_TURN;
        }
        offset = block->angle;
        if (0.0f > rate && offset < 0.0f) {
            block->angle = SLIDE_ANGLE_FULL_TURN + offset;
            offset = block->angle;
        }
        goalDistance = offset - block->goalAngle;
        if (goalDistance < 0.0f) {
            goalDistance = -goalDistance;
        }
        if (goalDistance <= (rate < 0.0f ? -rate : rate)
            || (offset = offset - block->goalAngle, (offset < 0.0f ? (offset = -offset) : offset) <= SLIDE_ARRIVE_EPSILON)) {
            block->angle = block->goalAngle;
            arrived = 1;
        }
    }
    return arrived;
}

/* slideSnapAngleToGoal: the angle jumps to goalAngle. */
void slideSnapAngleToGoal(SlideBlock *block) {
    block->angle = block->goalAngle;
}
