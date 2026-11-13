#include "common.h"
#include "main/pad.h"
#include "main/tu101.h"
#include "main/tu109.h"
#include "main/tu096.h"
#include "main/tu097.h"
#include "main/tu110.h"

/* Font draw state word; the draw call in textDrawSegment takes it as its third argument. */
extern int D_0069D580;

void func_0021A520(void)
{
    D_0069D580 = 0xFFFFF0;
}

/* One text segment: optional colour and alpha control tags, then the draw call. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", textDrawSegment);

/* Multi-line text: a backslash-n escape starts a new line, textCursorY is the y cursor. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", textDrawLines);

/* The request block fontObjDraw (packet builder, main/tu097) reads. The submit
   helpers store the command word, operands and flags; menuDrawAnimatedBand stores
   a length (0x134) as the command word, an animation phase, a reset value and the
   kind 0x21 as flags. */
typedef struct PacketRequest {
    int command;
    int first;
    int second;
    int phase;
    int reset_value;
    int fourth;
    int fifth;
    int flags;
} PacketRequest;

extern PacketRequest packetRequest;
int fontObjDraw(PacketRequest *block);

/* Fills the request's command word, two operands and flags (bits 3 and 5 cleared), then submits it. */
void packetSubmitRequest(int command, int first, int second, int flags)
{
    packetRequest.command = command;
    packetRequest.first = first;
    packetRequest.second = second;
    packetRequest.flags = flags & ~0x28;
    fontObjDraw(&packetRequest);
}

/* Like packetSubmitRequest with two more operands; bit 3 of flags set, bit 5 cleared. */
void packetSubmitRequestEx(int command, int first, int second, int fourth, int fifth, int flags)
{
    packetRequest.command = command;
    packetRequest.first = first;
    packetRequest.second = second;
    packetRequest.fourth = fourth;
    packetRequest.fifth = fifth;
    packetRequest.flags = (flags | 8) & ~0x20;
    fontObjDraw(&packetRequest);
}

extern signed char textCharSpacing;
unsigned long long fontObjGetColor(void);
int fontObjCellOffsetX(int command);

/* Glyph strips of textDrawGlyphString: the request command of the first draw of each class,
 * and the cell (strip) that class's face is drawn from. */
#define GLYPH_CMD_UPPER 0x12
#define GLYPH_CMD_LOWER 0x13
#define GLYPH_CMD_DIGIT 0x14
#define GLYPH_CMD_PERIOD 0x15
#define GLYPH_CMD_PLUS 0x4C
#define GLYPH_CELL_UPPER 0xA0
#define GLYPH_CELL_LOWER 0x16
#define GLYPH_CELL_DIGIT 0x17
#define GLYPH_CELL_PERIOD 0x20
#define GLYPH_CELL_PLUS 0x4B
/* Glyph index inside the strip for symbols. */
#define GLYPH_INDEX_HYPHEN 0x1A
#define GLYPH_INDEX_SLASH 0xA
#define GLYPH_INDEX_COLON 0xB
#define GLYPH_SHADOW_OFFSET 4
#define GLYPH_FLAGS_SHADOW 0x24
#define GLYPH_FLAGS_FACE 0x64

/* Text run: maps each character to a glyph cell (capitals, small letters, digits and a few
   symbols), draws it twice (shadow in `colour` shifted up-left, then the current font colour)
   and advances x by the cell width with per-glyph kerning. Once y is stored, the parameter is
   reused as the glyph index inside the cell row. */
void textDrawGlyphString(int x, int y, unsigned char *text, unsigned long long colour)
{
    unsigned long long fontColour = fontObjGetColor();
    unsigned char *p;
    int cell;
    PacketRequest *request;

    packetRequest.first = x;
    packetRequest.second = y;
    packetRequest.reset_value = 0;
    /* Original walks a copy of the parameter. */
    p = text;
    while (*p != 0) {
        if ((unsigned char) (*p - 'A') < 26) {
            packetRequest.command = GLYPH_CMD_UPPER;
            cell = GLYPH_CELL_UPPER;
            y = *p - 'A';
        } else if ((unsigned char) (*p - 'a') < 26) {
            packetRequest.command = GLYPH_CMD_LOWER;
            cell = GLYPH_CELL_LOWER;
            y = *p - 'a';
        } else if ((unsigned char) (*p - '0') < 10) {
            packetRequest.command = GLYPH_CMD_DIGIT;
            cell = GLYPH_CELL_DIGIT;
            y = *p - '0';
        } else {
            switch (*p) {
            case ' ':
                packetRequest.first += textCharSpacing;
                p++;
                continue;
            case '-':
                packetRequest.command = GLYPH_CMD_LOWER;
                cell = GLYPH_CELL_LOWER;
                y = GLYPH_INDEX_HYPHEN;
                break;
            case '/':
                packetRequest.command = GLYPH_CMD_DIGIT;
                cell = GLYPH_CELL_DIGIT;
                y = GLYPH_INDEX_SLASH;
                break;
            case ':':
                packetRequest.command = GLYPH_CMD_DIGIT;
                cell = GLYPH_CELL_DIGIT;
                y = GLYPH_INDEX_COLON;
                break;
            case '.':
                packetRequest.command = GLYPH_CMD_PERIOD;
                cell = GLYPH_CELL_PERIOD;
                y = 0;
                packetRequest.first -= 2;
                break;
            case '+':
                packetRequest.command = GLYPH_CMD_PLUS;
                cell = GLYPH_CELL_PLUS;
                y = 0;
                break;
            default:
                p++;
                continue;
            }
        }
        switch (*p) {
        case 'I':
        case 'i':
        case 'l':
        case 'r':
            packetRequest.first -= 1;
            break;
        case 'm':
        case 'w':
            packetRequest.first += 1;
            break;
        }
        /* Original code takes the address twice (here and before the face draw). */
        request = &packetRequest;
        request->phase = y * fontObjCellOffsetX(request->command);
        request->first -= GLYPH_SHADOW_OFFSET;
        request->second -= GLYPH_SHADOW_OFFSET;
        fontObjSetColor(colour);
        request->flags = GLYPH_FLAGS_SHADOW;
        fontObjDraw(request);
        request->first += GLYPH_SHADOW_OFFSET;
        request->second += GLYPH_SHADOW_OFFSET;
        if (*p == '.') {
            request->first += 2;
        }
        if (*p == 'Z') {
            request->command = cell + 1;
            request->phase = 0;
        } else {
            request->command = cell;
            request->phase = y * fontObjCellOffsetX(cell);
        }
        request = &packetRequest;
        fontObjSetColor(fontColour);
        request->flags = GLYPH_FLAGS_FACE;
        fontObjDraw(request);
        request->first += fontObjCellOffsetX(cell) - 1;
        if (cell == GLYPH_CELL_DIGIT || cell == GLYPH_CELL_PLUS) {
            request->first += 3;
        }
        switch (*p) {
        case '.':
            packetRequest.first -= 3;
            break;
        case 'I':
        case 'f':
        case 'i':
        case 'j':
        case 'l':
        case 't':
            packetRequest.first -= 1;
            break;
        case 'm':
        case 'w':
            packetRequest.first += 1;
            break;
        }
        p++;
    }
}

