/*
 * OV01 original TU 66: 0x00aa8930..0x00aa8b70 (4 functions)
 */
#include "common.h"
#include "ov01/tu066.h"

/* The flag bit decoDisp and decoDisp2 set (argument 0) or clear (any other value) in the first word of a record. */
#define DECO_FLAG_BIT 0x4
/* Entries of menuStatRecords and of objPathTable. */
#define DECO_STAT_RECORD_COUNT 3
#define DECO_PATH_COUNT 5

extern unsigned int *menuStatRecords[];
extern unsigned int *objPathTable[];
extern unsigned int *menuCursor;
extern unsigned int *menuIconListObj;
extern unsigned int *menuValuePanelObj;
extern unsigned int *actStepIconObj;
extern unsigned int *menuTgtStatusPanel;

/*
 * Sets flag bit 0x4 in the first word of every entry: the three of menuStatRecords and the five of
 * objPathTable (null array entries are skipped) and the five single pointers (not checked for null).
 * Declared int with no value returned: battleEndCheck (tu035) only matches against a value-returning
 * callee, and this body has no result to return.
 */
int decoFlagsSetAll(void) {
    unsigned int *flagWord;
    int index;

    for (index = 0; index < DECO_STAT_RECORD_COUNT; index++) {
        flagWord = menuStatRecords[index];
        if (flagWord != 0) {
            *flagWord |= DECO_FLAG_BIT;
        }
    }
    for (index = 0; index < DECO_PATH_COUNT; index++) {
        flagWord = objPathTable[index];
        if (flagWord != 0) {
            *flagWord |= DECO_FLAG_BIT;
        }
    }
    *menuCursor |= DECO_FLAG_BIT;
    *menuIconListObj |= DECO_FLAG_BIT;
    *menuValuePanelObj |= DECO_FLAG_BIT;
    *actStepIconObj |= DECO_FLAG_BIT;
    *menuTgtStatusPanel |= DECO_FLAG_BIT;
}

/* Clears flag bit 0x4 in every group that decoFlagsSetAll sets. */
void decoFlagsClearAll(void) {
    unsigned int *flagWord;
    int index;

    for (index = 0; index < DECO_STAT_RECORD_COUNT; index++) {
        flagWord = menuStatRecords[index];
        if (flagWord != 0) {
            *flagWord &= ~DECO_FLAG_BIT;
        }
    }
    for (index = 0; index < DECO_PATH_COUNT; index++) {
        flagWord = objPathTable[index];
        if (flagWord != 0) {
            *flagWord &= ~DECO_FLAG_BIT;
        }
    }
    *menuCursor &= ~DECO_FLAG_BIT;
    *menuIconListObj &= ~DECO_FLAG_BIT;
    *menuValuePanelObj &= ~DECO_FLAG_BIT;
    *actStepIconObj &= ~DECO_FLAG_BIT;
    *menuTgtStatusPanel &= ~DECO_FLAG_BIT;
}

/* Sets flag bit 0x4 in the first group only (menuStatRecords). */
void decoFlagsSetFirst(void) {
    unsigned int *flagWord;
    int index;

    for (index = 0; index < DECO_STAT_RECORD_COUNT; index++) {
        flagWord = menuStatRecords[index];
        if (flagWord != 0) {
            *flagWord |= DECO_FLAG_BIT;
        }
    }
}

/* Clears flag bit 0x4 in the first group only (menuStatRecords). */
void decoFlagsClearFirst(void) {
    unsigned int *flagWord;
    int index;

    for (index = 0; index < DECO_STAT_RECORD_COUNT; index++) {
        flagWord = menuStatRecords[index];
        if (flagWord != 0) {
            *flagWord &= ~DECO_FLAG_BIT;
        }
    }
}
