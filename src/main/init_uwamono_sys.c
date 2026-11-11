#include "common.h"
#include "shared.h"

typedef struct UwamonoRecord UwamonoRecord;
typedef struct UwamonoHitQuery UwamonoHitQuery;
/* Number of MapUnit[] slots (map_2.h's MapUnit table). */
#define MAP_UNIT_COUNT 64

int HitCheckMapUnitPos(const Vector4 *position);
extern void xglVectorLength(float *destination, const Vector4 *vector);

/* MapUnitRecord.flags */
enum {
    MAP_UNIT_NO_DRAW = 0x4,             /* the model is not drawn */
    MAP_UNIT_ACTIVE = 0x10000,          /* the slot is live: updated and collidable */
    MAP_UNIT_LANDED = 0x80000,          /* a falling unit has come to rest (uwamonoCheckHeight) */
    MAP_UNIT_BROKEN = 0x20000,          /* a breakable that has been destroyed */
    MAP_UNIT_SELECTABLE = 0x40000,      /* currently a valid shoot/aim target */
    MAP_UNIT_HIDDEN = 0x100000,         /* excluded from collision (the unit it stands in for is inactive) */
    MAP_UNIT_SAVE_SYMBOL = 0x20000000,  /* save point symbol */
    MAP_UNIT_SHOP_SYMBOL = 0x40000000   /* shop symbol */
};

/* Collision shape of a unit (state byte, crossType, KowareKind shape). */
enum {
    UWAMONO_SHAPE_CIRCLE = 1,
    UWAMONO_SHAPE_BOX = 2
};

/* Record mode: which collision test a record gets (uwamonoHitCheckRecordAt and uwamonoCrossPointAt dispatch on it). */
enum {
    UWAMONO_MODE_BOX = 1,
    UWAMONO_MODE_CIRCLE = 2
};

/* Records of uwamonoRecords the scans visit. */
#define UWAMONO_RECORD_COUNT 0x100
/* Move and turn steps one record holds (the moveX/turnX tables). */
#define UWAMONO_STEP_COUNT 0x20

int HitCheckMapUnitPosSize(const Vector4 *position, float radius);


float CheckDist2D(const Vector4 *first, const Vector4 *second);

#include "vu0_macro.h"
/* Actors collide against a unit at 1.5 times their radius. */
#define UWAMONO_RADIUS_SCALE 1.5f

/*
 * The unit's 4x4 world matrix (MapUnitRecord.basis) read as rows: x axis,
 * y axis, z axis and anchor (translation); the checks copy it whole.
 */
typedef struct UwamonoCrossBasis {
    union {
        struct {
            float xAxisX;
            unsigned char unmodeled_04[4];
            float xAxisZ;
            unsigned char unmodeled_0c[0x14];
            float zAxisX;
            unsigned char unmodeled_24[4];
            float zAxisZ;
            unsigned char unmodeled_2c[4];
            float anchor[4];
        };
        float rows[4][4]; /* the same matrix as MapUnitRecord.basis rows */
    };
} UwamonoCrossBasis;

typedef struct UwamonoMapSlot UwamonoMapSlot;

typedef struct UwamonoBoxBasis {
    float x;
    unsigned char unmodeled_04[4];
    float z;
} UwamonoBoxBasis;

struct UwamonoMapSlot {
    unsigned int flags;
    void (*update)(void *);
    void (*draw)(void *);
    unsigned char unmodeled_0c[4];
    AlignedVector4 position;
    float rotationX, rotationY, rotationZ;
    unsigned char unmodeled_2c[0x14];
    Matrix4 localMatrix;
    union {
        struct UwamonoBoxBasis *basis;
        Matrix4 *matrixView;
    };
    unsigned char unmodeled_84[0x1e];
    unsigned char actionSub;
    unsigned char linkedUnitIndex;
    short serial;
    unsigned char unmodeled_a6[0x0a];
    AlignedVector4 dimensions;
    unsigned char unmodeled_c0[0x28];
    int effectCf[3];
    unsigned char unmodeled_f4[0xac];
    struct UwamonoMapCollisionState {
        short kindIndex;
        unsigned char unmodeled_02[2];
        signed char signal;
        unsigned char unmodeled_05[0x28];
        signed char crossType;
        signed char hideOverride;
        unsigned char unmodeled_2f[0x131];
    } state;
};

extern UwamonoMapSlot MapUnit[];
extern const float mapUnitPosHitHeightScale;
extern const float mapUnitPosHitBelowMargin;
/* Box corner test of an actor against a unit: the box test decides, and a 2 result also needs the corner test. */ int HitCheckBoxCorner(UwamonoRecord *actor, UwamonoRecord *unit);
unsigned int HitCheckMapUnitPosAt(UwamonoRecord *query, UwamonoRecord *record);
int HitCheckCorner(UwamonoRecord *query, UwamonoRecord *record);
void CheckNearPoint(const Vector4 *origin, const Vector4 *candA,
                    const Vector4 *candB, Vector4 *out);

/* Per-entry transform that uwamonoUpdateTransform writes its translation into (+0x264 of UwamonoRecord). */
typedef struct UwamonoTransform {
    Matrix4 matrix;                     /* the rows; row 3 (+0x30) is the translation: x, y, z */
} UwamonoTransform;

struct UwamonoRecord {
    short id;                    /* +0x00: zeroed by uwamonoClearMoveResetWord and uwamonoClearRotateResetWord */
    unsigned char kind;                 /* +0x02: the scans match kind 2 */
    unsigned char unmodeled_03;
    int key;                            /* +0x04: compared by uwamonoHitCheckOther */
    unsigned char unmodeled_08[4];
    float pendingValue;                 /* +0x0c: stored by uwamonoSetPendingValue while uwamonoDrawDisabled is 0 */
    AlignedVector4 position;            /* +0x10: x, y (+0x14), z */
    unsigned char unmodeled_20[0x14];
    float fallSpeed;                    /* +0x34: vertical velocity, written by uwamonoCheckHeight */
    unsigned char unmodeled_38[0x08];
    AlignedVector4 rotation;              /* +0x40: anchor.y at +0x44 */
    unsigned char unmodeled_50[0x60];
    float radius;                       /* +0xb0: added to other->extent.x by uwamonoNyuruCircle */
    unsigned char unmodeled_b4[0x1b0];
    /* The word view gives the pointer load alias set 0 (uwamonoCalcNearCrossPointBox; style debt). */
    union {
        UwamonoTransform *transform;    /* +0x264 */
        int transformWord;
    };
    unsigned char unmodeled_268[0x88];
    UwamonoCrossBasis matrix;            /* +0x2f0: the unit's cross basis (x axis, anchor) */
    unsigned char unmodeled_330[0x10];
    float moveX[0x20];                  /* +0x340: x of each move step, written by uwamonoAddMoveStep */
    float moveY[0x20];                  /* +0x3c0: y of each move step */
    short moveStepCount;                /* +0x440: steps added so far (below 32) */
    unsigned char unmodeled_442[2];
    float turnX[0x20];                  /* +0x444: x of each turn step, written by uwamonoAddRotateStep */
    float turnY[0x20];                  /* +0x4c4: y of each turn step */
    short turnStepCount;                /* +0x544: turn steps added so far (below 32) */
    short target;                       /* +0x546: -1 when unset */
    unsigned char unmodeled_548[0x28];
    int flags;                          /* +0x570: bit 0x2 read by the scans, bits 0x2 and 0x8 by uwamonoUpdateTransform */
    int landedFlags;                    /* +0x574: MAP_UNIT_LANDED set by uwamonoCheckHeight */
    unsigned char unmodeled_578[0x08];
    AlignedVector4 extent;              /* +0x580: x read by uwamonoNyuruCircle */
    signed char mode;                   /* +0x590: 0 when inactive; 1 and 2 dispatch in uwamonoCrossPointAt */
    unsigned char unmodeled_591[0x1af];
    unsigned char hitBuffer[8];         /* +0x740: address passed to mapMoveProbe */
    unsigned short hitFlags;            /* +0x748: |= 0x1000 by uwamonoNyuruCircle */
    unsigned char unmodeled_74a[0x126];
};

