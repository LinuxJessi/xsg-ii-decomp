#include "common.h"
#include "shared.h"
#include "main/tu097.h"
#include "main/tu101.h"
#include "ov01/tu023.h"
#include "ov01/tu043.h"
#include "main/tu051.h"
#include "main/tu048.h"

/* ptFindRecord result. */
typedef struct ResultLabel {
    unsigned char *text;
    unsigned char unmodeled_04[4];
    unsigned char icon;
} ResultLabel;

typedef struct ResultValue {
    int unmodeled_00;
    int amount;
} ResultValue;

typedef struct PtRecord {
    int id;
    ResultLabel *label;
    ResultValue *value;
} PtRecord;

/* func_A8B490 result: gauge is an unaligned word at +2. */
typedef struct ResultText {
    unsigned char unmodeled_00[2];
    long gauge : 32;
    short value_06;
    unsigned char unmodeled_08[11];
    signed char value_13;
    unsigned char unmodeled_14[72];
    int value_5c;
    short value_60;
} ResultText;

/* Battle result screen: scene states (battleResultState), its frame timing and the display limits of the figures. */
enum BattleResultState {
    BATTLE_RESULT_STATE_COUNT_UP = 0,
    BATTLE_RESULT_STATE_TALLY = 1,
    BATTLE_RESULT_STATE_SUMMARY = 2,
    BATTLE_RESULT_STATE_ENTRY_LIST = 4
};
/* Frames of the count-up before the tally starts, and frames per second for the clear time. */
#define BATTLE_RESULT_COUNT_FRAMES 180
#define BATTLE_RESULT_FRAMES_PER_SECOND 30
/* Largest clear time shown (99:59:59 at 30 frames per second). */
#define BATTLE_RESULT_CLEAR_TIME_CAP 0xA4CB7F
/* Largest score (eight digits) and largest five-digit figure of the score parts. */
#define BATTLE_RESULT_SCORE_CAP 0x05F5E0FFULL
#define BATTLE_RESULT_PART_CAP 0x1869FULL
/* Menu text ids: the list heading and the unit that follows each count. */
#define BATTLE_RESULT_TXD_HEADING 0xE5
#define BATTLE_RESULT_TXD_COUNT_UNIT 0xE6

extern int battleResultScoreMax;
/* The rows battleResultApply (ov01 tu023.c) fills, one per party unit. */
extern struct BattleResultRow *battleResultRows;
extern int (*battleResultStats)[4];
extern unsigned char menuPanelFrame[];
extern unsigned char battleResultScoreDashes[];
extern unsigned char battleResultSignedDashes[];
extern const unsigned char battleResultTextSetup[];
extern const unsigned char battleResultClearTimeLabel[];
extern const unsigned char battleResultTimeTemplate[];
extern unsigned char battleResultTitle[];
extern int battleResultState;
extern int battleResultStateTimer;
extern int battleResultMemberCount;
extern int battleResultEntryCount;
extern int battleResultClearFrames;
extern signed char D_0069D91C;
PtRecord *ptFindRecord(int key);
ResultText *func_A8B490(int id);
void textDrawShadow(int x, int y, void *text, unsigned long long style);
void textDrawDecimalShadow(int x, int y, int value, int digits, unsigned long long style);
void textDrawSignedDecimalShadow(int x, int y, int value, int digits, unsigned long long style);
void textDrawDecimalAltShadow(int x, int y, int value, int digits, unsigned long long style);
void panelDrawNineSlice(unsigned char *frame, int x, int y, int width, int height);
void menuDrawSplitFrame(int x, int y, int width, int height, int extra);
void menuDrawIndexedIcon(int x, int y, unsigned char icon);
void menuDrawStatIcon(int a, int x, int y, unsigned long long style);
void menuDrawLevelShadow(int x, int y, int value);
void menuDrawHpShadow(int x, int y, int a, long b);
void menuDrawEpShadow(int x, int y, int a, int b);
void menuDrawHpWideShadow(int x, int y, int a, long b);
void menuDrawEpWideShadow(int x, int y, int a, int b);
void menuDrawShortOutlineFrame(int x, int y, int a, int b);
long long func_002A5C88(long long a, long long b);
void battleResultDrawEntries(int x, int y);
void textDrawSegment(int x, int y, void *text, unsigned long long style);
void textDrawDecimalShadowZero(int x, int y, int value, int digits, unsigned long long style);
void menuDrawKindIcon(int x, int y, int kind);
void *menuMainTxdEntry(int id);

