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

extern const char menuMainTxdPath[];
extern const char menuTutorialTxdPath[];
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

extern struct PartyMemberStatus D_006A5A80[];
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

extern struct IdSlot ptMemberRecords[];

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
