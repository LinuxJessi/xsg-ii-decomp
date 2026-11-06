#include "common.h"
#include "main/tu022.h"
#include "main/tu110.h"
#include "main/tu121.h"
#include "main/tu148.h"

INCLUDE_ASM("asm/main/nonmatchings/tu110", func_0022A560);

#include "shared.h"

int *func_0022A560(void);
void func_A8B490(int);

void func_0022A5A0(void) {
    func_A8B490(*func_0022A560());
}

INCLUDE_ASM("asm/main/nonmatchings/tu110", ptFindSlot);

/* Partial view of one 0x870-byte record of D_003400F8: only the leading
 * float is read by these functions. */
typedef struct ScaledEntry {
    float value;
    unsigned char unmodeled_04[0x86C];
} ScaledEntry;

/* Party state block ptState, as these functions use it:
 *   word 0      pointer to a clock object whose +4 word is a running time total (tu092 adds to it)
 *   words 1-10  member slots 0-9; each points at a record whose word 0 is the member id
 *   words 11-20 a second roster of the same kind (swapped by func_0022A7D8, searched by func_0022ADA0)
 *   byte 0x54   split mode (set to 1 by ptEnterSplit, cleared by ptLeaveSplit)
 *   byte 0x55   current front index (slots 0-6)
 *   byte 0x56   current back index (slot - 7, slots 7-9)
 *   byte 0x57-0x59 member counts; 0x5A selected index
 * tu092, tu100 and tu109 read the same object with other types. */
extern int *ptState[];
extern ScaledEntry D_003400F8[];
extern int uwamonoCurrentIndex;
extern unsigned char ptSplitMode[];

int *ptFindRecord(int id) {
    int *record;
    int *found;
    int misses;
    int **slot;
    int **table;

    found = 0;
    misses = 0;
    table = ptState;
    slot = table + 1;
    for (;;) {
        record = *slot;
        slot++;
        if (record != 0 && *record == id) {
            found = record;
            break;
        }
        misses++;
        if (misses >= 10) {
            break;
        }
    }
    return found;
}

INCLUDE_ASM("asm/main/nonmatchings/tu110", ptSwapSlots);

INCLUDE_ASM("asm/main/nonmatchings/tu110", func_0022A7D8);

INCLUDE_ASM("asm/main/nonmatchings/tu110", ptAddMember);

INCLUDE_ASM("asm/main/nonmatchings/tu110", ptRemoveMember);

INCLUDE_ASM("asm/main/nonmatchings/tu110", ptResetSlots);

INCLUDE_ASM("asm/main/nonmatchings/tu110", ptSetCurrentSlot);

int ptFindSlot();
void ptSetCurrentSlot(int);

