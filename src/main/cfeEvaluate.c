#include "common.h"
#include "shared.h"
#include "main/cfePlugIn.h"
#include "main/sef.h"
#include "main/sdv_033.h"
#include "main/tu021.h"
#include "main/tu138.h"
#include "main/tu256.h"
#include "gs.h"
#include "main/tu045.h"
#include "cfeEvaluate.h"
#include "main/tu145.h"
#include "main/xgl_studio.h"
#include "main/nml_model_set.h"
#include "main/tu093.h"
#include "main/tu255.h"
#include "main/xgl_2.h"
#include "main/tu026.h"
#include "main/tu051.h"
#include "main/xenoActor.h"


/* The object whose flag words (+0x4C0) the records set; +0x5EC is its record list. */
typedef struct CfeFlagOwner {
    unsigned char unmodeled_000[0x4C0];
    u32 flags[(0x5EC - 0x4C0) / 4];
    struct CfeFlagList *list;
} CfeFlagOwner;

typedef struct CfeFlagList {
    unsigned char unmodeled_00[0x60];
    int count;
} CfeFlagList;

/* 1 when the list exists and has items. */
int cfeListHasItems(CfeFlagList *list)
{
    if (list != 0 && list->count != 0) {
        return 1;
    }
    return 0;
}

/* Samples a flag record: its track at `time` into its value and into bit
 * `index` of the owner's flag words. */
void cfeRecordSample(int time, CfeFlagOwner *owner, CfeFlagRecord *record, int index)
{
    int value;

    if (record->track[0] != 0) {
        value = (int) func_002508C8(record->track[0], time);
        record->value = value;
        if (value != 0) {
            owner->flags[index >> 5] |= 1 << (index & 0x1F);
        } else {
            owner->flags[index >> 5] &= ~(1 << (index & 0x1F));
        }
    }
}

/* Samples every record of the owner's list (records[i] into flag bit i). */
void cfeRecordListSample(int time, CfeFlagOwner *owner, CfeFlagRecord *records)
{
    CfeFlagList *list = owner->list;
    int i;

    if (cfeListHasItems(list)) {
        for (i = 0; i < list->count; i++) {
            cfeRecordSample(time, owner, &records[i], i);
        }
    }
}

void XenoActorSetModeInSlot(struct XenoActor *actor, int slot, int mode);
float XenoActorSetSelector(struct XenoActor *actor, int selector);
int XenoActorReleaseChild(struct XenoActor *parent, struct XenoActor *child);
int XenoActorAppendChildOnJoint(struct XenoActor *parent, struct XenoActor *child, int joint);
/* TiMRPackage.c takes its own view of the state (TiMRState). */
void tiMRStateSetSpeed(XenoMotionState *state, float value);
extern const char D_0068D140[];
extern const char D_0068D180[];
extern const char D_0068D198[];

/* The 0x1B0-byte entries of cfePlugInSlots as the attachment reads them: word 0 is the parent actor. */
typedef struct CfeNodeSlot {
    struct XenoActor *object;
    unsigned char unmodeled_004[0x1B0 - 4];
} CfeNodeSlot;

extern CfeNodeSlot cfePlugInSlots[];


/* XenoActor's vector at +0x90 as the node's scale tracks write it. The original reaches it through a
 * union here: after a store to a component the node's actor pointer is loaded again (alias set 0).
 * uwamonoPlayerUpdate needs the plain member of XenoActor, so the union is this file's view. */
typedef struct CfeActorScale {
    unsigned char unmodeled_000[0x90];
    union {
        AlignedVector4 scale;
    };
} CfeActorScale;

/* XenoActor drawFlags: hidden, translucent and drawn with the alpha. */
#define CFE_ACTOR_HIDDEN 0x4
#define CFE_ACTOR_ALPHA 0x10000
#define CFE_ACTOR_TRANSLUCENT 0x2000000

/* Evaluates one entry of cfeEntries at `time`: every track that is set drives its value (the actor's transform,
 * channel values, mode, selector, play range, speeds, draw state and attachment, the flag records and the entry
 * position), and the entry position is moved from the world into the space in front of the actor. */
