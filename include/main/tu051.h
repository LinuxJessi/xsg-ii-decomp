#ifndef INCLUDE_MAIN_TU051_H
#define INCLUDE_MAIN_TU051_H

#include "shared.h"

/* Types used by the map object code (tu051.c, tu075.c); the actor is only partly recovered. */
/* The XenoActor of a map object (xenoActor.c); only the fields used below. */
struct XenoActor {
    unsigned char unmodeled_000[0x10];
    AlignedVector4 position;                /* 0x010 */
    unsigned char unmodeled_020[0x20];
    AlignedVector4 velocity;                /* 0x040: this frame's movement */
    AlignedVector4 rotation;                /* 0x050: y is the facing angle */
    AlignedVector4 direction;               /* 0x060: y is the moving angle */
    AlignedVector4 vector70;                /* 0x070 */
    AlignedVector4 vector80;                /* 0x080 */
    AlignedVector4 vector90;                /* 0x090 */
    unsigned char unmodeled_0a0[0x10];
    float radius;                           /* 0x0B0 */
    unsigned char unmodeled_0b4[8];
    int drawFlags;                          /* 0x0BC: 0x4 hides the actor, 0x08000000 draws it translucent */
    float alpha;                            /* 0x0C0 */
    int drawMode;                           /* 0x0C4: 1 while attached as a part (uwamonoAttachPlayerParts) */
    unsigned char unmodeled_0c8[0x5CC - 0xC8];
    u32 flags;                              /* 0x5CC */
    unsigned char unmodeled_5d0[4];
    int mode;                               /* 0x5D4: the motion XenoActorCheckMode compares against */
    unsigned char unmodeled_5d8[4];
    int requested_mode;                     /* 0x5DC */
    unsigned char unmodeled_5e0[0x5EC - 0x5E0];
    /* The model resources resPathSlotLoadID returns (uwamonoLoadPlayer copies them in). */
    int pack;                               /* 0x5EC: the model pack (xenoActor.c reads it as PxyHeader *) */
    int effect;                             /* 0x5F0 */
    int hierarchy;                          /* 0x5F4: XenoActorAllocMatrices builds the matrices from it */
    unsigned char unmodeled_5f8[4];
    int slotTable;                          /* 0x5FC: XenoActorGetSlotRange's table */
    unsigned char unmodeled_600[0x618 - 0x600];
    int lookLink;                           /* 0x618 */
    unsigned char unmodeled_61c[0x640 - 0x61C];
    int channel[4];                         /* 0x640: the tiMRPackage channel block (xenoActor.c handle640) */
    unsigned char unmodeled_650[0x654 - 0x650];
    float motionSpeed;                      /* 0x654 */
    float motionFrame;                      /* 0x658 */
    unsigned char unmodeled_65c[0x740 - 0x65C];
    unsigned char hitBuffer[8];             /* 0x740: the result block of mapMoveProbe */
    unsigned short hitFlags;                /* 0x748 */
    unsigned short groundFlags;             /* 0x74A: 0x10 while on a slope */
    unsigned char unmodeled_74c[3];
    unsigned char probeEnabled;             /* 0x74F: set to 1 when a player model is loaded */
    float stepHeight;                       /* 0x750 */
    unsigned char unmodeled_754[4];
    int ground;                             /* 0x758 */
    unsigned char unmodeled_75c[4];
    int status;                             /* 0x760: ground status (0x1xxxx slope, 0x1xx ladder, 0x2xx elevator) */
    unsigned char unmodeled_764[4];
    float probeRadius;                      /* 0x768: set with radius to the form's size */
    unsigned char unmodeled_76c[0x780 - 0x76C];
    DoublewordVector4 previous;             /* 0x780: the position before this frame */
    unsigned char unmodeled_790[4];
    int moved;                              /* 0x794 */
    /* A one-member union: the original reads this field with alias set 0, after the vector copies before it. */
    union {
        int kind;                           /* 0x798: 0x10000..0x13FFF for an ES (the mecha the party rides) */
    };
};

