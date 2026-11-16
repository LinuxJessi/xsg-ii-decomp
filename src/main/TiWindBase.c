#include "common.h"
#include "main/TiWindBase.h"
#include "main/tu026.h"
#include "main/xgl_2.h"

/* Three-float vector as used by the node positions below. */
typedef struct Vector3 {
    float x;
    float y;
    float z;
} Vector3;

/* Node record, 0x40 bytes: position at 0x10, secondPosition at 0x20 (both moved by a link), otherPosition at 0x30 (read only). */
typedef struct TiWindNode {
    unsigned char unmodeled_00[0x10];
    Vector3 position;
    unsigned char unmodeled_1C[0x4];
    Vector3 secondPosition;
    unsigned char unmodeled_2C[0x4];
    Vector3 otherPosition;
    unsigned char unmodeled_3C[0x4];
} TiWindNode;

void tiWindSolveLink(TiWindNode *node, const TiWindNode *other, const float *restLength)
{
    Vector3 dir;
    float length;
    float correction;

    dir = (Vector3) { other->otherPosition.x - node->position.x, other->otherPosition.y - node->position.y, other->otherPosition.z - node->position.z };
    length = __builtin_sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    correction = *restLength - length;
    node->secondPosition.x -= dir.x * correction;
    node->secondPosition.y -= dir.y * correction;
    node->secondPosition.z -= dir.z * correction;
    node->position.z -= dir.z * correction;
    node->position.x -= dir.x * correction;
    node->position.y -= dir.y * correction;
}

#include "shared.h"
#define DIR_WIND_MAX 4U
#define POINT_WIND_MAX 4U
#define WIND_FLAG_ENABLED 0x1
#define WIND_FLAG_GLOBAL 0x2
/* Gust redraw: xglSRand() % WIND_GUST_STEPS, gust = step * 0.01 + 0.5. */
#define WIND_GUST_STEPS 70U

/*
 * Directional wind table entry, 0x60 bytes.
 * flags: 0x1 enabled, 0x2 global (no falloff with distance).
 * 0x10: unit direction (w = 1 at 0x1C). 0x20: velocity, direction * phase * 0.1.
 * 0x30: position (w = 1 at 0x3C). 0x40: phase, oscillates between upper and lower.
 * phaseStep: +1 or -1, the direction of the phase; speed scales each step.
 * 0x4C: upper bound, 0x50: lower bound, 0x54: radius of the falloff.
 */
typedef struct DirWind {
    int flags;
    unsigned char unmodeled_04[0xC];
    float direction[4];
    float velocity[4];
    float position[4];
    float phase;
    float phaseStep;
    float speed;
    float upper;
    float lower;
    float radius;
    unsigned char unmodeled_58[0x8];
} DirWind;

/* D_00989D90 is the DirWind table 0x40 bytes in; the speed setter reaches its fields through it. */
typedef struct DirWindValue {
    unsigned char unmodeled_00[4];
    float phaseStep;
    float speed;
    unsigned char unmodeled_0C[0x54];
} DirWindValue;

/*
 * Point wind table entry, 0x40 bytes.
 * flags as DirWind. 0x10: position (w = 1 at 0x1C). 0x24: scalar wave, phase * 0.1.
 * 0x28: phase. phaseStep: +1 or -1. speedRange[0]: speed, [1]: upper bound, [2]: lower bound.
 * 0x3C: radius of the falloff.
 */
typedef struct PointWind {
    int flags;
    unsigned char unmodeled_04[0xC];
    float position[4];
    unsigned char unmodeled_20[0x4];
    float wave;
    float phase;
    float phaseStep;
    float speedRange[3];
    float radius;
} PointWind;

/* D_00989F00 is the PointWind table 0x30 bytes in; the range setter reaches speedRange[1] and [2] through it. */
typedef struct PointWindValues {
    float speedRange[3];
    unsigned char unmodeled_0C[0x34];
} PointWindValues;

/* Node records (TiWindNode), indexed from the first node of the wind's cell. */

/* Cell grid: the four starts sum to the first node index; the cell list follows at pairOffset. */
typedef struct TiWindGrid {
    unsigned char unmodeled_00[0x10];
    int nodeStart[4];
    unsigned char unmodeled_20[0x28];
    int pairTotal;
    unsigned char unmodeled_4C[0xC];
    int pairOffset;
} TiWindGrid;

