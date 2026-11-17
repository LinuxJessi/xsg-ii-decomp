#ifndef INCLUDE_MAIN_TU051_H
#define INCLUDE_MAIN_TU051_H

#include "shared.h"

/* Types used by the map object code (tu051.c, tu075.c); the actor is only partly recovered. */
/* One motion state of the actor's TiMRPackage channel block (TiMRPackage.c's TiMRState, 0x50 bytes). */
typedef struct XenoMotionState {
    u32 flags;
    float speed;                            /* +0x04: tiMRStateSetSpeed */
    float frame;                            /* +0x08 */
    float value;                            /* +0x0C: XenoActorSetChannelValue */
    int range[2];                           /* +0x10: the play range (xenoActor.c names word 0 source_value) */
    unsigned char unmodeled_18[0x50 - 0x18];
} XenoMotionState;

/* The XenoActor of a map object (xenoActor.c); only the fields used below. */
/* XenoActor.rsrc: the model pack (.pxy, xenoActor.c reads it as PxyHeader *), the texture (.txy), the hierarchy
 * (.xhr, XenoActorAllocMatrices builds the matrices from it), the motion table (.xap, XenoActorGetSlotRange's
 * table) and its second extension. No assert names slots 3 and 5. */
enum XActRsrc {
    XAct_Rsrc_MDL,
    XAct_Rsrc_TEX,
    XAct_Rsrc_XHR,
    XAct_Rsrc_3,
    XAct_Rsrc_XAP,
    XAct_Rsrc_5,
    XAct_Rsrc_XAP_Ext2,
    XAct_Rsrc_Count
};

/* The light block copied into every actor of a battle unit (tu010.c unitCreateSide). */
typedef struct LightBlocks {
    u64 word[48];
} LightBlocks;

struct XenoActor {
    unsigned char unmodeled_000[0x10];
    /* 0x010: ov01 reads it as a float array (tu019 zeroes x/y/z that way) and moves it as two words (tu012, tu016). */
    AlignedVector4 position;                /* 0x010 */
    AlignedVector4 previousPosition;        /* 0x020: x and z before this frame's move (tiWaterUpdateSource) */
    unsigned char unmodeled_030[0x10];
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
    float alpha;                            /* 0x0C0: tu033 pauses a battle actor by setting it to 0, a fade writes its progress */
    int drawMode;                           /* 0x0C4: 1 while attached as a part (uwamonoAttachPlayerParts), 2 on a battle unit's root */
    unsigned char unmodeled_0c8[4];
    float drawParam;                        /* 0x0CC: 0.5 with drawMode 2 */
    unsigned char unmodeled_0d0[0x100 - 0xD0];
    Matrix4 matrix;                         /* 0x100: the matrix unitFaceTarget applies with xglVectorMulMat */
    float focusX;                           /* 0x140: focus offset, damped by cfeDampDirOffset */
    float focusY;                           /* 0x144 */
    unsigned char unmodeled_148[0x150 - 0x148];
    float moveAngle;                        /* 0x150: XenoActorUpdateMoveState advances it to 1 (xenoActor.c names) */
    float moveVelocity;                     /* 0x154 */
    float shadowValue;                      /* 0x158 */
    int shadowCount;                        /* 0x15C: XenoActorUpdateMoveState runs while it is set */
    int moveState;                          /* 0x160: the state XenoActorUpdateMoveState switches on (0..7) */
    float shadowValue2;                     /* 0x164 */
    float shadowValue3;                     /* 0x168 */
    unsigned char unmodeled_16c[0x190 - 0x16C];
    LightBlocks lightBlocks;                /* 0x190 */
    unsigned char unmodeled_310[0x490 - 0x310];
    DoublewordVector4 runtimeVecA;          /* 0x490: runtimeVecPtrGet's outputs, copied into every battle actor */
    DoublewordVector4 runtimeVecB;          /* 0x4A0 */
    unsigned char unmodeled_4b0[0x4C0 - 0x4B0];
    u32 visibility[8];                      /* 0x4C0: one bit per geometry of the pack (xenoActor.c's name);
                                             * a cut-scene actor's flag records set them */
    unsigned char unmodeled_4e0[0x5A4 - 0x4E0];
    struct XenoActor *parent;               /* 0x5A4: the actor this one is attached to (XenoActorReleaseChild) */
    unsigned char unmodeled_5a8[0x5CC - 0x5A8];
    u32 flags;                              /* 0x5CC */
    float frame;                            /* 0x5D0: the scene frame (tiWaterSceneLoop counts it to 120) */
    int mode;                               /* 0x5D4: the motion XenoActorCheckMode compares against */
    u32 phaseLock;                          /* 0x5D8: nonzero keeps unitSetPhase from its early return and clears its value */
    int requested_mode;                     /* 0x5DC */
    unsigned char unmodeled_5e0[4];
    int selector;                           /* 0x5E4: the last selector (xenoActor.c cachedSelector) */
    unsigned char unmodeled_5e8[4];
    /* 0x5EC: the actor's resources, rsrc[XAct_Rsrc_*] in the original's asserts (resPathSlotLoadID returns
     * them, uwamonoLoadPlayer copies them in). */
    int rsrc[XAct_Rsrc_Count];
    unsigned char unmodeled_608[0x610 - 0x608];
    void *sharedResource;                   /* 0x610: resource 0x30041, loaded when the hierarchy needs it */
    unsigned char unmodeled_614[0x618 - 0x614];
    int lookLink;                           /* 0x618 */
    unsigned char unmodeled_61c[0x640 - 0x61C];
    int channel[4];                         /* 0x640: the tiMRPackage channel block (xenoActor.c handle640) */
    XenoMotionState motion[2];              /* 0x650, 0x6A0: the TiMRPackage states A and B of the channel block */
    unsigned char unmodeled_6f0[0x710 - 0x6F0];
    struct XenoHrc *table;                  /* 0x710: the joint table XenoActorGetHrcPoint reads */
    unsigned char unmodeled_714[0x740 - 0x714];
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
    int state;                              /* 0x79C: written by XenoActorSetState; XenoActorDrawMotion reads it and clears it */
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

/* gameWork as far as its control word: the files that read more of it keep their own view. */
typedef struct GameWorkControl {
    unsigned char unmodeled_00[8];
    int control;                            /* +0x08: CONTROL_* */
} GameWorkControl;

/* Bits of gameWork.control. */
#define CONTROL_PAUSED 0x1
#define CONTROL_SCRIPTED 0x8000
#define CONTROL_REPLAYED 0x10000
#define CONTROL_SEARCHING 0x20000
#define CONTROL_TOP_EXIT 0x20 /* reaching the top of a ladder moves on to the top state (tu075.c) */
/* Set while a cut scene runs (cfePlugIn.c sets it at the start, clears it at the end). */
#define CONTROL_CFE_SCENE 0x200
#define CONTROL_CFE_NO_FADE 0x100000 /* set with it: events then start no fade-out (tu069.c) */
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
