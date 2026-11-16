#include "common.h"
#include "shared.h"
#include "main/pad.h"
#include "main/tu108.h"
#include "main/tu022.h"
#include "main/tu051.h"
#include "main/tu121.h"
#include "main/tu148.h"
#include "main/xgl_cd.h"
#include "main/xgl_flags.h"

extern unsigned int D_003EC458[];
extern unsigned int D_003EC468[];
extern unsigned int D_003EC490[];
extern unsigned int D_003EC4A0[];
extern unsigned char D_003EC4C0[];
extern unsigned int D_003EC474[];
extern unsigned int D_003EC60C[];
extern unsigned int D_003EC614[];
extern unsigned int D_003EC618[];
extern unsigned int D_003EC624[];
extern unsigned int D_003EC634[];

/* The settings of SaveData used here (tu101.c menuDrawConfig uses the same type). dolby_digital
 * and caption_all are game_movie.c's names, cursor_memory and cursor_memory_alt tu101.c's. */
typedef struct ConfigSaveSettings {
    unsigned char unmodeled_00[0x28];
    unsigned char soundMode;            /* +0x28: 0..2 (tu100.c's option page cycles it); the sound
                                         * function func_001A4400 (tu022.c) gets soundMode != 0 */
    unsigned char dolby_digital;        /* +0x29 */
    unsigned char unmodeled_2a[0x32 - 0x2A];
    short displayOffsetX;               /* +0x32: the horizontal argument of tyaSetDisplayOffset */
    short displayOffsetY;               /* +0x34: the vertical argument */
    unsigned char unmodeled_36[0x3A - 0x36];
    unsigned char caption_all;          /* +0x3A */
    unsigned char unmodeled_3b[0x3E - 0x3B];
    unsigned char actuatorMode;         /* +0x3E: copied to both pads' PadRecord.actuatorMode */
    unsigned char unmodeled_3f[0x46 - 0x3F];
    unsigned char cursor_memory;        /* +0x46 */
    unsigned char cursor_memory_alt;    /* +0x47 */
} ConfigSaveSettings;

extern ConfigSaveSettings SaveData;
void tyaDisplaySetting(int framebuffer);

void saveDataSetDefaults(void) {
    SaveData.soundMode = 1;
    SaveData.caption_all = 2;
    SaveData.cursor_memory = 0;
    SaveData.cursor_memory_alt = 0;
    SaveData.actuatorMode = 0;
    SaveData.dolby_digital = 0;
    tyaDisplaySetting(1);
}

/* signed char: ptReset tests it with lb. */
extern signed char menuLoadPending;

/* Sets the load-pending flag that ptReset reads. */
void menuRequestLoad(void) {
    menuLoadPending = 1;
}

extern char menuMainTxdPath[];
extern char menuTutorialTxdPath[];
extern unsigned char menuMainTxdBuffer[];
extern unsigned char menuTutorialTxdBuffer[];

/* Reads yosikawa\MainMenu.txd and yosikawa\Tutorial.txd from the disc into their buffers, then polls
 * xglCdServiceIfPending until it returns 0. */
void menuLoadTxds(void) {
    xglCdReadFile(menuMainTxdPath, menuMainTxdBuffer, 1, 0);
    xglCdReadFile(menuTutorialTxdPath, menuTutorialTxdBuffer, 1, 0);
    do {
    } while (xglCdServiceIfPending() != 0);
}

/* The status record of one party member (D_006A5A80[index]): two texts of MainMenu.txd and the
 * member index, which saveWriteImage saves as memberIndex + 1. */
struct PartyMemberStatus {
    char *text;                         /* +0x00: the txd entry of the member index */
    char *textAlt;                      /* +0x04: the txd entry of the member index + 17 */
    unsigned char memberIndex;          /* +0x08 */
    unsigned char unmodeled_09[3];
};

/* One 0x40-byte slot record of D_0061C400 (ov01 tu027.c); tu100.c and tu101.c read it as
 * ControlLimits, tu109.c as SelectionCounter. No member is accessed here. */
struct SlotRecord {
    unsigned char unmodeled_00[0x40];
};

