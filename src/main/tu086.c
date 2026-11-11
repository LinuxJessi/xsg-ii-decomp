#include "common.h"
#include "shared.h"
#include "main/tu051.h"

enum {
    UWAMONO_ROUTE_NODE_MAX = 64,        /* route node rows of UwamonoState */
    UWAMONO_NO_ROUTE_NODE = -1,         /* a route node with no parent */
    UWAMONO_NO_TARGET = -1,             /* the target slot when no target is held */
    UWAMONO_FLAG_CANDIDATE = 0x1000000  /* record flags word: marks a candidate target */
};

/*
 * Each 0x870-byte record of uwamonoStateSlots starts with a pointer to its state
 * block. The route node table, the target priority, the target slot and the
 * target vector are read and written by these functions; the rest of the block
 * and the record is unmodeled. The route node count is read as a signed short
 * and incremented as an unsigned short: the two views are the same 16 bits.
 */
union UwamonoNodeCount {
    short signedValue;
    unsigned short unsignedValue;
};

struct UwamonoState {
    unsigned char unmodeled_0000[0x30B0];
    float routeNodes[UWAMONO_ROUTE_NODE_MAX][4];          /* +0x30B0: x, y, z of each route node; w is not written */
    short routeEdge[UWAMONO_ROUTE_NODE_MAX][4];           /* +0x34B0: linked node indices per node */
    short routeEdgeCount[UWAMONO_ROUTE_NODE_MAX];         /* +0x36B0 */
    short routeParent[UWAMONO_ROUTE_NODE_MAX];            /* +0x3730: parent node, -1 when none */
    union UwamonoNodeCount routeNodeCount; /* +0x37B0 */
    unsigned char unmodeled_37B2[0x4028 - 0x37B2];
    /* Byte views: the original accesses these with alias set 0 (style debt, alias lever). */
    union {
        short targetPriority;
        unsigned char targetPriorityBytes[2];
    }; /* +0x4028 */
    union {
        short targetSlot;
        unsigned char targetSlotBytes[2];
    }; /* +0x402A: -1 when no target is held */
    unsigned char unmodeled_402C[0x4030 - 0x402C];
    Vector4 targetVector; /* +0x4030: the point the held target is moved to */
};

struct UwamonoStateSlot {
    struct UwamonoState *state;
    unsigned char unmodeled_04[0x870 - 4];
};

extern struct UwamonoStateSlot uwamonoStateSlots[];

/*
 * Appends a route node at the current node count with the given position. A
 * node whose parent differs from itself records the parent; a parent other
 * than -1 also links the two nodes in each other's edge row.
 */
void uwamonoAddRouteNode(int index, short parent, float x, float y, float z)
{
    struct UwamonoState *state = uwamonoStateSlots[index].state;
    short node = state->routeNodeCount.signedValue;
    state->routeNodes[node][0] = x;
    state->routeNodes[node][1] = y;
    state->routeNodes[node][2] = z;
    if (node != parent) {
        state->routeParent[node] = parent;
        if (parent != UWAMONO_NO_ROUTE_NODE) {
            state->routeEdge[node][state->routeEdgeCount[node]] = parent;
            state->routeEdge[parent][state->routeEdgeCount[parent]] = node;
            state->routeEdgeCount[node]++;
            state->routeEdgeCount[parent]++;
        }
    }
    state->routeNodeCount.unsignedValue++;
}

/* Record view of uwamonoRecords (tu050 owns MapObjectRecord): 0x870 bytes, the fields read here. */
/* Slot table at the same 0x870 stride: the object pointer at +0 is the record actor (uwamonoRecords[i].actor). */
struct UwamonoObjectSlot {
    struct XenoActor *object;
    unsigned char unmodeled_04[0x870 - 4];
};

extern struct UwamonoObjectSlot uwamonoSlotTable[];
extern float D_0069B134;
/* the float at gp-31932 (gp = 0x6A2DF0), read as the fan limit */
int unduCheckInsideFan(int index, const Vector4 *origin, const Vector4 *target, int mode, float facing, float range, float limit, float height);
float Get_Distance3D(const Vector4 *first, const Vector4 *second);