/* Text run drawn at (x + 1, y + 3) after setting a state word with zero colour bytes; the previous state is restored. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", textDrawShadow);

/* Text run with the second glyph bank (kinds 0x7A to 0x7F). */
INCLUDE_ASM("asm/main/nonmatchings/tu101", textDrawStringAlt);

/* Shadowed text run over the second glyph bank. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", textDrawShadowAlt);

/* Text run with a leading mode word (0x10034 or 0x74 in the request flags). */
INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021AFE8);

/* Formats an integer in decimal into a static buffer with width and zero-pad flag; returns the buffer. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", textFormatDecimal);

/* Decimal number at (x, y): width 1 to 9, 9-pixel character spacing, space padding. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", textDrawDecimal);

/* textDrawDecimalAlt variant: second glyph bank, width up to 10, 7-pixel spacing. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", textDrawDecimalAlt);

/* textDrawDecimal with zero padding. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", textDrawDecimalZero);

/* Decimal number through the shadowed text run. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", textDrawDecimalShadow);

/* textDrawDecimalAlt through the shadowed second-bank run. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", textDrawDecimalAltShadow);

/* textDrawDecimalShadow with zero padding. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", textDrawDecimalShadowZero);

/* Signed decimal (leading '+' or '-') through the shadowed text run. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", textDrawSignedDecimalShadow);

/* Decimal digits at (x, y), 9-pixel pitch, glyph kind 0x1D, digits taken from the value by repeated division by 10. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", textDrawDigits);

/* Digit run of the same family; the loop differs from textDrawDigits (unnamed). */
INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021B8C8);

/* Digit run from textDrawDigits followed by one glyph of kind 0x63 at x + 9. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", textDrawDigitsPair);

/* Digit run in glyph kind 0x27B with a 12-pixel pitch. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", textDrawDigitsWide);

/* Digit run in glyph kind 0x20C. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021BB10);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021BBE8);

/* Nine-piece frame (corners, edges, centre) from the record at a0; centre kind 0x109. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", panelDrawNineSlice);

/* Nine-piece frame, sprite bank not yet identified. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021BFA0);

/* Nine-piece frame with centre kind 0x109 and an extra kind 0xA11. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021C228);

/* Nine-piece frame, sprite bank 0x108. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021C498);

/* Record group of eleven submits, sprite kinds 0x116 to 0x11F. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021C6F8);

/* Record group, sprite kinds 0x135 to 0x137 and 0x11A to 0x11E. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021CA40);

/* Record group of ten submits, sprite kinds 0x100 to 0x10A. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021CC08);


/* Calls the tu104 routine menuDrawOutlineFrame and nothing else: the callers' four arguments pass straight through. */
void func_0021CEE0(int x, int y, int width, int height) {
    menuDrawOutlineFrame(x, y, width, height);
}

/* Fixed set of records (kinds 0x52 to 0x5A and 0x118) after func_0021E338. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021CEF8);

/* func_0021CEF8 with a colour state set first, then restored. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021D048);

/* func_0021D048 with a second colour word. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021D1B8);

/* Group of eleven records (kinds 0x226 to 0x231, 0x254) placed from (x, y). */
INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021D328);

/* Three-record group (kinds 0x13A to 0x13C) at (x, y, width). */
INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021D550);

INCLUDE_ASM("asm/main/nonmatchings/tu101", menuDrawBorder);

INCLUDE_ASM("asm/main/nonmatchings/tu101", menuDrawPanelFrame);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021D8B8);


/* Panel frame: border and corner pieces 0x2F-0x35 with the fill in mode 0x106; the tint is set first. */
void menuDrawPanelFrameAlt(int x, int y, int width, int height, int color) {
    int bottomY;
    int innerY;
    int innerX;
    int innerWidth;
    int innerHeight;

    bottomY = (y + height) - 0x10;
    innerY = y + 0x10;
    innerX = x + 0x10;
    innerWidth = width - 0x20;
    fontObjSetColor(color);
    packetSubmitRequest(0x2F, x, bottomY, 6);
    innerHeight = height - 0x20;
    packetSubmitRequestEx(0x30, innerX, bottomY, innerWidth, 0x10, 6);
    packetSubmitRequest(0x31, (x + width) - 0x10, bottomY, 6);
    packetSubmitRequestEx(0x32, x, innerY, 0x10, innerHeight, 6);
    packetSubmitRequestEx(0x33, innerX, innerY, innerWidth, innerHeight, 0x106);
    packetSubmitRequestEx(0x32, (x + width) - 0x10, innerY, 0x10, innerHeight, 6);
    packetSubmitRequest(0x34, x, y, 6);
    packetSubmitRequestEx(0x30, innerX, y, innerWidth, 0x10, 6);
    packetSubmitRequest(0x35, (x + width) - 0x10, y, 6);
    fontObjResetColor();
}

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021DB98);

INCLUDE_ASM("asm/main/nonmatchings/tu101", menuDrawPanel);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021DDB8);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021DE60);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021E338);

void menuDrawOutlineFrame(int x, int y, int width, int height) {
    int rightX;
    int bottomY;
    int innerY;
    int innerHeight;
    int innerX;
    int innerWidth;

    rightX = (x + width) - 0x10;
    bottomY = (y + height) - 0x10;
    innerY = y + 0x10;
    innerHeight = height - 0x20;
    innerX = x + 0x10;
    packetSubmitRequest(0x63, x, bottomY, 0);
    packetSubmitRequestEx(0x64, x, innerY, 0x10, innerHeight, 0);
    innerWidth = width - 0x20;
    packetSubmitRequest(0x65, x, y, 0);
    packetSubmitRequestEx(0x66, innerX, y, innerWidth, 0x10, 0);
    packetSubmitRequest(0x67, rightX, y, 0);
    packetSubmitRequest(0x5D, x, bottomY, 1);
    packetSubmitRequestEx(0x5E, innerX, bottomY, innerWidth, 0x10, 1);
    packetSubmitRequest(0x5F, rightX, bottomY, 1);
    packetSubmitRequestEx(0x60, rightX, innerY, 0x10, innerHeight, 1);
    packetSubmitRequest(0x61, rightX, y, 1);
}

/* Split frame: outer pieces 0x4D-0x59, the 0x62 mark at split and 0x50 bars on both sides of it. */
void menuDrawSplitFrame(int x, int y, int width, int height, int split) {
    int bottom;
    int top_inner;
    int split_x;
    int right;
    int left_inner;

    bottom = (y + height) - 0x10;
    top_inner = y + 0x11;
    split_x = x + split;
    right = (x + width) - 0x10;
    left_inner = x + 0x10;
    packetSubmitRequest(0x55, x, bottom, 0);
    packetSubmitRequestEx(0x56, left_inner, bottom, width - 0x20, 0x10, 0);
    packetSubmitRequest(0x57, right, bottom, 0);
    packetSubmitRequestEx(0x58, right, y + 0x21, 0x10, height - 0x31, 0);
    packetSubmitRequest(0x59, right, top_inner, 0);
    packetSubmitRequest(0x4D, x, bottom, 1);
    packetSubmitRequestEx(0x4E, x, y + 0x10, 0x10, height - 0x20, 1);
    packetSubmitRequest(0x4F, x, y, 1);
    packetSubmitRequest(0x51, right, top_inner, 1);
    packetSubmitRequest(0x62, split_x, y, 1);
    packetSubmitRequestEx(0x50, left_inner, y, split - 0x10, 0x10, 1);
    packetSubmitRequestEx(0x50, split_x + 0x20, top_inner, (width - split) - 0x30, 0x10, 1);
}

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021E7B0);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021EE68);