/* One 0x2B-byte record of D_006A5880 (tu110.c's SaveMemberName, copied whole into the save
 * image; tu100.c, tu101.c and tu109.c read it as ControlBuffer, and the member names are
 * tu109.c's): the taken bits of one member (header, cells), the highest open group, the taken
 * count and the entries tu109.c's selectionFindIndex searches. ptReset clears all of it. */
struct SaveMemberName {
    unsigned char header[4];            /* +0x00 */
    unsigned char cells[4][8];          /* +0x04 */
    unsigned char openGroup;            /* +0x24 */
    unsigned char takenCount;           /* +0x25 */
    unsigned char entries[4];           /* +0x26 */
    unsigned char entry_count;          /* +0x2A */
};

/* The clock reading xglClockRead fills (xgl_cd.c's XglClock). */
typedef struct XglClock {
    unsigned char status;
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char day;
    unsigned char month;
    unsigned short year;
} XglClock;

static struct PartyMemberStatus D_006A5A80[10];
extern struct SlotRecord D_0061C400[];
extern struct SaveMemberName D_006A5880[];
extern PadRecord PadData[2];
void selectionFlagTableInit(void);
void saveTextStateReset(void);
void tyaSetDisplayOffset(int horizontal, int vertical);
void xglClockRead(XglClock *clock);
unsigned int xglClockDayTime2UInt(XglClock *clock_time);
void ptResetStatusBlock(void);
void ptAddMember(int handle);
void ptSetCurrentSlot(int slot);
void ptRefresh(void);
void selectionBlockClear(void);

/* One 16-byte record of the 10-entry table ptMemberRecords, keyed by its id. */
struct IdSlot {
    int id;
    struct PartyMemberStatus *status;   /* +0x04: D_006A5A80[index] */
    struct SlotRecord *slotRecord;      /* +0x08: the slot record of the member's id */
    struct SaveMemberName *buffer;      /* +0x0C: D_006A5880[index], the member's taken bits */
};

static struct IdSlot ptMemberRecords[10];

/* The object attached to the current uwamono (D_006A5A30, the record ptState.progress points at;
 * tu109.c reads it as SelectionBlock, tu110.c as PartyProgress). */
struct UwamonoContext {
    unsigned short progressId;          /* +0x00 */
    unsigned int playTime;              /* +0x04: xglClockDayTime2UInt reading at reset */
    int mapId;                          /* +0x08: the map it was attached on, -1 while detached */
    unsigned char unmodeled_0C[4];
    DoublewordAlignedVector4 position;  /* +0x10: the actor position copied on attach */
    unsigned char savedSplitMode;       /* +0x20: ptState.splitMode when it was attached */
    unsigned char mapFlag;              /* +0x21 */
};

/* The global binding ptState (tu110.c's PartyState, whose member names are used here): the
 * attached object, the member slots and roster that ptReset clears, the split mode (set by
 * ptEnterSplit, cleared by ptLeaveSplit), the current front and back slots and the counts. */
struct UwamonoBinding {
    struct UwamonoContext *progress;
    struct IdSlot *slots[10];           /* +0x04 */
    struct IdSlot *roster[10];          /* +0x2C */
    unsigned char splitMode;            /* +0x54 */
    unsigned char frontIndex;           /* +0x55 */
    unsigned char backIndex;            /* +0x56 */
    unsigned char counts[3];            /* +0x57 */
};

/* gameWork as used here (each TU keeps its own view): the control word and the current map. */
struct PartyGameWork {
    unsigned char unmodeled_00[8];
    int control;                        /* +0x08: CONTROL_* bits (main/tu051.h) */
    unsigned char unmodeled_0C[0x18 - 0x0C];
    int mapId;                          /* +0x18 */
};

/* gameWork.control bit set while ptState.progress is attached to a uwamono (tu067.c's script
 * native reads it back as a 0/1 value). */
#define CONTROL_PARTY_ATTACHED 0x800

