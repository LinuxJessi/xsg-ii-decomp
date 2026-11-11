#include "common.h"
#include "shared.h"
#include "main/tu051.h"

float Get_Distance(const Vector4 *first, const Vector4 *second);
float Get_Angle(const Vector4 *first, const Vector4 *second);
float Get_Decimal_Surplus_for_Radius(float angle);
extern float sinf(float angle);
extern float cosf(float angle);

struct UwamonoState {
    unsigned char unmodeled_00[0x08];
    int flagIndex;
    unsigned char unmodeled_0c[0x20 - 0x0c];
    Vector4 target;
    float stopRange;
    unsigned char unmodeled_34[0x90 - 0x34];
    union {
        Vector4 trailPoints[256];
    };
    short trailCount;
    unsigned char unmodeled_1092[0x4054 - 0x1092];
    int children[4];
};

struct UwamonoStateSlot {
    struct UwamonoState *state;
    unsigned char unmodeled_04[0x870 - 4];
};

extern struct UwamonoStateSlot uwamonoStateSlots[];

void uwamonoClampToStopRange(int index)
{
    UwamonoObject *record;
    struct XenoActor *actor;
    struct UwamonoState *state;
    float limit;
    float angle;
    float x;
    float z;

    record = &uwamonoRecords[index];
    actor = record->actor;
    state = (struct UwamonoState *)record->motions;
    limit = state->stopRange - actor->radius - 0.1f;
    if (Get_Distance(&state->target, &actor->position) < limit) {
        return;
    }
    angle = Get_Angle(&state->target, &actor->position);
    x = state->target.x + sinf(angle) * limit;
    actor->position.x = x;
    record->position.x = x;
    z = state->target.z + cosf(angle) * limit;
    actor->position.z = z;
    record->position.z = z;
    actor->direction.y = Get_Decimal_Surplus_for_Radius(angle + 3.1415927f);
}

void uwamonoPushTrailPoint(int index, float x, float y, float z)
{
    struct UwamonoState *state;

    state = uwamonoStateSlots[index].state;
    state->trailPoints[state->trailCount].x = x;
    state->trailPoints[state->trailCount].y = y;
    state->trailPoints[state->trailCount].z = z;
    state->trailPoints[state->trailCount++].w = 1.0f;
}

struct GameWorkFlags {
    unsigned char unmodeled_00[8];
    int flags;
    unsigned char unmodeled_0c[0x1720 - 0x0c];
    unsigned char releaseFlagsDefault[0x200];
    unsigned char releaseFlagsBit800[0x200];
};

extern struct GameWorkFlags gameWork;
void uwamonoReleaseRecord(int index);

void uwamonoReleaseChildren(int index)
{
    struct UwamonoState *state;
    unsigned int workBase;
    int children[4];
    int i;

    if (index == -1) {
        return;
    }
    state = (struct UwamonoState *)uwamonoRecords[index].motions;
    for (i = 0; i < 4; i++) {
        children[i] = state->children[i];
    }
    if (children[0] == -1 && children[1] == -1 && children[2] == -1 && children[3] == -1) {
        struct UwamonoState *childState;
        struct GameWorkFlags *selfWork;
        int offset;

        childState = (struct UwamonoState *)uwamonoRecords[index].motions;
        selfWork = &gameWork;
        offset = childState->flagIndex + 0x1920;
        if (!(selfWork->flags & 0x800)) {
            offset = childState->flagIndex + 0x1720;
        }
        ((unsigned char *)selfWork)[offset] = 1;
        uwamonoReleaseRecord(index);
        return;
    }
    workBase = (unsigned int)&gameWork;
    for (i = 0; i < 4; i++) {
        int child = children[i];

        if (child != -1) {
            struct UwamonoState *childState;
            int offset;

            childState = (struct UwamonoState *)uwamonoRecords[child].motions;
            offset = childState->flagIndex + 0x1920;
            if (!(((struct GameWorkFlags *)workBase)->flags & 0x800)) {
                offset = childState->flagIndex + 0x1720;
            }
            *(unsigned char *)(offset + workBase) = 1;
            uwamonoReleaseRecord(child);
        }
    }
}
