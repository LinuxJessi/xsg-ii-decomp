#include "common.h"
#include "main/sdv_033.h"
#include "main/pad.h"
#include "main/tu021.h"
#include "main/tu051.h"
#include "main/tu086.h"
#include "main/xenoActor.h"
#include "main/xgl_font.h"
#include "main/xgl_2.h"

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001D9280);

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraSaveState);

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraRestoreState);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001D9580);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001D95F0);

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraInit);

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraReset);

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraSetMode);

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraSetIndex);

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraSlotSetup);

#include "shared.h"

/* One of the four camera slots: indexA/indexB and the enable flags, set by cameraSlotSetup. */
struct CameraSlot {
    unsigned char unmodeled_00[2];
    signed char indexA;                 /* +0x02: stored by cameraSlotSetup when >= 0 */
    signed char indexB;                 /* +0x03: stored by cameraSlotSetup when >= 0 */
    unsigned char unmodeled_04[4];
    float enabled;                      /* +0x08: the enable flag as a float; a slot is active when nonzero */
    float disabled;                     /* +0x0C: 1.0 when the enable flag was 0 */
};

/* The camera state block at gameWork. */
struct CameraState {
    unsigned char unmodeled_000[8];
    /* A one-member union: the original reads this field with alias set 0, after the stores before it. */
    union {
        int control;                    /* +0x08: player control flags (0x1 paused, 0x8000 scripted, 0x10000 replayed input) */
    };
    unsigned char unmodeled_00C[0xC4];
    unsigned short flags;               /* +0xD0: 0x10 set by cameraApplySlots, 0x20 by cameraSetIndex and cameraApplyAllSlots, 0x40 by cameraSetMode */
    unsigned char unmodeled_0D2[3];
    signed char mode;                   /* +0xD5: set by cameraSetMode */
    unsigned char unmodeled_0D6[6];
    signed char trackIndex;             /* +0xDC: set by cameraSetIndex */
    unsigned char unmodeled_0DD[0x63];
    struct CameraSlot slot[4];
    unsigned char unmodeled_180[0x10];
    float stickAngle;                   /* +0x190: the camera yaw the stick directions are relative to */
    unsigned char unmodeled_194[0xC];
    /* src_* are staged vectors and dst_* the applied copies that cameraApplySlots writes per slot;
       the meaning of each vector is not established yet. */
    float src_1a0;
    float src_1a4;
    float src_1a8;
    unsigned char unmodeled_1AC[0x14];
    float src_1c0;
    float src_1c4;
    float src_1c8;
    float src_1cc;
    float src_1d0;
    float src_1d4;
    float src_1d8;
    float src_1dc;
    float src_1e0;
    float src_1e4;
    unsigned char unmodeled_1E8[0x4];
    float src_1ec;
    float dst_1f0;
    float dst_1f4;
    float dst_1f8;
    float dst_1fc;
    float dst_200;
    float dst_204;
    float dst_208;
    float dst_20c;
    float dst_210;
    float dst_214;
    unsigned char unmodeled_218[0x4];
    float dst_21c;
    unsigned char unmodeled_220[0x30];
    float dst_250;
    float dst_254;
    float dst_258;
    unsigned char unmodeled_25C[0x74];
    float frameScale;                   /* +0x2D0: elapsed frames, scales the per-frame movement */
    unsigned char unmodeled_2D4[0x3C];
    int replayStatus;                   /* +0x310: ground status recorded with the input */
    int replayStickX;                   /* +0x314 */
    int replayStickY;                   /* +0x318 */
    unsigned char unmodeled_31C[0x14];
    short fadeTimer;                    /* +0x330: frames left of the rider fade */
    unsigned char unmodeled_332[0x16];
    DoublewordAlignedVector4 extent;    /* +0x348 (MainWork in tu050) */
};

extern struct CameraState gameWork;
/* slot[0..3] of gameWork, reached by their own symbols (the scaffold names them): the original addresses
   the camera state as slot symbol minus its offset, and the relocations only match through these names. */
extern struct CameraSlot D_002B5ED0[];
extern struct CameraSlot D_002B5EE0[];
extern struct CameraSlot D_002B5EF0[];
extern struct CameraSlot D_002B5F00[];
int cameraSlotSetup(struct CameraSlot *slot, int index_a, int index_b, int enable_flag);

#define CAMERA_SLOT_MASK(slot) (1 << (slot))
#define CAMERA_FLAG_SLOTS_APPLIED 0x10

/* Applies the slots in slot_mask (zero: every slot whose enabled flag is set) and copies each slot's staged
   vectors to its applied fields; sets flag 0x10 when enable_flag is nonzero. */