extern struct UwamonoBinding ptState;
extern struct UwamonoContext D_006A5A30;
extern struct PartyGameWork gameWork;
void uwamonoSetTransition(signed char mode, int state, int stateArg, int timer);
void ptEnterSplit(void);
void ptLeaveSplit(void);

/* One 0x870-byte record of uwamonoSlotTable (as elevator.c and tu073.c model it); only the actor
 * pointer is read here. */
struct UwamonoSlot {
    struct XenoActor *actor;            /* +0x00 */
    struct UwamonoState *state;         /* +0x04 */
    unsigned char unmodeled_08[0x870 - 0x08];
};

extern struct UwamonoSlot uwamonoSlotTable[];
extern int D_003EBA88[8];

/* Resets the party. On a pending load it first calls selectionFlagTableInit and
 * saveTextStateReset, writes the default settings (saveDataSetDefaults), applies them to the
 * pads, func_001A4400 and the display offset as padApplySaveSettings does, and reads the menu
 * text tables (menuLoadTxds). Then it rebuilds the ten member records, clears the party state,
 * stamps the play time, adds the default members, makes slot 0 current and clears the
 * selection block and the pending flag. */
void ptReset(void) {
    XglClock now;
    int i;
    int j;
    int k;

    if (menuLoadPending) {
        selectionFlagTableInit();
        saveTextStateReset();
        saveDataSetDefaults();
        PadData[0].actuatorMode = PadData[1].actuatorMode = SaveData.actuatorMode;
        func_001A4400(SaveData.soundMode != 0);
        tyaSetDisplayOffset(SaveData.displayOffsetX, SaveData.displayOffsetY);
        menuLoadTxds();
    }
    D_006A5A30.mapId = -1;
    D_006A5A30.progressId = 0;
    D_006A5A30.mapFlag = 0;
    for (i = 0; i < 10; i++) {
        D_006A5A80[i].text = (char *)txdTableEntry((int *)menuMainTxdBuffer, i);
        D_006A5A80[i].textAlt = (char *)txdTableEntry((int *)menuMainTxdBuffer, i + 17);
        D_006A5A80[i].memberIndex = i;
        for (j = 0; j < 4; j++) {
            D_006A5880[i].header[j] = 0;
            for (k = 7; k >= 0; k--) {
                D_006A5880[i].cells[j][k] = 0;
            }
            D_006A5880[i].entries[j] = 0;
        }
        D_006A5880[i].openGroup = 0;
        D_006A5880[i].takenCount = 0;
        D_006A5880[i].entry_count = 0;
        /* Members 0-6 are ids 1-7 on slot records 0-6, members 7-9 ids 11-13 on records 10-12. */
        if (i >= 7 && i < 10) {
            ptMemberRecords[i].id = i + 4;
            ptMemberRecords[i].slotRecord = &D_0061C400[i + 3];
        } else {
            ptMemberRecords[i].slotRecord = &D_0061C400[i];
            ptMemberRecords[i].id = i + 1;
        }
        ptMemberRecords[i].status = &D_006A5A80[i];
        ptMemberRecords[i].buffer = &D_006A5880[i];
        ptState.slots[i] = 0;
        ptState.roster[i] = 0;
    }
    ptState.progress = &D_006A5A30;
    ptState.splitMode = 0;
    ptState.frontIndex = 0;
    ptState.backIndex = 0;
    ptState.counts[0] = 0;
    ptState.counts[1] = 0;
    ptState.counts[2] = 0;
    xglClockRead(&now);
    /* The same record as D_006A5A30, reloaded through the pointer just stored: written as
     * D_006A5A30.playTime the store goes to the symbol address and the bytes change. */
    ptState.progress->playTime = xglClockDayTime2UInt(&now);
    ptResetStatusBlock();
    ptAddMember(1);
    ptAddMember(4);
    ptAddMember(5);
    ptAddMember(2);
    ptAddMember(3);
    ptAddMember(6);
    ptAddMember(7);
    ptAddMember(11);
    ptAddMember(12);
    ptAddMember(13);
    ptSetCurrentSlot(0);
    ptRefresh();
    selectionBlockClear();
    menuLoadPending = 0;
}