void cfeEvaluateNode(void *data, int time)
{
    CfeNode *entry = data;
    struct XenoActor *actor;
    struct XenoActor *shown;
    float alpha;
    Vector4 offset;
    int slot;
    int changed;

    if (entry->transformTrack[0] != 0) {
        entry->actor->position.x = cfeTrackSample(entry->transformTrack[0], time);
    }
    if (entry->transformTrack[1] != 0) {
        entry->actor->position.y = cfeTrackSample(entry->transformTrack[1], time);
    }
    if (entry->transformTrack[2] != 0) {
        entry->actor->position.z = cfeTrackSample(entry->transformTrack[2], time);
    }
    if (entry->transformTrack[3] != 0) {
        entry->actor->rotation.x = cfeTrackSample(entry->transformTrack[3], time);
    }
    if (entry->transformTrack[4] != 0) {
        entry->actor->rotation.y = cfeTrackSample(entry->transformTrack[4], time);
    }
    if (entry->transformTrack[5] != 0) {
        entry->actor->rotation.z = cfeTrackSample(entry->transformTrack[5], time);
    }
    if (entry->transformTrack[6] != 0) {
        ((CfeActorScale *) entry->actor)->scale.x = cfeTrackSample(entry->transformTrack[6], time);
    }
    if (entry->transformTrack[7] != 0) {
        ((CfeActorScale *) entry->actor)->scale.y = cfeTrackSample(entry->transformTrack[7], time);
    }
    if (entry->transformTrack[8] != 0) {
        ((CfeActorScale *) entry->actor)->scale.z = cfeTrackSample(entry->transformTrack[8], time);
    }
    offset.x = entry->position.x - entry->actor->position.x;
    offset.y = entry->position.y - entry->actor->position.y;
    offset.z = entry->position.z - entry->actor->position.z;
    entry->position.z = __builtin_sqrtf(offset.x * offset.x + offset.z * offset.z);
    entry->position.y = offset.y;
    entry->position.x = 0.0f;
    xglCurSetRotateY(entry->actor->rotation.y);
    xglCurApplyVector(&entry->position, &entry->position);
    entry->position.x += entry->actor->position.x;
    entry->position.y += entry->actor->position.y;
    entry->position.z += entry->actor->position.z;
    if (entry->channelTrack != 0) {
        entry->channelValue = func_002508C8(entry->channelTrack, time);
        XenoActorSetChannelValue(entry->actor, entry->channelValue);
        XenoActorSetChannelValueB(entry->actor, entry->channelValue);
    }
    slot = entry->slot;
    if (entry->slotTrack != 0) {
        slot = func_002508C8(entry->slotTrack, time);
    }
    if (entry->speedTrack[0] != 0) {
        entry->speed[0] = func_002508C8(entry->speedTrack[0], time);
        tiMRStateSetSpeed(&entry->actor->motion[0], entry->speed[0]);
    }
    if (entry->speedTrack[1] != 0) {
        entry->speed[1] = func_002508C8(entry->speedTrack[1], time);
        tiMRStateSetSpeed(&entry->actor->motion[1], entry->speed[1]);
    }
    if (entry->forceModeTrack != 0) {
        entry->forceMode = func_002508C8(entry->forceModeTrack, time);
    }
    if (entry->modeTrack != 0) {
        entry->mode = func_002508C8(entry->modeTrack, time);
        if (entry->actor->mode != entry->mode || slot != entry->slot || time == cfeRangeGetMin()
            || entry->forceMode != 0) {
            entry->slot = slot;
            XenoActorEnableChannel(entry->actor);
            XenoActorSetModeInSlot(entry->actor, entry->slot, entry->mode);
            entry->playStart = entry->actor->motion[0].range[0];
            entry->playEnd = entry->actor->motion[0].range[1];
        }
    }
    changed = 0;
    if (entry->playRangeTrack[0] != 0) {
        entry->playStart = func_002508C8(entry->playRangeTrack[0], time);
        changed = 1;
    }
    if (entry->playRangeTrack[1] != 0) {
        entry->playEnd = func_002508C8(entry->playRangeTrack[1], time);
        changed = 1;
    }
    if (changed) {
        XenoActorSetPlayRange(entry->actor, entry->playStart, entry->playEnd);
    }
    if (entry->selectorTrack != 0) {
        entry->selector = func_002508C8(entry->selectorTrack, time);
        if (entry->actor->selector != entry->selector || time == cfeRangeGetMin()) {
            XenoActorEnableChannelB(entry->actor);
            XenoActorSetSelector(entry->actor, entry->selector);
        }
    }
    if (entry->drawModeTrack != 0) {
        entry->drawMode = func_002508C8(entry->drawModeTrack, time);
        entry->actor->drawMode = entry->drawMode;
    }
    if (entry->translucentTrack != 0) {
        entry->translucent = func_002508C8(entry->translucentTrack, time);
    }
    if (entry->translucent != 0) {
        entry->actor->drawFlags |= CFE_ACTOR_TRANSLUCENT;
    } else {
        entry->actor->drawFlags &= ~CFE_ACTOR_TRANSLUCENT;
    }
    if (entry->alphaTrack != 0) {
        alpha = cfeTrackSample(entry->alphaTrack, time);
        actor = entry->actor;
        actor->alpha = alpha;
        if (alpha == 1.0f) {
            actor->drawFlags &= ~CFE_ACTOR_ALPHA;
        } else {
            actor->drawFlags |= CFE_ACTOR_ALPHA;
        }
        shown = entry->actor;
        if (shown->alpha <= 0.0f) {
            shown->drawMode = 0;
            shown->drawFlags |= CFE_ACTOR_HIDDEN;
        } else {
            shown->drawMode = entry->drawMode;
            shown->drawFlags &= ~CFE_ACTOR_HIDDEN;
        }
    }
    if (entry->attachSlotTrack != 0) {
        entry->attachSlot = func_002508C8(entry->attachSlotTrack, time);
    }
    if (entry->attachJointTrack != 0) {
        entry->attachJoint = func_002508C8(entry->attachJointTrack, time);
    }
    if (entry->attachSlot >= 0) {
        if (entry->actor->parent != 0) {
            if (XenoActorReleaseChild(entry->actor->parent, entry->actor) != 1) {
                assertFailed(D_0068D140, D_0068D180, 393);
            }
        }
        if (XenoActorAppendChildOnJoint(cfePlugInSlots[entry->attachSlot].object, entry->actor,
                                        entry->attachJoint) != 1) {
            assertFailed(D_0068D198, D_0068D180, 396);
        }
    } else if (entry->actor->parent != 0) {
        if (XenoActorReleaseChild(entry->actor->parent, entry->actor) != 1) {
            assertFailed(D_0068D140, D_0068D180, 402);
        }
    }
    if (entry->dampOffsetTrack != 0) {
        entry->dampOffset = func_002508C8(entry->dampOffsetTrack, time);
    }
    if (entry->firstWordTrack != 0) {
        entry->firstWord = func_002508C8(entry->firstWordTrack, time);
        XenoActorSetFirstWord(entry->actor, entry->firstWord);
    }
    /* CfeFlagOwner is this file's view of the actor (its flag words and record list). */
    cfeRecordListSample(time, (CfeFlagOwner *) entry->actor, entry->records);
    if (entry->shownTrack != 0) {
        if (func_002508C8(entry->shownTrack, time) != 0.0f) {
            entry->flags |= CFE_NODE_SHOWN;
        } else {
            entry->flags &= ~CFE_NODE_SHOWN;
        }
    }
    if (entry->positionTrack[0] != 0) {
        entry->position.x = cfeTrackSample(entry->positionTrack[0], time);
    }
    if (entry->positionTrack[1] != 0) {
        entry->position.y = cfeTrackSample(entry->positionTrack[1], time);
    }
    if (entry->positionTrack[2] != 0) {
        entry->position.z = cfeTrackSample(entry->positionTrack[2], time);
    }
}


extern int cfeEntryCount;
extern unsigned char cfeEntries[];

/* Evaluates every node of cfeEntries (cfeEntryCount nodes of 0x1B0 bytes) with cfeEvaluateNode. */
int cfeEvaluateNodes(int sample_param)
{
    unsigned char *entry;
    int result;
    int i;

    result = cfeEntryCount;
    if (result != 0) {
        i = 0;
        if (result > 0) {
            entry = cfeEntries;
            do {
                i++;
                cfeEvaluateNode(entry, sample_param);
                entry += 0x1B0;
                result = i < cfeEntryCount;
            } while (result != 0);
        }
    }
    return result;
}

void xglCurSetTranslate(const Vector4 *vector);
void xglCurMulScaleXYZ(const Vector4 *scale);

/* Builds the record's matrix into dest_matrix: translation from evaluated[0], rotations from evaluated[1], scale from evaluated[2]. */
void cfeRecordBuildMatrix(CfeEvalRecord *record, f32 dest_matrix[4][4]) {
    xglCurSetTranslate(&record->evaluated[0].vector);
    xglCurMulRotateZ(record->evaluated[1].vector.z);
    xglCurMulRotateY(record->evaluated[1].vector.y);
    xglCurMulRotateX(record->evaluated[1].vector.x);
    xglCurMulScaleXYZ(&record->evaluated[2].vector);
    xglCurMatrixGet(dest_matrix);
}


/* Samples the nine sources of a record into its three vectors, then builds its matrix (cfeRecordBuildMatrix). */
void cfeRecordEvaluate(CfeEvalRecord *record, f32 dest_matrix[4][4], int sample_param) {
    if (record->source[0] != 0) {
        record->evaluated[0].vector.x = cfeTrackSample(record->source[0], sample_param);
    }
    if (record->source[1] != 0) {
        record->evaluated[0].vector.y = cfeTrackSample(record->source[1], sample_param);
    }
    if (record->source[2] != 0) {
        record->evaluated[0].vector.z = cfeTrackSample(record->source[2], sample_param);
    }
    if (record->source[3] != 0) {
        record->evaluated[1].vector.x = cfeTrackSample(record->source[3], sample_param);
    }
    if (record->source[4] != 0) {
        record->evaluated[1].vector.y = cfeTrackSample(record->source[4], sample_param);
    }
    if (record->source[5] != 0) {
        record->evaluated[1].vector.z = cfeTrackSample(record->source[5], sample_param);
    }
    if (record->source[6] != 0) {
        record->evaluated[2].vector.x = cfeTrackSample(record->source[6], sample_param);
    }
    if (record->source[7] != 0) {
        record->evaluated[2].vector.y = cfeTrackSample(record->source[7], sample_param);
    }
    if (record->source[8] != 0) {
        record->evaluated[2].vector.z = cfeTrackSample(record->source[8], sample_param);
    }
    cfeRecordBuildMatrix(record, dest_matrix);
}

INCLUDE_ASM("asm/main/nonmatchings/cfeEvaluate", cfeGroupTableEvaluate);

void *memset(void *destination, int value, unsigned int size);

/* The shake offsets of the camera: position x, y, z, then rotation x, y, z. */
typedef struct CfeCameraOffset {
    f32 position[3];
    f32 rotation[3];
} CfeCameraOffset;

/* The studio block D_006384C0 (0x70 bytes) as the camera evaluation sees it: the camera, its key tracks
 * and the camera shake. cfePlugIn.c sees the same block as the
 * camera and 27 key lists (CfeStudioBlock). */
