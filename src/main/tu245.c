#include "common.h"
#include "main/tu206.h"
#include "main/tu245.h"
#include "main/xgl_cd.h"

extern void *memset(void *destination, int value, unsigned int count);

/*
 * One entry of the 20-entry text tag table. `kind` is the glyph index (1..5) drawn by
 * textTagDrawGlyph; 0 marks a free entry. payload[0] is compared with the current time by
 * the activity test func_002872C0; payload[1] and payload[2] are the glyph x and y.
 */
struct TextTag {
    int key;
    int payload[3];
    int kind;
};

extern struct TextTag textTagTable[];

/* Sign of a cursor step: 1, -1 or 0. msgWindowStepCursorSkipEmpty (main/tu238) adds it to the list cursor. */
int msgWindowStepDirection(int step)
{
    if (step > 0) {
        return 1;
    }
    if (step < 0) {
        return -1;
    }
    return 0;
}

/* Clears the first six tag entries (0x78 bytes). */
void textTagTableClear(void)
{
    memset(textTagTable, 0, 0x78);
}

int func_001E0768(char *name, void **data, unsigned int *size, void (*callback)(int index, void *buffer, unsigned int size, int value));
extern char msgBotanPath[];
extern void *msgBotanHandle;

/* Loads omiya\botan.xtx (the path string msgBotanPath) into the handle msgBotanHandle and waits for the load. */
void msgWindowLoadBotanXtx(void)
{
    func_001E0768(msgBotanPath, &msgBotanHandle, 0, 0);
    while (xglCdServiceIfPending() != 0) {
    }
}

int func_001DF818(void *buffer);

/* Releases the botan.xtx handle. */
void msgWindowUnloadBotanXtx(void)
{
    func_001DF818(msgBotanHandle);
    msgBotanHandle = 0;
}

/*
 * Frame draw: finds the first tag that func_002872C0 reports active, sets up the frame
 * state from `info` (grSetupFrameState), draws every active tag with textTagDrawGlyph, and ends
 * the packet with grFlushQuadBatch. Does nothing when no tag is active.
 */
INCLUDE_ASM("asm/main/nonmatchings/tu245", textTagDrawActive);

/* Activity test for one tag entry at the given time (its callee is not identified). */
INCLUDE_ASM("asm/main/nonmatchings/tu245", func_002872C0);

/* Frame state for a window described by info: GS registers set from info and sRender. */
INCLUDE_ASM("asm/main/nonmatchings/tu245", grSetupFrameState);

/* Frame state with the scissor covering the full 512 pixels, from sRender. */
INCLUDE_ASM("asm/main/nonmatchings/tu245", grSetupFullFrameState);


/* Writes the RGBAQ register (GS register 0x01) with a packed colour for the next vertices. */
void grSetRgbaq(int value)
{
    xglPacketScratchAddQuad(1, value);
}

/* Draws the glyph of kind 1..5 (glyph rectangles in textTagGlyphRects) at x and y + 10. */
INCLUDE_ASM("asm/main/nonmatchings/tu245", textTagDrawGlyph);

/* Ends the packet: sets the scissor, patches the tag count and sends the batch. */
INCLUDE_ASM("asm/main/nonmatchings/tu245", grFlushQuadBatch);

struct TextTag *textTagFindFree(void);

/* Adds a tag in the first free entry; a zero kind leaves the table unchanged. */
void textTagAdd(int key, int payload0, int payload1, int payload2, int kind)
{
    struct TextTag *record;

    record = textTagFindFree();
    if (kind != 0) {
        record->key = key;
        record->payload[0] = payload0;
        record->kind = kind;
        record->payload[1] = payload1;
        record->payload[2] = payload2;
    }
}

/* First free tag entry (kind 0), or NULL when the table is full. */
INCLUDE_ASM("asm/main/nonmatchings/tu245", textTagFindFree);

/* Clears every in-use tag whose key matches. */
void textTagRemoveKey(int key)
{
    int index;

    for (index = 0; index < 20; index++) {
        if (textTagTable[index].kind != 0 && textTagTable[index].key == key) {
            memset(&textTagTable[index], 0, sizeof(struct TextTag));
        }
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu245", func_002878D0);

INCLUDE_ASM("asm/main/nonmatchings/tu245", func_00287920);

/*
 * The message window as msgWindowDrawListIcons reads it: only the word at +0x80, which is
 * MsgWindow.list.item_count in main/tu238 (list at +0x28, count at list +0x58). It holds the number
 * of filled list entries; the icons are bright when it is nonzero.
 */
struct MsgWindow {
    unsigned char unmodeled_00[0x80];
    int item_count;
};

void func_001B2120(int);
void grSetupFullFrameState(void);
void textTagDrawGlyph(int, int, int);
void grFlushQuadBatch(void);

/*
 * Draws the two list icons (kinds 1 and 4) at x 0xA7 and 0x10B, y 0x152, called from msgWindowDraw in the list
 * branch: bright when the list has items (window->item_count), dim otherwise.
 */
void msgWindowDrawListIcons(struct MsgWindow *window)
{
    func_001B2120(msgBotanHandle);
    grSetupFullFrameState();
    if (window->item_count != 0) {
        grSetRgbaq(0x80808080);
    } else {
        grSetRgbaq(0x80404040);
    }
    textTagDrawGlyph(0xA7, 0x152, 1);
    grSetRgbaq(0x80808080);
    textTagDrawGlyph(0x10B, 0x152, 4);
    grFlushQuadBatch();
}