struct IdSlot *ptMemberRecordAt(int index) {
    return &ptMemberRecords[index];
}

/* Returns the first of the 10 slots whose id matches, or NULL. */
struct IdSlot *ptMemberRecordFindById(int id) {
    struct IdSlot *found = 0;
    int entry_index;

    for (entry_index = 0; entry_index < 10; entry_index++) {
        if (ptMemberRecords[entry_index].id == id) {
            found = &ptMemberRecords[entry_index];
            break;
        }
    }
    return found;
}

/* Attaches ptState.progress to the current uwamono: saves the current map, the actor's position
 * and the split mode, enters or leaves split mode as runSplit asks and starts the transition to
 * the map of kind (D_003EBA88). */
void uwamonoAttachCurrent(unsigned int kind, int runSplit) {
    struct UwamonoContext *progress = ptState.progress;

    if (progress->mapId == -1 && uwamonoCurrentIndex != -1 && kind < 8) {
        progress->mapId = gameWork.mapId;
        progress->position = uwamonoSlotTable[uwamonoCurrentIndex].actor->position;
        progress->savedSplitMode = ptState.splitMode;
        if (runSplit && ptState.splitMode == 0) {
            ptEnterSplit();
        } else if (!runSplit && ptState.splitMode != 0) {
            ptLeaveSplit();
        }
        if (!runSplit && kind == 7) {
            xglFlagsSet1(0x2F0, 1);
        }
        gameWork.control |= CONTROL_PARTY_ATTACHED;
        uwamonoSetTransition(0, D_003EBA88[kind], kind == 6 ? 4 : 201, 0);
        msgResetAll();
    }
}

/* Detaches ptState.progress: restores the split mode saved on attach, clears flag 0x2F0 and
 * CONTROL_PARTY_ATTACHED, starts the transition back to the saved map and marks it detached. */
void uwamonoDetachCurrent(void) {
    if (ptState.progress->mapId != -1) {
        if (ptState.progress->savedSplitMode != 0) {
            if (ptState.splitMode == 0) {
                ptEnterSplit();
            }
        } else if (ptState.splitMode != 0) {
            ptLeaveSplit();
        }
        xglFlagsSet1(0x2F0, 0);
        gameWork.control &= ~CONTROL_PARTY_ATTACHED;
        uwamonoSetTransition(0, ptState.progress->mapId, -2, 0);
        msgResetAll();
        ptState.progress->mapId = -1;
    }
}

unsigned char D_0069D768[3] = { 0x02, 0x06, 0x07 };

signed char menuLoadPending = 1;

unsigned char selectionWindowCount = 0;

unsigned char selectionPlainCount = 0;

char menuMainTxdPath[24] = "yosikawa\\MainMenu.txd";

char menuTutorialTxdPath[24] = "yosikawa\\Tutorial.txd";

int D_003EBA88[8] = {
    1010, 2110, 2210, 3120, 3410, 4310, 4610, 5010
};

