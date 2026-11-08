#include "common.h"
#include "shared.h"
#include "main/xgl_font.h"
#include "main/xgl_cd.h"
#include "gs.h"
#include "main/xgl_2.h"

extern void xglFontPrintf(int x, int y, unsigned int color, void *arg);
unsigned int xglFontAscii2Euc(signed char character, signed char **text);
int xglFontGetSPcodeSize(int code, const unsigned char *text);
unsigned short xglFontGetFlags(void);
void xglFontPrintDirect(const char *text);
void xglFontSetFlags(int flags);
extern int xglFontGetStringWidth2(const char *text, int);

#define FONT_STATE_FLAGS (FS.state.flags)

/*
 * Control codes embedded in queued text. Bytes below 32 start a command whose length
 * xglFontGetSPcodeSize returns; the compact position codes 1..5 are the high byte of an
 * X (1..3) or Y (4..5) coordinate.
 */
typedef enum FontOpcode {
    FONT_OP_POS_X_0 = 1,
    FONT_OP_POS_X_1 = 2,
    FONT_OP_POS_X_2 = 3,
    FONT_OP_POS_Y_0 = 4,
    FONT_OP_POS_Y_1 = 5,
    FONT_OP_DEPTH = 6,
    FONT_OP_NOP = 7,
    FONT_OP_POSITION = 8,
    FONT_OP_TAB = 9,
    FONT_OP_NEWLINE = 10,
    FONT_OP_RESET = 11,
    FONT_OP_COLOR = 12,
    FONT_OP_EFFECT_MODE = 13,
    FONT_OP_EFFECT_PARAMS = 14,
    FONT_OP_BLEND = 15,
    FONT_OP_HEX_DIGITS = 16,
    FONT_OP_NUMBER = 17,
    FONT_OP_SCALE = 18,
    FONT_OP_EFFECT_PACKET = 19,
    FONT_OP_CONFIG = 21,
    FONT_OP_INCLUDE = 22,
    FONT_OP_CALLBACK = 23,
    FONT_OP_COMPACT = 24,
    FONT_OP_STYLE = 25,
    FONT_OP_GLYPH = 27
} FontOpcode;

/* FS.state.flags. */
#define FONT_STATE_ENABLED 1
#define FONT_STATE_DEBUG_PRINT 2
/* Size of the print queue buckets and of the draw cursor's scratchpad packet. */
#define FONT_QUEUE_BUCKETS 16
/* Coordinates above these use the extended position command. */
#define FONT_COMPACT_X_LIMIT 768
#define FONT_COMPACT_Y_LIMIT 512
/* FontNumericCommand.radix value that selects a float. */
#define FONT_NUMERIC_FLOAT 0xff
/* FontState.text_mode bits, set by the $prop and $han inline commands. */
#define FONT_MODE_PROP 1
#define FONT_MODE_HAN 2
/* Glyph indices below this have proportional bounds; others advance a fixed FONT_GLYPH_ADVANCE. */
#define FONT_PROPORTIONAL_GLYPHS 0x180
#define FONT_GLYPH_ADVANCE 19
/* FontPositionCommand.flags: which coordinates follow, and whether the line origin stays. */
#define FONT_POS_SET_X 0x01
#define FONT_POS_SET_Y 0x02
#define FONT_POS_SET_DEPTH 0x04
#define FONT_POS_ADD_X 0x08
#define FONT_POS_ADD_Y 0x10
#define FONT_POS_KEEP_ORIGIN 0x80
#define FONT_TAB_WIDTH 64
/* EUC-JP code points and rows. */
#define FONT_JIS_ROW_SIZE 94
#define EUC_FIRST 0xa1
#define EUC_ROW_SYMBOLS 0xa1
#define EUC_ROW_ALNUM 0xa3
#define EUC_ROW_HIRAGANA 0xa4
#define EUC_ROW_GREEK 0xa6
#define EUC_ROW_NEC_SPECIAL 0xad
#define EUC_ROW_SYMBOLS_2 0xa2
#define EUC_ROW_KANJI_LEVEL1 0xb0
#define EUC_ROW_KANJI_LEVEL2 0xd0
#define EUC_SPACE 0xa1a1

struct FontImage {
    unsigned char kanji_texture_0[0x3c020];
    unsigned char kanji_texture_1[0x3c020];
    unsigned short proportional_widths[256];
};

struct FontState {
    struct FontImage *font_image;
    void *window_image;
    unsigned short flags;
    unsigned char font_selection;
    unsigned char debug_mode;
    unsigned short line_height;
    unsigned short character_spacing;
    unsigned char default_width;
    unsigned char text_mode;
    unsigned char proportional_mode;
    unsigned char horizontal_shift;
    int stored_value;
    /* XS1 keeps the queue write cursor at +0x1c; XS2 moved it to the end of
     * the block (FontStateFromWindow.queue_cursor, FS + 0x128). */
    unsigned char unmodeled_18[8];
};

/* The partial state view shares the complete 0x20-byte retail allocation. */
typedef union FontStateStorage {
    struct FontState state;
    unsigned char bytes[0x20];
} FontStateStorage;

/*
 * Each 4-byte slot of the font print-queue buffer at fontPrintQueueBuffer (16 slots,
 * scaffold-owned splat name): link is the byte offset, from this array's
 * base, of the next queued record for this bucket (0 marks the bucket
 * empty -- xglFontFlush follows it as a chain), and selfOffset is the
 * slot's own byte offset within the array.
 */
typedef struct FontQueueSlot {
    unsigned short link;
    unsigned short selfOffset;
} FontQueueSlot;

typedef union FontPrintQueueStorage {
    unsigned long long alignment;
    FontQueueSlot slots[FONT_QUEUE_BUCKETS];
    unsigned char bytes[0x2860];
} FontPrintQueueStorage;

typedef union FontImageStorage {
    struct FontImage image;
    unsigned char bytes[0x78800];
} FontImageStorage;

typedef struct FontState FontState;

struct FontPositionCommand {
    unsigned char opcode;
    unsigned char flags;
    unsigned char x_high;
    unsigned char x_low;
    unsigned char y_high;
    unsigned char y_low;
    unsigned char color_high;
    unsigned char color_middle;
    unsigned char color_low;
};

struct FontCompactPositionCommand {
    unsigned char x_high;
    unsigned char x_low;
    unsigned char y_high;
    unsigned char y_low;
    unsigned char opcode;
    unsigned char color_high;
    unsigned char color_middle;
    unsigned char color_low;
};

struct FontNumericCommand {
    unsigned char opcode;
    unsigned char radix;
    unsigned char digits_minus_one;
    unsigned char value_high;
    unsigned char value_mid_high;
    unsigned char value_mid_low;
    unsigned char value_low;
};

