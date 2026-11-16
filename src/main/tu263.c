#include "common.h"
#include "main/TiGlobalColl.h"
#include "main/TiMtnBlend.h"
#include "main/TiWindBase.h"
#include "shared.h"

/* TiSolverNode flag bits tested by tiDynamicsNodeSolvePalette: a set bit zeroes that axis of the local position. */
#define TI_NODE_FLAG_ZERO_Y 0x4
#define TI_NODE_FLAG_ZERO_Z 0x2
/* TiSolverObject flag bit tested by tiDynamicsCollSolveNode: clamp the node scale. */
#define TI_OBJECT_FLAG_CLAMP_SCALE 0x80

/* Unit quaternion (x, y, z, w) to a row-major rotation matrix; the last column is zero except m[3][3] = 1. */
void XrgQuaternionToMatrix(Matrix4 matrix, const Vector4 *quaternion) {
    matrix[0][0] = 1.0f - 2.0f * (quaternion->y * quaternion->y + quaternion->z * quaternion->z);
    matrix[0][1] = 2.0f * (quaternion->x * quaternion->y + quaternion->w * quaternion->z);
    matrix[0][2] = 2.0f * (quaternion->x * quaternion->z - quaternion->w * quaternion->y);
    matrix[0][3] = 0.0f;
    matrix[1][0] = 2.0f * (quaternion->x * quaternion->y - quaternion->w * quaternion->z);
    matrix[1][1] = 1.0f - 2.0f * (quaternion->x * quaternion->x + quaternion->z * quaternion->z);
    matrix[1][2] = 2.0f * (quaternion->y * quaternion->z + quaternion->w * quaternion->x);
    matrix[1][3] = 0.0f;
    matrix[2][0] = 2.0f * (quaternion->x * quaternion->z + quaternion->w * quaternion->y);
    matrix[2][1] = 2.0f * (quaternion->y * quaternion->z - quaternion->w * quaternion->x);
    matrix[2][2] = 1.0f - 2.0f * (quaternion->x * quaternion->x + quaternion->y * quaternion->y);
    matrix[2][3] = 0.0f;
    matrix[3][0] = 0.0f;
    matrix[3][1] = 0.0f;
    matrix[3][2] = 0.0f;
    matrix[3][3] = 1.0f;
}

/* Quaternion from the rotation part of a row-major 4x4 matrix (Shepperd's method). */
void xglMatrix2Quaternion(float quat[4], float matrix[4][4]) {
    float trace;
    float s;
    float two_s;
    int i;
    int j;
    int k;
    float *slot;
    int off_i;
    int off_j;
    int off_k;
    int off_larger_diag;
    int off_ji;
    int off_ij;
    int off_ki;
    int off_ik;
    int off_jk;
    int off_kj;
    int quat_off_j;
    int quat_off_k;
    /* Byte-offset float reads kept: the typed matrix[i][j] and quat[j] forms change the code (check.py). */

    trace = matrix[0][0] + matrix[1][1] + matrix[2][2] + matrix[3][3];
    if (trace >= 1.0f) {
        s = __builtin_sqrtf(trace);
        two_s = s + s;
        quat[3] = two_s * 0.25f;
        quat[0] = (matrix[1][2] - matrix[2][1]) / two_s;
        quat[1] = (matrix[2][0] - matrix[0][2]) / two_s;
        quat[2] = (matrix[0][1] - matrix[1][0]) / two_s;
    } else {
        if (matrix[1][1] < matrix[0][0]) {
            i = 0;
        } else {
            i = 1;
        }
        off_larger_diag = 20 * i;
        if (matrix[2][2] > *(float *)((char *)matrix + off_larger_diag)) {
            i = 2;
        }
        j = (i + 1) % 3;
        k = (j + 1) % 3;
        off_i = 20 * i;
        off_j = 20 * j;
        off_k = 20 * k;
        s = __builtin_sqrtf(*(float *)((char *)matrix + off_i) - *(float *)((char *)matrix + off_j) - *(float *)((char *)matrix + off_k) + 1.0f);
        two_s = s + s;
        slot = &quat[i];
        *slot = two_s * 0.25f;
        off_ji = 16 * j + 4 * i;
        off_ij = 16 * i + 4 * j;
        quat_off_j = 4 * j;
        *(float *)((char *)quat + quat_off_j) = (*(float *)((char *)matrix + off_ji) + *(float *)((char *)matrix + off_ij)) / two_s;
        off_ki = 16 * k + 4 * i;
        off_ik = 16 * i + 4 * k;
        quat_off_k = 4 * k;
        *(float *)((char *)quat + quat_off_k) = (*(float *)((char *)matrix + off_ki) + *(float *)((char *)matrix + off_ik)) / two_s;
        off_jk = 16 * j + 4 * k;
        off_kj = 16 * k + 4 * j;
        quat[3] = (*(float *)((char *)matrix + off_jk) - *(float *)((char *)matrix + off_kj)) / two_s;
    }
}

void xglVectorOuter(Vector4 *destination, const Vector4 *left, const Vector4 *right);
void xglVectorInner(float *result, const Vector4 *left, const Vector4 *right);