unsigned char D_003EBAA8[1408] = {
    0, 0, 0, 0, 2, 2, 0, 1,
    1, 1, 1, 0, 0, 0, 68, 0,
    0, 0, 0, 0, 2, 1, 0, 1,
    2, 1, 1, 0, 0, 0, 68, 0,
    0, 0, 0, 0, 1, 1, 0, 1,
    3, 1, 1, 0, 0, 0, 68, 0,
    0, 0, 0, 0, 1, 2, 0, 1,
    4, 1, 1, 0, 0, 0, 68, 0,
    0, 0, 0, 0, 3, 0, 0, 1,
    10, 1, 1, 0, 2, 0, 68, 0,
    0, 0, 0, 0, 3, 0, 0, 1,
    9, 1, 1, 2, 0, 0, 68, 0,
    0, 0, 0, 0, 3, 3, 0, 1,
    12, 1, 1, 2, 0, 0, 68, 0,
    0, 0, 0, 0, 2, 2, 0, 1,
    1, 1, 1, 0, 0, 0, 4, 0,
    0, 0, 0, 0, 2, 1, 0, 1,
    2, 1, 1, 0, 0, 0, 4, 0,
    0, 0, 0, 0, 1, 1, 0, 1,
    3, 1, 1, 0, 0, 0, 4, 0,
    0, 0, 0, 0, 1, 2, 0, 1,
    4, 1, 1, 0, 0, 0, 4, 0,
    0, 0, 0, 0, 3, 0, 0, 1,
    10, 1, 1, 0, 1, 0, 4, 0,
    0, 0, 0, 0, 3, 0, 0, 1,
    7, 1, 1, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 3, 3, 0, 1,
    8, 1, 1, 3, 0, 0, 1, 0,
    0, 0, 0, 0, 3, 0, 0, 1,
    11, 2, 2, 0, 0, 0, 32, 0,
    0, 0, 0, 0, 3, 3, 0, 1,
    11, 2, 2, 0, 0, 0, 128, 0,
    0, 0, 0, 0, 3, 3, 3, 1,
    11, 2, 2, 0, 0, 0, 32, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 2, 2, 0, 1,
    1, 1, 1, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 2, 1, 0, 1,
    2, 1, 1, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 1, 1, 0, 1,
    3, 1, 1, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 1, 2, 0, 1,
    4, 1, 1, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 3, 0, 0, 1,
    10, 1, 1, 0, 1, 0, 4, 0,
    0, 0, 0, 0, 3, 0, 0, 1,
    7, 1, 1, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 3, 3, 0, 1,
    8, 1, 1, 3, 0, 0, 1, 0,
    0, 0, 0, 0, 2, 2, 0, 1,
    1, 1, 1, 0, 0, 0, 12, 0,
    0, 0, 0, 0, 2, 1, 0, 1,
    2, 1, 1, 0, 0, 0, 12, 0,
    0, 0, 0, 0, 1, 1, 0, 1,
    3, 1, 1, 0, 0, 0, 12, 0,
    0, 0, 0, 0, 1, 2, 0, 1,
    4, 1, 1, 0, 0, 0, 12, 0,
    0, 0, 0, 0, 3, 0, 0, 1,
    10, 1, 1, 0, 2, 0, 12, 0,
    0, 0, 0, 0, 3, 0, 0, 1,
    9, 1, 1, 2, 0, 0, 9, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 3, 0, 0, 1,
    11, 2, 2, 0, 0, 0, 2, 0,
    0, 0, 0, 0, 3, 3, 0, 1,
    11, 2, 2, 0, 0, 0, 2, 0,
    0, 0, 0, 0, 3, 3, 3, 1,
    11, 2, 2, 0, 0, 0, 2, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 2, 2, 0, 1,
    1, 1, 2, 0, 0, 0, 2, 0,
    0, 0, 0, 0, 2, 1, 0, 1,
    2, 1, 2, 0, 0, 0, 2, 0,
    0, 0, 0, 0, 1, 1, 0, 1,
    3, 1, 2, 0, 0, 0, 2, 0,
    0, 0, 0, 0, 1, 2, 0, 1,
    4, 1, 2, 0, 0, 0, 2, 0,
    0, 0, 0, 0, 3, 0, 0, 1,
    10, 1, 2, 0, 0, 0, 2, 0,
    0, 0, 0, 0, 3, 0, 0, 1,
    7, 1, 2, 0, 0, 0, 2, 0,
    0, 0, 0, 0, 3, 3, 0, 1,
    8, 1, 2, 3, 0, 0, 2, 0,
    0, 0, 0, 0, 2, 0, 0, 1,
    0, 1, 1, 0, 0, 0, 4, 0,
    0, 0, 0, 0, 1, 0, 0, 1,
    0, 2, 2, 0, 0, 0, 128, 0,
    0, 0, 0, 0, 3, 0, 0, 1,
    0, 2, 2, 0, 0, 0, 128, 0,
    0, 0, 0, 0, 1, 29, 0, 1,
    0, 1, 2, 0, 0, 0, 4, 0,
    0, 0, 0, 0, 2, 4, 0, 1,
    0, 1, 2, 0, 0, 0, 36, 0,
    0, 0, 0, 0, 2, 8, 0, 1,
    0, 1, 1, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 2, 1, 0, 1,
    0, 1, 1, 0, 0, 0, 68, 0,
    0, 0, 0, 0, 2, 16, 0, 1,
    0, 2, 2, 0, 0, 0, 8, 0,
    0, 0, 0, 0, 3, 4, 1, 2,
    0, 2, 2, 0, 0, 0, 128, 0,
    0, 0, 0, 0, 3, 8, 1, 1,
    0, 1, 2, 0, 0, 0, 65, 0,
    0, 0, 0, 0, 2, 0, 0, 1,
    0, 1, 1, 0, 0, 0, 4, 0,
    0, 0, 0, 0, 1, 0, 0, 1,
    0, 2, 2, 0, 0, 0, 2, 0,
    0, 0, 0, 0, 3, 0, 0, 1,
    0, 2, 2, 0, 0, 0, 2, 0,
    0, 0, 0, 0, 1, 29, 0, 2,
    0, 2, 2, 0, 0, 0, 2, 0,
    0, 0, 0, 0, 2, 4, 0, 1,
    0, 2, 2, 0, 0, 0, 2, 0,
    0, 0, 0, 0, 2, 8, 0, 1,
    0, 2, 2, 0, 3, 0, 66, 0,
    0, 0, 0, 0, 2, 1, 0, 1,
    0, 1, 1, 0, 0, 0, 17, 0,
    0, 0, 0, 0, 2, 16, 0, 1,
    0, 1, 1, 0, 0, 0, 12, 0,
    0, 0, 0, 0, 3, 29, 1, 2,
    0, 2, 2, 0, 0, 0, 128, 0,
    0, 0, 0, 0, 3, 16, 1, 1,
    0, 2, 2, 0, 0, 0, 8, 0,
    0, 0, 0, 0, 2, 0, 0, 1,
    0, 1, 1, 0, 0, 0, 4, 0,
    0, 0, 0, 0, 1, 0, 0, 1,
    0, 1, 2, 0, 0, 0, 2, 0,
    0, 0, 0, 0, 3, 0, 0, 1,
    0, 1, 2, 0, 0, 0, 2, 0,
    0, 0, 0, 0, 1, 29, 0, 1,
    0, 2, 2, 0, 0, 0, 128, 0,
    0, 0, 0, 0, 2, 4, 0, 1,
    0, 1, 2, 0, 0, 0, 2, 0,
    0, 0, 0, 0, 2, 8, 0, 1,
    0, 1, 1, 0, 0, 0, 65, 0,
    0, 0, 0, 0, 2, 1, 0, 1,
    0, 2, 2, 0, 0, 0, 16, 0,
    0, 0, 0, 0, 2, 16, 0, 1,
    0, 2, 2, 0, 0, 0, 8, 0,
    0, 0, 0, 0, 3, 5, 1, 1,
    0, 2, 2, 0, 0, 0, 128, 0,
    0, 0, 0, 0, 3, 8, 1, 2,
    0, 2, 2, 0, 0, 0, 64, 0,
    0, 0, 0, 0, 4, 1, 0, 1,
    11, 1, 2, 2, 0, 0, 17, 0,
    0, 0, 0, 0, 3, 6, 1, 2,
    0, 3, 2, 1, 4, 0, 0, 0,
    0, 0, 0, 0, 7, 1, 2, 2,
    11, 2, 2, 2, 0, 0, 64, 0,
    0, 0, 0, 0, 7, 3, 3, 2,
    11, 1, 2, 2, 0, 0, 2, 0,
    0, 0, 0, 0, 3, 1, 4, 1,
    11, 2, 2, 2, 0, 0, 128, 0,
    0, 0, 0, 0, 5, 2, 5, 1,
    10, 1, 1, 2, 0, 0, 4, 0,
    0, 0, 0, 0, 3, 2, 6, 1,
    10, 1, 1, 2, 5, 0, 4, 0,
    0, 0, 0, 0, 7, 6, 7, 1,
    11, 2, 2, 2, 0, 0, 0, 0,
    0, 0, 0, 0, 5, 6, 8, 1,
    11, 1, 2, 3, 0, 0, 12, 0
};

