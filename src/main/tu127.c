#include "common.h"
#include "main/control_entry.h"
#include "main/xgl_render.h"
#include "main/sdv_033.h"
#include "main/xgl_cd.h"
#include "main/pad.h"
#include "main/game_snap_shot.h"
#include "main/tu124.h"
#include "main/tu098.h"
#include "main/xgl_2.h"
#include "shared.h"

/* Debug test menu (testUiMenuRun): item count, arena size, clear colour and list layout. */
#define TEST_MENU_ITEM_COUNT 7
#define TEST_MENU_ARENA_BYTES 0x40000
#define TEST_MENU_ROW_X 40
#define TEST_MENU_ROW_Y 40
#define TEST_MENU_ROW_HEIGHT 20
#define TEST_MENU_CLEAR_R 16
#define TEST_MENU_CLEAR_G 16
#define TEST_MENU_CLEAR_B 48
#define TEST_MENU_CLEAR_A 128

/* Debug counter (debugCounterStepAndDraw): bit 0x2000 steps it up, 0x8000 steps it down, and the value is kept in 15 bits. */
#define DEBUG_COUNTER_UP 0x2000
#define DEBUG_COUNTER_DOWN 0x8000
#define DEBUG_COUNTER_MASK 0x7FFF

/* Choice menu test (choiceMenuTest): 19 random items in the main menu, 4 in the submenu, state machine values. */
#define CHOICE_TEST_ITEM_COUNT 19
#define CHOICE_TEST_SUB_ITEM_COUNT 4
#define CHOICE_TEST_VALUE_MASK 0x7F
enum ChoiceTestState {
    CHOICE_TEST_STATE_MAIN = 0,
    CHOICE_TEST_STATE_SUB = 1,
    CHOICE_TEST_STATE_DONE = -1
};

/*
 * The 0xD0-byte packet template at debugCounterPacket, sent whole by
 * sceVif1PkAddDataN (52 words). The debug counter writer fills the members
 * below; the rest of the template is never written by this TU.
 */
typedef struct DebugCounterPacket {
    unsigned char unmodeled_00[0x60];
    u64 tag;
    unsigned char unmodeled_68[0x28];
    u32 ctl_90;
    u32 ctl_94;
    unsigned char unmodeled_98[0x8];
    u32 reg_a0;
    u32 reg_a4;
    unsigned char unmodeled_a8[0x8];
    u32 reg_b0;
    u32 reg_b4;
    unsigned char unmodeled_b8[0x8];
    u32 reg_c0;
    u32 reg_c4;
    unsigned char unmodeled_c8[0x8];
} DebugCounterPacket;

/* One 20-byte entry of a texture upload list. */
typedef struct TexUploadEntry {
    u32 header;
    u32 word4;
    u32 word8;
    u32 count12;
    u32 offset16;
} TexUploadEntry;

/* The upload list header: entries follow at data_offset from the list start. */
typedef struct TexUploadList {
    unsigned char unmodeled_00[0x8];
    int entry_count;
    u32 data_offset;
} TexUploadList;

/* The two 64-bit tags written per entry into the shared upload buffer. */
typedef struct TexUploadBuffer {
    unsigned char unmodeled_00[0x20];
    u64 tag_20;
    unsigned char unmodeled_28[0x18];
    u64 tag_40;
} TexUploadBuffer;

extern TexUploadBuffer texUploadTagBuffer;
extern DebugCounterPacket debugCounterPacket;
extern u16 D_0063EDAC[];
extern int debugCounterValue;

typedef struct ClearEnvColor {
    u8 unmodeled_00[0x20];
    u32 color_r;
    u32 color_g;
    u32 color_b;
    u32 color_a;
} ClearEnvColor;

extern ClearEnvColor ClearEnv;

typedef struct TestUiTextBlock {
    int word_00;
    int word_04;
    int word_08;
    int word_0c;
    int word_10;
    int word_14;
    int word_18;
    int word_1c;
    int word_20;
    u8 byte_24;
    u8 byte_25;
    u8 byte_26;
    u8 byte_27;
} TestUiTextBlock;

