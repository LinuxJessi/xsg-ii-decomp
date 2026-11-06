#include "common.h"
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
void cfeRecordBuildMatrix(CfeEvalRecord *record, int dest_matrix) {
    xglCurSetTranslate(&record->evaluated[0]);
    xglCurMulRotateZ(record->evaluated[1].z);
    xglCurMulRotateY(record->evaluated[1].y);
    xglCurMulRotateX(record->evaluated[1].x);
    xglCurMulScaleXYZ(&record->evaluated[2]);
    xglCurMatrixGet(dest_matrix);
}

f32 cfeTrackSample(int, int);

/* Samples the nine sources of a record into its three vectors, then builds its matrix (cfeRecordBuildMatrix). */
void cfeRecordEvaluate(CfeEvalRecord *record, int dest_matrix, int sample_param) {
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

/* Builds the command word from page_index and page_offset; the flag selects bank 0x20000000 or 0x10000000. */
void cfeCommandSetOwnerFlags(CfeEvalCommand *command) {
    if (command->flag != 0) {
        sefSetOwnerFlags((command->page_index << 0xC) | command->page_offset | 0x20000000, 0x80);
        return;
    }
    sefSetOwnerFlags((command->page_index << 0xC) | command->page_offset | 0x10000000, 0x80);
}


void cfeCommandReleaseRecords(CfeEvalCommand *command) {
    if (command->flag != 0) {
        sefReleaseOwnedRecords((command->page_index << 0xC) | command->page_offset | 0x20000000);
        return;
    }
    sefReleaseOwnedRecords((command->page_index << 0xC) | command->page_offset | 0x10000000);
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
