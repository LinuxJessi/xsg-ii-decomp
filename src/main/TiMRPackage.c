#include "common.h"
#include "shared.h"
#include "main/TiMRPackage.h"
#include "main/tu026.h"

typedef struct TiMRState TiMRState;

typedef struct TiMRPackage TiMRPackage;

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
    unsigned char unmodeled_00[4];
    float speed;
    float position;
    float value;
    unsigned char unmodeled_10[0x40];
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
    int entryA;               /* +0xC0: entry of state A; the update checks its version float as pAnimData (xac) */
    int entryTableA;          /* +0xC4: offset table of state A (see tu253.c) */
    int entryB;               /* +0xC8: entry of state B; the update checks its version float as pFaceAnimData (fac) */
    int entryTableB;          /* +0xCC: offset table of state B (see tu253.c) */
    int hrcData;              /* +0xD0: pHrcData, version float at +4 */
    int clampRange;           /* +0xD4: two shorts at +8 and +A bound state A */
    int setting_d8;
    int localMat;             /* +0xDC: pLocalMat */
    int globalMat;            /* +0xE0: pGlobalMat */
    int setting_e4;
    int setting_e8;
    unsigned char unmodeled_ec[0x14];
};

extern unsigned char D_0068F480[];
extern unsigned char tiMRPackageSourceFile[];
extern unsigned char D_0068F4A8[];
extern unsigned char D_0068F4B0[];
void tiWordZero(int *word);
void tiMRPackageSetEntryA();
void tiMRPackageSetHrcData(TiMRPackage *pMRPackage, int value);
void tiMRPackageFillNodes();
void memset();

/* Sets the bits of a flag word. */
void tiWordSetBits(int *word, int bits) {
    *word |= bits;
}

/* Clears the bits of a flag word. */
void tiWordClearBits(int *word, int bits) {
    *word &= ~bits;
}

/* Zeroes one word; the init code passes the flag word of each state. */
void tiWordZero(int *word) {
    *word = 0;
}

void tiMRPackageSetLocalMat(TiMRPackage *pMRPackage, int value);
void tiMRPackageSetGlobalMat(TiMRPackage *pMRPackage, int value);
void func_00291F80(TiMRPackage *pMRPackage, int value);
void func_00291FD0(TiMRPackage *pMRPackage, int value);
void tiMRPackageSetClampRange(TiMRPackage *pMRPackage, int value);
void func_002922F0(TiMRPackage *pMRPackage, int value);
void tiMRStateSetSpeed(TiMRState *pState, float value);
void tiMRStateSetPosition(TiMRState *pState, float value);
void tiMRStateSetValue(TiMRState *pState, float value);
void tiMRPackageSetFirstWord(TiMRPackage *pMRPackage, int value);
void tiMRPackageSetVector(TiMRPackage *pMRPackage, float x, float y, float z);

/* Package initializer: clears everything, sets speed 1.0 and position 0.0 on both states, vector (1,1,1). */
void tiMRPackageInit(TiMRPackage *pMRPackage) {
    TiMRState *pStateA;
    TiMRState *pStateB;

    pStateA = &pMRPackage->state_a;
    pStateB = &pMRPackage->state_b;
    if (pMRPackage == 0) {
        assertFailed(D_0068F480, tiMRPackageSourceFile, 38);
    }
    memset(pMRPackage, 0, sizeof(TiMRPackage));
    tiWordZero(pStateA);
    tiWordZero(pStateB);
    tiMRStateSetPosition(pStateA, 0.0f);
    tiMRStateSetPosition(pStateB, 0.0f);
    tiMRStateSetValue(pStateA, 0.0f);
    tiMRStateSetValue(pStateB, 0.0f);
    tiMRStateSetSpeed(pStateA, 1.0f);
    tiMRStateSetSpeed(pStateB, 1.0f);
    tiMRPackageSetFirstWord(pMRPackage, 1);
    tiMRPackageSetVector(pMRPackage, 1.0f, 1.0f, 1.0f);
    tiMRPackageSetHrcData(pMRPackage, 0);
    tiMRPackageSetEntryA(pMRPackage, 0);
    tiMRPackageSetLocalMat(pMRPackage, 0);
    tiMRPackageSetGlobalMat(pMRPackage, 0);
    func_00291F80(pMRPackage, 0);
    func_00291FD0(pMRPackage, 0);
    tiMRPackageSetClampRange(pMRPackage, 0);
    func_002922F0(pMRPackage, 0);
    tiMRPackageFillNodes(pMRPackage, 0);
    pMRPackage->init_code = 0x1D;
}