extern PadRecord PadData[];
extern CursorBuffer D_006AC060[];
extern u32 gameWork[];

typedef struct TestUiItemNames {
    const char *name[TEST_MENU_ITEM_COUNT];
} TestUiItemNames;

extern TestUiItemNames testUiItemNames;
void ov01InitAll(void);
extern const char testUiKanaSample[];
extern const char testUiFontMarkup[];
extern const char testUiTitle[];
extern char testTextEntryWindowName[];
extern char testTextEntryTestName[];
extern char testTextEntryThanksName[];
void resInitPathHeader(int key, int value);
void xglRenderClearFrame(void);
void XenoActorPoolInitDefault(void);
void xglStudioMainCameraInit(void);
void xglSleep(void);
void cursorBufferInit(CursorBuffer *buffer, int capacity, int unit_size, int mode_flags);
void cursorBufferMoveBy(CursorBuffer *buffer, int delta);
void menuSceneReset(void);
void xglCdArenaOpen(int a, int b);
void xglCdArenaClose(void);
void textEntryPoolReset(void);
void choiceMenuResetAll(void);
void textWindowReset(void);
void textBlockClear(void);
void menuSceneRunLoop(void);
void menuSceneRunSplit(void);
void saveScreenRun(int a);
void texUploadTest(void);
void choiceMenuTest(void);
void *textEntryAlloc(int a, int b, int c, int d, int e, void **names);
void textBlockStart(int x, int y, TestUiTextBlock *block);
void textDrawSegment(int, int, const char *, int);
void textWindowShow(int, int, int, const char *, int);
void textUpdate(void);
void XenoActorDoit(void);
void XenoActorDisp(void);
extern void xglFontDebugPrintf(int x, int y, const char *format, ...);

void testUiMenuRun(void) {
    TestUiItemNames items;
    void *entryNames[4];
    void *textEntry = 0;
    u32 i;
    CursorBuffer *cursor;
    TestUiTextBlock block;

    items = testUiItemNames;
    entryNames[0] = testTextEntryWindowName;
    entryNames[1] = testTextEntryTestName;
    entryNames[2] = testTextEntryThanksName;
    entryNames[3] = 0;
    resInitPathHeader(0x01000000, 0x01000000);
    ov01InitAll();
    xglRenderClearFrame();
    XenoActorPoolInitDefault();
    xglStudioMainCameraInit();
    xglSleep();
    ClearEnv.color_r = TEST_MENU_CLEAR_R;
    ClearEnv.color_g = TEST_MENU_CLEAR_G;
    ClearEnv.color_b = TEST_MENU_CLEAR_B;
    ClearEnv.color_a = TEST_MENU_CLEAR_A;
    xglRenderClearFrame();
    cursorBufferInit(D_006AC060, TEST_MENU_ITEM_COUNT, 10, 0);
    menuSceneReset();
    xglCdArenaOpen(0, TEST_MENU_ARENA_BYTES);
    textEntryPoolReset();
    choiceMenuResetAll();
    textWindowReset();
    textBlockClear();

    while ((PadData[0].pressed & PAD_CROSS) == 0) {
        cursor = D_006AC060;
        cursorBufferMoveBy(cursor, ((PadData[0].directionRepeated >> 14) & 1) - ((PadData[0].directionRepeated >> 12) & 1));
        if (PadData[0].pressed & PAD_CIRCLE) {
            switch (cursor->cursor) {
            case 0:
                xglCdArenaClose();
                menuSceneRunLoop();
                xglCdArenaOpen(0, TEST_MENU_ARENA_BYTES);
                break;
            case 1:
                xglCdArenaClose();
                menuSceneRunSplit();
                xglCdArenaOpen(0, TEST_MENU_ARENA_BYTES);
                break;
            case 2:
                xglCdArenaClose();
                if (PadData[0].held & PAD_R2) {
                    saveScreenRun(1);
                } else {
                    saveScreenRun(0);
                }
                xglCdArenaOpen(0, TEST_MENU_ARENA_BYTES);
                break;
            case 3:
                texUploadTest();
                break;
            case 4:
                if (textEntry == 0) {
                    textEntry = textEntryAlloc(200, 300, 200, 88, 7, entryNames);
                }
                break;
            case 5:
                choiceMenuTest();
                break;
            case 6:
                block.word_00 = 123456;
                block.word_04 = 123456;
                block.word_08 = 12;
                block.word_0c = 12;
                block.word_10 = 123456;
                block.word_14 = 1234;
                block.word_18 = 1234;
                block.byte_24 = xglSRand() & 3;
                block.byte_25 = xglSRand();
                block.word_1c = 5;
                block.word_20 = 8;
                block.byte_26 = 2;
                block.byte_27 = 1;
                textBlockStart(100, 150, &block);
                break;
            }
            ClearEnv.color_r = TEST_MENU_CLEAR_R;
            ClearEnv.color_g = TEST_MENU_CLEAR_G;
            ClearEnv.color_b = TEST_MENU_CLEAR_B;
            ClearEnv.color_a = TEST_MENU_CLEAR_A;
            gameWork[2] &= 0xEFFFFFFF;
        }
        if (PadData[0].pressed & PAD_SELECT) {
            textWindowShow(32, 400, 0, testUiKanaSample, 60);
        }
        if (textEntry != 0) {
            if (textEntryPlayVoice(textEntry)) {
                objRemove(textEntry);
                textEntry = 0;
            }
        }
        textUpdate();
        XenoActorDoit();
        XenoActorDisp();
        xglFontPrint(0, 0, 0xFFFFF0, testUiFontMarkup);
        for (i = 0; i < D_006AC060->capacity; i++) {
            textDrawSegment(TEST_MENU_ROW_X, TEST_MENU_ROW_Y + i * TEST_MENU_ROW_HEIGHT, items.name[i], i == D_006AC060->cursor);
        }
        xglFontDebugPrintf(0, 0, testUiTitle);
        xglSleep();
    }
    xglCdArenaClose();
}