typedef struct CfeCameraEval {
    XglCamera *camera;          /* +0x00 */
    CfeTrackNode *position[3];  /* +0x04: tracks of travel.position */
    CfeTrackNode *rotation[3];  /* +0x10: tracks of travel.rotation */
    CfeTrackNode *fov;          /* +0x1C: track of travel.fov */
    CfeTrackNode *shakeTrack[3]; /* +0x20: tracks of shakeOn, shakeFrames and shakeAmount */
    int flags;                  /* +0x2C: CFE_CAMERA_HELD leaves the camera alone */
    int shakeOn;                /* +0x30 */
    int shakeFrames;            /* +0x34: frames from one shake offset to the next */
    int shakeAmount;            /* +0x38: the largest offset in 1/1000 units (rotation 1/10000) */
    int shakeCounter;           /* +0x3C: frames left to the next offset; -1 starts a new one */
    CfeCameraOffset from;       /* +0x40 */
    CfeCameraOffset to;         /* +0x58 */
} CfeCameraEval;

/* The original reads and writes the block through a union member, so its accesses carry alias set 0 (the
 * camera pointer and the counter are read again after every camera store). */
typedef union CfeStudioView {
    CfeCameraEval eval;
} CfeStudioView;

extern CfeStudioView D_006384C0;
/* -1.0f and 1.0f: the random sign of a shake offset. */
extern f32 D_006318B0[2];

/* The flag of CfeCameraEval.flags that stops the evaluation. */
#define CFE_CAMERA_HELD 0x1
/* The units of the shake amount for the position and the rotation. */
#define CFE_SHAKE_POSITION_UNIT 0.001f
#define CFE_SHAKE_ROTATION_UNIT 0.0001f

/* Evaluates the studio camera at `time`: its position, rotation and field of view from their tracks, and
 * the camera shake: while it is on, the camera moves from one random offset to the next over shakeFrames
 * frames (the counter only runs when `paused` is 0); off, the offsets are cleared. Then moves the studio. */
void func_00248390(int time, int paused)
{
    float blend;
    float rest;

    xglStudioGetCamera(&D_006384C0.eval.camera, 0);
    if (D_006384C0.eval.flags & CFE_CAMERA_HELD) {
        return;
    }
    if (D_006384C0.eval.position[0] != 0) {
        D_006384C0.eval.camera->travel.position.x = cfeTrackSample(D_006384C0.eval.position[0], time);
    }
    if (D_006384C0.eval.position[1] != 0) {
        D_006384C0.eval.camera->travel.position.y = cfeTrackSample(D_006384C0.eval.position[1], time);
    }
    if (D_006384C0.eval.position[2] != 0) {
        D_006384C0.eval.camera->travel.position.z = cfeTrackSample(D_006384C0.eval.position[2], time);
    }
    if (D_006384C0.eval.rotation[0] != 0) {
        D_006384C0.eval.camera->travel.rotation.x = cfeTrackSample(D_006384C0.eval.rotation[0], time);
    }
    if (D_006384C0.eval.rotation[1] != 0) {
        D_006384C0.eval.camera->travel.rotation.y = cfeTrackSample(D_006384C0.eval.rotation[1], time);
    }
    if (D_006384C0.eval.rotation[2] != 0) {
        D_006384C0.eval.camera->travel.rotation.z = cfeTrackSample(D_006384C0.eval.rotation[2], time);
    }
    if (D_006384C0.eval.fov != 0) {
        D_006384C0.eval.camera->travel.fov = cfeTrackSample(D_006384C0.eval.fov, time);
    }
    if (D_006384C0.eval.shakeTrack[0] != 0) {
        D_006384C0.eval.shakeOn = func_002508C8(D_006384C0.eval.shakeTrack[0], time);
    }
    if (D_006384C0.eval.shakeTrack[1] != 0) {
        D_006384C0.eval.shakeFrames = func_002508C8(D_006384C0.eval.shakeTrack[1], time);
    }
    if (D_006384C0.eval.shakeTrack[2] != 0) {
        D_006384C0.eval.shakeAmount = func_002508C8(D_006384C0.eval.shakeTrack[2], time);
    }
    if (D_006384C0.eval.shakeOn != 0 && D_006384C0.eval.shakeAmount != 0) {
        if (D_006384C0.eval.shakeCounter < 0 && paused == 0) {
            D_006384C0.eval.shakeCounter = D_006384C0.eval.shakeFrames;
            D_006384C0.eval.from = D_006384C0.eval.to;
            D_006384C0.eval.to.position[0] = xglSRand() % (D_006384C0.eval.shakeAmount + 1) * CFE_SHAKE_POSITION_UNIT
                                             * D_006318B0[xglSRand() & 1];
            D_006384C0.eval.to.position[1] = xglSRand() % (D_006384C0.eval.shakeAmount + 1) * CFE_SHAKE_POSITION_UNIT
                                             * D_006318B0[xglSRand() & 1];
            D_006384C0.eval.to.position[2] = xglSRand() % (D_006384C0.eval.shakeAmount + 1) * CFE_SHAKE_POSITION_UNIT
                                             * D_006318B0[xglSRand() & 1];
            D_006384C0.eval.to.rotation[0] = xglSRand() % (D_006384C0.eval.shakeAmount + 1) * CFE_SHAKE_ROTATION_UNIT
                                             * D_006318B0[xglSRand() & 1];
            D_006384C0.eval.to.rotation[1] = xglSRand() % (D_006384C0.eval.shakeAmount + 1) * CFE_SHAKE_ROTATION_UNIT
                                             * D_006318B0[xglSRand() & 1];
            D_006384C0.eval.to.rotation[2] = xglSRand() % (D_006384C0.eval.shakeAmount + 1) * CFE_SHAKE_ROTATION_UNIT
                                             * D_006318B0[xglSRand() & 1];
        }
        blend = (float) D_006384C0.eval.shakeCounter / (float) (D_006384C0.eval.shakeFrames + 1);
        if (1.0f < blend) {
            blend = 1.0f;
        }
        rest = 1.0f - blend;
        D_006384C0.eval.camera->travel.position.x
            += D_006384C0.eval.from.position[0] * blend + D_006384C0.eval.to.position[0] * rest;
        D_006384C0.eval.camera->travel.position.y
            += D_006384C0.eval.from.position[1] * blend + D_006384C0.eval.to.position[1] * rest;
        D_006384C0.eval.camera->travel.position.z
            += D_006384C0.eval.from.position[2] * blend + D_006384C0.eval.to.position[2] * rest;
        D_006384C0.eval.camera->travel.rotation.x
            += D_006384C0.eval.from.rotation[0] * blend + D_006384C0.eval.to.rotation[0] * rest;
        D_006384C0.eval.camera->travel.rotation.y
            += D_006384C0.eval.from.rotation[1] * blend + D_006384C0.eval.to.rotation[1] * rest;
        D_006384C0.eval.camera->travel.rotation.z
            += D_006384C0.eval.from.rotation[2] * blend + D_006384C0.eval.to.rotation[2] * rest;
        if (paused == 0) {
            D_006384C0.eval.shakeCounter--;
        }
    } else {
        D_006384C0.eval.shakeCounter = 0;
        memset(&D_006384C0.eval.from, 0, sizeof(D_006384C0.eval.from));
        memset(&D_006384C0.eval.to, 0, sizeof(D_006384C0.eval.to));
    }
    xglStudioMove();
}

/* The scale of a spawned effect; the original writes it through a union member (alias set 0). */
typedef union CfeEffectScale {
    float element[3];
} CfeEffectScale;

/* The spawned effect record, as far as func_002488F8 writes it. */
typedef struct CfeSpawnedEffect {
    unsigned char unmodeled_00[0x20];
    CfeEffectScale scale;       /* +0x20 */
    unsigned char unmodeled_2c[0x44 - 0x2C];
    int flags;                  /* +0x44 */
} CfeSpawnedEffect;