/* The motion number table of a map object (+0x4078: one entry per walk mode). */
typedef struct UwamonoMotionTable {
    unsigned char unmodeled_0000[0x3FF0];
    int flags;                              /* +0x3FF0: bit 0 keeps the player from being pushed */
    unsigned char unmodeled_3ff4[0x4028 - 0x3FF4];
    short targetPriority;                   /* +0x4028: 3 keeps the current target */
    unsigned char unmodeled_402a[0x4078 - 0x402A];
    int walk[3];
} UwamonoMotionTable;

/* The talk/search block of a map object record (+0x590). */
typedef struct UwamonoSearch {
    signed char mode;                       /* +0x590: 0 or 1 by object type (tu082.c), argument 2 of the kowaremono native (tu074.c) */
    signed char shape;                      /* +0x591: selects the search test (tu088.c uwamonoSelectTarget) and the collision test (tu089.c) */
    unsigned char unmodeled_592[0xA];
    int level;                              /* +0x59C: the jump height step */
} UwamonoSearch;

/* One 0x870-byte record of uwamonoRecords (see init_uwamono_sys.c). */
typedef struct UwamonoObject {
    short id;                               /* +0x00 */
    unsigned char kind;                     /* +0x02: 1 talks, 2 is searched */
    unsigned char unmodeled_03;
    int key;                                /* +0x04 */
    int type;                               /* +0x08: extra-object type and model index (mapObjectResources[type + 2]): 0-3 traps "ExTrp",
                                             * 4-6 treasure boxes "ExTbx", 7-9 save points "ExSav", 10-11 dummies, 12 and up items "ExItm";
                                             * -1 has no model (tu082.c uwamonoInitRecord, tu050.c) */
    unsigned char unmodeled_0c[4];
    AlignedVector4 position;                /* +0x10 */
    unsigned char unmodeled_20[0x20];
    AlignedVector4 rotation;                /* +0x40: angles; y is the heading */
    AlignedVector4 scale;                   /* +0x50: x, y, z set by the player's form */
    unsigned char unmodeled_60[0x10];
    float fade;                             /* +0x70: rider fade, 0..100 */
    unsigned char unmodeled_74[0xC];
    AlignedVector4 moveFrom;                /* +0x80: end points of the running move (tu050.c); the ladder exit uses them too (tu075.c) */
    AlignedVector4 moveTo;                  /* +0x90 */
    AlignedVector4 turnFrom;                /* +0xA0: end angles of the running turn; a sector record keeps its angle in x (tu089.c) */
    AlignedVector4 turnTo;                  /* +0xB0: a sector record keeps its width in x */
    unsigned char unmodeled_c0[0x264 - 0xC0];
    Matrix4 *transform;                     /* +0x264: world matrix (init_uwamono_sys.c) */
    unsigned char unmodeled_268[0x2F0 - 0x268];
    Matrix4 matrix;                         /* +0x2F0: collision basis rows (init_uwamono_sys.c, tu050.c) */
    struct XenoActor *actor;                /* +0x330 */
    UwamonoMotionTable *motions;            /* +0x334 */
    float form;                             /* +0x338: UWAMONO_FORM_ES while riding an ES, else 5.0 */
    unsigned short formSet;                 /* +0x33C: 1 once the form is chosen */
    unsigned char unmodeled_33e[2];
    float moveX[32];                        /* +0x340: move curve, uwamonoAddMoveStep (init_uwamono_sys.c) */
    float moveY[32];                        /* +0x3C0 */
    short moveStepCount;                    /* +0x440 */
    unsigned char unmodeled_442[2];
    float turnX[32];                        /* +0x444: rotate curve, uwamonoAddRotateStep */
    float turnY[32];                        /* +0x4C4 */
    short turnStepCount;                    /* +0x544 */
    short target;                           /* +0x546: index of the record this one follows or stands on, -1 when none */
    unsigned char unmodeled_548[0x570 - 0x548];
    int flags;                              /* +0x570 */
    int landedFlags;                        /* +0x574: uwamonoCheckHeight (init_uwamono_sys.c) */
    unsigned char unmodeled_578[8];
    AlignedVector4 extent;                  /* +0x580: x is the form's size (init_uwamono_sys.c) */
    UwamonoSearch search;                   /* +0x590 */
    int effectId;                           /* +0x5A0: effect or motion started when searched (tu074.c, tu083.c) */
    int linkA;                              /* +0x5A4: partner records set by the kowaremono native (tu074.c) */
    int linkB;                              /* +0x5A8 */
    unsigned char unmodeled_5ac[4];
    Vector4 focusOffset;                    /* +0x5B0: from the position to the point that is marked, aimed at and ridden
                                             * (tu083.c selection marker, tu086.c target vector, tu050.c riders) */
    signed char motionKind;                 /* +0x5C0: 1 = moves between two points, else rotates (tu074.c) */
    signed char searchState;                /* +0x5C1 */
    signed char busy;                       /* +0x5C2: set while the door motion runs */
    unsigned char unmodeled_5c3;
    short timer;                            /* +0x5C4 */
    unsigned char unmodeled_5c6[0x5D0 - 0x5C6];
    Vector4 closedPosition;                 /* +0x5D0: the two states of a door: set by uwamonoSetDoorAll (tu074.c), moved between by
                                             * uwamonoStartPairMove (tu083.c) */
    Vector4 openPosition;                   /* +0x5E0 */
    Vector4 closedRotation;                 /* +0x5F0 */
    Vector4 openRotation;                   /* +0x600 */
    float speed;                            /* +0x610 */
    float swingLength;                      /* +0x614 */
    int linkIndex;                          /* +0x618: the linked record, -1 when none (tu083.c) */
    unsigned char unmodeled_61c[4];
    int triggerSound;                       /* +0x620: seVoiceStartDefault id played when the record is triggered (tu083.c), -1 none */
    int moveSound;                          /* +0x624: played when the pair move starts */
    unsigned char unmodeled_628[4];
    signed char state;                      /* +0x62C: 0 out of reach, 1 selected, 2 in reach (tu083.c) */
    unsigned char unmodeled_62d[0x634 - 0x62D];
    int cellMask;                           /* +0x634: map cells the record belongs to (tu083.c) */
    void *handles[3];                       /* +0x638: effect and rider handles released with the record (tu083.c) */
    int releaseNextIndex;                   /* +0x644 */
    unsigned char unmodeled_648[0x740 - 0x648];
    unsigned char hitBuffer[8];             /* +0x740: mapMoveProbe result (init_uwamono_sys.c) */
    unsigned short hitFlags;                /* +0x748 */
    unsigned char unmodeled_74a[0x870 - 0x74A];
} UwamonoObject;

/* Bits of gameWork.control. */
#define CONTROL_PAUSED 0x1
#define CONTROL_SCRIPTED 0x8000
#define CONTROL_REPLAYED 0x10000
#define CONTROL_SEARCHING 0x20000
#define CONTROL_TOP_EXIT 0x20 /* reaching the top of a ladder moves on to the top state (tu075.c) */
#define CONTROL_SOFT_RESET 0x10000000 /* the soft-reset pad combo was pressed; Title will not start a game while it is set */

extern UwamonoObject uwamonoRecords[];
extern int uwamonoCurrentIndex;

/* One 0x870-byte record of uwamonoScaleRecords: the scale of the map object record in the same slot. */
typedef struct UwamonoScaleRecord {
    float scale;                        /* +0x000 */
    unsigned char unmodeled_004[0x870 - 0x004];
} UwamonoScaleRecord;

extern UwamonoScaleRecord uwamonoScaleRecords[];

/* Functions of src/main/tu051.c that other files call. */

void cameraApplySlots(int slot_mask, int index_a, int index_b, int enable_flag);

#endif /* INCLUDE_MAIN_TU051_H */
