#include "common.h"
#include "shared.h"

extern void *memset(void *destination, int value, unsigned int count);

/*
 * One 0x98-byte slot of a message window (msgWindowRunSlots runs five of them, tu215.c). Its type
 * at +0x1C selects the handlers msgSlotRun calls: 1 text, 2 window, 3 timecode, 4 item or choice.
 * msgSlotInit clears it and sets the type.
 */
typedef struct MsgSlot {
    u32 flags;                        /* +0x00: status bits */
    unsigned char unmodeled_04[0x14];
    int reset_value;                  /* +0x18 */
    int state;                        /* +0x1C: 3 and 4 select the handlers */
    u32 init_word;                    /* +0x20 */
    unsigned char unmodeled_24[0x18];
    int pending;                      /* +0x3C */
    int timer;                        /* +0x40: counted down by the type 4 tick while flag 0x8 is set */
    unsigned char unmodeled_44[0x4];
    long long wide_first;             /* +0x48: 64-bit value stored by timecodeSetFramePair */
    long long wide_second;            /* +0x50: 64-bit value stored by timecodeSetFramePair */
    unsigned char unmodeled_58[0x24];
    int stored_7c;                    /* +0x7C */
    int stored_80;                    /* +0x80 */
    int stored_84;                    /* +0x84 */
    unsigned char unmodeled_88[0x10];
} MsgSlot;