void sefClearOwnerFlags(unsigned int id, int flags);
CfeSpawnedEffect *sefSpawnEffectRecordById(unsigned int id, Vector4 *position, Vector4 *rotation);
void sefExecEffectSteps(CfeSpawnedEffect *effect, float frames, float steps);
void sefSetRecordOwner(CfeSpawnedEffect *effect, void *owner);

/* Spawns the command's effect (clearing its owner flags first), sets its scale, runs its first
 * frames (one step per 15 frames) and, when the command names an owner entry, hands the effect
 * to that entry's object. */
void func_002488F8(CfeEvalCommand *command)
{
    CfeSpawnedEffect *effect;
    unsigned char *entry;

    if (command->flag != 0) {
        sefClearOwnerFlags((command->page_index << CFE_PAGE_SHIFT) | command->page_offset | CFE_BANK_FLAGGED,
                           CFE_OWNER_FLAGS);
        effect = sefSpawnEffectRecordById((command->page_index << CFE_PAGE_SHIFT) | command->page_offset
                                          | CFE_BANK_FLAGGED, &command->position, &command->rotation);
    } else {
        sefClearOwnerFlags((command->page_index << CFE_PAGE_SHIFT) | command->page_offset | CFE_BANK_PLAIN,
                           CFE_OWNER_FLAGS);
        effect = sefSpawnEffectRecordById((command->page_index << CFE_PAGE_SHIFT) | command->page_offset
                                          | CFE_BANK_PLAIN, &command->position, &command->rotation);
    }
    if (effect != 0) {
        effect->scale.element[0] = command->scale[0];
        effect->scale.element[1] = command->scale[1];
        effect->scale.element[2] = command->scale[2];
        sefExecEffectSteps(effect, command->frames, command->frames / 15 + 1);
        effect->flags |= 0x10;
    }
    if (command->owner >= 0 && effect != 0) {
        entry = cfeGetEntry(command->owner);
        if (entry != 0) {
            sefSetRecordOwner(effect, *(void **) (entry + 0xC0));
        }
    }
}

void sefSetOwnerFlags(int, int);

/* Builds the command word from page_index and page_offset; the flag selects bank 0x20000000 or 0x10000000. */
void cfeCommandSetOwnerFlags(CfeEvalCommand *command) {
    if (command->flag != 0) {
        sefSetOwnerFlags((command->page_index << CFE_PAGE_SHIFT) | command->page_offset | CFE_BANK_FLAGGED, CFE_OWNER_FLAGS);
        return;
    }
    sefSetOwnerFlags((command->page_index << CFE_PAGE_SHIFT) | command->page_offset | CFE_BANK_PLAIN, CFE_OWNER_FLAGS);
}


void cfeCommandReleaseRecords(CfeEvalCommand *command) {
    if (command->flag != 0) {
        sefReleaseOwnedRecords((command->page_index << CFE_PAGE_SHIFT) | command->page_offset | CFE_BANK_FLAGGED);
        return;
    }
    sefReleaseOwnedRecords((command->page_index << CFE_PAGE_SHIFT) | command->page_offset | CFE_BANK_PLAIN);
}

/* The actions of a command (CfeEvalCommand.action). */
#define CFE_ACTION_NONE 0
#define CFE_ACTION_SPAWN 1
#define CFE_ACTION_SET_OWNER_FLAGS 2
#define CFE_ACTION_RELEASE 3
#define CFE_ACTION_RESPAWN 4

/* Releases the command's records and spawns its effect again. */
static inline void cfeCommandRespawn(CfeEvalCommand *command)
{
    cfeCommandReleaseRecords(command);
    func_002488F8(command);
}

/* Samples the command's tracks at `time` (position, rotation, scale, frames, owner, action) and runs
 * its action: 1 spawns the effect, 2 sets the owner flags, 3 releases the owned records, 4 releases
 * them and spawns the effect again. */
void cfeCommandEvaluate(CfeEvalCommand *command, int time)
{
    if (command->positionTracks[0] != 0) {
        command->position.x = cfeTrackSample(command->positionTracks[0], time);
    }
    if (command->positionTracks[1] != 0) {
        command->position.y = cfeTrackSample(command->positionTracks[1], time);
    }
    if (command->positionTracks[2] != 0) {
        command->position.z = cfeTrackSample(command->positionTracks[2], time);
    }
    if (command->rotationTracks[0] != 0) {
        command->rotation.x = cfeTrackSample(command->rotationTracks[0], time);
    }
    if (command->rotationTracks[1] != 0) {
        command->rotation.y = cfeTrackSample(command->rotationTracks[1], time);
    }
    if (command->rotationTracks[2] != 0) {
        command->rotation.z = cfeTrackSample(command->rotationTracks[2], time);
    }
    if (command->scaleTracks[0] != 0) {
        command->scale[0] = cfeTrackSample(command->scaleTracks[0], time);
    }
    if (command->scaleTracks[1] != 0) {
        command->scale[1] = cfeTrackSample(command->scaleTracks[1], time);
    }
    if (command->scaleTracks[2] != 0) {
        command->scale[2] = cfeTrackSample(command->scaleTracks[2], time);
    }
    if (command->framesTrack != 0) {
        command->frames = (int) func_002508C8(command->framesTrack, time);
    }
    if (command->ownerTrack != 0) {
        command->owner = (int) func_002508C8(command->ownerTrack, time);
    }
    if (command->actionTrack != 0) {
        command->action = (int) func_002508C8(command->actionTrack, time);
        switch (command->action) {
        case CFE_ACTION_NONE:
            break;
        case CFE_ACTION_SPAWN:
            func_002488F8(command);
            break;
        case CFE_ACTION_SET_OWNER_FLAGS:
            cfeCommandSetOwnerFlags(command);
            break;
        case CFE_ACTION_RELEASE:
            cfeCommandReleaseRecords(command);
            break;
        case CFE_ACTION_RESPAWN:
            cfeCommandRespawn(command);
            break;
        }
    }
}

extern int cfeCommandCount;
extern CfeEvalCommand cfeCommands[];

/* Evaluates every command of cfeCommands at `time`. */
void cfeCommandTableEvaluate(int time)
{
    int i;

    if (cfeCommandCount != 0) {
        for (i = 0; i < cfeCommandCount; i++) {
            cfeCommandEvaluate(&cfeCommands[i], time);
        }
    }
}

/* A row of cfeDirections (0x40 bytes): the angles of a light direction, the vectors it drives and
 * the tracks they are sampled from. */
typedef struct CfeDirectionRow {
    float angles[3];            /* +0x00: X and Y rotation of the direction */
    unsigned char unmodeled_0c[4];
    Vector4 *direction;         /* +0x10: rotated from angles */
    Vector4 *color;             /* +0x14 */
    Vector4 *extra;             /* +0x18 */
    CfeTrackNode *extraTracks[3]; /* +0x1C */
    CfeTrackNode *angleTracks[3]; /* +0x28 */
    CfeTrackNode *colorTracks[3]; /* +0x34 */
} CfeDirectionRow;

extern CfeDirectionRow cfeDirections[4];

