#include "common.h"

extern unsigned char textMarkerFound;
extern unsigned char *textMarkerPtrs[];

#define PAGE_MARKER_LEAD 0xA1
#define PAGE_MARKER_TRAIL 0xFC
#define PAGE_MARKER_FILLER 7
#define FONT_COLOR_DEFAULT 0xFFFFF0
#define BOX_OPEN_STEPS 4 /* opening box: tick 0..4 scales the size */
#define CHOICE_VALUE_UNIT 0x10 /* ChoiceMenu.flags: values carry a unit mark */
#define CHOICE_ITEM_HIGHLIGHT 2 /* ChoiceMenu.itemFlags: highlighted item */

/* Cuts text at its page-th 0xA1 0xFC page marker: every marker before it becomes two 0x07 bytes and every
 * marker position is recorded for textMarkerRestore. */
void textMarkerScan(unsigned char *text, unsigned char page)
{
    int found;

    if (text == 0 || page == 0) {
        return;
    }
    found = 0;
    while (*text != 0) {
        if (text[0] == PAGE_MARKER_LEAD && text[1] == PAGE_MARKER_TRAIL) {
            found++;
            textMarkerPtrs[textMarkerFound] = text;
            textMarkerFound++;
            if (found == page) {
                *text = 0;
                break;
            }
            text[0] = PAGE_MARKER_FILLER;
            text[1] = PAGE_MARKER_FILLER;
            text += 2;
        } else {
            if ((signed char)text[0] < 0) {
                text++;
            }
            text++;
        }
    }
}

/* Writes the 0xA1 0xFC page marker back at every position textMarkerScan recorded, then clears the count. */
void textMarkerRestore(void) {
    int markerIndex;

    markerIndex = 0;
    if (textMarkerFound != 0) {
        do {
            textMarkerPtrs[markerIndex][0] = PAGE_MARKER_LEAD;
            textMarkerPtrs[markerIndex][1] = PAGE_MARKER_TRAIL;
            markerIndex++;
        } while (markerIndex < textMarkerFound);
    }
    textMarkerFound = 0;
}

INCLUDE_ASM("asm/main/nonmatchings/tu125", textMarkerCount);

INCLUDE_ASM("asm/main/nonmatchings/tu125", textEntryUpdate);

INCLUDE_ASM("asm/main/nonmatchings/tu125", textDrawControlCode);

INCLUDE_ASM("asm/main/nonmatchings/tu125", func_00236890);

INCLUDE_ASM("asm/main/nonmatchings/tu125", textDrawString);

#include "main/tu097.h"
#include "main/tu101.h"
#include "main/tu110.h"
#include "main/xgl_font.h"

/* Text window: the box at the bottom of the screen and its single line of text. */
typedef struct TextWindow {
    int x;
    int y;
    int unmodeled_08[5];
    int width;
    int fade;              /* +0x20 frames of the fade-in, opaque from 8 on */
    unsigned char *text;
} TextWindow;

/* One of the four text entries: an opening box with a list of pages. */
typedef struct TextEntry {
    int x;
    int y;
    int width;
    int height;
    unsigned char **pages;
    int page;
    unsigned char tick;    /* +0x18 opening step, 0..4 */
    unsigned char state;   /* +0x19 0 free, 1 open, other values opening or closing */
    unsigned char unmodeled_1a[2];
    char *title;           /* +0x1C box title, NULL for none */
    unsigned char marker;  /* +0x20 nonzero: the page holds page markers */
    unsigned char unmodeled_21[3];
} TextEntry;

/* One of the two choice menus. */
typedef struct ChoiceMenu {
    int x;
    int y;
    int width;
    int height;
    unsigned char **items;
    short *values;         /* +0x14 number printed after each item, NULL for none */
    unsigned char *itemFlags; /* +0x18 bit 1 highlights the item */
    unsigned int flags;    /* +0x1C bit 4: values carry a unit mark */
    int unmodeled_20[2];
    int rowOffset;         /* +0x28 scroll offset of the first row */
    int unmodeled_2c[5];
    unsigned int selected;
    int unmodeled_44;
    unsigned int count;
    unsigned int columns;
    int unmodeled_50;
    unsigned char tick;
    unsigned char state;
    unsigned char unmodeled_56[2];
    char *title;
} ChoiceMenu;