/* Font colours of the highlight shadow and fill (packed float 1.0 with colour words). */
#define RESULT_SHADOW_COLOUR 0x3F80000080000000LL
#define RESULT_FILL_COLOUR 0x3F80000080407080LL
/* Result row: label, the score and time values, and the adjusted sub-row for entry index. */
void battleResultDrawRow(int x, int y, int index, int tally)
{
    struct BattleResultRow *row;
    PtRecord *record;
    ResultLabel *label;
    ResultValue *valueRecord;
    ResultText *resultText;
    int wideOffset;
    int subFrameY;
    int cardX;
    int lowerRowY;
    int adjust;
    int scoreRowY;
    int markerY;
    int markerX;
    int markerFillX;
    unsigned long long total;
    unsigned long long rest;

    row = battleResultRows + index;
    record = ptFindRecord(row->key);
    if (record == 0) {
        return;
    }
    label = record->label;
    resultText = func_A8B490(record->id);
    valueRecord = record->value;
    if (row->key >= 11) {
        x -= 8;
        wideOffset = 17;
    } else {
        wideOffset = 0;
    }
    panelDrawNineSlice(menuPanelFrame, x, y, wideOffset + 374, 106);
    menuDrawSplitFrame(x + 4, y + 5, wideOffset + 366, 96, wideOffset + 176);
    cardX = x + wideOffset;
    menuDrawOutlineFrame(x + 13, y + 28, wideOffset + 173, 66);
    subFrameY = y + 53;
    menuDrawOutlineFrame(cardX + 191, subFrameY, 100, 41);
    adjust = tally != battleResultScoreMax ? row->levelGain : 0;
    panelDrawNineSlice(menuPanelFrame, x + 21, y + 35, wideOffset + 157, 39);
    menuDrawIndexedIcon(x + 27, y + 10, label->icon);
    packetSubmitRequest(288, x + 68, y + 8, 17);
    textDrawShadow(x + 73, y + 11, label->text, 0xFE00ULL << 46);
    menuDrawLevelShadow(x + wideOffset + 140, y + 7, resultText->value_13 - adjust);
    if (row->levelGain != 0) {
        if (adjust == 0) {
            packetSubmitRequest(396, x + wideOffset + 240, y + 3, 17);
        }
        adjust = 0;
        if (tally != battleResultScoreMax) {
            adjust = row->maxHpGain;
        }
        if (wideOffset != 0) {
            menuDrawHpWideShadow(x + 69, y + 36, resultText->value_5c, (int)(resultText->gauge - adjust));
        } else {
            menuDrawHpShadow(x + 69, y + 36, resultText->value_5c, (int)(resultText->gauge - adjust));
        }
        adjust = tally != battleResultScoreMax ? row->maxEpGain : 0;
        if (wideOffset != 0) {
            menuDrawEpWideShadow(x + 69, y + 52, resultText->value_60, resultText->value_06 - adjust);
        } else {
            menuDrawEpShadow(x + 69, y + 52, resultText->value_60, resultText->value_06 - adjust);
        }
    } else {
        if (wideOffset != 0) {
            menuDrawHpWideShadow(x + 69, y + 36, resultText->value_5c, resultText->gauge);
        } else {
            menuDrawHpShadow(x + 69, y + 36, resultText->value_5c, resultText->gauge);
        }
        if (wideOffset != 0) {
            menuDrawEpWideShadow(x + 69, y + 52, resultText->value_60, resultText->value_06);
        } else {
            menuDrawEpShadow(x + 69, y + 52, resultText->value_60, resultText->value_06);
        }
    }
    fontObjSetColor(RESULT_SHADOW_COLOUR);
    menuDrawStatIcon(5, x + 70, y + 74, 0xFE00ULL << 46);
    fontObjSetColor(0x3F80000080665B6EULL);
    menuDrawStatIcon(5, x + 69, y + 71, 0xFE00ULL << 46);
    fontObjResetColor();
    if (tally == battleResultScoreMax) {
        lowerRowY = y + 75;
        textDrawDecimalAltShadow(x + wideOffset + 117, lowerRowY, valueRecord->amount, 8, 0xFE00ULL << 46);
    } else {
        lowerRowY = y + 75;
        textDrawDecimalAltShadow(x + wideOffset + 117, lowerRowY, battleResultStats[0][index], 8, 0xFE00ULL << 46);
    }
    fontObjSetColor(RESULT_SHADOW_COLOUR);
    scoreRowY = y + 32;
    markerX = x + wideOffset + 199;
    menuDrawStatIcon(6, markerX, scoreRowY, 0xFE00ULL << 46);
    adjust = y + 57;
    menuDrawStatIcon(2, markerX, adjust, 0xFE00ULL << 46);
    menuDrawStatIcon(3, markerX, lowerRowY, 0xFE00ULL << 46);
    fontObjSetColor(0x3F80000080665B6EULL);
    markerFillX = markerX - 1;
    markerY = y + 29;
    menuDrawStatIcon(6, markerFillX, markerY, 0xFE00ULL << 46);
    fontObjSetColor(0x3F800000807F5747ULL);
    menuDrawStatIcon(2, markerFillX, y + 54, 0xFE00ULL << 46);
    menuDrawStatIcon(3, markerFillX, y + 72, 0xFE00ULL << 46);
    fontObjResetColor();
    total = row->gainFirst;
    total = func_002A5C88(total * tally, battleResultScoreMax);
    total += battleResultStats[3][index];
    if (total > BATTLE_RESULT_SCORE_CAP) {
        total = BATTLE_RESULT_SCORE_CAP;
    }
    textDrawDecimalAltShadow(x + wideOffset + 226, scoreRowY, (int)total, 10, 0xFE00ULL << 46);
    if (row->key >= 11) {
        textDrawShadow(cardX + 238, adjust, battleResultScoreDashes, 0xFE00ULL << 46);
        textDrawShadow(cardX + 238, lowerRowY, battleResultScoreDashes, 0xFE00ULL << 46);
        textDrawShadow(cardX + 302, adjust, battleResultSignedDashes, 0xFE00ULL << 46);
        textDrawShadow(cardX + 302, lowerRowY, battleResultSignedDashes, 0xFE00ULL << 46);
    } else {
        unsigned long long part;

        part = row->gainThird;
        part = func_002A5C88(part * tally, battleResultScoreMax) + battleResultStats[1][index];
        if (part > BATTLE_RESULT_PART_CAP) {
            part = BATTLE_RESULT_PART_CAP;
        }
        textDrawDecimalShadow(cardX + 238, adjust, (int)part, 5, 0xFE00ULL << 46);
        part = row->gainThird;
        part = part * (battleResultScoreMax - tally);
        part = func_002A5C88(part, battleResultScoreMax);
        textDrawSignedDecimalShadow(cardX + 300, adjust, (int)part, 5, 0xFE00ULL << 46);
        part = row->gainSecond;
        part = func_002A5C88(part * tally, battleResultScoreMax) + battleResultStats[2][index];
        if (part > BATTLE_RESULT_PART_CAP) {
            part = BATTLE_RESULT_PART_CAP;
        }
        textDrawDecimalShadow(cardX + 238, lowerRowY, (int)part, 5, 0xFE00ULL << 46);
        part = row->gainSecond;
        part = part * (battleResultScoreMax - tally);
        part = func_002A5C88(part, battleResultScoreMax);
        textDrawSignedDecimalShadow(cardX + 300, lowerRowY, (int)part, 5, 0xFE00ULL << 46);
    }
    rest = row->gainFirst;
    rest = rest * (battleResultScoreMax - tally);
    rest = func_002A5C88(rest, battleResultScoreMax);
    textDrawSignedDecimalShadow(x + wideOffset + 300, scoreRowY, (int)rest, 5, 0xFE00ULL << 46);
    menuDrawShortOutlineFrame(x + wideOffset + 191, markerY, 170, 21);
    menuDrawShortOutlineFrame(x + wideOffset + 294, y + 53, 67, 41);
}