unsigned char D_003EC028[175] = {
    0, 0, 0, 0, 4, 3, 9, 1,
    10, 1, 1, 2, 0, 0, 1, 0,
    0, 0, 0, 0, 2, 6, 10, 2,
    11, 4, 2, 2, 6, 0, 0, 0,
    0, 0, 0, 0, 6, 1, 11, 2,
    0, 3, 2, 3, 7, 0, 0, 0,
    0, 0, 0, 0, 3, 2, 12, 1,
    11, 2, 2, 3, 0, 0, 128, 0,
    0, 0, 0, 0, 2, 1, 13, 1,
    10, 1, 1, 3, 0, 0, 4, 0,
    0, 0, 0, 0, 5, 7, 14, 2,
    11, 1, 2, 2, 0, 0, 10, 0,
    0, 0, 0, 0, 4, 3, 15, 1,
    11, 2, 2, 3, 0, 0, 64, 0,
    0, 0, 0, 0, 4, 0, 0, 1,
    0, 1, 2, 4, 0, 0, 2, 0,
    0, 0, 0, 0, 4, 0, 0, 2,
    0, 1, 2, 5, 0, 0, 4, 0,
    0, 0, 0, 0, 4, 0, 0, 1,
    0, 2, 2, 4, 0, 0, 128, 0,
    0, 0, 0, 0, 4, 0, 0, 3,
    0, 2, 2, 6, 0, 0, 128
};