#include "shared.h"

/* Record reset by menuResetRecord: status is set to -1, both blocks are
   cleared through fontTrackSetBoth and pending is zeroed. */
typedef struct Tu104Record {
    int status;
    struct FontTrack first_block;
    struct FontTrack second_block;
    int pending;
} Tu104Record;


void fontObjSetColorRgba(int, int, int, int);
void textDrawDigits(int, int, int, int);
void func_0021CC08(int, int, int, int);
void func_0021F658(int, int, int, int, int);
extern unsigned short menuIconKinds[];
extern struct FontTrack menuCursorPos[];
extern struct FontTrack menuCursorSize[];

/* Icon at (x, y): the backing piece 0x157, then the kind stored in menuIconKinds[index]. */
void menuDrawIndexedIcon(int x, int y, int index)
{
    unsigned short iconKind = menuIconKinds[index];

    packetSubmitRequest(0x157, x, y, 0x10);
    packetSubmitRequest(iconKind, x, y, 0x11);
}

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021EFC8);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021F0A0);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021F110);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021F190);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021F218);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021F2F0);

/* Cursor box: position from menuCursorPos, size from menuCursorSize, kind 0x82 with mode 0x105. */
void menuDrawCursor(void)
{
    fontTrackEase(menuCursorPos);
    fontTrackEase(menuCursorSize);
    fontTrackEase(menuCursorPos);
    fontTrackEase(menuCursorSize);
    fontObjSetColorRgba(0x24, 0, 0x48, 0x80);
    packetSubmitRequestEx(0x82, menuCursorPos[0].current_pair[0], menuCursorPos[0].current_pair[1], menuCursorSize[0].current_pair[0], menuCursorSize[0].current_pair[1], 0x105);
    fontObjResetColor();
}

/* Select arrows 0x164 and 0x165 beside a row; their bob offset comes from xglTickGet. */
void menuDrawSelectArrows(int x, int width, int y)
{
    int offset = (xglTickGet() & 0xF) >> 1;
    int row = y + 0x3C;

    packetSubmitRequest(0x164, x - offset - 8, row, 1);
    packetSubmitRequest(0x165, x + width + offset - 8, row, 1);
}

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021F530);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021F658);

/* List window: a 0xE0 x 0x11F panel; with two or more entries it adds the 0x1C mark and the rows (func_0021F658). */
void menuDrawListWindow(int x, int y, int layer, int count)
{
    func_0021CC08(x, y, 0xE0, 0x11F);
    if (count >= 2) {
        packetSubmitRequest(0x1C, x + 0x94, y + 0x10C, 0x40);
        textDrawDigits(x + 0xC0, y + 0x10C, layer + 1, 2);
        func_0021F658(x + 0xC, y + 0x101, layer, count, 8);
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021F8A8);

INCLUDE_ASM("asm/main/nonmatchings/tu101", menuDrawCharInfo);

INCLUDE_ASM("asm/main/nonmatchings/tu101", menuDrawItemList);

INCLUDE_ASM("asm/main/nonmatchings/tu101", menuDrawItemListDetail);

INCLUDE_ASM("asm/main/nonmatchings/tu101", menuListContains);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_00220210);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_00220550);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_00220840);

INCLUDE_ASM("asm/main/nonmatchings/tu101", menuDrawCharStats);

INCLUDE_ASM("asm/main/nonmatchings/tu101", menuDrawEquipPanel);

INCLUDE_ASM("asm/main/nonmatchings/tu101", menuDrawItemCategory);

void menuDrawKindIcon(int x, int y, int index)
{
    fontObjSetColor(0x3F80000080000000LL);
    packetSubmitRequest(0x44, x, y, 4);
    fontObjResetColor();
    packetSubmitRequest(index + 0x47, x, y, 1);
}

INCLUDE_ASM("asm/main/nonmatchings/tu101", menuDrawAttackCommand);

INCLUDE_ASM("asm/main/nonmatchings/tu101", menuDrawSpecialAttack);

INCLUDE_ASM("asm/main/nonmatchings/tu101", menuDrawCommandInfo);

INCLUDE_ASM("asm/main/nonmatchings/tu101", menuDrawPartnerInfo);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_00222860);

INCLUDE_ASM("asm/main/nonmatchings/tu101", menuDrawCopilotInfo);

INCLUDE_ASM("asm/main/nonmatchings/tu101", menuDrawClassInfo);

/* The context returned by ptGetMenuMember (defined in main/tu100). */
struct ControlLimits {
    unsigned char unmodeled_00[8];
    int limit_a;
    int limit_b;
};

struct ControlBuffer {
    unsigned char header[4];
    unsigned char cells[4][8];
    unsigned char flags[2];
};

struct ControlContext {
    unsigned char unmodeled_00[8];
    struct ControlLimits *limits;
    struct ControlBuffer *buffer;
};

/* A scrolling list cursor: selected row, row count and the blink flag. */
typedef struct LicenseCursor {
    int index;
    int unmodeled_04;
    unsigned int count;
    int unmodeled_0c;
    int flags;
} LicenseCursor;

typedef struct LicenseRecord {
    unsigned char kind;
    unsigned char dispatch_value;
    unsigned short value;
    unsigned char bit;
    unsigned char unmodeled_05;
} LicenseRecord;

struct ControlContext *func_0022A560();
int selectionCanOpenSlot(struct ControlContext *context, int group, int sub);
int selectionCanTakeSub(struct ControlContext *context, int group, int slot, int sub);
void func_0021CEF8(int x, int y, int width, int height, void *text);
void func_0021CC08(int, int, int, int);
void menuDrawBorder(int x, int y, int width, int height, unsigned long long color);
int cursorBufferGetAlignedOffset(LicenseCursor *cursor);
void menuDrawCharInfo(int x, int y, int group, int slot, int mode);
void textDrawSegment(int, int, void *, int);
void textDrawGlyphString(int x, int y, unsigned char *text, u64 colour);
void textDrawDecimalShadow(int x, int y, int value, int digits, unsigned long long color);
void func_0021F2F0(int x, int y, int width, int blink);
void func_0021F8A8(int x, int y, int record);
int selectionEntryBitOutsideMask(int group, int slot, int sub, int mask);
int selectionRecordIndex(int group, int slot, int sub);
extern LicenseRecord selectionRecordTable[];
extern unsigned char msgStyleInit[];
extern unsigned char D_0069D620[];
extern unsigned char D_0068AB18[];
extern unsigned char D_0069D720[];
extern unsigned char D_0069D728[];
extern unsigned char D_0069D730[];
extern unsigned char D_0069D738[];
extern unsigned char D_0069D608[];
extern unsigned short D_003EC0E8[4][8];