#include "main/tu110.h"

/* One 16-byte entry: a (category, id) pair, its repeat count and the highlight flag. */
struct BattleResultEntry {
    int category;
    int id;
    int count;
    int highlight;
};

/* Entry records built by battleResultBuildEntries and their count. */
extern struct BattleResultEntry *battleResultEntries;

/* Pixels between entry rows in the list. */
#define RESULT_ROW_HEIGHT 0x14

extern const unsigned char D_0068B490[];
extern const char battleResultCountFormat[];
int sprintf(unsigned char *dst, const char *format, int count, void *unit);
unsigned char *func_00A9B7D8(int item, int arg);
unsigned char *func_00A9B8D8(int item, int arg);
unsigned char *func_00A9B950(int item, int arg);
unsigned char *func_00A9B9C8(int item, int arg);

/* Draws the entry list: each 16-byte record dispatches by type to a draw helper. */
void battleResultDrawEntries(int x, int y) {
    unsigned char countText[16];
    int i;
    unsigned char *itemText;
    int highlighted;

    i = 0;
    highlighted = 0;
    panelDrawNineSlice(menuPanelFrame, x, y, 0xF6, battleResultEntryCount * RESULT_ROW_HEIGHT + 0x36);
    menuDrawShortOutlineFrame(x + 4, y + 5, 0xEE, 0x1B);
    menuDrawShortOutlineFrame(x + 4, y + 0x22, 0xEE, battleResultEntryCount * RESULT_ROW_HEIGHT + 0xF);
    textDrawSegment(x + 0x40, y + 8, menuMainTxdEntry(BATTLE_RESULT_TXD_HEADING), 0);
    textDrawSegment(x + 0x40, y + 8, D_0068B490, 0);

    for (; i < battleResultEntryCount; i++) {
        itemText = 0;
        switch (battleResultEntries[i].category) {
        case 1:
            itemText = func_00A9B8D8(battleResultEntries[i].id, 0);
            break;
        case 2:
            itemText = func_00A9B7D8(battleResultEntries[i].id, 0);
            break;
        case 3:
            itemText = func_00A9B9C8(battleResultEntries[i].id, 0);
            break;
        case 4:
            itemText = func_00A9B950(battleResultEntries[i].id, 0);
            break;
        }
        if (itemText != 0) {
            textDrawSegment(x + 0x14, y + i * RESULT_ROW_HEIGHT + 0x2A, itemText, 0);
        }
        sprintf(countText, battleResultCountFormat, battleResultEntries[i].count, menuMainTxdEntry(BATTLE_RESULT_TXD_COUNT_UNIT));
        textDrawSegment(x + 0xB6, y + i * RESULT_ROW_HEIGHT + 0x2A, countText, 0);
        if (battleResultEntries[i].highlight != 0) {
            highlighted = 1;
            fontObjSetColor(RESULT_SHADOW_COLOUR);
            packetSubmitRequest(0xC8, x + 0xE7, y + i * RESULT_ROW_HEIGHT + 0x2D, 4);
            fontObjSetColor(RESULT_FILL_COLOUR);
            packetSubmitRequest(0xC8, x + 0xE6, y + i * RESULT_ROW_HEIGHT + 0x2A, 4);
            fontObjResetColor();
        }
    }
    if (highlighted != 0) {
        x -= 0x20;
        panelDrawNineSlice(menuPanelFrame, x, y + battleResultEntryCount * RESULT_ROW_HEIGHT + 0x40, 0x136, 0x25);
        menuDrawShortOutlineFrame(x + 4, y + battleResultEntryCount * RESULT_ROW_HEIGHT + 0x45, 0x12E, 0x1B);
        fontObjSetColor(RESULT_SHADOW_COLOUR);
        packetSubmitRequest(0xC8, x + 0xD, y + battleResultEntryCount * RESULT_ROW_HEIGHT + 0x4B, 4);
        fontObjSetColor(RESULT_FILL_COLOUR);
        packetSubmitRequest(0xC8, x + 0xC, y + battleResultEntryCount * RESULT_ROW_HEIGHT + 0x48, 4);
        fontObjResetColor();
        textDrawSegment(x + 0x18, y + battleResultEntryCount * RESULT_ROW_HEIGHT + 0x48, menuMainTxdEntry(0xE7), 0);
    }
}

