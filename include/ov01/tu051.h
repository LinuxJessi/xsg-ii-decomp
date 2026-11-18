#ifndef INCLUDE_OV01_TU051_H
#define INCLUDE_OV01_TU051_H

#include "ov01/tu005.h"

/* Types and functions of src/ov01/tu051.c that other files use. */

/*
 * The slide block of the menu cursor (+0x10..+0x8F of MenuCursor):
 * menuCursorCreate clears it with ov01MatrixBlockInit, menuCursorUpdate steps
 * it with slideSetStep/objStdMove from the position towards the target.
 */
typedef union MenuCursorSlide {
    struct MatrixBlock block;
    struct {
        float x;                          /* +0x00 */
        float y;                          /* +0x04 */
        float z;                          /* +0x08 */
        unsigned char unmodeled_0c[0x60 - 0x0C];
        float target_x;                   /* +0x60 */
        float target_y;                   /* +0x64 */
        float target_z;                   /* +0x68 */
    } pos;
} MenuCursorSlide;

/* A sliding menu object, an objSlotAlloc slot: the menu cursor (menuCursor),
 * the icon list (menuIconListObj) and the five path objects of objPathTable
 * (tu050) and the stat records of menuStatRecords (tu049) are built this way. From +0x94 on the fields depend
 * on the kind. */
typedef struct MenuObject MenuObject;

struct MenuObject {
    unsigned char unmodeled_00[0x04];
    void (*update)(MenuObject *object);   /* +0x04 */
    void (*draw)(MenuObject *object);     /* +0x08 */
    unsigned char unmodeled_0c[0x04];
    MenuCursorSlide slide;                /* +0x10 */
    int moving;                           /* +0x90: 1 while the slide runs to the target */
    union {
        struct {
            int visible;                  /* +0x94: menuCursorDraw draws the unit list when 1 */
        } cursor;
        struct {
            struct Ov01Object *unit;      /* +0x94: the unit the record shows */
            float shown[3];               /* +0x98: the gauge values shown, eased towards the unit's by
                                           * menuStatGaugeStep */
        } stat;
        struct {
            int slot;                     /* +0x94: its index in objPathTable (and actStockGet's table) */
            int index;                    /* +0x98: current point of objPathPoints */
            int fade;                     /* +0x9C: stepped by 4 up to 0x18 by objMoveUpdate */
            int shown;                    /* +0xA0 */
        } path;
    } kind;
};

typedef MenuObject MenuCursor;

extern MenuCursor *menuCursor;

/* Home position of the cursor, and the parked position menuCursorCreate and
 * menuCursorHide set. */
#define MENU_CURSOR_HOME_X 288.0f
#define MENU_CURSOR_HOME_Y 390.0f
#define MENU_CURSOR_PARKED_Y 454.0f

void menuCursorCreate(void);

#endif /* INCLUDE_OV01_TU051_H */