#define LICENSE_ROW_PITCH 0x17 /* rows of the group and sub pages */
#define LICENSE_ROW_PITCH_COMPACT 0x12 /* rows of the slot page */
#define LICENSE_COLOR_SELECTED 0x3F80000080120B50LL
#define LICENSE_COLOR_IDLE 0x3F80000000000000LL
#define LICENSE_MARK_SHADOW 0x3F80000080000000LL
#define LICENSE_MARK_FILL 0x3F80000080407080LL
#define LICENSE_MARK_CMD 0x80 /* request command of one mark */
#define LICENSE_MARK_BITS 4

/* License board page: three tabs (mode 0, 1, 2), each with its own list of rows. */
void menuDrawLicenseBoard(int x, int y, LicenseCursor *groupCursor, LicenseCursor *slotCursor,
                          LicenseCursor *subCursor, int mode, int mask)
{
    int firstRow; /* first visible row; scrolls the highlight */
    int unlocked = 0;
    int markCount = 0;
    struct ControlBuffer *board = ptGetMenuMember()->buffer;
    LicenseCursor *cursor;
    int i;
    int rowStyle;
    unsigned long long color;
    int k;
    int tabX;
    int record;
    int kind;

    switch (mode) {
    case 0:
    default:
        func_0021CEF8(x + 8, y + 0x51, 0x9F, 0xAE, D_0069D620);
        func_0021CEF8(x + 0xA9, y + 0x51, 0x44, 0xAE, D_0068AB18);
        cursor = groupCursor;
        break;
    case 1:
        func_0021CEF8(x + 8, y + 0x51, 0xA9, 0xAE, D_0069D620);
        func_0021CEF8(x + 0xB3, y + 0x51, 0x3A, 0xAE, D_0069D720);
        cursor = slotCursor;
        break;
    case 2:
        func_0021CEF8(x + 8, y + 0x51, 0xA9, 0x77, D_0069D620);
        func_0021CEF8(x + 0xB3, y + 0x51, 0x3A, 0x77, D_0069D728);
        cursor = subCursor;
        break;
    }
    firstRow = cursorBufferGetAlignedOffset(cursor);
    func_0021CC08(x, y, 0xF5, 0x11F);
    menuDrawBorder(x + 8, y + 0x1F, 0xE5, 0x2E, 0x3F80000080381A1ALL);
    func_0021CEF8(x + 0xC, y + 0x24, 0xDD, 0x24, D_0069D730);
    packetSubmitRequest(0x12E, x + 0x4C, y + 4, 1);
    textDrawSegment(0, 0, msgStyleInit, 0);
    menuDrawCharInfo(x + 0x18, y + 0x31, groupCursor->index, slotCursor->index, mode);

    for (i = 0; i < cursor->count; i++) {
        switch (mode) {
        case 0:
            if (i <= board->flags[0]) {
                unlocked = 1;
            } else {
                unlocked = 0;
            }
            break;
        case 1:
            if (selectionCanOpenSlot(func_0022A560(), groupCursor->index, i)
                || ((board->header[groupCursor->index] >> i) & 1)) {
                unlocked = 1;
            } else {
                unlocked = 0;
            }
            markCount = 0;
            for (k = 0; k < LICENSE_MARK_BITS; k++) {
                markCount += (board->cells[groupCursor->index][i] >> k) & 1;
            }
            break;
        case 2:
            if (selectionCanTakeSub(func_0022A560(), groupCursor->index, slotCursor->index, i)
                || ((board->cells[groupCursor->index][slotCursor->index] >> i) & 1)) {
                unlocked = 1;
            } else {
                unlocked = 0;
            }
            break;
        }
        rowStyle = i == cursor->index;
        color = rowStyle ? LICENSE_COLOR_SELECTED : LICENSE_COLOR_IDLE;
        if (!unlocked) {
            rowStyle |= 2;
        }
        switch (mode) {
        case 0:
        default:
            textDrawSegment(x + 0x18, y + 0x62 + i * LICENSE_ROW_PITCH, menuMainTxdEntry(i + 0x36), rowStyle);
            if (unlocked) {
                textDrawGlyphString(x + 0xB8, y + 0x66 + i * LICENSE_ROW_PITCH, D_0069D738, color);
            } else {
                textDrawGlyphString(x + 0xBA, y + 0x66 + i * LICENSE_ROW_PITCH, D_0069D608, color);
            }
            break;
        case 1:
            textDrawSegment(x + 0x18, y + 0x62 + i * LICENSE_ROW_PITCH_COMPACT, menuMainTxdEntry(i + 0x3A), rowStyle);
            for (k = 0; k < markCount; k++) {
                fontObjSetColor(LICENSE_MARK_SHADOW);
                packetSubmitRequest(LICENSE_MARK_CMD, x + 0x71 + k * 0xE, y + 0x67 + i * LICENSE_ROW_PITCH_COMPACT, 0x44);
                fontObjSetColor(LICENSE_MARK_FILL);
                packetSubmitRequest(LICENSE_MARK_CMD, x + 0x70 + k * 0xE, y + 0x64 + i * LICENSE_ROW_PITCH_COMPACT, 0x44);
                fontObjResetColor();
            }
            if ((board->header[groupCursor->index] >> i) & 1) {
                textDrawGlyphString(x + 0xBF, y + 0x66 + i * LICENSE_ROW_PITCH_COMPACT, D_0069D608, color);
            } else {
                if (D_003EC0E8[groupCursor->index][i] > ptGetMenuMember()->limits->limit_b) {
                    fontObjSetOpacity(0x40);
                } else {
                    fontObjResetOpacity();
                }
                textDrawDecimalShadow(x + 0xBF, y + 0x66 + i * LICENSE_ROW_PITCH_COMPACT, D_003EC0E8[groupCursor->index][i], 4, color);
            }
            break;
        case 2:
            {
                int outside = selectionEntryBitOutsideMask(groupCursor->index, slotCursor->index, i, mask);
                record = selectionRecordIndex(groupCursor->index, slotCursor->index, i);

                if (outside) {
                    rowStyle |= 2;
                    textDrawSegment(x + 0x10, y + 0x62 + i * LICENSE_ROW_PITCH, menuMainTxdEntry(0x190), rowStyle);
                    textDrawGlyphString(x + 0xBF, y + 0x66 + i * LICENSE_ROW_PITCH, D_0069D608, color);
                } else {
                    textDrawSegment(x + 0x10, y + 0x62 + i * LICENSE_ROW_PITCH,
                                    selectionSetValue(groupCursor->index, slotCursor->index, i, 0), rowStyle);
                    kind = selectionRecordTable[record].kind;
                    if (kind == 0 || kind == 1) {
                        if (kind == 0) {
                            packetSubmitRequest(0x133, x + 0x98, y + 0x62 + i * LICENSE_ROW_PITCH, 0);
                        } else {
                            packetSubmitRequest(0x131, x + 0x98, y + 0x62 + i * LICENSE_ROW_PITCH, 0);
                        }
                    } else {
                        packetSubmitRequest(0x132, x + 0x98, y + 0x62 + i * LICENSE_ROW_PITCH, 0);
                    }
                    if ((board->cells[groupCursor->index][slotCursor->index] >> i) & 1) {
                        fontObjSetColor(LICENSE_MARK_SHADOW);
                        packetSubmitRequest(LICENSE_MARK_CMD, x + 0xCB, y + 0x69 + i * LICENSE_ROW_PITCH, 0x44);
                        fontObjSetColor(LICENSE_MARK_FILL);
                        packetSubmitRequest(LICENSE_MARK_CMD, x + 0xCA, y + 0x66 + i * LICENSE_ROW_PITCH, 0x44);
                        fontObjResetColor();
                    } else {
                        if (selectionGetValue(groupCursor->index, slotCursor->index,
                                              i) > ptGetMenuMember()->limits->limit_a) {
                            fontObjSetOpacity(0x40);
                        } else {
                            fontObjResetOpacity();
                        }
                        textDrawDecimalShadow(x + 0xBF, y + 0x66 + i * LICENSE_ROW_PITCH,
                                              selectionGetValue(groupCursor->index, slotCursor->index, i), 4, color);
                    }
                }
            }
            break;
        }
        fontObjResetOpacity();
    }

    if (cursor->flags & 1) {
        switch (mode) {
        case 0:
            func_0021F2F0(x + 0xA, y + 0x62 + (cursor->index - firstRow) * LICENSE_ROW_PITCH, 0x9B, cursor->flags & 1);
            break;
        case 1:
            func_0021F2F0(x + 0xA, y + 0x62 + (cursor->index - firstRow) * LICENSE_ROW_PITCH_COMPACT, 0xA5, cursor->flags & 1);
            break;
        case 2:
            func_0021F2F0(x + 0xA, y + 0x62 + (cursor->index - firstRow) * LICENSE_ROW_PITCH, 0xA5, cursor->flags & 1);
            break;
        }
        menuDrawCursor();
    }

    tabX = x + 0x10;
    for (i = 0; i < 3; i++) {
        if (i == mode) {
            tabX -= 4;
            packetSubmitRequest(0x130, tabX, y + 0x101, 1);
            tabX += 0x18;
        } else {
            packetSubmitRequest(0x15A, tabX, y + 0x10B, 1);
            tabX += 0xC;
        }
    }
    switch (mode) {
    case 0:
    default:
        packetSubmitRequest(0x21, x + 0xB2, y + 0x10C, 0x40);
        break;
    case 1:
        packetSubmitRequest(0x81, x + 0xB2, y + 0x10D, 0x40);
        break;
    case 2:
        packetSubmitRequest(0x22, x + 0xB2, y + 0x10C, 0x40);
        {
            int entry = selectionRecordIndex(groupCursor->index, slotCursor->index, subCursor->index);

            func_0021F8A8(x + 0x7A, y + 0xE0, entry);
        }
        break;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu101", menuDrawConfig);

INCLUDE_ASM("asm/main/nonmatchings/tu101", menuDrawControllerConfig);

INCLUDE_ASM("asm/main/nonmatchings/tu101", menuDrawPlayTime);

void menuResetRecord(Tu104Record *record)
{
    if (record != 0) {
        record->status = -1;
        fontTrackSetBoth(&record->first_block, 0, 0);
        fontTrackSetBoth(&record->second_block, 0, 0);
        record->pending = 0;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_002245B8);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_00224608);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_002246A0);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_00224740);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_00224810);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_00224928);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_00224A28);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_00224B40);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_00224C40);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_00224CF8);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_00224E20);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_00224F20);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_00225048);