void ptSelectMember(void) {
    int slot;

    slot = ptFindSlot();
    if (slot != -1) {
        ptSetCurrentSlot(slot);
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu110", ptGetCurrentSlot);

INCLUDE_ASM("asm/main/nonmatchings/tu110", ptGetCurrentId);

/* Index 0-6 of the front slot (words 1-7) holding the member that second-roster slot secondIndex
 * (word 11 + secondIndex) names, or -1 when no front slot holds it. */
int ptFindFrontSlot(int secondIndex) {
    int **frontSlot;
    int *secondRecord;
    int frontIndex;
    int found;

    found = -1;
    secondRecord = ptState[secondIndex + 11];
    frontSlot = &ptState[1];
    frontIndex = 0;
    for (;;) {
        if (secondRecord == 0) {
            frontIndex++;
        } else if (*frontSlot == secondRecord) {
            found = frontIndex;
            break;
        } else {
            frontIndex++;
        }
        frontSlot++;
        if (frontIndex >= 7) {
            break;
        }
    }
    return found;
}

INCLUDE_ASM("asm/main/nonmatchings/tu110", func_0022ADA0);

INCLUDE_ASM("asm/main/nonmatchings/tu110", func_0022AE10);

int uwamonoLoadCurrent();
int uwamonoStoreCurrent();
int func_0022AF98();

void ptRefreshCurrent(void) {
    int flag;

    flag = 0;
    if (uwamonoCurrentIndex != -1) {
        if (!(D_003400F8[uwamonoCurrentIndex].value < 5.0f)) {
            flag = 1;
        }
        flag = flag == ptSplitMode[0];
    }
    if (flag != 0) {
        uwamonoLoadCurrent();
    }
    func_0022AF98();
    if (flag != 0) {
        uwamonoStoreCurrent();
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu110", func_0022AF98);

void ptSwapSlots(int, int);
void ptAddMember();
void ptEnterSplit();
void ptLeaveSplit();
void func_0022BAF8();

void ptPlaceMember(int id) {
    int slot;

    /* ids 11 and up go to the back slot 7 in split mode; the others to front slot 0. */
    ptAddMember();
    if (id >= 0xB) {
        ptEnterSplit();
        slot = ptFindSlot(id);
        ptSwapSlots(7, slot);
        ptSetCurrentSlot(7);
    } else {
        ptLeaveSplit();
        slot = ptFindSlot(id);
        ptSwapSlots(0, slot);
        ptSetCurrentSlot(0);
    }
    func_0022BAF8();
    ptRefreshCurrent();
}

INCLUDE_ASM("asm/main/nonmatchings/tu110", func_0022B0B8);

INCLUDE_ASM("asm/main/nonmatchings/tu110", func_0022B1C8);

INCLUDE_ASM("asm/main/nonmatchings/tu110", ptEnterSplit);

INCLUDE_ASM("asm/main/nonmatchings/tu110", ptLeaveSplit);

void func_0022B1C8(void);

void func_0022B458(void)
{
    int i;
    int *slots;

    /* The three slots start 0x38 bytes into the table. */
    slots = ptState;
    slots = slots + 14;
    for (i = 0; i < 3; i++) {
        *slots = 0;
        slots++;
    }
    func_0022B1C8();
}

INCLUDE_ASM("asm/main/nonmatchings/tu110", func_0022B498);

INCLUDE_ASM("asm/main/nonmatchings/tu110", func_0022B4E8);

INCLUDE_ASM("asm/main/nonmatchings/tu110", func_0022B538);

INCLUDE_ASM("asm/main/nonmatchings/tu110", ptGroupCount);

INCLUDE_ASM("asm/main/nonmatchings/tu110", func_0022B5E0);

INCLUDE_ASM("asm/main/nonmatchings/tu110", ptRefresh);

INCLUDE_ASM("asm/main/nonmatchings/tu110", func_0022BA18);

INCLUDE_ASM("asm/main/nonmatchings/tu110", func_0022BAF8);

INCLUDE_ASM("asm/main/nonmatchings/tu110", saveChecksum);

INCLUDE_ASM("asm/main/nonmatchings/tu110", saveWriteImage);

INCLUDE_ASM("asm/main/nonmatchings/tu110", saveReadImage);

INCLUDE_ASM("asm/main/nonmatchings/tu110", func_0022CEE0);

void tyaSetDisplayOffset(s16, s16);
extern u8 PadData[];
extern u8 SaveData[];

/* Save-record fields read by padApplySaveSettings. Unmodeled spans keep the layout. */
typedef struct SaveRecordFields {
    u8 unmodeled_00[0x28];
    u8 flag;
    u8 unmodeled_29[0x32 - 0x29];
    s16 pairFirst;
    s16 pairSecond;
    u8 unmodeled_36[0x3E - 0x36];
    u8 copyByte;
} SaveRecordFields;

/* Pad-record fields written by padApplySaveSettings. */
typedef struct PadRecordFields {
    u8 unmodeled_00[0x56];
    u8 copiedValue;
    u8 unmodeled_57[0xD6 - 0x57];
    u8 copiedValueAlt;
} PadRecordFields;

void padApplySaveSettings(void)
{
    PadRecordFields *pad;
    SaveRecordFields *save;

    pad = (PadRecordFields *) PadData;
    save = (SaveRecordFields *) SaveData;
    pad->copiedValue = pad->copiedValueAlt = save->copyByte;
    func_001A4400(save->flag != 0);
    tyaSetDisplayOffset(save->pairFirst, save->pairSecond);
}

extern u8 menuMainTxdBuffer[];

int menuMainTxdEntry(int index) {
    return txdTableEntry(menuMainTxdBuffer, index);
}

extern u8 menuTutorialTxdBuffer[];

int menuTutorialTxdEntry(int index) {
    return txdTableEntry(menuTutorialTxdBuffer, index);
}

int *ptGetState(void) {
    return ptState;
}

extern int D_003EC60C[];

int ptGetMember(int index) {
    return D_003EC60C[index];
}

extern u8 ptStatusBlock[];

u8 *ptGetStatusBlock(void) {
    return ptStatusBlock;
}

extern int talkResult;
extern int D_002B5D98[];

int talk_0022D170(void) {
    int previous;

    previous = talkResult;
    talkResult = -1;
    if (msgWindowBusy() != 0) {
        return -1;
    }
    return (D_002B5D98[0] & 0x20000) ? -1 : previous;
}

int talk_0022D1C8(void) {
    talkResult = -1;
    return 1;
}
