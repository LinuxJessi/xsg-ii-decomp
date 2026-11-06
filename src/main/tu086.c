#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tu086", uwamonoAddRouteNode);

INCLUDE_ASM("asm/main/nonmatchings/tu086", uwamonoFindNearbyTarget);

INCLUDE_ASM("asm/main/nonmatchings/tu086", uwamonoUpdateTargetVector);

INCLUDE_ASM("asm/main/nonmatchings/tu086", uwamonoOfferTarget);

/*
 * Each 0x870-byte record of D_003400F4 starts with a pointer to its state
 * block. Only the target priority and target slot read and written by these
 * functions are evidenced; the rest of the block and the record is unmodeled.
 */
struct UwamonoState {
    unsigned char unmodeled_0000[0x4028];
    short targetPriority; /* +0x4028 */
    short targetSlot;     /* +0x402A: -1 when no target is held */
};

struct UwamonoStateSlot {
    struct UwamonoState *state;
    unsigned char unmodeled_04[0x870 - 4];
};

extern struct UwamonoStateSlot D_003400F4[];

/*
 * Clears the held target when the offered threshold is not below the target
 * priority. Returns 1 when the target was cleared, 0 otherwise.
 */
int uwamonoTryClearTarget(int index, signed char threshold)
{
    struct UwamonoState *state;

    state = D_003400F4[index].state;
    if (threshold < state->targetPriority) {
        return 0;
    }
    state->targetPriority = 0;
    state->targetSlot = -1;
    return 1;
}

/* Drops the held target: no slot, zero priority. */
void uwamonoClearTarget(int index)
{
    struct UwamonoState *state;

    state = D_003400F4[index].state;
    state->targetSlot = -1;
    state->targetPriority = 0;
}
