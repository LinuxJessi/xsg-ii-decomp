#include "common.h"
#include "shared.h"
#include "main/cfePlugIn.h"
#include "main/tu175.h"
#include "main/tu256.h"

INCLUDE_ASM("asm/main/nonmatchings/cfeEvaluate", cfeListHasItems);

INCLUDE_ASM("asm/main/nonmatchings/cfeEvaluate", cfeRecordSample);

INCLUDE_ASM("asm/main/nonmatchings/cfeEvaluate", cfeRecordListSample);

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
    xglCurSetTranslate(&record->evaluated[0]);
    xglCurMulRotateZ(record->evaluated[1].z);
    xglCurMulRotateY(record->evaluated[1].y);
    xglCurMulRotateX(record->evaluated[1].x);
    xglCurMulScaleXYZ(&record->evaluated[2]);
    xglCurMatrixGet(dest_matrix);
}

f32 cfeTrackSample(int, int);

/* Samples the nine sources of a record into its three vectors, then builds its matrix (cfeRecordBuildMatrix). */
void cfeRecordEvaluate(CfeEvalRecord *record, f32 dest_matrix[4][4], int sample_param) {
    if (record->source[0] != 0) {
        record->evaluated[0].x = cfeTrackSample(record->source[0], sample_param);
    }
    if (record->source[1] != 0) {
        record->evaluated[0].y = cfeTrackSample(record->source[1], sample_param);
    }
    if (record->source[2] != 0) {
        record->evaluated[0].z = cfeTrackSample(record->source[2], sample_param);
    }
    if (record->source[3] != 0) {
        record->evaluated[1].x = cfeTrackSample(record->source[3], sample_param);
    }
    if (record->source[4] != 0) {
        record->evaluated[1].y = cfeTrackSample(record->source[4], sample_param);
    }
    if (record->source[5] != 0) {
        record->evaluated[1].z = cfeTrackSample(record->source[5], sample_param);
    }
    if (record->source[6] != 0) {
        record->evaluated[2].x = cfeTrackSample(record->source[6], sample_param);
    }
    if (record->source[7] != 0) {
        record->evaluated[2].y = cfeTrackSample(record->source[7], sample_param);
    }
    if (record->source[8] != 0) {
        record->evaluated[2].z = cfeTrackSample(record->source[8], sample_param);
    }
    cfeRecordBuildMatrix(record, dest_matrix);
}

INCLUDE_ASM("asm/main/nonmatchings/cfeEvaluate", cfeGroupTableEvaluate);

INCLUDE_ASM("asm/main/nonmatchings/cfeEvaluate", func_00248390);

INCLUDE_ASM("asm/main/nonmatchings/cfeEvaluate", func_002488F8);

void sefSetOwnerFlags(int, int);

/* Command word layout: page index above bit 12, a bank bit selected by the flag. */
#define CFE_PAGE_SHIFT 0xC
#define CFE_BANK_FLAGGED 0x20000000
#define CFE_BANK_PLAIN 0x10000000
#define CFE_OWNER_FLAGS 0x80

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

INCLUDE_ASM("asm/main/nonmatchings/cfeEvaluate", cfeCommandEvaluate);

INCLUDE_ASM("asm/main/nonmatchings/cfeEvaluate", cfeCommandTableEvaluate);

INCLUDE_ASM("asm/main/nonmatchings/cfeEvaluate", cfeEvaluateDirections);

f32 func_002508C8(int, int);
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

INCLUDE_ASM("asm/main/nonmatchings/cfeEvaluate", func_00249248);

/* Updates one 0x50-byte sound record: the value at +0x40 moves toward the
   target, and +0x44 holds the duration. */
void func_00249248(CfeSndRecord *record, int target);

/* Sets the target of record 0. The argument's address is taken: the original
   keeps a frame for the call and does not tail-call func_00249248. */
void cfeSndSetTarget(int target)
{
  int *target_ref;
  target_ref = &target;
  func_00249248(cfeSndRecords, *target_ref);
}

INCLUDE_ASM("asm/main/nonmatchings/cfeEvaluate", func_00249340);

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
void func_00249340(int x0, int y0, int x1, int y1, unsigned int color0, unsigned int color1, unsigned int flags);

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

INCLUDE_ASM("asm/main/nonmatchings/cfeEvaluate", cfeUpdateEntries);
