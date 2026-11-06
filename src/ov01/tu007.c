/*
 * OV01 original TU 7: 0x00a814a8..0x00a818e8 (6 functions)
 *
 * Proposed names (applied later from proposals/ov01-tu007.json, not here):
 *   slideSetStep slideSetStep            slideHorizontalDistance slideHorizontalDistance
 *   slideLimitGoalXZ slideLimitGoalXZ        slideSnapPosToGoal slideSnapPosToGoal
 *   slideAngleStep slideAngleStep          slideSnapAngleToGoal slideSnapAngleToGoal
 */
#include "common.h"

/* slideSetStep: per-frame step from position to goal over `frames`; axes within 0.5 of goal snap onto it. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu007", slideSetStep);

/* slideHorizontalDistance: XZ distance from position to goal (no y term). */
INCLUDE_ASM("asm/nonmatchings/ov01/tu007", slideHorizontalDistance);

#include "shared.h"

f32 slideHorizontalDistance(void);

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

/* slideLimitGoalXZ: pulls goal toward position so its XZ distance is `numerator`. */
void slideLimitGoalXZ(SlideBlock *block, f32 numerator) {
    f32 fraction;
    f32 position_z;

    fraction = numerator / slideHorizontalDistance();
    block->goal.x = block->position.x + (block->goal.x - block->position.x) * fraction;
    position_z = block->position.z;
    block->goal.z = position_z + (block->goal.z - position_z) * fraction;
}

/* slideSnapPosToGoal: the position jumps to the goal (16 bytes copied). */
void slideSnapPosToGoal(SlideBlock *block) {
    block->position = block->goal;
}

/* slideAngleStep: steps angle toward goalAngle by angleRate, wrapping at 2*pi; snaps and returns 1 on arrival. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu007", slideAngleStep);

/* slideSnapAngleToGoal: the angle jumps to goalAngle. */
void slideSnapAngleToGoal(SlideBlock *block) {
    block->angle = block->goalAngle;
}