void cameraApplySlots(int slot_mask, int index_a, int index_b, int enable_flag)
{
    if (slot_mask == 0) {
        if (gameWork.slot[0].enabled != 0.0f) {
            slot_mask = CAMERA_SLOT_MASK(0);
        }
        if (gameWork.slot[1].enabled != 0.0f) {
            slot_mask |= CAMERA_SLOT_MASK(1);
        }
        if (gameWork.slot[2].enabled != 0.0f) {
            slot_mask |= CAMERA_SLOT_MASK(2);
        }
        if (gameWork.slot[3].enabled != 0.0f) {
            slot_mask |= CAMERA_SLOT_MASK(3);
        }
        if (slot_mask == 0) {
            return;
        }
    }
    if (slot_mask & CAMERA_SLOT_MASK(0)) {
        struct CameraState *state = (struct CameraState *)((unsigned char *)D_002B5ED0 - 0x140);
        cameraSlotSetup(D_002B5ED0, index_a, index_b, enable_flag);
        state->dst_200 = state->src_1d0;
        state->dst_204 = state->src_1d4;
        state->dst_208 = state->src_1d8;
    }
    if (slot_mask & CAMERA_SLOT_MASK(1)) {
        struct CameraState *state = (struct CameraState *)((unsigned char *)D_002B5EE0 - 0x150);
        cameraSlotSetup(D_002B5EE0, index_a, index_b, enable_flag);
        state->dst_1f0 = state->src_1c0;
        state->dst_1f4 = state->src_1c4;
        state->dst_1f8 = state->src_1c8;
        state->dst_210 = state->src_1e0;
        state->dst_214 = state->src_1e4;
        state->dst_21c = state->src_1ec;
        state->dst_250 = state->src_1a0;
        state->dst_254 = state->src_1a4;
        state->dst_258 = state->src_1a8;
    }
    if (slot_mask & CAMERA_SLOT_MASK(2)) {
        struct CameraState *state = (struct CameraState *)((unsigned char *)D_002B5EF0 - 0x160);
        cameraSlotSetup(D_002B5EF0, index_a, index_b, enable_flag);
        state->dst_1fc = state->src_1cc;
    }
    if (slot_mask & CAMERA_SLOT_MASK(3)) {
        struct CameraState *state = (struct CameraState *)((unsigned char *)D_002B5F00 - 0x170);
        cameraSlotSetup(D_002B5F00, index_a, index_b, enable_flag);
        state->dst_20c = state->src_1dc;
    }
    if (enable_flag != 0) {
        gameWork.flags |= CAMERA_FLAG_SLOTS_APPLIED;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraAnchorGet);

/* Anchor record returned by cameraAnchorGet: the yaw and pitch used to turn a camera offset. */
struct CameraAnchor {
    unsigned char unmodeled_00[0x50];
    float rotation_x;
    float rotation_y;
};

void xglMatrixStackTrans();
struct CameraAnchor *cameraAnchorGet(Vector4 *, int, int);
void xglMatrixStackRotX(float);
void xglMatrixStackRotY(float);
void xglMatrixStackSave(float *);

/* Moves position by offset. With a valid anchor and a unit offset.w, the offset is turned by the anchor's
   yaw and pitch first; otherwise it is added as it is. Returns the anchor. */
struct CameraAnchor *cameraApplyAnchorOffset(Vector4 *position, int anchorIndex, int anchorSubIndex, Vector4 *offset)
{
    float matrix[16];
    struct CameraAnchor *node;

    node = cameraAnchorGet(position, anchorIndex, anchorSubIndex);
    if (node != 0 && offset != 0 && offset->w == 1.0f) {
        xglMatrixStackUnit();
        xglMatrixStackRotY(node->rotation_y);
        xglMatrixStackRotX(node->rotation_x);
        xglMatrixStackTrans(offset);
        xglMatrixStackSave(matrix);
        position->x += matrix[12];
        position->y += matrix[13];
        position->z += matrix[14];
    } else if (offset != 0) {
        position->x += offset->x;
        position->y += offset->y;
        position->z += offset->z;
    }
    return node;
}

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraApplyAllSlots);

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraSlotFind);

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraOrbitPosition);

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraAnglesBetween);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001D9FD0);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DA210);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DA468);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DAA38);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DABE8);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DAD30);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DADD0);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DAF10);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DB240);

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraEase);

/* Linear interpolation from `from` to `to` by `fraction`. */
float cameraLerpFloat(float from, float to, float fraction)
{
    return ((to - from) * fraction) + from;
}

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraLerpAngle);

/* VU0 linear interpolation of two vectors into destination, by fraction. */
void cameraLerpVector(Vector4 *destination, const Vector4 *from, const Vector4 *to, float fraction)
{
    __asm__ __volatile__(
        "lqc2 vf4, 0(%0)\n\t"
        "lqc2 vf2, 0(%1)\n\t"
        "lqc2 vf1, 0(%2)\n\t"
        "qmtc2.ni %3, vf3\n\t"
        "vsub.xyz vf4, vf2, vf1\n\t"
        "vmulx.xyz vf2, vf4, vf3x\n\t"
        "vadd.xyz vf4, vf2, vf1\n\t"
        "sqc2 vf4, 0(%0)"
        : : "r"(destination), "r"(to), "r"(from), "r"(fraction) : "memory");
}

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DB580);

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraAdvanceTracks);

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraUpdateFrame);

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraApplyView);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DC378);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DC468);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DC568);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DC860);