/* Samples the tracks of the four light rows at `time` and rotates each row's direction. */
void cfeEvaluateDirections(int time)
{
    CfeDirectionRow *row;
    int i;

    for (i = 0; i < 4; i++) {
        row = &cfeDirections[i];
        if (row->extraTracks[0] != 0) {
            row->extra->x = cfeTrackSample(row->extraTracks[0], time);
        }
        if (row->extraTracks[1] != 0) {
            row->extra->y = cfeTrackSample(row->extraTracks[1], time);
        }
        if (row->extraTracks[2] != 0) {
            row->extra->z = cfeTrackSample(row->extraTracks[2], time);
        }
        if (row->colorTracks[0] != 0) {
            row->color->x = cfeTrackSample(row->colorTracks[0], time);
        }
        if (row->colorTracks[1] != 0) {
            row->color->y = cfeTrackSample(row->colorTracks[1], time);
        }
        if (row->colorTracks[2] != 0) {
            row->color->z = cfeTrackSample(row->colorTracks[2], time);
        }
        if (row->angleTracks[0] != 0) {
            row->angles[0] = cfeTrackSample(row->angleTracks[0], time);
        }
        if (row->angleTracks[1] != 0) {
            row->angles[1] = cfeTrackSample(row->angleTracks[1], time);
        }
        if (row->angleTracks[2] != 0) {
            row->angles[2] = cfeTrackSample(row->angleTracks[2], time);
        }
        if (row->direction != 0) {
            cfeRotateVectorXY(row->direction, row->angles);
        }
    }
}

extern CfeEvalGlobal cfeEvalGlobal;

/* Samples the eight sources of the global block cfeEvalGlobal. */
void cfeGlobalEvaluate(int sample_param)
{
    if (cfeEvalGlobal.source[0] != 0) {
        cfeEvalGlobal.evaluated[0].x = cfeTrackSample(cfeEvalGlobal.source[0], sample_param);
    }
    if (cfeEvalGlobal.source[1] != 0) {
        cfeEvalGlobal.evaluated[0].y = cfeTrackSample(cfeEvalGlobal.source[1], sample_param);
    }
    if (cfeEvalGlobal.source[2] != 0) {
        cfeEvalGlobal.evaluated[0].z = cfeTrackSample(cfeEvalGlobal.source[2], sample_param);
    }
    if (cfeEvalGlobal.source[3] != 0) {
        cfeEvalGlobal.evaluated[1].x = cfeTrackSample(cfeEvalGlobal.source[3], sample_param);
    }
    if (cfeEvalGlobal.source[4] != 0) {
        cfeEvalGlobal.evaluated[1].y = cfeTrackSample(cfeEvalGlobal.source[4], sample_param);
    }
    if (cfeEvalGlobal.source[5] != 0) {
        cfeEvalGlobal.evaluated[1].z = cfeTrackSample(cfeEvalGlobal.source[5], sample_param);
    }
    if (cfeEvalGlobal.source[6] != 0) {
        cfeEvalGlobal.evaluated[1].w = cfeTrackSample(cfeEvalGlobal.source[6], sample_param);
    }
    if (cfeEvalGlobal.source[7] != 0) {
        cfeEvalGlobal.result_int = (int) func_002508C8(cfeEvalGlobal.source[7], sample_param);
    }
}

/* Samples the ten sources of one entry of cfeEvalEntries. */
void cfeEntryEvaluate(CfeEvalEntry *entry, int sample_param) {
    if (entry->source[0] != 0) {
        entry->evaluated[0].vector.x = cfeTrackSample(entry->source[0], sample_param);
    }
    if (entry->source[1] != 0) {
        entry->evaluated[0].vector.y = cfeTrackSample(entry->source[1], sample_param);
    }
    if (entry->source[2] != 0) {
        entry->evaluated[0].vector.z = cfeTrackSample(entry->source[2], sample_param);
    }
    if (entry->source[3] != 0) {
        entry->evaluated[1].vector.x = cfeTrackSample(entry->source[3], sample_param);
    }
    if (entry->source[4] != 0) {
        entry->evaluated[1].vector.y = cfeTrackSample(entry->source[4], sample_param);
    }
    if (entry->source[5] != 0) {
        entry->evaluated[1].vector.z = cfeTrackSample(entry->source[5], sample_param);
    }
    if (entry->source[6] != 0) {
        entry->evaluated[2].vector.x = cfeTrackSample(entry->source[6], sample_param);
    }
    if (entry->source[7] != 0) {
        entry->evaluated[2].vector.y = cfeTrackSample(entry->source[7], sample_param);
    }
    if (entry->source[8] != 0) {
        entry->evaluated[2].vector.z = cfeTrackSample(entry->source[8], sample_param);
    }
    if (entry->source[9] != 0) {
        entry->result_int = (int) cfeTrackSample(entry->source[9], sample_param);
    }
}

extern int cfeEvalEntryCount;
extern CfeEvalEntry cfeEvalEntries[];

/* Evaluates the first cfeEvalEntryCount entries of cfeEvalEntries. */
int cfeEntryTableEvaluate(int sample_param)
{
    CfeEvalEntry *entry;
    CfeEvalEntry *next;
    int result;
    int i;

    result = cfeEvalEntryCount;
    if (result != 0) {
        i = 0;
        if (result > 0) {
            next = &cfeEvalEntries[0];
            do {
                i++;
                entry = next;
                next++;
                cfeEntryEvaluate(entry, sample_param);
                result = i < cfeEvalEntryCount;
            } while (result != 0);
        }
    }
    return result;
}

/* Samples the three sources of one slot of cfeEvalSlots. */
void cfeSlotEvaluate(CfeEvalSlot *slot, int sample_param) {
    if (slot->source_for_result_int != 0) {
        slot->result_int = (int) func_002508C8(slot->source_for_result_int, sample_param);
    }
    if (slot->source_for_sample_float != 0) {
        slot->sample_float = cfeTrackSample(slot->source_for_sample_float, sample_param);
    }
    if (slot->source_for_sample_int != 0) {
        slot->sample_int = (int) cfeTrackSample(slot->source_for_sample_int, sample_param);
    }
}

extern CfeEvalSlot cfeEvalSlots[3];

/* Evaluates the three slots of cfeEvalSlots. */
void cfeSlotTableEvaluate(int sample_param)
{
    int i;

    for (i = 0; i < 3; i++)
    {
        cfeSlotEvaluate(&cfeEvalSlots[i], sample_param);
    }
}

/* Updates one sound record at `time`: samples its duration and value tracks and, when it has a
 * duration, sets the volume to the new value over |old - new| / duration steps (at least 1). */
void func_00249248(CfeSndRecord *record, int time)
{
    int old;
    int value;
    int steps;

    if (record->durationTrack != 0) {
        record->duration = (int) func_002508C8(record->durationTrack, time);
    }
    if (record->valueTrack != 0) {
        old = record->value;
        value = (int) func_002508C8(record->valueTrack, time);
        record->value = value;
        if (record->duration != 0) {
            steps = (int) (__builtin_fabsf((float) (old - value)) / (float) record->duration);
            if (steps == 0) {
                steps = 1;
            }
            seVolumeSet(value, steps);
        }
    }
}

/* Sets the target of record 0. The argument's address is taken: the original
   keeps a frame for the call and does not tail-call func_00249248. */
void cfeSndSetTarget(int target)
{
  int *target_ref;
  target_ref = &target;
  func_00249248(cfeSndRecords, *target_ref);
}

extern unsigned short sRenderDepthBase;

/* RGBAQ q of 1.0 and the screen origin of the 12.4 vertex coordinates. */
#define CFE_GS_Q_ONE 0x3F800000
#define CFE_GS_ORIGIN_X 0x7000
#define CFE_GS_ORIGIN_Y 0x71C0

/* GS primitive: line. */
#define CFE_GS_PRIM_LINE 1

/* An RGBA colour word (red in the low byte) as an RGBAQ value with q = 1. */
#define CFE_GS_RGBAQ(color) \
    GS_SET_RGBAQ((color) & 0xFF, ((color) >> 8) & 0xFF, ((color) >> 16) & 0xFF, ((color) >> 24) & 0xFF, CFE_GS_Q_ONE)

/* Queues a Gouraud-shaded line from (x0, y0) in colour0 to (x1, y1) in colour1 at `depth` (pixels,
 * depth test GEQUAL), submitted deferred. */
