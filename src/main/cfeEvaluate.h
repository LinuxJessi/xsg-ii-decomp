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

/* A vector of a CfeEvalRecord. The original writes them through a union member (alias set 0): the
 * record pointer is reloaded after every store. */
typedef union CfeEvalVector {
    Vector4 vector;
} CfeEvalVector;

/* Record written by cfeRecordEvaluate: three vectors of results, the starting position and angles and nine source
 * handles. */
typedef struct CfeEvalRecord {
    CfeEvalVector evaluated[3]; /* +0x00: position (w = 1), angles, scale */
    CfeEvalVector initial[2];   /* +0x30: the position and angles the record starts from (func_0024B6D8) */
    int source[9];              /* +0x50 */
    unsigned char unmodeled_74[0x80 - 0x74];
} CfeEvalRecord;

/* Table entry (stride 0x230) of cfeEntryEvaluate: three vectors, a result word and ten source handles. */
typedef struct CfeEvalEntry {
    /* +0x000..0x0CF: the movie handle xglMovieInfoInit / xglMovieOpen work on (XS1 MovieInfo). */
    unsigned char unmodeled_00[0x40];
    short width;                /* +0x040: the movie size in pixels */
    short height;               /* +0x042 */
    unsigned char unmodeled_44[0x4C - 0x44];
    int bufferQuads;            /* +0x04C: quadwords per frame buffer (each is allocated 16 * n + 0x20 bytes) */
    unsigned char unmodeled_50[0x90 - 0x50];
    void *data;                 /* +0x090: the movie file, read into a GameResBufferAlloc block */
    int dataSize;               /* +0x094 */
    unsigned char unmodeled_98[2];
    unsigned char opened;       /* +0x09A */
    unsigned char unmodeled_9b[0xB0 - 0x9B];
    void *frameBuffers[2];      /* +0x0B0: two GameResBufferAlloc blocks */
    unsigned char unmodeled_b8[0xC8 - 0xB8];
    unsigned char firstBuffer;  /* +0x0C8: nonzero while frameBuffers[0] holds the frame to show, else [1] */
    unsigned char unmodeled_c9[0xD0 - 0xC9];
    char name[0x40];            /* +0x0D0: the movie file name */
    Vector4 evaluated[3];
    AlignedVector4 corners[4];  /* +0x140: the quad the entry is drawn with (cfeEvalEntriesInit) */
    unsigned char unmodeled_180[0x80];
    int result_int;
    int source[10];
    unsigned char unmodeled_22c[4];
} CfeEvalEntry;

struct XenoActor;
/* One sampled flag of a node (8 bytes, one per geometry of the actor's pack): the value and the track it is
 * sampled from. The key commands address the track as track[column]; the script passes column 0. */
typedef struct CfeFlagRecord {
    int value;
    int track[1];
} CfeFlagRecord;

/* One 0x1B0-byte entry of cfeEntries as its evaluation sees it: the actor it drives, the values it last set and
 * the tracks (handles, see cfeTrackSample) they come from. tu141.c (CfeNodeInit) and tu138.c see other parts. */
typedef struct CfeNode {
    unsigned char unmodeled_000[0xC0];
    struct XenoActor *actor;    /* +0x0C0 */
    unsigned char unmodeled_0c4[0xE0 - 0xC4];
    Vector4 position;           /* +0x0E0: the sampled offset, then in front of the actor */
    int flags;                  /* +0x0F0: CFE_NODE_SHOWN */
    int mode;                   /* +0x0F4 */
    int selector;               /* +0x0F8 */
    int slot;                   /* +0x0FC: the mode slot */
    float channelValue;         /* +0x100 */
    int playRange[2];           /* +0x104 */
    int attachSlot;             /* +0x10C: the cfePlugInSlots entry the actor hangs on, negative for none */
    int attachJoint;            /* +0x110 */
    float speed[2];             /* +0x114: of the motion states A and B */
    int dampOffset;             /* +0x11C: passed to cfeDampDirOffset every frame */
    int firstWord;              /* +0x120 */
    int drawMode;               /* +0x124 */
    int translucent;            /* +0x128 */
    int forceMode;              /* +0x12C: nonzero sets the mode again every time */
    int transformTrack[9];      /* +0x130: position, rotation and scale of the actor */
    int modeTrack;              /* +0x154 */
    int selectorTrack;          /* +0x158 */
    int slotTrack;              /* +0x15C */
    int channelTrack;           /* +0x160 */
    int playRangeTrack[2];      /* +0x164 */
    int attachSlotTrack;        /* +0x16C */
    int attachJointTrack;       /* +0x170 */
    int speedTrack[2];          /* +0x174 */
    int alphaTrack;             /* +0x17C */
    int dampOffsetTrack;        /* +0x180 */
    int firstWordTrack;         /* +0x184 */
    int drawModeTrack;          /* +0x188 */
    int translucentTrack;       /* +0x18C */
    int forceModeTrack;         /* +0x190 */
    int shownTrack;             /* +0x194 */
    int positionTrack[3];       /* +0x198 */
    struct CfeFlagRecord *records; /* +0x1A4: the flag records of the actor's list */
    unsigned char unmodeled_1a8[0x1B0 - 0x1A8];
} CfeNode;

/* CfeNode.flags: the entry's actor follows its position. */
#define CFE_NODE_SHOWN 0x1

/* Functions of src/main/cfeEvaluate.c that cfePlugIn.c calls. */
int cfeEvaluateNodes(int sample_param);
void cfeGroupTableEvaluate(int time);
void func_00248390(int time, int paused);
void cfeCommandTableEvaluate(int time);
void cfeEvaluateDirections(int time);
void cfeGlobalEvaluate(int sample_param);
int cfeEntryTableEvaluate(int sample_param);
void cfeSlotTableEvaluate(int sample_param);
void cfeSndSetTarget(int target);
void cfeUpdateEntries(int noPlay);

#endif /* MAIN_CFEEVALUATE_H */
