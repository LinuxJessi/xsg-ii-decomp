#include "common.h"
#include "gs.h"
#include "main/tu093.h"
#include "main/tu206.h"
#include "main/tu229.h"
#include "main/tu236.h"
#include "main/tu242.h"
#include "main/tu245.h"
#include "main/xgl_cd.h"

extern void *memset(void *destination, int value, unsigned int count);
extern unsigned int strlen(const char *string);

#define TEXT_TAG_CLEAR_COUNT 6          /* entries cleared by textTagTableClear */
#define GLYPH_KIND_MAX 5                /* glyph kinds run 1..5 */
#define GLYPH_Y_OFFSET 10               /* glyphs are drawn this far below the tag y */
#define GLYPH_ICON_LEFT_KIND 1          /* the two list icons of msgWindowDrawListIcons */
#define GLYPH_ICON_RIGHT_KIND 4
#define GLYPH_ICON_LEFT_X 0xA7
#define GLYPH_ICON_RIGHT_X 0x10B
#define GLYPH_ICON_Y 0x152
#define RGBAQ_GRAY 0x80808080           /* RGBAQ with every channel 0x80 */
#define RGBAQ_GRAY_DIM 0x80404040       /* the dimmed icon: colour channels 0x40 */
#define FRAME_SCISSOR_TOP_INSET 0x21    /* the text window scissor starts this far below the slot y (as talkBoxScissorCallback in tu234) */
#define FRAME_SCISSOR_BOTTOM_INSET 0x20
#define FRAME_SCISSOR_WINDOW_RIGHT 0x200 /* right edge of the window scissor, one pixel wider than the screen scissor */
#define FRAME_SPRITE_PRIM 0x15E         /* PRIM: sprite (6), IIP, TME, ABE and FST bits */
#define FRAME_ZBUF_VALUE 0x131000000ULL /* ZBUF_1: ZMSK 1, 24-bit format 1; the original constant also has bits 28-29 set */
#define FRAME_FRAME_BUFFER_WIDTH 8      /* FRAME_1 FBW in 64-pixel units */
#define FONT_TEXTURE_WIDTH 16           /* TEX0 TBW of the font texture */
#define FONT_TEXTURE_CLUT_OFFSET 0x10   /* CLUT sits this many words after the texture */

/*
 * One entry of the 20-entry text tag table. `kind` is the glyph index (1..5) drawn by
 * textTagDrawGlyph; 0 marks a free entry. The activity test textTagIsActive treats key as the
 * start of a string and payload[0] as a text position; payload[1] and payload[2] are the glyph x and y.
 */
struct TextTag {
    int key;
    int payload[3];
    int kind;
};

extern struct TextTag textTagTable[];


void xglPacketScratchBegin(void);
int gsMakeAlpha(int blend);

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

/* Clears the first TEXT_TAG_CLEAR_COUNT tag entries. */
void textTagTableClear(void)
{
    memset(textTagTable, 0, TEXT_TAG_CLEAR_COUNT * sizeof(struct TextTag));
}

int GameResLoadName(char *name, void **data, unsigned int *size, void (*callback)(int index, void *buffer, unsigned int size, int value));
extern char msgBotanPath[];
extern void *msgBotanHandle;

/* Loads omiya\botan.xtx (the path string msgBotanPath) into the handle msgBotanHandle and waits for the load. */
void msgWindowLoadBotanXtx(void)
{
    GameResLoadName(msgBotanPath, &msgBotanHandle, 0, 0);
    while (xglCdServiceIfPending() != 0) {
    }
}

int GameResBlockFreeByBuffer(void *buffer);

/* Releases the botan.xtx handle. */
void msgWindowUnloadBotanXtx(void)
{
    GameResBlockFreeByBuffer(msgBotanHandle);
    msgBotanHandle = 0;
}

void xglPacketTextureTrans(void *);
void grSetupFrameState(MsgSlot *slot);
void textTagDrawGlyph(int, int, int);
void grFlushQuadBatch(void);
int textTagIsActive(struct TextTag *tag, char *pos);

/*
 * Frame draw: finds the first tag that textTagIsActive reports active, sets up the frame
 * state from `info` (grSetupFrameState), draws every active tag with textTagDrawGlyph, and ends
 * the packet with grFlushQuadBatch. Does nothing when no tag is active.
 */
void textTagDrawActive(const char *text, int yOffset, MsgSlot *slot)
{
    int index;
    struct TextTag *tag;

    for (index = 0, tag = textTagTable; index < TEXT_TAG_COUNT; index++) {
        if (textTagIsActive(tag++, (char *)text)) {
            break;
        }
    }
    if (index == TEXT_TAG_COUNT) {
        return;
    }
    xglPacketTextureTrans(msgBotanHandle);
    grSetupFrameState(slot);
    for (index = 0; index < TEXT_TAG_COUNT; index++) {
        tag = &textTagTable[index];
        if (textTagIsActive(tag, (char *)text)) {
            textTagDrawGlyph(tag->payload[1], tag->payload[2] - yOffset, tag->kind);
        }
    }
    grFlushQuadBatch();
}

