#ifndef MAIN_CFEEVALUATE_H
#define MAIN_CFEEVALUATE_H

#include "shared.h"

/* Command word built from a page index and a page offset (cfeCommandSetOwnerFlags). */
typedef struct CfeEvalCommand {
    int flag;
    int page_index;
    int page_offset;
} CfeEvalCommand;

/* One 0x18-byte slot of the table at cfeEvalSlots: three sources and their results. */
typedef struct CfeEvalSlot {
    int result_int;
    int sample_int;
    float sample_float;
    int source_for_result_int;
    int source_for_sample_float;
    int source_for_sample_int;
} CfeEvalSlot;

/* Global block at cfeEvalGlobal (cfeGlobalEvaluate): one result word, two vectors and eight source handles. */
typedef struct CfeEvalGlobal {
    int result_int;
    unsigned char unmodeled_04[0x0C];
    Vector4 evaluated[2];
    int source[8];
} CfeEvalGlobal;

/* Record written by cfeRecordEvaluate: three vectors of results and nine source handles. */
typedef struct CfeEvalRecord {
    Vector4 evaluated[3];
    unsigned char unmodeled_30[0x20];
    int source[9];
} CfeEvalRecord;

/* Table entry (stride 0x230) of cfeEntryEvaluate: three vectors, a result word and ten source handles. */
typedef struct CfeEvalEntry {
    unsigned char unmodeled_00[0x110];
    Vector4 evaluated[3];
    unsigned char unmodeled_140[0xC0];
    int result_int;
    int source[10];
    unsigned char unmodeled_22c[4];
} CfeEvalEntry;

#endif /* MAIN_CFEEVALUATE_H */
