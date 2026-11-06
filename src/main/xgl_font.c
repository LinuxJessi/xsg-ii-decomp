#include "common.h"
#include "shared.h"
#include "main/xgl_font.h"

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
/* EUC-JP code points and rows. */
#define FONT_JIS_ROW_SIZE 94
#define EUC_FIRST 0xa1
#define EUC_ROW_SYMBOLS 0xa1
#define EUC_ROW_ALNUM 0xa3
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
    unsigned char queue_reset_flag;
    unsigned char proportional_mode;
    unsigned char horizontal_shift;
    int stored_value;
    unsigned char unmodeled_18[4];
    unsigned char *queue_cursor;
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

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", xglFontDebugMode);

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", func_00194DD0);

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

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", set_ot);

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", set_xyz);

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

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", xglFontPrintDirectCore);

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

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", xglFontPrintExtFunc);

/* Stores a value in the font state's stored_value field. */
void xglFontSetStoredValue(int stored_value) {
    FS.state.stored_value = stored_value;
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", xglFontDebugPrintf);

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", xglFontDebugHex);

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", xglFontGetStringWidth2);

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

/*
 * The FontState fields from offset 4 on, read through the scaffold symbol D_008DAD00 (FS + 4),
 * up to the queue end word that sits just below the print-queue slot array.
 */
struct FontStateFromWindow {
    void *window_image;
    unsigned char unmodeled_04[8];
    unsigned char default_width;
    unsigned char queue_reset_flag;
    unsigned char proportional_mode;
    unsigned char unmodeled_0f[1];
    int stored_value;
    unsigned char unmodeled_14[0x110];
    FontQueueSlot *queue_end;
};

extern struct FontStateFromWindow D_008DAD00;

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
    /* The block starts 0x128 bytes before the slot array, at D_008DAD00. */
    block = (struct FontStateFromWindow *) ((unsigned char *) &fontPrintQueueBuffer - 0x128);
    block->stored_value = 0;
    block->queue_reset_flag = 0;
    block->queue_end = slot;
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", func_00197200);

int func_00197200(int, int, int, int);

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

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", xglFontFlushCore);

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

unsigned short xglFontCharToEuc(signed char character)
{
    if (character < 0x20) {
        return EUC_SPACE;
    }
    return asciiEucTable[character];
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", xglFontReloadTexture);

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", xglFontFlush);

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

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", xglFontLoad);

INCLUDE_ASM("asm/main/nonmatchings/xgl_font", xglFontInitial);