int XrgQuaternionFromVectors(Vector4 *destination, const Vector4 *left, const Vector4 *right) {
    Vector4 cross;
    float dot;
    float length;
    double abs_length;
    double zero;

    xglVectorOuter(&cross, left, right);
    xglVectorInner(&dot, left, right);
    length = __builtin_sqrtf((dot + 1.0f) * 2.0f);
    zero = 0.0;
    abs_length = length;
    if (abs_length >= zero) {
    } else {
        abs_length = zero - abs_length;
    }
    if (abs_length < 0.01f) {
        cross.y = 1.0f;
        cross.x = 0.0f;
        cross.z = 0.0f;
        destination->y = cross.y;
        destination->x = cross.x;
        destination->z = cross.z;
        destination->w = 0.0f;
        return 0;
    }
    {
        float inverse = 1.0f / length;

        destination->w = 0.5f * length;
        destination->x = cross.x * inverse;
        destination->y = cross.y * inverse;
        destination->z = cross.z * inverse;
    }
    return 1;
}

/* base_offset locates a palette of 4x4 matrices (0x40 bytes each), indexed by node->index. */
struct TiSolverObject {
    unsigned char unmodeled_00[0x10];
    int flags;
    unsigned char unmodeled_14[0xCC];
    int base_offset;
};

/* extents[] are element counts of three record groups that start at 0xB0 (16 bytes each).
 * The first doubleword is read whole by tiDynamicsNodeSolvePalette (flag bits 1-2 of flags). */
struct TiSolverNode {
    union {
        unsigned long long header;
        struct {
            int index;
            unsigned int flags;
        };
    };
    float normal_scale;
    float bias_y;
    float weight;
    float scale;
    unsigned char unmodeled_18[0x4];
    int extents[3];
    unsigned char unmodeled_28[0x4];
    float blend_weight;
    Vector4 vector_30;
};

/* position is the node's current point; accumulator sums the weighted corrections. */
struct TiSolverState {
    unsigned char previous_position[0x10];
    Vector4 position;
    Vector4 accumulator;
};

typedef union { float value; } FloatAlias;

/* Advances one node's point: the previous position is kept in the state's first words, the
 * accumulator is damped to a length of 0.2 when it is longer, then scaled by the node weight. */
void tiDynamicsNodeIntegrate(struct TiSolverNode *node, struct TiSolverState *state) {
    /* Union view kept: a float store through the union changes the code (check.py). */
    FloatAlias *previous = (FloatAlias *)state->previous_position;
    float length;
    float root;
    float delta;

    delta = state->accumulator.y - node->bias_y;
    state->accumulator.y = delta;
    previous[0].value = state->position.x;
    previous[1].value = state->position.y;
    previous[2].value = state->position.z;
    length = state->accumulator.x * state->accumulator.x;
    length = length + delta * delta;
    length = length + state->accumulator.z * state->accumulator.z;
    root = __builtin_sqrtf(length);
    if (root >= 0.2f) {
        float inv = 1.0f / root;

        state->accumulator.y = (state->accumulator.y * inv) * 0.2f;
        state->accumulator.x = (state->accumulator.x * inv) * 0.2f;
        state->accumulator.z = (state->accumulator.z * inv) * 0.2f;
    }
    state->position.x = state->position.x + state->accumulator.x;
    state->position.y = state->position.y + state->accumulator.y;
    state->position.z = state->position.z + state->accumulator.z;
    state->accumulator.x = state->accumulator.x * node->weight;
    state->accumulator.y = state->accumulator.y * node->weight;
    state->accumulator.z = state->accumulator.z * node->weight;
}

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
    if (object->flags & TI_OBJECT_FLAG_CLAMP_SCALE) {
        tiGlobalCollClampScale(state);
    }
}

void xglVectorMulMat(Vector4 *destination, float matrix[4][4], const Vector4 *vector);

/* Pulls the node's point toward its matrix-transformed target, weighted by node->scale. */
void tiDynamicsNodePullToTarget(float (*matrix)[4], struct TiSolverNode *node, struct TiSolverState *state) {
    Vector4 transformed;

    if (node->scale != 0.0f) {
        xglVectorMulMat(&transformed, matrix, &node->vector_30);
        state->accumulator.x += (transformed.x - state->position.x) * node->scale;
        state->accumulator.y += (transformed.y - state->position.y) * node->scale;
        state->accumulator.z += (transformed.z - state->position.z) * node->scale;
    }
}

void xglVectorNormal_001A9200(Vector4 *destination, const Vector4 *source);

void tiDynamicsNodeConstrainNormal(float (*matrix)[4], struct TiSolverNode *node, struct TiSolverState *state) {
    Vector4 delta;
    Vector4 unit;
    float scaled_x;
    float scaled_y;
    float scaled_z;

    delta.x = state->position.x - matrix[3][0];
    delta.y = state->position.y - matrix[3][1];
    delta.z = state->position.z - matrix[3][2];
    xglVectorNormal_001A9200(&unit, &delta);

    scaled_x = unit.x * node->normal_scale;
    state->position.x = scaled_x;
    scaled_y = unit.y * node->normal_scale;
    state->position.y = scaled_y;
    scaled_z = unit.z * node->normal_scale;
    state->position.z = scaled_z;

    state->accumulator.x += (scaled_x - delta.x) * node->weight;
    state->accumulator.y += (scaled_y - delta.y) * node->weight;
    state->accumulator.z += (scaled_z - delta.z) * node->weight;

    state->position.x = scaled_x + matrix[3][0];
    state->position.y = scaled_y + matrix[3][1];
    state->position.z = scaled_z + matrix[3][2];
}

