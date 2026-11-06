#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tu105", menuPlaceItemCursor);

INCLUDE_ASM("asm/main/nonmatchings/tu105", menuRefreshItems);

INCLUDE_ASM("asm/main/nonmatchings/tu105", menuDraw);

INCLUDE_ASM("asm/main/nonmatchings/tu105", func_00226948);

INCLUDE_ASM("asm/main/nonmatchings/tu105", menuSetMode);

typedef signed char s8;

typedef int s32;

/* A cursor menu. Partial layout: only the fields this TU's functions touch.
 * Item i uses the 0x40-byte stride at object + i * 0x40: its sub-objects sit at +0x90 and +0xAC of that
 * stride, its flags word at +0xC8 (0x1 selected, 0x8 disabled). cursor is the selected index, wrapped modulo
 * item_count; prev_cursor is the one before it.
 * flags: 0x4 locks the cursor. */
typedef struct MenuObject {
    s32 state_words[4];
    unsigned char unmodeled_10[0x3CC - 0x10];
    s32 cursor;
    s32 prev_cursor;
    unsigned char unmodeled_3D4[4];
    s32 item_count;
    unsigned char unmodeled_3DC[8];
    s32 flags;
} MenuObject;

void fontTrackAdvance(void *);
void menuPlaceItemCursor(void *, s32);
void menuSetMode();

void menuHide(MenuObject *object) {
    s32 index;
    char *element;

    index = 0;
    menuSetMode();
    if (object->item_count > 0) {
        element = (char *) object;
        do {
            index += 1;
            fontTrackAdvance(element + 0x90);
            fontTrackAdvance(element + 0xAC);
            element += 0x40;
        } while (index < object->item_count);
    }
    menuPlaceItemCursor(object, object->cursor);
    fontTrackAdvance((char *) object + 0x1C);
    fontTrackAdvance((char *) object + 0x38);
    fontTrackAdvance((char *) object + 0x54);
}

INCLUDE_ASM("asm/main/nonmatchings/tu105", menuSkipDisabled);

s32 seVoiceStartDefault(s32);
s32 menuSkipDisabled();

/* Moves the cursor by delta and plays the cursor sound when it actually moved. */
void menuMoveCursor(MenuObject *object, s32 delta) {
    s32 old_cursor;

    if (object->flags & 4) {
        return;
    }
    old_cursor = object->cursor;
    object->prev_cursor = old_cursor;
    object->cursor = old_cursor + delta;
    menuSkipDisabled(object);
    if (object->prev_cursor != object->cursor) {
        seVoiceStartDefault(0x01000001);
    }
}

/* Sets the cursor to an absolute index. */
void menuSetCursor(MenuObject *object, s32 value) {
    s32 old_cursor;

    old_cursor = object->cursor;
    object->cursor = value;
    object->prev_cursor = old_cursor;
    menuSkipDisabled(object);
}

INCLUDE_ASM("asm/main/nonmatchings/tu105", menuNavigate);

INCLUDE_ASM("asm/main/nonmatchings/tu105", menuSetSelected);

INCLUDE_ASM("asm/main/nonmatchings/tu105", func_00227408);

INCLUDE_ASM("asm/main/nonmatchings/tu105", func_00227458);

INCLUDE_ASM("asm/main/nonmatchings/tu105", func_002275D0);

void menuResetState(MenuObject *object) {
    if (object != 0) {
        object->state_words[0] = 0;
        object->state_words[1] = 0;
        object->state_words[2] = 0;
        object->state_words[3] = 0;
    }
}