struct KanjiClutEnvironment {
    unsigned char unmodeled_00[0xb0];
    unsigned short code_by_character[32];
};

extern unsigned char spCodeParameterSizeTable[];
extern signed char tbl_7[] ;
void set_xyz(int x, int y, int color);
void xglFontPrintSub(void *args);
void xglFontPrintDirectCore(const char *text);
void set_ot(int ot);
extern int xglFontCheckProportional(unsigned char high, int low);
extern float dptofp(unsigned long long value);
extern unsigned short ascii2euc[96];
extern struct KanjiClutEnvironment KanjiClutEnv;
extern unsigned char spcode_0[32];
extern signed char tbl_7[];
extern FontStateStorage FS;
extern FontPrintQueueStorage fontPrintQueueBuffer;
extern FontImageStorage FontImage;
int xglFontCheckProportional(unsigned char row, int column);
unsigned int hex2val(signed char digit);
int xglFontGetStringWidth2(const char *text, int delimiter);
/* Defined here, not with the data above: moving it before the functions that use it changes the original code or data order. */
extern unsigned char spCodeParameterSizeTable[8];

/*
 * The FontState fields from offset 4 on, read through the scaffold symbol D_008DAD00 (FS + 4),
 * up to the queue write cursor that sits just below the print-queue slot array. The queue
 * functions write their command bytes through queue_cursor; xglFontResetPrintQueue points it
 * at the end of the slot array, where the first record goes.
 */
struct FontStateFromWindow {
    void *window_image;
    unsigned short flags;
    unsigned char font_selection;
    unsigned char debug_mode;
    unsigned short line_height;
    unsigned short character_spacing;
    unsigned char default_width;
    unsigned char text_mode;
    unsigned char proportional_mode;
    unsigned char unmodeled_0f[1];
    int stored_value;
    unsigned char unmodeled_14[8];
    signed char compact_width;
    unsigned char unmodeled_1d[0x107];
    unsigned char *queue_cursor;
};

extern struct FontStateFromWindow D_008DAD00;

/* The state block ends where the print-queue slot array begins. */
#define FONT_STATE_BLOCK() ((struct FontStateFromWindow *) &fontPrintQueueBuffer - 1)

typedef struct XglPacket XglPacket;
typedef struct FontDrawCursor FontDrawCursor;
/* A queued draw callback: called with the draw cursor and the argument
 * registered with it. */
typedef void (*FontExtFunc)(FontDrawCursor *cursor, void *argument);

struct FontExtCommand {
    unsigned char opcode;
    unsigned char function_high;
    unsigned char function_middle;
    unsigned char function_low;
    unsigned char argument_high;
    unsigned char argument_mid_high;
    unsigned char argument_mid_low;
    unsigned char argument_low;
};


/* xglFontPrintExtFunc's queue record: the callback command, then the byte
 * that ends the record. */
struct FontExtRecord {
    struct FontExtCommand command;
    unsigned char terminator;
};

struct FontHexCommand {
    unsigned char opcode;
    unsigned char radix;
    unsigned char digits_minus_one;
    unsigned char value_high;
    unsigned char value_mid_high;
    unsigned char value_mid_low;
    unsigned char value_low;
    unsigned char terminator;
};

struct FontGsRegisterWrite {
    unsigned long long value;
    unsigned long long register_address;
};

/* The two debug-mode GS packets; xglFontDebugMode patches their FRAME value. */
struct FontModePacket {
    unsigned int vif_commands[4];
    unsigned long long gif_tag[2];
    struct FontGsRegisterWrite frame;
    struct FontGsRegisterWrite state_registers[5];
};

extern struct FontModePacket ModeEnv[2];
/* "font0.us" and "font1.us". */
extern char fontTexture0Path[];
extern char fontTexture1Path[];
void xglFontPrintExtFunc(int ot, FontExtFunc function, void *argument);

void xglFontDebugMode(int mode)
{
    if (mode >= 0) {
        struct FontModePacket *packets = ModeEnv;
        int value0;
        unsigned long long value1;

        D_008DAD00.debug_mode = mode;
        value1 = D_008DAD00.debug_mode | 0x50000ULL;
        value0 = 0x50000;
        value0 |= ((unsigned int)D_008DAD00.debug_mode & 0xff) + 0x23;

        packets[0].frame.value = value0;
        packets[1].frame.value = value1;
    }
}

/* Nonzero for the EUC-JP codes drawn from the font texture's own glyph rows: the whole hiragana,
 * Greek and NEC special rows, and a few symbols of rows 1 and 2. */
static int xglFontIsTextureGlyphCode(unsigned char high, unsigned char low)
{
    if (low == 0) {
        return 0;
    }
    if (high == EUC_ROW_HIRAGANA || high == EUC_ROW_GREEK || high == EUC_ROW_NEC_SPECIAL) {
        return 1;
    }
    if (high == EUC_ROW_SYMBOLS && (low == 0xfb || low == 0xbd || low == 0xda || low == 0xdb || low == 0xdf ||
                         low == 0xf9 || low == 0xfa)) {
        return 1;
    }
    if (high == EUC_ROW_SYMBOLS_2 && ((unsigned char) (low + 0x5e) < 3 || low == 0xf6)) {
        return 1;
    }
    return 0;
}

void *func_0012B908(int, int);

/*
 * Allocates a packet and writes a GIF tag: NLOOP = count with the EOP bit 0x8000, one register
 * (NREG = 1 in the upper word) and the register list 0xE (A+D). Returns the address 16 bytes in.
 */
void *xglFontAllocGifPacket(int *heap, int count)
{
    unsigned int *packet;

    packet = func_0012B908(*heap, count * 4 + 4);
    packet[0] = count + 0x8000;
    packet[1] = 0x10000000;
    packet[2] = 0xE;
    return packet + 4;
}

