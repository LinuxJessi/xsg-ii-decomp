/*
 * OV01 original TU 54: 0x00aa4bd8..0x00aa5248 (8 functions)
 */
#include "common.h"
#include "ov01/tu048.h"
#include "ov01/tu054.h"
#include "ov01/tu063.h"

INCLUDE_ASM("asm/nonmatchings/ov01/tu054", arrowCreate);

/* The cycle counter at +0x94 of the object: arrowCycleUpdate advances it by one
 * and resets it to zero once it reaches 0x11. */
struct ArrowObject {
    unsigned char unmodeled_00[0x94];
    int cycle;
};

/* Arrow update (callback +4): advances the cycle and wraps it at 0x11. */
void arrowCycleUpdate(struct ArrowObject *object) {
    int next;

    next = object->cycle + 1;
    if (next >= 0x11) {
        next = 0;
    }
    object->cycle = next;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu054", arrowDraw);

void objSlotRelease(int);
extern int arrowObj;

void arrowRemove(void) {
    if (arrowObj != 0) {
        objSlotRelease(arrowObj);
        arrowObj = 0;
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu054", func_00AA4DB8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu054", func_00AA4F20);

int grBBCalc(int, int *, int *);
void func_00AA5050(GrPacket *packet, int first, int second, int index);
void menuTexFlush(void);

/* Draw callback of the object made by func_00AA4DB8 (its +0x08 slot): grBBCalc
 * projects the object's origin (+0x10) to the screen, and when it is on screen
 * func_00AA5050 draws the entry there. The object is passed as an int here. */
void func_00AA4FD0(int index) {
    GrPacket packet;
    int screen_x;
    int screen_y;

    if (grBBCalc(index + 0x10, &screen_x, &screen_y) != 0) {
        menuTexFlush();
        grGpInit(&packet);
        func_00AA8450(&packet);
        grPutSprOpen(&packet);
        func_00AA5050(&packet, screen_x, screen_y, index);
        grPutSprClose();
        grPacketSend(&packet);
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu054", func_00AA5050);