#define NULL ((void *) 0)

/* One entry of elvSlopeTable. */
typedef struct UwamonoSlope {
    float x;
    float z;
    float push;
    float speed;
} UwamonoSlope;

/* The probe block mapMoveProbe fills for the ground under the player. */
typedef struct UwamonoProbe {
    unsigned char unmodeled_00[0x18];
    int ground;                             /* +0x18 */
} UwamonoProbe;


extern int D_0069B988[];
extern int D_0069B978;
extern int D_0069B97C;
extern int D_0069B980;
extern short D_0069B992;
extern signed char D_0069B9B6;
extern float D_0069E184;
extern int D_0069E188;
extern PadRecord PadData;
extern UwamonoSlope elvSlopeTable[];
extern float D_002B9C98[];
extern unsigned char D_002B9CA8[];
extern UwamonoProbe D_002B9CB8;

/* Per jump height step (UwamonoSearch.level): the frames before the landing
 * check (D_0069B978) and before the target action (D_0069B97C). */
short D_0069B9A8[3] = { -1, -1, -1 };
short D_0069B9B0[3] = { -1, -1, -1 };

XglCamera *xglStudioGetCamera2(int index);
int uwamonoUpdateLadderState(void);
void XenoActorHidePart(struct XenoActor *actor, const char *part);
void XenoActorShowPart(struct XenoActor *actor, const char *part);
int func_001FD7F8(void);
void func_00207650(int kind, int id, int value);
void func_001FF3C0(int index);
void uwamonoReleaseHandles(void);
void XenoActorSetSelector(struct XenoActor *actor, int selector);
float I2F(int value);
float atan2f(float y, float x);
float func_002A1B30(float value);
float Get_Decimal_Surplus_for_Radius(float angle);
float xglSin(float angle);
float xglCos(float angle);
void XenoActorCheckMode(struct XenoActor *actor, int mode);
void XenoActorSetMode(struct XenoActor *actor, int mode);
int func_001FEAC8(struct XenoActor *actor, float range);
unsigned int msgWindowInUse(void);
void talkSetResult(int id);
void mapObjectSetActive(UwamonoObject *object, UwamonoObject *target);
void func_001FE5B0(int index, int value);
void func_001FE830(int index, int kind, int frames);
void func_00205D20(int index, AlignedVector4 *position);
void func_001FDE18(int step);
float Get_Angle(const Vector4 *first, const Vector4 *second);
float cursolGetByReduceSpeedAngleLoop(float current, float target, float speed);
int Get_Rnd(int min, int max);
int sefSpawnEffectRecordById(int id, int arg1, int arg2);
void sefSetRecordOwner(int record, struct XenoActor *owner);
void sefSetRecordValues(int record, struct XenoActor *first, struct XenoActor *second, int value);
void func_00208B50(int value);
void elvGetPosition(int index, int arg, Vector4 *position);
float mapMoveProbe(const Vector4 *from, const Vector4 *to, void *result);
void mapProbeInit(UwamonoProbe *probe);
void uwamonoHitCheckWithNyuru(struct XenoActor *actor);
int func_001DE738(int index, int arg, float range);
void func_001DE8F0(int index, struct XenoActor *actor);
void uwamonoPickTarget(short index);
void func_00205C78(void);
void uwamonoSelectTarget(int index);
void func_0029E690(void);

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))


#define UWAMONO_IS_ES(actor) ((unsigned int) ((actor)->kind - 0x10000) < 0x4000)

/* Stick directions snap to 16 sectors: pi/8 per sector, offset by half a sector. */
#define EIGHTH_TURN 0.39269909f
#define EIGHTH_TURN_HALF 0.19634955f
#define QUARTER_TURN_FLOAT 0.78539819f /* pi/4 as the original float constant */

/* Draw flags of a XenoActor (drawFlags). */
#define UWAMONO_DRAW_HIDDEN 0x4
#define UWAMONO_DRAW_TRANSLUCENT 0x08000000

/* Record keys: the ES range of the party starts at 0x10000; 0x10026 is Jin's ES. */
#define UWAMONO_KEY_JIN_ES 0x10026
#define UWAMONO_KEY_IN(key, first, count) ((unsigned int) ((key) - (first)) < (count))

/* Ground status classes of XenoActor.status. */
#define GROUND_SLOPE 0x10000
#define GROUND_LADDER 0x100
#define GROUND_ELEVATOR 0x200

/* Candidate target points and their reach (10 entries). */
extern Vector4 D_0033EE00[];
extern float D_0033EE90[];
extern int D_002B5D98[];

float Get_Distance(const Vector4 *from, const Vector4 *to);
int uwamonoTryClearTarget(int index, signed char threshold);
int uwamonoFindNearbyTarget(int index);
void uwamonoUpdateTargetVector(int index);

