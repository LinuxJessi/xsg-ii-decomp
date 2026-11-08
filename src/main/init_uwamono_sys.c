#include "common.h"
#include "shared.h"
/* Number of MapUnit[] slots (map_2.h's MapUnit table). */
#define MAP_UNIT_COUNT 64

int HitCheckMapUnitPos(const Vector4 *position);
extern void xglVectorLength(float *destination, const Vector4 *vector);

typedef struct UwamonoMapUnitState {
    unsigned char unmodeled_00[2];
    short kindIndex;
    signed char signal;                 /* +0x04 */
    signed char hideObjectIndex;
    unsigned char unmodeled_06[6];
    float baseSizeX;
    float baseSizeY;
    float baseSizeZ;
    float sizeOffsetX;
    float sizeOffsetY;
    float sizeOffsetZ;
    unsigned char unmodeled_24[8];
    signed char enemyIndex;
    signed char state;                 /* +0x2d */
    unsigned char unmodeled_2e[2];
    short initialPartId;               /* +0x30 */
    signed char target;                /* +0x32 */
    unsigned char unmodeled_33[9];
    int frameLimit;                    /* +0x3c */
    union { int effectResourceId; int kindParameter; };              /* +0x40 */
    int timer;                         /* +0x44 */
} UwamonoMapUnitState;

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

int HitCheckMapUnitPosSize(const Vector4 *position, float radius);

/* Number of entries of the global `actor` array. */
#define ACTOR_COUNT 64

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

/*
 * The MapUnit[] vectors the box checks copy are DoublewordAlignedVector4: copied one
 * 64-bit word at a time, high word first (ld/sd pairs at +8 then +0).
 */
typedef struct UwamonoCollisionUnit {
    unsigned char unmodeled_00[0x10];
    DoublewordVector4 position; /* copied word by word (words[1] first) in HitCheckBoxCorner */
    float rotationX;                  /* +0x20 */
    float rotationY;                  /* +0x24 */
    float rotationZ;                  /* +0x28 */
    unsigned char unmodeled_2c[0x14];
    Matrix4 matrix;                   /* +0x40 */
    struct UwamonoCrossBasis *basis;
    unsigned char unmodeled_84[0x2c];
    AlignedVector4 dimensions;
    unsigned char unmodeled_c0[0xe0];
    UwamonoMapUnitState state;
    unsigned char unmodeled_1e8[0x118];
} UwamonoCollisionUnit;

typedef struct UwamonoAimQuery {
    unsigned char unmodeled_00[8];
    unsigned short attributeMask;
    unsigned char unmodeled_0a[0x0e];
    void *header;
    unsigned char unmodeled_1c[0x24];
} UwamonoAimQuery;

typedef struct UwamonoCollisionActor {
    unsigned char unmodeled_00[0x10];
    AlignedVector4 position;
    unsigned char unmodeled_20[0x10];
    AlignedVector4 displacement;       /* +0x30 */
    unsigned char unmodeled_40[0x488];
    UwamonoAimQuery undulation; /* +0x4c8 */
    unsigned char unmodeled_508[0x4e0];
    float radius;
    unsigned char unmodeled_9ec[0x84];
} UwamonoCollisionActor;

typedef struct UwamonoEnemyUnit UwamonoEnemyUnit;

struct UwamonoEnemyUnit {
    unsigned int flags;
    void (*update)(UwamonoEnemyUnit *);
    unsigned char unmodeled_08[8];
    AlignedVector4 position;
    unsigned char unmodeled_20[0x30];
    float rotationX;                   /* +0x50 */
    float heading;                     /* +0x54 */
    float rotationZ;                   /* +0x58 */
    unsigned char unmodeled_5c[0x24];
    unsigned char actorIndex;          /* +0x80: this actor's actor[] index */
    unsigned char unmodeled_81[5];
    short resourceId;                  /* +0x86: negative for a free actor */
    unsigned char unmodeled_88[0x848];
    void *collisionObject;             /* +0x8d0 */
    unsigned char unmodeled_8d4[0x110];
    float actorHeading;                /* +0x9e4 */
    float radius;                      /* +0x9e8 */
    unsigned char unmodeled_9ec[0x84];
};

typedef struct UwamonoCornerCollisionUnit {
    unsigned char unmodeled_00[0x10];
    DoublewordVector4 position; /* copied word by word (words[1] first) in HitCheckBox */
    unsigned char unmodeled_20[0x60];
    const float (*basis)[4];
    unsigned char unmodeled_84[0x2c];
    AlignedVector4 halfSize;
} UwamonoCornerCollisionUnit;

