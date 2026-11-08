#include "common.h"
#include "shared.h"
#include "main/tu092.h"
#include "main/tu094.h"
#include "main/tu096.h"
#include "main/tu097.h"
#include "main/tu100.h"
#include "main/tu101.h"
#include "main/tu109.h"
#include "main/tu110.h"
#include "main/tu121.h"
#include "main/xgl_cd.h"

void bgLayerInit();
void fontObjInitGlobals();
void cursorBufferInit();
void cursorBufferTablesInit(void);
void func_00210C78();
void func_002285C8();
extern int D_0069D4DC;
extern u8 D_0069D4E0;
extern u8 *D_00645868[];
extern unsigned char D_006A4900[];
extern unsigned char D_006A4928[];
extern unsigned char xglCallbackQueueRun[];

void menuSceneReset(void) {
    xglCdPendingReadsClear();
    func_002285C8();
    selectionBuildKeyTable();
    func_00229898();
    xglTickReset();
    cursorBufferInit(D_006A4900, 6, 6, 0);
    cursorBufferInit(D_006A4900 + 0x14, 4, 4, 0);
    cursorBufferInit(D_006A4928, 0, 0, 0);
    D_0069D4DC = 0;
    D_0069D4E0 = 0;
    func_00233D70();
    fontObjInitGlobals();
    func_00210C78();
    bgLayerInit();
    D_00645868[0] = xglCallbackQueueRun;
    cursorBufferTablesInit();
}

extern signed char D_0069D502;
extern u8 D_0069D504;
extern u8 selectionWindowCount;
extern signed char D_0069E228;
extern unsigned char D_006A5150[];
extern unsigned char D_006A5168[];
extern unsigned char D_006A5180[];
extern unsigned char D_006A5198[];
extern unsigned char D_006A51B0[];
extern unsigned char D_006A51C8[];
extern unsigned char D_006A51E0[];
extern unsigned char D_006A5220[];
extern unsigned char D_006A5238[];

void cursorBufferTablesInit(void) {
    cursorBufferInit(D_006A4900, 6, 6, 0);
    cursorBufferInit(D_006A4900 + 0x14, 4, 4, 0);
    cursorBufferInit(D_006A5150, selectionWindowCount, 0xA, 0);
    cursorBufferInit(D_006A5168, 0x6B, 0x12, 0);
    cursorBufferInit(D_006A5180, 3, 4, 0);
    cursorBufferInit(D_006A5198, 7, 7, 0);
    cursorBufferInit(D_006A51B0, D_0069D504, 4, 0);
    cursorBufferInit(D_006A51C8, 8, 8, 0);
    cursorBufferInit(D_006A51E0, 4, 4, 0);
    cursorBufferInit(D_006A51E0 + 0x14, 8, 8, 0);
    cursorBufferInit(D_006A51E0 + 0x28, 4, 4, 0);
    cursorBufferInit(D_006A5238, 8, 8, 0);
    cursorBufferInit(D_006A5220, 0xA, 0xA, 0);
    D_0069E228 = 0;
    D_0069D502 = 0;
}

typedef unsigned char u8;

/* A cursor buffer (main/tu098 CursorBuffer); only its unit size at 0x0C is touched here. */
typedef struct CursorBuffer {
    u8 unmodeled_00[12];
    int unit_size;              /* 0x0C */
} CursorBuffer;

void cursorBufferSetCapacity(CursorBuffer *buffer, int capacity);
extern signed char D_0069D4EC;

