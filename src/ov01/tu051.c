/*
 * OV01 original TU 51: 0x00aa3e28..0x00aa4280 (5 functions)
 */
#include "common.h"
#include "ov01/tu005.h"
#include "ov01/tu012.h"
#include "ov01/tu023.h"
#include "ov01/tu036.h"
#include "ov01/tu048.h"
#include "ov01/tu051.h"

int objStdMove(void *);
void slideSetStep(void *, int);
void *objSlotAlloc(void);
void menuCursorUpdate(MenuCursor *cursor);
void menuCursorDraw(MenuCursor *cursor);

/* Creates the menu cursor: update and draw callbacks, a cleared slide block
 * with the cursor parked below the home position, not moving and hidden. */
void menuCursorCreate(void)
{
    MenuCursor *cursor;

    cursor = objSlotAlloc();
    if (cursor != 0) {
        cursor->update = menuCursorUpdate;
        cursor->draw = menuCursorDraw;
        menuCursor = cursor;
        ov01MatrixBlockInit(&cursor->slide.block);
        cursor->slide.pos.x = MENU_CURSOR_HOME_X;
        cursor->slide.pos.y = MENU_CURSOR_PARKED_Y;
        cursor->slide.pos.z = 0;
        cursor->moving = 0;
        cursor->kind.cursor.visible = 0;
    }
}

/* Cursor update (callback +4 of menuCursor): state 1 steps the cursor position and clears
 * state on arrival. */
void menuCursorUpdate(MenuCursor *cursor) {
    void *pos;

    if (cursor->moving != 0) {
        if (cursor->moving == 1) {
            pos = &cursor->slide;
            slideSetStep(pos, 2);
            if (objStdMove(pos) != 0) {
                cursor->moving = 0;
            }
        }
    }
}

int actStockHasBoost(void);
void menuUnitListDraw(MenuCursor *);
void menuCursorTargetDraw(MenuCursor *cursor);

/* Cursor draw (callback +8): parks the cursor at the home position and draws the target
 * status when actStockHasBoost is set, otherwise draws the unit list when substate is 1. */
void menuCursorDraw(MenuCursor *cursor) {
    if (actStockHasBoost() != 0) {
        cursor->slide.pos.x = MENU_CURSOR_HOME_X;
        cursor->slide.pos.y = MENU_CURSOR_HOME_Y;
        menuCursorTargetDraw(cursor);
        return;
    }
    if (cursor->kind.cursor.visible != 1) {
        return;
    }
    menuUnitListDraw(cursor);
}

/* The two 4-byte colours at D_00ABACC8, copied as one 8-byte block. */
typedef struct ColourPair {
    unsigned char bytes[8];
} ColourPair;

extern ColourPair D_00ABACC8;

extern void func_00AA84D0(GrPacket *packet);
extern void menuKeyDraw(GrPacket *packet, int x, int y, short value, int);
extern void menuStatSpriteRGB(GrPacket *packet, int x, int y, int width, int height,
                              int srcX, int srcY, const void *colour);

/* Screen offset of each party slot in the unit list (x, y). */
typedef struct MenuUnitSlotOffset {
    int x;
    int y;
} MenuUnitSlotOffset;

extern MenuUnitSlotOffset menuUnitSlotOffsets[];
/* Frame colour of each party slot (R, G, B, A). */
extern unsigned char menuUnitSlotColours[][4];

int unitTblGet(int side, Ov01Object ***table);
extern void menuStatSprite(GrPacket *packet, int x, int y, int width, int height,
                           int srcX, int srcY);

/* Unit list at the cursor: the list frame, then per party unit its slot frame
 * and, when the unit is shown and calcBoostOk(unit, 1) allows it, its status. */
void menuUnitListDraw(MenuCursor *cursor)
{
    GrPacket packet;
    Ov01Object **table;
    Ov01Object *unit;
    int count;
    int i;
    int x;
    int y;

    grGpInit(&packet);
    x = (int) cursor->slide.pos.x;
    y = (int) cursor->slide.pos.y;
    func_00AA84D0(&packet);
    grPutSprOpen(&packet);
    menuStatSprite(&packet, x - 42, y - 14, 110, 14, 146, 0);
    grPutSprClose();
    count = unitTblGet(0, &table);
    for (i = 0; i < count; i++) {
        func_00AA84D0(&packet);
        grPutSprOpen(&packet);
        menuStatSpriteRGB(&packet, x + menuUnitSlotOffsets[i].x - 2, y + menuUnitSlotOffsets[i].y - 2, 28, 42, 227, 77,
                          menuUnitSlotColours[i]);
        grPutSprClose();
        unit = table[i];
        if (unit == 0 || (unit->flags & UNIT_FLAG_DOWN)) {
            continue;
        }
        if (calcBoostOk(unit, 1) != 0) {
            menuKeyDraw(&packet, x + menuUnitSlotOffsets[i].x, y + menuUnitSlotOffsets[i].y,
                          table[i]->stats->key, 1);
        }
    }
    grPacketSend(&packet);
}

/* Status sprite for the work unit (else the actor unit) at the cursor position. */
void menuCursorTargetDraw(MenuCursor *cursor) {
    GrPacket packet;
    ColourPair palette[2];
    Ov01Object *actor;
    int index;
    int x;
    int y;

    palette[0] = D_00ABACC8;
    if (actStockGet()->boost[0] != 0) {
        index = 1;
        actor = actStockGet()->boost[0];
    } else if (actStockGet()->boost[1] != 0) {
        index = 0;
        actor = actStockGet()->boost[1];
    } else {
        return;
    }
    x = (int) cursor->slide.pos.x;
    y = (int) cursor->slide.pos.y;
    grGpInit(&packet);
    func_00AA84D0(&packet);
    grPutSprOpen(&packet);
    menuStatSpriteRGB(&packet, x - 42, y + 26, 110, 24, 146, 14, &palette[0].bytes[index * 4]);
    grPutSprClose();
    menuKeyDraw(&packet, x,
                  ((void *) actor != tgtUnitGet()) ? y : y - 4,
                  actor->stats->key, 1);
    grPacketSend(&packet);
}