int xglFontGetKanjiClutUV(int code, int *clut)
{
    unsigned short index = code;
    unsigned char column = index >> 8;
    unsigned char row = index;
    int first_range_or_texture_column;
    int second_range;

    first_range_or_texture_column = row < 0xad;
    second_range = row < EUC_ROW_KANJI_LEVEL1;
    index = (column + row * FONT_JIS_ROW_SIZE) - EUC_FIRST;
    if (first_range_or_texture_column) {
        index += 0xc4e2;
    } else if (second_range) {
        index += 0xc30c;
    } else {
        index += 0xc250;
    }
    column = index & 0x1f;
    row = index >> 5;
    first_range_or_texture_column = column;
    if (clut != 0) {
        *clut = ((row >> 5) << 12) + 0x1004;
    }
    return ((row & 0x1f) * 0x01800000) +
        (first_range_or_texture_column * 0x140);
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", func_00194FA8);

void set_ot(int ot)
{
    struct FontStateFromWindow *state = FONT_STATE_BLOCK();
    FontQueueSlot *slot = &fontPrintQueueBuffer.slots[((ot >> 22) & 0x3c) / 4];
    unsigned char *queue_bytes = (unsigned char *)&fontPrintQueueBuffer;
    unsigned short offset = state->queue_cursor - queue_bytes;
    unsigned char *previous = queue_bytes + slot->selfOffset;

    previous[0] = offset;
    previous[1] = offset >> 8;
    slot->selfOffset = offset;
    state->queue_cursor[0] = 0;
    state->queue_cursor[1] = 0;
    state->queue_cursor += 2;
}

void set_xyz(int x, int y, int color)
{
    set_ot(color);
    if ((unsigned int)x >= FONT_COMPACT_X_LIMIT || y < 0 || y >= FONT_COMPACT_Y_LIMIT) {
        struct FontPositionCommand *command = (struct FontPositionCommand *)D_008DAD00.queue_cursor;

        command->opcode = FONT_OP_POSITION;
        command->flags = 7;
        command->x_high = x >> 8;
        command->x_low = x;
        command->y_high = y >> 8;
        command->y_low = y;
        command->color_high = color >> 16;
        command->color_middle = color >> 8;
        command->color_low = color;
        D_008DAD00.queue_cursor += sizeof(*command);
    } else {
        struct FontCompactPositionCommand *command = (struct FontCompactPositionCommand *)D_008DAD00.queue_cursor;

        command->x_high = (x >> 8) + FONT_OP_POS_X_0;
        command->x_low = x;
        command->y_high = (y >> 8) + FONT_OP_POS_Y_0;
        command->y_low = y;
        command->opcode = FONT_OP_DEPTH;
        command->color_high = color >> 16;
        command->color_middle = color >> 8;
        command->color_low = color;
        D_008DAD00.queue_cursor += sizeof(*command);
    }
}

int xglFontGetSPcodeSize(int code, const unsigned char *text)
{
    unsigned char size;

    code &= 0xff;
    size = spcode_0[code];

    if (size == 255) {
        switch (code) {
        case FONT_OP_POSITION: {
            unsigned char parameter_sizes[8];
            unsigned char flags = text[1];
            int bit;

            __builtin_memcpy(parameter_sizes, spCodeParameterSizeTable,
                             sizeof(parameter_sizes));
            size = 1;
            for (bit = 0; bit < 8; bit++) {
                if (flags & 1) {
                    size += parameter_sizes[bit];
                }
                flags >>= 1;
            }
            break;
        }
        case FONT_OP_CONFIG:
            size = text[1] + 1;
            break;
        }
    }

    return size;
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", xglFontPrintSub);

void xglFontPrintDirectCore(const char *text)
{
    while (*text != 0) {
        unsigned char code = *text;

        if (code < 32) {
            int remaining = xglFontGetSPcodeSize(code, (const unsigned char *)text);

            while (remaining > 0) {
                *D_008DAD00.queue_cursor++ = *text++;
                remaining--;
            }
        }
        *D_008DAD00.queue_cursor++ = *text++;
    }
    *D_008DAD00.queue_cursor++ = 0;
}

void xglFontPrintf(int x, int y, unsigned int color, void *arg)
{
    if (FONT_STATE_FLAGS & FONT_STATE_ENABLED & 0xFFFF) {
        set_xyz(x, y, color);
        xglFontPrintSub(arg);
    }
}

void xglFontPrint(int x, int y, unsigned int color, const char *text)
{
    if (FONT_STATE_FLAGS & FONT_STATE_ENABLED & 0xFFFF) {
        set_xyz(x, y, color);
        xglFontPrintDirectCore(text);
    }
}

void xglFontPrintDirect(const char *text)
{
    xglFontPrintDirectOT(0, text);
}

void xglFontPrintDirectOT(int ot, const char *text)
{
    if (FONT_STATE_FLAGS & FONT_STATE_ENABLED & 0xFFFF) {
        set_ot(ot);
        xglFontPrintDirectCore(text);
    }
}

void xglFontPrintExtFunc(int ot, FontExtFunc function, void *argument)
{
    struct FontStateFromWindow *state = &D_008DAD00;
    struct FontExtRecord *record;
    unsigned int function_address = (unsigned int)function;
    unsigned int argument_word = (unsigned int)argument;

    if (state->flags & FONT_STATE_ENABLED & 0xffff) {
        set_ot(ot);
        record = (struct FontExtRecord *)D_008DAD00.queue_cursor;
        record->command.opcode = FONT_OP_CALLBACK;
        record->command.function_high = function_address >> 16;
        record->command.function_middle = function_address >> 8;
        record->command.function_low = function_address;
        record->command.argument_high = argument_word >> 24;
        record->command.argument_mid_high = argument_word >> 16;
        record->command.argument_mid_low = argument_word >> 8;
        record->command.argument_low = argument_word;
        record->terminator = 0;
        D_008DAD00.queue_cursor += sizeof(*record);
    }
}

/* Stores a value in the font state's stored_value field. */
void xglFontSetStoredValue(int stored_value) {
    FS.state.stored_value = stored_value;
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", xglFontDebugPrintf);

void xglFontDebugHex(int x, int y, unsigned int value, int digits)
{
    struct FontStateFromWindow *state = &D_008DAD00;
    struct FontHexCommand *command;

    if ((state->flags & (FONT_STATE_ENABLED | FONT_STATE_DEBUG_PRINT))
            == (FONT_STATE_ENABLED | FONT_STATE_DEBUG_PRINT)
        && digits != 0) {
        if (state->debug_mode == 0) {
            x <<= 1;
            y <<= 1;
        }
        set_xyz(x, y, -1);
        command = (struct FontHexCommand *)D_008DAD00.queue_cursor;
        command->opcode = FONT_OP_RESET;
        command->radix = FONT_OP_HEX_DIGITS;
        command->digits_minus_one = digits - 1;
        command->value_high = value >> 24;
        command->value_mid_high = value >> 16;
        command->value_mid_low = value >> 8;
        command->value_low = value;
        command->terminator = 0;
        D_008DAD00.queue_cursor += sizeof(*command);
    }
}


/* Advance of each printable ASCII character (32..127) in the debug font. */
extern int D_0063E878[];

int func_00196260(const char **cursor, int *value);
unsigned short xglFontCharToEuc(signed char character, unsigned char **text);
int func_00198928(int high, int low);
int func_001979E0(int high, int low);

int xglFontGetStringWidth2(const char *text, int delimiter)
{
    int width = 0;
    int compact = 0;
    int first = 1;
    unsigned char high;

    while (*text != 0 && (high = *text, !(!first && delimiter != 0 && high == ' '))) {
        unsigned char low;

        if (high == '$') {
            if (text[1] == 'p' && text[2] == 'r' && text[3] == 'o' && text[4] == 'p') {
                D_008DAD00.text_mode &= ~FONT_MODE_PROP;
                D_008DAD00.text_mode |= text[5] & FONT_MODE_PROP;
            } else if (text[1] == 'h' && text[2] == 'a' && text[3] == 'n') {
                D_008DAD00.text_mode &= ~FONT_MODE_HAN;
                D_008DAD00.text_mode |= (text[4] & 1) << 1;
            } else if (text[1] == 'x' && text[2] == 'x') {
                if (text[3] == '1' && text[4] == ',') {
                    const char *number = text + 5;
                    int value;

                    func_00196260(&number, &value);
                    D_008DAD00.default_width = value;
                }
                if (text[3] == '3' && text[4] == ',') {
                    const char *number = text + 5;
                    int value;

                    func_00196260(&number, &value);
                    D_008DAD00.proportional_mode = value;
                }
            } else if (text[1] == 'r' && text[2] == 'u' && text[3] == 'b' && text[4] == 'y' &&
                       text[5] == 's' && text[6] == ';') {
                const unsigned char *scan = (const unsigned char *) text + 7;

                while (*scan != 0) {
                    if (scan[0] == '$' && scan[1] == 'r' && scan[2] == 'u' && scan[3] == 'b' &&
                        scan[4] == 'y' && scan[5] == 'e' && scan[6] == ';') {
                        text = (const char *) scan;
                        break;
                    }
                    if (*scan < 0x20) {
                        scan += xglFontGetSPcodeSize(*scan, scan + 1);
                    }
                    scan++;
                }
            }
            do {
                high = *text++;
            } while (high != ';');
        } else if (high < 0x20) {
            switch (high) {
            case FONT_OP_CONFIG:
                switch (text[2]) {
                case 2:
                    D_008DAD00.default_width = text[3];
                    break;
                case 4:
                    D_008DAD00.proportional_mode = text[3];
                    break;
                }
                break;
            case FONT_OP_COMPACT:
                compact = text[1];
                break;
            case FONT_OP_STYLE:
                D_008DAD00.text_mode = text[1];
                break;
            }
            text += xglFontGetSPcodeSize(high, (const unsigned char *) text) + 1;
        } else {
            if (high < 0xa0) {
                if ((signed char) high >= 0) {
                    if (!(D_008DAD00.text_mode & FONT_MODE_HAN)) {
                        width += D_0063E878[high - 32];
                        text++;
                        first = 0;
                        continue;
                    }
                }
            }
            if ((signed char) high >= 0) {
                unsigned short code = xglFontCharToEuc(high, (unsigned char **) &text);
                low = code;
                high = code >> 8;
                text++;
            } else {
                low = text[1];
                if (xglFontIsTextureGlyphCode(high, low)) {
                    text += 2;
                } else {
                    text++;
                    low = high + 0x80;
                    high = 0xa9;
                }
            }
            if (compact) {
                if (func_00198928(high, low) & 0x20000000) {
                    width += (D_008DAD00.compact_width * 5) >> 4;
                }
                width += (D_008DAD00.compact_width * 8) >> 4;
            } else {
                int index = func_001979E0(high, low);
                if ((D_008DAD00.text_mode & FONT_MODE_PROP) && index < FONT_PROPORTIONAL_GLYPHS) {
                    unsigned int bounds = xglFontGetProportionalSize(index);
                    width += ((bounds >> 8) & 0xff) - (bounds & 0xff) + 1;
                } else {
                    width += FONT_GLYPH_ADVANCE;
                }
            }
        }
        first = 0;
    }
    return width;
}

int xglFontGetStringWidth(const char *text)
{
    return xglFontGetStringWidth2(text, 0);
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", func_00195E90);

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", func_00196118);

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", func_001961D0);

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", func_00196260);

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", func_00196398);

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", func_00196460);

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", cfe_00196508);

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", col);

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", func_00196660);

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", shadow);

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", alpha);

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", zoom);

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", func_00196958);

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", ruby);

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", han);

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", mark);

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", func_001970E0);

extern FontPrintQueueStorage fontPrintQueueBuffer;


void xglFontResetPrintQueue(void)
{
    struct FontStateFromWindow *block;
    FontQueueSlot *slot;
    int index;

    slot = fontPrintQueueBuffer.slots;
    for (index = 0; index < FONT_QUEUE_BUCKETS; index++) {
        slot->selfOffset = (unsigned short) ((unsigned char *) slot - (unsigned char *) fontPrintQueueBuffer.slots);
        slot->link = 0;
        slot++;
    }
    /* The block ends where the slot array begins; it is the state starting at D_008DAD00. */
    block = (struct FontStateFromWindow *) &fontPrintQueueBuffer - 1;
    block->stored_value = 0;
    block->text_mode = 0;
    block->queue_cursor = (unsigned char *) slot;
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", func_00197200);

void func_00197200(int u, int v, int size, int flags);

/* Draws one hex digit (0..15) through func_00197200, at the glyph offset digit << 7. */
void xglFontDrawHexDigit(int digit)
{
    func_00197200(digit << 7, 0x2F00, 0x608, 0x10);
}

/*
 * EE scratchpad at 0x70000000: halfword slot 2 keeps the start x of the line, slot 4 is set back
 * to it and slot 5 (y) moves down by one line height.
 */
void xglFontNewLine(void)
{
    unsigned short *scratchpad = (unsigned short *) 0x70000000;

    scratchpad[4] = scratchpad[2];
    scratchpad[5] += FS.state.line_height;
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", func_00197988);

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", func_001979E0);

/* Packet that FONT_OP_EFFECT_PACKET builds in the cursor's work area: an A+D GIF tag and one
 * register write. */
struct FontEffectScratchPacket {
    u64 vif_tag;
    u64 vif_count;
    u64 effect_state;
    u64 payload_count;
};

/* The font renderer's draw cursor, built by xglFontFlush at the scratchpad base. */
struct FontDrawCursor {
    XglPacket *packet;
    short origin_x;
    unsigned char unmodeled_06[2];
    unsigned short x;
    unsigned short y;
    int depth;
    unsigned short offset_x;
    unsigned short offset_y;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char unmodeled_17;
    unsigned char flags;
    unsigned char effect;
    unsigned char attributes;
    unsigned char draw_state_high;
    unsigned char unmodeled_1c[4];
    signed char effect_x;
    signed char effect_y;
    unsigned char unmodeled_22[2];
    unsigned char effect_red;
    unsigned char effect_green;
    unsigned char effect_blue;
    unsigned char unmodeled_27;
    unsigned char primitive_flags;
    unsigned char primitive_mode;
    unsigned char alpha;
    unsigned char unmodeled_2b;
    unsigned short scale_x;
    unsigned short scale_y;
    union {
        struct FontEffectScratchPacket effect_packet;
        u64 qwords[(0x4000 - 0x30) / sizeof(u64)];
    } work;
};

/* FONT_OP_EFFECT_MODE: how a glyph is drawn (cursor->effect). */
#define FONT_EFFECT_PLAIN 1
#define FONT_EFFECT_SHADOW 2
#define FONT_EFFECT_OUTLINE 3
#define FONT_EFFECT_BOLD 4
/* FONT_OP_CONFIG sub-commands. */
#define FONT_CONFIG_LINE_HEIGHT 1
#define FONT_CONFIG_DEFAULT_WIDTH 2
#define FONT_CONFIG_WRAP_WIDTH 3
#define FONT_CONFIG_PROPORTIONAL_MODE 4
#define FONT_CONFIG_HORIZONTAL_SHIFT 5
/* FONT_OP_STYLE argument bits, and the cursor flags they set. */
#define FONT_STYLE_PROPORTIONAL 1
#define FONT_STYLE_WIDE_ASCII 2
#define FONT_CURSOR_COMPACT 1
#define FONT_CURSOR_PROPORTIONAL 2
#define FONT_CURSOR_WIDE_ASCII 4
#define FONT_CURSOR_STYLE_CLEAR \
    (~(FONT_CURSOR_COMPACT | FONT_CURSOR_PROPORTIONAL | FONT_CURSOR_WIDE_ASCII) & 0xff)
/* The compact position codes store x + 0x100 and y + 0x400 as 16-bit values. */
#define FONT_POS_X_BIAS 0x100
#define FONT_POS_Y_BIAS 0x400
/* Texture coordinate / size / draw-flag arguments of func_00197200. */
#define FONT_ASCII_V_LOW 0x2f00
#define FONT_ASCII_V_HIGH 0x2f80
#define FONT_SYMBOL_V 0x2e80
#define FONT_GLYPH_8X8 0x808
#define FONT_DRAW_ASCII 0x10
#define FONT_DRAW_COMPACT 0x20
/* Hex digit glyphs 0..15, then the minus sign and decimal point; 32 is blank. */
#define FONT_HEX_MINUS 16
#define FONT_HEX_POINT 17
#define FONT_HEX_BLANK 32
#define FONT_SCALE_ONE 16
#define FONT_COLOR_NEUTRAL 0x80
#define FONT_ALPHA_NEUTRAL 0x80
/* IEEE single precision fields used by the %f printer. */
#define FONT_FLOAT_MANTISSA_MASK 0x7fffff
#define FONT_FLOAT_HIDDEN_BIT 0x800000
#define FONT_FLOAT_EXPONENT_SHIFT_BASE 150 /* bias 127 + 23 mantissa bits */

struct FontLiteralHexCommand {
    unsigned char opcode;
    unsigned char digit_index;
    unsigned char value_high;
    unsigned char value_mid_high;
    unsigned char value_mid_low;
    unsigned char value_low;
};

struct FontIncludeCommand {
    unsigned char opcode;
    unsigned char text_high;
    unsigned char text_mid_high;
    unsigned char text_mid_low;
    unsigned char text_low;
};

/* One A+D register write after the GIF tag xglFontAllocGifPacket writes. */
struct FontAlphaWrite {
    unsigned int primitive_flags;
    unsigned int primitive_mode;
    unsigned int register_address;
    unsigned int register_address_high;
};

struct FontRealDigits {
    signed char value[16];
};

/* GS state packets: the full reset, the two effect modes. */
extern unsigned int D_0063EB40[16];
extern unsigned int D_0063EB80[8];
extern unsigned int D_0063EBA0[8];

void sceVif1PkAddDirectDataN(XglPacket *packet, const void *data, int qwc);
int func_001970E0(FontDrawCursor *cursor, unsigned char **text);
void func_00194FA8(FontDrawCursor *cursor, int mode);

void xglFontDrawHexDigit(int digit);
void xglFontNewLine(void);

void xglFontFlushCore(FontDrawCursor *cursor, unsigned char *text)
{
    struct FontRealDigits real_digits;
    signed char integer_digits[32];

    while (*text != 0) {
        unsigned char opcode;

        if (*text == '$' && func_001970E0(cursor, &text) != 0) {
            continue;
        }
        opcode = *text;
        switch (opcode) {
        case FONT_OP_POS_X_0: case FONT_OP_POS_X_1: case FONT_OP_POS_X_2: {
            unsigned int position_high = text[0];
            unsigned int position_low = text[1];
            int position;
            text += 2;
            position = position_low + (position_high << 8) - FONT_POS_X_BIAS;
            cursor->x = position;
            cursor->origin_x = position;
            break;
        }
        case FONT_OP_POS_Y_0: case FONT_OP_POS_Y_1: {
            unsigned int position_high = text[0];
            unsigned int position_low = text[1];
            int position;
            text += 2;
            position = position_low + (position_high << 8) - FONT_POS_Y_BIAS;
            cursor->y = position;
            break;
        }
        case FONT_OP_DEPTH: {
            int depth = (text[1] << 16) + (text[2] << 8) + text[3];
            text += 4;
            cursor->depth = depth;
            break;
        }
        case FONT_OP_NOP:
            text++;
            break;
        case FONT_OP_POSITION: {
            unsigned char flags = text[1];
            text += 2;
            if (flags & FONT_POS_SET_X) {
                cursor->x = text[1] + (text[0] << 8);
                if (!(flags & FONT_POS_KEEP_ORIGIN)) cursor->origin_x = cursor->x;
                text += 2;
            }
            if (flags & FONT_POS_SET_Y) {
                cursor->y = text[1] + (text[0] << 8);
                text += 2;
            }
            if (flags & FONT_POS_SET_DEPTH) {
                cursor->depth = (text[0] << 16) + (text[1] << 8) + text[2];
                text += 3;
            }
            if (flags & FONT_POS_ADD_X) {
                cursor->x += text[1] + (text[0] << 8);
                if (!(flags & FONT_POS_KEEP_ORIGIN)) cursor->origin_x = cursor->x;
                text += 2;
            }
            if (flags & FONT_POS_ADD_Y) {
                cursor->y += text[1] + (text[0] << 8);
                text += 2;
            }
            break;
        }
        case FONT_OP_TAB:
            cursor->x = (cursor->x + FONT_TAB_WIDTH) & ~(FONT_TAB_WIDTH - 1) & 0xffff;
            text++;
            break;
        case FONT_OP_NEWLINE:
            xglFontNewLine();
            text++;
            break;
        case FONT_OP_RESET:
            cursor->effect = FONT_EFFECT_PLAIN;
            cursor->red = cursor->green = cursor->blue = FONT_COLOR_NEUTRAL;
            cursor->primitive_flags = 0;
            cursor->scale_x = FONT_SCALE_ONE;
            cursor->alpha = FONT_ALPHA_NEUTRAL;
            cursor->scale_y = FONT_SCALE_ONE;
            cursor->flags &= FONT_CURSOR_STYLE_CLEAR;
            if (!FS.state.debug_mode) sceVif1PkAddDirectDataN(cursor->packet, D_0063EB40, 4);
            text++;
            break;
        case FONT_OP_COLOR:
            cursor->red = text[1]; cursor->green = text[2]; cursor->blue = text[3];
            text += 4;
            break;
        case FONT_OP_EFFECT_MODE: {
            cursor->effect = text[1];
            switch (cursor->effect) {
            case 0:
            case FONT_EFFECT_SHADOW:
            case FONT_EFFECT_OUTLINE:
            case FONT_EFFECT_BOLD:
                sceVif1PkAddDirectDataN(cursor->packet, D_0063EB80, 2);
                break;
            case FONT_EFFECT_PLAIN:
            default:
                sceVif1PkAddDirectDataN(cursor->packet, D_0063EBA0, 2);
                break;
            }
            text += 2;
            break;
        }
        case FONT_OP_EFFECT_PARAMS:
            cursor->effect_x = text[1]; cursor->effect_y = text[2];
            cursor->effect_red = text[3]; cursor->effect_green = text[4]; cursor->effect_blue = text[5];
            text += 6;
            break;
        case FONT_OP_BLEND: {
            struct FontAlphaWrite *packet;
            cursor->primitive_flags = text[1]; cursor->primitive_mode = text[2]; cursor->alpha = text[3];
            packet = xglFontAllocGifPacket((int *) cursor, 1);
            packet->primitive_flags = cursor->primitive_flags;
            packet->primitive_mode = cursor->primitive_mode;
            packet->register_address = GS_ALPHA_1;
            text += 4;
            break;
        }
        case FONT_OP_HEX_DIGITS: {
            const struct FontLiteralHexCommand *command =
                (const struct FontLiteralHexCommand *) text;
            unsigned int value = ((unsigned int) command->value_high << 24) +
                ((unsigned int) command->value_mid_high << 16) +
                ((unsigned int) command->value_mid_low << 8) + command->value_low;
            int digit = command->digit_index;
            while (digit >= 0) {
                xglFontDrawHexDigit((value >> (digit * 4)) & 15);
                digit--;
            }
            text += sizeof(*command);
            break;
        }
        case FONT_OP_NUMBER: {
            const struct FontNumericCommand *command =
                (const struct FontNumericCommand *) text;
            if (command->radix == FONT_NUMERIC_FLOAT) {
                unsigned int mid_high = (unsigned int) command->value_mid_high << 16;
                unsigned int high = (unsigned int) command->value_high << 24;
                unsigned int mid = ((unsigned int) command->value_mid_low << 8) + mid_high;
                unsigned int bits = high + command->value_low + mid;
                unsigned int mantissa = (bits & FONT_FLOAT_MANTISSA_MASK) + FONT_FLOAT_HIDDEN_BIT;
                unsigned int shift = FONT_FLOAT_EXPONENT_SHIFT_BASE - ((bits >> 23) & 255);
                long long integer;
                unsigned long long mask;
                unsigned long long fraction;
                int digit;
                int printed;
                int remaining;

                if ((int) bits < 0)
                    xglFontDrawHexDigit(FONT_HEX_MINUS);
                else
                    xglFontDrawHexDigit(FONT_HEX_BLANK);
                if (shift >= 32) {
                    integer = 0;
                    mask = (0x100000000ULL << (shift - 32)) - 1;
                } else {
                    integer = mantissa >> shift;
                    mask = (int) ((1U << shift) - 1);
                }
                fraction = mantissa & mask;
                for (digit = 7; digit >= 0; digit--) {
                    real_digits.value[digit] = integer % 10;
                    integer /= 10;
                    fraction *= 10;
                    if (shift >= 32)
                        real_digits.value[digit + 8] = ((long long) fraction >> 32) >> (shift - 32);
                    else
                        real_digits.value[digit + 8] = (long long) fraction >> shift;
                    fraction &= mask;
                }
                printed = 0;
                for (digit = 0; digit < 7; digit++) {
                    if (real_digits.value[digit] || printed) {
                        printed++;
                        xglFontDrawHexDigit(real_digits.value[digit]);
                    }
                }
                xglFontDrawHexDigit(real_digits.value[7]);
                xglFontDrawHexDigit(FONT_HEX_POINT);
                printed += 2;
                remaining = 7;
                for (; remaining >= 0 && printed < 9; remaining--) {
                    printed++;
                    xglFontDrawHexDigit(real_digits.value[remaining + 8]);
                }
            } else {
                int suppress_zero = 0;
                int radix = command->radix;
                int digit = command->digits_minus_one;
                unsigned int value = ((unsigned int) command->value_high << 24) +
                    ((unsigned int) command->value_mid_high << 16) +
                    ((unsigned int) command->value_mid_low << 8) + command->value_low;
                if (radix == 10) {
                    suppress_zero = 1;
                    if ((int) value < 0) { value = -value; suppress_zero = 2; }
                }
                integer_digits[digit + 1] = -1;
                while (digit >= 0) {
                    integer_digits[digit] = value % radix;
                    value /= radix;
                    digit--;
                }
                for (digit = 0; integer_digits[digit] >= 0; digit++) {
                    if (suppress_zero && !integer_digits[digit] && integer_digits[digit + 1] >= 0) {
                        if (suppress_zero == 2 && integer_digits[digit + 1] > 0) xglFontDrawHexDigit(FONT_HEX_MINUS);
                        else xglFontDrawHexDigit(FONT_HEX_BLANK);
                    } else {
                        xglFontDrawHexDigit(integer_digits[digit]);
                        suppress_zero = 0;
                    }
                }
            }
            text += sizeof(*command);
            break;
        }
        case FONT_OP_SCALE:
            cursor->scale_x = text[1]; cursor->scale_y = text[2];
            func_00194FA8(cursor, 1);
            text += 3;
            break;
        case FONT_OP_EFFECT_PACKET: {
            struct FontEffectScratchPacket *packet = (struct FontEffectScratchPacket *)
                (((unsigned int) &cursor->work.effect_packet + 15) & ~15);
            long long horizontal_mode;
            long long horizontal_mask;
            long long vertical_mask;
            long long vertical_mode;
            long long horizontal_random;
            long long vertical_random;
            packet->vif_tag = 0x1000000000000000ULL | 0x8000 | 1ULL;
            packet->vif_count = GIF_AD;
            if (!text[1]) {
                horizontal_mode = 1; horizontal_mask = horizontal_random = 0;
            } else {
                unsigned long long mask_byte = text[1];
                horizontal_mode = 3; horizontal_mask = mask_byte ^ 0x3ff;
                if (mask_byte & 0x80) {
                    unsigned short random = xglSRand();
                    horizontal_random = text[1] & random;
                } else {
                    horizontal_random = 0;
                }
            }
            if (!text[2]) {
                vertical_mode = 1; vertical_mask = vertical_random = 0;
            } else {
                vertical_mode = 3; vertical_mask = text[1] ^ 0x3ff;
                if (text[2] & 0x80) {
                    unsigned short random = xglSRand();
                    vertical_random = text[1] & random;
                } else {
                    vertical_random = 0;
                }
            }
            packet->payload_count = 8;
            packet->effect_state = horizontal_mode | (vertical_mode << 2) | (horizontal_mask << 4) |
                (horizontal_random << 14) | (vertical_mask << 24) | (vertical_random << 34);
            sceVif1PkAddDirectDataN(cursor->packet, packet, 2);
            text += 3;
            break;
        }
        case FONT_OP_CONFIG: {
            const unsigned char *payload = text + 2;
            int length = text[1];
            text += length + 2;
            switch (payload[0]) {
            case FONT_CONFIG_LINE_HEIGHT: if (length == 2) FS.state.line_height = payload[1]; break;
            case FONT_CONFIG_DEFAULT_WIDTH: if (length == 2) FS.state.default_width = payload[1]; break;
            case FONT_CONFIG_WRAP_WIDTH: if (length == 3) FS.state.character_spacing = (payload[1] << 8) + payload[2]; break;
            case FONT_CONFIG_PROPORTIONAL_MODE: if (length == 2) FS.state.proportional_mode = payload[1]; break;
            case FONT_CONFIG_HORIZONTAL_SHIFT: if (length == 2) FS.state.horizontal_shift = payload[1]; break;
            }
            break;
        }
        case FONT_OP_INCLUDE: {
            const struct FontIncludeCommand *command =
                (const struct FontIncludeCommand *) text;
            unsigned char *included = (unsigned char *) (((unsigned int) command->text_high << 24) +
                ((unsigned int) command->text_mid_high << 16) +
                ((unsigned int) command->text_mid_low << 8) + command->text_low);
            text += sizeof(*command);
            xglFontFlushCore(cursor, included);
            break;
        }
        case FONT_OP_CALLBACK: {
            const struct FontExtCommand *command =
                (const struct FontExtCommand *) text;
            FontExtFunc callback = (FontExtFunc)
                (((unsigned int) command->function_high << 16) +
                 ((unsigned int) command->function_middle << 8) +
                 command->function_low);
            void *argument = (void *) (((unsigned int) command->argument_high << 24) +
                ((unsigned int) command->argument_mid_high << 16) +
                ((unsigned int) command->argument_mid_low << 8) +
                command->argument_low);
            text += sizeof(*command);
            callback(cursor, argument);
            break;
        }
        case FONT_OP_COMPACT:
            if (text[1]) {
                func_00194FA8(cursor, 0);
                cursor->flags |= FONT_CURSOR_COMPACT;
            } else {
                func_00194FA8(cursor, 1);
                cursor->flags &= ~FONT_CURSOR_COMPACT;
            }
            text += 2;
            break;
        case FONT_OP_STYLE: {
            unsigned char bits = text[1];
            unsigned char flags = cursor->flags & (~(FONT_CURSOR_PROPORTIONAL | FONT_CURSOR_WIDE_ASCII) & 0xff);
            text += 2;
            if (bits & FONT_STYLE_PROPORTIONAL) flags |= FONT_CURSOR_PROPORTIONAL;
            if (bits & FONT_STYLE_WIDE_ASCII) flags |= FONT_CURSOR_WIDE_ASCII;
            cursor->flags = flags;
            break;
        }
        case FONT_OP_GLYPH: {
            signed char character = text[1];
            if (character < 64) func_00197200((character & 63) << 7, FONT_ASCII_V_LOW, FONT_GLYPH_8X8, FONT_DRAW_ASCII);
            else func_00197200((character & 63) << 7, FONT_ASCII_V_HIGH, FONT_GLYPH_8X8, FONT_DRAW_ASCII);
            text += 2;
            break;
        }
        case 20: case 26:
        case 28: case 29: case 30: case 31:
            break;
        default: {
            struct FontStateFromWindow *state;
            unsigned char high = opcode;
            unsigned char low;
            unsigned int code;

            if ((signed char) high >= 0) {
                if (cursor->flags & FONT_CURSOR_WIDE_ASCII) {
                    code = xglFontCharToEuc(high, &text);
                    high = code >> 8;
                    low = code;
                } else {
                    code = high;
                    low = 0;
                }
                text++;
            } else {
                low = text[1];
                if (xglFontIsTextureGlyphCode(high, low)) {
                    text += 2;
                    code = ((high << 8) + low) & 0xffff;
                } else {
                    low = high + 0x80;
                    high = 0xa9;
                    text++;
                    code = ((high << 8) + low) & 0xffff;
                }
            }
            state = &D_008DAD00;
            if ((short) state->character_spacing > 0 && opcode == ' ' &&
                (short) cursor->x - cursor->origin_x + xglFontGetStringWidth2((const char *) text, 1) >
                    (short) state->character_spacing) {
                xglFontNewLine();
                text++;
            } else if (code < 0x40) {
                func_00197200((opcode & 63) << 7, FONT_ASCII_V_LOW, FONT_GLYPH_8X8, FONT_DRAW_ASCII);
            } else if (code < 0x80) {
                func_00197200((opcode & 63) << 7, FONT_ASCII_V_HIGH, FONT_GLYPH_8X8, FONT_DRAW_ASCII);
            } else if (code < 0xa0) {
                func_00197200(0x1f80, FONT_ASCII_V_HIGH, FONT_GLYPH_8X8, FONT_DRAW_ASCII);
            } else if (cursor->flags & FONT_CURSOR_COMPACT) {
                unsigned int glyph = func_00198928(high, low);
                func_00197200((glyph >> 16) & 0x1f80, glyph & 0xffff, FONT_GLYPH_8X8, FONT_DRAW_COMPACT);
                if (glyph & 0x20000000) {
                    func_00197200((((int) glyph >> 30) + 0x3e) << 7, FONT_SYMBOL_V, 0x508, FONT_DRAW_COMPACT);
                }
            } else {
                code = func_001979E0(high, low);
                if ((cursor->flags & FONT_CURSOR_PROPORTIONAL) && code < 0x180) {
                    unsigned int bounds = xglFontGetProportionalSize(code);
                    unsigned short row = code >> 5;
                    unsigned int left = bounds & 255;
                    unsigned int right = (bounds >> 8) & 255;
                    func_00197200(((code & 31) * 20 + left) * 16, row * 384 + 0x1b00,
                        (right - left) * 256 + 0x118, 0);
                } else {
                    low = code & 31;
                    high = code >> 5;
                    func_00197200(low * 320, (high & 31) * 384, 0x1318, high >> 5);
                }
            }
            break;
        }
        }
    }
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", func_00198928);

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", func_00198AE0);

/* Advance of character code in the window's proportional widths; the default width when the low byte is 0xFF. */
int xglFontGetProportionalSize(int code)
{
    struct FontStateFromWindow *state;
    struct FontImage *image;
    int width;

    state = &D_008DAD00;
    width = 0xF06;
    if (state->proportional_mode == 0) {
        width = 0xFFFF;
        image = (struct FontImage *) state->window_image;
        width = (image->proportional_widths[code] + 0xFF) & width;
        if ((width & 0xFF) == 0xFF) {
            width = state->default_width << 8;
        }
    }
    return width;
}

/* ASCII character to EUC-JP code, indexed by the raw character; a control character becomes a full-width space. */
extern unsigned short asciiEucTable[];

unsigned short xglFontCharToEuc(signed char character, unsigned char **text)
{
    if (character < 0x20) {
        return EUC_SPACE;
    }
    return asciiEucTable[character];
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", xglFontReloadTexture);

/* EE scratchpad RAM, where xglFontFlush builds the draw cursor. */
#define SCRATCHPAD_BASE 0x70000000
/* xglFontFlush's cursor defaults: screen origin, colour and alpha. */
#define FONT_SCREEN_ORIGIN_X 0x700
#define FONT_SCREEN_ORIGIN_Y 0x720
#define FONT_COLOR_INITIAL 127
#define FONT_ALPHA_SOURCE_BLEND 0x44
#define GS_ALPHA_OPAQUE 0x80

/* A queued record: the byte offset of the next record of its bucket, then
 * the text. */
struct FontQueueRecord {
    unsigned char next_low;
    unsigned char next_high;
};

extern XglPacket *xglPacketGetCurrent(void);
extern void sceVif1PkCloseDirectHLCode(XglPacket *packet);
void xglFontReloadTexture(FontDrawCursor *cursor, unsigned int request);
void xglFontResetPrintQueue(void);
/* The GS packets xglFontFlush closes with (6 quadwords in debug mode, then 2). */
extern unsigned int D_0063ECD0[24];
extern unsigned int D_0063ED30[8];

void xglFontFlush(void)
{
    /* The font drawing cursor resides at the base of EE scratchpad RAM. */
    struct FontDrawCursor *cursor = (struct FontDrawCursor *)SCRATCHPAD_BASE;
    FontQueueSlot *slot;
    int remaining;

    *D_008DAD00.queue_cursor = 0;
    cursor->depth = -1;
    cursor->blue = FONT_COLOR_INITIAL;
    cursor->green = FONT_COLOR_INITIAL;
    cursor->red = FONT_COLOR_INITIAL;
    if (D_008DAD00.debug_mode) {
        cursor->offset_x = 0;
        cursor->offset_y = 0;
    } else {
        cursor->offset_x = FONT_SCREEN_ORIGIN_X;
        cursor->offset_y = FONT_SCREEN_ORIGIN_Y;
    }
    cursor->flags = 0;
    cursor->effect_y = cursor->effect_x = cursor->effect = 1;
    cursor->effect_red = 0;
    cursor->scale_x = FONT_SCALE_ONE;
    cursor->effect_green = 0;
    cursor->primitive_flags = FONT_ALPHA_SOURCE_BLEND;
    cursor->alpha = GS_ALPHA_OPAQUE;
    cursor->effect_blue = 0;
    cursor->primitive_mode = 0;
    cursor->scale_y = FONT_SCALE_ONE;
    remaining = FONT_QUEUE_BUCKETS - 1;
    cursor->packet = xglPacketGetCurrent();
    xglFontReloadTexture(cursor, 0x80000001);

    slot = fontPrintQueueBuffer.slots;
    do {
        unsigned short offset = slot->link;
        if (offset) {
            do {
                struct FontQueueRecord *record =
                    (struct FontQueueRecord *)&fontPrintQueueBuffer.bytes[offset];
                unsigned char *text = (unsigned char *)(record + 1);
                offset = record->next_low + (record->next_high << 8);
                xglFontFlushCore(cursor, text);
            } while (offset);
        }
        remaining--;
        slot++;
    } while (remaining >= 0);
    if (D_008DAD00.debug_mode) {
        sceVif1PkAddDirectDataN(cursor->packet, D_0063ECD0, 6);
    }
    sceVif1PkAddDirectDataN(cursor->packet, D_0063ED30, 2);
    sceVif1PkCloseDirectHLCode(cursor->packet);
    xglFontResetPrintQueue();
}

int xglFontGetLoadAddress(void)
{
    return (int) FS.state.window_image;
}

unsigned short xglFontGetFlags(void)
{
    return FONT_STATE_FLAGS;
}

void xglFontSetFlags(int flags)
{
    FONT_STATE_FLAGS = flags & (0xFFFF & ~FONT_STATE_DEBUG_PRINT);
}

unsigned char xglFontLoad(int font_index, int load_flags)
{
    unsigned char previous_font;
    int read_mode;

    previous_font = D_008DAD00.font_selection;
    D_008DAD00.font_selection = (unsigned char)font_index;
    read_mode = 1;
    if (load_flags == 0) {
        if (font_index != 0) {
            read_mode = 0;
            load_flags = 1;
        } else {
            read_mode = 0;
            load_flags = 1;
        }
    }
    if (font_index == 0) {
        xglCdReadFile(fontTexture0Path, D_008DAD00.window_image, read_mode, load_flags);
    } else {
        xglCdReadFile(fontTexture1Path, D_008DAD00.window_image, read_mode, load_flags);
    }
    return previous_font;
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", xglFontInitial);