/* A cell header (first node index, pair count) is followed by its pairs (second node index). */
typedef struct TiWindPair {
    int first;
    int second;
    unsigned char unmodeled_08[0x8];
} TiWindPair;

typedef struct TiWindWorld {
    unsigned char unmodeled_00[0xD0];
    TiWindGrid *grid;
    unsigned char unmodeled_D4[0xC];
    TiWindNode *otherNodes;
    unsigned char unmodeled_E4[0x8];
    TiWindNode *nodes;
} TiWindWorld;

void tiDynamicsCollSolveSpherePair(TiWindNode *a, TiWindNode *b, TiWindWorld *world);
void tiDynamicsCollSolveMatrixPair(TiWindNode *a, TiWindNode *b, TiWindWorld *world);
void tiDynamicsCollSolveCapsulePair(TiWindNode *a, TiWindNode *b, TiWindWorld *world);
void xglVectorNormal_001A9200(float *destination, const float *source);
extern DirWind tiWindDirTable[DIR_WIND_MAX];
extern DirWindValue D_00989D90[DIR_WIND_MAX];
extern PointWind tiWindPointTable[POINT_WIND_MAX];
extern PointWindValues D_00989F00[POINT_WIND_MAX];
extern const char D_0068F7B0[];
extern const char D_0068F7C8[];
extern const char D_0068F7E0[];

/* Runs the collision solvers over every pair of each cell of the grid. */
void tiWindSolveCellPairs(TiWindWorld *world)
{
    TiWindNode *nodes;
    int leftStart;
    int rightStart;
    TiWindPair *cell;
    TiWindPair *pair;
    int nodeBase;
    u32 pairIndex;
    u32 cellIndex;

    nodes = world->nodes;
    leftStart = world->grid->nodeStart[0] + world->grid->nodeStart[3];
    rightStart = world->grid->nodeStart[2] + world->grid->nodeStart[1];
    nodeBase = leftStart + rightStart;
    cell = (TiWindPair *)((char *)world->grid + world->grid->pairOffset);
    cellIndex = 0;
    if (world->grid->pairTotal != 0) {
        do {
            pair = cell + 1;
            pairIndex = 0;
            if (cell->second != 0) {
                do {
                    pairIndex++;
                    tiWindSolveLink(&nodes[cell->first - nodeBase], &world->otherNodes[pair->second], pair);
                    tiDynamicsCollSolveSpherePair(&nodes[cell->first - nodeBase], &nodes[pair->second - nodeBase], world);
                    tiDynamicsCollSolveMatrixPair(&nodes[cell->first - nodeBase], &nodes[pair->second - nodeBase], world);
                    tiDynamicsCollSolveCapsulePair(&nodes[cell->first - nodeBase], &nodes[pair->second - nodeBase], world);
                    pair++;
                } while (pairIndex < cell->second);
            }
            cell = pair;
        } while (++cellIndex < world->grid->pairTotal);
    }
}

/* Clears every directional and point wind entry. */
void tiWindClearTables(void)
{
    u32 index;

    for (index = 0; index < DIR_WIND_MAX; index++) {
        memset(&tiWindDirTable[index], 0, sizeof(DirWind));
    }
    for (index = 0; index < POINT_WIND_MAX; index++) {
        memset(&tiWindPointTable[index], 0, sizeof(PointWind));
    }
}

/* Enables or disables directional wind `index`. */
void tiWindDirSetEnabled(u32 index, int enabled)
{
    if (index >= DIR_WIND_MAX) {
        assertFailed(D_0068F7B0, D_0068F7C8, 29);
    }
    if (enabled != 0) {
        tiWindDirTable[index].flags |= WIND_FLAG_ENABLED;
        return;
    }
    tiWindDirTable[index].flags &= ~WIND_FLAG_ENABLED;
}