void menuDrawAnimatedBand(int first, int second)
{
    packetRequest.first = first;
    packetRequest.second = second;
    packetRequest.command = 0x134;
    packetRequest.phase = (xglTickGet() & 7) * 0x10;
    packetRequest.flags = 0x21;
    packetRequest.reset_value = 0;
    fontObjDraw(&packetRequest);
}

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_002251A0);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_002253A8);

/* The window a party entry is drawn in: position, opacity, fade and layout flags. */
typedef struct MenuPartyPanel {
    unsigned char unmodeled_00[4];
    int x;                                  /* 0x04 */
    int y;                                  /* 0x08 */
    unsigned char unmodeled_0c[0x14];
    int opacity;                            /* 0x20 */
    int fade;                               /* 0x24: 0 none, else scales the opacity of the fade mark by /128 */
    unsigned char unmodeled_28[0x14];
    unsigned int flags;                     /* 0x3C */
} MenuPartyPanel;

/* Name and icon of a party member. */
typedef struct MenuPartyName {
    char *text;                             /* 0x00 */
    unsigned char unmodeled_04[4];
    unsigned char icon;                     /* 0x08 */
} MenuPartyName;

typedef struct MenuPartyValues {
    unsigned char unmodeled_00[4];
    int value4;                             /* 0x04 */
    int value8;                             /* 0x08 */
    int valueC;                             /* 0x0C */
} MenuPartyValues;

typedef struct MenuPartyEntry {
    int id;                                 /* 0x00: character, looked up with func_A8B490 */
    MenuPartyName *name;                    /* 0x04 */
    MenuPartyValues *values;                /* 0x08 */
} MenuPartyEntry;

/* Admitted aligned(1) scalar: the gauge word sits at the unaligned offset 2 of the record. */
typedef signed int UnalignedS32 __attribute__((aligned(1)));

/* The character record returned by func_A8B490 (OV01). */
typedef struct MenuPartyRecord {
    unsigned char unmodeled_00[2];
    UnalignedS32 gauge;                     /* 0x02 */
    short value6;                           /* 0x06 */
    unsigned char unmodeled_08[0xB];
    signed char level;                      /* 0x13 */
    unsigned char unmodeled_14[0x48];
    int value5C;                            /* 0x5C */
    short value60;                          /* 0x60 */
} MenuPartyRecord;

#define MENU_PARTY_TEXT_COLOR 0x3F8000008054200BLL
#define MENU_PARTY_ICON_SHADOW 0x3F80000080665B6ELL
#define MENU_PARTY_ICON_FILL 0x3F80000080410C3ALL
#define MENU_PARTY_FADE_SHIFT 7 /* panel->fade is a fraction of 128 */
#define MENU_PARTY_HIDDEN 9 /* layout value that draws nothing */
/* MenuPartyPanel.flags */
#define PARTY_PANEL_ALT_FRAME 0x4 /* draws the 0x121/0x115 frame pair instead of 0x120/0x114 */
#define PARTY_PANEL_MARK_B 0x10 /* mark 0x13F instead of 0x13E */
#define PARTY_PANEL_MARK_A 0x200 /* mark 0x13D */
#define PARTY_PANEL_VALUES 0x400 /* layout 4 shows values C and 8 instead of gauge and level */
#define PARTY_PANEL_TALL 0x10000 /* tall variant of layouts 5 to 7 */