/* Result screen: RESULT and ClearTime labels, clear time split into minutes and seconds. */
void battleResultDrawScreen(void)
{
    int clearFrames;
    int clearSeconds;
    int minutes;
    int state;
    int rows;
    int emptyRows;
    int rowX;
    int rowY;
    int i;
    int tally;

    fontObjResetAndBuild();
    textDrawSegment(0, 0, battleResultTextSetup, 0);
    panelDrawNineSlice(menuPanelFrame, 4, 8, 84, 37);
    menuDrawShortOutlineFrame(8, 13, 76, 27);
    panelDrawNineSlice(menuPanelFrame, 155, 20, 201, 37);
    menuDrawShortOutlineFrame(159, 25, 193, 27);
    fontObjSetColor(0x3F80000080808080ULL);
    textDrawShadow(18, 19, battleResultTitle, 0xFE00ULL << 46);
    textDrawShadow(167, 31, battleResultClearTimeLabel, 0xFE00ULL << 46);
    textDrawShadow(247, 31, battleResultTimeTemplate, 0xFE00ULL << 46);
    clearFrames = battleResultClearFrames;
    if (clearFrames > BATTLE_RESULT_CLEAR_TIME_CAP) {
        clearFrames = BATTLE_RESULT_CLEAR_TIME_CAP;
    }
    clearSeconds = clearFrames / BATTLE_RESULT_FRAMES_PER_SECOND;
    textDrawDecimalShadowZero(325, 31, clearSeconds % 60, 2, 0);
    minutes = clearSeconds / 60;
    textDrawDecimalShadowZero(293, 31, minutes % 60, 2, 0);
    textDrawDecimalShadow(261, 31, minutes / 60, 2, 0);
    fontObjResetColor();

    state = battleResultState;
    if (state >= BATTLE_RESULT_STATE_ENTRY_LIST) {
        if (D_0069D91C == 0) {
            menuDrawKindIcon(414, 410, 3);
            textDrawSegment(434, 410, menuMainTxdEntry(232), 0);
        }
        battleResultDrawEntries(133, 140);
        return;
    }

    rows = 3;
    if (battleResultMemberCount < 4) {
        rows = battleResultMemberCount;
    }
    tally = state != 0 ? battleResultStateTimer : 0;
    if (state == BATTLE_RESULT_STATE_SUMMARY) {
        if (battleResultEntryCount == 0) {
            if (D_0069D91C == 0) {
                menuDrawKindIcon(414, 410, 3);
                textDrawSegment(434, 410, menuMainTxdEntry(232), 0);
            }
        } else {
            menuDrawKindIcon(414, 410, 3);
            textDrawSegment(434, 410, menuMainTxdEntry(233), 0);
        }
    }
    i = 0;
    if (rows > 0) {
        emptyRows = 3 - rows;
        rowX = 11 * emptyRows + 47;
        rowY = 48 * emptyRows + 64;
        for (; i < rows; i++) {
            battleResultDrawRow(rowX, rowY, i, tally);
            rowX += 22;
            rowY += 111;
        }
    }
}

