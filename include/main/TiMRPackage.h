#ifndef INCLUDE_MAIN_TIMRPACKAGE_H
#define INCLUDE_MAIN_TIMRPACKAGE_H

/* Functions of src/main/TiMRPackage.c that other files call. */

typedef struct TiMRState TiMRState;

typedef struct TiMRPackage TiMRPackage;

typedef struct OffsetTable OffsetTable;

typedef struct TiOffsetEntry TiOffsetEntry;

typedef struct TiMRClamp TiMRClamp;

typedef struct TiVersionedBlock TiVersionedBlock;

/*
 * One state block, reached at +0x10 (state A) and +0x60 (state B) of TiMRPackage.
 * Its first word is a flag word: the advance functions test bits 0x2, 0x8 and
 * 0x20 of it and the table selectors set bit 0x40. `speed` (+4) is the rate the
 * advance functions multiply by the global step; `position` (+8) is the value
 * they accumulate and compare with the range. The block size of 0x50 follows
 * from the second block's offset. `value` (+0xC) is stored by tiMRStateSetValue and
 * passed by XenoActorSetChannelValue (and its B variant) to the channel blocks
 * of XenoActor at +0x650 and +0x6A0; no reader of it is found in this TU.
 */
struct TiMRState {
    int flags;      /* +0x0: flag word (tiWordSetBits/tiWordClearBits) */
    float speed;
    float position;
    float value;
    int rangeMin;   /* +0x10: the range of the advance (tiMRStateSetRange) */
    int rangeMax;   /* +0x14 */
    unsigned char unmodeled_18[0x38];
};

/*
 * Partial layout of the package. The first word, the init code at +0x04, the
 * two state blocks, the entry and table pointers of each state and the
 * pointer fields the per-frame update checks are evidenced. The memset in the
 * initializer covers the whole 0x100 bytes. The +0xB0 vector is set by
 * tiMRPackageSetVector (its meaning is not evidenced).
 */
struct TiMRPackage {
    int first_word;
    int init_code;
    unsigned char unmodeled_08[8];
    TiMRState state_a;
    TiMRState state_b;
    float x;
    float y;
    float z;
    unsigned char unmodeled_bc[4];
    TiOffsetEntry *entryA;    /* +0xC0: entry of state A; the update checks its version float as pAnimData (xac) */
    OffsetTable *entryTableA; /* +0xC4: offset table of state A (tiMRPackageSelectEntryA) */
    TiOffsetEntry *entryB;    /* +0xC8: entry of state B; the update checks its version float as pFaceAnimData (fac) */
    OffsetTable *entryTableB; /* +0xCC: offset table of state B (tiMRPackageSelectEntryB) */
    TiVersionedBlock *hrcData; /* +0xD0: pHrcData, version float at +4 */
    TiMRClamp *clampRange;    /* +0xD4: two shorts at +8 and +A bound state A */
    int setting_d8;
    int localMat;             /* +0xDC: pLocalMat */
    int globalMat;            /* +0xE0: pGlobalMat */
    int setting_e4;
    int setting_e8;
    unsigned char unmodeled_ec[0x14];
};

/*
 * Counted table of entries: the entry count sits at +8 and a table of byte
 * offsets, relative to the table itself, starts at +0xC.
 */
struct OffsetTable {
    unsigned char unmodeled_00[8];
    unsigned int count;      /* +8 */
    int entryOffsets[1];     /* +0xC: byte offset of entry i from the table */
};

/*
 * One entry of an offset table, bound to a state as its animation data: the
 * per-frame update checks its version float at +4; its float range is at +8
 * (min) and +C (max).
 */
struct TiOffsetEntry {
    unsigned char unmodeled_00[4];
    float version;   /* +4 */
    float rangeMin;  /* +8 */
    float rangeMax;  /* +C */
};

/* The clamp record at +0xD4 of the package: two shorts bound state A (lower at +8, upper at +A). */
struct TiMRClamp {
    unsigned char unmodeled_00[8];
    short lowerLimit;  /* +8 */
    short upperLimit;  /* +A */
};

/* A block whose float version sits at +4: the HRC data and the animation data the update checks. */
struct TiVersionedBlock {
    unsigned char unmodeled_00[4];
    float version;  /* +4 */
};

void tiWordClearBits(int *word, int bits);
void tiWordSetBits(int *word, int bits);

#endif /* INCLUDE_MAIN_TIMRPACKAGE_H */
