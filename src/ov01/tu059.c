/*
 * OV01 original TU 59: 0x00aa6390..0x00aa64a8 (3 functions)
 */
#include "common.h"
#include "ov01/tu059.h"

/*
 * menuUnitPanelOpen (proposed menuUnitPanelOpen, arguments unit and kind): opens the
 * unit status panel menuUnitPanel (ov01/tu058) for the unit. For kind 1 the first
 * call creates a stat panel with menuStatPanelNew(0) and marks the word at +0xC
 * as 2. When the panel's kind at +0x98 differs, the panel takes the unit as owner,
 * state 1 (slide in) and origin (-64, 80), and disarms its timer.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu059", menuUnitPanelOpen);

/* The unit status panel record of ov01/tu058 (UnitPanel); only the timer halfword is named here. */
typedef struct UnitPanel {
    unsigned char unmodeled_00[0x9C];
    short timerArmed;   /* +0x9C: 1 while the auto-close timer runs (step of ov01/tu058) */
} UnitPanel;

extern UnitPanel *menuUnitPanel;

/* Proposed menuUnitPanelArmTimer: starts the auto-close count of the unit status panel. */
void menuUnitPanelArmTimer(void) {
    menuUnitPanel->timerArmed = 1;
}

/*
 * menuUnitPanelBind (proposed menuUnitPanelBind, argument unit): sets the panel's
 * owner to the unit, its state to 0 and its kind to the unit body's kind byte
 * at +0x70, sets the timer at +0x9E to 16 with timerArmed 0 (the count is not
 * running), and places the origin at (16, 80).
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu059", menuUnitPanelBind);
