#include "common.h"
#include "main/tu092.h"
#include "main/tu096.h"
#include "main/tu097.h"

INCLUDE_ASM("asm/main/nonmatchings/tu097", fontTrackSetPoint);

INCLUDE_ASM("asm/main/nonmatchings/tu097", fontTrackSetBoth);

INCLUDE_ASM("asm/main/nonmatchings/tu097", fontTrackAdvance);

#include "shared.h"

/* 1 when the current pair of a track equals its target pair. */
int fontTrackAtTarget(int *track_words) {
    int at_target = 0;

    if (track_words[0] == track_words[2]) {
        at_target = track_words[1] == track_words[3];
    }
    return at_target;
}

INCLUDE_ASM("asm/main/nonmatchings/tu097", fontObjInitGlobals);

INCLUDE_ASM("asm/main/nonmatchings/tu097", fontObjLoadTexture);

INCLUDE_ASM("asm/main/nonmatchings/tu097", func_0020D0D0);

void xglDirectQueueBegin(void);
void xglDirectQueuePush(int, int);
void func_001D33D8(void);

/* Brackets one TEXFLUSH (GS register 0x3F) write with the GS begin and end helpers. */
void fontObjTexFlush(void) {
    xglDirectQueueBegin();
    xglDirectQueuePush(0x3F, 0);
    func_001D33D8();
}

INCLUDE_ASM("asm/main/nonmatchings/tu097", fontObjBuildVifPacket);

INCLUDE_ASM("asm/main/nonmatchings/tu097", func_0020D260);

extern int fontObjBoundSlot;
extern int fontObjDepth;

void fontObjResetState(void) {
    fontObjDepth = 0;
    fontObjBoundSlot = -1;
    fontObjTexFlush();
}

void func_0020D260(void);

void fontObjResetAndBuild(void) {
    fontObjResetState();
    func_0020D260();
}

extern signed char D_0069E200[6];
extern signed char D_0069E208[6];

/* One 0x1C-byte track of struct FontObject. current_pair moves toward target_pair under the
 * CE98 advance; the scaled copies and the step counter at +0x10 are written by the asm callees. */
struct FontTrack {
    int current_pair[2];
    int target_pair[2];
    unsigned char unmodeled_10[12];
};

/* Four tracks at 0x00, then the words and bytes the callers touch. draw_bits is the Q word
 * fontObjDrawQuad receives; move_phase is 0 idle, 1 opening and 2 closing. The active flag
 * 0x10000 in flags is set when an opening starts and cleared when a closing ends or
 * fontObjAdvanceTracks runs. */
struct FontObject {
    struct FontTrack tracks[4];
    int draw_bits;
    int flags;
    unsigned char move_phase;
    unsigned char aux_state;
};