int uwamonoFindNearbyTarget(short index)
{
    struct XenoActor *actor;
    float distance;
    float nearestDistance;
    int bestIndex;
    short candidate;

    actor = uwamonoSlotTable[index].object;
    bestIndex = -1;
    nearestDistance = 5.0f;
    for (candidate = 0; candidate < 256; candidate++) {
        if (index == candidate) {
            continue;
        }
        if (uwamonoRecords[candidate].kind != 4 && uwamonoRecords[candidate].kind != 1) {
            continue;
        }
        if ((uwamonoRecords[candidate].flags & UWAMONO_FLAG_CANDIDATE) == 0) {
            continue;
        }
        if (!unduCheckInsideFan(index, &actor->position, &uwamonoRecords[candidate].actor->position, -1, actor->rotation.y, 5.0f, D_0069B134, actor->radius)) {
            continue;
        }
        distance = Get_Distance3D(&actor->position, &uwamonoRecords[candidate].actor->position);
        if (distance < nearestDistance && distance <= 8.0f) {
            nearestDistance = distance;
            bestIndex = candidate;
        }
    }
    return bestIndex;
}

void func_0028B268(struct XenoActor *actor, Vector4 *target);

void uwamonoUpdateTargetVector(int index)
{
    struct UwamonoState *state;
    UwamonoObject *targetRecord;
    struct XenoActor *actor;
    short targetSlot;
    float x;
    float y;
    float z;

    actor = uwamonoRecords[index].actor;
    state = (struct UwamonoState *) uwamonoRecords[index].motions;
    targetSlot = state->targetSlot;
    if (targetSlot == UWAMONO_NO_TARGET) {
        func_0028B268(actor, &state->targetVector);
        return;
    }
    targetRecord = &uwamonoRecords[targetSlot];
    x = targetRecord->position.x;
    state->targetVector.x = x;
    y = targetRecord->position.y;
    state->targetVector.y = y;
    z = targetRecord->position.z;
    state->targetVector.w = 1.0f;
    state->targetVector.z = z;
    if (targetRecord->kind == 1 || targetRecord->kind == 4) {
        state->targetVector.y = y + targetRecord->extent.y;
    } else {
        state->targetVector.x = x + targetRecord->focusOffset.x;
        state->targetVector.y = y + (targetRecord->focusOffset.y + targetRecord->extent.y * 0.5f);
        state->targetVector.z = z + targetRecord->focusOffset.z;
    }
    func_0028B268(actor, &state->targetVector);
}

/*
 * Offers a target to the unit at index. The offer is refused (returns 0) when
 * its priority is below the held priority. Otherwise it takes the target slot;
 * a target of -1 also stores the point as the target vector with w 1.
 * Returns 1 when the offer is taken.
 */
int uwamonoOfferTarget(int index, signed char priority, int targetSlot, Vector4 *point)
{
    struct UwamonoState *state;

    state = uwamonoStateSlots[index].state;
    if (priority < state->targetPriority) {
        return 0;
    }
    if (targetSlot != UWAMONO_NO_TARGET) {
        state->targetPriority = priority;
        state->targetSlot = targetSlot;
    } else {
        state->targetSlot = targetSlot;
        state->targetVector.x = point->x;
        state->targetVector.y = point->y;
        state->targetVector.z = point->z;
        state->targetPriority = priority;
        state->targetVector.w = 1.0f;
    }
    return 1;
}

/*
 * Clears the held target when the offered threshold is not below the target
 * priority. Returns 1 when the target was cleared, 0 otherwise.
 */
int uwamonoTryClearTarget(int index, signed char threshold)
{
    struct UwamonoState *state;

    state = uwamonoStateSlots[index].state;
    if (threshold < state->targetPriority) {
        return 0;
    }
    state->targetPriority = 0;
    state->targetSlot = UWAMONO_NO_TARGET;
    return 1;
}

/* Drops the held target: no slot, zero priority. */
void uwamonoClearTarget(int index)
{
    struct UwamonoState *state;

    state = uwamonoStateSlots[index].state;
    state->targetSlot = UWAMONO_NO_TARGET;
    state->targetPriority = 0;
}