/* Tail call to the result screen (battleResultDrawScreen). */
void battleResultDrawCallback(void) {
    battleResultDrawScreen();
}

/* Builds the 16-byte entry records at battleResultEntries from the table at a0 (count at +0x4C). */
void battleResultBuildEntries(struct BattleResult *result) {
    int entryCount = 0;
    int i;
    int j;

    for (i = 0; i < result->count; i++) {
        int found = 0;

        for (j = 0; j < entryCount; j++) {
            if (battleResultEntries[j].category == result->categories[i] && battleResultEntries[j].id == result->ids[i]) {
                battleResultEntries[j].count++;
                found = 1;
                break;
            }
        }
        if (!found) {
            battleResultEntries[entryCount].category = result->categories[i];
            battleResultEntries[entryCount].id = result->ids[i];
            battleResultEntries[entryCount].count = 1;
            entryCount++;
        }
    }
    battleResultEntryCount = entryCount;
    for (i = 0; i < battleResultEntryCount; i++) {
        if (battleResultEntries[i].count + dataBoxChk(battleResultEntries[i].category, battleResultEntries[i].id) >= 100) {
            battleResultEntries[i].highlight = 1;
        }
    }
}

#include "main/pad.h"
#include "main/tu022.h"
#include "main/tu092.h"
#include "main/tu096.h"