extern unsigned char menuPanelFrame[];
extern char D_0069D6E0[];

MenuPartyRecord *func_A8B490(int id);
int fontObjCellOffsetX(int command);
void fontObjSetColor(u64 colour);
void panelDrawNineSlice(void *style, int x, int y, int width, int height);
void textDrawStringAlt(int x, int y, char *text, u64 colour);
void textDrawDecimal(int x, int y, int value, int digits, u64 colour);
void textDrawDecimalAlt(int x, int y, int value, int digits, u64 colour);
void func_0021C6F8(int x, int y, int width, int height);
void func_0021CA40(int x, int y);
void func_0021D550(int x, int y, int height);
void func_0021EFC8(int x, int y, int icon);
void func_0021F0A0(int x, int y, int icon);
void func_0021F218(int x, int y, int icon);
void func_00224608(int kind, int x, int y, u64 colour);
void func_002246A0(int x, int y, int level);
void func_00224810(int x, int y, int value, int gauge);
void func_00224928(int x, int y, int first, int second);
void func_00224A28(int x, int y, int value, int gauge);
void func_00224B40(int x, int y, int first, int second);
void func_002253A8(int x, int y, int kind);

void menuDrawPartyPanel(MenuPartyPanel *panel, MenuPartyEntry *entry, int x, int y, int layout)
{
    MenuPartyRecord *record;
    MenuPartyName *name;
    MenuPartyValues *values;

    if (panel == 0 || entry == 0 || layout == MENU_PARTY_HIDDEN) {
        return;
    }
    x += panel->x;
    y += panel->y;
    name = entry->name;
    record = func_A8B490(entry->id);
    values = entry->values;
    fontObjSetOpacity(panel->opacity);
    switch (layout) {
    case 0:
    case 1:
        if (panel->flags & PARTY_PANEL_ALT_FRAME) {
            panelDrawNineSlice(menuPanelFrame, x, y, 0xE2, 0x2B);
            packetSubmitRequest(0x121, x + 0xE, y + 4, 0x11);
            func_0021F218(x + 8, y + 7, name->icon);
            textDrawGlyphString(x + 0x28, y + 7, name->text, MENU_PARTY_TEXT_COLOR);
            func_002246A0(x + 0x2F, y + 0x14, record->level);
            func_00224810(x + 0x70, y + 4, record->value5C, record->gauge);
            func_00224928(x + 0x79, y + 0x14, record->value60, record->value6);
            if (panel->fade != 0) {
                fontObjSetOpacity((panel->opacity * panel->fade) >> MENU_PARTY_FADE_SHIFT);
                func_002253A8(x - 0xA, y - 0xE, 1);
            }
        } else {
            panelDrawNineSlice(menuPanelFrame, x, y + 0x19, 0x9D, 0x27);
            packetSubmitRequest(0x120, x + 0x2F, y + 4, 0x11);
            func_0021EFC8(x + 6, y, name->icon);
            textDrawGlyphString(x + 0x34, y + 7, name->text, MENU_PARTY_TEXT_COLOR);
            func_002246A0(x + 0x77, y + 3, record->level);
            func_00224810(x + 0x30, y + 0x1A, record->value5C, record->gauge);
            func_00224928(x + 0x30, y + 0x2A, record->value60, record->value6);
            fontObjSetColor(MENU_PARTY_ICON_SHADOW);
            func_00224608(5, x + 0x30, y + 0x3D, MENU_PARTY_ICON_FILL);
            fontObjResetColor();
            textDrawDecimalAlt(x + 0x60, y + 0x41, values->value4, 8, MENU_PARTY_TEXT_COLOR);
            if (panel->fade != 0) {
                fontObjSetOpacity((panel->opacity * panel->fade) >> MENU_PARTY_FADE_SHIFT);
                func_002253A8(x - 0xA, y - 0xA, 0);
            }
        }
        break;
    case 4:
        func_0021CA40(x, y);
        if (panel->flags & PARTY_PANEL_ALT_FRAME) {
            packetSubmitRequestEx(0x121, x + 0xE, y + 6, fontObjCellOffsetX(0x121), 8, 0x11);
            packetSubmitRequestEx(0x115, x + 0x80, y + 1, 0x5A, 0x10, 0x11);
        } else {
            packetSubmitRequestEx(0x120, x + 0xE, y + 6, fontObjCellOffsetX(0x120), 8, 0x11);
            packetSubmitRequestEx(0x114, x + 0x80, y + 1, 0x5A, 0x10, 0x11);
        }
        func_0021F0A0(x + 8, y + 2, name->icon);
        textDrawGlyphString(x + 0x24, y - 1, name->text, MENU_PARTY_TEXT_COLOR);
        if (panel->flags & PARTY_PANEL_VALUES) {
            fontObjSetColor(0x3F800000807F5747LL);
            func_00224608(2, x + 0x30, y + 0xD, 0x3F8000008052081BLL);
            func_00224608(3, x + 0x84, y + 0xD, 0x3F8000008052081BLL);
            fontObjResetColor();
            textDrawDecimal(x + 0x50, y + 0x11, values->valueC, 5, MENU_PARTY_TEXT_COLOR);
            textDrawDecimal(x + 0xA4, y + 0x11, values->value8, 5, MENU_PARTY_TEXT_COLOR);
        } else {
            func_00224810(x + 0x28, y + 0xD, record->value5C, record->gauge);
            fontObjSetColor(0x3F800000807D763FLL);
            func_00224608(1, x + 0x94, y + 0xD, 0x3F80000080572000LL);
            fontObjResetColor();
            textDrawGlyphString(x + 0xBF, y + 0x11, D_0069D6E0, MENU_PARTY_TEXT_COLOR);
            textDrawDecimal(x + 0xAC, y + 0x11, record->value60, 2, MENU_PARTY_TEXT_COLOR);
            textDrawDecimal(x + 0xC9, y + 0x11, record->value6, 2, MENU_PARTY_TEXT_COLOR);
        }
        break;
    case 7:
        if (panel->flags & PARTY_PANEL_TALL) {
            func_0021C6F8(x, y + 0xB, 0xAE, 0x47);
            if (panel->flags & PARTY_PANEL_ALT_FRAME) {
                packetSubmitRequest(0x121, x + 0x2F, y + 4, 0x11);
                packetSubmitRequest(0x115, x + 0x32, y + 0x1B, 0x11);
            } else {
                packetSubmitRequest(0x120, x + 0x2F, y + 4, 0x11);
                packetSubmitRequest(0x114, x + 0x32, y + 0x1B, 0x11);
            }
            menuDrawIndexedIcon(x + 6, y, name->icon);
            textDrawGlyphString(x + 0x32, y + 7, name->text, MENU_PARTY_TEXT_COLOR);
            func_002246A0(x + 0x88, y + 3, record->level);
            func_00224A28(x + 0x30, y + 0x2B, record->value5C, record->gauge);
            func_00224B40(x + 0x30, y + 0x3B, record->value60, record->value6);
            if (panel->fade != 0) {
                fontObjSetOpacity((panel->opacity * panel->fade) >> MENU_PARTY_FADE_SHIFT);
                func_002253A8(x - 9, y - 0xA, 2);
            }
        } else {
            func_0021D550(x, y, 0x5E);
            func_0021F218(x + 8, y - 9, name->icon);
            textDrawStringAlt(x + 0x24, y + 5, name->text, MENU_PARTY_TEXT_COLOR);
            if (panel->flags & PARTY_PANEL_MARK_A) {
                packetSubmitRequest(0x13D, x + 0x1C, y - 0xC, 1);
            } else if (panel->flags & PARTY_PANEL_MARK_B) {
                packetSubmitRequest(0x13F, x + 0x1C, y - 0xC, 1);
            } else {
                packetSubmitRequest(0x13E, x + 0x1C, y - 0xC, 1);
            }
            if (panel->fade != 0) {
                fontObjSetOpacity((panel->opacity * panel->fade) >> MENU_PARTY_FADE_SHIFT);
                func_002253A8(x - 8, y - 0x12, 3);
            }
        }
        break;
    case 5:
    case 6:
        if (panel->flags & PARTY_PANEL_TALL) {
            panelDrawNineSlice(menuPanelFrame, x, y + 0x19, 0xAF, 0x27);
            if (panel->flags & PARTY_PANEL_ALT_FRAME) {
                packetSubmitRequest(0x121, x + 0x2F, y + 4, 0x11);
            } else {
                packetSubmitRequest(0x120, x + 0x2F, y + 4, 0x11);
            }
            func_0021EFC8(x + 6, y, name->icon);
            textDrawGlyphString(x + 0x32, y + 7, name->text, MENU_PARTY_TEXT_COLOR);
            func_002246A0(x + 0x88, y + 3, record->level);
            func_00224A28(x + 0x30, y + 0x1A, record->value5C, record->gauge);
            func_00224B40(x + 0x30, y + 0x2A, record->value60, record->value6);
            fontObjSetColor(MENU_PARTY_ICON_SHADOW);
            func_00224608(5, x + 0x30, y + 0x3D, MENU_PARTY_ICON_FILL);
            fontObjResetColor();
            textDrawDecimalAlt(x + 0x70, y + 0x41, values->value4, 8, MENU_PARTY_TEXT_COLOR);
            if (panel->fade != 0) {
                fontObjSetOpacity((panel->opacity * panel->fade) >> MENU_PARTY_FADE_SHIFT);
                func_002253A8(x - 9, y - 0xA, 2);
            }
        } else {
            func_0021D550(x, y, 0x5E);
            func_0021F218(x + 8, y - 9, name->icon);
            textDrawStringAlt(x + 0x24, y + 5, name->text, MENU_PARTY_TEXT_COLOR);
            if (panel->flags & PARTY_PANEL_MARK_A) {
                packetSubmitRequest(0x13D, x + 0x1C, y - 0xC, 1);
            } else if (panel->flags & PARTY_PANEL_MARK_B) {
                packetSubmitRequest(0x13F, x + 0x1C, y - 0xC, 1);
            } else {
                packetSubmitRequest(0x13E, x + 0x1C, y - 0xC, 1);
            }
            if (panel->fade != 0) {
                fontObjSetOpacity((panel->opacity * panel->fade) >> MENU_PARTY_FADE_SHIFT);
                func_002253A8(x - 8, y - 0x12, 3);
            }
        }
        break;
    case 8:
        panelDrawNineSlice(menuPanelFrame, x, y + 0x29, 0xAE, 0x27);
        if (panel->flags & PARTY_PANEL_ALT_FRAME) {
            packetSubmitRequest(0x121, x + 0x2F, y + 4, 0x11);
            packetSubmitRequest(0x115, x + 0x38, y + 0x19, 0x11);
        } else {
            packetSubmitRequest(0x120, x + 0x2F, y + 4, 0x11);
            packetSubmitRequest(0x114, x + 0x38, y + 0x19, 0x11);
        }
        menuDrawIndexedIcon(x + 6, y, name->icon);
        textDrawGlyphString(x + 0x32, y + 7, name->text, MENU_PARTY_TEXT_COLOR);
        func_002246A0(x + 0x88, y + 3, record->level);
        func_00224A28(x + 0x30, y + 0x2A, record->value5C, record->gauge);
        func_00224B40(x + 0x30, y + 0x3A, record->value60, record->value6);
        break;
    case 2:
    case 3:
    default:
        func_0021C6F8(x, y + 0xB, 0x9D, 0x47);
        if (panel->flags & PARTY_PANEL_ALT_FRAME) {
            packetSubmitRequest(0x121, x + 0x2F, y + 4, 0x11);
            packetSubmitRequest(0x115, x + 0x32, y + 0x1B, 0x11);
        } else {
            packetSubmitRequest(0x120, x + 0x2F, y + 4, 0x11);
            packetSubmitRequest(0x114, x + 0x32, y + 0x1B, 0x11);
        }
        menuDrawIndexedIcon(x + 6, y, name->icon);
        textDrawGlyphString(x + 0x34, y + 7, name->text, MENU_PARTY_TEXT_COLOR);
        func_002246A0(x + 0x77, y + 3, record->level);
        func_00224810(x + 0x30, y + 0x2B, record->value5C, record->gauge);
        func_00224928(x + 0x30, y + 0x3B, record->value60, record->value6);
        if (panel->fade != 0) {
            fontObjSetOpacity((panel->opacity * panel->fade) >> MENU_PARTY_FADE_SHIFT);
            func_002253A8(x - 9, y - 0xA, 2);
        }
        break;
    }
    fontObjResetOpacity();
}