extern UwamonoRecord uwamonoRecords[];

/* Appends the move step (x, y) to the record's move table; returns 0 when the table is full. */
int uwamonoAddMoveStep(int index, float x, float y)
{
    UwamonoRecord *record = &uwamonoRecords[index];
    int count = record->moveStepCount;

    if (count >= UWAMONO_STEP_COUNT) {
        return 0;
    }
    record->moveX[count] = x;
    record->moveStepCount = record->moveStepCount + 1;
    record->moveY[count] = y;
    return 1;
}

/* Appends the turn step (x, y) to the record's turn table; returns 0 when the table is full. */
int uwamonoAddRotateStep(int index, float x, float y)
{
    UwamonoRecord *record = &uwamonoRecords[index];
    int count = record->turnStepCount;

    if (count >= UWAMONO_STEP_COUNT) {
        return 0;
    }
    record->turnX[count] = x;
    record->turnStepCount = record->turnStepCount + 1;
    record->turnY[count] = y;
    return 1;
}

extern UwamonoRecord uwamonoMoveRecords[];

/* Zeroes the first word of one entry of the move reset table (setStepMove calls this). */
void uwamonoClearMoveResetWord(int index) {
    uwamonoMoveRecords[index].id = 0;
}

extern UwamonoRecord uwamonoRotateRecords[];

/* Same as uwamonoClearMoveResetWord for the rotate reset table (setStepRotate calls this). */
void uwamonoClearRotateResetWord(int index) {
    uwamonoRotateRecords[index].id = 0;
}

int uwamonoHitCheckRecordAt(UwamonoHitQuery *query, UwamonoRecord *record);

/* Returns the index of the first active kind-2 record that the query hits, or -1. */
int uwamonoHitCheckRecord(UwamonoHitQuery *query) {
    int index;

    for (index = 0; index < UWAMONO_RECORD_COUNT; index++) {
        UwamonoRecord *record = &uwamonoRecords[index];

        if (record->kind == 2 && (record->flags & 2) && record->mode != 0
            && uwamonoHitCheckRecordAt(query, record) != 0) {
            return index;
        }
    }
    return -1;
}

int uwamonoHitCheckNyuru(UwamonoRecord *record, UwamonoRecord *other);
int uwamonoNyuruMatrix(UwamonoRecord *record, UwamonoRecord *other, int skipSnap);

/*
 * A saved pose copy. Its stores go through a union member (alias set 0), so
 * they stay ahead of the hit-list loads in the loop, as in the original.
 */
typedef union UwamonoPoseSave {
    AlignedVector4 vector;
} UwamonoPoseSave;

/*
 * Collects the kind-2 records that the record hits (index list ended by -1), then
 * pushes the record out of each of them, restoring its pose between the pushes.
 */
int uwamonoHitCheckWithNyuru(UwamonoRecord *record)
{
    int hits[256];
    int hitCount;
    int index;
    int slot;
    UwamonoPoseSave savedPosition;
    UwamonoPoseSave savedAnchor;

    for (index = 0; index < 256; index++) {
        hits[index] = -1;
    }
    hitCount = 0;
    for (index = 0; index < UWAMONO_RECORD_COUNT; index++) {
        UwamonoRecord *other = &uwamonoRecords[index];

        if (other->kind == 2 && !(other->flags & 0x02000000) && other->mode != 0
            && uwamonoHitCheckRecordAt((UwamonoHitQuery *)record, other) != 0) {
            hits[hitCount++] = index;
        }
    }
    if (hitCount == 0) {
        return 0;
    }
    if (hitCount == 1) {
        return uwamonoHitCheckNyuru(record, &uwamonoRecords[hits[0]]);
    }
    for (slot = 0; hits[slot] != -1; slot++) {
        UwamonoRecord *other;

        savedPosition.vector = record->position;
        savedAnchor.vector = record->rotation;
        other = &uwamonoRecords[hits[slot]];
        switch (other->mode) {
        case UWAMONO_MODE_BOX:
            uwamonoNyuruMatrix(record, other, 1);
            break;
        case 2:
            uwamonoNyuruCircle(record, other);
            break;
        }
        if (uwamonoHitCheckRecord((UwamonoHitQuery *)record) == -1) {
            return 1;
        }
        record->position = savedPosition.vector;
        record->rotation = savedAnchor.vector;
    }
    record->position.x -= record->rotation.x;
    record->position.z -= record->rotation.z;
    record->rotation.x = 0.0f;
    record->rotation.z = 0.0f;
    return 0;
}

/*
 * HitCheckMapUnitPos (main:0x002c02f0): a thin tail call into
 * HitCheckMapUnitPosSize with its distance threshold fixed at 0.0f (a
 * true `j`, no `jal`, so this function's own return value is whatever
 * HitCheckMapUnitPosSize returns).
 */
int HitCheckMapUnitPos(const Vector4 *position) {
    return HitCheckMapUnitPosSize(position, 0.0f);
}

void xglMatrixStackUnit(void);
void xglMatrixStackRotY(float angle);
void xglMatrixStackMulVector(Vector4 *destination, const Vector4 *source);
float xglSin(float angle);
float xglCos(float angle);
float atan2f(float y, float x);

struct UwamonoHitQuery {
    unsigned char unmodeled_00[0x10];
    Vector4 position;                   /* +0x10 */
    unsigned char unmodeled_20[0x90];
    float radius;                       /* +0xb0 */
    float positionYBias;                /* +0xb4: added to position.y by uwamonoHitCheckRecordAt */
    unsigned char unmodeled_b8[0x7f0 - 0xb8];
};

int HitCheckMapUnitPosSize(const Vector4 *position, float radius)
{
    UwamonoHitQuery query;
    int index;

    query.position = *position;
    query.radius = radius;
    for (index = 0; index < UWAMONO_RECORD_COUNT; index++) {
        UwamonoRecord *record = &uwamonoRecords[index];

        if (record->kind < 2 || record->kind == 7 || (record->flags & 0x2000000) || record->mode == 0) {
            continue;
        }
        if (HitCheckMapUnitPosAt((UwamonoRecord *)&query, record)) {
            return index;
        }
    }
    return -1;
}

float Get_Distance(const Vector4 *from, const Vector4 *to);
int uwamonoHitCheckBox(UwamonoRecord *query, UwamonoRecord *record);
extern const float D_0069B064;

int uwamonoHitCheckRecordAt(UwamonoHitQuery *query, UwamonoRecord *record)
{
    if (record->position.y + record->extent.y * D_0069B064 < query->position.y) {
        return 0;
    }
    if (query->position.y + query->positionYBias < record->position.y) {
        return 0;
    }
    switch (record->mode) {
    case UWAMONO_MODE_BOX:
        return uwamonoHitCheckBox((UwamonoRecord *)query, record);
    case 2:
        return Get_Distance(&query->position, &record->position) < query->radius + record->extent.x;
    default:
        return 0;
    }
}

unsigned int HitCheckMapUnitPosAt(UwamonoRecord *query, UwamonoRecord *record)
{
    if (record->position.y + record->extent.y * 0.9f < query->position.y) {
        return 0;
    }
    if (query->position.y < record->position.y - 1.8f) {
        return 0;
    }
    if (record->mode != UWAMONO_MODE_BOX) {
        if (record->mode == UWAMONO_MODE_CIRCLE) {
            if (!(CheckDist2D(&query->position, &record->position) < query->radius + record->extent.x)) {
                return 0;
            }
            return 1;
        }
        return 0;
    }
    return HitCheckBoxCorner(query, record);
}

int uwamonoPointInBox(const Vector4 *from, const Vector4 *position, const Vector4 *extent, float radius, float angle)
{
    Vector4 delta;
    Vector4 rotated;

    xglMatrixStackUnit();
    xglMatrixStackRotY(-angle);
    delta.x = from->x - position->x;
    delta.y = 0.0f;
    delta.z = from->z - position->z;
    delta.w = 1.0f;
    xglMatrixStackMulVector(&rotated, &delta);
    {
        int inside = 1;

        if (__builtin_fabsf(rotated.x) > extent->x + radius || __builtin_fabsf(rotated.z) > extent->z + radius) {
            inside = 0;
        }
        return inside;
    }
}