/* Sets the direction of directional wind `index`: normalized, with the velocity reset to rest. */
void tiWindDirSetDirection(u32 index, float x, float y, float z)
{
    if (index >= DIR_WIND_MAX) {
        assertFailed(D_0068F7B0, D_0068F7C8, 42);
    }
    tiWindDirTable[index].direction[0] = x;
    tiWindDirTable[index].direction[1] = y;
    tiWindDirTable[index].direction[2] = z;
    tiWindDirTable[index].direction[3] = 1.0f;
    xglVectorNormal_001A9200(tiWindDirTable[index].direction, tiWindDirTable[index].direction);
    tiWindDirTable[index].velocity[0] = 0.0f;
    tiWindDirTable[index].velocity[1] = 0.0f;
    tiWindDirTable[index].velocity[2] = 0.0f;
    tiWindDirTable[index].velocity[3] = 1.0f;
}

/* Sets the position of directional wind `index`; the fourth component is the homogeneous 1. */
void tiWindDirSetPosition(u32 index, const float *position)
{
    if (index >= DIR_WIND_MAX) {
        assertFailed(D_0068F7B0, D_0068F7C8, 60);
    }
    tiWindDirTable[index].position[0] = position[0];
    tiWindDirTable[index].position[1] = position[1];
    tiWindDirTable[index].position[2] = position[2];
    tiWindDirTable[index].position[3] = 1.0f;
}

/* Sets the speed of directional wind `index`. A new speed resets the phase step to +1. */
void tiWindDirSetSpeed(u32 index, float speed)
{
    DirWindValue *entry;

    if (index >= DIR_WIND_MAX) {
        assertFailed(D_0068F7B0, D_0068F7C8, 73);
    }
    entry = &D_00989D90[index];
    if (entry->speed != speed) {
        entry->speed = speed;
        entry->phaseStep = 1.0f;
    }
}

/* Sets the upper and lower phase bounds of directional wind `index`. */
void tiWindDirSetRange(u32 index, float upper, float lower)
{
    DirWind *entry;

    if (index >= DIR_WIND_MAX) {
        assertFailed(D_0068F7B0, D_0068F7C8, 87);
    }
    entry = &tiWindDirTable[index];
    entry->upper = upper;
    tiWindDirTable[index].lower = lower;
}

/* Sets the falloff radius of directional wind `index`; a non-zero `global` sets the global flag (no falloff with distance). */
void tiWindDirSetRadius(u32 index, float radius, int global)
{
    if (index >= DIR_WIND_MAX) {
        assertFailed(D_0068F7B0, D_0068F7C8, 98);
    }
    tiWindDirTable[index].radius = radius;
    if (global != 0) {
        tiWindDirTable[index].flags |= WIND_FLAG_GLOBAL;
    } else {
        tiWindDirTable[index].flags &= ~WIND_FLAG_GLOBAL;
    }
}

/* Enables or disables point wind `index`. */
void tiWindPointSetEnabled(u32 index, int enabled)
{
    if (index >= POINT_WIND_MAX) {
        assertFailed(D_0068F7E0, D_0068F7C8, 112);
    }
    if (enabled != 0) {
        tiWindPointTable[index].flags |= WIND_FLAG_ENABLED;
        return;
    }
    tiWindPointTable[index].flags &= ~WIND_FLAG_ENABLED;
}

/* Sets the position of point wind `index`; the fourth component is the homogeneous 1. */
void tiWindPointSetPosition(u32 index, const float *position)
{
    if (index >= POINT_WIND_MAX) {
        assertFailed(D_0068F7E0, D_0068F7C8, 125);
    }
    tiWindPointTable[index].position[0] = position[0];
    tiWindPointTable[index].position[1] = position[1];
    tiWindPointTable[index].position[2] = position[2];
    tiWindPointTable[index].position[3] = 1.0f;
}

/* Sets the speed of point wind `index`, value[0] of its speed and range triple. */
void tiWindPointSetSpeed(u32 index, float speed)
{
    if (index >= POINT_WIND_MAX) {
        assertFailed(D_0068F7E0, D_0068F7C8, 138);
    }
    if (tiWindPointTable[index].speedRange[0] != speed) {
        tiWindPointTable[index].speedRange[0] = speed;
        tiWindPointTable[index].phaseStep = 1.0f;
    }
}

/* Sets the upper and lower phase bounds of point wind `index`. */
void tiWindPointSetRange(u32 index, float upper, float lower)
{
    PointWindValues *entry;

    if (index >= POINT_WIND_MAX) {
        assertFailed(D_0068F7E0, D_0068F7C8, 152);
    }
    entry = &D_00989F00[index];
    entry->speedRange[1] = upper;
    entry->speedRange[2] = lower;
}

