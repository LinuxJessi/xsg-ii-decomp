#include "common.h"
#include "shared.h"

/* Record flags bit 0: the record takes no push-out. */
#define TI_DYNAMICS_COLL_RECORD_DISABLED 0x1
/* The accumulator takes this share of each push-out move. */
#define TI_DYNAMICS_COLL_ACCUMULATE_FRACTION 0.2f

/* The solver object: record_set (+0xD0) holds the record offsets; base_offset (+0xE0) locates a palette of 4x4
 * matrices (0x40 bytes each), indexed by record->index. */
struct TiSolverObjectView {
    unsigned char unmodeled_00[0xD0];
    struct TiSolverRecordSet *record_set;
    unsigned char unmodeled_d4[0xC];
    int base_offset;
};

/* One 208-byte entry of the object's record list: index selects the palette matrix, flags bit 0 gates the push-out. */
struct TiSolverMatrixRecord {
    int index;
    unsigned int flags;
    unsigned char unmodeled_08[0x8];
    Matrix4 matrix_10;
    Matrix4 matrix_50;
    Matrix4 matrix_90;
};

void xglVectorMulMat(Vector4 *destination, Matrix4 matrix, const Vector4 *vector);

struct TiVector3 {
    float x;
    float y;
    float z;
};

/* 48-byte sphere record: radius (+4), flags (+8, bit 0 disables the push-out) and centre (+0x20). */
struct TiSphereRecord {
    unsigned char unmodeled_00[0x4];
    float radius;
    unsigned int flags;
    unsigned char unmodeled_0c[0x14];
    struct TiVector3 center;
    unsigned char unmodeled_2c[0x4];
};

/* Pushes a position out of a sphere: the offset from the center must lie inside the box of the radius and inside
 * the sphere; the point moves to the sphere surface along the normalized offset, and the 0.2-weighted correction
 * accumulates into the accumulator. */
void tiDynamicsCollPushOutSphere(Vector4 *position, Vector4 *accumulator, const struct TiSphereRecord *sphere) {
    struct TiVector3 offset;
    struct TiVector3 original;
    float distance;
    float radius;

    offset.x = position->x - sphere->center.x;
    offset.y = position->y - sphere->center.y;
    offset.z = position->z - sphere->center.z;
    radius = sphere->radius + 0.0f;
    if (sphere->flags & TI_DYNAMICS_COLL_RECORD_DISABLED) {
        return;
    }
    if (!(__builtin_fabsf(offset.x) < radius)) {
        return;
    }
    if (!(__builtin_fabsf(offset.y) < radius)) {
        return;
    }
    if (!(__builtin_fabsf(offset.z) < radius)) {
        return;
    }
    distance = __builtin_sqrtf(offset.x * offset.x + offset.y * offset.y + offset.z * offset.z);
    if (distance == 0.0f) {
        distance = 1e-6f;
    }
    if (!(distance < radius)) {
        return;
    }
    distance = 1.0f / distance;
    original.x = position->x;
    original.y = position->y;
    original.z = position->z;
    offset.x = offset.x * distance;
    offset.y = offset.y * distance;
    offset.z = offset.z * distance;
    position->x = offset.x * radius + sphere->center.x;
    position->y = offset.y * radius + sphere->center.y;
    position->z = offset.z * radius + sphere->center.z;
    accumulator->x += (position->x - original.x) * TI_DYNAMICS_COLL_ACCUMULATE_FRACTION;
    accumulator->y += (position->y - original.y) * TI_DYNAMICS_COLL_ACCUMULATE_FRACTION;
    accumulator->z += (position->z - original.z) * TI_DYNAMICS_COLL_ACCUMULATE_FRACTION;
}

/* Pushes one position out of the ground plane: transform by matrix_90 then matrix_50; when the point is below
 * y = 0 and the record is not flagged, project it to y = 0 and transform by matrix_10 and the palette matrix,
 * then accumulate the 0.2-weighted correction and store the point back. */
