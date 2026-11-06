/*
 * OV01 original TU 51: 0x00aa3e28..0x00aa4280 (5 functions)
 */
#include "common.h"

INCLUDE_ASM("asm/nonmatchings/ov01/tu051", menuCursorCreate);

int objStdMove(void *);
void slideSetStep(void *, int);

/* Fields this TU reads or writes; the object is larger than this view. */
typedef struct MenuCursor MenuCursor;

struct MenuCursor {
    unsigned char unmodeled_00[0x10];
    float x; /* +0x10: menuCursorDraw stores 288.0f here */
    float y; /* +0x14: menuCursorDraw stores 390.0f here */
    unsigned char unmodeled_18[0x90 - 0x18];
    int state; /* +0x90 */
    int substate; /* +0x94: menuCursorDraw tests it against 1 */
};

/* Cursor update (callback +4 of menuCursor): state 1 steps the cursor position and clears
 * state on arrival. */
void menuCursorUpdate(MenuCursor *cursor) {
    void *pos;

    if (cursor->state != 0) {
        if (cursor->state == 1) {
            pos = &cursor->x;
            slideSetStep(pos, 2);
            if (objStdMove(pos) != 0) {
                cursor->state = 0;
            }
        }
    }
}

int func_00A91750(void);
void menuUnitListDraw(MenuCursor *);
void menuCursorTargetDraw(MenuCursor *cursor);

/* Cursor draw (callback +8): parks the cursor at the home position and draws the target
 * status when func_00A91750 is set, otherwise draws the unit list when substate is 1. */
void menuCursorDraw(MenuCursor *cursor) {
    if (func_00A91750() != 0) {
        cursor->x = 288.0f;
        cursor->y = 390.0f;
        menuCursorTargetDraw(cursor);
        return;
    }
    if (cursor->substate != 1) {
        return;
    }
    menuUnitListDraw(cursor);
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu051", menuUnitListDraw);

/* The two 4-byte colours at D_00ABACC8, copied as one 8-byte block. */
typedef struct ColourPair {
    unsigned char bytes[8];
} ColourPair;

extern ColourPair D_00ABACC8;

/* Only the members this TU touches are modelled. */
typedef struct UnitModel UnitModel;

struct UnitModel {
    unsigned char unmodeled_00[0x62];
    short value; /* +0x62: the fourth argument of func_00AA7750 */
};

typedef struct UnitActor UnitActor;

struct UnitActor {
    unsigned char unmodeled_00[0x144];
    UnitModel *model; /* +0x144 */
};

typedef struct UnitSlots UnitSlots;

struct UnitSlots {
    unsigned char unmodeled_00[0x10];
    UnitActor *work;  /* +0x10 */
    UnitActor *actor; /* +0x14 */
};

extern UnitSlots *actStockGet(void);
extern void *tgtUnitGet(void);
/* The GIF/VIF packet is passed through as storage: grGpInit writes it and
 * the rest of this TU never reads its members. */
extern void grGpInit(void *packet);
extern void grPacketSend(void *packet);
extern void func_00AA84D0(void *packet);
extern void grPutSprOpen(void *packet);
extern void grPutSprClose(void);
extern void func_00AA7750(void *packet, int x, int y, short value, int);
extern void menuStatSpriteRGB(void *packet, int x, int y, int width, int height,
                              int srcX, int srcY, const void *colour);

/* Status sprite for the work unit (else the actor unit) at the cursor position. */
void menuCursorTargetDraw(MenuCursor *cursor) {
    unsigned int packet[4];
    ColourPair palette[2];
    UnitActor *actor;
    int index;
    int x;
    int y;

    palette[0] = D_00ABACC8;
    if (actStockGet()->work != 0) {
        index = 1;
        actor = actStockGet()->work;
    } else if (actStockGet()->actor != 0) {
        index = 0;
        actor = actStockGet()->actor;
    } else {
        return;
    }
    x = (int) cursor->x;
    y = (int) cursor->y;
    grGpInit(packet);
    func_00AA84D0(packet);
    grPutSprOpen(packet);
    menuStatSpriteRGB(packet, x - 42, y + 26, 110, 24, 146, 14, &palette[0].bytes[index * 4]);
    grPutSprClose();
    func_00AA7750(packet, x,
                  ((void *) actor != tgtUnitGet()) ? y : y - 4,
                  actor->model->value, 1);
    grPacketSend(packet);
}
