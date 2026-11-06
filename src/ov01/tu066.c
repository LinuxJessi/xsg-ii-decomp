/*
 * OV01 original TU 66: 0x00aa8930..0x00aa8b70 (4 functions)
 */
#include "common.h"
#include "ov01/tu066.h"

/*
 * Sets flag bit 0x4 in the first word of every entry: the three of menuStatRecords,
 * the five of objPathTable and the five single pointers, skipping null entries.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu066", decoFlagsSetAll);

/* Groups of object flag words, each entry pointing at a word whose bit 0x4 is the flag. */
extern unsigned int *menuStatRecords[];
extern unsigned int *objPathTable[];
extern unsigned int *menuCursor;
extern unsigned int *menuIconListObj;
extern unsigned int *menuValuePanelObj;
extern unsigned int *D_00AE0EC0;
extern unsigned int *menuTgtStatusPanel;

/* Clears flag bit 0x4 in every group that decoFlagsSetAll sets. */
void decoFlagsClearAll(void) {
    unsigned int *entry;
    int i;

    for (i = 0; i < 3; i++) {
        entry = menuStatRecords[i];
        if (entry != 0) {
            *entry &= ~4;
        }
    }
    for (i = 0; i < 5; i++) {
        entry = objPathTable[i];
        if (entry != 0) {
            *entry &= ~4;
        }
    }
    *menuCursor &= ~4;
    *menuIconListObj &= ~4;
    *menuValuePanelObj &= ~4;
    *D_00AE0EC0 &= ~4;
    *menuTgtStatusPanel &= ~4;
}

/* Sets flag bit 0x4 in the first group only (menuStatRecords). */
void decoFlagsSetFirst(void) {
    unsigned int *entry;
    int i;

    for (i = 0; i < 3; i++) {
        entry = menuStatRecords[i];
        if (entry != 0) {
            *entry |= 4;
        }
    }
}

/* Clears flag bit 0x4 in the first group only (menuStatRecords). */
void decoFlagsClearFirst(void) {
    unsigned int *entry;
    int i;

    for (i = 0; i < 3; i++) {
        entry = menuStatRecords[i];
        if (entry != 0) {
            *entry &= ~4;
        }
    }
}
