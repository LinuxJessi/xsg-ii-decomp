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
    unsigned char unmodeled_0c4[0x5CC - 0xC4];
    u32 flags;                              /* 0x5CC */
    unsigned char unmodeled_5d0[4];
    int mode;                               /* 0x5D4: the motion XenoActorCheckMode compares against */
    unsigned char unmodeled_5d8[4];
    int requested_mode;                     /* 0x5DC */
    unsigned char unmodeled_5e0[0x654 - 0x5E0];
    float motionSpeed;                      /* 0x654 */
    float motionFrame;                      /* 0x658 */
    unsigned char unmodeled_65c[0x740 - 0x65C];
    unsigned char hitBuffer[8];             /* 0x740: the result block of mapMoveProbe */
    unsigned short hitFlags;                /* 0x748 */
    unsigned short groundFlags;             /* 0x74A: 0x10 while on a slope */
    unsigned char unmodeled_74c[4];
    float stepHeight;                       /* 0x750 */
    unsigned char unmodeled_754[4];
    int ground;                             /* 0x758 */
    unsigned char unmodeled_75c[4];
    int status;                             /* 0x760: ground status (0x1xxxx slope, 0x1xx ladder, 0x2xx elevator) */
    unsigned char unmodeled_764[0x780 - 0x764];
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
    unsigned char unmodeled_0000[0x4028];
    short targetPriority;                   /* +0x4028: 3 keeps the current target */
    unsigned char unmodeled_402a[0x4078 - 0x402A];
    int walk[3];
} UwamonoMotionTable;

/* The talk/search block of a map object record (+0x590). */
typedef struct UwamonoSearch {
    signed char mode;                       /* +0x590 */
    unsigned char unmodeled_591[0xB];
    int level;                              /* +0x59C: the jump height step */
} UwamonoSearch;

/* One 0x870-byte record of uwamonoRecords (see init_uwamono_sys.c). */
typedef struct UwamonoObject {
    short id;                               /* +0x00 */
    unsigned char kind;                     /* +0x02: 1 talks, 2 is searched */
    unsigned char unmodeled_03;
    int key;                                /* +0x04 */
    unsigned char unmodeled_08[8];
    AlignedVector4 position;                /* +0x10 */
    unsigned char unmodeled_20[0x20];
    AlignedVector4 anchor;                  /* +0x40 */
    unsigned char unmodeled_50[0x20];
    float fade;                             /* +0x70: rider fade, 0..100 */
    unsigned char unmodeled_74[0xC];
    AlignedVector4 exitFrom;                /* +0x80: the ladder exit starts here (tu075.c) */
    AlignedVector4 exitTo;                  /* +0x90 */
    unsigned char unmodeled_a0[0x330 - 0xA0];
    struct XenoActor *actor;                /* +0x330 */
    UwamonoMotionTable *motions;            /* +0x334 */
    unsigned char unmodeled_338[0x238];
    int flags;                              /* +0x570 */
    unsigned char unmodeled_574[0x1C];
    UwamonoSearch search;                   /* +0x590 */
    unsigned char unmodeled_5a0[0x21];
    signed char searchState;                /* +0x5C1 */
    unsigned char unmodeled_5c2[0x2AE];
} UwamonoObject;

/* Bits of gameWork.control. */
#define CONTROL_PAUSED 0x1
#define CONTROL_SCRIPTED 0x8000
#define CONTROL_REPLAYED 0x10000
#define CONTROL_SEARCHING 0x20000
#define CONTROL_TOP_EXIT 0x20 /* reaching the top of a ladder moves on to the top state (tu075.c) */

extern UwamonoObject uwamonoRecords[];
extern int uwamonoCurrentIndex;

/* Functions of src/main/tu051.c that other files call. */

void cameraApplySlots(int slot_mask, int index_a, int index_b, int enable_flag);

#endif /* INCLUDE_MAIN_TU051_H */