INCLUDE_ASM("asm/main/nonmatchings/tu101", func_00225FE0);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_00226140);

INCLUDE_ASM("asm/main/nonmatchings/tu101", menuPlaceItemCursor);

INCLUDE_ASM("asm/main/nonmatchings/tu101", menuRefreshItems);

INCLUDE_ASM("asm/main/nonmatchings/tu101", menuDraw);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_00226948);

INCLUDE_ASM("asm/main/nonmatchings/tu101", menuSetMode);

typedef signed char s8;

typedef int s32;

/* A cursor menu. Partial layout: only the fields this TU's functions touch.
 * Item i uses the 0x40-byte stride at object + i * 0x40: its sub-objects sit at +0x90 and +0xAC of that
 * stride, its flags word at +0xC8 (0x1 selected, 0x8 disabled). cursor is the selected index, wrapped modulo
 * item_count; prev_cursor is the one before it.
 * flags: 0x4 locks the cursor. */
typedef struct MenuObject {
    s32 state_words[4];
    unsigned char unmodeled_10[0x1C - 0x10];
    struct FontTrack track_1C;
    struct FontTrack track_38;
    struct FontTrack track_54;
    unsigned char unmodeled_70[0x3CC - 0x70];
    s32 cursor;
    s32 prev_cursor;
    unsigned char unmodeled_3D4[4];
    s32 item_count;
    unsigned char unmodeled_3DC[8];
    s32 flags;
} MenuObject;

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
            fontTrackAdvance((struct FontTrack *) (element + 0x90));
            fontTrackAdvance((struct FontTrack *) (element + 0xAC));
            element += 0x40;
        } while (index < object->item_count);
    }
    menuPlaceItemCursor(object, object->cursor);
    fontTrackAdvance(&object->track_1C);
    fontTrackAdvance(&object->track_38);
    fontTrackAdvance(&object->track_54);
}

