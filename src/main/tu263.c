#include "common.h"
#include "main/TiGlobalColl.h"
#include "main/TiWindBase.h"

INCLUDE_ASM("asm/main/nonmatchings/tu263", XrgQuaternionToMatrix);

INCLUDE_ASM("asm/main/nonmatchings/tu263", xglMatrix2Quaternion);

INCLUDE_ASM("asm/main/nonmatchings/tu263", func_00297580);

INCLUDE_ASM("asm/main/nonmatchings/tu263", func_002976E8);

#include "shared.h"

/* Provisional names: the solver family is not yet attributed to a module. */

/* base_offset locates a palette of 4x4 matrices (0x40 bytes each), indexed by node->index. */
struct TiSolverObject {
    unsigned char unmodeled_00[0x10];
    int flags;
    unsigned char unmodeled_14[0xCC];
    int base_offset;
};

/* extents[] are element counts of three record groups that start at 0xB0 (16 bytes each). */
struct TiSolverNode {
    int index;
    unsigned char unmodeled_04[0x4];
    float normal_scale;
    unsigned char unmodeled_0c[0x4];
    float weight;
    float scale;
    unsigned char unmodeled_18[0x4];
    int extents[3];
    unsigned char unmodeled_28[0x8];
    Vector4 vector_30;
};

/* position is the node's current point; accumulator sums the weighted corrections. */
struct TiSolverState {
    unsigned char unmodeled_00[0x10];
    Vector4 position;
    Vector4 accumulator;
};

void tiDynamicsCollPushOutSphereList(void);
void tiDynamicsCollPushOutMatrixList(struct TiSolverObject *object, struct TiSolverNode *node, struct TiSolverState *state);
void tiDynamicsCollPushOutCapsuleList(struct TiSolverObject *object, struct TiSolverNode *node, struct TiSolverState *state);
void tiGlobalCollClampScale(void *state);

/* Collision pass for one node: the dynamics push-outs (sphere list, matrix list, capsule list), the
 * global collision body and, when object flag 0x80 is set, the scale clamp. Called twice per step. */
void tiDynamicsCollSolveNode(struct TiSolverObject *object, struct TiSolverNode *node, struct TiSolverState *state) {
    tiDynamicsCollPushOutSphereList();
    tiDynamicsCollPushOutMatrixList(object, node, state);
    tiDynamicsCollPushOutCapsuleList(object, node, state);
    tiGlobalCollSolveBody(node, state);
    if (object->flags & 0x80) {
        tiGlobalCollClampScale(state);
    }
}

void xglVectorMulMat(Vector4 *destination, float matrix[4][4], const Vector4 *vector);

/* Pulls the node's point toward its matrix-transformed target, weighted by node->scale. */
void func_002978C8(float (*matrix)[4], struct TiSolverNode *node, struct TiSolverState *state) {
    float transformed[3];

    if (node->scale != 0.0f) {
        xglVectorMulMat(transformed, matrix, &node->vector_30.x);
        state->accumulator.x += (transformed[0] - state->position.x) * node->scale;
        state->accumulator.y += (transformed[1] - state->position.y) * node->scale;
        state->accumulator.z += (transformed[2] - state->position.z) * node->scale;
    }
}

void xglVectorNormal_001A9200(Vector4 *destination, const Vector4 *source);

void func_00297980(float (*matrix)[4], struct TiSolverNode *node, struct TiSolverState *state) {
    float delta[3];
    float unit[3];
    float scaled_x;
    float scaled_y;
    float scaled_z;

    delta[0] = state->position.x - matrix[3][0];
    delta[1] = state->position.y - matrix[3][1];
    delta[2] = state->position.z - matrix[3][2];
    xglVectorNormal_001A9200(unit, delta);

    scaled_x = unit[0] * node->normal_scale;
    state->position.x = scaled_x;
    scaled_y = unit[1] * node->normal_scale;
    state->position.y = scaled_y;
    scaled_z = unit[2] * node->normal_scale;
    state->position.z = scaled_z;

    state->accumulator.x += (scaled_x - delta[0]) * node->weight;
    state->accumulator.y += (scaled_y - delta[1]) * node->weight;
    state->accumulator.z += (scaled_z - delta[2]) * node->weight;

    state->position.x = scaled_x + matrix[3][0];
    state->position.y = scaled_y + matrix[3][1];
    state->position.z = scaled_z + matrix[3][2];
}

INCLUDE_ASM("asm/main/nonmatchings/tu263", func_00297AA0);

INCLUDE_ASM("asm/main/nonmatchings/tu263", func_00297B90);

void func_002976E8(struct TiSolverNode *node, struct TiSolverState *state);

/* One solver step for a node: integrate, wind, pull toward the target, collide, constrain to the
 * normal distance and collide again. The palette matrix of the node is node->index (0x40 bytes each). */
void func_00297CF8(struct TiSolverObject *object, struct TiSolverNode *node, struct TiSolverState *state) {
    func_002976E8(node, state);
    tiWindApplyAll(node, state);
    func_002978C8((float (*)[4])(object->base_offset + (node->index << 6)), node, state);
    tiDynamicsCollSolveNode(object, node, state);
    func_00297980((float (*)[4])(object->base_offset + (node->index << 6)), node, state);
    tiDynamicsCollSolveNode(object, node, state);
}

void func_00297B90(float (*matrix)[4], struct TiSolverNode *node, struct TiSolverState *state);

/* Steps one node, then runs func_00297B90 on the same palette matrix, and returns the pointer just past
 * its three record groups (the groups start at 0xB0 and hold extents[0] + extents[1] + extents[2] entries of 0x10 bytes). */
void *func_00297DA0(int unused_arg0, struct TiSolverObject *object, struct TiSolverNode *node, struct TiSolverState *state)
{
    int extent_sum;
    unsigned char *block;

    func_00297CF8(object, node, state);
    func_00297B90((float (*)[4])(object->base_offset + (node->index << 6)), node, state);
    extent_sum = node->extents[0] + node->extents[1];
    block = (unsigned char *)node + 0xB0;
    extent_sum += node->extents[2];

    return block + extent_sum * 0x10;
}