/* Refreshes a cursor buffer: while D_0069D4EC is set only the unit size and capacity change, otherwise a full init. */
void cursorBufferSetOrInit(CursorBuffer *buffer, int capacity, int unit_size, int mode_flags) {
    if (D_0069D4EC != 0) {
        buffer->unit_size = unit_size;
        cursorBufferSetCapacity(buffer, capacity);
    } else {
        cursorBufferInit(buffer, capacity, unit_size, mode_flags);
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_002104B8);

/* One 0x14-byte entry of the table at D_006A53A0. */
typedef struct ControlSlot {
    u8 unmodeled_00[8];         /* 0x00: first word is a callback, called by func_00214F88 */
    void *link;                 /* 0x08 */
    short value;                /* 0x0C: written when flag is set */
    signed char state;          /* 0x0E */
    signed char flag;           /* 0x0F */
    u8 flags;                   /* 0x10: bit 1 set here */
    u8 unmodeled_11[3];
} ControlSlot;

/* Stores flag in the slot and marks the slot active (flags bit 1). */
void func_00210560(ControlSlot *slot, int flag, int value) {
    flag &= 0xFF;
    if (flag != 0) {
        slot->value = (short) value;
    }
    slot->flag = (signed char) flag;
    slot->flags |= 2;
}

void fontTrackSetBoth();
extern signed char D_0069E25C;
extern signed char D_0069E25D;
extern int D_0069E260;
extern int D_006A5480[];
extern unsigned char D_006A54A0[];

void func_00210580(void) {
    int i;
    int *sequenceSlot;

    sequenceSlot = D_006A5480;
    sequenceSlot += 7;
    for (i = 0; i < 8; i++) {
        *sequenceSlot = 0;
        sequenceSlot -= 1;
    }
    D_0069E25C = 0;
    D_0069E25D = 0;
    D_0069E260 = 0;
    fontTrackSetBoth(D_006A54A0, 0, 0);
    fontTrackSetBoth(D_006A54A0 + 0x1C, 0, 0);
    fontTrackSetBoth(D_006A54A0 + 0x38, 0, 0);
    fontTrackSetBoth(D_006A54A0 + 0x54, 0, 0);
}

int xglCdBufferReserve();
void xglCdReadFileAsync();
extern int D_003EB148[];

void func_00210620(int count) {
    int remaining;
    int buffer;
    int *sequenceSlot;

    if (count > 0) {
        if (count < 9) {
            D_0069E260 = xglCdBufferReserve(0xE000);
            if (count > 0) {
                remaining = count;
                sequenceSlot = D_006A5480;
                do {
                    remaining -= 1;
                    buffer = xglCdBufferReserve(0xE000);
                    *sequenceSlot = buffer;
                    sequenceSlot += 1;
                } while (remaining != 0);
            }
            D_0069E25C = (signed char) count;
            D_0069E25D = 0;
            xglCdReadFileAsync(D_003EB148[0], D_0069E260, 4, 0);
        }
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_002106D0);

INCLUDE_ASM("asm/main/nonmatchings/tu100", xglPacketAddTexturedDraw);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_002109A8);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00210BA8);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00210C78);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00210CD8);

extern signed char D_0069E229;
extern unsigned char D_006A52C0[];

void func_00210D38(int key) {
    D_006A52C0[((u8) key) >> 1] += 1;
    if (D_006A52C0[2] == 3 && D_006A52C0[3] == D_006A52C0[2] && D_006A52C0[4] == D_006A52C0[3] && D_006A52C0[5] == D_006A52C0[4] && D_006A52C0[6] == D_006A52C0[5] && D_006A52C0[7] == D_006A52C0[6] && D_006A52C0[8] == D_006A52C0[7] && D_006A52C0[9] == D_006A52C0[8] && D_006A52C0[10] == D_006A52C0[9]) {
        D_0069E229 = 1;
    }
}

extern int D_003EB098[];
extern int D_003EB09C[];
extern unsigned char D_006A5260[];
extern unsigned char D_006A5290[];

/* Runs once per index: the byte at D_006A52C0[index] is set on the first call, then the two files of that
   index are read into the buffers held in D_006A5260 and D_006A5290 (buffers and file ids not yet proved). */
void func_00210DD8(int index) {
    u8 *tally;
    int firstBuffer;
    int secondBuffer;

    tally = &D_006A52C0[index];
    if (*tally == 0) {
        firstBuffer = ((int *) D_006A5260)[index];
        *tally += 1;
        if (firstBuffer != 0) {
            xglCdReadFileAsync(D_003EB098[index * 2], firstBuffer, 3, (index * 2) & 0xFE);
        }
        secondBuffer = ((int *) D_006A5290)[index];
        if (secondBuffer != 0) {
            xglCdReadFileAsync(D_003EB09C[index * 2], secondBuffer, 3, (index * 2 + 1) & 0xFF);
        }
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00210EA0);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00210F70);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00211518);

/* The object a ControlSlot links to; its flags word holds the current bit. */
typedef struct ControlLink {
    u8 unmodeled_00[16];
    int flags;                  /* 0x10: bit 0 set while this link is current */
} ControlLink;

extern ControlLink *D_0069D4CC;

/* Moves the current mark (flags bit 0) from the old current link to link, and makes link current. */
void func_00211918(ControlLink *link) {
    if (D_0069D4CC != 0) {
        D_0069D4CC->flags &= ~1;
    }
    if (link != 0) {
        link->flags |= 1;
    }
    D_0069D4CC = link;
}

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00211950);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_002119C8);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00211A80);

void func_00211950(int from, int to);
void func_002119C8(int to, int from);
extern int D_0069D4D4;
extern ControlSlot D_006A53A0[];

void func_00211B88(int index, int flag) {
    ControlSlot *slot = &D_006A53A0[index];

    slot->state = 0;
    slot->flags |= 1;
    func_00211918(slot->link);
    func_00211950(D_0069D4D4, index);
    if (flag != 0) {
        func_002119C8(7, D_0069D4D4);
    } else {
        func_00211950(7, index);
    }
    func_002119C8(index, D_0069D4D4);
    D_0069D4D4 = index;
}

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00211C28);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00211D70);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00211DB8);