/* Views of the structures the scene loop reads; each keeps only the members it uses. */
typedef struct SceneClearEnv {
    unsigned char unmodeled_00[0x20];
    unsigned int color_r;
    unsigned int color_g;
    unsigned int color_b;
    unsigned int color_a;
} SceneClearEnv;

typedef struct SceneStats {
    int w0;
    int w4;
    int w8;
    int wC;
} SceneStats;

typedef struct SceneMember {
    unsigned char unmodeled_00[8];
    SceneStats *stats;
} SceneMember;

extern SceneClearEnv ClearEnv;
extern PadRecord PadData[];
void xglCdArenaOpen(int size, long long handle);
void *xglCdSectorAlloc(int size);
int ov01SceneFrameCount(void);
int ptGroupCount(void);
void fontObjLoadTexture(int index);
void bgLayerLoadAll(void);
void xglCdWaitIdle(void);
void bgLayerCameraSetup(void);
void GameResArenaReload(void);
void fadeStartIn(int frames);
void xglPadSetRepeat(int port, int mask, int a, int b);
void pxyTickScaleDoubleOverride(int enable);
int xglCdServiceIfPending(void);
void seVoiceStartDefault(int voice);
void XenoActorSetFlagAll(int flag);
void bgLayerClearAllFlags(void);
void XenoActorDoit(void);
void XenoActorDisp(void);
void xglTickAdvance(void);
void xglSleep(void);
void xglRenderClearFrame(void);
void fadeStartOut(int frames);
void battleReviveDownedParty(void);
void bgLayerResetState(void);
void bgLayerRelease(void);
void xglCdArenaClose(void);
void GameResArenaRelease(void);
void fadeReset(void);
void *memset(void *dest, int value, unsigned int count);