void fontObjClearSlots(void)
{
    int slot;

    xglCdWaitIdle();
    for (slot = 0; slot < 6; slot++) {
        D_0069E200[slot] = -1;
        D_0069E208[slot] = 0;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu097", func_0020D430);

extern int fontObjRunLists[];

/* Zeroes the six run-list words of fontObjRunLists, from the last slot down. */
int *fontObjClearRunLists(void)
{
    int *run_lists;
    int *cursor;
    int slot;

    fontObjClearSlots();
    run_lists = fontObjRunLists;
    cursor = run_lists + 5;
    for (slot = 0; slot < 6; slot++) {
        *cursor = 0;
        cursor--;
    }

    return cursor;
}

INCLUDE_ASM("asm/main/nonmatchings/tu097", fontObjDraw);

extern u16 D_003E7EE6[];

/* Cell-table fields: the u16 at +6 (and +8 in fontObjCellOffsetY) of 16-byte entry index. */
u16 fontObjCellOffsetX(int index) {
    return D_003E7EE6[index * 8];
}

extern u16 D_003E7EE8[];

u16 fontObjCellOffsetY(int index) {
    return D_003E7EE8[index * 8];
}

extern u64 fontObjColor;

void fontObjSetColor(u64 colour) {
    fontObjColor = colour;
}

INCLUDE_ASM("asm/main/nonmatchings/tu097", fontObjSetColorRgba);

void fontObjSetAlpha(int alpha) {
    fontObjColor = (fontObjColor & 0xFFFFFF) | (((long long) alpha & 0xFF) << 0x18);
}

u64 fontObjGetColor(void) {
    return fontObjColor;
}

void fontObjResetColor(void) {
    fontObjColor = 0x3F80000080808080LL;
}

/* Sets the depth word that fontObjDraw writes into the high half (Z) of each XYZ2 vertex. */
void fontObjSetDepth(int depth) {
    fontObjDepth = depth;
}

extern int fontObjOpacity;

void fontObjSetOpacity(int opacity) {
    fontObjOpacity = opacity;
}

u8 fontObjGetOpacity(void) {
    return fontObjOpacity;
}

void fontObjResetOpacity(void) {
    fontObjOpacity = 0x80;
}

int fontTrackSetBoth(struct FontTrack *track, int x, int y);

/* Resets all four tracks to (0,0), clears extra state and sets flags to 0x12. */
void fontObjInitObject(struct FontObject *object)
{
    fontTrackSetBoth(&object->tracks[0], 0, 0);
    fontTrackSetBoth(&object->tracks[1], 0, 0);
    fontTrackSetBoth(&object->tracks[2], 0, 0);
    fontTrackSetBoth(&object->tracks[3], 0, 0);
    object->draw_bits = 0;
    object->aux_state = 0;
    object->move_phase = 0;
    object->flags = 0x12;
}

INCLUDE_ASM("asm/main/nonmatchings/tu097", func_0020DCF0);

void fontTrackSetPoint(struct FontTrack *track, int x, int y);

/* Starts the closing of the object (phase 2) unless it is already closing: track 2 heads to the
 * target of track 0 and track 3, the size, heads to zero. func_0020DFA8 ends the closing. */
void fontObjStartClose(struct FontObject *object)
{
    unsigned char phase;
    struct FontTrack *size_track;

    phase = object->move_phase;
    if (phase != 2) {
        if (object->flags & 2) {
            if (phase == 1) {
                fontTrackSetTarget(&object->tracks[2], object->tracks[0].target_pair[0],
                              object->tracks[0].target_pair[1]);
                size_track = &object->tracks[3];
            } else {
                fontTrackSetPoint(&object->tracks[2], object->tracks[0].current_pair[0],
                              object->tracks[0].current_pair[1]);
                fontTrackSetTarget(&object->tracks[2], object->tracks[0].target_pair[0],
                              object->tracks[0].target_pair[1]);
                fontTrackSetPoint(&object->tracks[3], object->tracks[1].current_pair[0],
                              object->tracks[1].current_pair[1]);
                size_track = &object->tracks[3];
            }
            fontTrackSetTarget(size_track, 0, 0);
        } else {
            fontTrackSetBoth(&object->tracks[2], object->tracks[0].target_pair[0],
                          object->tracks[0].target_pair[1]);
            fontTrackSetBoth(&object->tracks[3], 0, 0);
        }
        object->move_phase = 2;
    }
}

void fontTrackAdvance(struct FontTrack *track);

void fontObjAdvanceTracks(struct FontObject *object)
{
    fontTrackAdvance(&object->tracks[0]);
    fontTrackAdvance(&object->tracks[1]);
    object->move_phase = 0;
    object->flags &= 0xFFFEFFFF;
}

void func_0020CD50(struct FontTrack *track);
int fontTrackAtTarget(int *track_words);

/* Opening step (phase 1), called by fontObjUpdate: advances tracks 2 and 3; once both are at their
 * targets, their current pairs become the new targets of tracks 0 and 1 and the phase returns to idle. */
void fontObjSettleTracks(struct FontObject *object)
{
    if (object->flags & 2) {
        func_0020CD50(&object->tracks[2]);
        func_0020CD50(&object->tracks[3]);
    } else {
        fontTrackAdvance(&object->tracks[2]);
        fontTrackAdvance(&object->tracks[3]);
    }
    if (fontTrackAtTarget(&object->tracks[2]) != 0 && fontTrackAtTarget(&object->tracks[3]) != 0) {
        fontTrackSetTarget(&object->tracks[0], object->tracks[2].current_pair[0], object->tracks[2].current_pair[1]);
        fontTrackSetTarget(&object->tracks[1], object->tracks[3].current_pair[0], object->tracks[3].current_pair[1]);
        object->move_phase = 0;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu097", func_0020DFA8);

INCLUDE_ASM("asm/main/nonmatchings/tu097", fontObjDrawQuad);

INCLUDE_ASM("asm/main/nonmatchings/tu097", fontObjGetQuadCorners);

void fontObjDrawQuad(int x0, int y0, int x1, int y1, int q_bits, int draw_bits);
void fontObjGetQuadCorners(struct FontObject *object, int *corners);

/* 0x40000000 is the bit pattern of 2.0f; the callee takes it in a general register. */
void fontObjDrawQuadFromTracks(struct FontObject *object)
{
    int corners[4];

    fontObjGetQuadCorners(object, corners);
    fontObjDrawQuad(corners[0], corners[1], corners[2], corners[3], 0x40000000, object->draw_bits);
}

INCLUDE_ASM("asm/main/nonmatchings/tu097", fontObjUpdate);

/* Text block: a track at +0x1C and a sub-object at +0x58. E3C0 reads the block as a FontObject,
 * so its flags word at +0x74 is the object flags. origin_x and origin_y (the current pair of track 0)
 * and text are stored by fontObjSetTextPosition; fontObjLayoutString reads +0x10 as the string it lays out. */
typedef struct FontPrintBlock {
    int origin_x;
    int origin_y;
    unsigned char unmodeled_08[8];
    int text;
    unsigned char unmodeled_14[8];
    unsigned char track_1c[0x3C];
    unsigned char sub_object[0x1C];
    unsigned int flags;
} FontPrintBlock;

/* Draws the object's box quad only while its active flag (0x10000) is set. */
void fontObjDrawIfActive(FontPrintBlock *block) {
    if (block->flags & 0x10000) {
        fontObjDrawQuadFromTracks((struct FontObject *) block);
    }
}

void fontObjUpdate(void *obj);

/* Ticks the sub-object of a text block (block+0x58) through fontObjUpdate. */
void fontObjSubUpdate(void *obj) {
    fontObjUpdate(obj);
}

/* Sets track 0 of the object (current and target pair): the position of the box corner. */
int fontObjSetPosition(void *obj, int x, int y) {
    return fontTrackSetBoth(obj, x, y);
}

/* Sets track 1 of the object (current and target pair): the size that GetQuadCorners adds to the position. */
int fontObjSetSize(FontPrintBlock *block, int x, int y) {
    return fontTrackSetBoth(&block->track_1c, x, y);
}

/* Sets the target pair of track 0 (the position, see fontObjSetPosition) of the object. */
void fontObjSetPositionTarget(void *obj, int x, int y) {
    fontTrackSetTarget(obj, x, y);
}

void fontObjSetExtra(struct FontObject *obj, int draw_bits) {
    obj->draw_bits = draw_bits;
}

/* Sets the target pair of track 1 (the size, at +0x1C, see fontObjSetSize) of the object. */
void fontObjSetSizeTarget(u8 *obj, int x, int y) {
    fontTrackSetTarget(obj + 0x1C, x, y);
}

INCLUDE_ASM("asm/main/nonmatchings/tu097", func_0020E470);

INCLUDE_ASM("asm/main/nonmatchings/tu097", func_0020E490);

INCLUDE_ASM("asm/main/nonmatchings/tu097", func_0020E4B0);

/* Stores the print origin (origin_x, origin_y) of a text block and the text word at +0x10 that the layout reads. */
void fontObjSetTextPosition(FontPrintBlock *block, int origin_x, int origin_y, int text) {
    if (block != 0) {
        block->origin_x = origin_x;
        block->origin_y = origin_y;
        block->text = text;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu097", fontObjLayoutString);

/* Three words: an offset point; the first two are displaced and restored around the call. */
struct FontPoint {
    int x;
    int y;
    int z;
};

void fontObjLayoutString(struct FontPoint *point);

/* Shifts point by (dx, dy), sets z, lays the text out, then restores the point. */
void fontObjLayoutAtOffset(int dx, int dy, int z, struct FontPoint *point)
{
    int saved_z;
    int x;
    int y;

    if (point != 0) {
        x = point->x + dx;
        saved_z = point->z;
        y = point->y + dy;
        point->x = x;
        point->y = y;
        point->z = z;
        fontObjLayoutString(point);
        x = point->x - dx;
        point->z = saved_z;
        y = point->y - dy;
        point->y = y;
        point->x = x;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu097", fontObjInitBlock);

INCLUDE_ASM("asm/main/nonmatchings/tu097", fontObjSetText);

void fontObjSetText(void *obj, int text);

void fontObjClearText(void *obj) {
    fontObjSetText(obj, 0);
}

void func_0020E950(void) {
}

INCLUDE_ASM("asm/main/nonmatchings/tu097", fontObjDrawText);

int func_0020DCF0(void *object);

/* Opens the sub-object of a text block through func_0020DCF0 (the phase 1 start). */
void fontObjSubStartOpen(FontPrintBlock *block) {
    if (block != 0) {
        func_0020DCF0(&block->sub_object);
    }
}


/* Starts the closing of the sub-object of a text block through fontObjStartClose. */
void fontObjSubStartClose(FontPrintBlock *block) {
    if (block != 0) {
        fontObjStartClose(&block->sub_object);
    }
}


/* Advances the tracks of the sub-object of a text block; the advance ends any move and clears the active flag. */
void fontObjSubAdvanceTracks(FontPrintBlock *block) {
    if (block != 0) {
        fontObjAdvanceTracks(&block->sub_object);
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu097", func_0020EAD0);

INCLUDE_ASM("asm/main/nonmatchings/tu097", func_0020EBA0);

void func_0020EBA0(void *obj, int text);

/* Clears the text of a cursor buffer entry: func_0020EBA0 with a null text (its only caller, cursorBufferPush, passes pool entries). */
void cursorBufferClearText(void *obj) {
    func_0020EBA0(obj, 0);
}

void func_0020EBF0(void) {
}

INCLUDE_ASM("asm/main/nonmatchings/tu097", func_0020EBF8);