INCLUDE_ASM("asm/main/nonmatchings/tu100", xglActiveCameraSetup);

void xglCdArenaOpen(int first, int second);
void xglCdBufferAlloc(int value);
void fontObjLoadTexture(int mode);
void func_00210580(void);
void func_00210BA8(void);

void menuSceneLoadAssets(void) {
    while (xglCdServiceIfPending() != 0) {
    }
    xglCdArenaOpen(0, 0);
    xglCdBufferAlloc(0x140000);
    func_00210580();
    func_00210BA8();
    bgLayerLoadAll();
    fontObjLoadTexture(2);
    fontObjLoadTexture(3);
    fontObjLoadTexture(0);
}

void bgLayerResetState(void);

void menuSceneUnloadAssets(void) {
    xglCdBufferClear();
    bgLayerResetState();
    fontObjClearSlots();
    xglCdArenaClose();
}

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00212798);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00212FD8);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00213188);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00213300);


/* Submits the four object requests of the layout at base: ids 0xE4 and 0xE6, then 0xEA twice. */
void func_00213A78(int base) {
    packetSubmitRequest(0xE4, 0, base, 0x11);
    packetSubmitRequest(0xE6, 0, base - 0x1D, 0x11);
    packetSubmitRequestEx(0xEA, 0, base, 0x200, 0x10, 0x801);
    packetSubmitRequestEx(0xEA, 0, base + 0x19, 0x200, 0x10, 0x801);
}

void gameOverDrawOverlay(int value);
void choiceDraw(void);
void xglFontFlush(void);
extern int D_0069D4E4;
extern int countdownActive;
extern u8 D_0069D518;
extern u8 D_0069D529;
extern u8 D_003EC662[];

/* The state block at D_006A4940; only two fields are touched here. */
typedef struct StateBlock {
    u8 unmodeled_00[12];
    int offset;                 /* 0x0C */
    u8 unmodeled_10[0x3BC];
    u8 flag3CC;                 /* 0x3CC */
} StateBlock;

extern StateBlock D_006A4940;

/* Draws the menu overlays, each one gated by its flag, then flushes the font queue. */
void menuSceneDrawOverlays(void) {
    D_003EC662[0] = D_006A4940.flag3CC;
    if (D_0069D529 != 0) {
        D_006A4940.offset = -0x30;
    } else {
        D_006A4940.offset = 0;
    }
    if (D_0069D518 != 0) {
        gameOverDrawOverlay(0x40);
    }
    fontObjResetAndBuild();
    if (D_0069D4E4 != 0) {
        choiceDraw();
    }
    if (countdownActive != 0) {
        countdownDraw();
    }
    xglFontFlush();
}

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00213B80);

void menuDrawItemList(int id, int unit, void *target);
void func_00226948(u8 *table);
extern u8 D_006A4D28[];

/* Draws the item list of the slot's link, then the labels in the table D_006A4D28. */
void menuDrawSlotItems(ControlSlot *slot) {
    menuDrawItemList(0xD, 0x5F, slot->link);
    func_00226948(D_006A4D28);
}

void func_00220550();
extern int D_0069E238;

/* Submits the four layout packets; D_0069D4E0 picks one of two coordinate sets. */
void menuLayoutSubmit(void) {
    func_00220550(0xD, 0xD4, D_006A5180, &D_0069E238);
    if (D_0069D4E0 != 0) {
        packetSubmitRequest(0x161, 0x11, 0xB7, 1);
        packetSubmitRequest(0x163, 0xAF, 0xB7, 1);
        packetSubmitRequestEx(0x162, 0x21, 0xB7, 0x8E, 0x11, 1);
        packetSubmitRequest(0x139, 0x48, 0xB6, 1);
    } else {
        packetSubmitRequest(0x161, 0x11, 0xB7, 1);
        packetSubmitRequest(0x163, 0x9E, 0xB7, 1);
        packetSubmitRequestEx(0x162, 0x21, 0xB7, 0x7D, 0x11, 1);
        packetSubmitRequest(0x139, 0x40, 0xB6, 1);
    }
    func_00226948(D_006A4D28);
}

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00214378);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_002144F8);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_002146B8);

void func_00214C98(void) {

}

struct ControlRecord {
    unsigned char unmodeled_00[8];
    int value;
};

int menuDrawItemListDetail(int, int, int);

/* Draws the detail panel of the record's value through menuDrawItemListDetail. */
int menuRecordItemDetail(struct ControlRecord *record) {
    return menuDrawItemListDetail(0x1F, 0x5F, record->value);
}

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00214CC0);