int uwamonoHitCheckBox(UwamonoRecord *query, UwamonoRecord *record)
{
    DoublewordAlignedVector4 extent;
    UwamonoCrossBasis *basis = &record->matrix;
    Vector4 anchor;
    float deltaX;
    float deltaZ;
    float value;
    float zRadius;
    float major;
    float axisX;
    float axisZ;
    float limit;
    float minor;
    float scaled;

    extent = record->extent;
    axisZ = basis->xAxisZ;
    axisX = basis->xAxisX;
    deltaX = query->position.x - basis->anchor[0];
    deltaZ = query->position.z - basis->anchor[2];
    value = deltaX * axisX + deltaZ * axisZ;
    anchor.x = basis->anchor[0];
    anchor.y = basis->anchor[1];
    anchor.z = basis->anchor[2];
    anchor.w = 1.0f;
    if (extent.x + query->radius < __builtin_fabsf(value)) {
        return 0;
    }
    zRadius = extent.z + query->radius;
    major = query->position.x - (value * axisX + anchor.x);
    minor = query->position.z - (value * axisZ + anchor.z);
    value = major * major + minor * minor;
    limit = zRadius * zRadius;
    if (!(value < limit)) {
        return 0;
    }
    value = __builtin_sqrtf(query->rotation.x * query->rotation.x + query->rotation.z * query->rotation.z)
            * __builtin_sqrtf(deltaX * deltaX + deltaZ * deltaZ);
    major = extent.x;
    minor = extent.z;
    if (extent.x < extent.z) {
        major = extent.z;
        minor = extent.x;
    }
    scaled = (minor / major) * 0.3f;
    limit = 1.0f - scaled;
    if (value > 1e-5f) {
        value = (query->rotation.x * deltaX + query->rotation.z * deltaZ) / value;
    }
    if (!(limit < value)) {
        return 1;
    }
    return 0;
}

/* Box corner test of an actor against a unit: the box test decides, and a 2 result also needs the corner test. */
int HitCheckBoxCorner(UwamonoRecord *actor, UwamonoRecord *unit)
{
    int hit = uwamonoPointInBox(&actor->position, &unit->position, &unit->extent, actor->radius, unit->rotation.y);

    if (hit == 0) {
        return 0;
    }
    if (hit == 2) {
        return HitCheckCorner(actor, unit) != 0;
    }
    return 1;
}

float sqrtf(float x);

int HitCheckCorner(UwamonoRecord *query, UwamonoRecord *record)
{
    Vector4 corner[4];
    DoublewordAlignedVector4 position;
    DoublewordAlignedVector4 extent;
    float (*transform)[4];

    transform = (*(UwamonoTransform **) &record->transform)->matrix;
    position = record->position;
    extent = record->extent;

    corner[0].x = position.x + (extent.x * transform[0][0] + extent.z * transform[2][0]);
    corner[0].y = position.y;
    corner[0].z = position.z + (extent.x * transform[0][2] + extent.z * transform[2][2]);
    if (CheckDist2D(&query->position, &corner[0]) < query->radius) {
        return 1;
    }
    corner[1].x = position.x - (extent.z * transform[0][2] + extent.x * transform[2][2]);
    corner[1].y = position.y;
    corner[1].z = position.z + (extent.z * transform[0][0] + extent.x * transform[2][0]);
    if (CheckDist2D(&query->position, &corner[1]) < query->radius) {
        return 1;
    }
    corner[2].x = position.x - (extent.x * transform[0][0] + extent.z * transform[2][0]);
    corner[2].y = position.y;
    corner[2].z = position.z - (extent.x * transform[0][2] + extent.z * transform[2][2]);
    if (CheckDist2D(&query->position, &corner[2]) < query->radius) {
        return 1;
    }
    corner[3].x = position.x + (extent.z * transform[0][2] + extent.x * transform[2][2]);
    corner[3].y = position.y;
    corner[3].z = position.z - (extent.z * transform[0][0] + extent.x * transform[2][0]);
    if (CheckDist2D(&query->position, &corner[3]) < query->radius) {
        return 1;
    }
    return 0;
}

extern const float D_0069B068;
extern const float D_0069B06C;
int uwamonoNyuruCircle(UwamonoRecord *record, UwamonoRecord *other);

int uwamonoHitCheckNyuru(UwamonoRecord *record, UwamonoRecord *other)
{
    DoublewordAlignedVector4 savedPosition;
    DoublewordAlignedVector4 savedAnchor;
    int hit;

    savedPosition = record->position;
    savedAnchor = record->rotation;
    hit = -1;
    switch (other->mode) {
    case UWAMONO_MODE_CIRCLE:
        if (uwamonoNyuruCircle(record, other) == 0) {
            return 0;
        }
        hit = uwamonoHitCheckRecord((UwamonoHitQuery *)record);
        break;
    case UWAMONO_MODE_BOX:
        if (uwamonoNyuruMatrix(record, other, 0) == 0) {
            return 0;
        }
        hit = uwamonoHitCheckRecord((UwamonoHitQuery *)record);
        break;
    }
    if (hit != -1) {
    record->position = savedPosition;
    record->rotation = savedAnchor;
    record->position.x = record->position.x - record->rotation.x;
    record->rotation.x = 0.0f;
    record->position.z = record->position.z - record->rotation.z;
    record->rotation.z = 0.0f;
        return 0;
    }
    return 1;
}

int uwamonoPushOutOfCircle(Vector4 *from, Vector4 *to, const Vector4 *other, float radius)
{
    Vector4 delta;
    float length;
    float angle;
    float sine;
    float cosine;
    float z;

    delta.x = from->x - other->x;
    delta.y = 0.0f;
    delta.z = from->z - other->z;
    xglVectorLength(&length, &delta);
    if (radius <= length) {
        return 1;
    }
    length = radius + 0.01f;
    angle = atan2f(delta.x, delta.z);
    sine = xglSin(angle);
    delta.x = other->x + length * sine;
    cosine = xglCos(angle);
    z = other->z + length * cosine;
    from->x = from->x - to->x;
    delta.z = z;
    from->z = from->z - to->z;
    to->x = delta.x - from->x;
    to->z = z - from->z;
    return 0;
}

float mapMoveProbe(const Vector4 *from, const Vector4 *to, void *result);

/*
 * Circle-shaped record against another record: returns 1 when the other record's
 * circle does not block it; otherwise it lands the record on the ground height
 * mapMoveProbe reports and returns 1, or 0 when there is no ground.
 */
int uwamonoNyuruCircle(UwamonoRecord *record, UwamonoRecord *other) {
    float height;

    if (uwamonoPushOutOfCircle(&record->position, &record->rotation, &other->position,
                      record->radius + other->extent.x) != 0) {
        return 1;
    }
    record->hitFlags |= 0x1000;
    height = mapMoveProbe(&record->position, &record->rotation, record->hitBuffer);
    if (height == -1000.0f) {
        return 0;
    }
    record->position.y = height;
    return 1;
}

extern const float D_0069B07C;
extern const float D_0069B080;