extern UwamonoMapSlot MapUnit[];
extern const float mapUnitPosHitHeightScale;
extern const float mapUnitPosHitBelowMargin;
extern UwamonoEnemyUnit actor[ACTOR_COUNT];
int HitCheckBoxCorner(UwamonoCollisionActor *actor, UwamonoCollisionUnit *unit);
unsigned int HitCheckMapUnitPosAt(UwamonoCollisionActor *actor, UwamonoCollisionUnit *unit);
int HitCheckCorner(UwamonoCollisionActor *actor, UwamonoCollisionUnit *unit);
void CheckNearPoint(const Vector4 *origin, const Vector4 *candA,
                    const Vector4 *candB, Vector4 *out);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", uwamonoAddMoveStep);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", uwamonoAddRotateStep);

/* Per-entry transform that uwamonoUpdateTransform writes its translation into (+0x264 of UwamonoRecord). */
typedef struct UwamonoTransform {
    unsigned char unmodeled_00[0x30];
    Vector4 translation;                /* +0x30: x, y, z */
} UwamonoTransform;

/*
 * One 0x870-byte entry of the record tables: the two reset tables (D_00340200,
 * D_00340304) and the table scanned by uwamonoHitCheckRecord and uwamonoHitCheckOther (uwamonoRecords).
 */
typedef struct UwamonoRecord {
    short resetWord;                    /* +0x00: zeroed by uwamonoClearMoveResetWord and uwamonoClearRotateResetWord */
    unsigned char kind;                 /* +0x02: the scans match kind 2 */
    unsigned char unmodeled_03;
    int key;                            /* +0x04: compared by uwamonoHitCheckOther */
    unsigned char unmodeled_08[4];
    float pendingValue;                 /* +0x0c: stored by uwamonoSetPendingValue while D_0069B948 is 0 */
    AlignedVector4 position;            /* +0x10: x, y (+0x14), z */
    unsigned char unmodeled_20[0x20];
    AlignedVector4 anchor;              /* +0x40: anchor.y at +0x44 */
    unsigned char unmodeled_50[0x60];
    float radius;                       /* +0xb0: added to other->extent.x by uwamonoNyuruCircle */
    unsigned char unmodeled_b4[0x1b0];
    UwamonoTransform *transform;        /* +0x264 */
    unsigned char unmodeled_268[0x2de];
    short target;                       /* +0x546: -1 when unset */
    unsigned char unmodeled_548[0x28];
    int flags;                          /* +0x570: bit 0x2 read by the scans, bits 0x2 and 0x8 by uwamonoUpdateTransform */
    unsigned char unmodeled_574[0x0c];
    AlignedVector4 extent;              /* +0x580: x read by uwamonoNyuruCircle */
    signed char mode;                   /* +0x590: 0 when inactive; 1 and 2 dispatch in uwamonoCrossPointAt */
    unsigned char unmodeled_591[0x1af];
    unsigned char hitBuffer[8];         /* +0x740: address passed to mapMoveProbe */
    unsigned short hitFlags;            /* +0x748: |= 0x1000 by uwamonoNyuruCircle */
    unsigned char unmodeled_74a[0x126];
} UwamonoRecord;

extern UwamonoRecord D_00340200[];

/* Zeroes the first word of one entry of the move reset table (setStepMove calls this). */
void uwamonoClearMoveResetWord(int index) {
    D_00340200[index].resetWord = 0;
}

extern UwamonoRecord D_00340304[];

/* Same as uwamonoClearMoveResetWord for the rotate reset table (setStepRotate calls this). */
void uwamonoClearRotateResetWord(int index) {
    D_00340304[index].resetWord = 0;
}

int uwamonoHitCheckRecordAt(int query, UwamonoRecord *record);
extern UwamonoRecord uwamonoRecords[];

/* Returns the index of the first active kind-2 record that the query hits, or -1. */
int uwamonoHitCheckRecord(int query) {
    int index;

    for (index = 0; index < 0x100; index++) {
        UwamonoRecord *record = &uwamonoRecords[index];

        if (record->kind == 2 && (record->flags & 2) && record->mode != 0
            && uwamonoHitCheckRecordAt(query, record) != 0) {
            return index;
        }
    }
    return -1;
}

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", uwamonoHitCheckWithNyuru);

/*
 * HitCheckMapUnitPos (main:0x002c02f0): a thin tail call into
 * HitCheckMapUnitPosSize with its distance threshold fixed at 0.0f (a
 * true `j`, no `jal`, so this function's own return value is whatever
 * HitCheckMapUnitPosSize returns).
 */
int HitCheckMapUnitPos(const Vector4 *position) {
    return HitCheckMapUnitPosSize(position, 0.0f);
}

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", HitCheckMapUnitPosSize);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", uwamonoHitCheckRecordAt);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", HitCheckMapUnitPosAt);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00200308);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", uwamonoHitCheckBox);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", HitCheckBoxCorner);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", HitCheckCorner);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", uwamonoHitCheckNyuru);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00200950);

int func_00200950(const Vector4 *from, const Vector4 *to, const Vector4 *other, float radius);
float mapMoveProbe(const Vector4 *from, const Vector4 *to, void *result);