/* Scene loop: clears the environment, builds the entries, runs frames until done. */
void battleResultScene(void)
{
    StatOwner **snapshots;
    int slotCount;
    int i;
    StatOwner *slot;
    SceneMember *member;
    struct BattleResultRow *rec;
    int bonusTotal;
    int scoreTotal;
    int exitRequested;
    PadRecord *pad;
    int stateTimer;

    i = 0;
    exitRequested = 0;
    xglCdArenaOpen(0, 0);
    ClearEnv.color_r = 0;
    ClearEnv.color_g = 0;
    ClearEnv.color_b = 0;
    ClearEnv.color_a = 0;
    xglRenderClearFrame();
    xglSleep();
    battleResultClearFrames = ov01SceneFrameCount();

    battleResultEntries = xglCdSectorAlloc(128);
    memset(battleResultEntries, 0, 128);
    battleResultRows = xglCdSectorAlloc(280);
    memset(battleResultRows, 0, 280);
    battleResultStats = xglCdSectorAlloc(64);
    memset(battleResultStats, 0, 64);

    battleResultMemberCount = ptGroupCount();
    slotCount = partySnapshotGet(&snapshots);
    if (slotCount < battleResultMemberCount) {
        battleResultMemberCount = slotCount;
    }

    if (battleResultMemberCount > 0) {
        i = 0;
        do {
            slot = snapshots[i];
            if (slot != 0) {
                member = ptFindRecord(slot->stats->key);
                if (member != 0) {
                    int (*table)[4] = battleResultStats;
                    table[0][i] = member->stats->w4;
                    table[1][i] = member->stats->wC;
                    table[2][i] = member->stats->w8;
                    table[3][i] = member->stats->w0;
                }
            }
            i++;
        } while (i < battleResultMemberCount);
    }

    battleResultBuildEntries(battleResultGet());
    battleResultApply(battleResultRows);

    if (battleResultMemberCount != 0) {
        bonusTotal = 0;
        battleResultScoreMax = 0;
        scoreTotal = 0;
        if (battleResultMemberCount > 0) {
            rec = battleResultRows;
            i = battleResultMemberCount;
            do {
                bonusTotal += rec->levelGain;
                if (battleResultScoreMax < rec->gainFirst) {
                    battleResultScoreMax = rec->gainFirst;
                }
                if (battleResultScoreMax < rec->gainSecond) {
                    battleResultScoreMax = rec->gainSecond;
                }
                if (battleResultScoreMax < rec->gainThird) {
                    battleResultScoreMax = rec->gainThird;
                }
                scoreTotal += rec->gainFirst + rec->gainSecond + rec->gainThird;
                rec++;
                i--;
            } while (i != 0);
        }

        i = 0;
        fontObjLoadTexture(0);
        fontObjLoadTexture(3);
        bgLayerLoadAll();
        xglCdWaitIdle();
        bgLayerCameraSetup();
        GameResArenaReload();
        fadeStartIn(16);
        xglPadSetRepeat(0, PAD_CIRCLE, 8, 1);
        pxyTickScaleDoubleOverride(1);
        battleResultState = BATTLE_RESULT_STATE_COUNT_UP;
        battleResultStateTimer = 0;
        if (battleResultScoreMax <= 0) {
            battleResultScoreMax = 1;
        }

        pad = &PadData[0];
        while (1) {
            D_0069D91C = (signed char) xglCdServiceIfPending();
            if (((pad->directionPressed & PAD_START) != 0
                    && (pad->held & PAD_SOFT_RESET_HELD) == PAD_SOFT_RESET_HELD)
                || (gameWork.control & CONTROL_SOFT_RESET) != 0) {
                gameWork.control |= CONTROL_SOFT_RESET;
                soundSlotsRestart();
                soundSlotReopen(0);
                break;
            }
            if (exitRequested != 0) {
                soundSlotSetVolume(0, 0, 8);
                seVoiceStartDefault(0x01000002);
                break;
            }

            switch (battleResultState) {
            case BATTLE_RESULT_STATE_COUNT_UP:
                stateTimer = battleResultStateTimer + 1;
                battleResultStateTimer = stateTimer;
                if (stateTimer >= BATTLE_RESULT_COUNT_FRAMES || (pad->unmodeled_2e & PAD_CIRCLE) != 0
                    || (pad->heldAndPressed & 0x2000200000ULL) == 0x2000000000ULL) {
                    battleResultState = BATTLE_RESULT_STATE_TALLY;
                    battleResultStateTimer = 0;
                    if (scoreTotal != 0) {
                        seVoiceStartDefault(0x0100000A);
                    }
                }
                break;
            case BATTLE_RESULT_STATE_TALLY:
                battleResultStateTimer += 1;
                if ((pad->held & PAD_CIRCLE) != 0) {
                    battleResultStateTimer = battleResultStateTimer + (battleResultScoreMax >> 5) + 1;
                }
                if (battleResultScoreMax < battleResultStateTimer) {
                    battleResultStateTimer = battleResultScoreMax;
                    battleResultState = BATTLE_RESULT_STATE_SUMMARY;
                }
                if (scoreTotal != 0 && (xglTickGet() & 3) == 0) {
                    seVoiceStartDefault(0x0100000A);
                }
                break;
            case BATTLE_RESULT_STATE_SUMMARY:
                if ((pad->directionPressed & PAD_CIRCLE) != 0 || i >= 30) {
                    if (battleResultEntryCount == 0) {
                        exitRequested = 1;
                    } else {
                        battleResultState = BATTLE_RESULT_STATE_ENTRY_LIST;
                        battleResultStateTimer = 0;
                        seVoiceStartDefault(0x01000002);
                    }
                } else {
                    i = (pad->held & PAD_CIRCLE) != 0 ? i + 1 : 0;
                }
                if (bonusTotal != 0) {
                    bonusTotal = 0;
                    seVoiceStartDefault(0x01000009);
                    seVoiceStartDefault(0x01000009);
                    seVoiceStartDefault(0x01000009);
                }
                break;
            case BATTLE_RESULT_STATE_ENTRY_LIST:
                if ((pad->pressed & PAD_CIRCLE) != 0) {
                    exitRequested = 1;
                } else if ((pad->repeated & PAD_CIRCLE) != 0) {
                    battleResultStateTimer = battleResultStateTimer + 1;
                    exitRequested = 0;
                    if (battleResultStateTimer >= 16) {
                        exitRequested = 1;
                    }
                }
                break;
            }

            XenoActorSetFlagAll(2);
            bgLayerClearAllFlags();
            xglCallbackQueueAdd((int) battleResultDrawCallback);
            XenoActorDoit();
            XenoActorDisp();
            xglTickAdvance();
            xglSleep();
        }

        xglPadSetRepeat(0, PAD_UP | PAD_RIGHT | PAD_DOWN | PAD_LEFT | PAD_L1 | PAD_R1, 8, 1);
        if ((gameWork.control & CONTROL_SOFT_RESET) == 0) {
            fadeStartOut(15);
            for (i = 0; i < 16; i++) {
                XenoActorSetFlagAll(2);
                bgLayerClearAllFlags();
                xglCallbackQueueAdd((int) battleResultDrawCallback);
                XenoActorDoit();
                XenoActorDisp();
                xglTickAdvance();
                xglSleep();
            }
        }
        battleReviveDownedParty();
        bgLayerResetState();
        bgLayerRelease();
        xglCdArenaClose();
        pxyTickScaleDoubleOverride(0);
        if ((gameWork.control & CONTROL_SOFT_RESET) == 0) {
            GameResArenaRelease();
            fadeReset();
            fadeStartIn(30);
        }
    }
}

