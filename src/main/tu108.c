#include "common.h"
#include "shared.h"
#include "main/tu108.h"
#include "main/tu022.h"
#include "main/tu121.h"
#include "main/tu148.h"
#include "main/xgl_cd.h"
#include "main/xgl_flags.h"

/* The settings of SaveData used here. The member names are the ones the tree already uses for
 * these offsets: flag, pairFirst, pairSecond and copyByte as in tu110.c padApplySaveSettings,
 * dolby_digital and caption_all as in game_movie.c, cursor_memory and cursor_memory_alt as in
 * tu101.c menuDrawConfig (the same type name there). */
typedef struct ConfigSaveSettings {
    unsigned char unmodeled_00[0x28];
    unsigned char flag;                 /* +0x28 */
    unsigned char dolby_digital;        /* +0x29 */
    unsigned char unmodeled_2a[0x32 - 0x2A];
    short pairFirst;                    /* +0x32: horizontal display offset (tyaSetDisplayOffset) */
    short pairSecond;                   /* +0x34: vertical display offset */
    unsigned char unmodeled_36[0x3A - 0x36];
    unsigned char caption_all;          /* +0x3A */
    unsigned char unmodeled_3b[0x3E - 0x3B];
    unsigned char copyByte;             /* +0x3E: copied to both pads' actuatorMode */
    unsigned char unmodeled_3f[0x46 - 0x3F];
    unsigned char cursor_memory;        /* +0x46 */
    unsigned char cursor_memory_alt;    /* +0x47 */
} ConfigSaveSettings;

extern ConfigSaveSettings SaveData;
void tyaDisplaySetting(int framebuffer);

void saveDataSetDefaults(void) {
    SaveData.flag = 1;
    SaveData.caption_all = 2;
    SaveData.cursor_memory = 0;
    SaveData.cursor_memory_alt = 0;
    SaveData.copyByte = 0;
    SaveData.dolby_digital = 0;
    tyaDisplaySetting(1);
}

/* signed char: func_002285C8 tests it with lb. */
extern signed char menuLoadPending;

/* Sets the load-pending flag that func_002285C8 reads. */
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

/* The status record of one party member (tu110.c's PartyMemberStatus; tu101.c's menuDrawPartyPanel
 * reads it as MenuPartyName, whose member text is used here): two texts of MainMenu.txd and value,
 * which saveWriteImage saves as value + 1. */
struct PartyMemberStatus {
    char *text;                         /* +0x00: the txd entry of the member index */
    char *textAlt;                      /* +0x04: the txd entry of the member index + 17 */
    unsigned char value;                /* +0x08: func_002285C8 stores the member index */
};

/* One 0x40-byte slot record of slotRecordTable (ov01 tu027.c); tu100.c and tu101.c read it as
 * ControlLimits, tu109.c as SelectionCounter. No member is accessed here. */
struct SlotRecord {
    unsigned char unmodeled_00[0x40];
};

/* One 0x2B-byte record of D_006A5880 (tu110.c's SaveMemberName, copied whole into the save
 * image). It is the record tu100.c, tu101.c and tu109.c read as ControlBuffer, with their member
 * names: the taken bits of one member (header, cells), the highest open group and the taken count
 * (tu109.c's names; tu100.c and tu101.c spell these two bytes flags[2]) and the entries tu109.c's
 * selectionFindIndex searches. func_002285C8 clears all of it. */
struct SaveMemberName {
    unsigned char header[4];            /* +0x00 */
    unsigned char cells[4][8];          /* +0x04 */
    unsigned char openGroup;            /* +0x24 */
    unsigned char takenCount;           /* +0x25 */
    unsigned char entries[4];           /* +0x26 */
    unsigned char entry_count;          /* +0x2A */
};

/* A port record of PadData (0x80 bytes); only actuatorMode (tu060.c's name for +0x56) is modeled. */
struct PadPort {
    unsigned char unmodeled_00[0x56];
    unsigned char actuatorMode;         /* +0x56 */
    unsigned char unmodeled_57[0x80 - 0x57];
};

