#ifndef INCLUDE_OV01_TU058_H
#define INCLUDE_OV01_TU058_H

/* Types of src/ov01/tu058.c that src/ov01/tu059.c shares: the unit status panel record and its state values. */

#include "ov01/tu012.h"

/* Values of UnitPanel.state (menuUnitPanelBind stores idle, menuUnitPanelOpen sliding in), and the timer count at which the panel closes. */
enum UnitPanelState {
    UNIT_PANEL_IDLE = 0,
    UNIT_PANEL_SLIDING_IN = 1
};

#define UNIT_PANEL_CLOSE_TIME 16

/* The unit status panel record. Only the evidenced fields are modeled: the motion block at +0x10 (x and y at +0x10 and +0x14,
 * passed to the slide functions; tu059 names the same bytes origin, and the slide end target at +0x70), the state word at +0x90,
 * the owner at +0x94, the unit kind at +0x98 that the timer expiry clears, the halfword timerArmed at +0x9C and the halfword
 * timer at +0x9E. Bytes between the fields are untouched.
 *
 * The owner word holds the unit pointer (menuUnitPanelOpen and menuUnitPanelBind store it there); menuUnitPanelDraw reads the
 * unit body at its +0x144. The owner store of tu059 is alias set 0 (the ownerWord view). */
typedef struct UnitPanel UnitPanel;

struct UnitPanel {
    unsigned char unmodeled_00[4];
    void (*update)(UnitPanel *panel);   /* +0x04: the step menuUnitPanelStep */
    void (*draw)(UnitPanel *panel);     /* +0x08: the draw menuUnitPanelDraw */
    unsigned char unmodeled_0C[4];
    union {
        float motion[(0x90 - 0x10) / 4]; /* +0x10: the block slideSetStep and objStdMove move; motion[0] and [1] are x and y */
        struct {
            float origin[2];            /* +0x10: x and y of the panel (tu059) */
            unsigned char unmodeled_18[0x70 - 0x18];
            float target[2];            /* +0x70: where the slide ends, (16, 80) when the panel opens */
            unsigned char unmodeled_78[0x90 - 0x78];
        };
    };
    enum UnitPanelState state;          /* +0x90 */
    union {
        Ov01Object *owner;              /* +0x94: the unit the panel shows */
        unsigned int ownerWord;         /* the same word; this store is alias set 0 */
    };
    int kind;                           /* +0x98: the unit body state kind; 1 for the stat panel */
    union {
        struct {
            s16 timerArmed;             /* +0x9C: 1 while the auto-close timer runs */
            s16 timer;                  /* +0x9E: counts up while armed, closes at UNIT_PANEL_CLOSE_TIME */
        };
        int timers;                     /* the two halfwords as one word: New and Open clear both with one store */
    };
};

/* Functions of src/ov01/tu058.c that other files call. */
void menuUnitPanelNew(void);

#endif /* INCLUDE_OV01_TU058_H */