int uwamonoNyuruMatrix(UwamonoRecord *record, UwamonoRecord *other, int skipSnap)
{
    DoublewordVector4 extent;
    Vector4 dir;
    UwamonoTransform matrixCopy;
    float (*transform)[4];
    float length;
    float dx;
    float dz;
    float dot;
    float projection;
    float height;

    transform = other->transform->matrix;
    matrixCopy = *other->transform;
    extent.words[0] = ((u64 *)&other->extent)[0];
    extent.words[1] = ((u64 *)&other->extent)[1];
    record->position.x -= record->rotation.x;
    record->position.z -= record->rotation.z;
    dx = record->position.x - other->position.x;
    dz = record->position.z - other->position.z;
    dot = dx * transform[0][0] + dz * transform[0][2];
    length = __builtin_sqrtf(record->rotation.x * record->rotation.x + record->rotation.z * record->rotation.z);
    if (length == 0.0f) {
        return 0;
    }
    dir.x = record->rotation.x / length;
    dir.y = 0.0f;
    dir.z = record->rotation.z / length;
    if (__builtin_fabsf(dot) < extent.vector.x + record->radius) {
        projection = dir.x * transform[0][0] + dir.z * transform[0][2];
        record->rotation.x = projection * transform[0][0] * length;
        record->rotation.z = projection * transform[0][2] * length;
        if (__builtin_fabsf(record->rotation.x) < D_0069B07C) {
            if (__builtin_fabsf(record->rotation.z) < D_0069B07C && !skipSnap) {
                if (extent.vector.x < dot) {
                    record->rotation.x = transform[0][0] * length * 0.5f;
                    record->rotation.z = transform[0][2] * length * 0.5f;
                } else if (dot < -extent.vector.x) {
                    record->rotation.x = -transform[0][0] * length * 0.5f;
                    record->rotation.z = -transform[0][2] * length * 0.5f;
                }
            }
        }
    } else {
        projection = dir.x * transform[2][0] + dir.z * transform[2][2];
        record->rotation.x = projection * transform[2][0] * length;
        record->rotation.z = projection * transform[2][2] * length;
        if (__builtin_fabsf(record->rotation.x) < D_0069B080) {
            if (__builtin_fabsf(record->rotation.z) < D_0069B080 && !skipSnap) {
                dot = dx * transform[2][0] + dz * transform[2][2];
                if (extent.vector.z < dot) {
                    record->rotation.x = transform[2][0] * length * 0.5f;
                    record->rotation.z = transform[2][2] * length * 0.5f;
                } else if (dot < -extent.vector.z) {
                    record->rotation.x = -transform[2][0] * length * 0.5f;
                    record->rotation.z = -transform[2][2] * length * 0.5f;
                }
            }
        }
    }
    record->hitFlags |= 0x1000;
    height = mapMoveProbe(&record->position, &record->rotation, record->hitBuffer);
    if (height == -1000.0f) {
        record->position.x -= record->rotation.x;
        record->position.z -= record->rotation.z;
        return 0;
    } else {
        record->position.y = height;
        return 1;
    }
}

int uwamonoAreaCheck(const Vector4 *from, UwamonoRecord *record, float radius);
int uwamonoCrossPointAt(const Vector4 *first, const Vector4 *second, UwamonoRecord *record, AlignedVector4 *third);
int CheckPointLine(const Vector4 *from, const Vector4 *to, const Vector4 *point);
void xglVectorNormal_001A9200(Vector4 *destination, const Vector4 *vector);

int uwamonoCrossPoint(const Vector4 *origin, const Vector4 *target, AlignedVector4 *out, float radius)
{
    int hitIndex[256];
    AlignedVector4 startLeft;
    AlignedVector4 endLeft;
    AlignedVector4 startRight;
    AlignedVector4 endRight;
    Vector4 dir;
    AlignedVector4 crossPoint;
    UwamonoRecord *record;
    float length;
    float limit;
    float lenSq;
    float dot;
    float sideX;
    float sideZ;
    int count;
    int i;
    int leftSide;
    int rightSide;
    int off;

    dir.x = origin->x - target->x;
    dir.z = origin->z - target->z;
    dir.y = 0.0f;
    dir.w = 1.0f;
    limit = dir.x * dir.x + dir.y + dir.z * dir.z;
    limit = __builtin_sqrtf(limit) + radius;

    for (i = 0; i < 256; i++) {
        hitIndex[i] = -1;
    }

    count = 0;
    for (i = 0; i < UWAMONO_RECORD_COUNT; i++) {
        record = &uwamonoRecords[i];
        if (record->kind != 2 || (record->flags & 0x2) == 0) {
            continue;
        }
        if (__builtin_fabsf(origin->y - record->position.y) > 0.3f) {
            continue;
        }
        if (record->mode != 0 && uwamonoAreaCheck(origin, record, limit)) {
            hitIndex[count++] = i;
        }
    }

    out->x = 65535.0f;
    out->y = -1000.0f;
    out->z = 65535.0f;
    if (count == 0) {
        *out = *target;
        return 0;
    }

    xglVectorNormal_001A9200(&dir, &dir);

    startLeft = *origin;
    endLeft = *target;
    startRight = *origin;
    endRight = *target;
    startLeft.w = 1.0f;
    endLeft.w = 1.0f;
    startRight.w = 1.0f;
    endRight.w = 1.0f;
    sideX = radius * dir.z;
    sideZ = radius * dir.x;
    startLeft.x = startLeft.x + sideX;
    startLeft.z = startLeft.z - sideZ;
    endLeft.x = endLeft.x + sideX;
    endLeft.z = endLeft.z - sideZ;
    startRight.x = startRight.x - sideX;
    startRight.z = startRight.z + sideZ;
    endRight.x = endRight.x - sideX;
    endRight.z = endRight.z + sideZ;

    for (i = 0; i < 256; i++) {
        UwamonoRecord *hitRecord;

        if (hitIndex[i] == -1) {
            break;
        }
        hitRecord = &uwamonoRecords[hitIndex[i]];
        uwamonoCrossPointAt(&startLeft, &endLeft, hitRecord, &crossPoint);
        if (crossPoint.y != -1000.0f) {
            CheckNearPoint(origin, &crossPoint, out, out);
        }
        uwamonoCrossPointAt(&startRight, &endRight, hitRecord, &crossPoint);
        if (crossPoint.y != -1000.0f) {
            CheckNearPoint(origin, &crossPoint, out, out);
        }
        crossPoint.x = origin->x - hitRecord->position.x;
        crossPoint.z = origin->z - hitRecord->position.z;
        if (dir.x * crossPoint.x + dir.z * crossPoint.z > 0.0f) {
            leftSide = CheckPointLine(&startLeft, &endLeft, &hitRecord->position);
            rightSide = CheckPointLine(&startRight, &endRight, &hitRecord->position);
            if (leftSide != rightSide) {
                uwamonoCrossPointAt(origin, &hitRecord->position, hitRecord, &crossPoint);
                CheckNearPoint(origin, &crossPoint, out, out);
            }
        }
    }

    if (out->y == -1000.0f) {
        *out = *target;
        return 0;
    }

    endLeft.x = origin->x - out->x;
    endLeft.y = origin->y;
    endLeft.z = origin->z - out->z;
    limit = dir.x * endLeft.x + dir.z * endLeft.z;
    limit = limit - radius;
    if (limit < 0.0f) {
        *out = *origin;
        return 1;
    }
    out->x = origin->x - limit * dir.x;
    out->y = origin->y;
    out->z = origin->z - limit * dir.z;
    return 1;
}

int uwamonoCalcNearCrossPointCircle(const Vector4 *origin, const Vector4 *target, UwamonoRecord *record, AlignedVector4 *out);
int uwamonoCalcNearCrossPointBox(const Vector4 *origin, const Vector4 *target, UwamonoRecord *record, AlignedVector4 *out);

/* Near-crossing calculation for one record, chosen by its mode: 1 = box, 2 = circle; any other mode returns 2. */
int uwamonoCrossPointAt(const Vector4 *first, const Vector4 *second, UwamonoRecord *record, AlignedVector4 *third) {
    if (record->mode != UWAMONO_MODE_BOX) {
        if (record->mode == UWAMONO_MODE_CIRCLE) {
            return uwamonoCalcNearCrossPointCircle(first, second, record, third);
        }
        return 2;
    }
    return uwamonoCalcNearCrossPointBox(first, second, record, third);
}

/* Declared without a prototype: uwamonoCrossCheckRecordAt passes three arguments (a tail jump), the definition takes four. */