const unsigned char D_0068B490[16] = "$han1;$prop1;";
const unsigned char battleResultTextSetup[64] = "$init;$bg2;$shadow+1,+3,000000;$zoom13;$han1;$prop1;$colc0c0c0;";
const unsigned char battleResultClearTimeLabel[16] = "ClearTime";
const unsigned char battleResultTimeTemplate[16] = "/      :      :";
const char D_0068B500[16] = "EXPLANATION";
int battleResultState = 0;
int battleResultStateTimer = 0;
int battleResultScoreMax = 0;
int battleResultMemberCount = 0;
struct BattleResultRow *battleResultRows = 0;
struct BattleResultEntry *battleResultEntries = 0;
int battleResultEntryCount = 0;
int (*battleResultStats)[4] = 0;
int battleResultClearFrames = 0;
signed char D_0069D91C = 0;
unsigned char battleResultScoreDashes[8] = "-----";
unsigned char battleResultSignedDashes[8] = "------";
const char battleResultCountFormat[8] = "%d%s";
unsigned char battleResultTitle[8] = "RESULT";
char D_0069D940[8] = "STATUS";
unsigned int D_0069D948[2] = { 0 };
char D_0069D950[8] = "W ETHER";
char D_0069D958[8] = "SPECIAL";
char D_0069D960[8] = "DOUBLE";