void tiMRPackageSetLocalMat(TiMRPackage *pMRPackage, int value) {
    if (pMRPackage == 0) {
        assertFailed(&D_0068F480, &tiMRPackageSourceFile, 70);
    }
    pMRPackage->localMat = value;
}

void tiMRPackageSetGlobalMat(TiMRPackage *pMRPackage, int value) {
    if (pMRPackage == 0) {
        assertFailed(&D_0068F480, &tiMRPackageSourceFile, 80);
    }
    pMRPackage->globalMat = value;
}

void func_00291F80(TiMRPackage *pMRPackage, int value) {
    if (pMRPackage == 0) {
        assertFailed(&D_0068F480, &tiMRPackageSourceFile, 90);
    }
    pMRPackage->setting_e4 = value;
}

void func_00291FD0(TiMRPackage *pMRPackage, int value) {
    if (pMRPackage == 0) {
        assertFailed(&D_0068F480, &tiMRPackageSourceFile, 100);
    }
    pMRPackage->setting_e8 = value;
}

/*
 * Partial view of a state block: the range of its advance, written by
 * tiMRStateSetRange. Overlays TiMRState at +0x10 (state A) or +0x60 (state B).
 * The advance functions compare the position with rangeMin (+0x10) and
 * rangeMax (+0x14) as the bounds of the loop.
 */
typedef struct TiMRStateRange TiMRStateRange;

struct TiMRStateRange {
    unsigned char unmodeled_00[0x10];
    int rangeMin;  /* +0x10 */
    int rangeMax;  /* +0x14 */
};

/* Stores the integer range of a state (min at +0x10, max at +0x14). */
void tiMRStateSetRange(TiMRStateRange *record, int rangeMin, int rangeMax) {
    record->rangeMin = rangeMin;
    record->rangeMax = rangeMax;
}

/*
 * Binds entry A: a null entry clears +0xC0; an entry whose float range at +8
 * and +C is valid (min <= max) is stored at +0xC0 and its range copied into
 * state A (tiMRStateSetRange). Same validity test as tiMRPackageIsEntryValidA.
 */
INCLUDE_ASM("asm/main/nonmatchings/TiMRPackage", tiMRPackageSetEntryA);

void tiMRPackageSetTableA(TiMRPackage *pMRPackage, int value) {
    if (pMRPackage == 0) {
        assertFailed(&D_0068F480, &tiMRPackageSourceFile, 137);
    }
    pMRPackage->entryTableA = value;
}

/* Binds entry B the same way as tiMRPackageSetEntryA, into +0xC8 and state B at +0x60. */
INCLUDE_ASM("asm/main/nonmatchings/TiMRPackage", tiMRPackageSetEntryB);

/*
 * Returns 1 when entry `index` of offset table A exists and its range is valid
 * (float at +8 <= float at +C); 0 otherwise. The second argument is not read.
 */
INCLUDE_ASM("asm/main/nonmatchings/TiMRPackage", tiMRPackageIsEntryValidA);

void tiMRPackageSetTableB(TiMRPackage *pMRPackage, int value) {
    if (pMRPackage == 0) {
        assertFailed(&D_0068F480, &tiMRPackageSourceFile, 191);
    }
    pMRPackage->entryTableB = value;
}

