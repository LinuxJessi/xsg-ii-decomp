#include "common.h"
#include "main/tu097.h"
#include "main/tu110.h"

INCLUDE_ASM("asm/main/nonmatchings/tu126", saveProgressDraw);

#include "shared.h"

/* One 0x7920-byte save slot of the table at saveSlotTable, 20 slots in all. Word 0x7900 is the slot status
 * (below 2 reads as NO DATA), 0x7910 its saved time, 0x7918/0x791C two text values; only the touched words are named. */
typedef struct SaveSlot {
    int active;
    unsigned char unmodeled_04[0x7900 - 0x04];
    int state;
    unsigned char unmodeled_7904[0x7920 - 0x7904];
} SaveSlot;

extern unsigned char *saveSlotTable;

void saveSlotsDeactivateAll(void) {
    SaveSlot *record;
    int index;

    record = (SaveSlot *) (saveSlotTable + 0x97660);
    for (index = 0; index < 20; index++) {
        record->active = 0;
        record--;
    }
}

void saveSlotsActivateAll(void) {
    SaveSlot *record;
    int index;

    record = (SaveSlot *) (saveSlotTable + 0x97660);
    for (index = 0; index < 20; index++) {
        record->active = 1;
        record--;
    }
}

void func_00237A70(void) {
    SaveSlot *record;
    int index;

    record = (SaveSlot *) saveSlotTable;
    for (index = 0; index < 20; index++) {
        if (record->state == 1) {
            record->state = 3;
        }
        record++;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu126", saveSlotSetTexts);

INCLUDE_ASM("asm/main/nonmatchings/tu126", saveLabelBuild);

/* Empties the SJIS string at text (its first byte becomes the terminator). Callers clear D_006ABF78 before
   sjisStrcat appends to it. */
void sjisStrClear(signed char *text) {
    *text = 0;
}

INCLUDE_ASM("asm/main/nonmatchings/tu126", sjisStrcat);

INCLUDE_ASM("asm/main/nonmatchings/tu126", sjisCharCount);

/* The sub-object at offset 4 of a SaveItem: initialised by cursorBufferInit and set by cursorBufferSetOffset;
 * its first word is read back by saveItemGetValue (saveItemGetValue). */
typedef struct SaveItemSub {
    int value;
    unsigned char unmodeled_04[0x14 - 0x04];
} SaveItemSub;

typedef struct SaveItem {
    int id;
    SaveItemSub sub;
    signed char flag;
} SaveItem;

void cursorBufferInit();
void cursorBufferSetOffset();

void saveItemInit(SaveItem *item, int value, int id) {
    item->id = id;
    cursorBufferInit(&item->sub, 2, 2, 0);
    cursorBufferSetOffset(&item->sub, value);
    item->flag = 1;
}

int saveItemGetValue(SaveItem *item) {
    return item->sub.value;
}

/* Partial view of an option item: its label text, the on/off toggle and the animation step.
 * saveOptionItemDraw draws it and advances the step. */
typedef struct SaveOptionItem {
    unsigned char *text;
    int toggle;
    unsigned char unmodeled_08[0x10];
    signed char anim_step;
} SaveOptionItem;

void textDrawSegment(int, int, void *, int);
void panelDrawNineSlice();
void menuDrawTitledFrame();
void menuDrawShortOutlineFrame();
extern unsigned char D_003EB290[];
extern unsigned char D_0068B688[];
extern unsigned char D_0068B698[];
extern unsigned char D_0068B6D8[];

/* Draws one option item at (origin_x, origin_y): its label, then the image for the toggle (0x208 when the
 * toggle is clear, 0x209 when it is set), and advances the animation step while it is non-zero. */
void saveOptionItemDraw(SaveOptionItem *item, int origin_x, int origin_y)
{
    int label_x;
    int value_x;
    int magnitude;
    signed char anim_step;
    unsigned char step;
    unsigned char *first_image;
    unsigned char *second_image;

    label_x = origin_x + 4;
    anim_step = item->anim_step;
    magnitude = anim_step;
    if (anim_step <= -1)
    {
        magnitude = -magnitude;
    }
    value_x = origin_x + 0x3C;
    if (anim_step != 0)
    {
        fontObjSetOpacity(magnitude * 0x10);
        panelDrawNineSlice(D_003EB290, origin_x, origin_y, 0xB6, 0x68);
        menuDrawTitledFrame(label_x, origin_y + 5, 0xAE, 0x26, D_0068B688);
        menuDrawShortOutlineFrame(label_x, origin_y + 0x2D, 0xAE, 0x36);
        textDrawSegment(0, 0, D_0068B698, 0);
        textDrawSegment(0, 0, D_0068B6D8, 0);
        textDrawSegment(origin_x + 0x10, origin_y + 0x13, item->text, 4);
        first_image = menuMainTxdEntry(0x208);
        textDrawSegment(value_x, origin_y + 0x32, first_image, (1 - item->toggle) | 4);
        second_image = menuMainTxdEntry(0x209);
        textDrawSegment(value_x, origin_y + 0x48, second_image, item->toggle | 4);
        fontObjResetOpacity();
        step = item->anim_step;
        if (step & 7)
        {
            item->anim_step = (signed char) (step + 1);
        }
    }
}

void saveOptionItemFadeOut(SaveItem *item) {
    item->flag = -7;
}

INCLUDE_ASM("asm/main/nonmatchings/tu126", func_00238038);

/* One row of the slot list: seven words, the first is its draw value. Rows are at saveSlotRows, stride 0x1C. */
typedef struct SaveSlotRow {
    int value;
    unsigned char unmodeled_04[0x1C - 0x04];
} SaveSlotRow;

extern unsigned char D_0069DA15;
extern unsigned char *saveSlotRows;
extern int saveCursor[];

/* Lays out the 20 slot rows: the cursor row (saveCursor[0]) gets 0xC, the three rows from `band` get 0x1E,
 * every other row 0x200. */
void saveSlotRowsLayout(void)
{
    SaveSlotRow *row;
    unsigned char band;
    int index;

    row = (SaveSlotRow *) (saveSlotRows + 8);
    band = D_0069DA15;
    index = 0;
    do
    {
        if (index == saveCursor[0])
        {
            row->value = 0xC;
        }
        else if ((index >= (signed char) band) && (index < (signed char) band + 3))
        {
            row->value = 0x1E;
        }
        else
        {
            row->value = 0x200;
        }
        index += 1;
        row += 1;
    }
    while (index < 0x14);
}

INCLUDE_ASM("asm/main/nonmatchings/tu126", func_002381A8);

INCLUDE_ASM("asm/main/nonmatchings/tu126", saveUiInit);

INCLUDE_ASM("asm/main/nonmatchings/tu126", saveUiDraw);

INCLUDE_ASM("asm/main/nonmatchings/tu126", func_002388D0);

INCLUDE_ASM("asm/main/nonmatchings/tu126", func_00238A68);

INCLUDE_ASM("asm/main/nonmatchings/tu126", saveSlotDrawDetail);

INCLUDE_ASM("asm/main/nonmatchings/tu126", saveSlotListDraw);

void saveOptionItemDraw(SaveOptionItem *item, int origin_x, int origin_y);
extern unsigned char saveUiOptionItem[];

void saveUiOptionDraw(void) {
    saveOptionItemDraw(saveUiOptionItem, 0xA5, 0xCE);
}

INCLUDE_ASM("asm/main/nonmatchings/tu126", saveScreenDraw);

int saveScreenDraw();

void saveScreenDrawCallback(void) {
    saveScreenDraw();
}

INCLUDE_ASM("asm/main/nonmatchings/tu126", saveMenuUpdate);

INCLUDE_ASM("asm/main/nonmatchings/tu126", saveSlotFromData);

INCLUDE_ASM("asm/main/nonmatchings/tu126", func_00239F98);

INCLUDE_ASM("asm/main/nonmatchings/tu126", func_0023A0C8);

INCLUDE_ASM("asm/main/nonmatchings/tu126", func_0023A608);

int padCirclePressedAlone();
int padCrossPressedAlone();
int func_002381A8();
extern signed char D_0069DA1C;

int saveUiPollButtons(void) {
    D_0069DA1C = 1;
    if ((padCirclePressedAlone() != 0) || (padCrossPressedAlone() != 0)) {
        D_0069DA1C = 0;
        func_002381A8(0);
    }
    return 0;
}

INCLUDE_ASM("asm/main/nonmatchings/tu126", func_0023AC08);

INCLUDE_ASM("asm/main/nonmatchings/tu126", func_0023AD18);

INCLUDE_ASM("asm/main/nonmatchings/tu126", saveTextStateReset);

INCLUDE_ASM("asm/main/nonmatchings/tu126", configScreenDraw);

/* Six per-channel maxima, each raised to the matching byte of the source entry (bytes 0 and 4 are not used). */
extern signed char saveTextMaxima[6];

void saveTextRaiseMaxima(unsigned char *entry)
{
    unsigned char value;

    value = entry[1];
    if (saveTextMaxima[0] < value) {
        saveTextMaxima[0] = value;
    }
    value = entry[2];
    if (saveTextMaxima[1] < value) {
        saveTextMaxima[1] = value;
    }
    value = entry[3];
    if (saveTextMaxima[2] < value) {
        saveTextMaxima[2] = value;
    }
    value = entry[5];
    if (saveTextMaxima[3] < value) {
        saveTextMaxima[3] = value;
    }
    value = entry[6];
    if (saveTextMaxima[4] < value) {
        saveTextMaxima[4] = value;
    }
    if (saveTextMaxima[5] < entry[7]) {
        saveTextMaxima[5] = entry[7];
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu126", func_0023B8B8);

INCLUDE_ASM("asm/main/nonmatchings/tu126", func_0023BA10);

INCLUDE_ASM("asm/main/nonmatchings/tu126", saveTextMaximaCopy);

INCLUDE_ASM("asm/main/nonmatchings/tu126", func_0023C308);