int uwamonoCalcNearCrossPointCircle(const Vector4 *origin, const Vector4 *target, UwamonoRecord *record, AlignedVector4 *out)
{
    AlignedVector4 dir;
    AlignedVector4 normal;
    float length;

    if (CrossCheckMapUnitCircle(origin, target, record, out) != 0) {
        dir = *origin;
        dir.y = record->position.y;
        Vu0SubVectorXYZ(&dir, &record->position, origin);
        xglVectorLength(&length, &dir);
        xglVectorNormal_001A9200(&normal, &dir);
        length = length - record->extent.x;
        if (length < 0.0f) {
            *out = *origin;
            return;
        }
        out->x = origin->x + length * normal.x;
        out->y = origin->y;
        out->z = origin->z + length * normal.z;
    } else {
        out->y = -1000.0f;
    }
}

int CheckCrossLine(const Vector4 *origin, const Vector4 *target, const Vector4 *from, const Vector4 *to);
void CalcCrossPoint(const Vector4 *origin, const Vector4 *target, const Vector4 *from, const Vector4 *to, AlignedVector4 *hit);

int uwamonoCalcNearCrossPointBox(const Vector4 *origin, const Vector4 *target, UwamonoRecord *record, AlignedVector4 *out)
{
    Vector4 corner[4];
    AlignedVector4 position;
    AlignedVector4 extent;
    AlignedVector4 hit;
    float (*matrix)[4];
    Vector4 *edge1;
    float noCross;

    edge1 = &corner[1];
    noCross = -1000.0f;
    matrix = record->transform->matrix;
    position = record->position;
    extent = record->extent;
    out->y = noCross;
    hit.y = origin->y;

    corner[0].x = position.x + (extent.x * matrix[0][0] + extent.z * matrix[2][0]);
    corner[0].y = position.y;
    corner[0].z = position.z + (extent.x * matrix[0][2] + extent.z * matrix[2][2]);
    corner[1].x = position.x - (extent.z * matrix[0][2] + extent.x * matrix[2][2]);
    corner[1].y = position.y;
    corner[1].z = position.z + (extent.z * matrix[0][0] + extent.x * matrix[2][0]);
    corner[2].x = position.x - (extent.x * matrix[0][0] + extent.z * matrix[2][0]);
    corner[2].y = position.y;
    corner[2].z = position.z - (extent.x * matrix[0][2] + extent.z * matrix[2][2]);
    corner[3].x = position.x + (extent.z * matrix[0][2] + extent.x * matrix[2][2]);
    corner[3].y = position.y;
    corner[3].z = position.z - (extent.z * matrix[0][0] + extent.x * matrix[2][0]);

    if (CheckCrossLine(origin, target, &corner[0], edge1) != 0) {
        CalcCrossPoint(origin, target, &corner[0], edge1, &hit);
        *out = hit;
    }
    if (CheckCrossLine(origin, target, edge1, &corner[2]) != 0) {
        CalcCrossPoint(origin, target, edge1, &corner[2], &hit);
        if (out->y != -1000.0f) {
            CheckNearPoint(origin, &hit, out, out);
        } else {
            *out = hit;
        }
    }
    if (CheckCrossLine(origin, target, &corner[2], &corner[3]) != 0) {
        CalcCrossPoint(origin, target, &corner[2], &corner[3], &hit);
        if (out->y != -1000.0f) {
            CheckNearPoint(origin, &hit, out, out);
        } else {
            *out = hit;
        }
    }
    if (CheckCrossLine(origin, target, &corner[3], &corner[0]) != 0) {
        CalcCrossPoint(origin, target, &corner[3], &corner[0], &hit);
        if (out->y != -1000.0f) {
            CheckNearPoint(origin, &hit, out, out);
        } else {
            *out = hit;
        }
    }
}

void CheckNearPoint(const Vector4 *origin, const Vector4 *candA, const Vector4 *candB, Vector4 *out)
{
    Vector4 delta;
    float lenA;
    float lenB;

    Vu0SubVectorXYZ(&delta, origin, candA);
    xglVectorLength(&lenA, &delta);

    Vu0SubVectorXYZ(&delta, origin, candB);
    xglVectorLength(&lenB, &delta);

    if (lenA < lenB)
        *out = *candA;
    else
        *out = *candB;
}

int uwamonoAreaCheckBox(const Vector4 *from, UwamonoRecord *record, float radius);
/* Area test of a record against a point: the height band of the record, then its box or circle. */
extern float D_0069B08C;
extern float D_0069B090;

int uwamonoAreaCheck(const Vector4 *from, UwamonoRecord *record, float radius)
{
    if (record->position.y + record->extent.y * D_0069B08C < from->y) {
        return 0;
    }
    if (from->y < record->position.y - D_0069B090) {
        return 0;
    }
    if (record->mode != UWAMONO_MODE_BOX) {
        if (record->mode == UWAMONO_MODE_CIRCLE) {
            if (CheckDist2D(from, &record->position) < radius + record->extent.x) {
                return 1;
            }
        }
    } else {
        if (uwamonoAreaCheckBox(from, record, radius)) {
            return 1;
        }
    }
    return 0;
}

/* Box area test of a record (the record's extent and anchor height) against a point with a radius. */
int uwamonoAreaCheckBox(const Vector4 *from, UwamonoRecord *record, float radius) {
    return uwamonoPointInBox(from, &record->position, &record->extent, radius, record->rotation.y) != 0;
}

/* Tests one record against the check point: far in y is a miss; mode 1 is a box, 2 a circle, other modes never hit. */ int uwamonoCrossCheckRecordAt(const AlignedVector4 *checkPosition, const AlignedVector4 *exclude, UwamonoRecord *record);

int uwamonoCrossCheck(const AlignedVector4 *checkPosition, UwamonoRecord *exclude)
{
    UwamonoRecord *record;
    int index;

    record = uwamonoRecords;
    for (index = 0; index < UWAMONO_RECORD_COUNT; index++) {
        if (record->kind == 2 && !(record->flags & 0x02000000) && record != exclude && record->mode != 0
            && uwamonoCrossCheckRecordAt(checkPosition, &exclude->position, record) != 0) {
            return index;
        }
        record++;
    }
    return -1;
}

int CrossCheckMapUnitBox(const Vector4 *origin, const Vector4 *target, UwamonoRecord *record);

/* Tests one record against the check point: far in y is a miss; mode 1 is a box, 2 a circle, other modes never hit. */
int uwamonoCrossCheckRecordAt(const AlignedVector4 *checkPosition, const AlignedVector4 *exclude, UwamonoRecord *record)
{
    float dy = checkPosition->y - record->position.y;

    if (__builtin_fabsf(dy) > 0.6f) {
        return 0;
    }
    if (record->mode != 1) {
        if (record->mode != 2) {
            return 0;
        }
        return CrossCheckMapUnitCircle(checkPosition, exclude, record);
    }
    return CrossCheckMapUnitBox(checkPosition, exclude, record);
}

int CrossCheckMapUnitCircle(const Vector4 *origin, const Vector4 *target, UwamonoRecord *record, AlignedVector4 *out)
{
    Vector4 mid;
    Vector4 normal;
    float radius;
    float halfLength;
    float dot;
    float dx;
    float dz;
    float oz;
    float ox;
    float distSq;
    float px;
    float pz;

    normal.x = target->x - origin->x;
    normal.y = 0.0f;
    normal.z = target->z - origin->z;
    dx = normal.x;
    dz = normal.z;
    radius = record->extent.x;
    halfLength = __builtin_sqrtf(dx * dx + dz * dz) * 0.5f;
    xglVectorNormal_001A9200(&normal, &normal);
    ox = origin->x;
    oz = origin->z;
    mid.x = ox + (target->x - ox) * 0.5f;
    mid.z = oz + (target->z - oz) * 0.5f;
    dx = record->position.x - mid.x;
    dz = record->position.z - mid.z;
    dot = dx * normal.x + dz * normal.z;
    if (halfLength + radius < __builtin_fabsf(dot)) {
        return 0;
    }
    px = dot * normal.x + mid.x;
    pz = dot * normal.z + mid.z;
    dx = record->position.x - px;
    dz = record->position.z - pz;
    distSq = dx * dx + dz * dz;
    if (distSq < radius * radius) {
        return 1;
    }
    return 0;
}

extern float D_0069B098;

