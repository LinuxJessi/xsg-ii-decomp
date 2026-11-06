/*
 * OV01 original TU 58: 0x00aa6148..0x00aa6390 (3 functions)
 */
#include "common.h"

/*
 * menuUnitPanelNew (proposed menuUnitPanelNew): constructor of the unit status
 * panel. It allocates the panel with objSlotAlloc, sets its step menuUnitPanelStep
 * and its draw menuUnitPanelDraw, keeps it in menuUnitPanel and zeroes the owner, the
 * state, the kind and the timer fields. Called from the battle setup ov01StepNextEnemy.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu058", menuUnitPanelNew);

#include "shared.h"

/*
 * The unit status panel record. Only the evidenced fields are modeled: the
 * motion block at +0x10 (position at +0x10 and +0x14, passed to the slide
 * functions), the state word at +0x90, the owner word at +0x94, the unit kind
 * at +0x98 that the timer expiry clears, the halfword timerArmed at +0x9C and
 * the halfword timer at +0x9E. Bytes between the fields are untouched.
 *
 * The owner word is an int in this C, but the asm stores the unit pointer there
 * (menuUnitPanelOpen, menuUnitPanelBind) and the draw reads the unit body at its +0x144.
 */
typedef struct UnitPanel UnitPanel;

struct UnitPanel {
    unsigned char unmodeled_00[0x10];
    unsigned char motion[0x90 - 0x10]; /* +0x10: the block slideSetStep and objStdMove move */
    int state;                         /* +0x90 */
    int owner;                         /* +0x94 */
    int kind;                          /* +0x98 */
    s16 timerArmed;                    /* +0x9C */
    s16 timer;                         /* +0x9E */
};

int objStdMove(void *block);
void slideSetStep(void *block, int value);

/*
 * menuUnitPanelStep (proposed menuUnitPanelStep): with an owner, state 1 slides the
 * panel with slideSetStep until objStdMove reports arrival, then state goes
 * to 0. In state 0 an armed timer counts up, and at 16 the panel releases its
 * owner, kind and state.
 */
void menuUnitPanelStep(UnitPanel *panel)
{
    if (panel->owner != 0)
    {
        switch (panel->state)
        {
        case 1:
            slideSetStep(panel->motion, 2);
            if (objStdMove(panel->motion) != 0)
            {
                panel->state = 0;
            }
            break;
        case 0:
        default:
            if (panel->timerArmed == 1)
            {
                panel->timer = (s16) (panel->timer + 1);
                if (panel->timer >= 0x10)
                {
                    panel->owner = 0;
                    panel->kind = 0;
                    panel->state = 0;
                }
            }
            break;
        }
    }
}

/*
 * menuUnitPanelDraw (proposed menuUnitPanelDraw): while the panel has an owner it
 * reads the owner's unit body (+0x144), takes the kind byte at +0x70 (0 to 3)
 * and draws the panel's two sprites at the coordinates of that kind.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu058", menuUnitPanelDraw);