void tiDynamicsCollClampAbovePlane(Vector4 *position, Vector4 *accumulator, struct TiSolverMatrixRecord *record, struct TiSolverObjectView *object) {
    Vector4 point;

    point.x = position->x;
    point.y = position->y;
    point.z = position->z;
    point.w = 1.0f;
    xglVectorMulMat(&point, record->matrix_90, &point);
    xglVectorMulMat(&point, record->matrix_50, &point);
    if (record->flags & TI_DYNAMICS_COLL_RECORD_DISABLED) {
        return;
    }
    if (point.y < 0.0f) {
        point.y = 0.0f;
        xglVectorMulMat(&point, record->matrix_10, &point);
        xglVectorMulMat(&point, *(Matrix4 *)(object->base_offset + (record->index << 6)), &point);
        accumulator->x += (point.x - position->x) * TI_DYNAMICS_COLL_ACCUMULATE_FRACTION;
        accumulator->y += (point.y - position->y) * TI_DYNAMICS_COLL_ACCUMULATE_FRACTION;
        accumulator->z += (point.z - position->z) * TI_DYNAMICS_COLL_ACCUMULATE_FRACTION;
        position->x = point.x;
        position->y = point.y;
        position->z = point.z;
    }
}

/* Moves the point onto the sphere of the given radius around center when each offset component is below the
 * radius and the length is below it (one-sided tests, as in the original): the offset is normalized and scaled
 * by radius. */
