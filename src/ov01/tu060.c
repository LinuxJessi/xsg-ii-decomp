/*
 * OV01 original TU 60: 0x00aa64a8..0x00aa6bf8 (7 functions)
 */
#include "common.h"
#include "ov01/tu060.h"

/*
 * menuIconListNew (proposed menuIconListNew): constructor of the unit icon list
 * panel. It allocates the panel with objSlotAlloc, sets its step menuIconListStep
 * and its draw menuIconListDraw, keeps it in menuIconListPanel and zeroes its state, flag,
 * counters and close request. Called from the battle setup ov01StepNextEnemy.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu060", menuIconListNew);

/*
 * menuIconListStep (proposed menuIconListStep): with an owner, state 1 slides the
 * list in with slideSetStep and objStdMove. In state 0 a close request
 * (closeRequest 1) counts its timer at +0xAE up to 16, then clears the owner and
 * frees the row with menuPanelRowFree. Each frame the first up-to-8 slot counters
 * (bytes at +0x9C..+0xAB) grow and saturate at 4.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu060", menuIconListStep);

/*
 * menuIconListDraw (proposed menuIconListDraw): while the list has an owner it
 * draws its header and then one icon per entry (count at +0x98, up to 8) with
 * menuIconEntryDraw (proposed menuIconEntryDraw), using the entry's kind and mark
 * bytes. The count-based footer is drawn after the rows.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu060", menuIconListDraw);

/*
 * menuIconListAppend (proposed menuIconListAppend, called by func_00A94260): appends
 * one entry to the list for the unit (up to 8 entries; the unit body's entry
 * table at +0xDA and +0xE2 gives the kind and mark). When the unit has no
 * entries, or its kind is 2 or 3, it requests the close instead. An unowned
 * list is opened for the unit at origin (-152, row).
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu060", menuIconListAppend);

/*
 * The icon list panel record of menuIconListPanel (IconListPanel). Only the close request
 * is named here: closeRequest at +0xAC is set by menuIconListClose and read by the step.
 */
typedef struct IconListPanel {
    unsigned char unmodeled_00[0xAC];
    short closeRequest;     /* +0xAC: 1 asks the step to count down and close the list */
} IconListPanel;

extern IconListPanel *menuIconListPanel;

/* Proposed menuIconListClose: requests the icon list to close (the step counts its timer). */
void menuIconListClose(void) {
    menuIconListPanel->closeRequest = 1;
}

/*
 * menuIconListShow (proposed menuIconListShow, argument unit): for a unit of kind 0
 * with entries, frees the list's row if it is open, copies up to 8 entries from
 * the unit body (count at +0xEC), and opens the list at origin (16, row) with its
 * timer at 16. Otherwise it requests the close.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu060", menuIconListShow);

/*
 * menuIconEntryDraw (proposed menuIconEntryDraw): draws one icon of an entry. The
 * entry kind (1, 2 or 3) selects the sprite column 0x1BA, 0x1D2 or 0x1EA, the
 * flagged entries move by 0x15, and the position follows the index.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu060", menuIconEntryDraw);