extern XglPacket *xglPacketGetCurrent(void);
extern void sceVif1PkCnt(XglPacket *packet, int count);
extern void sceVif1PkAddDataN(XglPacket *packet, const void *data, int count);
extern void FlushCache(int mode);
extern void sceVif1PkRef(XglPacket *packet, const void *data, int count, int a, int b, int c);

void xglTexUploadList(TexUploadList *list) {
    XglPacket *packet = xglPacketGetCurrent();
    TexUploadEntry *entry;
    int i = 0;

    entry = (TexUploadEntry *)((u8 *)list + list->data_offset);
    if (list->entry_count > 0) {
        do {
            u32 header = entry->header;
            u32 low = header & 0xFFFF;
            int blocks;

            if (header == low) {
                blocks = ((int)low + 63) >> 6;
            } else {
                blocks = header >> 16;
            }
            i += 1;
            texUploadTagBuffer.tag_40 = ((u64)entry->word4 << 32) | low;
            texUploadTagBuffer.tag_20 = ((u64)((u32)(entry->word8 + 0xF0000) >> 6) << 32) | ((u64)blocks << 48);
            FlushCache(0);
            sceVif1PkRef(packet, &texUploadTagBuffer, 6, 0, 0, 0);
            sceVif1PkRef(packet, (u8 *)list + entry->offset16, entry->count12, 0, 0, 0);
            entry++;
        } while (i < list->entry_count);
    }
}

/* Format of the debug counter text ("%8d"). */
extern char debugCounterFormat[];

void debugCounterStepAndDraw(void) {
    XglPacket *packet = xglPacketGetCurrent();
    int counter;
    unsigned int stepFlags;

    stepFlags = D_0063EDAC[0];
    counter = debugCounterValue;
    debugCounterPacket.tag = ((u64)(counter + 0x3C00) << 37) | 0x2000000621343C00ULL;
    debugCounterPacket.ctl_90 = 0;
    debugCounterPacket.ctl_94 = 0;
    debugCounterPacket.reg_b0 = 0x2000;
    debugCounterPacket.reg_b4 = 0x2000;
    debugCounterPacket.reg_a0 = 0x6FF8;
    debugCounterPacket.reg_a4 = 0x71F7;
    debugCounterPacket.reg_c0 = 0x8FF8;
    debugCounterPacket.reg_c4 = 0x8DF7;
    if (stepFlags & DEBUG_COUNTER_UP) {
        debugCounterValue += 1;
    }
    if (stepFlags & DEBUG_COUNTER_DOWN) {
        debugCounterValue -= 1;
    }
    debugCounterValue &= DEBUG_COUNTER_MASK;
    sceVif1PkCnt(packet, 0);
    sceVif1PkAddDataN(packet, &debugCounterPacket, 52);
    xglFontDebugPrintf(16, 16, debugCounterFormat, debugCounterValue);
}