void tiMRPackageSetHrcData(TiMRPackage *pMRPackage, int value) {
    if (pMRPackage == 0) {
        assertFailed(&D_0068F480, &tiMRPackageSourceFile, 201);
    }
    pMRPackage->hrcData = value;
}

void tiMRPackageSetClampRange(TiMRPackage *pMRPackage, int value) {
    if (pMRPackage == 0) {
        assertFailed(&D_0068F480, &tiMRPackageSourceFile, 211);
    }
    pMRPackage->clampRange = value;
}

void func_002922F0(TiMRPackage *pMRPackage, int value) {
    if (pMRPackage == 0) {
        assertFailed(&D_0068F480, &tiMRPackageSourceFile, 221);
    }
    pMRPackage->setting_d8 = value;
}

/* Stores the rate of a state (speed, +4). */
void tiMRStateSetSpeed(TiMRState *pState, float value) {
    if (pState == 0) {
        assertFailed(&D_0068F4A8, &tiMRPackageSourceFile, 231);
    }
    pState->speed = value;
}

/* Stores the position of a state (+8), the value the advance functions accumulate. */
void tiMRStateSetPosition(TiMRState *pState, float value) {
    if (pState == 0) {
        assertFailed(&D_0068F4A8, &tiMRPackageSourceFile, 241);
    }
    pState->position = value;
}

void tiMRStateSetValue(TiMRState *pState, float value) {
    if (pState == 0) {
        assertFailed(&D_0068F4A8, &tiMRPackageSourceFile, 251);
    }
    pState->value = value;
}

/* Stores the first word of the package (set to 1 by the initializer and by the per-frame update). */
void tiMRPackageSetFirstWord(TiMRPackage *pMRPackage, int value) {
    if (pMRPackage == 0) {
        assertFailed(&D_0068F4B0, &tiMRPackageSourceFile, 261);
    }
    pMRPackage->first_word = value;
}

/* Sets x, y and z of the package to one value. */
void tiMRPackageSetVectorUniform(TiMRPackage *pMRPackage, float value) {
    pMRPackage->z = value;
    pMRPackage->y = value;
    pMRPackage->x = value;
}

/* Sets the package vector x, y, z. */
void tiMRPackageSetVector(TiMRPackage *pMRPackage, float x, float y, float z) {
    pMRPackage->x = x;
    pMRPackage->y = y;
    pMRPackage->z = z;
}

/*
 * Advances state A: position += speed times the global step; a position past
 * rangeMax wraps to rangeMin and a position below rangeMin jumps to rangeMax,
 * each setting flag 0x8 and clearing 0x2; a clamp record at +0xD4 then sets or
 * clears 0x8 again. Called from XenoActorUpdateTree.
 */
INCLUDE_ASM("asm/main/nonmatchings/TiMRPackage", tiMRPackageAdvanceA);

/* Advances state A like tiMRPackageAdvanceA; clears flag 0x2 only when it is set. */
INCLUDE_ASM("asm/main/nonmatchings/TiMRPackage", func_00292638);

/* Advances state B: position += speed times the global step, with the same wrap at its range. */
INCLUDE_ASM("asm/main/nonmatchings/TiMRPackage", tiMRPackageAdvanceB);

/* Advances state B like tiMRPackageAdvanceB; clears flag 0x2 only when it is set. */
INCLUDE_ASM("asm/main/nonmatchings/TiMRPackage", func_002928C8);

/*
 * Per-frame update of the package. Checks the matrices (pLocalMat +0xDC,
 * pGlobalMat +0xE0), clears flag 0x20 on both states, checks the version
 * floats of pHrcData (+0xD0, xhr), pAnimData (+0xC0, xac) and pFaceAnimData
 * (+0xC8, fac), runs the animation updates and ends by setting first_word to 1.
 */
INCLUDE_ASM("asm/main/nonmatchings/TiMRPackage", tiMRPackageUpdate);