/*
 * Circle-shaped record against another record: returns 1 when the other record's
 * circle does not block it; otherwise it lands the record on the ground height
 * mapMoveProbe reports and returns 1, or 0 when there is no ground.
 */
int uwamonoNyuruCircle(UwamonoRecord *record, UwamonoRecord *other) {
    float height;

    if (func_00200950(&record->position, &record->anchor, &other->position,
                      record->radius + other->extent.x) != 0) {
        return 1;
    }
    record->hitFlags |= 0x1000;
    height = mapMoveProbe(&record->position, &record->anchor, record->hitBuffer);
    if (height == -1000.0f) {
        return 0;
    }
    record->position.y = height;
    return 1;
}

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", uwamonoNyuruMatrix);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", uwamonoCrossPoint);

int uwamonoCalcNearCrossPointCircle(int first, int second, UwamonoRecord *record, int third);
int uwamonoCalcNearCrossPointBox(int first, int second, UwamonoRecord *record, int third);

/* Near-crossing calculation for one record, chosen by its mode: 1 = box, 2 = circle; any other mode returns 2. */
int uwamonoCrossPointAt(int first, int second, UwamonoRecord *record, int third) {
    if (record->mode != 1) {
        if (record->mode == 2) {
            return uwamonoCalcNearCrossPointCircle(first, second, record, third);
        }
        return 2;
    }
    return uwamonoCalcNearCrossPointBox(first, second, record, third);
}

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", uwamonoCalcNearCrossPointCircle);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", uwamonoCalcNearCrossPointBox);

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

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", uwamonoAreaCheck);

int func_00200308(const Vector4 *from, const Vector4 *position, const Vector4 *extent, float radius, float height);

/* Box area test of a record (the record's extent and anchor height) against a point with a radius. */
int uwamonoAreaCheckBox(const Vector4 *from, UwamonoRecord *record, float radius) {
    return func_00200308(from, &record->position, &record->extent, radius, record->anchor.y) != 0;
}

/*
 * One 0x870-byte record of uwamonoRecords. The search reads the kind byte at +0x02,
 * the flags word at +0x570, the enabled byte at +0x590 and the position at +0x10.
 */
typedef struct UwamonoSearchRecord {
    unsigned char unmodeled_00[2];
    unsigned char kind;
    unsigned char unmodeled_03[0x0d];
    AlignedVector4 position;
    unsigned char unmodeled_20[0x550];
    unsigned int flags;
    unsigned char unmodeled_574[0x1c];
    signed char enabled;
    unsigned char unmodeled_591[0x2df];
} UwamonoSearchRecord;

int uwamonoCrossCheckRecordAt(int checkParam, const AlignedVector4 *position, const UwamonoSearchRecord *record);

int uwamonoCrossCheck(int checkParam, UwamonoSearchRecord *exclude)
{
    UwamonoSearchRecord *record;
    int index;

    record = uwamonoRecords;
    for (index = 0; index < 0x100; index++) {
        if (record->kind == 2 && !(record->flags & 0x02000000) && record != exclude && record->enabled != 0
            && uwamonoCrossCheckRecordAt(checkParam, &exclude->position, record) != 0) {
            return index;
        }
        record++;
    }
    return -1;
}

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", uwamonoCrossCheckRecordAt);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", CrossCheckMapUnitCircle);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", CrossCheckMapUnitBox);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00201F88);

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
        transform->translation.x = record->position.x;
        transform->translation.y = record->position.y;
        transform->translation.z = record->position.z;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", uwamonoCheckHeight);

int uwamonoHitCheckOtherAt(UwamonoRecord *record, UwamonoRecord *candidate);

int uwamonoHitCheckOther(UwamonoRecord *record)
{
    int index;

    for (index = 0; index < 0x100; index++) {
        UwamonoRecord *candidate = &uwamonoRecords[index];

        if (candidate->kind == 2 && candidate->key != record->key
            && (candidate->flags & 2) && candidate->mode != 0
            && uwamonoHitCheckOtherAt(record, candidate) != 0) {
            return index;
        }
    }
    return -1;
}

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", uwamonoHitCheckOtherAt);

extern float D_0069B0B4;

int uwamonoHitCheckOtherBox(UwamonoRecord *record, UwamonoRecord *other) {
    return func_00200308(&record->position, &other->position, &other->extent, D_0069B0B4, other->anchor.y) != 0;
}

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_002025B0);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00202860);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00202940);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00202AA0);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00202C78);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00202CD0);

/* Empty stub: six callers, nothing to name from the body. */
void func_00202E60(void) {

}

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00202E68);

extern signed char D_0069B948;

/* Stores a value in the record's pendingValue, but only while the global gate D_0069B948 is clear. */
void uwamonoSetPendingValue(UwamonoRecord *record, float pendingValue) {
    if (D_0069B948 == 0) {
        record->pendingValue = pendingValue;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00202F00);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00202FA0);

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
void *xglStudioSelectGetActiveCamera(int);

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