void tiDynamicsCollPushPointOutOfSphere(Vector4 *point, const Vector4 *center, float radius) {
    struct TiVector3 delta;
    struct TiVector3 offset;
    float distance;
    int i;

    offset.x = point->x - center->x;
    offset.y = point->y - center->y;
    offset.z = point->z - center->z;
    delta = offset;
    if (!(delta.x < radius)) {
        return;
    }
    if (!(delta.y < radius)) {
        return;
    }
    if (!(delta.z < radius)) {
        return;
    }
    distance = __builtin_sqrtf(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
    if (!(distance < radius)) {
        return;
    }
    if (distance == 0.0f) {
        distance = 1e-7f;
    }
    distance = 1.0f / distance;
    for (i = 0; i < 3; i++) {
        float *offset_axis = &delta.x + i;
        const float *center_axis = &center->x + i;
        float *point_axis = &point->x + i;

        *offset_axis = *offset_axis * distance;
        *point_axis = *offset_axis * radius + *center_axis;
    }
}

/* The record set at object 0xD0: sphere_base_offset (0x4C) and matrix_base_offset (0x50) locate the sphere and
 * matrix record arrays from the set's own address. */
struct TiSolverRecordSet {
    unsigned char unmodeled_00[0x4C];
    int sphere_base_offset;
    int matrix_base_offset;
    int capsule_base_offset;
};

/* A capsule record: index selects a 64-byte palette matrix, flags bit 0 disables the push-out, radius and height
 * size the ring and the two end points. matrix_a0 and matrix_60 take the point into the capsule frame (in that
 * order), matrix_20 maps it back out before the palette matrix. */
struct TiCapsuleRecord {
    int index;
    unsigned int flags;
    unsigned char unmodeled_08[0x8];
    float radius;
    float height;
    unsigned char unmodeled_18[0x8];
    Matrix4 matrix_20;
    Matrix4 matrix_60;
    Matrix4 matrix_a0;
};

/* One 16-byte entry of a node's list: record_index selects a record of the object's record set (the sphere,
 * matrix or capsule group, in that order). */
struct TiSolverEntry {
    int record_index;
    unsigned char unmodeled_04[0xC];
};

/* sphere_count (0x1C) and matrix_count (0x20) bound the sphere and matrix entry groups; the entries start at 0xB0
 * and the matrix group follows the sphere group. */
struct TiSolverNodeView {
    unsigned char unmodeled_00[0x1C];
    unsigned int sphere_count;
    unsigned int matrix_count;
    unsigned int capsule_count;
    unsigned char unmodeled_28[0x88];
    struct TiSolverEntry entries[0];
};

/* A body of the pair solvers: position and accumulator are DoublewordVector4 union views, node is the body's
 * node list. */
struct TiSolverBodyView {
    unsigned char unmodeled_00[0x10];
    DoublewordVector4 position;
    DoublewordVector4 accumulator;
    struct TiSolverNodeView *node;
};

extern Vector4 tiDynamicsCollCapsuleCenter;

void tiDynamicsCollPushOutCapsule(Vector4 *position, Vector4 *accumulator, struct TiCapsuleRecord *capsule, struct TiSolverObjectView *object) {
    Vector4 point;
    float distance;
    float radius;
    double absHeight;

    point.w = 1.0f;
    point.x = position->x;
    point.y = position->y;
    point.z = position->z;
    xglVectorMulMat(&point, capsule->matrix_a0, &point);
    xglVectorMulMat(&point, capsule->matrix_60, &point);
    if (capsule->flags & TI_DYNAMICS_COLL_RECORD_DISABLED) {
        return;
    }
    tiDynamicsCollCapsuleCenter.y = capsule->height;
    tiDynamicsCollPushPointOutOfSphere(&point, &tiDynamicsCollCapsuleCenter, capsule->radius);
    tiDynamicsCollCapsuleCenter.y = -capsule->height;
    tiDynamicsCollPushPointOutOfSphere(&point, &tiDynamicsCollCapsuleCenter, capsule->radius);
    absHeight = point.y;
    if (absHeight < 0.0) {
        absHeight = 0.0 - absHeight;
    }
    if (!(absHeight < capsule->height + 0.0f)) {
        return;
    }
    distance = __builtin_sqrtf(point.x * point.x + point.z * point.z);
    radius = capsule->radius + 0.0f;
    if (!(distance < radius)) {
        return;
    }
    if (distance == 0.0f) {
        distance = 1e-7f;
    }
    distance = 1.0f / distance;
    point.x = point.x * distance * radius;
    point.z = point.z * distance * radius;
    xglVectorMulMat(&point, capsule->matrix_20, &point);
    xglVectorMulMat(&point, *(Matrix4 *)(object->base_offset + capsule->index * 64), &point);
    accumulator->x += (point.x - position->x) * TI_DYNAMICS_COLL_ACCUMULATE_FRACTION;
    accumulator->y += (point.y - position->y) * TI_DYNAMICS_COLL_ACCUMULATE_FRACTION;
    accumulator->z += (point.z - position->z) * TI_DYNAMICS_COLL_ACCUMULATE_FRACTION;
    position->x = point.x;
    position->y = point.y;
    position->z = point.z;
}

/* position and accumulator are the Vector4 pair that the push-outs read and write. */
struct TiSolverStateView {
    unsigned char unmodeled_00[0x10];
    Vector4 position;
    Vector4 accumulator;
};

void tiDynamicsCollPushOutSphereList(struct TiSolverObjectView *object, struct TiSolverNodeView *node, struct TiSolverStateView *state) {
    struct TiSphereRecord *records;
    struct TiSolverEntry *entry;
    unsigned int i;

    if (node->sphere_count == 0) {
        return;
    }
    entry = node->entries;
    records = (struct TiSphereRecord *)((unsigned char *)object->record_set + object->record_set->sphere_base_offset);
    for (i = 0; i < node->sphere_count; i++, entry++) {
        tiDynamicsCollPushOutSphere(&state->position, &state->accumulator, &records[entry->record_index]);
    }
}

void tiDynamicsCollPushOutMatrixList(struct TiSolverObjectView *object, struct TiSolverNodeView *node, struct TiSolverStateView *state) {
    struct TiSolverMatrixRecord *records;
    struct TiSolverEntry *entry;
    unsigned int i;

    if (node->matrix_count == 0) {
        return;
    }
    records = (struct TiSolverMatrixRecord *)((unsigned char *)object->record_set + object->record_set->matrix_base_offset);
    entry = &node->entries[node->sphere_count];
    for (i = 0; i < node->matrix_count; i++, entry++) {
        tiDynamicsCollClampAbovePlane(&state->position, &state->accumulator, &records[entry->record_index], object);
    }
}

void tiDynamicsCollPushOutCapsuleList(struct TiSolverObjectView *object, struct TiSolverNodeView *node, struct TiSolverBodyView *state) {
    struct TiCapsuleRecord *records;
    struct TiSolverRecordSet *record_set;
    struct TiSolverEntry *entry;
    unsigned int i;

    if (node->capsule_count == 0) {
        return;
    }
    record_set = object->record_set;
    records = (struct TiCapsuleRecord *)((unsigned char *)record_set + record_set->capsule_base_offset);
    entry = node->entries + node->sphere_count + node->matrix_count;
    for (i = 0; i < node->capsule_count; i++) {
        tiDynamicsCollPushOutCapsule(&state->position.vector, &state->accumulator.vector, &records[entry->record_index], object);
        entry++;
    }
}

void tiDynamicsCollSolveSpherePair(struct TiSolverBodyView *first, struct TiSolverBodyView *second, struct TiSolverObjectView *object) {
    DoublewordVector4 midpoint;
    DoublewordVector4 correction;
    struct TiSphereRecord *records;
    struct TiSolverEntry *entry;
    unsigned int i;
    int record_index;
    struct TiSolverRecordSet *record_set;

    if (first->node->sphere_count == 0) {
        return;
    }
    entry = first->node->entries;
    record_set = object->record_set;
    records = (struct TiSphereRecord *)((unsigned char *)record_set + record_set->sphere_base_offset);
    midpoint.vector.w = 1.0f;
    correction.vector.w = 1.0f;
    midpoint.vector.x = (second->position.vector.x - first->position.vector.x) * 0.5f + first->position.vector.x;
    correction.vector.x = 0.0f;
    midpoint.vector.y = (second->position.vector.y - first->position.vector.y) * 0.5f + first->position.vector.y;
    correction.vector.y = 0.0f;
    midpoint.vector.z = (second->position.vector.z - first->position.vector.z) * 0.5f + first->position.vector.z;
    correction.vector.z = 0.0f;
    for (i = 0; i < first->node->sphere_count; i++) {
        correction.vector.x = 0.0f;
        correction.vector.y = 0.0f;
        correction.vector.z = 0.0f;
        record_index = entry->record_index;
        entry++;
        tiDynamicsCollPushOutSphere(&midpoint.vector, &correction.vector, &records[record_index]);
        first->position.vector.x += correction.vector.x;
        first->position.vector.y += correction.vector.y;
        first->position.vector.z += correction.vector.z;
        first->accumulator.vector.x += correction.vector.x;
        first->accumulator.vector.y += correction.vector.y;
        first->accumulator.vector.z += correction.vector.z;
        second->position.vector.x += correction.vector.x;
        second->position.vector.y += correction.vector.y;
        second->position.vector.z += correction.vector.z;
        second->accumulator.vector.x += correction.vector.x;
        second->accumulator.vector.y += correction.vector.y;
        second->accumulator.vector.z += correction.vector.z;
    }
}

void tiDynamicsCollSolveMatrixPair(struct TiSolverBodyView *first, struct TiSolverBodyView *second, struct TiSolverObjectView *object) {
    DoublewordVector4 midpoint;
    DoublewordVector4 correction;
    struct TiSolverEntry *entry;
    struct TiSolverMatrixRecord *records;
    int processed;
    unsigned int sphereCount;
    struct TiSolverRecordSet *record_set;
    unsigned int record_index;

    if (first->node->matrix_count == 0) {
        return;
    }
    processed = 0;
    record_set = object->record_set;
    records = (struct TiSolverMatrixRecord *)((unsigned char *)record_set + record_set->matrix_base_offset);
    sphereCount = first->node->sphere_count;
    entry = &first->node->entries[sphereCount];
    midpoint.vector.w = 1.0f;
    correction.vector.w = 1.0f;
    correction.vector.x = 0.0f;
    correction.vector.y = 0.0f;
    correction.vector.z = 0.0f;
    midpoint.vector.x = (second->position.vector.x - first->position.vector.x) * 0.5f + first->position.vector.x;
    midpoint.vector.y = (second->position.vector.y - first->position.vector.y) * 0.5f + first->position.vector.y;
    midpoint.vector.z = (second->position.vector.z - first->position.vector.z) * 0.5f + first->position.vector.z;
    if (first->node->matrix_count != 0) {
        do {
            processed++;
            correction.vector.x = 0.0f;
            correction.vector.y = 0.0f;
            correction.vector.z = 0.0f;
            record_index = entry->record_index;
            entry++;
            tiDynamicsCollClampAbovePlane(&midpoint.vector, &correction.vector, &records[record_index], object);
            first->position.vector.x = first->position.vector.x + correction.vector.x;
            first->position.vector.y = first->position.vector.y + correction.vector.y;
            first->position.vector.z = first->position.vector.z + correction.vector.z;
            first->accumulator.vector.x = first->accumulator.vector.x + correction.vector.x;
            first->accumulator.vector.y = first->accumulator.vector.y + correction.vector.y;
            first->accumulator.vector.z = first->accumulator.vector.z + correction.vector.z;
            second->position.vector.x = second->position.vector.x + correction.vector.x;
            second->position.vector.y = second->position.vector.y + correction.vector.y;
            second->position.vector.z = second->position.vector.z + correction.vector.z;
            second->accumulator.vector.x = second->accumulator.vector.x + correction.vector.x;
            second->accumulator.vector.y = second->accumulator.vector.y + correction.vector.y;
            second->accumulator.vector.z = second->accumulator.vector.z + correction.vector.z;
        } while (processed < first->node->matrix_count);
    }
}

void tiDynamicsCollSolveCapsulePair(struct TiSolverBodyView *first, struct TiSolverBodyView *second, struct TiSolverObjectView *object) {
    DoublewordVector4 midpoint;
    DoublewordVector4 correction;
    struct TiCapsuleRecord *records;
    struct TiSolverEntry *entry;
    unsigned int i;
    struct TiSolverRecordSet *record_set;

    if (first->node->capsule_count == 0) {
        return;
    }
    entry = &(&first->node->entries[first->node->sphere_count])[first->node->matrix_count];
    record_set = object->record_set;
    records = (struct TiCapsuleRecord *)((unsigned char *)record_set + record_set->capsule_base_offset);
    midpoint.vector.w = 1.0f;
    correction.vector.w = 1.0f;
    midpoint.vector.x = (second->position.vector.x - first->position.vector.x) * 0.5f + first->position.vector.x;
    correction.vector.x = 0.0f;
    midpoint.vector.y = (second->position.vector.y - first->position.vector.y) * 0.5f + first->position.vector.y;
    correction.vector.y = 0.0f;
    midpoint.vector.z = (second->position.vector.z - first->position.vector.z) * 0.5f + first->position.vector.z;
    correction.vector.z = 0.0f;
    for (i = 0; i < first->node->capsule_count; i++) {
        correction.vector.x = 0.0f;
        correction.vector.y = 0.0f;
        correction.vector.z = 0.0f;
        tiDynamicsCollPushOutCapsule(&midpoint.vector, &correction.vector, &records[entry->record_index], object);
        entry++;
        first->position.vector.x += correction.vector.x;
        first->position.vector.y += correction.vector.y;
        first->position.vector.z += correction.vector.z;
        first->accumulator.vector.x += correction.vector.x;
        first->accumulator.vector.y += correction.vector.y;
        first->accumulator.vector.z += correction.vector.z;
        second->position.vector.x += correction.vector.x;
        second->position.vector.y += correction.vector.y;
        second->position.vector.z += correction.vector.z;
        second->accumulator.vector.x += correction.vector.x;
        second->accumulator.vector.y += correction.vector.y;
        second->accumulator.vector.z += correction.vector.z;
    }
}