unsigned int D_003EC0D8[4] = {
    0x7030707u, 0x3070306u, 771, 0
};

unsigned int D_003EC0E8[14] = {
    0x12C012Cu, 0x12C012Cu, 0x12C012Cu, 0x258012Cu, 0x2580258u, 0x2580258u, 0x2580258u, 0x4B00258u,
    0x4B004B0u, 0x4B004B0u, 0x4B004B0u, 0x96004B0u, 0x9600960u, 0x12C00960u
};

unsigned int D_003EC120[14] = {
    0x1C201C2u, 0x1C201C2u, 0x1C201C2u, 0x38401C2u, 0x3840384u, 0x3840384u, 0x3840384u, 0x7080384u,
    0x7080708u, 0x7080708u, 0x7080708u, 0x7080708u, 0xE100E10u, 0x1C200E10u
};

unsigned char D_003EC0D7[] = { 0 };

unsigned char selectionRecordTable[] = { 2 };

unsigned char D_003EC159[] = { 1 };

unsigned char D_003EC15A[766] = {
    200, 0, 0, 0, 2, 4, 150, 0,
    0, 0, 0, 5, 100, 0, 0, 0,
    0, 7, 100, 0, 0, 0, 2, 48,
    200, 0, 0, 0, 2, 49, 200, 0,
    0, 0, 1, 29, 100, 0, 0, 0,
    1, 30, 150, 0, 0, 0, 2, 42,
    150, 0, 0, 0, 0, 16, 100, 0,
    0, 0, 1, 33, 150, 0, 0, 0,
    1, 34, 150, 0, 0, 0, 2, 9,
    150, 0, 0, 0, 2, 14, 150, 0,
    0, 0, 1, 31, 150, 0, 0, 0,
    1, 32, 150, 0, 0, 0, 2, 32,
    200, 0, 0, 0, 2, 31, 200, 0,
    0, 0, 2, 33, 200, 0, 0, 0,
    2, 34, 200, 0, 0, 0, 2, 10,
    150, 0, 0, 0, 2, 11, 150, 0,
    0, 0, 2, 12, 150, 0, 0, 0,
    2, 13, 150, 0, 0, 0, 2, 15,
    150, 0, 0, 0, 2, 16, 150, 0,
    0, 0, 2, 17, 150, 0, 0, 0,
    2, 18, 150, 0, 0, 0, 2, 43,
    44, 1, 1, 0, 0, 17, 144, 1,
    2, 0, 0, 26, 88, 2, 3, 0,
    1, 45, 88, 2, 4, 0, 2, 24,
    44, 1, 0, 0, 2, 23, 44, 1,
    0, 0, 2, 22, 44, 1, 0, 0,
    2, 25, 44, 1, 0, 0, 2, 29,
    44, 1, 0, 0, 2, 27, 44, 1,
    0, 0, 2, 28, 44, 1, 0, 0,
    2, 30, 44, 1, 0, 0, 2, 7,
    88, 2, 0, 0, 2, 2, 88, 2,
    0, 0, 2, 3, 88, 2, 0, 0,
    2, 8, 144, 1, 0, 0, 2, 39,
    244, 1, 5, 0, 0, 1, 244, 1,
    0, 0, 0, 3, 144, 1, 0, 0,
    1, 38, 88, 2, 6, 0, 0, 8,
    200, 0, 0, 0, 0, 9, 200, 0,
    0, 0, 0, 12, 200, 0, 0, 0,
    0, 13, 200, 0, 0, 0, 2, 50,
    144, 1, 0, 0, 2, 51, 144, 1,
    0, 0, 2, 19, 244, 1, 7, 0,
    2, 35, 144, 1, 0, 0, 1, 56,
    144, 1, 0, 0, 1, 55, 144, 1,
    0, 0, 1, 57, 144, 1, 0, 0,
    1, 58, 144, 1, 0, 0, 2, 40,
    176, 4, 8, 0, 0, 15, 232, 3,
    9, 0, 0, 28, 220, 5, 10, 0,
    1, 53, 176, 4, 11, 0, 2, 5,
    32, 3, 0, 0, 2, 46, 176, 4,
    12, 0, 0, 4, 88, 2, 0, 0,
    1, 35, 220, 5, 13, 0, 1, 59,
    244, 1, 0, 0, 1, 60, 244, 1,
    0, 0, 1, 61, 244, 1, 0, 0,
    1, 62, 244, 1, 0, 0, 2, 6,
    32, 3, 0, 0, 0, 20, 232, 3,
    14, 0, 1, 36, 232, 3, 0, 0,
    1, 48, 176, 4, 15, 0, 0, 11,
    144, 1, 0, 0, 0, 6, 144, 1,
    0, 0, 0, 14, 144, 1, 0, 0,
    0, 10, 144, 1, 0, 0, 2, 41,
    176, 4, 0, 0, 2, 44, 32, 3,
    0, 0, 1, 37, 32, 3, 0, 0,
    1, 50, 232, 3, 16, 0, 2, 38,
    176, 4, 0, 0, 0, 18, 220, 5,
    17, 0, 1, 43, 220, 5, 0, 0,
    1, 39, 8, 7, 18, 0, 2, 21,
    232, 3, 0, 0, 2, 20, 32, 3,
    0, 0, 0, 25, 32, 3, 0, 0,
    1, 49, 8, 7, 19, 0, 0, 23,
    16, 14, 20, 0, 0, 21, 16, 14,
    21, 0, 1, 44, 16, 14, 22, 0,
    1, 41, 16, 14, 23, 0, 2, 36,
    96, 9, 0, 0, 2, 45, 160, 15,
    24, 0, 1, 40, 160, 15, 0, 0,
    1, 46, 128, 12, 0, 0, 2, 37,
    240, 10, 0, 0, 0, 27, 192, 18,
    25, 0, 0, 19, 96, 9, 0, 0,
    1, 47, 128, 12, 0, 0, 1, 52,
    16, 14, 26, 0, 1, 54, 128, 12,
    0, 0, 1, 42, 240, 10, 27, 0,
    0, 2, 96, 9, 0, 0, 2, 47,
    128, 37, 28, 0, 0, 24, 32, 28,
    29, 0, 0, 22, 32, 28, 30, 0,
    1, 51, 64, 31, 31, 0, 1, 63,
    0, 0, 0, 0, 2, 52, 0, 0,
    0, 0, 2, 53, 0, 0, 0, 0,
    2, 54, 0, 0, 0, 0, 2, 55,
    0, 0, 0, 0, 2, 56, 0, 0,
    0, 0, 2, 57, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0
};