/* The clock reading xglClockRead fills. */
typedef struct DayTime {
    unsigned char status;
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char day;
    unsigned char month;
    unsigned short year;
} DayTime;

extern struct PartyMemberStatus D_006A5A80[];
extern struct SlotRecord D_0061C400[];
extern struct SaveMemberName D_006A5880[];
extern struct PadPort PadData[];
void selectionFlagTableInit(void);
void saveTextStateReset(void);
void tyaSetDisplayOffset(int horizontal, int vertical);
void xglClockRead(DayTime *time);
unsigned int xglClockDayTime2UInt(DayTime *time);
void func_0022B5E0(void);
void ptAddMember(int id);
void ptSetCurrentSlot(int slot);
void ptRefresh(void);
void selectionBlockClear(void);

/* One 16-byte record of the 10-entry table ptMemberRecords, keyed by its id. */
struct IdSlot {
    int id;
    struct PartyMemberStatus *status;   /* +0x04: D_006A5A80[index] */
    struct SlotRecord *slotRecord;      /* +0x08: the slot record of the member's id */
    struct SaveMemberName *name;        /* +0x0C: D_006A5880[index], the member's taken bits */
};

extern struct IdSlot ptMemberRecords[];

/* The object attached to the current uwamono (D_006A5A30, the record ptState.obj points at;
 * tu109.c reads it as SelectionBlock with the same progressId, playTime, id and mapFlag, tu110.c
 * as PartyProgress). id is -1 while nothing is attached. */
struct UwamonoContext {
    unsigned short progressId;          /* +0x00 */
    unsigned int playTime;              /* +0x04: xglClockDayTime2UInt reading at reset */
    int id;
    unsigned char unmodeled_0C[4];
    DoublewordAlignedVector4 position;  /* +0x10: the actor position copied on attach */
    unsigned char state;                /* +0x20 */
    unsigned char mapFlag;              /* +0x21 */
};

/* The global binding ptState (tu110.c's PartyState, whose names slots, roster, frontIndex,
 * backIndex and counts are used here): the attached object, the member slots and roster that
 * func_002285C8 clears, and the flag copied into the object's state on attach. */
struct UwamonoBinding {
    struct UwamonoContext *obj;
    struct IdSlot *slots[10];           /* +0x04 */
    struct IdSlot *roster[10];          /* +0x2C */
    unsigned char attachState;          /* +0x54 */
    unsigned char frontIndex;           /* +0x55 */
    unsigned char backIndex;            /* +0x56 */
    unsigned char counts[3];            /* +0x57 */
};

/* gameWork as used here: its flags word. Shared with other TUs under their own types. */
struct SharedFlags {
    unsigned char unmodeled_00[8];
    int flags;                          /* +0x08; bit 0x800 is set on attach and cleared on detach */
    unsigned char unmodeled_0C[0x18 - 0x0C];
    int mapId;                          /* +0x18 */
};

extern struct UwamonoBinding ptState;
extern struct UwamonoContext D_006A5A30;
extern struct SharedFlags gameWork;
void uwamonoSetTransition(int, int, int, int);
void ptEnterSplit();
void ptLeaveSplit();

/* The uwamono actor (include/main/tu051.h's struct XenoActor); only its position is read here. */
struct XenoActor {
    unsigned char unmodeled_00[0x10];
    AlignedVector4 position;            /* +0x10 */
};

/* One 0x870-byte record of uwamonoSlotTable; only the actor pointer is read here. */
struct UwamonoSlot {
    struct XenoActor *actor;            /* +0x00 */
    unsigned char unmodeled_04[0x870 - 4];
};

extern struct UwamonoSlot uwamonoSlotTable[];
extern int uwamonoCurrentIndex;
extern int D_003EBA88[];