/* Normalizes the xyz part of the first three rows of a matrix in place; a zero length is replaced by 1e-7. */
void tiMatrixNormalizeRows(float (*matrix)[4]) {
    int i = 0;
    float *row_z = matrix[0] + 2;
    while (i < 3) {
        float length;

        length = __builtin_sqrtf(row_z[-2] * row_z[-2] + row_z[-1] * row_z[-1] + row_z[0] * row_z[0]);
        if (length == 0.0f) {
            length = 1.0e-7f;
        }
        length = 1.0f / length;
        row_z[-2] = row_z[-2] * length;
        ((FloatAlias *)row_z)[-1].value = ((FloatAlias *)row_z)[-1].value * length;
        ((FloatAlias *)row_z)[0].value = ((FloatAlias *)row_z)[0].value * length;
        row_z = row_z + 4;
        i++;
    }
}

void xglMatrixInverse(float (*destination)[4], float (*source)[4]);
void xglMatrixMul(float (*destination)[4], float (*left)[4], float (*right)[4]);
/* Solves one node's palette matrix: the inverse-transformed position is pushed through the matrix
 * when the node's flag bits (flags bits 1 and 2, tested together through the 64-bit header) clear v.y or v.z; the result is
 * blended with the rotation built from the normals. Runs only while blend_weight is below 1. */
void tiDynamicsNodeSolvePalette(float (*matrix)[4], struct TiSolverNode *node, struct TiSolverState *state) {
    Matrix4 blended;
    Matrix4 rotation;
    Vector4 local_position;
    Vector4 local_direction;
    Vector4 target_direction;
    Matrix4 inverse;
    Vector4 align_quat;
    float weight;

    if (1.0f <= node->blend_weight) {
        return;
    }
    xglMatrixInverse(inverse, matrix);
    xglVectorMulMat(&local_position, inverse, &state->position);
    if (node->header & ((unsigned long long)(TI_NODE_FLAG_ZERO_Y | TI_NODE_FLAG_ZERO_Z) << 32)) {
        if (node->flags & TI_NODE_FLAG_ZERO_Y) {
            local_position.y = 0.0f;
        }
        if (node->flags & TI_NODE_FLAG_ZERO_Z) {
            local_position.z = 0.0f;
        }
        xglVectorMulMat(&state->position, matrix, &local_position);
    }
    xglVectorNormal_001A9200(&local_direction, &local_position);
    xglVectorNormal_001A9200(&target_direction, &node->vector_30);
    XrgQuaternionFromVectors(&align_quat, &target_direction, &local_direction);
    XrgQuaternionToMatrix(rotation, &align_quat);
    if (0.0f != node->blend_weight) {
        xglMatrixMul(blended, matrix, rotation);
        weight = node->blend_weight;
        tiMtnBlendLerpMatrix(matrix, blended, matrix, weight);
        tiMatrixNormalizeRows(matrix);
    } else {
        xglMatrixMul(matrix, matrix, rotation);
    }
}

/* One solver step for a node: integrate, wind, pull toward the target, collide, constrain to the
 * normal distance and collide again. The palette matrix of the node is node->index (0x40 bytes each). */
void tiDynamicsNodeStep(struct TiSolverObject *object, struct TiSolverNode *node, struct TiSolverState *state) {
    tiDynamicsNodeIntegrate(node, state);
    tiWindApplyAll(node, state);
    tiDynamicsNodePullToTarget((float (*)[4])(object->base_offset + (node->index << 6)), node, state);
    tiDynamicsCollSolveNode(object, node, state);
    tiDynamicsNodeConstrainNormal((float (*)[4])(object->base_offset + (node->index << 6)), node, state);
    tiDynamicsCollSolveNode(object, node, state);
}

/* Steps one node, then runs tiDynamicsNodeSolvePalette on the same palette matrix, and returns the pointer just past
 * its three record groups (the groups start at 0xB0 and hold extents[0] + extents[1] + extents[2] entries of 0x10 bytes). */
void *tiDynamicsNodeSolveNext(int unused_arg0, struct TiSolverObject *object, struct TiSolverNode *node, struct TiSolverState *state)
{
    int extent_sum;
    unsigned char *block;

    tiDynamicsNodeStep(object, node, state);
    tiDynamicsNodeSolvePalette((float (*)[4])(object->base_offset + (node->index << 6)), node, state);
    extent_sum = node->extents[0] + node->extents[1];
    /* Byte pointer kept: the record groups start at 0xB0 inside the node (no typed field). */
    block = (unsigned char *)node + 0xB0;
    extent_sum += node->extents[2];

    return block + extent_sum * 0x10;
}
