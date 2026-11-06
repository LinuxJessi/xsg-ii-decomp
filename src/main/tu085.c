#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tu085", uwamonoInitState);

#include "shared.h"

/*
 * One 0x870-byte slot of uwamonoSlotTable. The actor body pointer at +0x00 and the
 * state block pointer at +0x04 are the only fields modeled here; the rest of
 * the slot is untouched.
 */
typedef struct UwamonoSlot UwamonoSlot;

typedef struct UwamonoActor UwamonoActor;

typedef struct UwamonoState UwamonoState;

struct UwamonoSlot {
    UwamonoActor *actor; /* +0x00 */
    UwamonoState *state; /* +0x04 */
    unsigned char unmodeled_08[0x870 - 0x08];
};

struct UwamonoActor {
    unsigned char unmodeled_00[0xB8];
    int flags; /* +0xB8 */
};

struct UwamonoState {
    unsigned char unmodeled_00[0x5A];
    u16 timer;      /* +0x5A: ticks since the current action started */
    s16 timerLimit; /* +0x5C */
    unsigned char unmodeled_5E[0x77 - 0x5E];
    u8 flags;       /* +0x77 */
};

void uwamonoSetAction(int slot_index, int action);
extern UwamonoSlot uwamonoSlotTable[];

/*
 * Advances the action timer of one slot. When the timer reaches its limit the
 * actor drops flag 0x800000 and uwamonoSetAction enters action 6 if state flag 1
 * is set, otherwise action 5.
 */
void uwamonoAdvanceTimer(int slotIndex)
{
    UwamonoSlot *slot;
    UwamonoState *state;
    UwamonoActor *actor;
    u16 timer;
    int flagBit;

    slot = &uwamonoSlotTable[slotIndex];
    state = slot->state;
    actor = slot->actor;
    timer = state->timer;
    state->timer = (u16) (timer + 1);
    if ((s16) timer < state->timerLimit)
    {
        return;
    }
    actor->flags &= 0xFF7FFFFF;
    flagBit = 1;
    if (state->flags & flagBit)
    {
        uwamonoSetAction(slotIndex, 6);
        return;
    }
    uwamonoSetAction(slotIndex, 5);
}

INCLUDE_ASM("asm/main/nonmatchings/tu085", uwamonoTickCooldown);

INCLUDE_ASM("asm/main/nonmatchings/tu085", uwamonoAimAtCurrent);

INCLUDE_ASM("asm/main/nonmatchings/tu085", func_00203528);

INCLUDE_ASM("asm/main/nonmatchings/tu085", uwamonoOrbitAnchor);

INCLUDE_ASM("asm/main/nonmatchings/tu085", uwamonoStepRoute);

INCLUDE_ASM("asm/main/nonmatchings/tu085", uwamonoFollowSpline);

INCLUDE_ASM("asm/main/nonmatchings/tu085", uwamonoNextAction);

INCLUDE_ASM("asm/main/nonmatchings/tu085", func_002048C8);

INCLUDE_ASM("asm/main/nonmatchings/tu085", uwamonoStepScript);

INCLUDE_ASM("asm/main/nonmatchings/tu085", uwamonoUpdateRecord);

INCLUDE_ASM("asm/main/nonmatchings/tu085", uwamonoMoveAlongHeading);

INCLUDE_ASM("asm/main/nonmatchings/tu085", uwamonoCopyPointIfNear);

INCLUDE_ASM("asm/main/nonmatchings/tu085", uwamonoScanTargets);

INCLUDE_ASM("asm/main/nonmatchings/tu085", uwamonoSetMotion);

INCLUDE_ASM("asm/main/nonmatchings/tu085", uwamonoSetAction);

INCLUDE_ASM("asm/main/nonmatchings/tu085", func_00205A20);

INCLUDE_ASM("asm/main/nonmatchings/tu085", uwamonoFindNearestReady);

INCLUDE_ASM("asm/main/nonmatchings/tu085", func_00205C78);

INCLUDE_ASM("asm/main/nonmatchings/tu085", func_00205D20);

INCLUDE_ASM("asm/main/nonmatchings/tu085", func_00205E50);

INCLUDE_ASM("asm/main/nonmatchings/tu085", func_00205FD8);

INCLUDE_ASM("asm/main/nonmatchings/tu085", uwamonoCompactRoute);

INCLUDE_ASM("asm/main/nonmatchings/tu085", uwamonoRebuildTrail);
