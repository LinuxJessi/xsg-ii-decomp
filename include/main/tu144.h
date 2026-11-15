#ifndef INCLUDE_MAIN_TU144_H
#define INCLUDE_MAIN_TU144_H

/* The track key node, the key lists of src/main/tu144.c and the functions other files call. */

/*
 * A key of a time-keyed track: one 0x30-byte node of the pool in src/main/tu146.c (bit 0 of flags marks a
 * node in use there). The tracks are doubly linked through prev and next in increasing startTime; the list
 * helpers here keep that order. value is the 8.8 fixed-point key value; on rotation tracks it is an angle that
 * cfeChainUnwrapAngles makes continuous along the track. Each direction pair is a 2-D unit vector in 8.8 fixed
 * point, normalized by cfeEntryNormalizeDirs and reset to (1, 0) by cfeEntryResetDirs.
 */
typedef struct CfeTrackNode {
    int flags;                       /* 0x00 */
    int startTime;                   /* 0x04 */
    int value;                       /* 0x08: 8.8 fixed point */
    int firstDirection[2];           /* 0x0C */
    int secondDirection[2];          /* 0x14 */
    struct CfeTrackNode *prev;       /* 0x1C */
    struct CfeTrackNode *next;       /* 0x20 */
    unsigned char unmodeled_24[0x0C];
} CfeTrackNode;

/* An 8.8 fixed-point key value as a float. */
#define CFE_FIXED_TO_FLOAT(value) ((float) ((value) / 256) + (float) ((value) % 256) * (1.0f / 256))

/* A float as an 8.8 fixed-point value: the whole part and the 1/256 steps are converted apart. The value is
 * not parenthesized inside the casts, as in the original: for a difference a - b only a is truncated. */
#define CFE_FLOAT_TO_FIXED(value) ((int) (((int) value) * 256.0f + (int) (((value) - ((int) value)) * 256.0f)))

CfeTrackNode *func_0024F698(CfeTrackNode **list, int key, float value);
CfeTrackNode *func_0024F730(CfeTrackNode **list, int key, float value);
CfeTrackNode *func_0024F7C0(CfeTrackNode **list, int key, float value);
CfeTrackNode *func_0024F860(CfeTrackNode **list, int key, float value);

#endif /* INCLUDE_MAIN_TU144_H */