extern const char testTexturePath[];
extern TexUploadList *WorkEnd;

void texUploadTest(void) {
    xglCdReadFile(testTexturePath, WorkEnd, 0, 1);
    if ((PadData[0].pressed & PAD_CROSS) == 0) {
        do {
            xglTexUploadList(WorkEnd);
            debugCounterStepAndDraw();
            xglSleep();
        } while ((PadData[0].pressed & PAD_CROSS) == 0);
    }
}

extern int *choiceTestItems[];
extern u16 choiceTestValues[];
extern u8 choiceTestFlags[];
extern int *choiceTestSubItems[];
extern int D_0069DA80;
void choiceMenuOpen(int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7, void *tbl0, void *tbl1, void *tbl2);
void choiceMenuSetActive(int index, int active);
int choiceMenuIsOpen(int index);
int choiceMenuGetResult(int index);

void choiceMenuTest(void) {
    int state;
    int i;
    u16 *pValue;
    int **pItem;
    PadRecord *pad;

    pValue = choiceTestValues;
    pItem = choiceTestItems;
    for (i = 0; i < CHOICE_TEST_ITEM_COUNT; i++) {
        *pItem++ = &D_0069DA80;
        *pValue++ = xglSRand() & CHOICE_TEST_VALUE_MASK;
        choiceTestFlags[i] = (xglSRand() & 1) * 3;
    }
    choiceTestItems[CHOICE_TEST_ITEM_COUNT] = 0;
    for (i = CHOICE_TEST_SUB_ITEM_COUNT - 1; i >= 0; i--) {
        choiceTestSubItems[i] = &D_0069DA80;
    }
    choiceTestSubItems[CHOICE_TEST_SUB_ITEM_COUNT] = 0;
    state = CHOICE_TEST_STATE_MAIN;
    choiceMenuOpen(0, 30, 200, 420, 111, 1, 2, 0, choiceTestItems, choiceTestValues, choiceTestFlags);
    pad = PadData;
    do {
        switch (state) {
        case CHOICE_TEST_STATE_MAIN:
            if (PadData[0].directionPressed & 0x40) {
                choiceMenuClose(0);
            }
            if (choiceMenuIsOpen(0) == 0) {
                choiceMenuGetResult(0);
                state = CHOICE_TEST_STATE_DONE;
            } else if (PadData[0].pressed & PAD_TRIANGLE) {
                choiceMenuSetActive(0, 0);
                choiceMenuOpen(1, 30, 330, 420, 39, 1, 2, 0, choiceTestSubItems, 0, 0);
                state = CHOICE_TEST_STATE_SUB;
            }
            break;
        case CHOICE_TEST_STATE_SUB:
            if (pad->directionPressed & 0x40) {
                choiceMenuClose(1);
            }
            if (choiceMenuIsOpen(1) == 0) {
                if (choiceMenuGetResult(1) == 0) {
                    choiceMenuSetActive(0, 1);
                    state = CHOICE_TEST_STATE_MAIN;
                } else {
                    choiceMenuClose(0);
                    state = CHOICE_TEST_STATE_DONE;
                }
            }
            break;
        }
        if (state == CHOICE_TEST_STATE_DONE) {
            break;
        }
        textUpdate();
        XenoActorDoit();
        XenoActorDisp();
        GameSnapShotCheck();
        xglSleep();
    } while (state != CHOICE_TEST_STATE_DONE);
}

/* Clears the frame, then sleeps forever: a test loop that never returns. */
void ItaiTest(void) {
    xglRenderClearFrame();
    for (;;) {
        xglSleep();
    }
}