void func_00249340(int x0, int y0, int x1, int y1, unsigned int color0, unsigned int color1, unsigned int depth)
{
    u64 *p;
    u64 *start;

    xglDirectQueueBegin();
    p = (u64 *) &xglDirectQueueBuffer[xglDirectQueueCount];
    start = p;
    GS_PUT(p, GS_TEST_1, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 2));
    GS_PUT(p, GS_ZBUF_1, GS_SET_ZBUF(sRenderDepthBase, 1, 0));
    GS_PUT(p, GS_PRIM, GS_SET_PRIM(CFE_GS_PRIM_LINE, 1, 0, 0, 0, 0, 0, 0, 0));
    GS_PUT(p, GS_RGBAQ, CFE_GS_RGBAQ(color0));
    GS_PUT(p, GS_XYZF2, GS_SET_XYZ((x0 << 4) + CFE_GS_ORIGIN_X, (y0 << 4) + CFE_GS_ORIGIN_Y, depth));
    GS_PUT(p, GS_RGBAQ, CFE_GS_RGBAQ(color1));
    GS_PUT(p, GS_XYZF2, GS_SET_XYZ((x1 << 4) + CFE_GS_ORIGIN_X, (y1 << 4) + CFE_GS_ORIGIN_Y, depth));
    xglDirectQueueCount += ((u32) p - (u32) start) >> 4;
    xglDirectQueueDefer(0, 10);
}

void xglVectorNormal_001A9200(Vector4 *destination, const Vector4 *source);
void func_00297580(Vector4 *quaternion, const Vector4 *from, const Vector4 *to);
void XrgQuaternionToMatrix(Matrix4 matrix, const Vector4 *quaternion);
void xglMatrixStackUnit(void);
void xglMatrixStackRotX(float angle);
void xglMatrixStackRotY(float angle);
void xglMatrixStackMul(Matrix4 matrix);
void xglMatrixStackSave(Matrix4 matrix);
void xglVectorInner(float *result, const Vector4 *left, const Vector4 *right);

/* A 0..1 float channel scaled to the 0..128 GS range. */
#define COLOR_BYTE(f) ((unsigned int) ((f) * 128.0f) & 0xFF)
/* Screen position of a camera-space point around (x + 60, y + 40): a
   perspective with the eye 300 units behind the plane and a 450-unit scale. */
#define SCREEN_X(coord, depth) (x + 60 + (coord) * (distance + (depth)) / screen)
#define SCREEN_Y(coord, depth) (y + 40 + (coord) * (distance + (depth)) / screen)
/* One 8-bit channel of a packed RGBA colour. */
#define CHANNEL(c, shift) (((c) >> (shift)) & 0xFF)

/* Draws the orientation grid of a rotation: like cfeDrawDirectionGrid, with the grid turned by the angles
 * (x, then y) instead of towards a direction vector. The direction of the angles is computed but not used. */
void func_00249518(int x, int y, Vector4 *angles, float *color0, float *color1)
{
    Matrix4 view;
    static const AlignedVector4 upAxis = { 0.0f, 1.0f, 0.0f, 1.0f };
    static const AlignedVector4 frontAxis = { 0.0f, 0.0f, 1.0f, 1.0f };
    AlignedVector4 up = upAxis;
    AlignedVector4 front = frontAxis;
    AlignedVector4 direction;
    float facing;
    XglCamera *camera;
    unsigned int near_color;
    unsigned int far_color;
    unsigned int mixed_color;
    float near_weight;
    float grid_pos;
    float distance;
    float screen;

    near_weight = 0.0f;
    facing = 0.0f;
    camera = xglStudioSelectGetActiveCamera(0);
    distance = 300.0f;
    screen = 450.0f;
    near_color = 0;
    if (color0 != 0) {
        near_color = COLOR_BYTE(color0[0]) + (COLOR_BYTE(color0[1]) << 8) + (COLOR_BYTE(color0[2]) << 16) + 0x80000000;
    }
    if (color1 != 0) {
        far_color = COLOR_BYTE(color1[0]) + (COLOR_BYTE(color1[1]) << 8) + (COLOR_BYTE(color1[2]) << 16) + 0x80000000;
    } else {
        far_color = near_color;
    }
    {
        AlignedVector4 line[4];
        AlignedVector4 tip;

        xglDirectionFromAngles(&direction, angles);
        xglMatrixStackUnit();
        xglMatrixStackRotX(camera->travel.rotation.x);
        xglMatrixStackRotY(-camera->travel.rotation.y);
        xglMatrixStackRotX(-angles->x);
        xglMatrixStackRotY(angles->y);
        xglMatrixStackSave(view);
        xglVectorMulMat(&up, view, &up);
        xglVectorInner(&facing, &up, &front);
        for (grid_pos = -60.0f; grid_pos <= 60.0f; grid_pos += 20.0f) {
            line[0].x = grid_pos;
            line[0].y = 0.0f;
            line[0].z = 60.0f;
            line[0].w = 1.0f;
            line[1].x = grid_pos;
            line[1].y = 0.0f;
            line[1].z = -60.0f;
            line[1].w = 1.0f;
            line[2].x = 60.0f;
            line[2].y = 0.0f;
            line[2].z = grid_pos;
            line[2].w = 1.0f;
            line[3].x = -60.0f;
            line[3].y = 0.0f;
            line[3].z = grid_pos;
            line[3].w = 1.0f;
            xglVectorMulMat(&line[0], view, &line[0]);
            xglVectorMulMat(&line[1], view, &line[1]);
            xglVectorMulMat(&line[2], view, &line[2]);
            xglVectorMulMat(&line[3], view, &line[3]);
            func_00249340(SCREEN_X(line[0].x, line[0].z), SCREEN_Y(line[0].y, line[0].z),
                          SCREEN_X(line[1].x, line[1].z), SCREEN_Y(line[1].y, line[1].z),
                          near_color, far_color, 0xFFFFFF);
            mixed_color = 0;
            mixed_color += (unsigned char) (near_weight * CHANNEL(near_color, 0)
                                            + (1.0f - near_weight) * CHANNEL(far_color, 0));
            mixed_color += (unsigned char) (near_weight * CHANNEL(near_color, 8)
                                            + (1.0f - near_weight) * CHANNEL(far_color, 8)) << 8;
            mixed_color += (unsigned char) (near_weight * CHANNEL(near_color, 16)
                                            + (1.0f - near_weight) * CHANNEL(far_color, 16)) << 16;
            mixed_color += (unsigned char) (near_weight * CHANNEL(near_color, 24)
                                            + (1.0f - near_weight) * CHANNEL(far_color, 24)) << 24;
            func_00249340(SCREEN_X(line[2].x, line[2].z), SCREEN_Y(line[2].y, line[2].z),
                          SCREEN_X(line[3].x, line[3].z), SCREEN_Y(line[3].y, line[3].z),
                          mixed_color, mixed_color, 0xFFFFFF);
            near_weight += 0.1f;
        }
        tip.x = 0.0f;
        tip.y = 0.0f;
        tip.z = 70.0f;
        tip.w = 1.0f;
        xglVectorMulMat(&tip, view, &tip);
        func_00249340(SCREEN_X(0.0f, tip.z), SCREEN_Y(0.0f, tip.z),
                      SCREEN_X(tip.x, tip.z), SCREEN_Y(tip.y, tip.z),
                      0x80FFFFFF, 0x80FFFFFF, 0xFFFFFF);
    }
}


/* Draws the orientation grid of a direction vector: a 7x7 grid of lines in
   front of the active camera, shaded from color0 to color1, and a 70-unit
   pointer, projected around the screen point (x + 60, y + 40). */