/* Resets the party. On a pending load it first calls selectionFlagTableInit and
 * saveTextStateReset, writes the default settings (saveDataSetDefaults), applies them to the
 * pads, func_001A4400 and the display offset as padApplySaveSettings does, and reads the menu
 * text tables (menuLoadTxds). Then it rebuilds the ten member records, clears the party state,
 * stamps the play time, adds the default members, makes slot 0 current and clears the
 * selection block and the pending flag. */
void func_002285C8(void) {
    DayTime now;
    int i;
    int j;
    int k;

    if (menuLoadPending) {
        selectionFlagTableInit();
        saveTextStateReset();
        saveDataSetDefaults();
        PadData[0].actuatorMode = PadData[1].actuatorMode = SaveData.copyByte;
        func_001A4400(SaveData.flag != 0);
        tyaSetDisplayOffset(SaveData.pairFirst, SaveData.pairSecond);
        menuLoadTxds();
    }
    D_006A5A30.id = -1;
    D_006A5A30.progressId = 0;
    D_006A5A30.mapFlag = 0;
    for (i = 0; i < 10; i++) {
        D_006A5A80[i].text = (char *)txdTableEntry((int *)menuMainTxdBuffer, i);
        D_006A5A80[i].textAlt = (char *)txdTableEntry((int *)menuMainTxdBuffer, i + 17);
        D_006A5A80[i].value = i;
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
        if (i >= 7 && i < 10) {
            ptMemberRecords[i].id = i + 4;
            ptMemberRecords[i].slotRecord = &D_0061C400[i + 3];
        } else {
            ptMemberRecords[i].slotRecord = &D_0061C400[i];
            ptMemberRecords[i].id = i + 1;
        }
        ptMemberRecords[i].status = &D_006A5A80[i];
        ptMemberRecords[i].name = &D_006A5880[i];
        ptState.slots[i] = 0;
        ptState.roster[i] = 0;
    }
    ptState.obj = &D_006A5A30;
    ptState.attachState = 0;
    ptState.frontIndex = 0;
    ptState.backIndex = 0;
    ptState.counts[0] = 0;
    ptState.counts[1] = 0;
    ptState.counts[2] = 0;
    xglClockRead(&now);
    ptState.obj->playTime = xglClockDayTime2UInt(&now);
    func_0022B5E0();
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

/* Attaches the ptState object to the current uwamono: takes the map id and the actor's position,
 * enters or leaves split mode as runSplit asks, and starts the kind's transition. */
void uwamonoAttachCurrent(unsigned int kind, int runSplit) {
    struct UwamonoContext *obj = ptState.obj;

    if (obj->id == -1 && uwamonoCurrentIndex != -1 && kind < 8) {
        obj->id = gameWork.mapId;
        obj->position = uwamonoSlotTable[uwamonoCurrentIndex].actor->position;
        obj->state = ptState.attachState;
        if (runSplit && ptState.attachState == 0) {
            ptEnterSplit();
        } else if (!runSplit && ptState.attachState != 0) {
            ptLeaveSplit();
        }
        if (!runSplit && kind == 7) {
            xglFlagsSet1(0x2F0, 1);
        }
        gameWork.flags |= 0x800;
        uwamonoSetTransition(0, D_003EBA88[kind], kind == 6 ? 4 : 201, 0);
        msgResetAll();
    }
}

/* Detaches the current uwamono object: stops its effect, clears its flags and frees the id. */
void uwamonoDetachCurrent(void) {
    if (ptState.obj->id != -1) {
        if (ptState.obj->state != 0) {
            if (ptState.attachState == 0) {
                ptEnterSplit();
            }
        } else if (ptState.attachState != 0) {
            ptLeaveSplit();
        }
        xglFlagsSet1(0x2F0, 0);
        gameWork.flags &= ~0x800;
        uwamonoSetTransition(0, ptState.obj->id, -2, 0);
        msgResetAll();
        ptState.obj->id = -1;
    }
}