INCLUDE_ASM("asm/main/nonmatchings/tu101", menuSkipDisabled);

void seVoiceStartDefault(int);
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

INCLUDE_ASM("asm/main/nonmatchings/tu101", menuNavigate);

INCLUDE_ASM("asm/main/nonmatchings/tu101", menuSetSelected);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_00227408);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_00227458);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_002275D0);

void menuResetState(MenuObject *object) {
    if (object != 0) {
        object->state_words[0] = 0;
        object->state_words[1] = 0;
        object->state_words[2] = 0;
        object->state_words[3] = 0;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu101", msgRangeSetText);

/* A message being typed out in a window. msgRangeSetText stores the escape-coded text and counts its
 * visible glyphs into glyph_count; msgRangeDrawRevealed advances reveal_pos (starts at -8, a short lead-in)
 * and drawn (the token count its draw loop reaches) toward glyph_count as well. */
typedef struct MsgRange {
    unsigned char *text;
    int glyph_count;
    int reveal_pos;
    int drawn;
} MsgRange;

void textDrawSegment(int, int, void *, int);
extern unsigned char msgStyleInit[];
extern unsigned char D_0068ABF0[];
extern unsigned char msgStyleReset[];

/* Clears the reveal so the next draw starts again from the lead-in. */
void msgRangeReset(MsgRange *range)
{
    if (range != 0) {
        range->drawn = 0;
        range->reveal_pos = -8;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu101", msgRangeDrawIcon);

INCLUDE_ASM("asm/main/nonmatchings/tu101", msgRangeDrawRevealed);

/* Returns 1 once every glyph is revealed, 0 while some are still hidden. */
int msgRangeIsRevealed(const MsgRange *range)
{
    if (range->reveal_pos < range->glyph_count) {
        return 0;
    }
    return 1;
}

/* Draws the whole message at (x, y) between the style reset strings. */
void msgRangeDraw(MsgRange *range, int x, int y)
{
    if (range->text != 0) {
        textDrawSegment(0, 0, msgStyleInit, 0);
        textDrawSegment(0, 0, D_0068ABF0, 0);
        textDrawSegment(x, y, range->text, 0);
        textDrawSegment(0, 0, msgStyleReset, 0);
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu101", msgBufferAppend);

INCLUDE_ASM("asm/main/nonmatchings/tu101", choiceInit);

extern PadRecord PadData[2];

/* The two-choice (yes/no) prompt at main 0x006a57d8. +0x04 is the row the highlight is placed at (choiceUpdate
 * passes it to the placement call); +0x38 is the selected option (0 or 1, 2 once closed); the words at +0x3C
 * (msg buffer), +0x40 and +0x44 are the prompt text and the two option labels drawn by choiceDraw. */
typedef struct ChoiceState {
    unsigned char unmodeled_00[0x04];
    int id;
    unsigned char unmodeled_08[0x30];
    unsigned char state;
} ChoiceState;

extern ChoiceState choiceState;

/* The countdown object at main 0x006a5820. Its +0x04 word is the one
 * D_006A5824 names; main 0x00228438 reaches the record through that label.
 * countdownInit sets count, countdownTick counts it down (a pad button zeroes it), countdownDraw draws
 * value and text. */
typedef struct CountdownState {
    int count;
    int value;
    unsigned char unmodeled_08[0x18];
    unsigned char *text;
} CountdownState;

extern CountdownState countdownState;
extern struct FontTrack D_006A5824[];
extern int choiceFirstOptionText[];

/* Sets the text of the first option of the choice prompt (choiceState +0x40); choiceDraw draws it at x 0xB4. */
void choiceSetFirstOptionText(int text)
{
    choiceFirstOptionText[0] = text;
}

extern int choiceSecondOptionText[];

/* Sets the text of the second option of the choice prompt (choiceState +0x44); choiceDraw draws it at x 0x12C. */
void choiceSetSecondOptionText(int text)
{
    choiceSecondOptionText[0] = text;
}

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_002280E0);

int choiceUpdate(void)
{
    if (choiceState.state < 2) {
        if (PadData[0].directionRepeated & 0xA000) {
            seVoiceStartDefault(0x01000001);
            choiceState.state ^= 1;
            fontTrackSetTarget((struct FontTrack *) &choiceState, (signed char) choiceState.state * 0x78 + 0xA0, choiceState.id);
        }
        if (PadData[0].directionPressed & 0x40) {
            choiceState.state = 2;
            seVoiceStartDefault(0x01000008);
            return 2;
        }
        if (PadData[0].directionPressed & 0x20) {
            choiceState.state |= 0x10;
            seVoiceStartDefault(0x01000002);
            return 1;
        }
    }
    return 0;
}

INCLUDE_ASM("asm/main/nonmatchings/tu101", choiceDraw);

INCLUDE_ASM("asm/main/nonmatchings/tu101", func_00228340);

INCLUDE_ASM("asm/main/nonmatchings/tu101", countdownInit);

int countdownTick(void)
{
    if (countdownState.count > 0) {
        countdownState.count = countdownState.count - 1;
    }
    if (PadData[0].pressed & 0x60) {
        if (countdownState.count >= -1) {
            seVoiceStartDefault(0x01000002);
            countdownState.count = 0;
        }
    }
    return countdownState.count;
}

void textDrawSegment(int, int, void *, int);
void func_0021DE60(int, int, int, int);
void func_0021E338(int, int, int, int);
void xglFontFlush(void);
extern unsigned char msgStyleInit[];
extern unsigned char D_0068AC80[];

void countdownDraw(void)
{
    CountdownState *countdown;

    /* The record starts four bytes before D_006A5824, the word that the
     * two callees below receive by address. */
    countdown = (CountdownState *) ((unsigned char *) D_006A5824 - 4);
    fontTrackEase(D_006A5824);
    fontTrackEase(D_006A5824);
    fontObjSetOpacity(countdown->value);
    func_0021DE60(0x50, 0xB4, 0x160, 0x50);
    func_0021E338(0x54, 0xB8, 0x158, 0x48);
    textDrawSegment(0, 0, msgStyleInit, 0);
    textDrawSegment(0, 0, D_0068AC80, 0);
    if (countdown->text != 0) {
        textDrawSegment(0x60, 0xC4, countdown->text, 4);
    }
    fontObjResetOpacity();
    xglFontFlush();
    fontObjResetAndBuild();
}

extern signed char textCharSpacing;

/* Sets the extra advance added to every glyph of text. */
void textSetCharSpacing(signed char spacing)
{
    textCharSpacing = spacing;
}

/* Restores the default glyph spacing of 4. */
void textResetCharSpacing(void)
{
    textCharSpacing = 4;
}
