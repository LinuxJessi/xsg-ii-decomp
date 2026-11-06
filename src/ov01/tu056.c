/*
 * OV01 original TU 56: 0x00aa5390..0x00aa5b60 (14 functions)
 */
#include "common.h"
#include "ov01/tu037.h"

/* dataTid2WepSp (already named): searches the 0xC-byte wepSpTbl entries for the technique id in $4. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu056", dataTid2WepSp);

/*
 * menuWepSpPanelNew (proposed menuWepSpPanelNew): allocates a menu panel with
 * objSlotAlloc, sets its step menuWepSpPanelStep and its draw menuWepSpPanelDraw,
 * origin (8, 312), state 0 and flag 0, and keeps it in menuWepSpPanel. Called from
 * the battle setup ov01StepNextEnemy.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu056", menuWepSpPanelNew);

/*
 * menuWepSpPanelStep (proposed menuWepSpPanelStep): the panel's step. It consumes a
 * show (1) or hide (2) request left in state by menuWepSpPanelShow/Hide and
 * sets state back to 0.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu056", menuWepSpPanelStep);

typedef struct GrPacket GrPacket;

extern void grGsRegSet(GrPacket *packet, unsigned long long register_id,
                       unsigned long long value);

/*
 * menuPulseAlphaSet (proposed menuPulseAlphaSet): sets GS ALPHA_1 (register 0x42)
 * with a fixed alpha that rises and falls with the scene frame counter
 * ov01SceneFrameCount (proposed ov01SceneFrameCount): a triangle wave from 0x2A to 0xA9.
 */
void menuPulseAlphaSet(GrPacket *packet)
{
    int phase;

    phase = (ov01SceneFrameCount() * 2) & 0xFF;
    if (phase >= 0x80) {
        phase = 0xFF - phase;
    }
    /* GS register 0x42 (ALPHA_1); the fixed alpha sits in the high word. */
    grGsRegSet(packet, 0x42, ((unsigned long long)(phase + 0x2A) << 32) | 0x68);
}

/*
 * menuWepSpPanelDraw (proposed menuWepSpPanelDraw): draws the panel while its flag
 * is set: a header, then one row per entry of the battle list from
 * func_00A89538, each row's wep-special record from dataTid2WepSp, and a footer
 * at count * 21 below the rows. Calls func_00AA60B8 for the units with keys 11 to 15.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu056", menuWepSpPanelDraw);

/*
 * The menu panel record (the same layout as MenuState of ov01/tu062) behind
 * menuWepSpPanel, menuTargetListPanel and menuTgtStatusPanel. Only the words the functions of this
 * TU use are named: state at +0x90 (0 idle, 1 show request, 2 hide request) and
 * flag at +0x94 (nonzero while the panel is shown and drawn).
 */
typedef struct MenuState {
    unsigned char unmodeled_00[0x90];
    int state;  /* +0x90 */
    int flag;   /* +0x94 */
} MenuState;

extern MenuState *menuWepSpPanel;

/* Proposed menuWepSpPanelShow: requests the wep-special panel to show. */
void menuWepSpPanelShow(void)
{
    menuWepSpPanel->state = 1;
    menuWepSpPanel->flag = 1;
}

/* Proposed menuWepSpPanelHide: requests the wep-special panel to hide. */
void menuWepSpPanelHide(void)
{
    menuWepSpPanel->state = 2;
    menuWepSpPanel->flag = 0;
}

/*
 * menuTargetListNew (proposed menuTargetListNew): the same constructor as
 * menuWepSpPanelNew, with step menuTargetListStep and draw menuTargetListDraw, kept in
 * menuTargetListPanel.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu056", menuTargetListNew);

/* menuTargetListStep (proposed menuTargetListStep): consumes the show or hide request, as menuWepSpPanelStep. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu056", menuTargetListStep);

/*
 * menuTargetListDraw (proposed menuTargetListDraw): while the flag is set, draws one
 * row per entry of the list from func_00A90A88. Each entry is looked up as a
 * battle entry (battleEntryFindByKey battleEntryFindByKey, battleSlotGet battleSlotGet)
 * and its values are printed with menuNumDrawTarget.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu056", menuTargetListDraw);

extern MenuState *menuTargetListPanel;

/* Proposed menuTargetListShow. */
void menuTargetListShow(void)
{
    menuTargetListPanel->state = 1;
    menuTargetListPanel->flag = 1;
}

/* Proposed menuTargetListHide. */
void menuTargetListHide(void)
{
    menuTargetListPanel->state = 2;
    menuTargetListPanel->flag = 0;
}

/*
 * menuTgtStatusNew (proposed menuTgtStatusNew): constructor of the target status
 * panel kept in menuTgtStatusPanel: empty step func_00AA5B58, draw menuTgtStatusDraw (ov01/tu057),
 * origin (348, 314), state 0 and flag 0.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu056", menuTgtStatusNew);

/* Empty step of the target status panel; the callback has no body, so it is left unnamed. */
void func_00AA5B58(void)
{
}