int CrossCheckMapUnitBox(const Vector4 *origin, const Vector4 *target, UwamonoRecord *record)
{
    Vector4 corner[4];
    AlignedVector4 position;
    DoublewordVector4 extent;
    float (*matrix)[4];

    extent.words[1] = ((const u64 *)&record->extent)[1];
    extent.words[0] = ((const u64 *)&record->extent)[0];
    extent.vector.z = extent.vector.z + D_0069B098;
    extent.vector.x = extent.vector.x + D_0069B098;
    position = record->position;
    matrix = record->transform->matrix;

    corner[0].x = position.x + (extent.vector.x * matrix[0][0] + extent.vector.z * matrix[2][0]);
    corner[0].y = position.y;
    corner[0].z = position.z + (extent.vector.x * matrix[0][2] + extent.vector.z * matrix[2][2]);
    corner[1].x = position.x - (extent.vector.z * matrix[0][2] + extent.vector.x * matrix[2][2]);
    corner[1].y = position.y;
    corner[1].z = position.z + (extent.vector.z * matrix[0][0] + extent.vector.x * matrix[2][0]);

    if (CheckCrossLine(origin, target, &corner[0], &corner[1]) != 0) {
        return 1;
    }

    corner[2].x = position.x - (extent.vector.x * matrix[0][0] + extent.vector.z * matrix[2][0]);
    corner[2].y = position.y;
    corner[2].z = position.z - (extent.vector.x * matrix[0][2] + extent.vector.z * matrix[2][2]);

    if (CheckCrossLine(origin, target, &corner[1], &corner[2]) != 0) {
        return 1;
    }

    corner[3].x = position.x + (extent.vector.z * matrix[0][2] + extent.vector.x * matrix[2][2]);
    corner[3].y = position.y;
    corner[3].z = position.z - (extent.vector.z * matrix[0][0] + extent.vector.x * matrix[2][0]);

    if (CheckCrossLine(origin, target, &corner[2], &corner[3]) != 0) {
        return 1;
    }

    return CheckCrossLine(origin, target, &corner[3], &corner[0]) != 0;
}

/* One slot of uwamonoSlotTable: the record of the object it tracks. */
typedef struct UwamonoSlot {
    UwamonoRecord *object;              /* +0x00 */
    unsigned char unmodeled_04[0x86c];
} UwamonoSlot;

/*
 * The probe block of mapMoveProbe (uwamonoCheckHeight and uwamonoCanSeeRecord pass one):
 * two cleared words, the attribute mask, a flag byte, the range, the ground height and
 * work words. The +0x28 word is set as a float by uwamonoCanSeeRecord (zeroed28 is its int view).
 */
typedef struct UwamonoProbeRequest {
    int clearedA;                       /* +0x00 */
    int clearedB;                       /* +0x04 */
    union { unsigned short mask; float maskBits; };  /* +0x08 */
    unsigned char unmodeled_0C[3];
    unsigned char flag;                 /* +0x0F */
    float range;                        /* +0x10 */
    unsigned char unmodeled_14[4];
    int ground;                         /* +0x18 */
    unsigned int scratchWork;           /* +0x1C */
    unsigned char unmodeled_20[0x08];
    union { float word; int zeroed28; }; /* +0x28 */
    unsigned char unmodeled_2C[0x14];
} UwamonoProbeRequest;

extern int uwamonoCurrentIndex;
extern UwamonoSlot uwamonoSlotTable[];
extern UwamonoProbeRequest uwamonoProbeTemplate;

int uwamonoCanSeeRecord(UwamonoRecord *record)
{
    UwamonoRecord *object;
    UwamonoProbeRequest request;
    DoublewordAlignedVector4 position;
    Vector4 delta;

    object = uwamonoSlotTable[uwamonoCurrentIndex].object;
    if (uwamonoCrossCheck(&object->position, record) != -1) {
        return 0;
    }
    request = uwamonoProbeTemplate;
    position = object->position;
    delta.x = record->position.x - object->position.x;
    delta.y = record->position.y - object->position.y;
    delta.z = record->position.z - object->position.z;
    delta.w = 1.0f;
    request.mask = (request.mask & 0xFFF8) | 0x803;
    request.flag = 0;
    request.word = 0.0f;
    mapMoveProbe(&position, 0, &request);
    if (mapMoveProbe(&position, &delta, &request) == -1000.0f) {
        return 0;
    }
    if (__builtin_fabsf(position.x - record->position.x) < 0.01f
        && __builtin_fabsf(position.z - record->position.z) < 0.01f) {
        return 1;
    }
    return 0;
}

void uwamonoCheckHeight(UwamonoRecord *record);
extern int D_002B5D98[];

/* Copies the record position into its transform translation when the record is visible and the global gate is clear. */
void uwamonoUpdateTransform(UwamonoRecord *record)
{
    int flags;
    UwamonoTransform *transform;

    flags = record->flags;
    if ((flags & 2) && !(D_002B5D98[0] & 1)) {
        if ((flags & 8) && record->target == -1) {
            uwamonoCheckHeight(record);
        }
        transform = record->transform;
        transform->matrix[3][0] = record->position.x;
        transform->matrix[3][1] = record->position.y;
        transform->matrix[3][2] = record->position.z;
    }
}

/* The fall constants of gameWork: gravity at +0x374, bounce divisor at +0x37c. */
typedef struct UwamonoFallConstants {
    unsigned char unmodeled_000[0x374];
    float gravity;
    unsigned char unmodeled_378[0x04];
    float bounceDivisor;
} UwamonoFallConstants;

extern UwamonoFallConstants gameWork;
void mapProbeInit(UwamonoProbeRequest *probe);
int mapFindBlockEntry(int key, int index);
int uwamonoHitCheckOther(UwamonoRecord *record);

/* Falls a unit under gravity until it lands on the map or on another unit. */
void uwamonoCheckHeight(UwamonoRecord *record)
{
    UwamonoProbeRequest probe;
    float velocity;
    float groundY;
    float newY;
    int hitIndex;

    mapProbeInit(&probe);
    probe.ground = mapFindBlockEntry(256, -1);
    probe.mask = 0;
    groundY = mapMoveProbe(&record->position, 0, &probe);
    if (groundY == -1000.0f) {
        return;
    }
    if (!(record->flags & 8)) {
        return;
    }
    velocity = record->fallSpeed;
    for (;;) {
        if (velocity > -0.6f) {
            velocity = record->fallSpeed = velocity - gameWork.gravity;
        }
        newY = record->position.y + velocity;
        record->position.y = newY;
        if (newY < groundY) {
            record->position.y = groundY + 0.001f;
            if ((*(unsigned long long *)&record->flags & 0x10000010ULL) == 0) {
                record->fallSpeed = -velocity / gameWork.bounceDivisor;
            } else {
                record->fallSpeed = 0.0f;
            }
            if (record->fallSpeed < 0.05f) {
                record->landedFlags |= MAP_UNIT_LANDED;
                record->fallSpeed = 0.0f;
            }
        } else {
            hitIndex = uwamonoHitCheckOther(record);
            if (hitIndex != -1) {
                record->position.y = uwamonoRecords[hitIndex].position.y
                    + uwamonoRecords[hitIndex].extent.y + 0.001f;
                if ((*(unsigned long long *)&record->flags & 0x10000010ULL) == 0) {
                    record->fallSpeed = -record->fallSpeed / gameWork.bounceDivisor;
                } else {
                    record->fallSpeed = 0.0f;
                }
                if (record->fallSpeed < 0.1f) {
                    record->fallSpeed = 0.0f;
                }
            }
        }
        if (!(record->flags & 0x10)) {
            return;
        }
        if (record->fallSpeed == 0.0f) {
            break;
        }
        velocity = record->fallSpeed;
    }
}

int uwamonoHitCheckOtherAt(UwamonoRecord *record, UwamonoRecord *candidate);

int uwamonoHitCheckOther(UwamonoRecord *record)
{
    int index;

    for (index = 0; index < UWAMONO_RECORD_COUNT; index++) {
        UwamonoRecord *candidate = &uwamonoRecords[index];

        if (candidate->kind == 2 && candidate->key != record->key
            && (candidate->flags & 2) && candidate->mode != 0
            && uwamonoHitCheckOtherAt(record, candidate) != 0) {
            return index;
        }
    }
    return -1;
}

