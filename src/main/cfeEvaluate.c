#include "common.h"
#include "shared.h"
#include "main/cfePlugIn.h"
#include "main/sef.h"
#include "main/sdv_033.h"
#include "main/tu138.h"
#include "main/tu256.h"
#include "gs.h"
#include "main/tu045.h"

f32 func_002508C8(int, int);
int cfeListHasItems(void *list);

INCLUDE_ASM("asm/main/nonmatchings/cfeEvaluate", cfeListHasItems);

/* One sampled flag: the value and the track it is sampled from (8 bytes). */
typedef struct CfeFlagRecord {
    int value;
    int track;
} CfeFlagRecord;

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

/* Samples a flag record: its track at `time` into its value and into bit
 * `index` of the owner's flag words. */
void cfeRecordSample(int time, CfeFlagOwner *owner, CfeFlagRecord *record, int index)
{
    int value;

    if (record->track != 0) {
        value = (int) func_002508C8(record->track, time);
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

INCLUDE_ASM("asm/main/nonmatchings/cfeEvaluate", cfeEvaluateNode);

#include "cfeEvaluate.h"

void cfeEvaluateNode(void *, int);
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

void xglCurMatrixGet(f32 matrix[4][4]);
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

f32 cfeTrackSample(int, int);

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

INCLUDE_ASM("asm/main/nonmatchings/cfeEvaluate", func_00248390);

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

INCLUDE_ASM("asm/main/nonmatchings/cfeEvaluate", cfeCommandTableEvaluate);

/* A row of cfeDirections (0x40 bytes): the angles of a light direction, the vectors it drives and
 * the tracks they are sampled from. */
typedef struct CfeDirectionRow {
    float angles[3];            /* +0x00: X and Y rotation of the direction */
    unsigned char unmodeled_0c[4];
    Vector4 *direction;         /* +0x10: rotated from angles */
    Vector4 *color;             /* +0x14 */
    Vector4 *extra;             /* +0x18 */
    int extraTracks[3];         /* +0x1C */
    int angleTracks[3];         /* +0x28 */
    int colorTracks[3];         /* +0x34 */
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
        entry->evaluated[0].x = cfeTrackSample(entry->source[0], sample_param);
    }
    if (entry->source[1] != 0) {
        entry->evaluated[0].y = cfeTrackSample(entry->source[1], sample_param);
    }
    if (entry->source[2] != 0) {
        entry->evaluated[0].z = cfeTrackSample(entry->source[2], sample_param);
    }
    if (entry->source[3] != 0) {
        entry->evaluated[1].x = cfeTrackSample(entry->source[3], sample_param);
    }
    if (entry->source[4] != 0) {
        entry->evaluated[1].y = cfeTrackSample(entry->source[4], sample_param);
    }
    if (entry->source[5] != 0) {
        entry->evaluated[1].z = cfeTrackSample(entry->source[5], sample_param);
    }
    if (entry->source[6] != 0) {
        entry->evaluated[2].x = cfeTrackSample(entry->source[6], sample_param);
    }
    if (entry->source[7] != 0) {
        entry->evaluated[2].y = cfeTrackSample(entry->source[7], sample_param);
    }
    if (entry->source[8] != 0) {
        entry->evaluated[2].z = cfeTrackSample(entry->source[8], sample_param);
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

void seVolumeSet(int value, int flags);

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

void xglDirectQueueBegin(void);
void xglDirectQueueDefer(int mode, int count);
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

INCLUDE_ASM("asm/main/nonmatchings/cfeEvaluate", func_00249518);

typedef struct PxyCamera {
    u8 unk00[0xA0];
    float rotX; /* +0xA0 */
    float rotY; /* +0xA4 */
} PxyCamera;

PxyCamera *xglStudioSelectGetActiveCamera(int index);
void xglVectorNormal_001A9200(Vector4 *destination, const Vector4 *source);
void func_00297580(Vector4 *quaternion, const Vector4 *from, const Vector4 *to);
void XrgQuaternionToMatrix(Matrix4 matrix, const Vector4 *quaternion);
void xglMatrixStackUnit(void);
void xglMatrixStackRotX(float angle);
void xglMatrixStackRotY(float angle);
void xglMatrixStackMul(Matrix4 matrix);
void xglMatrixStackSave(Matrix4 matrix);
void xglVectorMulMat(Vector4 *destination, Matrix4 matrix, const Vector4 *vector);
void xglVectorInner(float *result, const Vector4 *left, const Vector4 *right);

/* A 0..1 float channel scaled to the 0..128 GS range. */
#define COLOR_BYTE(f) ((unsigned int) ((f) * 128.0f) & 0xFF)
/* Screen position of a camera-space point around (x + 60, y + 40): a
   perspective with the eye 300 units behind the plane and a 450-unit scale. */
#define SCREEN_X(coord, depth) (x + 60 + (coord) * (distance + (depth)) / screen)
#define SCREEN_Y(coord, depth) (y + 40 + (coord) * (distance + (depth)) / screen)
/* One 8-bit channel of a packed RGBA colour. */
#define CHANNEL(c, shift) (((c) >> (shift)) & 0xFF)

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
    PxyCamera *camera;
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
        xglMatrixStackRotX(camera->rotX);
        xglMatrixStackRotY(-camera->rotY);
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

INCLUDE_ASM("asm/main/nonmatchings/cfeEvaluate", func_0024A8C0);

void xglMovieRewind(CfeEvalEntry *entry);
void xglMoviePlay(CfeEvalEntry *entry);
void func_0024A8C0(CfeEvalEntry *entry);

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
                xglMovieRewind(&cfeEvalEntries[i]);
            }
        if (__builtin_fabsf(cfeEvalEntries[i].evaluated[2].x) < CFE_ENTRY_MIN_SCALE
            || __builtin_fabsf(cfeEvalEntries[i].evaluated[2].y) < CFE_ENTRY_MIN_SCALE
            || __builtin_fabsf(cfeEvalEntries[i].evaluated[2].z) < CFE_ENTRY_MIN_SCALE) {
            continue;
        }
        if (noPlay == 0) {
            xglMoviePlay(&cfeEvalEntries[i]);
        }
        func_0024A8C0(&cfeEvalEntries[i]);
        }
    }
}