/* Sets the falloff radius of point wind `index`; a non-zero `global` sets the global flag (no falloff with distance). */
void tiWindPointSetRadius(u32 index, float radius, int global)
{
    if (index >= POINT_WIND_MAX) {
        assertFailed(D_0068F7E0, D_0068F7C8, 163);
    }
    tiWindPointTable[index].radius = radius;
    if (global != 0) {
        tiWindPointTable[index].flags |= WIND_FLAG_GLOBAL;
    } else {
        tiWindPointTable[index].flags &= ~WIND_FLAG_GLOBAL;
    }
}

/* The step record and the solver state the wind passes read and write; main/tu263.c owns their full layouts. */
typedef struct WindStep {
    unsigned char unmodeled_00[0x18];
    float time_step;
} WindStep;

typedef struct WindState {
    unsigned char unmodeled_00[0x10];
    Vector4 position;
    unsigned char unmodeled_20[0x14];
    float gust;
} WindState;

extern const char D_0068F7F8[];

/* Pushes the state with a directional wind: a global wind moves it by velocity * time step * gust; otherwise the push
 * falls off with distance over the radius. The gust is redrawn when the phase reaches its upper bound. */
void tiWindDirApply(DirWind *entry, WindStep *step, WindState *state)
{
    float delta[3];
    float dist;
    float falloff;
    int gustRoll;

    if (entry == 0) {
        assertFailed(D_0068F7F8, D_0068F7C8, 177);
    }
    if (entry->phase == entry->upper) {
        gustRoll = xglSRand() % WIND_GUST_STEPS;
        state->gust = (float)gustRoll * 0.01f + 0.5f;
    }
    if (entry->flags & WIND_FLAG_GLOBAL) {
        state->position.x += entry->velocity[0] * step->time_step * state->gust;
        state->position.y += entry->velocity[1] * step->time_step * state->gust;
        state->position.z += entry->velocity[2] * step->time_step * state->gust;
        return;
    }
    delta[0] = state->position.x - entry->position[0];
    delta[1] = state->position.y - entry->position[1];
    delta[2] = state->position.z - entry->position[2];
    dist = __builtin_sqrtf(delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2]);
    if (entry->radius == 0.0f || !(dist <= entry->radius)) {
        return;
    }
    falloff = dist / entry->radius;
    state->position.x += entry->velocity[0] * step->time_step * falloff * state->gust;
    state->position.y += entry->velocity[1] * step->time_step * falloff * state->gust;
    state->position.z += entry->velocity[2] * step->time_step * falloff * state->gust;
}

/* Pushes the state with a point wind: along the unit direction from the wind's position to the state, scaled by the
 * wave strength; a global wind has no falloff with distance. The gust is redrawn when the phase reaches its upper bound. */
void tiWindPointApply(PointWind *entry, WindStep *step, WindState *state)
{
    float delta[4];
    float dist;
    float scale;
    int gustRoll;

    if (entry == 0) {
        assertFailed(D_0068F7F8, D_0068F7C8, 224);
    }
    delta[0] = state->position.x - entry->position[0];
    delta[1] = state->position.y - entry->position[1];
    delta[2] = state->position.z - entry->position[2];
    delta[3] = 1.0f;
    if (entry->phase == entry->speedRange[1]) {
        gustRoll = xglSRand() % WIND_GUST_STEPS;
        state->gust = (float)gustRoll * 0.01f + 0.5f;
    }
    dist = __builtin_sqrtf(delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2]);
    if (dist == 0.0f) {
        dist = 1e-7f;
    }
    scale = 1.0f / dist;
    delta[0] *= scale;
    delta[1] *= scale;
    delta[2] *= scale;
    if (entry->flags & WIND_FLAG_GLOBAL) {
        state->position.x += delta[0] * entry->wave * step->time_step * state->gust;
        state->position.y += delta[1] * entry->wave * step->time_step * state->gust;
        state->position.z += delta[2] * entry->wave * step->time_step * state->gust;
        return;
    }
    if (entry->radius == 0.0f || !(dist <= entry->radius)) {
        return;
    }
    scale = dist / entry->radius;
    state->position.x += delta[0] * entry->wave * step->time_step * scale * state->gust;
    state->position.y += delta[1] * entry->wave * step->time_step * scale * state->gust;
    state->position.z += delta[2] * entry->wave * step->time_step * scale * state->gust;
}


