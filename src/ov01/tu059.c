/*
 * OV01 original TU 59: 0x00aa6390..0x00aa64a8 (3 functions)
 */
#include "common.h"
#include "ov01/tu059.h"
#include "ov01/tu012.h"
#include "ov01/tu058.h"

/* Kinds and statPanelState values of the unit status panel; its states are in ov01/tu058.h. */
#define UNIT_PANEL_KIND_STAT 1
#define STAT_PANEL_PENDING 1
#define STAT_PANEL_SHOWN 2

/* The step counter record of ov01/tu023 (ActStepCounter); only the word at +0xC is named here. */
struct ActStepCounter {
    unsigned char unmodeled_00[0x0C];
    int statPanelState;               /* +0x0C: for kind 1 the stat panel is created while this equals 1, then it holds 2 */
};

struct ActStepCounter *actStepCounterGet(void);
void menuStatPanelNew(int kind);

extern UnitPanel *menuUnitPanel;

/*
 * menuUnitPanelOpen: opens the unit status panel menuUnitPanel (ov01/tu058) for the
 * unit, arguments unit and kind. For kind 1 the first call creates a stat panel
 * with menuStatPanelNew(0) and marks the word at +0xC as 2. When the panel's kind
 * at +0x98 differs, the panel takes the unit as owner, state 1 (slide in), origin
 * (-64, 80) and target (16, 80), and disarms its timer.
 */
void menuUnitPanelOpen(Ov01Object *unit, int kind)
{
    UnitPanel *panel;

    panel = menuUnitPanel;
    if (kind == UNIT_PANEL_KIND_STAT) {
        if (actStepCounterGet()->statPanelState == STAT_PANEL_PENDING) {
            menuStatPanelNew(0);
            actStepCounterGet()->statPanelState = STAT_PANEL_SHOWN;
        }
    }
    if (panel->kind != kind) {
        panel->owner = unit;
        panel->state = UNIT_PANEL_SLIDING_IN;
        panel->kind = kind;
        panel->origin[0] = -64.0f;
        panel->target[0] = 16.0f;
        panel->timers = 0;
        panel->origin[1] = 80.0f;
        panel->target[1] = 80.0f;
    }
}

/* menuUnitPanelArmTimer: starts the auto-close count of the unit status panel. */
void menuUnitPanelArmTimer(void) {
    menuUnitPanel->timerArmed = 1;
}

/*
 * menuUnitPanelBind: binds the panel to the unit, argument unit. It sets the owner
 * to the unit, the state to 0 and the kind to the unit body's kind byte at +0x70,
 * sets the timer at +0x9E to 16 with timerArmed 0 (the count is not running), and
 * places the origin at (16, 80).
 */
void menuUnitPanelBind(Ov01Object *unit)
{
    UnitPanel *panel;
    float originX = 16.0f;
    float originY = 80.0f;

    panel = menuUnitPanel;
    panel->owner = unit;
    panel->state = UNIT_PANEL_IDLE;
    panel->kind = unit->stats->state70;
    panel->origin[1] = originY;
    panel->timer = UNIT_PANEL_CLOSE_TIME;
    panel->origin[0] = originX;
    panel->timerArmed = 0;
}
