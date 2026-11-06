/*
 * OV01 original TU 57: 0x00aa5b60..0x00aa6148 (5 functions)
 */
#include "common.h"

/*
 * menuTgtStatusDraw (proposed menuTgtStatusDraw): the draw of the target status panel
 * (menuTgtStatusPanel, ov01/tu056 menuTgtStatusNew). While its flag is set it takes the
 * target from tgtUnitGet and draws the unit's values from its body (+0x144):
 * the value at +0x5C in five digits (sprites above 99999), the halfwords at
 * +0x92..+0x98, the flag 0x800 at +0x1C, and the status icons of
 * menuUnitStatusIconsDraw (proposed menuUnitStatusIconsDraw).
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu057", menuTgtStatusDraw);

/*
 * menuUnitStatusIconsDraw (proposed menuUnitStatusIconsDraw): draws one icon per byte of
 * the unit body's list at +0x20, up to four bytes, stopping at a zero byte. The
 * byte picks the sprite (1, 2 or 4 give the sprite columns 0xAE, 0xBE or 0xCE),
 * and each icon is 16 units to the right of the last.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu057", menuUnitStatusIconsDraw);

#include "tu057.h"

extern void *menuTgtStatusPanel;

/* Proposed menuTgtStatusShow: sets the visible flag (+0x94) of the target status panel. */
void menuTgtStatusShow(void) {
    ((struct Ov01Tu057State *) menuTgtStatusPanel)->enabled = 1;
}

/* Proposed menuTgtStatusHide: clears the visible flag (+0x94) of the target status panel. */
void menuTgtStatusHide(void) {
    ((struct Ov01Tu057State *) menuTgtStatusPanel)->enabled = 0;
}

void grPutSprOpen(int *);
void grPutSprClose(void);
void menuNumDraw(int *, int, int, int, int, int);
void menuStatSprite(int *, int, int, int, int, int, int);
void menuTexFlush(void);
void func_00AA84D0(void *);
void grGpInit(void *);
void grPacketSend(void *);

/*
 * func_00AA60B8 (proposed menuUnitValueDraw, low: the meaning of the value is
 * not evidenced): draws a fixed panel with a three-digit number taken from the
 * link object at +0x144 of the unit (its halfword at +0x80). Called by
 * menuWepSpPanelDraw (ov01/tu056 menuWepSpPanelDraw) for units with keys 11 to 15.
 */
void func_00AA60B8(struct Ov01Tu057Context *unit) {
    int packet[4];

    menuTexFlush();
    grGpInit(packet);
    func_00AA84D0(packet);
    grPutSprOpen(packet);
    menuStatSprite(packet, 0x178, 0x154, 0x12, 0xC, 0xC2, 0x6A);
    grPutSprClose();
    menuNumDraw(packet, 0x1AE, 0x154, unit->link->value, 3, 2);
    grPacketSend(packet);
}