extern char *strchr(const char *string, int c);

/*
 * Activity test for one tag entry: true when the tag is in use, pos lies inside the tag's
 * string (from key up to its terminating NUL) and pos is past payload[0].
 */
int textTagIsActive(struct TextTag *tag, char *pos)
{
    if (tag->kind == 0) {
        return 0;
    }
    if (pos < (char *)tag->key || strchr((char *)tag->key, '\0') < pos) {
        return 0;
    }
    return (char *)tag->payload[0] < pos;
}

/* GS state shared by both frame setups: blend and test registers, frame buffer, font texture, sprite PRIM and white RGBAQ. */
#define GRSETUP_COMMON_STATE() \
    { \
        xglPacketScratchAddQuad(GS_COLCLAMP, 1); \
        xglPacketScratchAddQuad(GS_CLAMP_1, 0); \
        xglPacketScratchAddQuad(GS_ZBUF_1, FRAME_ZBUF_VALUE); \
        xglPacketScratchAddQuad(GS_TEX1_1, 0); \
        xglPacketScratchAddQuad(GS_TEST_1, 0); \
        xglPacketScratchAddQuad(GS_TEXA, 0); \
        xglPacketScratchAddQuad(GS_ALPHA_1, gsMakeAlpha(0)); \
        xglPacketScratchAddQuad(GS_FRAME_1, GS_SET_FRAME(sRender.display_base, FRAME_FRAME_BUFFER_WIDTH, GS_PSMCT32, 0)); \
        xglPacketScratchAddQuad(GS_TEX0_1, GS_SET_TEX0(sRender.frame_base_1 << 5, FONT_TEXTURE_WIDTH, GS_PSMT8, 6, 6, 1, 0, (sRender.frame_base_1 << 5) + FONT_TEXTURE_CLUT_OFFSET, 0, 0, 0, 1)); \
        xglPacketScratchAddQuad(GS_PRIM, FRAME_SPRITE_PRIM); \
        xglPacketScratchAddQuad(GS_RGBAQ, RGBAQ_GRAY); \
    }

/* Frame state for a window described by info: GS registers set from info and sRender. */
void grSetupFrameState(MsgSlot *slot)
{
    int top = slot->y;
    int bottom = slot->y + slot->extent_y;

    xglPacketScratchBegin();
    xglPacketScratchAddQuad(GIF_AD, TALK_GIF_TAG_AD_EOP);
    xglPacketScratchAddQuad(GS_TEXFLUSH, 0);
    xglPacketScratchAddQuad(GS_PABE, 0);
    xglPacketScratchAddQuad(GS_SCISSOR_1, GS_SET_SCISSOR(0, FRAME_SCISSOR_WINDOW_RIGHT, top + FRAME_SCISSOR_TOP_INSET, bottom - FRAME_SCISSOR_BOTTOM_INSET));
    GRSETUP_COMMON_STATE();
}

/* Frame state with the scissor covering the full 512 pixels, from sRender. */
void grSetupFullFrameState(void)
{
    xglPacketScratchBegin();
    xglPacketScratchAddQuad(GIF_AD, TALK_GIF_TAG_AD_EOP);
    xglPacketScratchAddQuad(GS_TEXFLUSH, 0);
    xglPacketScratchAddQuad(GS_PABE, 0);
    xglPacketScratchAddQuad(GS_SCISSOR_1, GS_SET_SCISSOR(0, TALK_SCREEN_RIGHT, 0, TALK_SCREEN_BOTTOM));
    GRSETUP_COMMON_STATE();
}

/* Writes the RGBAQ register (GS_RGBAQ) with a packed colour for the next vertices. */
void grSetRgbaq(int value)
{
    xglPacketScratchAddQuad(GS_RGBAQ, value);
}

/* Draws the glyph of kind 1..5 (glyph rectangles in textTagGlyphRects) at x and y + 10. */
/* Screen to GS primitive coordinates: 12.4 fixed point around the 2048 centre. */
#define GLYPH_GS_ORIGIN_X 1792.0f  /* GS primitive origin 2048 minus half the 512 pixel screen width */
#define GLYPH_GS_ORIGIN_Y 1824.0f  /* 2048 minus half the 448 pixel screen height */
#define GLYPH_GS_Z ((u64) 0x8000 << 17) /* XYZ z field (bit 32) set to 1 */
#define GLYPH_GS_X(x) ((int) (((float) (x) + GLYPH_GS_ORIGIN_X) * 16.0f))
#define GLYPH_GS_Y(y) ((int) (((float) (y) + GLYPH_GS_ORIGIN_Y) * 16.0f))