extern float D_0069B0B0;
int uwamonoHitCheckOtherBox(UwamonoRecord *record, UwamonoRecord *other);

int uwamonoHitCheckOtherAt(UwamonoRecord *record, UwamonoRecord *candidate)
{
    if (record->position.y < candidate->position.y) {
        return 0;
    }
    if (candidate->position.y + candidate->extent.y < record->position.y) {
        return 0;
    }
    switch (candidate->mode) {
    case UWAMONO_MODE_CIRCLE:
        if (CheckDist2D(&record->position, &candidate->position) < candidate->extent.x + D_0069B0B0) {
            return 1;
        }
        return 0;
    case UWAMONO_MODE_BOX:
        return uwamonoHitCheckOtherBox(record, candidate);
    default:
        return 0;
    }
}

extern float D_0069B0B4;

int uwamonoHitCheckOtherBox(UwamonoRecord *record, UwamonoRecord *other) {
    return uwamonoPointInBox(&record->position, &other->position, &other->extent, D_0069B0B4, other->rotation.y) != 0;
}

extern float D_0069B0B8;
extern signed char uwamonoDrawDisabled;
void DispPillar(Vector4 *source, float radius, float lift, int color);

/*
 * Builds the debug outline of every active record and draws nothing: a box record (mode 1) gets the four bottom
 * corners of its footprint and their copies raised by the extent height, a circle record (mode 2) goes to
 * DispPillar. The Xenosaga II form of Xenosaga I's DispCollUwamono. Does nothing while uwamonoDrawDisabled is set.
 */
void DispCollUwamono(void)
{
    DoublewordVector4 bottom[4];
    DoublewordVector4 top[4];
    AlignedVector4 position;
    DoublewordAlignedVector4 size;
    int colors[8];
    float cornerW;
    int i;

    if (uwamonoDrawDisabled == 0) {
        colors[0] = colors[1] = 0;
        colors[2] = 255;
        colors[3] = 128;
        colors[4] = 255;
        colors[5] = colors[6] = 0;
        colors[7] = 128;
        cornerW = D_0069B0B8;
        for (i = 0; i < UWAMONO_RECORD_COUNT; i++) {
            UwamonoRecord *record = &uwamonoRecords[i];
            UwamonoRecord *unit;
            float (*rotation)[4];

            if (record->kind < 2 || record->kind == 4 || record->kind == 5 || record->kind == 6
                || record->kind == 3 || record->kind == 7) {
                continue;
            }
            if (record->extent.x == 0.0f || record->extent.y == 0.0f || record->extent.z == 0.0f) {
                continue;
            }
            if (record->mode == 0) {
                continue;
            }
            unit = &uwamonoRecords[i];
            rotation = unit->matrix.rows;
            position.x = rotation[3][0];
            position.y = rotation[3][1];
            position.z = rotation[3][2];
            position.w = 1.0f;
            size = unit->extent;
            switch (record->mode) {
            case UWAMONO_MODE_BOX:
                bottom[0].vector.x = position.x + (size.x * rotation[0][0] + size.z * rotation[2][0]);
                bottom[0].vector.y = position.y;
                bottom[0].vector.z = position.z + (size.x * rotation[0][2] + size.z * rotation[2][2]);
                bottom[1].vector.x = position.x - (size.z * rotation[0][2] + size.x * rotation[2][2]);
                bottom[1].vector.y = position.y;
                bottom[1].vector.z = position.z + (size.z * rotation[0][0] + size.x * rotation[2][0]);
                bottom[2].vector.x = position.x - (size.x * rotation[0][0] + size.z * rotation[2][0]);
                bottom[2].vector.y = position.y;
                bottom[2].vector.z = position.z - (size.x * rotation[0][2] + size.z * rotation[2][2]);
                bottom[3].vector.x = position.x + (size.z * rotation[0][2] + size.x * rotation[2][2]);
                bottom[3].vector.y = position.y;
                bottom[3].vector.z = position.z - (size.z * rotation[0][0] + size.x * rotation[2][0]);
                bottom[0].vector.w = cornerW;
                bottom[1].vector.w = cornerW;
                bottom[2].vector.w = cornerW;
                bottom[3].vector.w = cornerW;
                top[0] = bottom[0];
                top[1] = bottom[1];
                top[2] = bottom[2];
                top[3] = bottom[3];
                top[0].vector.y += size.y;
                top[1].vector.y += size.y;
                top[2].vector.y += size.y;
                top[3].vector.y += size.y;
                break;
            case UWAMONO_MODE_CIRCLE:
                DispPillar(&record->position, record->extent.x, record->extent.y, 4);
                break;
            }
            /*
             * The unit pointer is set again with no use left, as Xenosaga I's DispCollUwamono sets it once per
             * branch. A pointer set more than once has no known base while the corners are stored, so the
             * rotation rows are loaded again after each corner store.
             */
            unit = &uwamonoRecords[i];
        }
    }
}

/* The state block of one uwamonoStateSlots entry: the pending points and the link table. */
struct UwamonoState {
    unsigned char unmodeled_0000[0x10a0];
    Vector4 points[512];                /* +0x10A0 */
    unsigned char unmodeled_30a0[6];
    short count;                        /* +0x30A6: points in use */
    unsigned char unmodeled_30a8[8];
    Vector4 vectors[0x40];              /* +0x30B0: copied one vector at a time */
    short linkTargets[0x40][4];         /* +0x34B0: vectors[linkTargets[i][j]] is linked to vectors[i] */
    short linkCounts[0x80];             /* +0x36B0: number of links of vectors[i] */
    short pointCount;                   /* +0x37B0 */
};

/*
 * Each 0x870-byte entry of uwamonoStateSlots starts with a pointer to its state block; the
 * rest of the entry is unmodeled here.
 */
typedef struct UwamonoStateSlot {
    struct UwamonoState *state;         /* +0x00 */
    unsigned char unmodeled_04[0x870 - 0x04];
} UwamonoStateSlot;



extern UwamonoStateSlot uwamonoStateSlots[];
extern float D_0069B0BC;

void uwamonoDrawPointPath(int slotIndex) {
    struct UwamonoState *state;
    int colour[4];
    DoublewordVector4 segment[2];
    int i;

    state = uwamonoStateSlots[slotIndex].state;
    if (uwamonoDrawDisabled == 0) {
        colour[0] = 255;
        colour[1] = 0;
        colour[2] = 0;
        colour[3] = 128;
        i = 0;
        if (state->count - 1 > 0) {
            do {
                segment[0].vector = state->points[i];
                segment[1].vector = state->points[i + 1];
                segment[0].vector.y += D_0069B0BC;
                segment[1].vector.y += D_0069B0BC;
                i++;
            } while (i < state->count - 1);
        }
    }
}





extern float D_0069B0C0;

/*
 * For each vector pair of the state's link table, copies both vectors into locals and
 * adds the gain to the y of each copy. Does nothing while uwamonoDrawDisabled is set.
 */
void uwamonoDrawStateLinks(int slotIndex)
{
    struct UwamonoState *state;
    int color[4];
    int i;
    int j;
    int target;

    state = uwamonoStateSlots[slotIndex].state;
    if (uwamonoDrawDisabled == 0) {
        color[0] = 255;
        color[1] = 0;
        color[2] = 255;
        color[3] = 128;
        for (i = 0; i < state->pointCount - 1; i++) {
            j = 0;
            if (state->linkCounts[i] > 0) {
                do {
                    DoublewordVector4 current;
                    DoublewordVector4 linked;
                    target = state->linkTargets[i][j];
                    current.vector = state->vectors[i];
                    linked.vector = state->vectors[target];
                    current.vector.y += D_0069B0C0;
                    linked.vector.y += D_0069B0C0;
                } while (++j < state->linkCounts[i]);
            }
        }
    }
}