/*
 * Picks the target of a map object: the nearest point in reach first, then
 * the script target, then a nearby object that can be targeted.
 */
void uwamonoPickTarget(short index)
{
    UwamonoMotionTable *motions = uwamonoRecords[index].motions;
    float nearest = 1000.0f;
    short found = -1;
    int scriptTarget;
    int nearby;
    int i;

    for (i = 0; i < 10; i++) {
        float distance = Get_Distance(&D_0033EE00[i], &uwamonoRecords[index].position);

        if (D_0033EE90[i] != 0.0f && distance <= D_0033EE90[i] && distance < nearest) {
            nearest = distance;
            found = i;
        }
    }
    if (found != -1) {
        uwamonoOfferTarget(index, 2, -1, &D_0033EE00[found]);
        uwamonoUpdateTargetVector(index);
        return;
    }
    uwamonoTryClearTarget(index, 2);
    if (motions->targetPriority == 3) {
        uwamonoUpdateTargetVector(index);
        return;
    }
    if (D_002B5D98[0] & CONTROL_SCRIPTED) {
        uwamonoTryClearTarget(index, 0);
        return;
    }
    scriptTarget = func_001FD7F8();
    if (scriptTarget != -1) {
        uwamonoOfferTarget(index, 0, scriptTarget, NULL);
        uwamonoUpdateTargetVector(index);
        return;
    }
    uwamonoTryClearTarget(index, 0);
    nearby = uwamonoFindNearbyTarget(index);
    if (nearby == -1) {
        uwamonoTryClearTarget(index, 1);
        return;
    }
    if (uwamonoRecords[nearby].flags & 2) {
        uwamonoOfferTarget(index, 1, nearby, NULL);
        uwamonoUpdateTargetVector(index);
        return;
    }
    /* The original tests the result here: the call is a jal + jr, not a tail call. */
    if (uwamonoTryClearTarget(index, 1)) {
        return;
    }
}

/*
 * Per-frame control of the player's map object: rider fade, input, walk
 * motions, talk/search targets, jumps, ground status and the move probe, then
 * copies the result to the record and to the riders.
 */