/* Texture rectangle of a glyph in textTagGlyphRects: top, left, right and bottom in texels. */
#define GLYPH_TOP 0
#define GLYPH_LEFT 1
#define GLYPH_RIGHT 2
#define GLYPH_BOTTOM 3
extern long long textTagGlyphRects[][4];

void textTagDrawGlyph(int x, int y, int kind)
{
    long long *rect = textTagGlyphRects[kind];

    if (kind > 0) {
        if (kind <= GLYPH_KIND_MAX) {
            struct {
                long long top;
                long long left;
                long long right;
                long long bottom;
            } screen;

            screen.left = x;
            screen.top = y + GLYPH_Y_OFFSET;
            screen.right = screen.left + (rect[GLYPH_RIGHT] - rect[GLYPH_LEFT]);
            screen.bottom = screen.top + (rect[GLYPH_BOTTOM] - rect[GLYPH_TOP]);
            xglPacketScratchAddQuad(GS_UV, (rect[GLYPH_LEFT] << 4) | (rect[GLYPH_TOP] << 20));
            xglPacketScratchAddQuad(GS_XYZ3, GLYPH_GS_X(screen.left) | ((long long) GLYPH_GS_Y(screen.top) << 16) | GLYPH_GS_Z);
            xglPacketScratchAddQuad(GS_UV, (rect[GLYPH_RIGHT] << 4) | (rect[GLYPH_BOTTOM] << 20));
            xglPacketScratchAddQuad(GS_XYZF2, GLYPH_GS_X(screen.right) | ((long long) GLYPH_GS_Y(screen.bottom) << 16) | GLYPH_GS_Z);
        }
    }
}

void xglPacketScratchPatchTag(void);
void xglPacketScratchAddDirect(void);

/* Ends the packet: sets the scissor, patches the tag count and sends the batch. */
void grFlushQuadBatch(void)
{
    xglPacketScratchAddQuad(GS_TEXFLUSH, 0);
    xglPacketScratchAddQuad(GS_SCISSOR_1, GS_SET_SCISSOR(0, TALK_SCREEN_RIGHT, 0, TALK_SCREEN_BOTTOM));
    xglPacketScratchPatchTag();
    xglPacketScratchAddDirect();
}

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
struct TextTag *textTagFindFree(void)
{
    int index;

    for (index = 0; index < TEXT_TAG_COUNT; index++) {
        if (textTagTable[index].kind == 0) {
            return &textTagTable[index];
        }
    }
    return 0;
}

/* Clears every in-use tag whose key matches. */
void textTagRemoveKey(int key)
{
    int index;

    for (index = 0; index < TEXT_TAG_COUNT; index++) {
        if (textTagTable[index].kind != 0 && textTagTable[index].key == key) {
            memset(&textTagTable[index], 0, sizeof(struct TextTag));
        }
    }
}

/* Names of the tag glyphs, indexed by glyph kind (1..5). */
extern char *D_0063D5F0[];

/* Returns text advanced past the name of tag glyph index (1..5); other indices leave text unchanged. */
char *textSkipGlyphName(char *text, int index)
{
    if (index <= 0) {
        return text;
    }
    if (index > GLYPH_KIND_MAX) {
        return text;
    }
    return text + strlen(D_0063D5F0[index]);
}

/* Returns 1 when text starts with the name of tag glyph index, else 0. */
int textStartsWithGlyphName(char *text, int index)
{
    char *name = D_0063D5F0[index];

    while (*name != 0) {
        char c = *name;
        char t = *text++;

        if (t != c) {
            return 0;
        }
        if (t == 0) {
            return 0;
        }
        name++;
    }
    return 1;
}

void grSetupFullFrameState(void);

/*
 * Draws the two list icons (kinds 1 and 4) at x GLYPH_ICON_LEFT_X and GLYPH_ICON_RIGHT_X, y GLYPH_ICON_Y, called from msgWindowDraw in the list
 * branch: bright when the list has items (u.list.item_count), dim otherwise.
 */
void msgWindowDrawListIcons(MsgSlot *window)
{
    xglPacketTextureTrans(msgBotanHandle);
    grSetupFullFrameState();
    if (window->u.list.item_count != 0) {
        grSetRgbaq(RGBAQ_GRAY);
    } else {
        grSetRgbaq(RGBAQ_GRAY_DIM);
    }
    textTagDrawGlyph(GLYPH_ICON_LEFT_X, GLYPH_ICON_Y, GLYPH_ICON_LEFT_KIND);
    grSetRgbaq(RGBAQ_GRAY);
    textTagDrawGlyph(GLYPH_ICON_RIGHT_X, GLYPH_ICON_Y, GLYPH_ICON_RIGHT_KIND);
    grFlushQuadBatch();
}