void cfeDrawDirectionGrid(int x, int y, Vector4 *direction, float *color0, float *color1)
{
    Matrix4 view;
    AlignedVector4 up = { 0.0f, 1.0f, 0.0f, 1.0f };
    AlignedVector4 front = { 0.0f, 0.0f, 1.0f, 1.0f };
    Matrix4 rotation;
    AlignedVector4 normal;
    float facing;
    XglCamera *camera;
    unsigned int near_color;
    unsigned int far_color;
    unsigned int mixed_color;
    float near_weight;
    float grid_pos;
    float distance;
    float screen;

    near_weight = 0.0f;
    facing = 0.0f;
    camera = xglStudioSelectGetActiveCamera(0);
    distance = 300.0f;
    screen = 450.0f;
    near_color = 0;
    if (color0 != 0) {
        near_color = COLOR_BYTE(color0[0]) + (COLOR_BYTE(color0[1]) << 8) + (COLOR_BYTE(color0[2]) << 16) + 0x80000000;
    }
    far_color = near_color;
    if (color1 != 0) {
        far_color = COLOR_BYTE(color1[0]) + (COLOR_BYTE(color1[1]) << 8) + (COLOR_BYTE(color1[2]) << 16) + 0x80000000;
    }
    {
        static const AlignedVector4 forward = { 0.0f, 0.0f, 1.0f, 1.0f };
        AlignedVector4 axis = forward;
        AlignedVector4 quaternion;
        AlignedVector4 line[4];
        AlignedVector4 tip;

        xglVectorNormal_001A9200(&normal, direction);
        normal.y = -normal.y;
        func_00297580(&quaternion, &axis, &normal);
        XrgQuaternionToMatrix(rotation, &quaternion);
        xglMatrixStackUnit();
        xglMatrixStackRotX(camera->travel.rotation.x);
        xglMatrixStackRotY(-camera->travel.rotation.y);
        xglMatrixStackMul(rotation);
        xglMatrixStackSave(view);
        xglVectorMulMat(&up, view, &up);
        xglVectorInner(&facing, &up, &front);
        for (grid_pos = -60.0f; grid_pos <= 60.0f; grid_pos += 20.0f) {
            line[0].x = grid_pos;
            line[0].y = 0.0f;
            line[0].z = 60.0f;
            line[0].w = 1.0f;
            line[1].x = grid_pos;
            line[1].y = 0.0f;
            line[1].z = -60.0f;
            line[1].w = 1.0f;
            line[2].x = 60.0f;
            line[2].y = 0.0f;
            line[2].z = grid_pos;
            line[2].w = 1.0f;
            line[3].x = -60.0f;
            line[3].y = 0.0f;
            line[3].z = grid_pos;
            line[3].w = 1.0f;
            xglVectorMulMat(&line[0], view, &line[0]);
            xglVectorMulMat(&line[1], view, &line[1]);
            xglVectorMulMat(&line[2], view, &line[2]);
            xglVectorMulMat(&line[3], view, &line[3]);
            func_00249340(SCREEN_X(line[0].x, line[0].z), SCREEN_Y(line[0].y, line[0].z),
                          SCREEN_X(line[1].x, line[1].z), SCREEN_Y(line[1].y, line[1].z),
                          near_color, far_color, 0xFFFFFF);
            mixed_color = 0;
            mixed_color += (unsigned char) (near_weight * CHANNEL(near_color, 0) + (1.0f - near_weight) * CHANNEL(far_color, 0));
            mixed_color += (unsigned char) (near_weight * CHANNEL(near_color, 8) + (1.0f - near_weight) * CHANNEL(far_color, 8)) << 8;
            mixed_color += (unsigned char) (near_weight * CHANNEL(near_color, 16) + (1.0f - near_weight) * CHANNEL(far_color, 16)) << 16;
            mixed_color += (unsigned char) (near_weight * CHANNEL(near_color, 24) + (1.0f - near_weight) * CHANNEL(far_color, 24)) << 24;
            func_00249340(SCREEN_X(line[2].x, line[2].z), SCREEN_Y(line[2].y, line[2].z),
                          SCREEN_X(line[3].x, line[3].z), SCREEN_Y(line[3].y, line[3].z),
                          mixed_color, mixed_color, 0xFFFFFF);
            near_weight += 0.1f;
        }
        tip.x = 0.0f;
        tip.y = 0.0f;
        tip.z = 70.0f;
        tip.w = 1.0f;
        xglVectorMulMat(&tip, view, &tip);
        func_00249340(SCREEN_X(0.0f, tip.z), SCREEN_Y(0.0f, tip.z),
                      SCREEN_X(tip.x, tip.z), SCREEN_Y(tip.y, tip.z),
                      0x80FFFFFF, 0x80FFFFFF, 0xFFFFFF);
    }
}

void _CurMatrixSet(Matrix4 matrix);
void _CurSetViewScaleTrans(Vector4 *scale, Vector4 *translation);
int _CurRotTransPersClipScreen(Vector4 *screen, const Vector4 *point);
void xglVectorScaleXYZ(Vector4 *destination, const Vector4 *source, float scale);

/* One scratchpad quadword: written as a vector, read back as raw GS words. */
typedef union CfeMovieWord {
    Vector4 vector;
    int word[4];
} CfeMovieWord;

/* The scratchpad work area of the movie quad: the screen points (x, y, z words and q), the corners in the world
 * and the texture coordinates (s, t, q). */
typedef struct CfeMovieScratch {
    CfeMovieWord screen[4];     /* +0x00 */
    CfeMovieWord world[4];      /* +0x40 */
    unsigned char unmodeled_80[0x20];
    CfeMovieWord stq[4];        /* +0xA0 */
} CfeMovieScratch;

#define CFE_MOVIE_SCRATCH ((CfeMovieScratch *) 0x70000000)

/* The image upload: VIF DIRECT (the quadword count is or-ed in), GIF tag EOP in word 0 and FLG IMAGE in word 1. */
#define CFE_VIF_DIRECT 0x51000000
#define CFE_GIF_TAG_EOP 0x8000
#define CFE_GIF_TAG_IMAGE 0x08000000

/* The texture coordinates stay this far inside the frame edges. */
#define CFE_MOVIE_TOP 0.01f
#define CFE_MOVIE_BOTTOM_INSET 0.005f

/* Draws the movie frame of the entry on its quad: the corners are moved by the entry's translation, rotation
 * and scale and projected (nothing is drawn if a corner is clipped), the frame is uploaded into the texture page
 * and drawn as a textured strip whose alpha is the z of the entry's scale. */