void uwamonoPlayerUpdate(void)
{
    /* record and the indexed uwamonoRecords[uwamonoCurrentIndex] reads below are the same object; the mix
       is kept for the original code generation. */
    UwamonoObject *record = &uwamonoRecords[uwamonoCurrentIndex];
    struct XenoActor *actor = uwamonoRecords[uwamonoCurrentIndex].actor;
    struct XenoActor *riders[2];
    Vector4 unused; /* Kept: the original builds this (0, 0, 0, 1) vector and never reads it. */
    Vector4 elevator;
    AlignedVector4 old;
    float speed;
    float frameScale;
    int i;
    int riding;
    int moving;
    int walk;
    int target;
    int stick_x;
    int stick_y;
    unsigned int status;

    if (D_0069B988[0] != -1) {
        riders[0] = uwamonoRecords[D_0069B988[0]].actor;
    } else {
        riders[0] = NULL;
    }
    if (D_0069B988[1] != -1) {
        riders[1] = uwamonoRecords[D_0069B988[1]].actor;
    } else {
        riders[1] = NULL;
    }
    actor->previous.vector.x = actor->position.x;
    actor->previous.vector.y = actor->position.y;
    actor->previous.vector.z = actor->position.z;
    actor->previous.vector.w = actor->position.w;
    actor->moved = 0;
    for (i = 0; i < 2; i++) {
        if (riders[i] != NULL) {
            riders[i]->moved = 0;
            riders[i]->radius = actor->radius;
            riders[i]->previous.vector.x = actor->position.x;
            riders[i]->previous.vector.y = actor->position.y;
            riders[i]->previous.vector.z = actor->position.z;
            riders[i]->previous.vector.w = actor->position.w;
        }
    }
    if (gameWork.control & CONTROL_PAUSED) {
        return;
    }
    if (xglStudioGetCamera2(0)->state != 4) {
        return;
    }
    if (gameWork.fadeTimer != 0) {
        if (--gameWork.fadeTimer <= 0) {
            record->fade = 0.0f;
            gameWork.fadeTimer = 0;
            actor->alpha = 1.0f;
            actor->drawFlags &= ~UWAMONO_DRAW_TRANSLUCENT;
            if (riders[0] != NULL) {
                riders[0]->alpha = 1.0f;
                riders[0]->drawFlags &= ~UWAMONO_DRAW_TRANSLUCENT;
            }
            if (riders[1] != NULL) {
                riders[1]->alpha = 1.0f;
                riders[1]->drawFlags &= ~UWAMONO_DRAW_TRANSLUCENT;
            }
        } else if (gameWork.fadeTimer % 2 == 0 || gameWork.fadeTimer >= 23) {
            actor->drawFlags |= UWAMONO_DRAW_TRANSLUCENT;
            actor->alpha = (100.0f - record->fade) / 100.0f;
            if (riders[0] != NULL) {
                riders[0]->drawFlags |= UWAMONO_DRAW_TRANSLUCENT;
                riders[0]->alpha = (100.0f - record->fade) / 100.0f;
            }
            if (riders[1] != NULL) {
                riders[1]->drawFlags |= UWAMONO_DRAW_TRANSLUCENT;
                riders[1]->alpha = (100.0f - record->fade) / 100.0f;
            }
        } else {
            actor->alpha = 1.0f;
            actor->drawFlags &= ~UWAMONO_DRAW_TRANSLUCENT;
            if (riders[0] != NULL) {
                riders[0]->alpha = 1.0f;
                riders[0]->drawFlags &= ~UWAMONO_DRAW_TRANSLUCENT;
            }
            if (riders[1] != NULL) {
                riders[1]->alpha = 1.0f;
                riders[1]->drawFlags &= ~UWAMONO_DRAW_TRANSLUCENT;
            }
        }
    }
    if (uwamonoUpdateLadderState() == 0) {
        speed = 0.0f;
        frameScale = gameWork.frameScale;
        if (D_0069B978 >= 0 && record->key == UWAMONO_KEY_JIN_ES) {
            if (actor->mode == 5 && actor->motionFrame >= 18.0f) {
                /* The original calls it twice in a row (and ShowPart below the same). */
                XenoActorHidePart(riders[0], "jin_saya_tuka");
                XenoActorHidePart(riders[0], "jin_saya_tuka");
            }
            if (record->key == UWAMONO_KEY_JIN_ES && actor->mode == 5 && actor->motionFrame >= 50.0f) {
                XenoActorShowPart(riders[0], "jin_saya_tuka");
                XenoActorShowPart(riders[0], "jin_saya_tuka");
            }
        }
        if (D_0069B97C != -1 && --D_0069B97C == -1 && D_0069B980 < 6 && D_0069B980 >= 3) {
            target = func_001FD7F8();
            if (target != -1) {
                func_00207650(1, uwamonoRecords[target].id, 0);
                if (uwamonoRecords[target].flags & 0x40000) {
                    func_001FF3C0(target);
                    uwamonoReleaseHandles();
                }
            }
        }
        if (D_0069B978 >= 0) {
            D_0069B978--;
        }
        /* "Wait %3d" */
        xglFontDebugPrintf(8, 0x40, "\xa5\xa6\xa5\xa7\xa5\xa4\xa5\xc8 %3d", D_0069B978);
        if (D_0069B978 == -1) {
            D_0069B978 = 0;
            if (D_0069B980 < 6) {
                if (D_0069B980 >= 3) {
                    XenoActorSetChannelValueB(actor, 4.0f);
                    XenoActorSetSelector(actor, 0);
                    gameWork.control &= ~CONTROL_SEARCHING;
                }
            }
            D_0069B980 = -1;
            if (UWAMONO_IS_ES(actor)) {
                /* do/while(0) with break: kept for the original branch layout (same in the blocks below). */
                do {
                    if (UWAMONO_KEY_IN(record->key, 0x10025, 7)) {
                        if (riders[0] != NULL) {
                            riders[0]->drawFlags &= ~UWAMONO_DRAW_HIDDEN;
                        }
                        if (riders[1] != NULL) {
                            riders[1]->drawFlags |= UWAMONO_DRAW_HIDDEN;
                        }
                        break;
                    }
                    if (UWAMONO_KEY_IN(record->key, 0x10015, 6)) {
                        if (riders[0] != NULL) {
                            riders[0]->drawFlags &= ~UWAMONO_DRAW_HIDDEN;
                        }
                        if (riders[1] != NULL) {
                            riders[1]->drawFlags &= ~UWAMONO_DRAW_HIDDEN;
                        }
                        break;
                    }
                    if (riders[0] != NULL) {
                        riders[0]->drawFlags |= UWAMONO_DRAW_HIDDEN;
                    }
                    if (riders[1] != NULL) {
                        riders[1]->drawFlags |= UWAMONO_DRAW_HIDDEN;
                    }
                } while (0);
            }
            if (!(gameWork.control & CONTROL_SCRIPTED) || (gameWork.control & CONTROL_REPLAYED)) {
                float x;
                float y;
                float angle;
                float step;
                int analog;
                int digital;

                if (gameWork.control & CONTROL_REPLAYED) {
                    stick_x = gameWork.replayStickX;
                    stick_y = gameWork.replayStickY;
                    actor->status = gameWork.replayStatus;
                } else {
                    /* Analog stick and the pad's digital direction (scaled to stick range): the one with
                       the larger magnitude wins, opposite signs cancel. */
                    analog = PadData.stickX;
                    digital = PadData.directionX * 2;
                    if (analog != 0 && digital != 0 && (analog ^ digital) < 0) {
                        stick_x = 0;
                    } else if (analog < 0 || digital < 0) {
                        stick_x = MIN(analog, digital);
                    } else {
                        stick_x = MAX(analog, digital);
                    }
                    analog = PadData.stickY;
                    digital = PadData.directionY * 2;
                    if (analog != 0 && digital != 0 && (analog ^ digital) < 0) {
                        stick_y = 0;
                    } else if (analog < 0 || digital < 0) {
                        stick_y = MIN(analog, digital);
                    } else {
                        stick_y = MAX(analog, digital);
                    }
                    gameWork.replayStickX = stick_x;
                    gameWork.replayStickY = stick_y;
                    gameWork.replayStatus = actor->status;
                }
                if (stick_x == 0 && stick_y == 0) {
                    D_0069E184 = gameWork.stickAngle;
                }
                x = I2F(stick_x);
                y = I2F(stick_y);
                speed = __builtin_sqrtf(x * x + y * y);
                if (speed > 256.0f) {
                    speed = 256.0f;
                }
                angle = Get_Decimal_Surplus_for_Radius(
                    func_002A1B30((atan2f(x, y) + EIGHTH_TURN_HALF) / EIGHTH_TURN) * EIGHTH_TURN + D_0069E184);
                if (speed > 0.0f) {
                    actor->direction.y = angle;
                }
                if ((PadData.held & PAD_R2) && speed > 191.0f) {
                    speed = 191.0f;
                }
                step = speed * frameScale;
                actor->velocity.x += xglSin(actor->direction.y) * step;
                actor->velocity.z += xglCos(actor->direction.y) * step;
            }
            if (!(gameWork.control & CONTROL_SCRIPTED)) {
                do {
                    if (speed <= 0.0f) {
                        actor->motionSpeed = 1.0f;
                        walk = 0;
                        break;
                    }
                    if (speed < 192.0f) {
                        actor->motionSpeed = speed * 2.6f / 192.0f;
                        walk = 1;
                        break;
                    }
                    walk = 2;
                    actor->motionSpeed = speed * (1.0f / 256.0f);
                } while (0);
                if (actor->mode != walk && (walk != 0 || actor->mode != 3) && (walk != 0 || actor->mode != 4)
                    && (walk != 0 || actor->mode != 5)) {
                    actor->flags &= ~2;
                    if (D_0069B9B6 != 0) {
                        XenoActorEnableChannel(actor);
                    }
                    XenoActorCheckMode(actor, uwamonoRecords[uwamonoCurrentIndex].motions->walk[walk]);
                    XenoActorSetChannelValue(actor, 4.0f);
                    /* moving and riding are kept as locals: the original goes through them for its code generation. */
                    moving = 1;
                    riding = UWAMONO_IS_ES(actor);
                    D_0069B9B6 = moving;
                    if (riding) {
                        for (i = 0; i < 2; i++) {
                            if (riders[i] != NULL) {
                                XenoActorSetChannelValue(riders[i], 4.0f);
                                XenoActorEnableChannel(riders[i]);
                                XenoActorSetMode(riders[i], walk);
                            }
                        }
                    }
                }
                target = func_001FEAC8(actor, 50.0f);
                /* "Talk target -> %8d" */
                xglFontDebugPrintf(8, 0x20, "\xa5\xc8\xa1\xbc\xa5\xaf\xc2\xd0\xbe\xdd \xa2\xaa %8d", target);
                if (target != -1) {
                    if (PadData.pressed & PAD_CIRCLE) {
                        UwamonoObject *object = &uwamonoRecords[target];

                        switch (object->kind) {
                        case 2:
                            if (UWAMONO_IS_ES(actor)) {
                                if (uwamonoRecords[target].searchState == 0) {
                                    func_001FE5B0(target, 0);
                                }
                            } else if (uwamonoRecords[target].searchState == 2) {
                                func_001FE5B0(target, 0);
                            }
                            func_001FE830(target, 5, 30);
                            break;
                        case 1: {
                            struct XenoActor *other = uwamonoRecords[target].actor;

                            if (UWAMONO_KEY_IN(other->kind, 0x10000, 0x8000) && msgWindowInUse() == 0) {
                                talkSetResult(object->id);
                                mapObjectSetActive(record, object);
                                XenoActorEnableChannel(record->actor);
                                XenoActorSetMode(record->actor, 0);
                            }
                            break;
                        }
                        }
                    } else if (uwamonoRecords[target].flags & 0x800) {
                        if (msgWindowInUse() == 0) {
                            talkSetResult(uwamonoRecords[target].id);
                        }
                    } else {
                        mapObjectSetActive(record, NULL);
                    }
                } else {
                    mapObjectSetActive(record, NULL);
                }
                if (walk == 2) {
                    func_00205D20(uwamonoCurrentIndex, &uwamonoRecords[uwamonoCurrentIndex].actor->position);
                }
                if (walk != 0) {
                    func_001FDE18(0);
                }
                if (!(gameWork.control & CONTROL_SCRIPTED) && !(gameWork.control & CONTROL_SEARCHING) && D_0069B97C == -1) {
                    if (PadData.pressed & PAD_L1) {
                        func_001FDE18(1);
                    }
                    if (PadData.pressed & PAD_R1) {
                        func_001FDE18(-1);
                    }
                    if (PadData.pressed & PAD_SQUARE) {
                        int searched;
                        int jumpMode;

                        searched = func_001FD7F8();
                        if (searched != -1 && (uwamonoRecords[searched].flags & 0x20000)) {
                            gameWork.control |= CONTROL_SEARCHING;
                            func_00207650(0, uwamonoRecords[searched].id, 0);
                            actor->direction.y = Get_Angle(&actor->position, &uwamonoRecords[searched].position);
                            actor->rotation.y = cursolGetByReduceSpeedAngleLoop(actor->rotation.y, actor->direction.y, 4.0f);
                            /* The smoothed value above is overwritten at once; the call stays for the original code. */
                            actor->rotation.y = Get_Angle(&actor->position, &uwamonoRecords[searched].position);
                            XenoActorSetChannelValue(actor, 0.0f);
                            D_0069B9B6 = 0;
                            XenoActorSetMode(actor, uwamonoRecords[searched].search.level + 3);
                            actor->requested_mode = 0;
                            D_0069B978 = D_0069B9A8[uwamonoRecords[searched].search.level];
                            D_0069B97C = D_0069B9B0[uwamonoRecords[searched].search.level];
                            if (UWAMONO_IS_ES(actor)) {
                                for (i = 0; i < 2; i++) {
                                    if (riders[i] != NULL) {
                                        XenoActorSetChannelValue(riders[i], 0.0f);
                                        XenoActorEnableChannel(riders[i]);
                                        XenoActorSetMode(riders[i], uwamonoRecords[searched].search.level + 3);
                                        riders[i]->flags |= 2;
                                    }
                                }
                                do {
                                    if (UWAMONO_KEY_IN(record->key, 0x10025, 7)) {
                                        if (riders[0] != NULL) {
                                            riders[0]->drawFlags &= ~UWAMONO_DRAW_HIDDEN;
                                        }
                                        if (riders[1] != NULL) {
                                            riders[1]->drawFlags &= ~UWAMONO_DRAW_HIDDEN;
                                        }
                                        break;
                                    }
                                    if (UWAMONO_KEY_IN(record->key, 0x10015, 6)) {
                                        if (riders[0] != NULL) {
                                            riders[0]->drawFlags &= ~UWAMONO_DRAW_HIDDEN;
                                        }
                                        if (riders[1] != NULL) {
                                            riders[1]->drawFlags &= ~UWAMONO_DRAW_HIDDEN;
                                        }
                                        break;
                                    }
                                    if (riders[0] != NULL) {
                                        riders[0]->drawFlags &= ~UWAMONO_DRAW_HIDDEN;
                                    }
                                    if (riders[1] != NULL) {
                                        riders[1]->drawFlags |= UWAMONO_DRAW_HIDDEN;
                                    }
                                } while (0);
                                seVoiceStartDefault(uwamonoRecords[searched].search.level * 3 + Get_Rnd(0, 2) + 0x07000001);
                                seVoiceStartDefault(uwamonoRecords[searched].search.level + 0x0700000B);
                                D_0069E188 = sefSpawnEffectRecordById((D_0069B992 << 12) | (uwamonoRecords[searched].search.level + 1) | 0x10000000, 0, 0);
                            } else if (uwamonoRecords[searched].search.level == 0) {
                                seVoiceStartDefault(0x0700000B);
                                D_0069E188 = sefSpawnEffectRecordById((D_0069B992 << 12) | 0x10000001, 0, 0);
                            } else {
                                seVoiceStartDefault(0x0700000C);
                                D_0069E188 = sefSpawnEffectRecordById((D_0069B992 << 12) | 0x10000002, 0, 0);
                            }
                            sefSetRecordOwner(D_0069E188, actor);
                            sefSetRecordValues(D_0069E188, riders[0], riders[1], 0);
                            /* jumpMode is kept as a local for the original code generation. */
                            jumpMode = uwamonoRecords[searched].search.level + 3;
                            actor->motionSpeed = 1.0f;
                            D_0069B980 = jumpMode;
                            XenoActorSetChannelValueB(actor, 4.0f);
                            XenoActorSetSelector(actor, 5);
                            func_00208B50(4);
                        }
                    }
                }
            }
        }
        status = actor->status;
        if ((status & 0xFFFF0000) == GROUND_SLOPE) {
            float angle;
            float distance;
            unsigned int slope = status & 0xF0F;

            if (!(slope & 0xF00)) {
                float factor;

                if (slope & 8) {
                    UwamonoSlope *entry = &elvSlopeTable[slope & 7];
                    float push = frameScale * entry->push * 256.0f;

                    actor->velocity.x += entry->x * push;
                    actor->velocity.z += entry->z * push;
                    factor = entry->speed;
                } else {
                    factor = D_002B9C98[slope & 3];
                }
                angle = actor->direction.y;
                distance = frameScale * ((factor - 1.0f) * speed);
            } else {
                angle = I2F(D_002B9CA8[slope >> 8]) * QUARTER_TURN_FLOAT;
                distance = frameScale * D_002B9C98[slope & 3] * 256.0f;
            }
            actor->velocity.x += xglSin(angle) * distance;
            actor->velocity.z += xglCos(angle) * distance;
        }
        if ((actor->status & ~0xFF) == GROUND_ELEVATOR) {
            elvGetPosition(actor->status & 0xF, 0, &elevator);
            actor->velocity.x += elevator.x;
            actor->velocity.z += elevator.z;
        }
        if (!(gameWork.control & CONTROL_SCRIPTED)) {
            actor->rotation.y = cursolGetByReduceSpeedAngleLoop(actor->rotation.y, actor->direction.y, 4.0f);
        }
        actor->hitFlags = (actor->hitFlags & ~7) | 0x302;
        if (!(uwamonoRecords[uwamonoCurrentIndex].flags & 0x04000000)) {
            float height;

            old = actor->position;
            height = mapMoveProbe(&actor->position, &actor->velocity, actor->hitBuffer);
            actor->velocity.x = actor->position.x - old.x;
            actor->velocity.z = actor->position.z - old.z;
            if (height != -1000.0f) {
                if ((actor->groundFlags & 0x10) && actor->velocity.y != 0.0f) {
                    actor->velocity.y = -0.011f;
                }
                if (actor->position.y < height
                    || (actor->velocity.y == 0.0f && actor->position.y - actor->stepHeight < height)) {
                    actor->position.y = height;
                    actor->velocity.y = 0.0f;
                } else {
                    actor->position.y += actor->velocity.y;
                    actor->velocity.y += -0.011f;
                }
            } else {
                actor->velocity.x = 0.0f;
                actor->velocity.y = 0.0f;
                actor->velocity.z = 0.0f;
            }
        }
        uwamonoHitCheckWithNyuru(actor);
        target = func_001DE738(uwamonoCurrentIndex, 0, 1.0f);
        if (target != -1) {
            func_001DE8F0(uwamonoCurrentIndex, uwamonoRecords[target].actor);
        }
        actor->velocity.x = actor->velocity.z = 0.0f;
        if (!(gameWork.control & 0x20) && (actor->status & ~0x1F) == GROUND_LADDER) {
            if (!(gameWork.control & 0x420)) {
                gameWork.control |= 0x400;
                func_00207650(3, (actor->status & 0x1F) + 1, 0);
            }
            gameWork.control |= 0x400;
        } else {
            gameWork.control &= ~0x400;
        }
    } else {
        if (UWAMONO_IS_ES(actor)) {
            do {
                if (UWAMONO_KEY_IN(record->key, 0x10025, 7)) {
                    if (riders[0] != NULL) {
                        riders[0]->drawFlags &= ~UWAMONO_DRAW_HIDDEN;
                    }
                    if (riders[1] != NULL) {
                        riders[1]->drawFlags |= UWAMONO_DRAW_HIDDEN;
                    }
                    break;
                }
                if (UWAMONO_KEY_IN(record->key, 0x10015, 6)) {
                    if (riders[0] != NULL) {
                        riders[0]->drawFlags &= ~UWAMONO_DRAW_HIDDEN;
                    }
                    if (riders[1] != NULL) {
                        riders[1]->drawFlags &= ~UWAMONO_DRAW_HIDDEN;
                    }
                    break;
                }
                if (riders[0] != NULL) {
                    riders[0]->drawFlags |= UWAMONO_DRAW_HIDDEN;
                }
                if (riders[1] != NULL) {
                    riders[1]->drawFlags |= UWAMONO_DRAW_HIDDEN;
                }
            } while (0);
        }
    }
    mapProbeInit(&D_002B9CB8);
    D_002B9CB8.ground = actor->ground;
    mapMoveProbe(&actor->position, NULL, &D_002B9CB8);
    record->position = actor->position;
    record->anchor = actor->rotation;
    if (UWAMONO_IS_ES(actor)) {
        for (i = 0; i < 2; i++) {
            if (riders[i] != NULL) {
                riders[i]->position = actor->position;
                riders[i]->rotation = actor->rotation;
                riders[i]->direction = actor->direction;
                riders[i]->vector90 = actor->vector90;
                riders[i]->vector70 = actor->vector70;
                riders[i]->vector80 = actor->vector80;
                riders[i]->velocity = actor->velocity;
                riders[i]->radius = actor->radius;
            }
        }
    }
    unused.x = unused.y = unused.z = 0.0f;
    unused.w = 1.0f;
    uwamonoPickTarget(uwamonoCurrentIndex);
    func_00205C78();
    uwamonoSelectTarget(uwamonoCurrentIndex);
    func_0029E690();
}

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DE738);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DE8F0);


/* The record at uwamonoCurrentIndex, or null while no record is selected (index below zero). */
void *uwamonoCurrentRecord(void)
{
    if (!(uwamonoCurrentIndex >= 0)) {
        return 0;
    }
    return &uwamonoRecords[uwamonoCurrentIndex];
}