int func_002109A8(int);

void func_00214D10(struct ControlRecord *record) {
    do {
        func_002109A8(record->value);
    } while (0);
}

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00214D30);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00214F88);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00215170);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00215308);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_002153F0);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00215450);

void seVoiceStartDefault(int value);
void msgBufferAppend(void *buffer);
void countdownInit(void *buffer, int size);
extern u8 D_0069D550[];

/* Loads the two message buffers, starts the voice cue 0x01000003 and sets countdownActive (countdownActive). */
void countdownStart(void) {
    u8 *firstBuffer;
    u8 *secondBuffer;

    firstBuffer = menuMainTxdEntry(0x49);
    countdownInit(firstBuffer, 0x12C);
    msgBufferAppend(D_0069D550);
    secondBuffer = menuMainTxdEntry(0x4A);
    msgBufferAppend(secondBuffer);
    seVoiceStartDefault(0x01000003);
    countdownActive = 1;
}

void func_002AA5D8(int *target, u8 *source, int value);
extern int D_0069E220;
extern u8 D_0069D558[];

/* Formats value as "%5d" into a shared buffer and returns that buffer. */
int *func_00215598(int value) {
    func_002AA5D8(&D_0069E220, D_0069D558, value);
    return &D_0069E220;
}

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_002155D0);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_002158B8);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00215B68);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00215D60);

void cursorBufferSetOffset();
void func_00211918(ControlLink *link);
void func_002155D0();
struct ControlContext *func_0022A560();
extern signed char D_0069D519;
extern signed char D_0069D51C;
extern signed char D_0069D528;
extern unsigned char D_006A53EA[];

/* The limits block of a ControlContext; both are set to 1000 by func_00215DE8. */
struct ControlLimits {
    unsigned char unmodeled_00[8];
    int limit_a;
    int limit_b;
};

/* The buffer of a ControlContext: a 4-byte header, a 4 x 8 cell grid and two flag bytes. */
struct ControlBuffer {
    unsigned char header[4];
    unsigned char cells[4][8];
    unsigned char flags[2];
};

/* The context returned by func_0022A560. */
struct ControlContext {
    unsigned char unmodeled_00[8];
    struct ControlLimits *limits;
    struct ControlBuffer *buffer;
};

void func_00215DE8(void) {
    int i;

    i = 0;
    D_0069D519 = 0;
    func_002155D0(0);
    D_0069D528 = 0;
    D_0069D51C = 0;
    do {
        func_0022A560()->buffer->header[i] = 0;
        i += 1;
    } while (i < 4);
    i = 0;
    do {
        func_0022A560()->buffer->cells[0][i] = 0;
        func_0022A560()->buffer->cells[1][i] = 0;
        func_0022A560()->buffer->cells[2][i] = 0;
        func_0022A560()->buffer->cells[3][i] = 0;
        i += 1;
    } while (i < 8);
    func_0022A560()->buffer->flags[0] = 0;
    func_0022A560()->buffer->flags[1] = 0;
    func_0022A560()->limits->limit_a = 1000;
    func_0022A560()->limits->limit_b = 1000;
    cursorBufferSetOffset(D_006A51E0, 0);
    cursorBufferSetOffset(D_006A51E0 + 0x14, 0);
    cursorBufferSetOffset(D_006A51E0 + 0x28, 0);
    D_006A53EA[0] = 4;
    func_00211918(D_006A51E0);
}

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00215F10);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00216858);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00216BC0);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_002171A0);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_002175A8);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00217D10);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00217E30);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00218800);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00218ED8);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00219140);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_002195C0);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00219840);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00219980);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00219AF8);

INCLUDE_ASM("asm/main/nonmatchings/tu100", func_00219B50);

/* An object reached through the pointer at ptState; only its key is read. */
typedef struct KeyedObject {
    u8 unmodeled_00[8];
    int key;                    /* 0x08: compared with -1 */
} KeyedObject;

int func_00219B50(void);
extern KeyedObject *ptState[];

/* Sets D_0069D502 when the first state object's key is -1, then runs the scene loop func_00219B50. */
void menuSceneRunKeyCheck(void) {
    if (ptState[0]->key == -1) {
        D_0069D502 = 1;
    }
    func_00219B50();
}

void ptEnterSplit(void);
void ptLeaveSplit(void);
extern u8 ptSplitMode[];

/* Runs the scene loop func_00219B50 inside the split that ptEnterSplit opens (roster compaction), unless ptSplitMode is 1. */
int menuSceneRunSplit(void) {
    int result;

    if (ptSplitMode[0] == 1) {
        result = func_00219B50();
    } else {
        ptEnterSplit();
        result = func_00219B50();
        ptLeaveSplit();
    }
    return result;
}
