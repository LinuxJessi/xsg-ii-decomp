#include "common.h"
#include "shared.h"

/* Number of MapUnit[] slots (map_2.h's MapUnit table). */
#define MAP_UNIT_COUNT 64

typedef float Matrix4[4][4];

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
    MAP_UNIT_LANDED = 0x80000,          /* a falling unit has come to rest (CheckUwamonoHeight) */
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

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_001FFC90);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_001FFCF0);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_001FFD50);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_001FFD78);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_001FFDA0);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_001FFE40);

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

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00200168);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00200238);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00200308);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_002003E8);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_002005D0);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00200638);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00200858);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00200950);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00200A68);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00200B08);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00200ED0);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00201440);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00201480);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_002015A8);

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

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_002019D0);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00201A90);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00201AC0);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00201B88);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", CrossCheckMapUnitCircle);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", CrossCheckMapUnitBox);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00201F88);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00202170);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_002021F0);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00202418);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_002024C8);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00202578);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_002025B0);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00202860);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00202940);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00202AA0);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00202C78);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00202CD0);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00202E60);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00202E68);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00202EE8);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00202F00);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00202FA0);

INCLUDE_ASM("asm/main/nonmatchings/init_uwamono_sys", func_00203078);
