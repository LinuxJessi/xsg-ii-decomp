#ifndef INCLUDE_MAIN_XENOACTOR_H
#define INCLUDE_MAIN_XENOACTOR_H

/* Functions of src/main/xenoActor.c that other files call. */

struct XenoActor;

/* Transform block at +0x70 of the motion hierarchy (the table at actor +0x710). */
typedef struct XenoTransform {
    unsigned char unmodeled_00[4];
    float translation[3];                   /* 0x04 */
    float rotationX;                        /* 0x10 */
    float rotationY;                        /* 0x14 */
    float rotationZ;                        /* 0x18 */
    float scale[3];                         /* 0x1C */
    unsigned char unmodeled_28[0x44 - 0x28];
    float point[3];                         /* 0x44: point the transform is applied to */
} XenoTransform;

/* Motion hierarchy (HRC) of an actor (actor->table). */
typedef struct XenoHrc {
    unsigned char unmodeled_00[0xC];
    unsigned int numTransform;              /* 0x0C */
    int leadCountA;                         /* 0x10: leading entries before the focus joint; the three counts sum to its matrix index */
    int leadCountB;                         /* 0x14 */
    int leadCountC;                         /* 0x18 */
    int focus;                              /* 0x1C */
    int jointRecordCount;                   /* 0x20: number of joint records (XenoActor.jointRecords); also the fourth term of the child joint index */
    unsigned int jointCount;                /* 0x24 */
    unsigned char unmodeled_28[0x3C - 0x28];
    unsigned int count;                     /* 0x3C: item count */
    unsigned char unmodeled_40[0x4C - 0x40];
    int dataOffset;                         /* 0x4C: bytes from the table to its items */
    unsigned char unmodeled_50[0x5C - 0x50];
    unsigned int flags;                     /* 0x5C: bit 4 = has a focus joint */
    unsigned char unmodeled_60[0x70 - 0x60];
    XenoTransform transform;                /* 0x70 */
} XenoHrc;

/* Bit 0x100 of XenoActor.drawFlags: the focus pair is locked (set while cfeDampDirOffset's damped offsets are nonzero). */
#define XENO_DRAW_FOCUS_LOCKED 0x100

void XenoActorInit(struct XenoActor *actor);
void XenoActorInitHeap(int memory, int size);
void XenoActorClearHoldFlag(void);
int XenoActorAttachChild(struct XenoActor *parent, struct XenoActor *child);
void XenoActorUpdateTree(struct XenoActor *actor);
void XenoActorPushMatrix(struct XenoActor *actor);
void XenoActorShiftJoints(struct XenoActor *actor, int sign, int x, int y, int z);
/* XenoActorSetMode and XenoActorSetSelector are hand-declared elsewhere with other parameter types (int handle, void *, void return): not published here until those agree. */
struct XenoActor *XenoActorAlloc(void);
void XenoActorRelease(struct XenoActor *actor);
void XenoActorEnableChannel(struct XenoActor *actor);
void XenoActorEnableChannelB(struct XenoActor *actor);
void XenoActorPoolInitDefault(void);
void XenoActorReleaseAll(void);
void XenoActorSetChannelValue(struct XenoActor *actor, float value);
void XenoActorSetChannelValueB(struct XenoActor *actor, float value);
int XenoActorIsDone(struct XenoActor *actor);
int XenoActorHidePart(struct XenoActor *actor, const char *partName);
int XenoActorShowPart(struct XenoActor *actor, const char *partName);
void XenoActorApplySourceValue(struct XenoActor *actor);
void XenoActorSetPlayRange(struct XenoActor *actor, int start, int end);
void XenoActorSetFirstWord(struct XenoActor *actor, int value);
void XenoActorSetModeInSlot(struct XenoActor *actor, unsigned int slot, int mode);
float XenoActorSetSelector(struct XenoActor *actor, int selector);
void XenoActorSetHoldFlag(void);
void XenoActorDoit(void);
void XenoActorDisp(void);

#endif /* INCLUDE_MAIN_XENOACTOR_H */