/* One 208-byte record of the ladder table ladderSlots (tu075 and tu076 name it LadderSlot). */
typedef struct UwamonoLadderRecord {
    short id;                               /* +0x00: -1 when the slot is unused */
    unsigned char unmodeled_02[0x0E];
    DoublewordAlignedVector4 origin;        /* +0x10: x, y (the baseY of the foot), z, w */
    float angle;                            /* +0x20: facing angle of the ladder */
    float height;                           /* +0x24 */
    float rungs[32];                        /* +0x28: height of each rung above the foot */
    unsigned char rungExits[32];            /* +0xA8: 1 the rung can be left to the left, 2 to the right */
    short rungCount;                        /* +0xC8 */
    unsigned char unmodeled_ca[0x06];
} UwamonoLadderRecord;

extern UwamonoLadderRecord ladderSlots[];
extern float D_0069B0C4;
extern float D_0069B0C8;
extern float sinf(float angle);
extern float cosf(float angle);

void uwamonoDrawLadder(int slotIndex)
{
    UwamonoLadderRecord *slot;
    int color[4];
    DoublewordAlignedVector4 rung;
    DoublewordAlignedVector4 point;
    float phase;
    float radius;
    int i;

    slot = &ladderSlots[slotIndex];
    if (uwamonoDrawDisabled == 0 && slot->id != -1) {
        color[0] = 255;
        color[1] = 255;
        color[2] = 0;
        color[3] = 128;
        point = slot->origin;
        i = 0;
        point.y = point.y + slot->height;
        point = slot->origin;
        point.x = point.x + sinf(slot->angle);
        point.z = point.z + cosf(slot->angle);
        if (slot->rungCount > 0) {
            phase = D_0069B0C4;
            radius = D_0069B0C8;
            do {
                rung = slot->origin;
                rung.y = rung.y + slot->rungs[i];
                if (slot->rungExits[i] & 1) {
                    point.x = rung.x + sinf(slot->angle + phase) * radius;
                    point.y = rung.y;
                    point.z = rung.z + cosf(slot->angle + phase) * radius;
                }
                if (slot->rungExits[i] & 2) {
                    point.x = rung.x + sinf(slot->angle - phase) * radius;
                    point.y = rung.y;
                    point.z = rung.z + cosf(slot->angle - phase) * radius;
                }
            } while (++i < slot->rungCount);
        }
    }
}

void func_00202C78(void)
{
    int table[4];
    int n;

    if (uwamonoDrawDisabled != 0) {
        return;
    }
    table[0] = 255;
    table[1] = 255;
    table[2] = 0;
    table[3] = 128;
    n = 123;
    do {
        n -= 4;
    } while (n > -1);
}

/* A circle is walked in 16 steps of an eighth of pi (22.5 degrees). */
#define CIRCLE_STEP 0.39269909f

/*
 * Walks a ring of the given radius around source in the XZ plane, one segment per step: each step builds the segment's
 * two end points; when lift is positive the end points are copied with y raised by lift. Does nothing while
 * uwamonoDrawDisabled is set. color is not read: the callers pass a small id there (1, 2, 4 or 7), where Xenosaga I's
 * DispPillar took its unused colors argument.
 */
void DispPillar(Vector4 *source, float radius, float lift, int color)
{
    DoublewordAlignedVector4 first;
    DoublewordAlignedVector4 second;
    DoublewordAlignedVector4 firstLifted;
    DoublewordAlignedVector4 secondLifted;
    float angle;
    int i;

    if (uwamonoDrawDisabled == 0) {
        for (i = 0; i < 17; i++) {
            angle = CIRCLE_STEP;
            angle = i * angle;
            first = *source;
            second = *source;
            first.x = first.x + xglSin(angle) * radius;
            first.z = first.z + xglCos(angle) * radius;
            angle = CIRCLE_STEP;
            angle = (i + 1) * angle;
            second.x = second.x + xglSin(angle) * radius;
            second.z = second.z + xglCos(angle) * radius;
            if (lift > 0.0f) {
                firstLifted = first;
                secondLifted = second;
                firstLifted.y = firstLifted.y + lift;
                secondLifted.y = secondLifted.y + lift;
            }
        }
    }
}

/* Empty stub: six callers, nothing to name from the body. */
void func_00202E60(void) {

}

void uwamonoDrawFanEdge(const Vector4 *from, float angle, float radius)
{
    Vector4 point;

    if (uwamonoDrawDisabled == 0) {
        point = *from;
        point.x += sinf(angle) * radius;
        point.z += cosf(angle) * radius;
    }
}

/* Stores a value in the record's pendingValue, but only while the global gate uwamonoDrawDisabled is clear. */
void uwamonoSetPendingValue(UwamonoRecord *record, float pendingValue) {
    if (uwamonoDrawDisabled == 0) {
        record->pendingValue = pendingValue;
    }
}

void uwamonoDrawSegmentBox(const Vector4 *from, const Vector4 *to)
{
    Vector4 outline[8];
    int i;

    if (uwamonoDrawDisabled != 0) {
        return;
    }
    for (i = 0; i < 4; i++) {
        outline[i] = *from;
        outline[i + 4] = *to;
    }
    outline[1].x = to->x;
    outline[2].x = to->x;
    outline[2].z = to->z;
    outline[3].z = to->z;
    outline[5].x = from->x;
    outline[6].x = from->x;
    outline[6].z = from->z;
    outline[7].z = from->z;
}

/*
 * Builds three copies of *position, each nudged by 0.5 on one axis (x, y, z),
 * and three RGBA colour rows (red, green, blue); no value leaves the function.
 */
void uwamonoDrawPointAxes(const Vector4 *position) {
    if (uwamonoDrawDisabled == 0) {
        Vector4 pointX = *position;
        Vector4 pointY = *position;
        Vector4 pointZ = *position;
        int colorRed[4];
        int colorGreen[4];
        int colorBlue[4];

        colorRed[0] = 255;
        colorRed[1] = 0;
        colorRed[2] = 0;
        colorRed[3] = 63;
        colorGreen[0] = 0;
        colorGreen[1] = 255;
        colorGreen[2] = 0;
        colorGreen[3] = 63;
        colorBlue[0] = 0;
        colorBlue[1] = 0;
        colorBlue[2] = 255;
        colorBlue[3] = 63;

        pointX.x += 0.5f;
        pointY.y += 0.5f;
        pointZ.z += 0.5f;
    }
}

/* Two screen coordinates, scaled by 16 before uwamonoProjectToScreen halves them. */
typedef struct UwamonoScreenPos {
    int x;
    int y;
} UwamonoScreenPos;

/* The part of the active camera that uwamonoProjectToScreen hands to the vector helpers. */
typedef struct UwamonoActiveCamera {
    unsigned char unmodeled_00[0x70];
    Vector4 secondVector;               /* +0x70: second operand of _CurSetViewScaleTrans */
    Vector4 firstVector;                /* +0x80: first operand of _CurSetViewScaleTrans */
    unsigned char unmodeled_90[0x3e0];
    Vector4 thirdVector;                /* +0x470: operand of _CurMatrixSet */
} UwamonoActiveCamera;

void _CurMatrixSet(Vector4 *vector);
void _CurSetViewScaleTrans(Vector4 *first, Vector4 *second);
int _CurRotTransPersClipScreen(UwamonoScreenPos *position, int query);
UwamonoActiveCamera *xglStudioSelectGetActiveCamera(int);

/*
 * Sets up the active camera's vectors, then rescales the two screen coordinates in
 * position (x16) to centred values; _CurRotTransPersClipScreen can replace both with 0x1000.
 */
void uwamonoProjectToScreen(int query, UwamonoScreenPos *position) {
    UwamonoActiveCamera *camera;

    camera = xglStudioSelectGetActiveCamera(0);
    _CurMatrixSet(&camera->thirdVector);
    _CurSetViewScaleTrans(&camera->firstVector, &camera->secondVector);
    if (_CurRotTransPersClipScreen(position, query) != 0) {
        position->x = 0x1000;
        position->y = 0x1000;
        return;
    }
    position->x = ((position->x >> 4) - 0x700) >> 1;
    position->y = ((position->y >> 4) - 0x720) >> 1;
}
