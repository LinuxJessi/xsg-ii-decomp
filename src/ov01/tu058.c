/*
 * OV01 original TU 58: 0x00aa6148..0x00aa6390 (3 functions)
 */
#include "common.h"
#include "shared.h"
#include "ov01/tu005.h"
#include "ov01/tu012.h"
#include "ov01/tu058.h"
#include "ov01/tu063.h"

int objStdMove(void *block);
void slideSetStep(void *block, int value);
void *objSlotAlloc(void);
UnitPanel *menuUnitPanel;
void menuUnitPanelStep(UnitPanel *panel);
void menuUnitPanelDraw(UnitPanel *panel);

/*
 * menuUnitPanelNew: constructor of the unit status panel. It allocates the panel
 * with objSlotAlloc, sets its step menuUnitPanelStep and its draw menuUnitPanelDraw,
 * keeps it in menuUnitPanel and zeroes the owner, the state, the kind and the timer
 * fields. Called from the battle setup ov01StepNextEnemy.
 */
void menuUnitPanelNew(void)
{
    UnitPanel *panel;

    panel = objSlotAlloc();
    if (panel != 0) {
        panel->update = menuUnitPanelStep;
        panel->draw = menuUnitPanelDraw;
        menuUnitPanel = panel;
        ov01MatrixBlockInit(panel->motion);
        panel->state = UNIT_PANEL_IDLE;
        panel->owner = 0;
        panel->kind = 0;
        panel->timers = 0;
    }
}

/*
 * menuUnitPanelStep: with an owner, state 1 slides the panel with slideSetStep
 * until objStdMove reports arrival, then state goes to 0. In state 0 an armed
 * timer counts up, and at 16 the panel releases its owner, kind and state.
 */
void menuUnitPanelStep(UnitPanel *panel)
{
    if (panel->owner != 0)
    {
        switch (panel->state)
        {
        case UNIT_PANEL_SLIDING_IN:
            slideSetStep(panel->motion, 2);
            if (objStdMove(panel->motion) != 0)
            {
                panel->state = UNIT_PANEL_IDLE;
            }
            break;
        case UNIT_PANEL_IDLE:
        default:
            if (panel->timerArmed == 1)
            {
                panel->timer = panel->timer + 1;
                if (panel->timer >= UNIT_PANEL_CLOSE_TIME)
                {
                    panel->owner = 0;
                    panel->kind = 0;
                    panel->state = UNIT_PANEL_IDLE;
                }
            }
            break;
        }
    }
}

void menuTexFlush(void);
void grPutSprClose(void);
void menuStatSprite(void *packet, int x, int y, int width, int height, int srcX, int srcY);

/*
 * menuUnitPanelDraw: while the panel has an owner it reads the owner's unit body
 * (+0x144), takes the kind byte at +0x70 (1 to 3; other values draw nothing) and
 * draws the panel's two sprites at the panel's origin: a bar of the kind's width
 * with a source row that depends on the kind, and a strip from x = 0 to the bar's end.
 */
void menuUnitPanelDraw(UnitPanel *panel)
{
    /* 16-byte frame slot (two GrPacket words wide, as the int[4] it was); packet[0] is the packet. */
    GrPacket packet[2];
    Ov01Object *owner;
    int originX;
    int originY;
    int width;
    int srcY;
    signed char kind;

    if (panel->owner != 0) {
        owner = panel->owner;
        kind = owner->stats->state70;
        switch (kind) {
        case 1:
            srcY = 0;
            width = 62;
            break;
        case 2:
            srcY = 36;
            width = 56;
            break;
        case 3:
            srcY = 18;
            width = 34;
            break;
        default:
            return;
        }
        originX = panel->motion[0];
        originY = panel->motion[1];
        menuTexFlush();
        grGpInit(packet);
        menuPanelTexState(packet);
        grPutSprOpen(packet);
        menuStatSprite(packet, 0, originY + 10, originX + width + 10, 10, 431 - (originX + width), 74);
        menuStatSprite(packet, originX, originY, width, 18, 256, srcY);
        grPutSprClose();
        grPacketSend(packet);
    }
}