/* Timecode slot (type 3): stores the two 64-bit frame counts that func_0027DBE8 (tu224) computes and sets flag 0x80. */
void timecodeSetFramePair(MsgSlot *record, long long first, long long second) {
    if (record->state == 3) {
        record->wide_second = second;
        record->wide_first = first;
        record->flags |= 0x80;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu242", func_00286100);

/* The same slot read as an object by the type 3 and type 4 handlers below, with the fields those handlers use (offsets as in MsgSlot). */
struct StateObject {
    int flags;
    int x;                      /* +0x04: slot origin x, stored by msgSlotSetGeometry */
    int y;                      /* +0x08: slot origin y */
    int unmodeled_0C;
    int extent_x;               /* +0x10: width, at least CLAMP_MINIMUM */
    int extent_y;               /* +0x14: height */
    int unmodeled_18;
    int state;
    int unmodeled_20[8];
    int timer;
    int unmodeled_44[14];
    int state3_value;
};

#define STATE_THREE 3
#define STATE_FOUR 4
#define FLAG_80 0x80
#define FLAG_02 0x2
#define FLAG_04 0x4
#define CLAMP_MINIMUM 0x5E

void timecodeClearFlag80(struct StateObject *object)
{
    if (object->state == STATE_THREE) {
        object->flags &= ~FLAG_80;
    }
}

void func_00286228(struct StateObject *object, int value)
{
    if (object->state == STATE_THREE) {
        object->state3_value = value;
    }
}

void func_00286240(MsgSlot *record, int value) {
    if (record->state == 3) {
        record->stored_80 = value;
        record->stored_84 = 0;
        record->flags |= 0x800;
    }
}

void msgSlotInit(MsgSlot *record, int state);

void msgSlotInitState4(MsgSlot *record)
{
    msgSlotInit(record, 4);
}

void msgSlotSetGeometry();

void msgSlotSetGeometryIfState4(MsgSlot *record) {
    if (record->state == 4) {
        msgSlotSetGeometry();
    }
}

void func_002862C8(int value, int option, int extra);

void msgSlotSetAcquiredItem(int value, int option)
{
    func_002862C8(value, option, 0);
}

INCLUDE_ASM("asm/main/nonmatchings/tu242", func_002862C8);

void msgSlotSetTimer(struct StateObject *object, int value)
{
    if (object->state == STATE_FOUR) {
        object->timer = value;
    }
}

void msgSlotSetFlag04(struct StateObject *object)
{
    if (object->state == STATE_FOUR) {
        object->flags = (object->flags & ~FLAG_02) | FLAG_04;
    }
}

void msgSlotResetCounters(MsgSlot *record) {
    u32 flags_without_4;

    if (record->state == 4) {
        record->pending = 0;
        record->reset_value = 0xC;
        flags_without_4 = record->flags & ~4;
        record->flags = (flags_without_4 & ~8) | 0x400;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu242", func_002863D0);

int func_0027EC18(void *record);
void drawAcquiredText(void *record);

int msgSlotDraw(MsgSlot *record) {
    int result;

    if (record->state == 4) {
        if (record->flags & 4) {
            func_0027EC18(record);
        }
        if (record->flags & 2) {
            func_0027EC18(record);
        }
        if (record->flags & 8) {
            func_0027EC18(record);
            drawAcquiredText(record);
        }
        result = record->flags & 0x400;
        if (result != 0) {
            result = func_0027EC18(record);
        }
        return result;
    }
}

void msgSlotSetFlag02(MsgSlot *record) {
    int clear_mask;

    clear_mask = ~4;
    if (record->state == 4) {
        record->flags = ((record->flags & clear_mask) & ~8) | 2;
    }
}

extern u8 *textOutputCursor;
extern unsigned char textOutputBuffer[];
void textTagTableClear(void);

/* Resets the text output: the cursor goes to the start of the output buffer, which becomes an empty string. */
void textOutputReset(void) {
    textOutputCursor = textOutputBuffer;
    memset(textOutputBuffer, 0, 2);
    textTagTableClear();
}

char *textOutputAdvance(void)
{
    char *next;

    next = textOutputCursor + 1;
    textOutputCursor = next;
    *next = '\0';
    return textOutputCursor;
}

INCLUDE_ASM("asm/main/nonmatchings/tu242", textFormatWrapped);

INCLUDE_ASM("asm/main/nonmatchings/tu242", textIsNonDecimalEscape);

void textCopyCounted(u8 *dst, u8 *src, int count);
extern unsigned char D_0069DD50[];

/* Appends the two-byte tag held in D_0069DD50 at the output cursor and counts it as two characters. */
int textAppendPairTag(unsigned int *cursor, int *total) {
    int new_total;

    textCopyCounted((u8 *) *cursor, D_0069DD50, 2);
    new_total = *total + 2;
    *total = new_total;
    *cursor += 2;
    return new_total;
}

INCLUDE_ASM("asm/main/nonmatchings/tu242", func_00286958);

int textLineWidth(int end);

int textRemainingWidth(int end, int value)
{
    return value - textLineWidth(end);
}

char *textLineStart(char *text);
int func_002AA7F0();
int func_002AAE40();
int xglFontGetStringWidth();
extern unsigned char textPropTag[];
extern unsigned char D_0069DD58[];

/* Width of the line that ends at end: "$prop1;" and the line text are built in the scratchpad buffer, then measured. */
int textLineWidth(int end) {
    char *scratch = (char *) 0x70000000;
    int start;

    start = (int) textLineStart((char *) end);
    *scratch = 0;
    func_002AA7F0(scratch, textPropTag);
    func_002AA7F0(scratch, D_0069DD58);
    func_002AAE40(scratch, start, end - start + 1);
    return xglFontGetStringWidth(scratch);
}

/* Start of the line that holds text: scans back to the previous newline or to the string start. */
char *textLineStart(char *text) {
    char *scan;
    int first_char;
    int ch;

    scan = text;
    first_char = *scan;
    if (first_char != 0 && first_char != '\n') {
        for (;;) {
            scan -= 1;
            ch = *scan;
            if (ch == 0 || ch == '\n') {
                break;
            }
        }
    }
    return scan + 1;
}

void textTagRemoveKey();

void textSetOutputCursor(int value) {
    textTagRemoveKey();
    textOutputCursor = (u8 *) (value - 1);
}

/* Copies count bytes from src to dst and writes a terminator just past them. */
void textCopyCounted(u8 *dst, u8 *src, int count) {
    if (count > 0) {
        do {
            *dst = *src;
            src += 1;
            count -= 1;
            dst += 1;
        } while (count > 0);
    }
    *dst = 0;
}

INCLUDE_ASM("asm/main/nonmatchings/tu242", textWordLength);

/* Clears the record, marks it in use (flags 1), sets its state and the init word 0x98808080 (gray, alpha 0x98). */
void msgSlotInit(MsgSlot *record, int state) {
    memset(record, 0, 0x98);
    record->flags = 1;
    record->state = state;
    record->init_word = 0x98808080;
}

/*
 * Stores the slot geometry: origin (x, y) and extent. The callers pass the dispatch
 * position first and the body extent last (main/tu219 msgWindowRunCurrentAt); the width is
 * clamped to at least CLAMP_MINIMUM.
 */
void msgSlotSetGeometry(struct StateObject *object, int x, int y, int extent_x, int extent_y)
{
    object->x = x;
    object->y = y;
    object->extent_x = extent_x;
    if (extent_x < CLAMP_MINIMUM) {
        object->extent_x = CLAMP_MINIMUM;
    }
    object->extent_y = extent_y;
}

INCLUDE_ASM("asm/main/nonmatchings/tu242", textSkipLineBreaksBack);

INCLUDE_ASM("asm/main/nonmatchings/tu242", func_00286CA0);

void sprintf(int first, const char *text, int second);
extern char textHanTagFormat[];

/* Formats the "$han1;" markup tag followed by the name into first. */
void textFormatHanTag(char *dst, const char *src)
{
    sprintf(dst, textHanTagFormat, src);
}

INCLUDE_ASM("asm/main/nonmatchings/tu242", func_00286D40);

INCLUDE_ASM("asm/main/nonmatchings/tu242", func_00286E28);
