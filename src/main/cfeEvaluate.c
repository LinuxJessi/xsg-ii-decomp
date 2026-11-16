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
#include "main/TiGlobalColl.h"


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
        value = (int) cfeTrackSampleStep(record->track[0], time);
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

int XenoActorReleaseChild(struct XenoActor *parent, struct XenoActor *child);
int XenoActorAppendChildOnJoint(struct XenoActor *parent, struct XenoActor *child, int joint);
/* TiMRPackage.c takes its own view of the state (TiMRState). */
void tiMRStateSetSpeed(XenoMotionState *state, float value);
extern const char cfeReleaseChildAssert[];
extern const char cfeEvaluateSourceFile[];
extern const char cfeAppendChildAssert[];


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
void cfeEvaluateNode(CfeActor *entry, int time)
{
    struct XenoActor *actor;
    struct XenoActor *shown;
    float alpha;
    Vector4 offset;
    int slot;
    int changed;

    if (entry->track[CFE_ACTOR_TRACK_POSITION + 0] != 0) {
        entry->actor->position.x = cfeTrackSample(entry->track[CFE_ACTOR_TRACK_POSITION + 0], time);
    }
    if (entry->track[CFE_ACTOR_TRACK_POSITION + 1] != 0) {
        entry->actor->position.y = cfeTrackSample(entry->track[CFE_ACTOR_TRACK_POSITION + 1], time);
    }
    if (entry->track[CFE_ACTOR_TRACK_POSITION + 2] != 0) {
        entry->actor->position.z = cfeTrackSample(entry->track[CFE_ACTOR_TRACK_POSITION + 2], time);
    }
    if (entry->track[CFE_ACTOR_TRACK_ROTATION + 0] != 0) {
        entry->actor->rotation.x = cfeTrackSample(entry->track[CFE_ACTOR_TRACK_ROTATION + 0], time);
    }
    if (entry->track[CFE_ACTOR_TRACK_ROTATION + 1] != 0) {
        entry->actor->rotation.y = cfeTrackSample(entry->track[CFE_ACTOR_TRACK_ROTATION + 1], time);
    }
    if (entry->track[CFE_ACTOR_TRACK_ROTATION + 2] != 0) {
        entry->actor->rotation.z = cfeTrackSample(entry->track[CFE_ACTOR_TRACK_ROTATION + 2], time);
    }
    if (entry->track[CFE_ACTOR_TRACK_SCALE + 0] != 0) {
        ((CfeActorScale *) entry->actor)->scale.x = cfeTrackSample(entry->track[CFE_ACTOR_TRACK_SCALE + 0], time);
    }
    if (entry->track[CFE_ACTOR_TRACK_SCALE + 1] != 0) {
        ((CfeActorScale *) entry->actor)->scale.y = cfeTrackSample(entry->track[CFE_ACTOR_TRACK_SCALE + 1], time);
    }
    if (entry->track[CFE_ACTOR_TRACK_SCALE + 2] != 0) {
        ((CfeActorScale *) entry->actor)->scale.z = cfeTrackSample(entry->track[CFE_ACTOR_TRACK_SCALE + 2], time);
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
    if (entry->track[CFE_ACTOR_TRACK_CHANNEL] != 0) {
        entry->channelValue = cfeTrackSampleStep(entry->track[CFE_ACTOR_TRACK_CHANNEL], time);
        XenoActorSetChannelValue(entry->actor, entry->channelValue);
        XenoActorSetChannelValueB(entry->actor, entry->channelValue);
    }
    slot = entry->slot;
    if (entry->track[CFE_ACTOR_TRACK_SLOT] != 0) {
        slot = cfeTrackSampleStep(entry->track[CFE_ACTOR_TRACK_SLOT], time);
    }
    if (entry->track[CFE_ACTOR_TRACK_SPEED_A] != 0) {
        entry->speedA = cfeTrackSampleStep(entry->track[CFE_ACTOR_TRACK_SPEED_A], time);
        tiMRStateSetSpeed(&entry->actor->motion[0], entry->speedA);
    }
    if (entry->track[CFE_ACTOR_TRACK_SPEED_B] != 0) {
        entry->speedB = cfeTrackSampleStep(entry->track[CFE_ACTOR_TRACK_SPEED_B], time);
        tiMRStateSetSpeed(&entry->actor->motion[1], entry->speedB);
    }
    if (entry->track[CFE_ACTOR_TRACK_FORCE_MODE] != 0) {
        entry->forceMode = cfeTrackSampleStep(entry->track[CFE_ACTOR_TRACK_FORCE_MODE], time);
    }
    if (entry->track[CFE_ACTOR_TRACK_MODE] != 0) {
        entry->mode = cfeTrackSampleStep(entry->track[CFE_ACTOR_TRACK_MODE], time);
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
    if (entry->track[CFE_ACTOR_TRACK_PLAY_START] != 0) {
        entry->playStart = cfeTrackSampleStep(entry->track[CFE_ACTOR_TRACK_PLAY_START], time);
        changed = 1;
    }
    if (entry->track[CFE_ACTOR_TRACK_PLAY_END] != 0) {
        entry->playEnd = cfeTrackSampleStep(entry->track[CFE_ACTOR_TRACK_PLAY_END], time);
        changed = 1;
    }
    if (changed) {
        XenoActorSetPlayRange(entry->actor, entry->playStart, entry->playEnd);
    }
    if (entry->track[CFE_ACTOR_TRACK_SELECTOR] != 0) {
        entry->selector = cfeTrackSampleStep(entry->track[CFE_ACTOR_TRACK_SELECTOR], time);
        if (entry->actor->selector != entry->selector || time == cfeRangeGetMin()) {
            XenoActorEnableChannelB(entry->actor);
            XenoActorSetSelector(entry->actor, entry->selector);
        }
    }
    if (entry->track[CFE_ACTOR_TRACK_DRAW_MODE] != 0) {
        entry->drawMode = cfeTrackSampleStep(entry->track[CFE_ACTOR_TRACK_DRAW_MODE], time);
        entry->actor->drawMode = entry->drawMode;
    }
    if (entry->track[CFE_ACTOR_TRACK_TRANSLUCENT] != 0) {
        entry->translucent = cfeTrackSampleStep(entry->track[CFE_ACTOR_TRACK_TRANSLUCENT], time);
    }
    if (entry->translucent != 0) {
        entry->actor->drawFlags |= CFE_ACTOR_TRANSLUCENT;
    } else {
        entry->actor->drawFlags &= ~CFE_ACTOR_TRANSLUCENT;
    }
    if (entry->track[CFE_ACTOR_TRACK_ALPHA] != 0) {
        alpha = cfeTrackSample(entry->track[CFE_ACTOR_TRACK_ALPHA], time);
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
    if (entry->track[CFE_ACTOR_TRACK_PARENT_ACT] != 0) {
        entry->parentActId = cfeTrackSampleStep(entry->track[CFE_ACTOR_TRACK_PARENT_ACT], time);
    }
    if (entry->track[CFE_ACTOR_TRACK_PARENT_MAT] != 0) {
        entry->parentMatId = cfeTrackSampleStep(entry->track[CFE_ACTOR_TRACK_PARENT_MAT], time);
    }
    if (entry->parentActId >= 0) {
        if (entry->actor->parent != 0) {
            if (XenoActorReleaseChild(entry->actor->parent, entry->actor) != 1) {
                assertFailed(cfeReleaseChildAssert, cfeEvaluateSourceFile, 393);
            }
        }
        if (XenoActorAppendChildOnJoint(cfeEntries[entry->parentActId].actor, entry->actor,
                                        entry->parentMatId) != 1) {
            assertFailed(cfeAppendChildAssert, cfeEvaluateSourceFile, 396);
        }
    } else if (entry->actor->parent != 0) {
        if (XenoActorReleaseChild(entry->actor->parent, entry->actor) != 1) {
            assertFailed(cfeReleaseChildAssert, cfeEvaluateSourceFile, 402);
        }
    }
    if (entry->track[CFE_ACTOR_TRACK_DAMP_OFFSET] != 0) {
        entry->dampOffset = cfeTrackSampleStep(entry->track[CFE_ACTOR_TRACK_DAMP_OFFSET], time);
    }
    if (entry->track[CFE_ACTOR_TRACK_FIRST_WORD] != 0) {
        entry->firstWord = cfeTrackSampleStep(entry->track[CFE_ACTOR_TRACK_FIRST_WORD], time);
        XenoActorSetFirstWord(entry->actor, entry->firstWord);
    }
    /* CfeFlagOwner is this file's view of the actor (its flag words and record list). */
    cfeRecordListSample(time, (CfeFlagOwner *) entry->actor, entry->records);
    if (entry->headTrack[CFE_ACTOR_HEAD_SHOWN] != 0) {
        if (cfeTrackSampleStep(entry->headTrack[CFE_ACTOR_HEAD_SHOWN], time) != 0.0f) {
            entry->flags |= CFE_ACTOR_SHOWN;
        } else {
            entry->flags &= ~CFE_ACTOR_SHOWN;
        }
    }
    if (entry->headTrack[CFE_ACTOR_HEAD_POSITION + 0] != 0) {
        entry->position.x = cfeTrackSample(entry->headTrack[CFE_ACTOR_HEAD_POSITION + 0], time);
    }
    if (entry->headTrack[CFE_ACTOR_HEAD_POSITION + 1] != 0) {
        entry->position.y = cfeTrackSample(entry->headTrack[CFE_ACTOR_HEAD_POSITION + 1], time);
    }
    if (entry->headTrack[CFE_ACTOR_HEAD_POSITION + 2] != 0) {
        entry->position.z = cfeTrackSample(entry->headTrack[CFE_ACTOR_HEAD_POSITION + 2], time);
    }
}


/* Evaluates every actor entry of cfeEntries (cfeEntryCount of them) with cfeEvaluateNode. */
int cfeEvaluateNodes(int sample_param)
{
    CfeActor *entry;
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
                entry++;
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

void func_001BB3FC(Vector4 *value);
void func_001BBBD8(void);


/* Evaluates every group at `time`: its position from tracks 0..2, the light value from track 4 and the
 * hidden flag from track 3 (hidden, or shown with its light applied and its collision set made current),
 * then the records of all its parts into their matrices. */
void cfeGroupTableEvaluate(int time)
{
    int i;
    int j;

    for (i = 0; i < cfeGroupCount; i++) {
        if (cfeGroups[i].tracks[0] != 0) {
            cfeGroups[i].position.x = cfeTrackSample(cfeGroups[i].tracks[0], time);
        }
        if (cfeGroups[i].tracks[1] != 0) {
            cfeGroups[i].position.y = cfeTrackSample(cfeGroups[i].tracks[1], time);
        }
        if (cfeGroups[i].tracks[2] != 0) {
            cfeGroups[i].position.z = cfeTrackSample(cfeGroups[i].tracks[2], time);
        }
        if (cfeGroups[i].tracks[4] != 0) {
            cfeGroups[i].light = cfeTrackSampleStep(cfeGroups[i].tracks[4], time);
        }
        if (cfeGroups[i].tracks[3] != 0) {
            if ((int) cfeTrackSampleStep(cfeGroups[i].tracks[3], time) != 0) {
                cfeGroups[i].object->drawFlags |= CFE_ACTOR_HIDDEN;
            } else {
                if (cfeGroups[i].light != 0) {
                    func_001BBBD8();
                }
                func_001BB3FC(&cfeGroups[i].position);
                cfeGroups[i].object->drawFlags &= ~CFE_ACTOR_HIDDEN;
                /* TiGlobalColl.c keeps the handle as an int. */
                tiGlobalCollSetHandle((int) cfeGroups[i].collision);
            }
        }
        if (cfeGroups[i].parts != 0) {
            for (j = 0; j < cfeGroups[i].parts->geometryCount; j++) {
                cfeRecordEvaluate(&cfeGroups[i].records[j], cfeGroups[i].matrices[j], time);
            }
        }
    }
}

void *memset(void *destination, int value, unsigned int size);

/* The shake offsets of the camera: position x, y, z, then rotation x, y, z. */
typedef struct CfeCameraOffset {
    f32 position[3];
    f32 rotation[3];
} CfeCameraOffset;

/* The studio block cfeCamera (0x70 bytes) as the camera evaluation sees it: the camera, its key tracks
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

extern CfeStudioView cfeCamera;
/* -1.0f and 1.0f: the random sign of a shake offset. */
extern f32 cfeShakeSigns[2];

/* The flag of CfeCameraEval.flags that stops the evaluation. */
#define CFE_CAMERA_HELD 0x1
/* The units of the shake amount for the position and the rotation. */
#define CFE_SHAKE_POSITION_UNIT 0.001f
#define CFE_SHAKE_ROTATION_UNIT 0.0001f

/* Evaluates the studio camera at `time`: its position, rotation and field of view from their tracks, and
 * the camera shake: while it is on, the camera moves from one random offset to the next over shakeFrames
 * frames (the counter only runs when `paused` is 0); off, the offsets are cleared. Then moves the studio. */
void cfeCameraEvaluate(int time, int paused)
{
    float blend;
    float rest;

    xglStudioGetCamera(&cfeCamera.eval.camera, 0);
    if (cfeCamera.eval.flags & CFE_CAMERA_HELD) {
        return;
    }
    if (cfeCamera.eval.position[0] != 0) {
        cfeCamera.eval.camera->travel.position.x = cfeTrackSample(cfeCamera.eval.position[0], time);
    }
    if (cfeCamera.eval.position[1] != 0) {
        cfeCamera.eval.camera->travel.position.y = cfeTrackSample(cfeCamera.eval.position[1], time);
    }
    if (cfeCamera.eval.position[2] != 0) {
        cfeCamera.eval.camera->travel.position.z = cfeTrackSample(cfeCamera.eval.position[2], time);
    }
    if (cfeCamera.eval.rotation[0] != 0) {
        cfeCamera.eval.camera->travel.rotation.x = cfeTrackSample(cfeCamera.eval.rotation[0], time);
    }
    if (cfeCamera.eval.rotation[1] != 0) {
        cfeCamera.eval.camera->travel.rotation.y = cfeTrackSample(cfeCamera.eval.rotation[1], time);
    }
    if (cfeCamera.eval.rotation[2] != 0) {
        cfeCamera.eval.camera->travel.rotation.z = cfeTrackSample(cfeCamera.eval.rotation[2], time);
    }
    if (cfeCamera.eval.fov != 0) {
        cfeCamera.eval.camera->travel.fov = cfeTrackSample(cfeCamera.eval.fov, time);
    }
    if (cfeCamera.eval.shakeTrack[0] != 0) {
        cfeCamera.eval.shakeOn = cfeTrackSampleStep(cfeCamera.eval.shakeTrack[0], time);
    }
    if (cfeCamera.eval.shakeTrack[1] != 0) {
        cfeCamera.eval.shakeFrames = cfeTrackSampleStep(cfeCamera.eval.shakeTrack[1], time);
    }
    if (cfeCamera.eval.shakeTrack[2] != 0) {
        cfeCamera.eval.shakeAmount = cfeTrackSampleStep(cfeCamera.eval.shakeTrack[2], time);
    }
    if (cfeCamera.eval.shakeOn != 0 && cfeCamera.eval.shakeAmount != 0) {
        if (cfeCamera.eval.shakeCounter < 0 && paused == 0) {
            cfeCamera.eval.shakeCounter = cfeCamera.eval.shakeFrames;
            cfeCamera.eval.from = cfeCamera.eval.to;
            cfeCamera.eval.to.position[0] = xglSRand() % (cfeCamera.eval.shakeAmount + 1) * CFE_SHAKE_POSITION_UNIT
                                             * cfeShakeSigns[xglSRand() & 1];
            cfeCamera.eval.to.position[1] = xglSRand() % (cfeCamera.eval.shakeAmount + 1) * CFE_SHAKE_POSITION_UNIT
                                             * cfeShakeSigns[xglSRand() & 1];
            cfeCamera.eval.to.position[2] = xglSRand() % (cfeCamera.eval.shakeAmount + 1) * CFE_SHAKE_POSITION_UNIT
                                             * cfeShakeSigns[xglSRand() & 1];
            cfeCamera.eval.to.rotation[0] = xglSRand() % (cfeCamera.eval.shakeAmount + 1) * CFE_SHAKE_ROTATION_UNIT
                                             * cfeShakeSigns[xglSRand() & 1];
            cfeCamera.eval.to.rotation[1] = xglSRand() % (cfeCamera.eval.shakeAmount + 1) * CFE_SHAKE_ROTATION_UNIT
                                             * cfeShakeSigns[xglSRand() & 1];
            cfeCamera.eval.to.rotation[2] = xglSRand() % (cfeCamera.eval.shakeAmount + 1) * CFE_SHAKE_ROTATION_UNIT
                                             * cfeShakeSigns[xglSRand() & 1];
        }
        blend = (float) cfeCamera.eval.shakeCounter / (float) (cfeCamera.eval.shakeFrames + 1);
        if (1.0f < blend) {
            blend = 1.0f;
        }
        rest = 1.0f - blend;
        cfeCamera.eval.camera->travel.position.x
            += cfeCamera.eval.from.position[0] * blend + cfeCamera.eval.to.position[0] * rest;
        cfeCamera.eval.camera->travel.position.y
            += cfeCamera.eval.from.position[1] * blend + cfeCamera.eval.to.position[1] * rest;
        cfeCamera.eval.camera->travel.position.z
            += cfeCamera.eval.from.position[2] * blend + cfeCamera.eval.to.position[2] * rest;
        cfeCamera.eval.camera->travel.rotation.x
            += cfeCamera.eval.from.rotation[0] * blend + cfeCamera.eval.to.rotation[0] * rest;
        cfeCamera.eval.camera->travel.rotation.y
            += cfeCamera.eval.from.rotation[1] * blend + cfeCamera.eval.to.rotation[1] * rest;
        cfeCamera.eval.camera->travel.rotation.z
            += cfeCamera.eval.from.rotation[2] * blend + cfeCamera.eval.to.rotation[2] * rest;
        if (paused == 0) {
            cfeCamera.eval.shakeCounter--;
        }
    } else {
        cfeCamera.eval.shakeCounter = 0;
        memset(&cfeCamera.eval.from, 0, sizeof(cfeCamera.eval.from));
        memset(&cfeCamera.eval.to, 0, sizeof(cfeCamera.eval.to));
    }
    xglStudioMove();
}

/* The scale of a spawned effect; the original writes it through a union member (alias set 0). */
typedef union CfeEffectScale {
    float element[3];
} CfeEffectScale;

/* The spawned effect record, as far as cfeCommandSpawn writes it. */
typedef struct CfeSpawnedEffect {
    unsigned char unmodeled_00[0x20];
    CfeEffectScale scale;       /* +0x20 */
    unsigned char unmodeled_2c[0x44 - 0x2C];
    int flags;                  /* +0x44 */
} CfeSpawnedEffect;

/* Effect record flag: the record is drawn without the view-volume test (sv_get_image_item.c's culling
 * skips records with it). */
#define CFE_EFFECT_NO_CULL 0x10

void sefClearOwnerFlags(unsigned int id, int flags);
void sefExecEffectSteps(CfeSpawnedEffect *effect, float frames, float steps);
void sefSetRecordOwner(CfeSpawnedEffect *effect, void *owner);

/* Spawns the command's effect (clearing its owner flags first), sets its scale, runs its first
 * frames (one step per 15 frames) and, when the command names an owner entry, hands the effect
 * to that entry's object. */
void cfeCommandSpawn(CfeEvalCommand *command)
{
    CfeSpawnedEffect *effect;
    unsigned char *entry;

    if (command->flag != 0) {
        sefClearOwnerFlags((command->page_index << CFE_PAGE_SHIFT) | command->page_offset | CFE_BANK_FLAGGED,
                           CFE_OWNER_FLAGS);
        effect = (CfeSpawnedEffect *) sefSpawnEffectRecordById(
            (command->page_index << CFE_PAGE_SHIFT) | command->page_offset | CFE_BANK_FLAGGED, (int) &command->position,
            (unsigned char *) &command->rotation);
    } else {
        sefClearOwnerFlags((command->page_index << CFE_PAGE_SHIFT) | command->page_offset | CFE_BANK_PLAIN,
                           CFE_OWNER_FLAGS);
        effect = (CfeSpawnedEffect *) sefSpawnEffectRecordById(
            (command->page_index << CFE_PAGE_SHIFT) | command->page_offset | CFE_BANK_PLAIN, (int) &command->position,
            (unsigned char *) &command->rotation);
    }
    if (effect != 0) {
        effect->scale.element[0] = command->scale[0];
        effect->scale.element[1] = command->scale[1];
        effect->scale.element[2] = command->scale[2];
        sefExecEffectSteps(effect, command->frames, command->frames / 15 + 1);
        effect->flags |= CFE_EFFECT_NO_CULL;
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
    cfeCommandSpawn(command);
}

/* Samples the command's tracks at `time` (position, rotation, scale, frames, owner, action) and runs
 * its action: 1 spawns the effect, 2 sets the owner flags, 3 releases the owned records, 4 releases
 * them and spawns the effect again. */
void cfeCommandEvaluate(CfeEvalCommand *command, int time)
{
    if (command->track[CFE_COMMAND_TRACK_POSITION + 0] != 0) {
        command->position.x = cfeTrackSample(command->track[CFE_COMMAND_TRACK_POSITION + 0], time);
    }
    if (command->track[CFE_COMMAND_TRACK_POSITION + 1] != 0) {
        command->position.y = cfeTrackSample(command->track[CFE_COMMAND_TRACK_POSITION + 1], time);
    }
    if (command->track[CFE_COMMAND_TRACK_POSITION + 2] != 0) {
        command->position.z = cfeTrackSample(command->track[CFE_COMMAND_TRACK_POSITION + 2], time);
    }
    if (command->track[CFE_COMMAND_TRACK_ROTATION + 0] != 0) {
        command->rotation.x = cfeTrackSample(command->track[CFE_COMMAND_TRACK_ROTATION + 0], time);
    }
    if (command->track[CFE_COMMAND_TRACK_ROTATION + 1] != 0) {
        command->rotation.y = cfeTrackSample(command->track[CFE_COMMAND_TRACK_ROTATION + 1], time);
    }
    if (command->track[CFE_COMMAND_TRACK_ROTATION + 2] != 0) {
        command->rotation.z = cfeTrackSample(command->track[CFE_COMMAND_TRACK_ROTATION + 2], time);
    }
    if (command->track[CFE_COMMAND_TRACK_SCALE + 0] != 0) {
        command->scale[0] = cfeTrackSample(command->track[CFE_COMMAND_TRACK_SCALE + 0], time);
    }
    if (command->track[CFE_COMMAND_TRACK_SCALE + 1] != 0) {
        command->scale[1] = cfeTrackSample(command->track[CFE_COMMAND_TRACK_SCALE + 1], time);
    }
    if (command->track[CFE_COMMAND_TRACK_SCALE + 2] != 0) {
        command->scale[2] = cfeTrackSample(command->track[CFE_COMMAND_TRACK_SCALE + 2], time);
    }
    if (command->track[CFE_COMMAND_TRACK_FRAMES] != 0) {
        command->frames = (int) cfeTrackSampleStep(command->track[CFE_COMMAND_TRACK_FRAMES], time);
    }
    if (command->track[CFE_COMMAND_TRACK_OWNER] != 0) {
        command->owner = (int) cfeTrackSampleStep(command->track[CFE_COMMAND_TRACK_OWNER], time);
    }
    if (command->track[CFE_COMMAND_TRACK_ACTION] != 0) {
        command->action = (int) cfeTrackSampleStep(command->track[CFE_COMMAND_TRACK_ACTION], time);
        switch (command->action) {
        case CFE_ACTION_NONE:
            break;
        case CFE_ACTION_SPAWN:
            cfeCommandSpawn(command);
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


/* Samples the tracks of the four light rows at `time` and rotates each row's direction. */
void cfeEvaluateDirections(int time)
{
    CfeLight *row;
    int i;

    for (i = 0; i < 4; i++) {
        row = &cfeDirections[i];
        if (row->track[CFE_LIGHT_TRACK_EXTRA + 0] != 0) {
            row->extra[0] = cfeTrackSample(row->track[CFE_LIGHT_TRACK_EXTRA + 0], time);
        }
        if (row->track[CFE_LIGHT_TRACK_EXTRA + 1] != 0) {
            row->extra[1] = cfeTrackSample(row->track[CFE_LIGHT_TRACK_EXTRA + 1], time);
        }
        if (row->track[CFE_LIGHT_TRACK_EXTRA + 2] != 0) {
            row->extra[2] = cfeTrackSample(row->track[CFE_LIGHT_TRACK_EXTRA + 2], time);
        }
        if (row->track[CFE_LIGHT_TRACK_COLOR + 0] != 0) {
            row->color[0] = cfeTrackSample(row->track[CFE_LIGHT_TRACK_COLOR + 0], time);
        }
        if (row->track[CFE_LIGHT_TRACK_COLOR + 1] != 0) {
            row->color[1] = cfeTrackSample(row->track[CFE_LIGHT_TRACK_COLOR + 1], time);
        }
        if (row->track[CFE_LIGHT_TRACK_COLOR + 2] != 0) {
            row->color[2] = cfeTrackSample(row->track[CFE_LIGHT_TRACK_COLOR + 2], time);
        }
        if (row->track[CFE_LIGHT_TRACK_ANGLE + 0] != 0) {
            row->angles[0] = cfeTrackSample(row->track[CFE_LIGHT_TRACK_ANGLE + 0], time);
        }
        if (row->track[CFE_LIGHT_TRACK_ANGLE + 1] != 0) {
            row->angles[1] = cfeTrackSample(row->track[CFE_LIGHT_TRACK_ANGLE + 1], time);
        }
        if (row->track[CFE_LIGHT_TRACK_ANGLE + 2] != 0) {
            row->angles[2] = cfeTrackSample(row->track[CFE_LIGHT_TRACK_ANGLE + 2], time);
        }
        if (row->direction != 0) {
            cfeRotateVectorXY(row->direction, row->angles);
        }
    }
}


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
        cfeEvalGlobal.fogEnabled = (int) cfeTrackSampleStep(cfeEvalGlobal.source[7], sample_param);
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
        entry->rewind = (int) cfeTrackSample(entry->source[9], sample_param);
    }
}


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
    if (slot->source[CFE_SLOT_SOURCE_RESULT_INT] != 0) {
        slot->result_int = (int) cfeTrackSampleStep(slot->source[CFE_SLOT_SOURCE_RESULT_INT], sample_param);
    }
    if (slot->source[CFE_SLOT_SOURCE_SAMPLE_FLOAT] != 0) {
        slot->sample_float = cfeTrackSample(slot->source[CFE_SLOT_SOURCE_SAMPLE_FLOAT], sample_param);
    }
    if (slot->source[CFE_SLOT_SOURCE_SAMPLE_INT] != 0) {
        slot->sample_int = (int) cfeTrackSample(slot->source[CFE_SLOT_SOURCE_SAMPLE_INT], sample_param);
    }
}


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
void cfeSndRecordEvaluate(CfeSndRecord *record, int time)
{
    int old;
    int value;
    int steps;

    if (record->track[CFE_SND_TRACK_DURATION] != 0) {
        record->duration = (int) cfeTrackSampleStep(record->track[CFE_SND_TRACK_DURATION], time);
    }
    if (record->track[CFE_SND_TRACK_VALUE] != 0) {
        old = record->value;
        value = (int) cfeTrackSampleStep(record->track[CFE_SND_TRACK_VALUE], time);
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
   keeps a frame for the call and does not tail-call cfeSndRecordEvaluate. */
void cfeSndSetTarget(int target)
{
  int *target_ref;
  target_ref = &target;
  cfeSndRecordEvaluate(cfeSndRecords, *target_ref);
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
void cfeDrawLine(int x0, int y0, int x1, int y1, unsigned int color0, unsigned int color1, unsigned int depth)
{
    u64 *cursor;
    u64 *start;

    xglDirectQueueBegin();
    cursor = (u64 *) &xglDirectQueueBuffer[xglDirectQueueCount];
    start = cursor;
    GS_PUT(cursor, GS_TEST_1, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 2));
    GS_PUT(cursor, GS_ZBUF_1, GS_SET_ZBUF(sRenderDepthBase, 1, 0));
    GS_PUT(cursor, GS_PRIM, GS_SET_PRIM(CFE_GS_PRIM_LINE, 1, 0, 0, 0, 0, 0, 0, 0));
    GS_PUT(cursor, GS_RGBAQ, CFE_GS_RGBAQ(color0));
    GS_PUT(cursor, GS_XYZF2, GS_SET_XYZ((x0 << 4) + CFE_GS_ORIGIN_X, (y0 << 4) + CFE_GS_ORIGIN_Y, depth));
    GS_PUT(cursor, GS_RGBAQ, CFE_GS_RGBAQ(color1));
    GS_PUT(cursor, GS_XYZF2, GS_SET_XYZ((x1 << 4) + CFE_GS_ORIGIN_X, (y1 << 4) + CFE_GS_ORIGIN_Y, depth));
    xglDirectQueueCount += ((u32) cursor - (u32) start) >> 4;
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
void cfeDrawRotationGrid(int x, int y, Vector4 *angles, float *color0, float *color1)
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
            cfeDrawLine(SCREEN_X(line[0].x, line[0].z), SCREEN_Y(line[0].y, line[0].z),
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
            cfeDrawLine(SCREEN_X(line[2].x, line[2].z), SCREEN_Y(line[2].y, line[2].z),
                          SCREEN_X(line[3].x, line[3].z), SCREEN_Y(line[3].y, line[3].z),
                          mixed_color, mixed_color, 0xFFFFFF);
            near_weight += 0.1f;
        }
        tip.x = 0.0f;
        tip.y = 0.0f;
        tip.z = 70.0f;
        tip.w = 1.0f;
        xglVectorMulMat(&tip, view, &tip);
        cfeDrawLine(SCREEN_X(0.0f, tip.z), SCREEN_Y(0.0f, tip.z),
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
            cfeDrawLine(SCREEN_X(line[0].x, line[0].z), SCREEN_Y(line[0].y, line[0].z),
                          SCREEN_X(line[1].x, line[1].z), SCREEN_Y(line[1].y, line[1].z),
                          near_color, far_color, 0xFFFFFF);
            mixed_color = 0;
            mixed_color += (unsigned char) (near_weight * CHANNEL(near_color, 0) + (1.0f - near_weight) * CHANNEL(far_color, 0));
            mixed_color += (unsigned char) (near_weight * CHANNEL(near_color, 8) + (1.0f - near_weight) * CHANNEL(far_color, 8)) << 8;
            mixed_color += (unsigned char) (near_weight * CHANNEL(near_color, 16) + (1.0f - near_weight) * CHANNEL(far_color, 16)) << 16;
            mixed_color += (unsigned char) (near_weight * CHANNEL(near_color, 24) + (1.0f - near_weight) * CHANNEL(far_color, 24)) << 24;
            cfeDrawLine(SCREEN_X(line[2].x, line[2].z), SCREEN_Y(line[2].y, line[2].z),
                          SCREEN_X(line[3].x, line[3].z), SCREEN_Y(line[3].y, line[3].z),
                          mixed_color, mixed_color, 0xFFFFFF);
            near_weight += 0.1f;
        }
        tip.x = 0.0f;
        tip.y = 0.0f;
        tip.z = 70.0f;
        tip.w = 1.0f;
        xglVectorMulMat(&tip, view, &tip);
        cfeDrawLine(SCREEN_X(0.0f, tip.z), SCREEN_Y(0.0f, tip.z),
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
void cfeEntryDrawMovie(CfeEvalEntry *entry)
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
            if (cfeEvalEntries[i].rewind != 0) {
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
        cfeEntryDrawMovie(&cfeEvalEntries[i]);
        }
    }
}
