#ifndef MAIN_CFEEVALUATE_H
#define MAIN_CFEEVALUATE_H

#include "shared.h"

/* One 0x80-byte command of cfeCommands: an effect (its id built from a page index and a page offset,
 * cfeCommandSetOwnerFlags), where and how it is spawned, and the tracks that drive those values. */
typedef struct CfeEvalCommand {
    int flag;                   /* +0x00 */
    int page_index;             /* +0x04 */
    int page_offset;            /* +0x08 */
    unsigned char unmodeled_0c[4];
    Vector4 position;           /* +0x10 */
    Vector4 rotation;           /* +0x20 */
    float scale[3];             /* +0x30 */
    unsigned char unmodeled_3c[4];
    int frames;                 /* +0x40 */
    int action;                 /* +0x44: what cfeCommandEvaluate does, 0 to 4 */
    int owner;                  /* +0x48: entry of cfeEntries that owns the effect, or negative */
    int positionTracks[3];      /* +0x4C */
    int rotationTracks[3];      /* +0x58 */
    int scaleTracks[3];         /* +0x64 */
    int actionTrack;            /* +0x70 */
    int framesTrack;            /* +0x74 */
    int ownerTrack;             /* +0x78 */
    unsigned char unmodeled_7c[4];
} CfeEvalCommand;

/* Command word layout: page index above bit 12, a bank bit selected by the flag. */
#define CFE_PAGE_SHIFT 0xC
#define CFE_BANK_FLAGGED 0x20000000
#define CFE_BANK_PLAIN 0x10000000
#define CFE_OWNER_FLAGS 0x80

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