/* The ANALYZE panel with the stats of the analysed enemy. */
typedef struct AnalyzePanel {
    int x;
    int y;
    unsigned char tick;
    unsigned char state;
    unsigned char unmodeled_0a[2];
    int hp;                /* +0x0C negative when unknown */
    int maxHp;
    int unmodeled_14[2];
    int exp;
    int skillPoints;
    int classPoints;
    int rareItem;          /* +0x28 0 none, negative unknown */
    int item;
    signed char attackType;
    unsigned char attributes;
    signed char rareItemKind;
    signed char itemKind;
} AnalyzePanel;

extern TextWindow textWindow;
extern TextEntry textEntryPool[4];
extern ChoiceMenu choiceMenuSlots[2];
extern AnalyzePanel textBlock;

extern char D_0068B588[];
extern char D_0068B5B8[];
extern char D_0068B5F8[];
extern char D_0068B608[];
extern char D_0068B618[];
extern char D_0068B628[];
extern char D_0069D948[];
extern char D_0069D968[];
extern char D_0069D988[];
extern char D_0069D990[];
extern char D_0069D998[];
extern char D_0069D9A0[];
extern char D_0069D9A8[];
extern char D_0069D9B0[];
extern char D_0069D9B8[];
extern char D_0069D9C0[];
extern char D_0069D9C8[];
extern char D_0069D9D0[];
extern char D_0069D9D8[];
extern char D_0069D9E0[];
extern char D_0069D9E8[];
extern char D_0069D9F0[];
extern char D_0069D9F8[];
extern char D_0069DA00[];
extern char D_0069DA08[];

void xglFontFlush(void);
int xglCdArenaGetSize(void);
void fontObjResetState(void);
void fontObjSetAlpha(int alpha);
void textDrawSegment(int x, int y, void *text, int style);
void textDrawString(int x, int y, unsigned char *text);
void menuDrawTitledWindow(int x, int y, int width, int height, char *title);
void grSetScissor(int x0, int y0, int x1, int y1);
void grResetScissor(void);
int textEncodeNewlines(unsigned char *text);
/* sprintf-like: formats one integer value into buffer. */
void sprintf(char *buffer, const char *format, int value);
/* ov01: name of an item by kind (kind 2 and the others). */
char *func_00A9B7D8(int item, int arg);
char *func_00A9B8D8(int item, int arg);