/* Applies every enabled wind to the solver state; the parameters are the step record and the state (void * in the header). */
void tiWindApplyAll(void *first_input, void *second_input)
{
    u32 index;

    for (index = 0; index < DIR_WIND_MAX; index++) {
        if (tiWindDirTable[index].flags & WIND_FLAG_ENABLED) {
            tiWindDirApply(&tiWindDirTable[index], first_input, second_input);
        }
    }
    for (index = 0; index < POINT_WIND_MAX; index++) {
        if (tiWindPointTable[index].flags & WIND_FLAG_ENABLED) {
            tiWindPointApply(&tiWindPointTable[index], first_input, second_input);
        }
    }
}

/* Advances the phase of directional wind `entry`: the phase is clamped to the upper bound (0x4C) and to the lower bound (0x50), reversing the step at each. The velocity is direction * phase * 0.1. */
void tiWindDirAdvance(DirWind *entry)
{
    float nextPhase;

    if (entry == 0) {
        assertFailed(D_0068F7F8, D_0068F7C8, 294);
    }
    nextPhase = entry->phase + entry->speed * entry->phaseStep * 0.1f;
    entry->phase = nextPhase;
    if (nextPhase <= entry->upper) {
        entry->phase = entry->upper;
        entry->phaseStep = 1.0f;
    } else if (entry->lower <= nextPhase) {
        entry->phase = entry->lower;
        entry->phaseStep = -1.0f;
    }
    entry->velocity[0] = entry->direction[0] * entry->phase * 0.1f;
    entry->velocity[1] = entry->direction[1] * entry->phase * 0.1f;
    entry->velocity[2] = entry->direction[2] * entry->phase * 0.1f;
}

/* Advances the phase of point wind `entry`: the phase is clamped to the lower bound (speedRange[1]) and to the upper bound (speedRange[2]), reversing the step at each. */
void tiWindPointAdvance(PointWind *entry)
{
    float nextPhase;

    if (entry == 0) {
        assertFailed(D_0068F7F8, D_0068F7C8, 320);
    }
    nextPhase = entry->phase + entry->speedRange[0] * entry->phaseStep * 0.1f;
    entry->phase = nextPhase;
    if (nextPhase <= entry->speedRange[1]) {
        entry->phase = entry->speedRange[1];
        entry->phaseStep = 1.0f;
    } else if (entry->speedRange[2] <= nextPhase) {
        entry->phase = entry->speedRange[2];
        entry->phaseStep = -1.0f;
    }
    entry->wave = entry->phase * 0.1f;
}


/* Advances the phase of every enabled wind. */
void tiWindAdvanceAll(void)
{
    u32 index;

    for (index = 0; index < DIR_WIND_MAX; index++) {
        if (tiWindDirTable[index].flags & WIND_FLAG_ENABLED) {
            tiWindDirAdvance(&tiWindDirTable[index]);
        }
    }
    for (index = 0; index < POINT_WIND_MAX; index++) {
        if (tiWindPointTable[index].flags & WIND_FLAG_ENABLED) {
            tiWindPointAdvance(&tiWindPointTable[index]);
        }
    }
}

/* Disables every wind; the entries keep their data. */
void tiWindDisableAll(void)
{
    u32 index;

    for (index = 0; index < DIR_WIND_MAX; index++) {
        tiWindDirTable[index].flags &= ~WIND_FLAG_ENABLED;
    }
    for (index = 0; index < POINT_WIND_MAX; index++) {
        tiWindPointTable[index].flags &= ~WIND_FLAG_ENABLED;
    }
}

/* Returns directional wind `index`. */
DirWind *tiWindDirGet(u32 index)
{
    if (index >= DIR_WIND_MAX) {
        assertFailed(D_0068F7B0, D_0068F7C8, 375);
    }
    return &tiWindDirTable[index];
}

/* Returns point wind `index`. */
PointWind *tiWindPointGet(u32 index)
{
    if (index >= POINT_WIND_MAX) {
        assertFailed(D_0068F7E0, D_0068F7C8, 385);
    }
    return &tiWindPointTable[index];
}