void func_0024A8C0(CfeEvalEntry *entry)
{
    u32 upload[8];
    XglCamera *camera = xglStudioSelectGetActiveCamera(0);
    void *frame;
    CfeMovieScratch *scratch = CFE_MOVIE_SCRATCH;

    if (entry->movie.bufferIndex) {
        frame = entry->movie.imageAddress[0];
    } else {
        frame = entry->movie.imageAddress[1];
    }
    xglCurSetTranslate(&entry->evaluated[0].vector);
    xglCurMulRotateZ(entry->evaluated[1].vector.z);
    xglCurMulRotateY(entry->evaluated[1].vector.y);
    xglCurMulRotateX(entry->evaluated[1].vector.x);
    xglCurMulScaleXYZ(&entry->evaluated[2].vector);
    xglCurApplyVector(&scratch->world[0].vector, &entry->corners[0]);
    xglCurApplyVector(&scratch->world[1].vector, &entry->corners[1]);
    xglCurApplyVector(&scratch->world[2].vector, &entry->corners[2]);
    xglCurApplyVector(&scratch->world[3].vector, &entry->corners[3]);
    scratch->stq[0].vector.x = 0.0f;
    scratch->stq[0].vector.y = CFE_MOVIE_TOP;
    scratch->stq[0].vector.z = 1.0f;
    scratch->stq[1].vector.x = (float) entry->movie.width / xglRoundUpPow2(entry->movie.width);
    scratch->stq[1].vector.y = CFE_MOVIE_TOP;
    scratch->stq[1].vector.z = 1.0f;
    scratch->stq[2].vector.x = 0.0f;
    scratch->stq[2].vector.y = (float) entry->movie.height / xglRoundUpPow2(entry->movie.height)
                               - CFE_MOVIE_BOTTOM_INSET;
    scratch->stq[2].vector.z = 1.0f;
    scratch->stq[3].vector.x = (float) entry->movie.width / xglRoundUpPow2(entry->movie.width);
    scratch->stq[3].vector.y = (float) entry->movie.height / xglRoundUpPow2(entry->movie.height)
                               - CFE_MOVIE_BOTTOM_INSET;
    scratch->stq[3].vector.z = 1.0f;
    _CurMatrixSet(camera->volume[0].viewMatrix);
    _CurSetViewScaleTrans(&camera->screen.screenScale, &camera->screen.screenOffset);
    if (_CurRotTransPersClipScreen(&scratch->screen[0].vector, &scratch->world[0].vector) != 0) {
        return;
    }
    if (_CurRotTransPersClipScreen(&scratch->screen[1].vector, &scratch->world[1].vector) != 0) {
        return;
    }
    if (_CurRotTransPersClipScreen(&scratch->screen[2].vector, &scratch->world[2].vector) != 0) {
        return;
    }
    if (_CurRotTransPersClipScreen(&scratch->screen[3].vector, &scratch->world[3].vector) != 0) {
        return;
    }
    xglVectorScaleXYZ(&scratch->stq[0].vector, &scratch->stq[0].vector,
                      scratch->screen[0].vector.w);
    xglVectorScaleXYZ(&scratch->stq[1].vector, &scratch->stq[1].vector,
                      scratch->screen[1].vector.w);
    xglVectorScaleXYZ(&scratch->stq[2].vector, &scratch->stq[2].vector,
                      scratch->screen[2].vector.w);
    xglVectorScaleXYZ(&scratch->stq[3].vector, &scratch->stq[3].vector,
                      scratch->screen[3].vector.w);
    upload[0] = 0;
    upload[1] = 0;
    upload[2] = 0;
    upload[3] = CFE_VIF_DIRECT | 1;
    upload[4] = CFE_GIF_TAG_EOP + entry->movie.width * entry->movie.height / 4;
    upload[5] = CFE_GIF_TAG_IMAGE;
    upload[6] = 0;
    upload[7] = 0;
    xglDirectQueueBegin();
    xglDirectQueuePush(GS_TEXFLUSH, 0);
    xglDirectQueuePush(GS_BITBLTBUF, GS_SET_BITBLTBUF(0, func_001D2E90(entry->movie.width, 0), 0,
                                                      sRender.frame_base_1 << 5,
                                                      func_001D2E90(entry->movie.width, 0), 0));
    xglDirectQueuePush(GS_TRXPOS, 0);
    xglDirectQueuePush(GS_TRXREG, GS_SET_TRXREG(entry->movie.width, entry->movie.height));
    xglDirectQueuePush(GS_TRXDIR, 0);
    xglDirectQueueDefer(0, 4);
    pxyAddEntryFlagged(4, upload, 2, 0);
    pxyAddEntryFull(5, frame, entry->movie.width * entry->movie.height / 4, 0,
                    CFE_VIF_DIRECT | entry->movie.width * entry->movie.height / 4);
    xglDirectQueueBegin();
    xglDirectQueuePush(GS_TEXFLUSH, 0);
    xglDirectQueuePush(GS_TEST_1, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 2));
    xglDirectQueuePush(GS_CLAMP_1, 0);
    xglDirectQueuePush(GS_ALPHA_1, GS_SET_ALPHA(0, 1, 0, 1, 0));
    xglDirectQueuePush(GS_TEX1_1, GS_SET_TEX1(0, 0, 1, 1, 0, 0, 0));
    xglDirectQueuePush(GS_TEX0_1, GS_SET_TEX0(sRender.frame_base_1 << 5, func_001D2E90(entry->movie.width, 0), 0,
                                              xglRoundUpLog2(entry->movie.width),
                                              xglRoundUpLog2(entry->movie.height), 1, 0, 0, 0, 0, 0, 1));
    xglDirectQueuePush(GS_PRIM, GS_SET_PRIM(4, 1, 1, 0, 1, 0, 0, 0, 0));
    xglDirectQueuePush(GS_RGBAQ, GS_SET_RGBAQ(0x80, 0x80, 0x80, (int) (entry->evaluated[2].vector.z * 128.0f),
                                              scratch->stq[0].word[2]));
    xglDirectQueuePush(GS_ST, scratch->stq[0].word[0] | (u64) scratch->stq[0].word[1] << 32);
    xglDirectQueuePush(GS_XYZF2, GS_SET_XYZ(scratch->screen[0].word[0], scratch->screen[0].word[1],
                                            scratch->screen[0].word[2]));
    xglDirectQueuePush(GS_RGBAQ, GS_SET_RGBAQ(0x80, 0x80, 0x80, (int) (entry->evaluated[2].vector.z * 128.0f),
                                              scratch->stq[1].word[2]));
    xglDirectQueuePush(GS_ST, scratch->stq[1].word[0] | (u64) scratch->stq[1].word[1] << 32);
    xglDirectQueuePush(GS_XYZF2, GS_SET_XYZ(scratch->screen[1].word[0], scratch->screen[1].word[1],
                                            scratch->screen[1].word[2]));
    xglDirectQueuePush(GS_RGBAQ, GS_SET_RGBAQ(0x80, 0x80, 0x80, (int) (entry->evaluated[2].vector.z * 128.0f),
                                              scratch->stq[2].word[2]));
    xglDirectQueuePush(GS_ST, scratch->stq[2].word[0] | (u64) scratch->stq[2].word[1] << 32);
    xglDirectQueuePush(GS_XYZF2, GS_SET_XYZ(scratch->screen[2].word[0], scratch->screen[2].word[1],
                                            scratch->screen[2].word[2]));
    xglDirectQueuePush(GS_RGBAQ, GS_SET_RGBAQ(0x80, 0x80, 0x80, (int) (entry->evaluated[2].vector.z * 128.0f),
                                              scratch->stq[3].word[2]));
    xglDirectQueuePush(GS_ST, scratch->stq[3].word[0] | (u64) scratch->stq[3].word[1] << 32);
    xglDirectQueuePush(GS_XYZF2, GS_SET_XYZ(scratch->screen[3].word[0], scratch->screen[3].word[1],
                                            scratch->screen[3].word[2]));
    xglDirectQueueDefer(0, 4);
}

/* Below this size an entry's scale counts as zero and the entry is skipped. */
#define CFE_ENTRY_MIN_SCALE 0.05f

/* Updates the entries: rewinds the movie of each entry with a result, then plays (unless
 * `noPlay`) and draws every entry whose scale is not zero. */
void cfeUpdateEntries(int noPlay)
{
    int i;

    if (cfeEvalEntryCount != 0) {
        for (i = 0; i < cfeEvalEntryCount; i++) {
            if (cfeEvalEntries[i].result_int != 0) {
                xglMovieRewind(&cfeEvalEntries[i].movie);
            }
        if (__builtin_fabsf(cfeEvalEntries[i].evaluated[2].vector.x) < CFE_ENTRY_MIN_SCALE
            || __builtin_fabsf(cfeEvalEntries[i].evaluated[2].vector.y) < CFE_ENTRY_MIN_SCALE
            || __builtin_fabsf(cfeEvalEntries[i].evaluated[2].vector.z) < CFE_ENTRY_MIN_SCALE) {
            continue;
        }
        if (noPlay == 0) {
            xglMoviePlay(&cfeEvalEntries[i].movie);
        }
        func_0024A8C0(&cfeEvalEntries[i]);
        }
    }
}