/* Draws the text window, the text entries, the choice menus and the ANALYZE panel. */
void textDrawAll(void)
{
    char buffer[16];
    int slot;
    int needSeparator;
    TextEntry *entry;
    ChoiceMenu *menu;
    AnalyzePanel *panel;
    int x;
    int y;
    int width;
    int height;
    int top;
    int bottom;
    int i;
    int columnWidth;
    int style;
    int valueX;

    needSeparator = 0;
    xglFontFlush();
    if (xglCdArenaGetSize()) {
        fontObjResetState();
        packetSubmitRequest(0x2BE, 0x200, 0x1C0, 0x10000);
    }
    if (textWindow.fade > 0) {
        fontObjResetState();
        fontObjSetAlpha(0x40);
        if (textWindow.fade < 8) {
            fontObjSetOpacity(textWindow.fade << 4);
        }
        packetSubmitRequestEx(0x2BE, textWindow.x, textWindow.y, textWindow.width, 0x20, 0x10104);
        if (textWindow.text) {
            textDrawSegment(0, 0, D_0068B588, 0);
            textDrawString(textWindow.x + 12, textWindow.y + 6, textWindow.text);
        }
        fontObjResetColor();
        fontObjResetOpacity();
        xglFontFlush();
    }

    for (slot = 0; slot < 4; slot++) {
        if (textEntryPool[slot].state == 0) {
            continue;
        }
        entry = &textEntryPool[slot];
        if (entry->state == 1) {
            x = entry->x;
            y = entry->y;
            width = entry->width;
            height = entry->height;
        } else {
            width = entry->width * entry->tick / BOX_OPEN_STEPS;
            height = entry->height * entry->tick / BOX_OPEN_STEPS;
            x = entry->x + ((entry->width - width) >> 1);
            y = entry->y + ((entry->height - height) >> 1);
            fontObjSetOpacity(entry->tick << 5);
        }
        fontObjResetState();
        menuDrawTitledWindow(x, y, width, height, entry->title);
        fontObjResetOpacity();
        grSetScissor(x + 5, entry->title == 0 ? y + 5 : y + 16, x + width + 26, y + height + 25);
        if (entry->pages && entry->pages[entry->page]) {
            if (entry->marker) {
                textMarkerScan(entry->pages[entry->page], entry->marker);
            }
            textDrawSegment(0, 0, D_0068B588, 0);
            textEncodeNewlines(entry->pages[entry->page]);
            textDrawString(entry->x + 8, entry->y + (entry->title == 0 ? 8 : 19),
                           entry->pages[entry->page]);
            if (entry->marker) {
                textMarkerRestore();
            }
        }
        xglFontFlush();
        grResetScissor();
    }

    for (slot = 0; slot < 2; slot++) {
        if (choiceMenuSlots[slot].state == 0) {
            continue;
        }
        menu = &choiceMenuSlots[slot];
        if (menu->state == 1) {
            x = menu->x;
            y = menu->y;
            width = menu->width;
            height = menu->height;
        } else {
            width = menu->width * menu->tick / BOX_OPEN_STEPS;
            height = menu->height * menu->tick / BOX_OPEN_STEPS;
            x = menu->x + ((menu->width - width) >> 1);
            y = menu->y + ((menu->height - height) >> 1);
            fontObjSetOpacity(menu->tick << 5);
        }
        top = y + 5;
        fontObjResetState();
        menuDrawTitledWindow(x, y, width, height, menu->title);
        fontObjResetOpacity();
        if (menu->title) {
            top = y + 16;
        }
        grSetScissor(x + 5, top, x + width + 26, y + height + 25);
        if (menu->items) {
            textDrawSegment(0, 0, D_0068B5B8, 0);
            i = 0;
            x = menu->x + 24;
            y = menu->y + menu->rowOffset + (menu->title == 0 ? 6 : 17);
            columnWidth = menu->width / menu->columns;
            if (menu->count != 0) {
                bottom = top + height + 25;
                do {
                    if (top < y + 20 && y < bottom) {
                        /* bit 0: the cursor row; bit 1: a highlighted item */
                        style = i == menu->selected;
                        if (menu->itemFlags) {
                            style = (menu->itemFlags[i] & CHOICE_ITEM_HIGHLIGHT) | (i == menu->selected);
                        }
                        if (menu->items[i]) {
                            textDrawSegment(x, y, menu->items[i], style);
                        }
                        if (menu->values) {
                            valueX = x + columnWidth + ((menu->flags & CHOICE_VALUE_UNIT) == 0 ? -48 : -68);
                            sprintf(buffer, D_0069D988, menu->values[i]);
                            textDrawSegment(0, 0, D_0069D990, 0);
                            textDrawSegment(0, 0, D_0068B5F8, 0);
                            textDrawSegment(valueX, y - 2, buffer, style);
                            textDrawSegment(0, 0, D_0069D998, 0);
                            textDrawSegment(0, 0, D_0068B608, 0);
                            if (menu->flags & CHOICE_VALUE_UNIT) {
                                textDrawSegment(valueX + 28, y, D_0069D9A0, style);
                            }
                        }
                    }
                    if (i % menu->columns == menu->columns - 1) {
                        x = menu->x + 24;
                        y += 24;
                    } else {
                        x += columnWidth;
                    }
                    i++;
                } while (i < menu->count);
            }
        }
        xglFontFlush();
        grResetScissor();
    }

    panel = &textBlock;
    if (panel->state == 0) {
        return;
    }
    if (panel->state == 1) {
        x = panel->x;
        width = 256;
        y = panel->y;
        height = 192;
    } else {
        width = 256 * panel->tick / BOX_OPEN_STEPS;
        height = 192 * panel->tick / BOX_OPEN_STEPS;
        x = panel->x + ((256 - width) >> 1);
        y = panel->y + ((192 - height) >> 1);
        fontObjSetOpacity(panel->tick << 5);
    }
    fontObjResetState();
    menuDrawTitledWindow(x, y, width, height, D_0069D9A8);
    fontObjResetOpacity();
    grSetScissor(x + 5, y + 5, x + width + 26, y + height + 25);
    x = panel->x;
    y = panel->y;
    textDrawSegment(0, 0, D_0068B588, 0);
    textDrawSegment(0, 0, D_0068B618, 0);
    textDrawSegment(x + 16, y + 16, D_0069D9B0, 0);
    textDrawSegment(x + 16, y + 40, D_0069D9B8, 0);
    textDrawSegment(x + 16, y + 64, D_0069D9C0, 0);
    textDrawSegment(x + 16, y + 92, D_0069D9C8, 0);
    textDrawSegment(x + 16, y + 116, D_0069D9D0, 0);
    textDrawSegment(x + 16, y + 140, D_0069D9D8, 0);
    textDrawSegment(x + 16, y + 164, D_0069D9E0, 0);
    textDrawSegment(x + 16, y + 188, D_0069D968, 0);
    textDrawSegment(0, 0, D_0068B628, 0);

    if (panel->hp >= 0) {
        sprintf(buffer, D_0069D9E8, panel->hp);
        textDrawSegment(x + 80, y + 16, buffer, 0);
    } else {
        textDrawSegment(x + 80, y + 16, D_0069D9F0, 0);
    }
    xglFontPrintDirectOT(FONT_COLOR_DEFAULT, D_0069D9F8);
    if (panel->maxHp >= 0) {
        sprintf(buffer, D_0069DA00, panel->maxHp);
        xglFontPrintDirectOT(FONT_COLOR_DEFAULT, buffer);
    } else {
        xglFontPrintDirectOT(FONT_COLOR_DEFAULT, D_0069D9F0);
    }

    if (panel->attackType != 0) {
        textDrawSegment(x + 88, y + 40, menuMainTxdEntry(panel->attackType + 0x151), 0);
    }
    textDrawSegment(x + 88, y + 64, D_0069D948, 0);
    for (i = 0; i < 8; i++) {
        if ((panel->attributes >> i) & 1) {
            if (needSeparator == 1) {
                xglFontPrintDirectOT(FONT_COLOR_DEFAULT, D_0069D9F8);
            }
            needSeparator = 1;
            xglFontPrintDirectOT(FONT_COLOR_DEFAULT, menuMainTxdEntry(i + 0xF0));
        }
    }

    sprintf(buffer, D_0069DA00, panel->exp);
    textDrawSegment(x + 88, y + 92, buffer, 0);
    sprintf(buffer, D_0069DA00, panel->skillPoints);
    textDrawSegment(x + 88, y + 116, buffer, 0);
    sprintf(buffer, D_0069DA00, panel->classPoints);
    textDrawSegment(x + 88, y + 140, buffer, 0);

    if (panel->rareItem > 0) {
        if (panel->rareItemKind == 2) {
            textDrawSegment(x + 88, y + 164, func_00A9B7D8(panel->rareItem, 0), 0);
        } else {
            textDrawSegment(x + 88, y + 164, func_00A9B8D8(panel->rareItem, 0), 0);
        }
    } else if (panel->rareItem < 0) {
        textDrawSegment(x + 88, y + 164, D_0069D9F0, 0);
    } else {
        textDrawSegment(x + 88, y + 164, D_0069DA08, 0);
    }
    if (panel->item > 0) {
        if (panel->itemKind == 2) {
            textDrawSegment(x + 88, y + 188, func_00A9B7D8(panel->item, 0), 0);
        } else {
            textDrawSegment(x + 88, y + 188, func_00A9B8D8(panel->item, 0), 0);
        }
    } else if (panel->item < 0) {
        textDrawSegment(x + 88, y + 188, D_0069D9F0, 0);
    } else {
        textDrawSegment(x + 88, y + 188, D_0069DA08, 0);
    }
    xglFontFlush();
    grResetScissor();
}


/* Clears the tick (+8) and the state byte (+9) of the text block. */
void textBlockClear(void) {
    textBlock.state = 0;
    textBlock.tick = 0;
}

INCLUDE_ASM("asm/main/nonmatchings/tu125", textBlockStart);

INCLUDE_ASM("asm/main/nonmatchings/tu125", textBlockIsActive);

INCLUDE_ASM("asm/main/nonmatchings/tu125", textBlockUpdate);

INCLUDE_ASM("asm/main/nonmatchings/tu125", textUpdate);
