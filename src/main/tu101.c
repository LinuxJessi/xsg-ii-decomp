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

/* Font control tags textDrawSegment prints around a segment: the opening tag, the
 * colour tag whose two wave bytes it rewrites, and the alpha tag whose fourth byte it
 * rewrites. */
extern unsigned char D_0069D588[];
extern unsigned char D_0069D590[8];
extern unsigned char D_0069D598[4];
/* Closing tags of the colour and alpha tags. */
extern unsigned char D_0068A600[];
extern unsigned char D_0068A610[];

void xglFontPrint(int x, int y, unsigned int color, const char *text);
int xglTickWave(float divisor, float amplitude);
u8 fontObjGetOpacity(void);

/* One text segment: optional colour and alpha control tags, then the draw call.
 * flags bit 0 pulses the colour, bit 1 halves the alpha, bit 2 scales it by the font opacity. */
void textDrawSegment(int x, int y, void *text, int flags)
{
    unsigned char alpha;

    if (text == 0) {
        return;
    }
    xglFontPrint(0, 0, D_0069D580, D_0069D588);
    if (flags & 1) {
        D_0069D590[2] = D_0069D590[3] = xglTickWave(32.0f, 24.0f) + 0x68;
        xglFontPrint(0, 0, D_0069D580, D_0069D590);
    }
    alpha = (flags & 2) ? 0x40 : 0x80;
    if (flags & 4) {
        alpha = (alpha * fontObjGetOpacity()) >> 7;
    }
    if ((signed char)alpha >= 0) {
        D_0069D598[3] = alpha;
        xglFontPrint(0, 0, D_0069D580, D_0069D598);
    }
    xglFontPrint(x, y, D_0069D580, text);
    if (flags & 1) {
        xglFontPrint(0, 0, D_0069D580, D_0068A600);
    }
    if ((signed char)alpha >= 0) {
        xglFontPrint(0, 0, D_0069D580, D_0068A610);
    }
}

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
/* Returns nothing: textDrawDigits and func_0021BB10 reload packetRequest.first after
   the call instead of using a result. */
void fontObjDraw(PacketRequest *block);

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

/* Two textDrawGlyphString runs, each with its own up-left layer in `colour`: the first at
   (x + 1, y + 3) with the font colour 0x3F80000080000000 (RGB 0, alpha 0x80) as its face, the
   second at (x, y) with the previous font colour. */
void textDrawShadow(int x, int y, unsigned char *text, unsigned long long colour)
{
    unsigned long long prevColour;

    prevColour = fontObjGetColor();
    fontObjSetColor(0x3F80000080000000ULL);
    textDrawGlyphString(x + 1, y + 3, text, colour);
    fontObjSetColor(prevColour);
    textDrawGlyphString(x, y, text, colour);
}

/* Text run with the second glyph bank (kinds 0x7A to 0x7F): like textDrawGlyphString, the shadow
   in `colour` 2 pixels up-left, then the face in the current font colour. Small letters, digits
   and most symbols sit `drop` pixels lower. */
void textDrawStringAlt(int x, int y, char *text, u64 colour)
{
    unsigned long long fontColour = fontObjGetColor();
    unsigned char *p;
    int cell;
    int drop;
    PacketRequest *request;

    packetRequest.first = x;
    packetRequest.second = y;
    packetRequest.reset_value = 0;
    p = (unsigned char *) text;
    drop = 0;
    while (*p != 0) {
        if ((unsigned char) (*p - 'A') < 26) {
            packetRequest.command = 0x7B;
            cell = 0x7A;
            y = *p - 'A';
        } else if ((unsigned char) (*p - 'a') < 26) {
            packetRequest.command = 0x7D;
            cell = 0x7C;
            drop = 2;
            y = *p - 'a';
        } else if ((unsigned char) (*p - '0') < 10) {
            packetRequest.command = 0x7F;
            cell = 0x7E;
            drop = 2;
            y = *p - '0';
        } else {
            switch (*p) {
            case ' ':
                packetRequest.first += textCharSpacing;
                p++;
                continue;
            case '-':
                packetRequest.command = 0x7F;
                cell = 0x7E;
                y = 0xD;
                drop = 1;
                break;
            case '/':
                packetRequest.command = 0x7F;
                cell = 0x7E;
                y = GLYPH_INDEX_SLASH;
                drop = 2;
                break;
            case ':':
                packetRequest.command = 0x7F;
                cell = 0x7E;
                y = GLYPH_INDEX_COLON;
                drop = 2;
                break;
            case '.':
                packetRequest.command = 0x7D;
                cell = 0x7C;
                y = 0x1B;
                drop = 2;
                break;
            case '+':
                packetRequest.command = 0x7F;
                cell = 0x7E;
                y = 0xC;
                drop = 2;
                break;
            default:
                p++;
                continue;
            }
        }
        switch (*p) {
        case 'I':
        case 'l':
            packetRequest.first -= 1;
            break;
        case '.':
        case 'i':
            packetRequest.first -= 2;
            break;
        }
        request = &packetRequest;
        request->phase = y * fontObjCellOffsetX(request->command);
        request->first -= 2;
        request->second += drop - 2;
        fontObjSetColor(colour);
        request->flags = GLYPH_FLAGS_SHADOW;
        fontObjDraw(request);
        request->first += 2;
        request->second += 2;
        request->command = cell;
        request->phase = y * fontObjCellOffsetX(cell);
        fontObjSetColor(fontColour);
        request->flags = GLYPH_FLAGS_FACE;
        fontObjDraw(request);
        request->first += fontObjCellOffsetX(cell);
        if (cell == 0x7E) {
            request->first += 1;
        }
        request->second -= drop;
        drop = 0;
        switch (*p) {
        case '.':
            request->first -= 2;
            break;
        case 'I':
        case 'i':
        case 't':
            packetRequest.first -= 1;
            break;
        }
        p++;
    }
}

/* Shadowed text run over the second glyph bank. */
void textDrawShadowAlt(int x, int y, char *text, unsigned long long colour)
{
    unsigned long long prevColour;

    prevColour = fontObjGetColor();
    fontObjSetColor(0x3F80000080000000ULL);
    textDrawStringAlt(x + 1, y + 3, text, colour);
    fontObjSetColor(prevColour);
    textDrawStringAlt(x, y, text, colour);
}

/* Text run whose request flags and glyph kinds depend on windowLayer: flags 0x10034 when it is
   nonzero (func_0021E7B0 passes 1; its window pieces all carry the 0x10000 bit), else 0x74. Glyph kinds
   start at 0x2BA with windowLayer, else 0x18: capitals, then +1 small letters, +2 digits (the
   strip starts at '1'), +3 the symbols '-', '/' and '.' ('.' sets flag bit 0x2000 for its two
   draws). Each glyph is drawn in colour 0x3F80000060000000 one pixel right and two down, then in
   the reset colour. */
void func_0021AFE8(int windowLayer, int x, int y, char *text)
{
    unsigned char *p = (unsigned char *) text;
    int command;
    int glyph;

    packetRequest.first = x;
    packetRequest.second = y;
    packetRequest.flags = 0x74;
    packetRequest.reset_value = 0;
    if (windowLayer != 0) {
        packetRequest.flags = 0x10034;
    }
    while (*p != 0) {
        command = windowLayer ? 0x2BA : 0x18;
        packetRequest.command = command;
        if ((unsigned char) (*p - 'A') < 26) {
            glyph = *p - 'A';
        } else if ((unsigned char) (*p - 'a') < 26) {
            packetRequest.command = command + 1;
            glyph = *p - 'a';
        } else if ((unsigned char) (*p - '0') < 10) {
            packetRequest.command = command + 2;
            glyph = *p - '0' + 9;
            glyph %= 10; /* the strip starts at '1': '0' is its last cell, 9 */
        } else {
            /* default comes first: listed last, the '-' block is cross-jumped into the '/'
               tail, which the original does not do. */
            switch (*p) {
            default:
                p++;
                continue;
            case ' ':
                packetRequest.first += textCharSpacing;
                p++;
                continue;
            case '-':
                packetRequest.command = command + 3;
                glyph = 2;
                break;
            case '/':
                packetRequest.command = command + 3;
                glyph = 1;
                break;
            case '.':
                packetRequest.command = command + 3;
                glyph = 0;
                packetRequest.flags |= 0x2000;
                break;
            }
        }
        switch (*p) {
        case 'I':
        case 'i':
        case 'l':
            packetRequest.first -= 1;
            break;
        case 'm':
            packetRequest.first += 1;
            break;
        }
        packetRequest.phase = glyph * fontObjCellOffsetX(packetRequest.command);
        fontObjSetColor(0x3F80000060000000ULL);
        packetRequest.first += 1;
        packetRequest.second += 2;
        fontObjDraw(&packetRequest);
        fontObjResetColor();
        packetRequest.first -= 1;
        packetRequest.second -= 2;
        fontObjDraw(&packetRequest);
        packetRequest.flags &= ~0x2000;
        packetRequest.first += fontObjCellOffsetX(packetRequest.command) - 2;
        if ((unsigned char) (*p - '0') < 10) {
            packetRequest.first += 2;
        }
        if ((unsigned char) (*p - 'A') < 26) {
            packetRequest.first += 1;
        }
        switch (*p) {
        case 'I':
        case 'f':
        case 'i':
        case 'j':
        case 'l':
        case 'r':
        case 't':
            packetRequest.first -= 1;
            /* no break: these letters also take the +1 of case 'm' */
        case 'm':
            packetRequest.first += 1;
            break;
        }
        p++;
    }
}

extern char textDecimalBuffer[16];
extern const char textDigits[];

/* Formats an integer in decimal into a static buffer with width and zero-pad flag; returns the buffer. */
char *textFormatDecimal(int value, int digits, int zeroPad)
{
    int i;
    int done;

    textDecimalBuffer[digits] = 0;
    done = 0;
    for (i = digits - 1; i >= 0; i--) {
        if (done) {
            textDecimalBuffer[i] = zeroPad ? '0' : ' ';
            continue;
        }
        textDecimalBuffer[i] = textDigits[value % 10];
        value /= 10;
        done = value == 0;
    }
    return textDecimalBuffer;
}

/* Decimal number at (x, y): width 1 to 9, 9-pixel character spacing, space padding. */
void textDrawDecimal(int x, int y, int value, int digits, u64 colour)
{
    char *text;

    if (digits > 0) {
        if (digits < 10) {
            textCharSpacing = 9;
            text = textFormatDecimal(value, digits, 0);
            textDrawGlyphString(x, y, (unsigned char *) text, colour);
            textCharSpacing = 4;
        }
    }
}

/* textDrawDecimalAlt variant: second glyph bank, width up to 10, 7-pixel spacing. */
void textDrawDecimalAlt(int x, int y, int value, int digits, u64 colour)
{
    char *text;

    if (digits > 0) {
        if (digits < 11) {
            textCharSpacing = 7;
            text = textFormatDecimal(value, digits, 0);
            textDrawStringAlt(x, y, text, colour);
            textCharSpacing = 4;
        }
    }
}

/* textDrawDecimal with zero padding. */
void textDrawDecimalZero(int x, int y, int value, int digits, u64 colour)
{
    char *text;

    if (digits > 0) {
        if (digits < 10) {
            textCharSpacing = 9;
            text = textFormatDecimal(value, digits, 1);
            textDrawGlyphString(x, y, (unsigned char *) text, colour);
            textCharSpacing = 4;
        }
    }
}

/* Decimal number through the shadowed text run. */
void textDrawDecimalShadow(int x, int y, int value, int digits, unsigned long long color)
{
    char *text;

    if (digits > 0) {
        if (digits < 10) {
            textCharSpacing = 9;
            text = textFormatDecimal(value, digits, 0);
            textDrawShadow(x, y, (unsigned char *) text, color);
            textCharSpacing = 4;
        }
    }
}

/* textDrawDecimalAlt through the shadowed second-bank run. */
void textDrawDecimalAltShadow(int x, int y, int value, int digits, u64 colour)
{
    char *text;

    if (digits > 0) {
        if (digits < 11) {
            textCharSpacing = 7;
            text = textFormatDecimal(value, digits, 0);
            textDrawShadowAlt(x, y, text, colour);
            textCharSpacing = 4;
        }
    }
}

/* textDrawDecimalShadow with zero padding. */
void textDrawDecimalShadowZero(int x, int y, int value, int digits, u64 colour)
{
    char *text;

    if (digits > 0) {
        if (digits < 10) {
            textCharSpacing = 9;
            text = textFormatDecimal(value, digits, 1);
            textDrawShadow(x, y, (unsigned char *) text, colour);
            textCharSpacing = 4;
        }
    }
}

extern char textSignedDecimalBuffer[];

/* Signed decimal through the shadowed text run: width 1 to 9 plus a '+' or '-' in front of the
   digits, space padding, drawn in a fixed colour. The digits are textDigits[value % 10] of the
   value as passed (no magnitude is taken): only a value >= 0 indexes the "0123456789" table. */
void textDrawSignedDecimalShadow(int x, int y, int value, int digits, u64 colour)
{
    int sign;
    int i;
    int state;

    sign = value;
    if (digits > 0) {
        if (digits < 10) {
            digits++;
            textSignedDecimalBuffer[digits] = 0;
            state = 0;
            for (i = digits - 1; i >= 0; i--) {
                if (state != 0) {
                    if (state == 1) {
                        textSignedDecimalBuffer[i] = sign >= 0 ? '+' : '-';
                        state = 2;
                    } else {
                        textSignedDecimalBuffer[i] = ' ';
                    }
                } else {
                    textSignedDecimalBuffer[i] = textDigits[value % 10];
                    value /= 10;
                    state = value == 0;
                }
            }
            textCharSpacing = 9;
            fontObjSetColor(0x3F800000806C7F4BULL);
            textDrawShadow(x, y, (unsigned char *) textSignedDecimalBuffer, colour);
            fontObjResetColor();
            textCharSpacing = 4;
        }
    }
}

/* Decimal digits at (x, y), 9-pixel pitch, glyph kind 0x1D: drawn right to left, digits taken
   from the value by repeated division by 10; once the value is used up nothing more is drawn. */
void textDrawDigits(int x, int y, int value, int digits)
{
    int i;
    int digit;

    if (digits > 0) {
        packetRequest.first = x + digits * 9 - 9;
        packetRequest.second = y;
        packetRequest.command = 0x1D;
        packetRequest.flags = 0x60;
        packetRequest.reset_value = 0;
        for (i = 0; i < digits; i++) {
            if (value > 0) {
                digit = value % 10;
                value /= 10;
                packetRequest.phase = (digit + 9) % 10 * 9;
                fontObjDraw(&packetRequest);
                packetRequest.first -= 9;
            }
        }
    }
}

/* textDrawDigits without the stop at a used-up value: zeros are drawn up to the full width. */
void func_0021B8C8(int x, int y, int value, int digits)
{
    int i;
    int digit;

    if (digits > 0) {
        packetRequest.first = x + digits * 9 - 9;
        packetRequest.second = y;
        packetRequest.command = 0x1D;
        packetRequest.flags = 0x60;
        packetRequest.reset_value = 0;
        for (i = 0; i < digits; i++) {
            digit = value % 10;
            value /= 10;
            packetRequest.phase = (digit + 9) % 10 * 9;
            fontObjDraw(&packetRequest);
            packetRequest.first -= 9;
        }
    }
}

/* Two digit runs of the same width joined by one separator glyph (phase 0x63 of kind 0x1D). */
void textDrawDigitsPair(int x, int y, int first, int second, int digits)
{
    textDrawDigits(x, y, first, digits);
    x += digits * 9;
    packetRequest.phase = 0x63;
    packetRequest.command = 0x1D;
    packetRequest.flags = 0x60;
    packetRequest.first = x;
    packetRequest.second = y;
    packetRequest.reset_value = 0;
    fontObjDraw(&packetRequest);
    textDrawDigits(x + 9, y, second, digits);
}

/* Digit run in glyph kind 0x27B with a 12-pixel pitch, drawn one pixel higher; zeros up to the
   full width. */
void textDrawDigitsWide(int x, int y, int value, int digits)
{
    int i;
    int digit;

    if (digits > 0) {
        packetRequest.first = x + digits * 12 - 12;
        packetRequest.second = y - 1;
        packetRequest.command = 0x27B;
        packetRequest.flags = 0x21;
        packetRequest.reset_value = 0;
        for (i = 0; i < digits; i++) {
            digit = value % 10;
            value /= 10;
            packetRequest.phase = (digit + 9) % 10 * 17;
            fontObjDraw(&packetRequest);
            packetRequest.first -= 12;
        }
    }
}

/* Digit run in glyph kind 0x20C, drawn right to left from x + (count - 1) * 12; each digit's
   phase is ((digit + 9) % 10) * 0x11. */
void func_0021BB10(int x, int y, int value, int count) {
    int i;

    if (count > 0) {
        packetRequest.second = y;
        packetRequest.first = x + count * 12 - 12;
        packetRequest.command = 0x20C;
        packetRequest.flags = 0x21;
        packetRequest.reset_value = 0;
        for (i = 0; i < count; i++) {
            int digit = value % 10;

            value /= 10;
            packetRequest.phase = ((digit + 9) % 10) * 0x11;
            fontObjDraw(&packetRequest);
            packetRequest.first -= 12;
        }
    }
}

/* Two-layer glyph: a shadow of kind 0x46 (strip index * 15, flags 0x24) in `colour` at (x - 4, y - 2),
   then the face of kind 0x45 (strip index * 7, flags 0x64) at (x, y) in a tint picked from `colour`. */
void func_0021BBE8(int x, int y, int index, u64 colour) {
    u64 tint;

    tint = 0x3F80000080808080LL;
    if (colour == 0x3F8000008010104BLL) {
        tint = 0x3F80000080545B80LL;
    } else if (colour == 0x3F80000080641B15LL) {
        tint = 0x3F80000080805148LL;
    }
    packetRequest.first = x - 4;
    packetRequest.second = y - 2;
    packetRequest.command = 0x46;
    packetRequest.flags = GLYPH_FLAGS_SHADOW;
    packetRequest.phase = index * 15;
    packetRequest.reset_value = 0;
    fontObjSetColor(colour);
    fontObjDraw(&packetRequest);
    fontObjResetColor();
    packetRequest.phase = index * 7;
    packetRequest.command = 0x45;
    packetRequest.flags = GLYPH_FLAGS_FACE;
    packetRequest.first = x;
    packetRequest.second = y;
    packetRequest.reset_value = 0;
    fontObjSetColor(tint);
    fontObjDraw(&packetRequest);
    fontObjResetColor();
}

u16 fontObjCellOffsetY(int index);

/* Nine-piece frame from a style of nine sprite kinds in keypad order (kinds[6..8] top row,
   kinds[3..5] middle, kinds[0..2] bottom): corners at their cell size, edges stretched with
   flags 9, the centre (skipped when its kind is negative) with flags 0x109. */
void panelDrawNineSlice(void *style, int x, int y, int width, int height) {
    int *kinds = style;
    int innerWidth;
    int innerHeight;

    innerWidth = width - fontObjCellOffsetX(kinds[6]) - fontObjCellOffsetX(kinds[8]);
    innerHeight = height - fontObjCellOffsetY(kinds[6]) - fontObjCellOffsetY(kinds[0]);
    packetRequest.command = kinds[6];
    packetRequest.first = x;
    packetRequest.second = y;
    packetRequest.flags = 1;
    fontObjDraw(&packetRequest);
    packetRequest.first += fontObjCellOffsetX(packetRequest.command);
    packetRequest.command = kinds[7];
    packetRequest.fourth = innerWidth;
    packetRequest.fifth = fontObjCellOffsetY(packetRequest.command);
    packetRequest.flags = 9;
    if (innerWidth > 0) {
        fontObjDraw(&packetRequest);
    }
    packetRequest.first += innerWidth;
    packetRequest.command = kinds[8];
    packetRequest.flags = 1;
    fontObjDraw(&packetRequest);
    packetRequest.first = x;
    packetRequest.second += fontObjCellOffsetY(packetRequest.command);
    packetRequest.command = kinds[3];
    packetRequest.fourth = fontObjCellOffsetX(packetRequest.command);
    packetRequest.fifth = innerHeight;
    packetRequest.flags = 9;
    if (innerHeight > 0) {
        fontObjDraw(&packetRequest);
    }
    packetRequest.first += fontObjCellOffsetX(packetRequest.command);
    if (kinds[4] >= 0) {
        packetRequest.command = kinds[4];
        packetRequest.flags = 0x109;
        packetRequest.fourth = innerWidth;
        if (innerWidth > 0 && innerHeight > 0) {
            fontObjDraw(&packetRequest);
        }
    }
    packetRequest.first += innerWidth;
    packetRequest.command = kinds[5];
    packetRequest.fourth = fontObjCellOffsetX(packetRequest.command);
    packetRequest.flags = 9;
    if (innerHeight > 0) {
        fontObjDraw(&packetRequest);
    }
    packetRequest.first = x;
    packetRequest.second += innerHeight;
    packetRequest.command = kinds[0];
    packetRequest.flags = 1;
    fontObjDraw(&packetRequest);
    packetRequest.first += fontObjCellOffsetX(packetRequest.command);
    packetRequest.command = kinds[1];
    packetRequest.fourth = innerWidth;
    packetRequest.fifth = fontObjCellOffsetY(packetRequest.command);
    packetRequest.flags = 9;
    if (innerWidth > 0) {
        fontObjDraw(&packetRequest);
    }
    packetRequest.first += innerWidth;
    packetRequest.command = kinds[2];
    packetRequest.flags = 1;
    fontObjDraw(&packetRequest);
}

/* panelDrawNineSlice with flag bit 17 on the corners top-left and bottom-right, the edges and the centre. */
void func_0021BFA0(void *style, int x, int y, int width, int height) {
    int *kinds = style;
    int innerWidth;
    int innerHeight;

    innerWidth = width - fontObjCellOffsetX(kinds[6]) - fontObjCellOffsetX(kinds[8]);
    innerHeight = height - fontObjCellOffsetY(kinds[6]) - fontObjCellOffsetY(kinds[0]);
    packetRequest.command = kinds[6];
    packetRequest.first = x;
    packetRequest.second = y;
    packetRequest.flags = 0x20001;
    fontObjDraw(&packetRequest);
    packetRequest.first += fontObjCellOffsetX(packetRequest.command);
    packetRequest.command = kinds[7];
    packetRequest.fourth = innerWidth;
    packetRequest.fifth = fontObjCellOffsetY(packetRequest.command);
    packetRequest.flags = 0x20009;
    if (innerWidth > 0) {
        fontObjDraw(&packetRequest);
    }
    packetRequest.first += innerWidth;
    packetRequest.command = kinds[8];
    packetRequest.flags = 1;
    fontObjDraw(&packetRequest);
    packetRequest.first = x;
    packetRequest.second += fontObjCellOffsetY(packetRequest.command);
    packetRequest.command = kinds[3];
    packetRequest.fourth = fontObjCellOffsetX(packetRequest.command);
    packetRequest.fifth = innerHeight;
    packetRequest.flags = 0x20009;
    if (innerHeight > 0) {
        fontObjDraw(&packetRequest);
    }
    packetRequest.first += fontObjCellOffsetX(packetRequest.command);
    if (kinds[4] >= 0) {
        packetRequest.command = kinds[4];
        packetRequest.flags = 0x20109;
        packetRequest.fourth = innerWidth;
        if (innerWidth > 0 && innerHeight > 0) {
            fontObjDraw(&packetRequest);
        }
    }
    packetRequest.first += innerWidth;
    packetRequest.command = kinds[5];
    packetRequest.fourth = fontObjCellOffsetX(packetRequest.command);
    packetRequest.flags = 0x20009;
    if (innerHeight > 0) {
        fontObjDraw(&packetRequest);
    }
    packetRequest.first = x;
    packetRequest.second += innerHeight;
    packetRequest.command = kinds[0];
    packetRequest.flags = 1;
    fontObjDraw(&packetRequest);
    packetRequest.first += fontObjCellOffsetX(packetRequest.command);
    packetRequest.command = kinds[1];
    packetRequest.fourth = innerWidth;
    packetRequest.fifth = fontObjCellOffsetY(packetRequest.command);
    packetRequest.flags = 0x20009;
    if (innerWidth > 0) {
        fontObjDraw(&packetRequest);
    }
    packetRequest.first += innerWidth;
    packetRequest.command = kinds[2];
    packetRequest.flags = 0x20001;
    fontObjDraw(&packetRequest);
}

/* panelDrawNineSlice with the top-left corner drawn with flags 0xA11. */
void func_0021C228(void *style, int x, int y, int width, int height) {
    int *kinds = style;
    int innerWidth;
    int innerHeight;

    innerWidth = width - fontObjCellOffsetX(kinds[6]) - fontObjCellOffsetX(kinds[8]);
    innerHeight = height - fontObjCellOffsetY(kinds[6]) - fontObjCellOffsetY(kinds[0]);
    packetRequest.command = kinds[6];
    packetRequest.first = x;
    packetRequest.second = y;
    packetRequest.flags = 0xA11;
    fontObjDraw(&packetRequest);
    packetRequest.first += fontObjCellOffsetX(packetRequest.command);
    packetRequest.command = kinds[7];
    packetRequest.fourth = innerWidth;
    packetRequest.fifth = fontObjCellOffsetY(packetRequest.command);
    packetRequest.flags = 9;
    if (innerWidth > 0) {
        fontObjDraw(&packetRequest);
    }
    packetRequest.first += innerWidth;
    packetRequest.command = kinds[8];
    packetRequest.flags = 1;
    fontObjDraw(&packetRequest);
    packetRequest.first = x;
    packetRequest.second += fontObjCellOffsetY(packetRequest.command);
    packetRequest.command = kinds[3];
    packetRequest.fourth = fontObjCellOffsetX(packetRequest.command);
    packetRequest.fifth = innerHeight;
    packetRequest.flags = 9;
    if (innerHeight > 0) {
        fontObjDraw(&packetRequest);
    }
    packetRequest.first += fontObjCellOffsetX(packetRequest.command);
    if (kinds[4] >= 0) {
        packetRequest.command = kinds[4];
        packetRequest.flags = 0x109;
        packetRequest.fourth = innerWidth;
        if (innerWidth > 0 && innerHeight > 0) {
            fontObjDraw(&packetRequest);
        }
    }
    packetRequest.first += innerWidth;
    packetRequest.command = kinds[5];
    packetRequest.fourth = fontObjCellOffsetX(packetRequest.command);
    packetRequest.flags = 9;
    if (innerHeight > 0) {
        fontObjDraw(&packetRequest);
    }
    packetRequest.first = x;
    packetRequest.second += innerHeight;
    packetRequest.command = kinds[0];
    packetRequest.flags = 1;
    fontObjDraw(&packetRequest);
    packetRequest.first += fontObjCellOffsetX(packetRequest.command);
    packetRequest.command = kinds[1];
    packetRequest.fourth = innerWidth;
    packetRequest.fifth = fontObjCellOffsetY(packetRequest.command);
    packetRequest.flags = 9;
    if (innerWidth > 0) {
        fontObjDraw(&packetRequest);
    }
    packetRequest.first += innerWidth;
    packetRequest.command = kinds[2];
    packetRequest.flags = 1;
    fontObjDraw(&packetRequest);
}

/* panelDrawNineSlice with flags 0 on the corners, 8 on the edges and 0x108 on the centre. */
void func_0021C498(void *style, int x, int y, int width, int height) {
    int *kinds = style;
    int innerWidth;
    int innerHeight;

    innerWidth = width - fontObjCellOffsetX(kinds[6]) - fontObjCellOffsetX(kinds[8]);
    innerHeight = height - fontObjCellOffsetY(kinds[6]) - fontObjCellOffsetY(kinds[0]);
    packetRequest.command = kinds[6];
    packetRequest.first = x;
    packetRequest.second = y;
    packetRequest.flags = 0;
    fontObjDraw(&packetRequest);
    packetRequest.first += fontObjCellOffsetX(packetRequest.command);
    packetRequest.command = kinds[7];
    packetRequest.fourth = innerWidth;
    packetRequest.fifth = fontObjCellOffsetY(packetRequest.command);
    packetRequest.flags = 8;
    if (innerWidth > 0) {
        fontObjDraw(&packetRequest);
    }
    packetRequest.first += innerWidth;
    packetRequest.command = kinds[8];
    packetRequest.flags = 0;
    fontObjDraw(&packetRequest);
    packetRequest.first = x;
    packetRequest.second += fontObjCellOffsetY(packetRequest.command);
    packetRequest.command = kinds[3];
    packetRequest.fourth = fontObjCellOffsetX(packetRequest.command);
    packetRequest.fifth = innerHeight;
    packetRequest.flags = 8;
    if (innerHeight > 0) {
        fontObjDraw(&packetRequest);
    }
    packetRequest.first += fontObjCellOffsetX(packetRequest.command);
    if (kinds[4] >= 0) {
        packetRequest.command = kinds[4];
        packetRequest.flags = 0x108;
        packetRequest.fourth = innerWidth;
        if (innerWidth > 0 && innerHeight > 0) {
            fontObjDraw(&packetRequest);
        }
    }
    packetRequest.first += innerWidth;
    packetRequest.command = kinds[5];
    packetRequest.fourth = fontObjCellOffsetX(packetRequest.command);
    packetRequest.flags = 8;
    if (innerHeight > 0) {
        fontObjDraw(&packetRequest);
    }
    packetRequest.first = x;
    packetRequest.second += innerHeight;
    packetRequest.command = kinds[0];
    packetRequest.flags = 0;
    fontObjDraw(&packetRequest);
    packetRequest.first += fontObjCellOffsetX(packetRequest.command);
    packetRequest.command = kinds[1];
    packetRequest.fourth = innerWidth;
    packetRequest.fifth = fontObjCellOffsetY(packetRequest.command);
    packetRequest.flags = 8;
    if (innerWidth > 0) {
        fontObjDraw(&packetRequest);
    }
    packetRequest.first += innerWidth;
    packetRequest.command = kinds[2];
    packetRequest.flags = 0;
    fontObjDraw(&packetRequest);
}

/* Record group of eleven submits, sprite kinds 0x116 to 0x11F. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021C6F8);

/* Fixed-size badge at (x, y): kind 0x136 and the 0x54-wide strip 0x137 (flags 0x819) on the
   left, then from x + 0x64 a small frame of kinds 0x135, 0x11D, 0x11A, 0x117, 0x11E, 0x118 and
   0x11B (flags 1 for 0x135, 0x11E and 0x118, flags 9 for the strips). */
void func_0021CA40(int x, int y) {
    packetRequest.command = 0x136;
    packetRequest.second = y + 0xA;
    packetRequest.first = x;
    packetRequest.flags = 1;
    fontObjDraw(&packetRequest);
    packetRequest.command = 0x137;
    packetRequest.first = x + 0x10;
    packetRequest.fourth = 0x54;
    packetRequest.fifth = fontObjCellOffsetY(0x137);
    packetRequest.flags = 0x819;
    fontObjDraw(&packetRequest);
    packetRequest.first = x + 0x64;
    packetRequest.command = 0x135;
    packetRequest.second = y;
    packetRequest.flags = 1;
    fontObjDraw(&packetRequest);
    packetRequest.first += 0x28;
    packetRequest.command = 0x11D;
    packetRequest.fourth = 0x46;
    packetRequest.fifth = fontObjCellOffsetY(0x11D);
    packetRequest.flags = 9;
    fontObjDraw(&packetRequest);
    packetRequest.command = 0x11A;
    packetRequest.fourth = 0x6E;
    packetRequest.first = x + 0x64;
    packetRequest.second = y + 0x10;
    packetRequest.fifth = 3;
    fontObjDraw(&packetRequest);
    packetRequest.second = y + 0x13;
    packetRequest.command = 0x117;
    packetRequest.fifth = fontObjCellOffsetY(0x117);
    fontObjDraw(&packetRequest);
    packetRequest.command = 0x11E;
    packetRequest.first = x + 0xD2;
    packetRequest.second = y;
    packetRequest.flags = 1;
    fontObjDraw(&packetRequest);
    packetRequest.second = y + 0x13;
    packetRequest.command = 0x118;
    fontObjDraw(&packetRequest);
    packetRequest.second = y + 0x10;
    packetRequest.command = 0x11B;
    packetRequest.fourth = fontObjCellOffsetX(0x11B);
    packetRequest.fifth = 3;
    packetRequest.flags = 9;
    fontObjDraw(&packetRequest);
}

/* Record group of ten submits, sprite kinds 0x100 to 0x10A. */
INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021CC08);


/* Calls the tu104 routine menuDrawOutlineFrame and nothing else: the callers' four arguments pass straight through. */
void func_0021CEE0(int x, int y, int width, int height) {
    menuDrawOutlineFrame(x, y, width, height);
}

void func_0021E338(int x, int y, int width, int height);

/* Titled frame: func_0021E338's outline, a title bar (kinds 0x52 and 0x54 with flags 1, the
   0x53 strip with flags 9, then the 0x5A overlay with flags 0x118), then the optional title
   text at (x + 4, y - 1). */
void func_0021CEF8(int x, int y, int width, int height, void *text) {
    int barWidth;

    func_0021E338(x, y, width, height);
    packetRequest.command = 0x52;
    packetRequest.first = x;
    packetRequest.second = y;
    packetRequest.flags = 1;
    barWidth = width - 0x10;
    fontObjDraw(&packetRequest);
    packetRequest.command = 0x53;
    packetRequest.first += 8;
    packetRequest.fourth = barWidth;
    packetRequest.fifth = 0x10;
    packetRequest.flags = 9;
    fontObjDraw(&packetRequest);
    packetRequest.command = 0x54;
    packetRequest.first += barWidth;
    packetRequest.flags = 1;
    fontObjDraw(&packetRequest);
    packetRequest.command = 0x5A;
    packetRequest.first = x;
    packetRequest.second = y;
    packetRequest.fourth = width;
    packetRequest.fifth = 0x10;
    packetRequest.flags = 0x118;
    fontObjDraw(&packetRequest);
    if (text != 0) {
        func_0021AFE8(0, x + 4, y - 1, text);
    }
}

/* func_0021CEF8's title bar tinted 0x3F80000080800000 (flags 5 and 0xD), the tint reset before
   the title text at (x + 12, y - 1). */
void func_0021D048(int x, int y, int width, int height, void *text) {
    int barWidth;

    func_0021E338(x, y, width, height);
    fontObjSetColor(0x3F80000080800000LL);
    packetRequest.command = 0x52;
    packetRequest.first = x;
    packetRequest.second = y;
    packetRequest.flags = 5;
    barWidth = width - 0x10;
    fontObjDraw(&packetRequest);
    packetRequest.command = 0x53;
    packetRequest.first += 8;
    packetRequest.fourth = barWidth;
    packetRequest.fifth = 0x10;
    packetRequest.flags = 0xD;
    fontObjDraw(&packetRequest);
    packetRequest.command = 0x54;
    packetRequest.first += barWidth;
    packetRequest.flags = 5;
    fontObjDraw(&packetRequest);
    packetRequest.command = 0x5A;
    packetRequest.flags = 0x118;
    packetRequest.fourth = width;
    packetRequest.fifth = 0x10;
    packetRequest.first = x;
    packetRequest.second = y;
    fontObjDraw(&packetRequest);
    fontObjResetColor();
    if (text != 0) {
        func_0021AFE8(0, x + 0xC, y - 1, text);
    }
}

/* func_0021D048 with the tint 0x3F80000080000080. */
void func_0021D1B8(int x, int y, int width, int height, void *text) {
    int barWidth;

    func_0021E338(x, y, width, height);
    fontObjSetColor(0x3F80000080000080LL);
    packetRequest.command = 0x52;
    packetRequest.first = x;
    packetRequest.second = y;
    packetRequest.flags = 5;
    barWidth = width - 0x10;
    fontObjDraw(&packetRequest);
    packetRequest.command = 0x53;
    packetRequest.first += 8;
    packetRequest.fourth = barWidth;
    packetRequest.fifth = 0x10;
    packetRequest.flags = 0xD;
    fontObjDraw(&packetRequest);
    packetRequest.command = 0x54;
    packetRequest.first += barWidth;
    packetRequest.flags = 5;
    fontObjDraw(&packetRequest);
    packetRequest.command = 0x5A;
    packetRequest.flags = 0x118;
    packetRequest.fourth = width;
    packetRequest.fifth = 0x10;
    packetRequest.first = x;
    packetRequest.second = y;
    fontObjDraw(&packetRequest);
    fontObjResetColor();
    if (text != 0) {
        func_0021AFE8(0, x + 0xC, y - 1, text);
    }
}

/* Fixed layout of twelve pieces placed from (x, y): kinds 0x226 to 0x232 and 0x254; the
   stretched strips are drawn with flags 8 and 9. */
void func_0021D328(int x, int y) {
    packetRequest.command = 0x22E;
    packetRequest.second = y + 0x36;
    packetRequest.first = x;
    packetRequest.flags = 0;
    fontObjDraw(&packetRequest);
    packetRequest.flags = 1;
    packetRequest.command = 0x226;
    fontObjDraw(&packetRequest);
    packetRequest.command = 0x22F;
    packetRequest.fourth = 0x8E;
    packetRequest.first += 0x10;
    packetRequest.fifth = 0x10;
    packetRequest.flags = 8;
    fontObjDraw(&packetRequest);
    packetRequest.command = 0x254;
    packetRequest.first += 0x8E;
    packetRequest.second -= 4;
    packetRequest.flags = 0;
    fontObjDraw(&packetRequest);
    packetRequest.fourth = 0x9D;
    packetRequest.command = 0x22F;
    packetRequest.first += 0x20;
    packetRequest.second -= 0xD;
    packetRequest.fifth = 0x10;
    packetRequest.flags = 8;
    fontObjDraw(&packetRequest);
    packetRequest.flags = 0;
    packetRequest.command = 0x230;
    packetRequest.first += 0x9D;
    fontObjDraw(&packetRequest);
    packetRequest.command = 0x227;
    packetRequest.second = y + 0x10;
    packetRequest.fifth = 0x26;
    packetRequest.first = x;
    packetRequest.fourth = 0x10;
    packetRequest.flags = 9;
    fontObjDraw(&packetRequest);
    packetRequest.command = 0x231;
    packetRequest.fifth = 0x15;
    packetRequest.first += 0x15B;
    packetRequest.flags = 8;
    packetRequest.fourth = 0x10;
    fontObjDraw(&packetRequest);
    packetRequest.first = x;
    packetRequest.command = 0x228;
    packetRequest.second = y;
    packetRequest.flags = 1;
    fontObjDraw(&packetRequest);
    packetRequest.command = 0x229;
    packetRequest.fourth = 0x14B;
    packetRequest.first += 0x10;
    packetRequest.fifth = 0x10;
    packetRequest.flags = 9;
    fontObjDraw(&packetRequest);
    packetRequest.flags = 0;
    packetRequest.command = 0x232;
    packetRequest.first += 0x14B;
    fontObjDraw(&packetRequest);
    packetRequest.flags = 1;
    packetRequest.command = 0x22A;
    fontObjDraw(&packetRequest);
}

/* Three-record group (kinds 0x13A to 0x13C) at (x, y, width). */
INCLUDE_ASM("asm/main/nonmatchings/tu101", func_0021D550);

/* Border without fill: edge and corner pieces 0x23-0x2A; the tint is set first. */
void menuDrawBorder(int x, int y, int width, int height, unsigned long long color) {
    int bottomY;
    int innerX;
    int innerWidth;
    int innerHeight;

    bottomY = (y + height) - 0x10;
    innerX = x + 0x10;
    innerWidth = width - 0x20;
    fontObjSetColor(color);
    packetSubmitRequest(0x23, x, bottomY, 5);
    innerHeight = height - 0x20;
    packetSubmitRequestEx(0x24, innerX, bottomY, innerWidth, 0x10, 5);
    packetSubmitRequest(0x25, (x + width) - 0x10, bottomY, 5);
    packetSubmitRequestEx(0x26, x, y + 0x10, 0x10, innerHeight, 5);
    packetSubmitRequestEx(0x27, (x + width) - 0x10, y + 0x10, 0x10, innerHeight, 5);
    packetSubmitRequest(0x28, x, y, 5);
    packetSubmitRequestEx(0x29, innerX, y, innerWidth, 0x10, 5);
    packetSubmitRequest(0x2A, (x + width) - 0x10, y, 5);
    fontObjResetColor();
}

/* Panel frame: border and corner pieces 0x2F-0x35 with the fill in mode 0x105; the tint is set first. */
void menuDrawPanelFrame(int x, int y, int width, int height, unsigned long long color) {
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
    packetSubmitRequest(0x2F, x, bottomY, 5);
    innerHeight = height - 0x20;
    packetSubmitRequestEx(0x30, innerX, bottomY, innerWidth, 0x10, 5);
    packetSubmitRequest(0x31, (x + width) - 0x10, bottomY, 5);
    packetSubmitRequestEx(0x32, x, innerY, 0x10, innerHeight, 5);
    packetSubmitRequestEx(0x33, innerX, innerY, innerWidth, innerHeight, 0x105);
    packetSubmitRequestEx(0x32, (x + width) - 0x10, innerY, 0x10, innerHeight, 5);
    packetSubmitRequest(0x34, x, y, 5);
    packetSubmitRequestEx(0x30, innerX, y, innerWidth, 0x10, 5);
    packetSubmitRequest(0x35, (x + width) - 0x10, y, 5);
    fontObjResetColor();
}

/* Wide panel frame grown by 14 pixels on every side: 30-pixel pieces 0x38-0x40 with the fill
   in mode 0x105; the tint is set first. */
void func_0021D8B8(int x, int y, int width, int height, unsigned long long color) {
    int bottomY;
    int innerY;
    int innerX;
    int innerWidth;
    int innerHeight;

    fontObjSetColor(color);
    y -= 0xE;
    height += 0x1C;
    x -= 0xE;
    width += 0x1C;
    bottomY = (y + height) - 0x1E;
    innerY = y + 0x1E;
    innerX = x + 0x1E;
    innerWidth = width - 0x3C;
    packetSubmitRequest(0x38, x, bottomY, 5);
    innerHeight = height - 0x3C;
    packetSubmitRequestEx(0x39, innerX, bottomY, innerWidth, 0x1E, 5);
    packetSubmitRequest(0x3A, (x + width) - 0x1E, bottomY, 5);
    packetSubmitRequestEx(0x3B, x, innerY, 0x1E, innerHeight, 5);
    packetSubmitRequestEx(0x3C, innerX, innerY, innerWidth, innerHeight, 0x105);
    packetSubmitRequestEx(0x3D, (x + width) - 0x1E, innerY, 0x1E, innerHeight, 5);
    packetSubmitRequest(0x3E, x, y, 5);
    packetSubmitRequestEx(0x3F, innerX, y, innerWidth, 0x1E, 5);
    packetSubmitRequest(0x40, (x + width) - 0x1E, y, 5);
    fontObjResetColor();
}


/* Panel frame: border and corner pieces 0x2F-0x35 with the fill in mode 0x106; the tint is set first. */
/* color is a 64-bit RGBA+Q word, as in menuDrawPanel's 0x3F800000607F7F7F tint. */
void menuDrawPanelFrameAlt(int x, int y, int width, int height, unsigned long long color) {
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

/* Wide panel frame grown by 14 pixels on every side: 30-pixel pieces 0x38-0x40 with the fill
   in mode 0x106; the tint is set first. */
void func_0021DB98(int x, int y, int width, int height, unsigned long long color) {
    int bottomY;
    int innerY;
    int innerX;
    int innerWidth;
    int innerHeight;

    fontObjSetColor(color);
    y -= 0xE;
    height += 0x1C;
    x -= 0xE;
    width += 0x1C;
    bottomY = (y + height) - 0x1E;
    innerY = y + 0x1E;
    innerX = x + 0x1E;
    innerWidth = width - 0x3C;
    packetSubmitRequest(0x38, x, bottomY, 6);
    innerHeight = height - 0x3C;
    packetSubmitRequestEx(0x39, innerX, bottomY, innerWidth, 0x1E, 6);
    packetSubmitRequest(0x3A, (x + width) - 0x1E, bottomY, 6);
    packetSubmitRequestEx(0x3B, x, innerY, 0x1E, innerHeight, 6);
    packetSubmitRequestEx(0x3C, innerX, innerY, innerWidth, innerHeight, 0x106);
    packetSubmitRequestEx(0x3D, (x + width) - 0x1E, innerY, 0x1E, innerHeight, 6);
    packetSubmitRequest(0x3E, x, y, 6);
    packetSubmitRequestEx(0x3F, innerX, y, innerWidth, 0x1E, 6);
    packetSubmitRequest(0x40, (x + width) - 0x1E, y, 6);
    fontObjResetColor();
}

/* Tint words of the panel layers: the frame and the fill under the border. */
#define MENU_PANEL_FRAME_COLOR 0x3F800000507F7F7FLL
#define MENU_PANEL_FILL_COLOR 0x3F800000607F7F7FLL

/* Panel: frame in mode 5, fill in mode 6, then the border tinted `color`. */
void menuDrawPanel(int x, int y, int width, int height, unsigned long long color) {
    menuDrawPanelFrame(x, y, width, height, MENU_PANEL_FRAME_COLOR);
    menuDrawPanelFrameAlt(x, y, width, height, MENU_PANEL_FILL_COLOR);
    menuDrawBorder(x, y, width, height, color);
}

/* menuDrawPanel with the wide frames: func_0021D8B8 in mode 5, func_0021DB98 in mode 6, then the border. */
void func_0021DDB8(int x, int y, int width, int height, unsigned long long color) {
    func_0021D8B8(x, y, width, height, MENU_PANEL_FRAME_COLOR);
    func_0021DB98(x, y, width, height, MENU_PANEL_FILL_COLOR);
    menuDrawBorder(x, y, width, height, color);
}

float xglSin(float angle);

/* Small window frame (func_0021E7B0's layout): width and height lose 0x22 first; two
   passes of the corner and edge pieces 0xB2-0xB5 (an edge is skipped when its length is
   0), one pass of 0xB6-0xB9, then a band of 5-pixel rows 0xBA-0xBC whose alpha follows a
   half sine over the reduced height + 20. A narrow window (width <= 0x40) draws its 0xBA
   fill at half that alpha, then sets the colour again from the halved alpha doubled (its
   low bit lost) for the 0xBC ends. */
void func_0021DE60(int x, int y, int width, int height)
{
    int bandHeight;
    int i;
    int alpha;
    int offset;

    width -= 0x22;
    height -= 0x22;
    if (width < 0 || height < 0) {
        return;
    }
    for (i = 0; i < 2; i++) {
        packetSubmitRequest(0xB4, x - 0x10, y - 0x10, 0);
        packetSubmitRequest(0xB4, x + width + 0x10, y - 0x10, 0x200);
        packetSubmitRequest(0xB4, x - 0x10, y + height + 0x10, 0x400);
        packetSubmitRequest(0xB4, x + width + 0x10, y + height + 0x10, 0x600);
        if (width != 0) {
            packetSubmitRequestEx(0xB5, x + 0x10, y - 0x10, width, 0x20, 0x800);
            packetSubmitRequestEx(0xB5, x + 0x10, y + height + 0x10, width, 0x20, 0xC00);
        }
        if (height != 0) {
            packetSubmitRequestEx(0xB2, x - 0x10, y + 0x10, 0x20, height, 0x1000);
            packetSubmitRequestEx(0xB2, x + width + 0x10, y + 0x10, 0x20, height, 0x1200);
        }
        if (width != 0 && height != 0) {
            packetSubmitRequestEx(0xB3, x + 0x10, y + 0x10, width, height, 0x100);
        }
    }

    packetSubmitRequest(0xB8, x - 0x10, y - 0x10, 1);
    packetSubmitRequest(0xB8, x + width + 0x10, y - 0x10, 0x201);
    packetSubmitRequest(0xB8, x - 0x10, y + height + 0x10, 0x401);
    packetSubmitRequest(0xB8, x + width + 0x10, y + height + 0x10, 0x601);
    if (width != 0) {
        packetSubmitRequestEx(0xB9, x + 0x10, y - 0x10, width, 0x20, 0x801);
        packetSubmitRequestEx(0xB9, x + 0x10, y + height + 0x10, width, 0x20, 0xC01);
    }
    if (height != 0) {
        packetSubmitRequestEx(0xB6, x - 0x10, y + 0x10, 0x20, height, 0x1001);
        packetSubmitRequestEx(0xB6, x + width + 0x10, y + 0x10, 0x20, height, 0x1201);
    }
    if (width != 0 && height != 0) {
        packetSubmitRequestEx(0xB7, x + 0x10, y + 0x10, width, height, 0x101);
    }

    bandHeight = height + 20;
    for (i = 0; i < bandHeight; i += 5) {
        alpha = (int)(xglSin((float)i * 3.1415927f / (float)bandHeight) * 128.0f);
        fontObjSetColor(((u64)alpha << 24) | 0x3F80000000808080LL);
        alpha >>= 1;
        offset = 1;
        if (width > 0x40) {
            packetSubmitRequest(0xBB, x + 0x10, y + i + 6, 0x15);
            packetSubmitRequest(0xBB, x + width - 0x11, y + i + 6, 0x215);
            offset = 2;
            packetSubmitRequestEx(0xBA, x + 0x30, y + i + 6, width - 0x40, 5, 0x815);
        } else {
            fontObjSetColor(((u64)alpha << 24) | 0x3F80000000808080LL);
            packetSubmitRequestEx(0xBA, x + 0x10, y + i + 6, width, 5, 0x815);
            alpha <<= 1;
            fontObjSetColor(((u64)alpha << 24) | 0x3F80000000808080LL);
        }
        packetSubmitRequest(0xBC, x - 0xF, y + i + 6, 0x15);
        packetSubmitRequest(0xBC, x + width - offset + 0x11, y + i + 6, 0x215);
    }
    fontObjResetColor();
}

/* Short outline frame: 10-pixel top and bottom bars, pieces 0x69-0x70 with sides 0x4E and 0x58. */
void func_0021E338(int x, int y, int width, int height) {
    int rightX;
    int bottomY;
    int innerY;
    int innerHeight;
    int innerX;
    int innerWidth;

    rightX = (x + width) - 0x10;
    innerY = y + 0xA;
    bottomY = (y + height) - 0xA;
    packetSubmitRequest(0x6D, x, bottomY, 0);
    innerWidth = width - 0x20;
    innerX = x + 0x10;
    innerHeight = height - 0x14;
    packetSubmitRequestEx(0x6E, innerX, bottomY, innerWidth, 0xA, 0);
    packetSubmitRequest(0x6F, rightX, bottomY, 0);
    packetSubmitRequestEx(0x58, rightX, innerY, 0x10, innerHeight, 0);
    packetSubmitRequest(0x70, rightX, y, 0);
    packetSubmitRequest(0x69, x, bottomY, 1);
    packetSubmitRequestEx(0x4E, x, innerY, 0x10, innerHeight, 1);
    packetSubmitRequest(0x6A, x, y, 1);
    packetSubmitRequestEx(0x6B, innerX, y, innerWidth, 0xA, 1);
    packetSubmitRequest(0x6C, rightX, y, 1);
}

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

/* Window frame around a width x height area: four groups of corner, edge and fill
   pieces 0x2A1-0x2B2 (an edge is skipped when its length is 0), the title bar
   0x2B6-0x2B8 with the title text when there is one, then a band of 5-pixel rows
   0x2B3-0x2B5 whose alpha follows a half sine over height + 20 (a window no wider than
   0x40 draws its 0x2B3 fill at half that alpha, as in func_0021DE60). */
void func_0021E7B0(int x, int y, int width, int height, char *title)
{
    int bandHeight;
    int left;
    int top;
    int right;
    int i;
    int alpha;
    int offset;

    if (width < 0 || height < 0) {
        return;
    }
    left = x - 0x10;
    top = y - 0x10;
    right = x + width;

    packetSubmitRequest(0x2A3, left, top, 0x10000);
    packetSubmitRequest(0x2A3, x + width + 0x10, top, 0x10200);
    packetSubmitRequest(0x2A3, left, y + height + 0x10, 0x10400);
    packetSubmitRequest(0x2A3, x + width + 0x10, y + height + 0x10, 0x10600);
    if (width != 0) {
        packetSubmitRequestEx(0x2A4, x + 0x10, top, width, 0x20, 0x10800);
        packetSubmitRequestEx(0x2A4, x + 0x10, y + height + 0x10, width, 0x20, 0x10C00);
    }
    if (height != 0) {
        packetSubmitRequestEx(0x2A1, left, y + 0x10, 0x20, height, 0x11000);
        packetSubmitRequestEx(0x2A1, x + width + 0x10, y + 0x10, 0x20, height, 0x11200);
    }
    if (width != 0 && height != 0) {
        packetSubmitRequestEx(0x2A2, x + 0x10, y + 0x10, width, height, 0x10100);
    }

    packetSubmitRequest(0x2B1, left, top, 0x10001);
    packetSubmitRequest(0x2B1, x + width + 0x10, top, 0x10201);
    packetSubmitRequest(0x2B1, left, y + height + 0x10, 0x10401);
    packetSubmitRequest(0x2B1, x + width + 0x10, y + height + 0x10, 0x10601);
    if (width != 0) {
        packetSubmitRequestEx(0x2B2, x + 0x10, top, width, 0x20, 0x10801);
        packetSubmitRequestEx(0x2B2, x + 0x10, y + height + 0x10, width, 0x20, 0x10C01);
    }
    if (height != 0) {
        packetSubmitRequestEx(0x2AF, left, y + 0x10, 0x20, height, 0x11001);
        packetSubmitRequestEx(0x2AF, x + width + 0x10, y + 0x10, 0x20, height, 0x11201);
    }
    if (width != 0 && height != 0) {
        packetSubmitRequestEx(0x2B0, x + 0x10, y + 0x10, width, height, 0x10101);
    }

    packetSubmitRequest(0x2A9, x + width + 0x10, top, 0x10000);
    packetSubmitRequest(0x2A5, left, y + height + 0x10, 0x10000);
    packetSubmitRequest(0x2A7, x + width + 0x10, y + height + 0x10, 0x10000);
    if (width != 0) {
        packetSubmitRequestEx(0x2A6, x + 0x10, y + height + 0x10, width, 0x20, 0x10800);
    }
    if (height != 0) {
        packetSubmitRequestEx(0x2A8, x + width + 0x10, y + 0x10, 0x20, height, 0x11000);
    }

    packetSubmitRequest(0x2AC, left, top, 0x10001);
    packetSubmitRequest(0x2AE, x + width + 0x10, top, 0x10001);
    packetSubmitRequest(0x2AA, left, y + height + 0x10, 0x10001);
    if (width != 0) {
        packetSubmitRequestEx(0x2AD, x + 0x10, top, width, 0x20, 0x10801);
    }
    if (height != 0) {
        packetSubmitRequestEx(0x2AB, left, y + 0x10, 0x20, height, 0x11001);
    }

    if (title != 0) {
        packetSubmitRequest(0x2B6, left, top, 0x10001);
        packetSubmitRequest(0x2B8, x + width + 0x10, top, 0x10001);
        if (width != 0) {
            packetSubmitRequestEx(0x2B7, x + 0x10, top, width, 0x20, 0x10801);
        }
        func_0021AFE8(1, x + 0x10, y + 2, title);
    }

    bandHeight = height + 20;
    for (i = 0; i < bandHeight; i += 5) {
        alpha = (int)(xglSin((float)i * 3.1415927f / (float)bandHeight) * 128.0f);
        fontObjSetColor(((u64)alpha << 24) | 0x3F80000000808080LL);
        alpha >>= 1;
        offset = 1;
        if (width > 0x40) {
            packetSubmitRequest(0x2B4, x + 0x10, y + i + 6, 0x10015);
            packetSubmitRequest(0x2B4, right - 0x11, y + i + 6, 0x10215);
            offset = 2;
            packetSubmitRequestEx(0x2B3, x + 0x30, y + i + 6, width - 0x40, 5, 0x10815);
        } else {
            fontObjSetColor(((u64)alpha << 24) | 0x3F80000000808080LL);
            packetSubmitRequestEx(0x2B3, x + 0x10, y + i + 6, width, 5, 0x10815);
            alpha <<= 1;
            fontObjSetColor(((u64)alpha << 24) | 0x3F80000000808080LL);
        }
        packetSubmitRequest(0x2B5, left, y + i + 6, 0x10015);
        packetSubmitRequest(0x2B5, right - offset + 0x10, y + i + 6, 0x10215);
    }
    fontObjResetColor();
}

/* Scroll frame for a count-sized panel: the frame pieces shift with offset = (count-4)*16,
   and the body gets one 0xE8 fill strip per entry. */
void func_0021EE68(int count)
{
    int offset = (count - 4) << 4;
    int negOffset;
    int y;
    int n;

    packetSubmitRequestEx(0xE5, 0, 0x183 - offset, 0x200, offset + 0x32, 0x101);
    packetSubmitRequest(0xE9, 0, 0x1BD, 1);
    packetSubmitRequest(0xE7, 0, 0x155 - offset, 1);
    if (count > 0) {
        negOffset = -offset;
        y = negOffset + 0x17D;
        n = count;
        do {
            packetSubmitRequest(0xE8, 0, y, 1);
            y += 0x10;
            n--;
        } while (n != 0);
    }
    packetSubmitRequestEx(0xEA, 0, 0x1B3, 0x200, 0x10, 0x801);
    packetSubmitRequestEx(0xEA, 0, 0x183 - offset, 0x200, 0x10, 0x801);
}


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
void func_0021CC08(int, int, int, int);
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

/* Party split state, same values as tu080.c/tu081.c: PARTY_SPLIT_ON while the split
   is open. ptState is read through this view: splitMode, the member counts and the
   member the menu shows. */
enum PartySplitState {
    PARTY_SPLIT_OFF = 0,
    PARTY_SPLIT_ON = 1
};

/* The record behind a member's status pointer (tu110.c's PartyMemberStatus): value is drawn
   as the member's icon here. */
typedef struct PartyMemberStatus {
    unsigned char unmodeled_00[8];
    unsigned char value;
    unsigned char unmodeled_09[0xC - 0x09];
} PartyMemberStatus;

/* The block behind a member's limits pointer: experience and the two point limits. */
struct ControlLimits {
    int total_exp;                   /* drawn beside "Total Exp" (menuDrawCharStats) */
    int next_level_exp;              /* drawn beside "Next Lv" */
    int limit_a;
    int limit_b;
};

struct ControlBuffer {
    unsigned char header[4];
    unsigned char cells[4][8];
    unsigned char flags[2];
    unsigned char entries[4];   /* 0x26: menuListContains searches entries[0 .. entry_count) */
    unsigned char entry_count;  /* 0x2A */
};

/* One party member record: the type of ptState's slots, of the context func_0022A560
   (defined in main/tu110) returns and of the holder argument of the selection functions
   (tu110.c's PartyMember, tu109.c's SelectionHolder). */
struct ControlContext {
    int id;                          /* character id, looked up with func_A8B490 */
    PartyMemberStatus *status;
    struct ControlLimits *limits;
    struct ControlBuffer *buffer;
};

/* tu110's progress record: only playTime is read here (menuDrawPlayTime). */
typedef struct PartyProgress {
    unsigned short id;
    unsigned int playTime;
} PartyProgress;

typedef struct PtPartyView {
    PartyProgress *progress;
    struct ControlContext *slots[10];   /* 0-6 front, 7-9 back */
    struct ControlContext *roster[10];  /* second roster (tu110.c PartyState.roster) */
    unsigned char splitMode;
    unsigned char frontIndex;
    unsigned char backIndex;
    unsigned char counts[3];
    unsigned char menuSlot;         /* 0x5A: the member the menu shows (tu110.c PartyState.menuSlot),
                                       func_00220550's highlighted tab */
} PtPartyView;

extern PtPartyView ptState;

/* Icon at (x, y) (menuDrawIndexedIcon) plus the 0x138 mark when it is the current
   member's: the back slot in split mode, the front slot otherwise. */
void func_0021EFC8(int x, int y, int icon)
{
    menuDrawIndexedIcon(x, y, icon);
    if (ptState.splitMode == PARTY_SPLIT_ON) {
        if (ptState.slots[ptState.backIndex + 7]->status->value == icon) {
            packetSubmitRequest(0x138, x - 3, y - 3, 0x10);
        }
    } else {
        if (ptState.slots[ptState.frontIndex]->status->value == icon) {
            packetSubmitRequest(0x138, x - 3, y - 3, 0x10);
        }
    }
}

/* Icon at (x, y): the backing piece 0x158, then menuIconKinds[icon] + 1. */
void func_0021F0A0(int x, int y, int icon)
{
    int kind = menuIconKinds[icon] + 1;

    packetSubmitRequest(0x158, x, y, 0x10);
    packetSubmitRequest(kind, x, y, 0x11);
}

/* Wide icon at (x, y): the backing piece 0x158, then menuIconKinds[icon] + 1, both 0x16 x 0x14. */
void func_0021F110(int x, int y, int icon)
{
    int kind = menuIconKinds[icon] + 1;

    packetSubmitRequestEx(0x158, x, y, 0x16, 0x14, 0x10);
    packetSubmitRequestEx(kind, x, y, 0x16, 0x14, 1);
}

/* Like func_0021F0A0 with a fixed colour set for both pieces. */
void func_0021F190(int x, int y, int icon)
{
    int kind = menuIconKinds[icon] + 1;

    fontObjSetColor(0x3F80000080404040LL);
    packetSubmitRequest(0x158, x, y, 0x14);
    packetSubmitRequest(kind, x, y, 0x15);
    fontObjResetColor();
}

/* Lit icon at (x, y) (func_0021F0A0) plus the 0x138 mark when it is the current
   member's: the back slot in split mode, the front slot otherwise. */
void func_0021F218(int x, int y, int icon)
{
    func_0021F0A0(x, y, icon);
    if (ptState.splitMode == PARTY_SPLIT_ON) {
        if (ptState.slots[ptState.backIndex + 7]->status->value == icon) {
            packetSubmitRequest(0x138, x - 3, y - 3, 0x10);
        }
    } else {
        if (ptState.slots[ptState.frontIndex]->status->value == icon) {
            packetSubmitRequest(0x138, x - 3, y - 3, 0x10);
        }
    }
}

/* Cursor placement: (0, 0, 0) resets menuCursorPos and menuCursorSize to zero; while their
   targets are still zero both are set straight to (x, y + 10) and (width, 0), a flat box on
   the row's centre line; otherwise, with blink set, the targets become (x, y) and
   (width, 0x14), and without blink nothing moves. */
void func_0021F2F0(int x, int y, int width, int blink)
{
    int blinkVal = blink ? 0x14 : 0;

    if (x == 0 && y == 0 && width == 0) {
        fontTrackSetBoth(menuCursorPos, 0, 0);
        fontTrackSetBoth(menuCursorSize, 0, 0);
        return;
    }
    if (menuCursorPos[2] == 0 && menuCursorPos[3] == 0 && menuCursorSize[2] == 0) {
        fontTrackSetBoth(menuCursorPos, x, y + 0xA);
        fontTrackSetBoth(menuCursorSize, width, 0);
        return;
    }
    if (blinkVal != 0) {
        fontTrackSetTarget(menuCursorPos, x, y - (blinkVal >> 1) + 0xA);
        fontTrackSetTarget(menuCursorSize, width, blinkVal);
        return;
    }
}

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

/* Four page tabs 0x15D+page; the tab of selected's page (selected / 8) opens into its
   cells instead: 8 per page (4 on the last), 0x12F at selected and 0x15C elsewhere. */
void func_0021F530(int x, int y, int selected)
{
    int page;
    int cell;
    int i;

    for (page = 0; page < 4; page++) {
        if (page == selected >> 3) {
            cell = page * 8;
            for (i = 0; i < (page < 3 ? 8 : 4); i++) {
                if (cell == selected) {
                    x -= 4;
                    packetSubmitRequest(0x12F, x, y, 1);
                    x += 0x15;
                } else {
                    packetSubmitRequest(0x15C, x, y + 0xC, 1);
                    x += 7;
                }
                cell++;
            }
        } else {
            packetSubmitRequest(0x15D + page, x, y + 4, 1);
            x += 0xE;
        }
    }
}

/* Row of count cells, 0x12F at selected; with more than wideCellLimit cells they shrink to
   0x15C and, from 15 cells on, wrap after cell 13 into a second row. 0x1C cells draw as
   func_0021F530's page tabs. */
void func_0021F658(int x, int y, int selected, int count, int wideCellLimit)
{
    int wrapX;
    int i;

    if (count == 0x1C) {
        func_0021F530(x, y, selected);
        return;
    }
    wrapX = -99999;
    if (count >= 15 && wideCellLimit < count) {
        wrapX = x;
        y -= 8;
    }
    for (i = 0; i < count; i++) {
        if (i == selected) {
            x -= 4;
            x -= wideCellLimit < count;
            packetSubmitRequest(0x12F, x, y, 1);
            x += 0x18;
            if (wideCellLimit < count) {
                x -= 3;
            }
        } else if (wideCellLimit < count) {
            packetSubmitRequest(0x15C, x, y + 0xC, 1);
            x += 7;
        } else {
            packetSubmitRequest(0x15B, x, y + 0xC, 1);
            x += 0xC;
        }
        if (wrapX != -99999 && i == 13) {
            x = wrapX;
            y += 8;
        }
    }
}

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

void func_00224608(int kind, int x, int y, u64 colour);
int func_002299B0(struct ControlContext *holder, int index);
int selectionFindIndex(struct ControlContext *holder, int value);

/* Front-party member icons at x - count*0xC + i*0x18: lit (func_0021F0A0) when
   func_002299B0 approves the entry, dimmed (func_0021F190) otherwise; a found
   selectionFindIndex match adds the 7 mark. */
void func_0021F8A8(int x, int y, int record)
{
    int offset;
    int memberX;
    int i = 0;
    struct ControlContext *member;

    if (ptState.counts[1] != 0) {
        offset = 0;
        do {
            member = ptState.slots[i];
            memberX = (x - ptState.counts[1] * 0xC) + offset;
            offset += 0x18;
            if (func_002299B0(member, record)) {
                func_0021F0A0(memberX, y, member->status->value);
            } else {
                func_0021F190(memberX, y, member->status->value);
            }
            if (selectionFindIndex(member, record) != -1) {
                fontObjSetColor(0x3F8000008072804FLL);
                func_00224608(7, memberX + 2, y - 0x12, 0x3F800000802B2900LL);
                fontObjResetColor();
            }
            i++;
        } while (i < ptState.counts[1]);
    }
}

extern unsigned char D_0068A9B0[];
extern unsigned char D_0069D608[];
extern unsigned char D_0069D610[];
extern unsigned char D_0069D618[];
extern void *D_003EB3C8[];

/* Character info line: label, then (mode>0) the group name and (mode>=2) the slot
   name; group>=3 and slot>=4 with mode 2 draws a single "none" segment instead. */
void menuDrawCharInfo(int x, int y, int group, int slot, int mode)
{
    textDrawSegment(x, y, D_0068A600, 0);
    if (group >= 3 && slot >= 4 && mode == 2) {
        textDrawSegment(x + 6, y, D_0069D608, 0);
        return;
    }
    textDrawSegment(x, y, D_0068A9B0, 0);
    if (mode <= 0) {
        return;
    }
    textDrawSegment(x + 6, y, D_0069D610, 0);
    textDrawSegment(x + 0x30, y, D_003EB3C8[group], 0);
    textDrawSegment(x + 0x3A, y, D_0068A9B0, 0);
    if (mode >= 2) {
        textDrawSegment(x + 0x40, y, D_0069D618, 0);
        textDrawSegment(x + 0x66, y, D_003EB3C8[4 + slot], 0);
        textDrawSegment(x + 0x76, y, D_0068A9B0, 0);
    }
}

/* A scrolling list cursor: selected row, row count and the blink flag. */
typedef struct LicenseCursor {
    int index;
    int unmodeled_04;
    unsigned int count;
    int unmodeled_0c;
    int flags;
} LicenseCursor;

int cursorBufferGetAlignedOffset(LicenseCursor *cursor);
extern unsigned char D_0069D620[];
unsigned int cursorBufferGetIndex(LicenseCursor *cursor);
int func_0020F230(LicenseCursor *cursor);
/* OV01 dataBoxChkCategory1 (config/symbols/ov01.txt). */
int func_A9C7A8(int value);
void *func_A9B8D8(int id, int unused);
int selectionEntryIsEligible(int index);
extern u8 selectionWindowCount;
extern unsigned char selectionWindowList[];
extern unsigned char D_0069D628[];
extern unsigned char msgStyleInit[];
#define LICENSE_COLOR_IDLE 0x3F80000000000000LL
#define LICENSE_COLOR_SELECTED 0x3F80000080120B50LL

/* Item window: up to ten rows of selectionWindowList from the cursor's first visible
   row, each with its count (func_A9C7A8) and name (func_A9B8D8); ineligible entries
   are drawn dimmed. */
void menuDrawItemList(int x, int y, LicenseCursor *cursor)
{
    int firstRow = cursorBufferGetAlignedOffset(cursor);
    int page = cursorBufferGetIndex(cursor);
    int i;
    int rowStyle;

    menuDrawListWindow(x, y, page, func_0020F230(cursor));
    func_0021CEF8(x + 8, y + 0x20, 0xA1, 0xDF, D_0069D620);
    func_0021CEF8(x + 0xAB, y + 0x20, 0x2D, 0xDF, D_0069D628);
    packetSubmitRequest(0x12B, x + 0x43, y + 4, 1);
    textDrawSegment(0, 0, msgStyleInit, 0);
    for (i = 0; i < 10 && i + firstRow < selectionWindowCount; i++) {
        rowStyle = firstRow + i == cursor->index;
        textDrawDecimalShadow(x + 0xB9, y + 0x32 + i * 0x14, func_A9C7A8(selectionWindowList[firstRow + i]), 2,
                              rowStyle ? LICENSE_COLOR_SELECTED : LICENSE_COLOR_IDLE);
        if (!selectionEntryIsEligible(firstRow + i)) {
            rowStyle |= 2;
        }
        textDrawSegment(x + 0x18, y + 0x30 + i * 0x14, func_A9B8D8(selectionWindowList[firstRow + i], 0), rowStyle);
    }
    if (cursor->flags & 1) {
        func_0021F2F0(x + 0xA, y + 0x30 + (cursor->index - firstRow) * 0x14, 0x9D, cursor->flags & 1);
        menuDrawCursor();
    }
}

/* OV01 dataBoxChkCategory3 (config/symbols/ov01.txt). */
int func_A9C868(int id);
void *func_A9B9C8(int id, int unused);
int selectionBlockTestBit(int slot);

/* Two-column item window: up to 18 rows (9 per column) from the cursor's first visible
   row; candidates whose selectionBlockTestBit bit is clear show entry 0x190 instead
   of their name and count. */
void menuDrawItemListDetail(int x, int y, LicenseCursor *cursor)
{
    int firstRow = cursorBufferGetAlignedOffset(cursor);
    int i;
    int columnX;
    int rowY;
    int count;
    int rowStyle;

    func_0021CC08(x, y, 0x1C2, 0x11F);
    packetSubmitRequest(0x1C, x + 0x176, y + 0x10C, 0x40);
    textDrawDigits(x + 0x1A2, y + 0x10C, cursorBufferGetIndex(cursor) + 1, 2);
    func_0021F658(x + 0xC, y + 0x101, cursorBufferGetIndex(cursor), func_0020F230(cursor), 0x14);
    menuDrawBorder(x + 8, y + 0x20, 0xD8, 0xDF, 0x3F80000080381A1ALL);
    func_0021CEF8(x + 0xC, y + 0x25, 0xA1, 0xD5, D_0069D620);
    func_0021CEF8(x + 0xAF, y + 0x25, 0x2D, 0xD5, D_0069D628);
    menuDrawBorder(x + 0xE2, y + 0x20, 0xD8, 0xDF, 0x3F80000080381A1ALL);
    func_0021CEF8(x + 0xE6, y + 0x25, 0xA1, 0xD5, D_0069D620);
    func_0021CEF8(x + 0x189, y + 0x25, 0x2D, 0xD5, D_0069D628);
    packetSubmitRequest(0x12B, x + 0xB4, y + 4, 1);
    textDrawSegment(0, 0, msgStyleInit, 0);
    for (i = 0; i < 18 && i + firstRow < cursor->count; i++) {
        columnX = x + (i / 9) * 0xDA;
        rowY = y + (i % 9) * 0x15 + 0x36;
        count = func_A9C868(firstRow + i + 1);
        rowStyle = firstRow + i == cursor->index;
        if (selectionBlockTestBit(firstRow + i + 1)) {
            textDrawDecimalShadow(columnX + 0xBD, rowY + 2, count, 2,
                                  rowStyle ? LICENSE_COLOR_SELECTED : LICENSE_COLOR_IDLE);
            textDrawSegment(columnX + 0x12, rowY, func_A9B9C8(firstRow + i + 1, 0), rowStyle);
        } else {
            textDrawSegment(columnX + 0x12, rowY, menuMainTxdEntry(0x190), rowStyle);
        }
    }
    if (cursor->flags & 1) {
        func_0021F2F0(x + ((cursor->index - firstRow) / 9) * 0xDA + 0xE,
                      y + ((cursor->index - firstRow) % 9) * 0x15 + 0x36, 0x9D, cursor->flags & 1);
        menuDrawCursor();
    }
}

/* 1 when item is one of the member's buffer entries. */
int menuListContains(struct ControlContext *member, unsigned char item)
{
    int found;
    int i;

    found = 0;
    for (i = 0; i < member->buffer->entry_count; i++) {
        if (member->buffer->entries[i] == item) {
            found = 1;
            break;
        }
    }
    return found;
}

typedef struct LicenseRecord {
    unsigned char kind;
    unsigned char dispatch_value;
    unsigned short value;
    unsigned char bit;
    unsigned char unmodeled_05;
} LicenseRecord;

/* Admitted aligned(1) scalar: the gauge word sits at the unaligned offset 2 of the record. */
typedef signed int UnalignedS32 __attribute__((aligned(1)));

/* The character record returned by func_A8B490 (OV01). */
typedef struct MenuPartyRecord {
    unsigned char unmodeled_00[2];
    UnalignedS32 gauge;                     /* 0x02 */
    short value6;                           /* 0x06 */
    short strength;                         /* 0x08: "Str" ("Pow" in split mode) */
    short vitality;                         /* 0x0A: "Vit" ("Arm" in split mode) */
    short etherAttack;                      /* 0x0C: "Eatk" */
    short etherDefense;                     /* 0x0E: "Edef" */
    signed char dexterity;                  /* 0x10: "Dex" */
    signed char evasion;                    /* 0x11: "Eva" */
    signed char agility;                    /* 0x12: "Agl" */
    signed char level;                      /* 0x13 */
    unsigned char unmodeled_14[0x48];
    int value5C;                            /* 0x5C */
    short value60;                          /* 0x60 */
    unsigned char unmodeled_62[0x108 - 0x62]; /* menuDrawCharStats copies 0x108 bytes */
} MenuPartyRecord;

struct ControlContext *func_0022A560();
/* OV01 battleContextStateGet (config/symbols/ov01.txt). */
MenuPartyRecord *func_A8B490(int id);

void *func_A9B7D8(int id, int unused);
/* OV01 dataBoxChkCategory2 (config/symbols/ov01.txt). */
int func_A9C808(int value);
extern unsigned char selectionPlainCount;
extern unsigned char selectionPlainList[];

/* Plain item list ("  List" / "  Num" columns): up to eight rows of selectionPlainList
   from the first visible row with their counts, the page marks when the list has
   more than one page, then the icons of the back members (slots 7-9) other than the
   current character, marked when the selected item is in that member's list. */
void func_00220210(int x, int y, LicenseCursor *cursor)
{
    int firstRow = cursorBufferGetAlignedOffset(cursor);
    int page = cursorBufferGetIndex(cursor);
    int pageCount = func_0020F230(cursor);
    int i;
    int rowStyle;
    unsigned char icon;

    func_0021CC08(x, y, 0xCF, 0x11F);
    if (pageCount >= 2) {
        packetSubmitRequest(0x1C, x + 0x83, y + 0x10C, 0x40);
        textDrawDigits(x + 0xAF, y + 0x10C, page + 1, 2);
        func_0021F658(x + 0xC, y + 0x101, page, pageCount, 8);
    }
    func_0021CEF8(x + 8, y + 0x20, 0x90, 0xBE, D_0069D620);
    func_0021CEF8(x + 0x9A, y + 0x20, 0x2D, 0xBE, D_0069D628);
    packetSubmitRequest(0x141, x + 0x1E, y + 4, 1);
    textDrawSegment(0, 0, msgStyleInit, 0);
    for (i = 0; i < 8 && i + firstRow < selectionPlainCount; i++) {
        rowStyle = (firstRow + i) == cursor->index;
        textDrawDecimalShadow(x + 0xA8, y + i * 0x14 + 0x34, func_A9C808(selectionPlainList[firstRow + i]), 2,
                              rowStyle ? LICENSE_COLOR_SELECTED : LICENSE_COLOR_IDLE);
        textDrawSegment(x + 0x10, y + i * 0x14 + 0x32, func_A9B7D8(selectionPlainList[firstRow + i], 0), rowStyle);
    }
    if (cursor->flags & 1) {
        func_0021F2F0(x + 0xA, y + (cursor->index - firstRow) * 0x14 + 0x32, 0x8C, 1);
        menuDrawCursor();
    }
    /* x now walks the member icons. */
    x += 0x30;
    for (i = 7; i < 10; i++) {
        if (ptState.slots[i] != 0 && func_0022A560()->id != ptState.slots[i]->id) {
            icon = ptState.slots[i]->status->value;
            if (cursor->count != 0 && menuListContains(ptState.slots[i], selectionPlainList[cursor->index])) {
                func_0021F0A0(x, y + 0xE3, icon);
                fontObjSetColor(0x3F80000060708040LL);
                packetSubmitRequest(0x2E, x + 0x1B, y + 0xE9, 5);
                fontObjResetColor();
            } else {
                func_0021F190(x, y + 0xE3, icon);
            }
            x += 0x4A;
        }
    }
}

extern unsigned char D_0069D630[];
/* OV01 battleContextApplyLookup (config/symbols/ov01.txt). */
int func_A8B4B0(int id, int item);
void *func_A9B750(int dispatch_value, int value);

/* Item list of the selected party member: member tabs (0x166 for the selected one),
   then one row per item with its count; a row whose count exceeds the member
   record's value60 (the member's EP, see func_00224740) is drawn dimmed. Only the count
   lookup tests func_0022A560's member for null; the dim test reads it unchecked. */
void func_00220550(int x, int y, LicenseCursor *cursor, unsigned char *items)
{
    int firstRow;
    int tabX;
    int tab;
    unsigned int i;
    int count;
    int memberCount;
    int rowStyle;
    unsigned char selected = ptState.menuSlot;

    firstRow = cursorBufferGetAlignedOffset(cursor);
    func_0021CC08(x, y, 0xE0, 0xAA);
    func_0021CEF8(x + 8, y + 0x20, 0xA1, 0x6A, D_0069D620);
    func_0021CEF8(x + 0xAB, y + 0x20, 0x2D, 0x6A, D_0069D630);
    packetSubmitRequest(0x12A, x + 0x41, y + 4, 1);
    packetSubmitRequest(0x43, x + 0x98, y + 0x97, 0x40);
    if (ptState.splitMode == PARTY_SPLIT_ON) {
        memberCount = ptState.counts[2];
    } else {
        memberCount = ptState.counts[1];
    }
    if (memberCount >= 2) {
        tabX = x + 0x10;
        for (tab = 0; tab < memberCount; tab++) {
            if (tab == selected) {
                tabX -= 2;
                packetSubmitRequest(0x166, tabX, y + 0x8A, 1);
                tabX += 0x10;
            } else {
                packetSubmitRequest(0x167, tabX, y + 0x94, 1);
                tabX += 9;
            }
        }
    }
    textDrawSegment(0, 0, msgStyleInit, 0);
    for (i = 0; i < cursor->count; i++) {
        count = 0;
        if (func_0022A560() != 0) {
            count = func_A8B4B0(func_0022A560()->id, items[i]);
        }
        rowStyle = (firstRow + i) == cursor->index;
        textDrawDecimalShadow(x + 0xB9, y + i * 0x14 + 0x32, count, 2,
                              rowStyle ? LICENSE_COLOR_SELECTED : LICENSE_COLOR_IDLE);
        if (func_A8B490(func_0022A560()->id)->value60 < count) {
            rowStyle |= 2;
        }
        textDrawSegment(x + 0x18, y + i * 0x14 + 0x30, func_A9B750(items[i], 0), rowStyle);
    }
    if (cursor->flags & 1) {
        func_0021F2F0(x + 0xA, y + (cursor->index - firstRow) * 0x14 + 0x30, 0x9D, 1);
        menuDrawCursor();
    }
}

/* The default menu text colour (64-bit RGBA+Q word). */
#define MENU_PARTY_TEXT_COLOR 0x3F8000008054200BLL

/* Draws a stat value, colour-coded against a second value (menuDrawCharStats): a
   marker is drawn beside it when the values differ. */
void func_00220840(int x, int y, int value, int compare)
{
    unsigned long long accentColour;
    unsigned long long textColour;

    if (value > compare) {
        accentColour = 0x3F8000008010104BULL;
        textColour = 0x3F80000080545B80ULL;
        func_0021BBE8(x + 0x1C, y, 0, accentColour);
    } else if (value < compare) {
        accentColour = 0x3F80000080641B15ULL;
        textColour = 0x3F80000080805148ULL;
        func_0021BBE8(x + 0x1C, y, 1, accentColour);
    } else {
        accentColour = MENU_PARTY_TEXT_COLOR;
        textColour = 0x3F80000080808080ULL;
    }

    fontObjSetColor(textColour);
    textDrawDecimal(x, y, value, 3, accentColour);
    fontObjResetColor();
}

extern unsigned char D_003EB290[];
extern unsigned char D_0069D638[];
extern unsigned char D_0069D640[];
extern unsigned char D_0069D648[];
extern unsigned char D_0069D650[];
extern unsigned char D_0069D658[];
extern unsigned char D_0069D660[];
extern unsigned char D_0069D668[];
extern unsigned char D_0069D670[];
extern unsigned char D_0069D678[];
extern unsigned char D_0069D680[];
extern unsigned char D_0069D688[];
extern unsigned char D_0068AA10[];
extern LicenseRecord selectionRecordTable[];
/* Copy of the member's record taken before the preview equip. */
extern MenuPartyRecord D_006A5530;
/* OV01 battleSlotSetByte, battleSlotSetShort and battleSlotGet (config/symbols/ov01.txt). */
void func_A8B9A0(int id, int slot, int item);
void func_A8B930(int id, int slot, int item);
MenuPartyRecord *func_A8AF10(int id);

/* Stat comparison panel: saves the member's record, equips item in slot (slot >= 0),
   draws the stat labels and each stat against the saved copy, then the level panel
   (Next Lv, Total Exp). Then, for slot >= 0, the slot is re-equipped: below entry_count
   with its previous entry (members outside 0-10 get the entry byte itself, members 0-10
   that entry's selectionRecordTable dispatch_value, and when that is 0x23 value5C and
   value60 come back from the saved copy), at or past entry_count with 0. The previous entry
   is read before the slot test, as in the asm: with slot -1 it is the byte in front of
   entries (flags[1]) and is not used. */
void menuDrawCharStats(int x, int y, struct ControlContext *member, int item, int slot)
{
    MenuPartyRecord *record;
    unsigned char previous;
    LicenseRecord *entry;

    record = func_A8B490(member->id);
    D_006A5530 = *record;
    previous = member->buffer->entries[slot];
    if (slot > -1) {
        if (member->id < 0 || member->id > 10) {
            func_A8B930(member->id, slot, item);
        } else {
            func_A8B9A0(member->id, slot, item);
        }
    }
    record = func_A8B490(member->id);
    panelDrawNineSlice(D_003EB290, x, y, 0x56, 0x8B);
    if (ptState.splitMode == PARTY_SPLIT_ON) {
        textDrawGlyphString(x + 8, y + 0xA, D_0069D638, MENU_PARTY_TEXT_COLOR);
    } else {
        textDrawGlyphString(x + 8, y + 0xA, D_0069D640, MENU_PARTY_TEXT_COLOR);
    }
    if (ptState.splitMode == PARTY_SPLIT_ON) {
        textDrawGlyphString(x + 8, y + 0x1C, D_0069D648, MENU_PARTY_TEXT_COLOR);
    } else {
        textDrawGlyphString(x + 8, y + 0x1C, D_0069D650, MENU_PARTY_TEXT_COLOR);
    }
    textDrawGlyphString(x + 8, y + 0x2E, D_0069D658, MENU_PARTY_TEXT_COLOR);
    textDrawGlyphString(x + 8, y + 0x40, D_0069D660, MENU_PARTY_TEXT_COLOR);
    textDrawGlyphString(x + 8, y + 0x52, D_0069D668, MENU_PARTY_TEXT_COLOR);
    textDrawGlyphString(x + 8, y + 0x64, D_0069D670, MENU_PARTY_TEXT_COLOR);
    textDrawGlyphString(x + 8, y + 0x76, D_0069D678, MENU_PARTY_TEXT_COLOR);
    textDrawGlyphString(x + 0x2A, y + 0xA, D_0069D680, MENU_PARTY_TEXT_COLOR);
    textDrawGlyphString(x + 0x2A, y + 0x1C, D_0069D680, MENU_PARTY_TEXT_COLOR);
    textDrawGlyphString(x + 0x2A, y + 0x2E, D_0069D680, MENU_PARTY_TEXT_COLOR);
    textDrawGlyphString(x + 0x2A, y + 0x40, D_0069D680, MENU_PARTY_TEXT_COLOR);
    textDrawGlyphString(x + 0x2A, y + 0x52, D_0069D680, MENU_PARTY_TEXT_COLOR);
    textDrawGlyphString(x + 0x2A, y + 0x64, D_0069D680, MENU_PARTY_TEXT_COLOR);
    textDrawGlyphString(x + 0x2A, y + 0x76, D_0069D680, MENU_PARTY_TEXT_COLOR);
    func_00220840(x + 0x35, y + 0xA, record->strength, D_006A5530.strength);
    func_00220840(x + 0x35, y + 0x1C, record->vitality, D_006A5530.vitality);
    func_00220840(x + 0x35, y + 0x2E, record->dexterity, D_006A5530.dexterity);
    func_00220840(x + 0x35, y + 0x40, record->evasion, D_006A5530.evasion);
    func_00220840(x + 0x35, y + 0x52, record->agility, D_006A5530.agility);
    func_00220840(x + 0x35, y + 0x64, record->etherAttack, D_006A5530.etherAttack);
    func_00220840(x + 0x35, y + 0x76, record->etherDefense, D_006A5530.etherDefense);
    if (ptState.splitMode == PARTY_SPLIT_ON) {
        x -= 0x11;
        panelDrawNineSlice(D_003EB290, x - 0xA3, y + 0x56, 0xAE, 0x35);
    } else {
        panelDrawNineSlice(D_003EB290, x - 0xA3, y + 0x56, 0x9D, 0x35);
    }
    textDrawGlyphString(x - 0x9E, y + 0x60, D_0069D688, MENU_PARTY_TEXT_COLOR);
    textDrawGlyphString(x - 0x9E, y + 0x74, D_0068AA10, MENU_PARTY_TEXT_COLOR);
    textDrawGlyphString(x - 0x58, y + 0x60, D_0069D680, MENU_PARTY_TEXT_COLOR);
    textDrawGlyphString(x - 0x58, y + 0x74, D_0069D680, MENU_PARTY_TEXT_COLOR);
    if (ptState.splitMode == PARTY_SPLIT_ON) {
        x += 0x11;
    }
    textDrawDecimal(x - 0x53, y + 0x60, member->limits->next_level_exp, 8, MENU_PARTY_TEXT_COLOR);
    textDrawDecimalAlt(x - 0x51, y + 0x74, member->limits->total_exp, 10, MENU_PARTY_TEXT_COLOR);
    if (slot > -1) {
        if (slot < member->buffer->entry_count) {
            if (member->id < 0 || member->id > 10) {
                func_A8B930(member->id, slot, previous);
            } else {
                entry = &selectionRecordTable[previous];
                func_A8B9A0(member->id, slot, entry->dispatch_value);
                if (entry->dispatch_value == 0x23) {
                    record = func_A8AF10(member->id);
                    record->value5C = D_006A5530.value5C;
                    record->value60 = D_006A5530.value60;
                }
            }
        } else {
            if (member->id < 0 || member->id > 10) {
                func_A8B930(member->id, slot, 0);
            } else {
                func_A8B9A0(member->id, slot, 0);
            }
        }
    }
}

void *selectionRecordSetValue(int index, int value);
extern unsigned char D_003EB380[];
extern unsigned char D_0069D690[];

/* Equip slot list: one row per entry in items, a text label (func_A9B7D8's text, the one
   func_00220210 lists, while the party split is open, otherwise selectionRecordSetValue's
   record text) and the selection cursor box. */
void menuDrawEquipPanel(int x, int y, LicenseCursor *cursor, unsigned char *items, unsigned char rowCount)
{
    int firstRow;
    int i;
    int rowStyle;

    firstRow = cursorBufferGetAlignedOffset(cursor);
    func_0021C228(D_003EB380, x, y, 0x9D, rowCount * 0x14 + 0x37);
    func_0021CEF8(x + 8, y + 0xF, 0x94, rowCount * 0x14 + 0x19, D_0069D690);
    textDrawSegment(0, 0, msgStyleInit, 0);

    for (i = 0; i < cursor->count; i++) {
        rowStyle = (firstRow + i) == cursor->index;
        if (ptState.splitMode == PARTY_SPLIT_ON) {
            textDrawSegment(x + 0x10, y + 0x20 + i * 0x14, func_A9B7D8(items[i], 0), rowStyle | 4);
        } else {
            textDrawSegment(x + 0x10, y + 0x20 + i * 0x14, selectionRecordSetValue(items[i], 0), rowStyle | 4);
        }
    }

    if (cursor->flags & 1) {
        func_0021F2F0(x + 0xA, y + (cursor->index - firstRow) * 0x14 + 0x20, 0x90, 1);
        menuDrawCursor();
    }
}

int selectionEntryBitOutsideMask(int group, int slot, int sub, int mask);
extern unsigned char D_0068AA20[];
extern unsigned char D_0069D698[];

/* A selection entry: the record index (group << 5) + (slot << 2) + sub. Copied by
   value (the asm keeps the byte unextended and masks it with 0xFF for the range test
   and the group and slot fields). */
typedef struct SelectionEntry {
    unsigned char id;
} SelectionEntry;

/* Whether the entry is a record entry (below 0x70) whose bit lies outside mask. */
static __inline__ int selectionEntryOutsideMask(SelectionEntry entry, int mask)
{
    if (entry.id < 0x70) {
        return selectionEntryBitOutsideMask(entry.id >> 5, (entry.id >> 2) & 7, entry.id & 3, mask);
    }
    return 0;
}

/* Selection entry list of one category: four rows from the first visible one (an
   entry outside mask shows text 0x190), the kind mark of each entry (0x133, 0x131 or
   0x132), the cursor, and the selected entry's detail (0x7F with an empty list). */
void menuDrawItemCategory(int x, int y, LicenseCursor *cursor, SelectionEntry *items, int mask)
{
    int firstRow = cursorBufferGetAlignedOffset(cursor);
    int page = cursorBufferGetIndex(cursor);
    int i;
    int outside;
    int rowStyle;
    unsigned char kind;

    menuDrawListWindow(x, y, page, func_0020F230(cursor));
    menuDrawBorder(x + 8, y + 0x1F, 0xD0, 0x2E, 0x3F80000080381A1ALL);
    func_0021CEF8(x + 0xC, y + 0x24, 0xC8, 0x24, D_0068AA20);
    func_0021CEF8(x + 8, y + 0x51, 0xA4, 0x77, D_0069D620);
    func_0021CEF8(x + 0xAD, y + 0x51, 0x2B, 0x77, D_0069D698);
    packetSubmitRequest(0x12C, x + 0x35, y + 4, 1);
    textDrawSegment(0, 0, msgStyleInit, 0);
    if (cursor->count != 0) {
        menuDrawCharInfo(x + 0x18, y + 0x31, items[cursor->index].id >> 5, (items[cursor->index].id >> 2) & 7, 2);
    }
    for (i = 0; i < 4 && firstRow + i < cursor->count; i++) {
        outside = selectionEntryOutsideMask(items[firstRow + i], mask);
        rowStyle = (firstRow + i) == cursor->index;
        if (func_002299B0(func_0022A560(), items[firstRow + i].id) == 0) {
            rowStyle |= 2;
        }
        if (outside) {
            textDrawSegment(x + 0x18, y + i * 0x17 + 0x62, menuMainTxdEntry(0x190), rowStyle);
        } else {
            textDrawSegment(x + 0x18, y + i * 0x17 + 0x62, selectionRecordSetValue(items[firstRow + i].id, 0),
                            rowStyle);
            kind = selectionRecordTable[items[firstRow + i].id].kind;
            if (kind == 0 || kind == 1) {
                if (kind == 0) {
                    packetSubmitRequest(0x133, x + 0xBA, y + i * 0x17 + 0x62, 0);
                } else {
                    packetSubmitRequest(0x131, x + 0xBA, y + i * 0x17 + 0x62, 0);
                }
            } else {
                packetSubmitRequest(0x132, x + 0xBA, y + i * 0x17 + 0x62, 0);
            }
        }
    }
    if (cursor->flags & 1) {
        func_0021F2F0(x + 0xA, y + (cursor->index - firstRow) * 0x17 + 0x62, 0xA0, cursor->flags & 1);
        menuDrawCursor();
    }
    if (cursor->count != 0) {
        func_0021F8A8(x + 0x70, y + 0xE0, items[cursor->index].id);
    } else {
        func_0021F8A8(x + 0x70, y + 0xE0, 0x7F);
    }
}

void menuDrawKindIcon(int x, int y, int index)
{
    fontObjSetColor(0x3F80000080000000LL);
    packetSubmitRequest(0x44, x, y, 4);
    fontObjResetColor();
    packetSubmitRequest(index + 0x47, x, y, 1);
}

/* Same records as tu109.c's LookupRecord (selectionBuildKeyTable fills value with
   txdTableEntry): value is the command's name text. The attribute bytes are the kind
   icons of an attack command; for a special attack its kind and copilot member mask, or
   the two members who perform it. The detail panels (func_00222860) show the rest as
   text entries: target 0x111 + n, zone 0x116 + n, type 0x125 + n, range 0x12B + n,
   condition 0x13A + n (from 4 on: "%1d" of n - 3), effect 0x140 + n, and one entry
   0x130 + n per bit n of elementMask ("Attribute"). D_003EB6D8 is the table's first
   record, D_003EBA38 its record 54 and D_003EBAA8 its record 61. */
struct LookupRecord {
    int value;
    unsigned char attributes[3];
    unsigned char target;               /* 0x07 */
    unsigned char zone;                 /* 0x08 */
    unsigned char type;                 /* 0x09 */
    unsigned char range;                /* 0x0A */
    unsigned char condition;            /* 0x0B */
    unsigned char effect;               /* 0x0C */
    unsigned char unmodeled_0d;
    unsigned short elementMask;         /* 0x0E */
};

extern struct LookupRecord D_003EBAA8[];
extern unsigned char D_0068AA30[];
extern unsigned char D_0068AA40[];
extern unsigned char D_0068AA50[];
extern unsigned char D_0068AA60[];

/* Attack command list of a party member: seven rows, each with the command text and
   its kind icons (kind 3 shows text 0x14A for members from 11 on, kind 4 text 0x14B).
   Members below 11 have seven commands of their own (rows from 7 on use the shared
   records from 0x5F), the others ten. */
void menuDrawAttackCommand(int x, int y, LicenseCursor *cursor, int member)
{
    int firstRow = cursorBufferGetAlignedOffset(cursor);
    int early = member < 11;
    int i;
    int rowStyle;
    int iconX;
    int index;

    func_0021C228(D_003EB380, x, y, 0xFD, 0xC7);
    if (!early) {
        func_0021CEF8(x + 8, y + 0xF, 0xA0, 0xA9, D_0068AA30);
    } else {
        func_0021CEF8(x + 8, y + 0xF, 0xA0, 0xA9, D_0068AA40);
    }
    func_0021CEF8(x + 0xAA, y + 0xF, 0x4B, 0xA9, D_0068AA50);
    packetSubmitRequest(0x169, x + 0xB8, y - 0x14, 1);
    packetSubmitRequest(0x16A, x + 0xD0, y - 0x14, 1);
    textDrawSegment(0, 0, msgStyleInit, 0);
    textDrawSegment(0, 0, D_0068AA60, 0);
    for (i = 0; i < 7 && i + firstRow < cursor->count; i++) {
        rowStyle = (firstRow + i) == cursor->index;
        if (!early) {
            iconX = x + 0xC6;
            index = (member - 11) * 10 + i + 0x31;
        } else {
            iconX = x + 0xB4;
            index = (member - 1) * 7 + i;
            if (firstRow + i >= 7) {
                index = i + 0x5F;
            }
        }
        textDrawSegment(x + 0x10, y + 0x1F + i * 0x15, (void *)D_003EBAA8[index].value, rowStyle);
        if (D_003EBAA8[index].attributes[0] != 0) {
            if (!early && D_003EBAA8[index].attributes[0] == 3) {
                textDrawSegment(iconX + 2, y + 0x1F + i * 0x15, menuMainTxdEntry(0x14A), rowStyle);
            } else if (D_003EBAA8[index].attributes[0] == 4) {
                textDrawSegment(iconX - 2, y + 0x1F + i * 0x15, menuMainTxdEntry(0x14B), rowStyle);
            } else {
                menuDrawKindIcon(iconX, y + 0x1F + i * 0x15, D_003EBAA8[index].attributes[0]);
            }
        }
        if (D_003EBAA8[index].attributes[1] != 0) {
            menuDrawKindIcon(iconX + 0x12, y + 0x1F + i * 0x15, D_003EBAA8[index].attributes[1]);
        }
        if (D_003EBAA8[index].attributes[2] != 0) {
            menuDrawKindIcon(iconX + 0x24, y + 0x1F + i * 0x15, D_003EBAA8[index].attributes[2]);
        }
    }
    if (cursor->flags & 1) {
        func_0021F2F0(x + 0xA, y + (cursor->index - firstRow) * 0x15 + 0x1F, 0x9C, cursor->flags & 1);
        menuDrawCursor();
    }
}

extern unsigned char D_0068AA70[];
extern unsigned char D_0068AA88[];
extern unsigned char D_0068AA98[];
extern unsigned char D_0068AAA8[];
int func_00229750(int member, int nth, unsigned short mask);
int func_00229578(int member, int nth, unsigned short mask);
int func_0022B4E8(int id);
struct ControlContext *ptMemberRecordAt(int index);

/* Special attack list of a party member. From member 11 on: the attack's kind text,
   dimmed unless the bit of the member's copilot (func_0022B4E8: the id in the roster
   entry paired with the member, 0 for none) is set in the attack's member mask, the one
   menuDrawCopilotInfo draws as "Copilot". Below 11: combination attacks, with the icon of
   the partner member. */
void menuDrawSpecialAttack(int x, int y, LicenseCursor *cursor, int member, unsigned short mode)
{
    int firstRow = cursorBufferGetAlignedOffset(cursor);
    int early = member < 11;
    unsigned int i;
    int rowStyle;
    int index;
    int copilotId;

    func_0021C228(D_003EB380, x, y, 0xFD, 0xC7);
    if (!early) {
        func_0021CEF8(x + 8, y + 0xF, 0xA0, 0xA9, D_0068AA70);
    } else {
        func_0021CEF8(x + 8, y + 0xF, 0xA0, 0xA9, D_0068AA88);
    }
    if (!early) {
        func_0021CEF8(x + 0xAA, y + 0xF, 0x4B, 0xA9, D_0068AA98);
    } else {
        func_0021CEF8(x + 0xAA, y + 0xF, 0x4B, 0xA9, D_0068AAA8);
    }
    packetSubmitRequest(0x168, x + 0xB8, y - 0x14, 1);
    packetSubmitRequest(0x16B, x + 0xD0, y - 0x14, 1);
    textDrawSegment(0, 0, msgStyleInit, 0);
    textDrawSegment(0, 0, D_0068AA60, 0);
    for (i = 0; i < cursor->count; i++) {
        rowStyle = (firstRow + i) == cursor->index;
        if (!early) {
            index = func_00229750(member, i, mode);
            copilotId = func_0022B4E8(member);
            if (copilotId <= 0 || !((D_003EBAA8[index].attributes[1] >> (copilotId - 1)) & 1)) {
                rowStyle |= 2;
            }
            textDrawSegment(x + 0xBE, y + i * 0x15 + 0x1F, menuMainTxdEntry(D_003EBAA8[index].attributes[0] + 0x14D),
                            rowStyle);
            textDrawSegment(x + 0x10, y + i * 0x15 + 0x1F, (void *)D_003EBAA8[index].value, rowStyle);
        } else {
            index = func_00229578(member, i, mode);
            if (index != -1) {
                if (D_003EBAA8[index].attributes[0] == member) {
                    func_0021F110(x + 0xC4, y + i * 0x15 + 0x1F,
                                  ptMemberRecordAt(D_003EBAA8[index].attributes[1] - 1)->status->value);
                } else {
                    func_0021F110(x + 0xC4, y + i * 0x15 + 0x1F,
                                  ptMemberRecordAt(D_003EBAA8[index].attributes[0] - 1)->status->value);
                }
                textDrawSegment(x + 0x10, y + i * 0x15 + 0x1F, (void *)D_003EBAA8[index].value, rowStyle);
            }
        }
    }
    if (cursor->flags & 1) {
        func_0021F2F0(x + 0xA, y + (cursor->index - firstRow) * 0x15 + 0x1F, 0x9C, cursor->flags & 1);
        menuDrawCursor();
    }
}

extern char D_0069D6E0[];
extern unsigned char D_003EB2B8[];
int xglFontGetTextWidth(const char *text);
extern struct LookupRecord D_003EB6D8[];
extern unsigned char D_0069D6A0[];
extern unsigned char D_0068AAB8[];
extern unsigned char D_0069D6A8[];
extern unsigned char D_0068AAC8[];
extern unsigned char D_0069D6B0[];
extern unsigned char D_0069D6B8[];
extern unsigned char D_0069D6C8[];
extern unsigned char D_0069D6D0[];
extern unsigned char D_0068AAD8[];
extern unsigned char D_0069D6D8[];
extern unsigned char D_0069D6F8[];
extern unsigned char D_0069D700[];
extern struct LookupRecord D_003EBA38[];
extern struct LookupRecord D_003EC028[];
extern unsigned char D_0069D6C0[];
extern unsigned char D_0068AAE8[];
extern unsigned char D_0069D6E8[];
int func_002AA5D8(char *buffer, const char *format, ...);

/* Command detail panel of an attack command (cursor rows below 7 are member's own
   commands, the others shared ones): "Command" (kind icons, or text 0x14B), "Target",
   "Zone", "Type", "Range", "Attribute" (joined with "/"), "Condition" and "Effect". */
void menuDrawCommandInfo(int x, int y, LicenseCursor *cursor, int member)
{
    struct LookupRecord *record;
    int i;
    int attributeX;
    int separate;
    char countText[16];

    if (cursor->index >= 7) {
        record = &D_003EC028[cursor->index];
    } else {
        record = &D_003EBA38[member * 7 + cursor->index];
    }
    panelDrawNineSlice(D_003EB2B8, x, y, 0xE0, 0x11F);
    menuDrawBorder(x + 8, y + 0x1F, 0xD0, 0x2E, 0x3F80000080381A1ALL);
    func_0021CEF8(x + 0xC, y + 0x24, 0xC8, 0x24, D_0069D6A0);
    func_0021CEF8(x + 8, y + 0x51, 0xD0, 0xBE, D_0068AAB8);
    packetSubmitRequest(0x128, x + 0x27, y + 4, 1);
    textDrawSegment(0, 0, msgStyleInit, 0);
    textDrawSegment(0, 0, D_0069D6A8, 0);
    textDrawSegment(0, 0, D_0068AAC8, 0);
    textDrawSegment(x + 0x10, y + 0x60, D_0069D6B0, 0);
    textDrawSegment(x + 0x10, y + 0x74, D_0069D6B8, 0);
    textDrawSegment(x + 0x10, y + 0x88, D_0069D6C0, 0);
    textDrawSegment(x + 0x10, y + 0x9C, D_0069D6C8, 0);
    textDrawSegment(x + 0x10, y + 0xB0, D_0069D6D0, 0);
    textDrawSegment(x + 0x10, y + 0xC4, D_0068AAD8, 0);
    textDrawSegment(x + 0x10, y + 0xD8, D_0068AAE8, 0);
    textDrawSegment(x + 0x10, y + 0xEC, D_0069D6D8, 0);
    textDrawSegment(0, 0, D_0068A600, 0);
    for (i = 0; i < 8; i++) {
        textDrawSegment(x + 0x58, y + i * 0x14 + 0x62, D_0069D680, 0);
    }
    textDrawSegment(x + 0x18, y + 0x32, (void *)record->value, 0);
    if (record->attributes[0] < 4) {
        if (record->attributes[0] != 0) {
            menuDrawKindIcon(x + 0x62, y + 0x60, record->attributes[0]);
        }
        if (record->attributes[1] != 0) {
            menuDrawKindIcon(x + 0x74, y + 0x60, record->attributes[1]);
        }
        if (record->attributes[2] != 0) {
            menuDrawKindIcon(x + 0x86, y + 0x60, record->attributes[2]);
        }
    } else {
        textDrawSegment(x + 0x62, y + 0x60, menuMainTxdEntry(0x14B), 0);
    }
    if (record->target != 0) {
        textDrawSegment(x + 0x64, y + 0x77, menuMainTxdEntry(record->target + 0x111), 0);
    }
    textDrawSegment(x + 0x64, y + 0x8B, menuMainTxdEntry(record->zone + 0x116), 0);
    if (record->type != 0) {
        textDrawSegment(x + 0x64, y + 0x9F, menuMainTxdEntry(record->type + 0x125), 0);
    }
    if (record->range != 0) {
        textDrawSegment(x + 0x64, y + 0xB3, menuMainTxdEntry(record->range + 0x12B), 0);
    }
    attributeX = 0;
    separate = 0;
    for (i = 0; i < 9; i++) {
        if ((record->elementMask >> i) & 1) {
            if (separate) {
                textDrawSegment(x + attributeX + 0x64, y + 0xC7, D_0069D6E0, 0);
                attributeX += 7;
            }
            textDrawSegment(x + attributeX + 0x64, y + 0xC7, menuMainTxdEntry(i + 0x130), 0);
            attributeX += (xglFontGetTextWidth(menuMainTxdEntry(i + 0x130)) * 13) >> 4;
            separate = 1;
        }
    }
    if (record->condition != 0) {
        if (record->condition >= 4) {
            func_002AA5D8(countText, D_0069D6E8, record->condition - 3);
            textDrawSegment(x + 0x64, y + 0xDB, menuMainTxdEntry(0x140), 0);
            textDrawSegment(x + 0xA0, y + 0xDB, countText, 0);
        } else {
            textDrawSegment(x + 0x64, y + 0xDB, menuMainTxdEntry(record->condition + 0x13A), 0);
        }
    }
    if (record->effect != 0) {
        textDrawSegment(x + 0x64, y + 0xEF, menuMainTxdEntry(record->effect + 0x140), 0);
    }
}

extern unsigned char D_0069D6F0[];

/* Detail panel of a combination attack: "Partner" (name text 0x10 + partner and
   icon), "Target", "Type", "Attribute" (text 0xEF for types 1 and 2 without
   attributes), "Condition" and "Effect". */
void menuDrawPartnerInfo(int x, int y, LicenseCursor *cursor, int member, unsigned short mode)
{
    struct LookupRecord *record;
    int partner;
    int i;
    int attributeX;
    int separate;
    char countText[16];

    panelDrawNineSlice(D_003EB2B8, x, y, 0xE0, 0x11F);
    menuDrawBorder(x + 8, y + 0x1F, 0xD0, 0x2E, 0x3F80000080381A1ALL);
    func_0021CEF8(x + 0xC, y + 0x24, 0xC8, 0x24, D_0069D6A0);
    func_0021CEF8(x + 8, y + 0x51, 0xD0, 0xBE, D_0068AAB8);
    packetSubmitRequest(0x128, x + 0x27, y + 4, 1);
    textDrawSegment(0, 0, msgStyleInit, 0);
    textDrawSegment(0, 0, D_0069D6A8, 0);
    textDrawSegment(0, 0, D_0068AAC8, 0);
    textDrawSegment(x + 0x10, y + 0x60, D_0069D6F0, 0);
    textDrawSegment(x + 0x10, y + 0x74, D_0069D6B8, 0);
    textDrawSegment(x + 0x10, y + 0x88, D_0069D6C8, 0);
    textDrawSegment(x + 0x10, y + 0x9C, D_0068AAD8, 0);
    textDrawSegment(x + 0x10, y + 0xB0, D_0068AAE8, 0);
    textDrawSegment(x + 0x10, y + 0xC4, D_0069D6D8, 0);
    textDrawSegment(0, 0, D_0068A600, 0);
    for (i = 0; i < 6; i++) {
        textDrawSegment(x + 0x58, y + i * 0x14 + 0x62, D_0069D680, 0);
    }
    /* i is reused for the record index. */
    i = func_00229578(member, cursor->index, mode);
    if (i != -1) {
        record = &D_003EBAA8[i];
        textDrawSegment(x + 0x18, y + 0x32, (void *)record->value, 0);
        if (record->attributes[0] != member) {
            partner = record->attributes[0];
        } else {
            partner = record->attributes[1];
        }
        textDrawSegment(0, 0, D_0069D6A8, 0);
        textDrawSegment(x + 0x64, y + 0x63, menuMainTxdEntry(partner + 0x10), 0);
        func_0021F0A0(x + 0xB4, y + 0x61, ptMemberRecordAt(partner - 1)->status->value);
        if (record->target != 0) {
            textDrawSegment(x + 0x64, y + 0x77, menuMainTxdEntry(record->target + 0x111), 0);
        }
        if (record->type != 0) {
            textDrawSegment(x + 0x64, y + 0x8B, menuMainTxdEntry(record->type + 0x125), 0);
        }
        attributeX = 0;
        separate = 0;
        if (record->elementMask == 0) {
            if (record->type == 1 || record->type == 2) {
                textDrawSegment(x + 0x64, y + 0x9F, menuMainTxdEntry(0xEF), 0);
            }
        } else {
            for (i = 0; i < 9; i++) {
                if ((record->elementMask >> i) & 1) {
                    if (separate) {
                        textDrawSegment(x + attributeX + 0x64, y + 0x9F, D_0069D6E0, 0);
                        attributeX += 7;
                    }
                    textDrawSegment(x + attributeX + 0x64, y + 0x9F, menuMainTxdEntry(i + 0x130), 0);
                    attributeX += (xglFontGetTextWidth(menuMainTxdEntry(i + 0x130)) * 13) >> 4;
                    separate = 1;
                }
            }
        }
        if (record->condition < 10) {
            func_002AA5D8(countText, D_0069D6E8, record->condition);
            textDrawSegment(x + 0x64, y + 0xB3, menuMainTxdEntry(0x140), 0);
            textDrawSegment(x + 0xA0, y + 0xB3, countText, 0);
        }
        if (record->effect != 0) {
            textDrawSegment(x + 0x64, y + 0xC7, menuMainTxdEntry(record->effect + 0x140), 0);
        }
    }
}

/* Command detail panel of member's selected command (ten records per member):
   "Command", "Target", "Type", "Range", "Attribute", "Effect" and "Copilot" (always
   "None") rows with their values; the attributes are joined with "/". */
void func_00222860(int x, int y, LicenseCursor *cursor, int member)
{
    struct LookupRecord *record = &D_003EB6D8[member * 10 + cursor->index];
    int i;
    int attributeX;
    int separate;

    panelDrawNineSlice(D_003EB2B8, x, y, 0xE0, 0x11F);
    menuDrawBorder(x + 8, y + 0x1F, 0xD0, 0x2E, 0x3F80000080381A1ALL);
    func_0021CEF8(x + 0xC, y + 0x24, 0xC8, 0x24, D_0069D6A0);
    func_0021CEF8(x + 8, y + 0x51, 0xD0, 0xBE, D_0068AAB8);
    packetSubmitRequest(0x128, x + 0x27, y + 4, 1);
    textDrawSegment(0, 0, msgStyleInit, 0);
    textDrawSegment(0, 0, D_0069D6A8, 0);
    textDrawSegment(0, 0, D_0068AAC8, 0);
    textDrawSegment(x + 0x10, y + 0x60, D_0069D6B0, 0);
    textDrawSegment(x + 0x10, y + 0x74, D_0069D6B8, 0);
    textDrawSegment(x + 0x10, y + 0x88, D_0069D6C8, 0);
    textDrawSegment(x + 0x10, y + 0x9C, D_0069D6D0, 0);
    textDrawSegment(x + 0x10, y + 0xB0, D_0068AAD8, 0);
    textDrawSegment(x + 0x10, y + 0xC4, D_0069D6D8, 0);
    textDrawSegment(x + 0x10, y + 0xD8, D_0069D6F8, 0);
    textDrawSegment(0, 0, D_0068A600, 0);
    for (i = 0; i < 7; i++) {
        textDrawSegment(x + 0x58, y + i * 0x14 + 0x62, D_0069D680, 0);
    }
    textDrawSegment(x + 0x18, y + 0x32, (void *)record->value, 0);
    if (record->attributes[0] != 0) {
        if (record->attributes[0] == 3) {
            textDrawSegment(x + 0x64, y + 0x63, menuMainTxdEntry(0x14A), 0);
        } else {
            menuDrawKindIcon(x + 0x62, y + 0x60, record->attributes[0]);
        }
    }
    if (record->target != 0) {
        textDrawSegment(x + 0x64, y + 0x77, menuMainTxdEntry(record->target + 0x111), 0);
    }
    if (record->type != 0) {
        textDrawSegment(x + 0x64, y + 0x8B, menuMainTxdEntry(record->type + 0x125), 0);
    }
    if (record->range != 0) {
        textDrawSegment(x + 0x64, y + 0x9F, menuMainTxdEntry(record->range + 0x12B), 0);
    }
    attributeX = 0;
    separate = 0;
    for (i = 0; i < 9; i++) {
        if ((record->elementMask >> i) & 1) {
            if (separate) {
                textDrawSegment(x + attributeX + 0x64, y + 0xB3, D_0069D6E0, 0);
                attributeX += 7;
            }
            textDrawSegment(x + attributeX + 0x64, y + 0xB3, menuMainTxdEntry(i + 0x130), 0);
            attributeX += (xglFontGetTextWidth(menuMainTxdEntry(i + 0x130)) * 13) >> 4;
            separate = 1;
        }
    }
    if (record->effect != 0) {
        textDrawSegment(x + 0x64, y + 0xC7, menuMainTxdEntry(record->effect + 0x140), 0);
    }
    textDrawSegment(x + 0x64, y + 0xDB, D_0069D700, 0);
}

extern unsigned char D_0069D708[];

/* Detail panel of a special attack: "Copilot" (icons of the front members whose bit
   is set in the attack's member mask), "Charge" (kind text 0x14D + n), "Target",
   "Type", "Range", "Attribute" and "Effect"; the effect text is cut at each backslash-n
   escape: every cut piece is drawn on the same first row and the rest 0x14 lower, so
   there are two rows whatever the number of escapes. i is reused as the record index and
   as the effect text index. */
void menuDrawCopilotInfo(int x, int y, LicenseCursor *cursor, int member, unsigned short mode)
{
    unsigned char line[32];
    int lineY = 0;
    int i;
    struct LookupRecord *record;
    int iconX;
    int attributeX;
    int separate;
    unsigned char *text;

    panelDrawNineSlice(D_003EB2B8, x, y, 0xE0, 0x11F);
    menuDrawBorder(x + 8, y + 0x1F, 0xD0, 0x2E, 0x3F80000080381A1ALL);
    func_0021CEF8(x + 0xC, y + 0x24, 0xC8, 0x24, D_0069D6A0);
    func_0021CEF8(x + 8, y + 0x51, 0xD0, 0xBE, D_0068AAB8);
    packetSubmitRequest(0x128, x + 0x27, y + 4, 1);
    textDrawSegment(0, 0, msgStyleInit, 0);
    textDrawSegment(0, 0, D_0069D6A8, 0);
    textDrawSegment(0, 0, D_0068AAC8, 0);
    textDrawSegment(x + 0x10, y + 0x60, D_0069D6F8, 0);
    textDrawSegment(x + 0x10, y + 0x7C, D_0069D708, 0);
    textDrawSegment(x + 0x10, y + 0x90, D_0069D6B8, 0);
    textDrawSegment(x + 0x10, y + 0xA4, D_0069D6C8, 0);
    textDrawSegment(x + 0x10, y + 0xB8, D_0069D6D0, 0);
    textDrawSegment(x + 0x10, y + 0xCC, D_0068AAD8, 0);
    textDrawSegment(x + 0x10, y + 0xE0, D_0069D6D8, 0);
    textDrawSegment(0, 0, D_0068A600, 0);
    /* Rows after the first ("Copilot") sit 8 lower. */
    for (i = 0; i < 7; i++) {
        textDrawSegment(x + 0x58, y + i * 0x14 + (i <= 0 ? 0x62 : 0x6A), D_0069D680, 0);
    }
    i = func_00229750(member, cursor->index, mode);
    if (i != -1) {
        record = &D_003EBAA8[i];
        textDrawSegment(x + 0x18, y + 0x32, (void *)record->value, 0);
        iconX = x + 0x64;
        for (i = 0; i < 7; i++) {
            if (ptState.slots[i] != 0 && ((record->attributes[1] >> (ptState.slots[i]->id - 1)) & 1)) {
                func_0021F0A0(iconX, y + 0x61, ptState.slots[i]->status->value);
                iconX += 0x18;
            }
        }
        if (record->attributes[0] != 0) {
            textDrawSegment(x + 0x64, y + 0x7F, menuMainTxdEntry(record->attributes[0] + 0x14D), 0);
        }
        if (record->target != 0) {
            textDrawSegment(x + 0x64, y + 0x93, menuMainTxdEntry(record->target + 0x111), 0);
        }
        if (record->type != 0) {
            textDrawSegment(x + 0x64, y + 0xA7, menuMainTxdEntry(record->type + 0x125), 0);
        }
        if (record->range != 0) {
            textDrawSegment(x + 0x64, y + 0xBB, menuMainTxdEntry(record->range + 0x12B), 0);
        }
        attributeX = 0;
        separate = 0;
        for (i = 0; i < 9; i++) {
            if ((record->elementMask >> i) & 1) {
                if (separate) {
                    textDrawSegment(x + attributeX + 0x64, y + 0xCF, D_0069D6E0, 0);
                    attributeX += 7;
                }
                textDrawSegment(x + attributeX + 0x64, y + 0xCF, menuMainTxdEntry(i + 0x130), 0);
                attributeX += (xglFontGetTextWidth(menuMainTxdEntry(i + 0x130)) * 13) >> 4;
                separate = 1;
            }
        }
        if (record->effect != 0) {
            text = menuMainTxdEntry(record->effect + 0x140);
            for (i = 0; text[i] != 0; i++) {
                if (text[i] == '\\' && text[i + 1] == 'n') {
                    line[i] = 0;
                    text += i + 2;
                    i = 0;
                    textDrawSegment(x + 0x64, y + 0xE3, line, 0);
                    lineY = 0x14;
                    line[i] = text[i];
                } else {
                    line[i] = text[i];
                }
            }
            line[i] = 0;
            textDrawSegment(x + 0x64, y + lineY + 0xE3, line, 0);
        }
    }
}

extern unsigned char D_0068AAF8[];
extern unsigned char D_0069D710[];
extern unsigned char D_0068AB08[];
extern unsigned char D_0069D718[];

/* Class panel: class level ("Class Lv", buffer flags[0] + 1) with the C.Pt limit,
   learned skills (flags[1]) with the S.Pt limit. Nothing without a control context. */
void menuDrawClassInfo(int x, int y)
{
    struct ControlContext *context = func_0022A560();

    if (context != 0) {
        panelDrawNineSlice(D_003EB290, x, y, 0xE5, 0x32);
        textDrawGlyphString(x + 0xA, y + 9, D_0068AAF8, MENU_PARTY_TEXT_COLOR);
        textDrawGlyphString(x + 0x53, y + 9, D_0069D680, MENU_PARTY_TEXT_COLOR);
        textDrawDecimal(x + 0x60, y + 9, context->buffer->flags[0] + 1, 3, MENU_PARTY_TEXT_COLOR);
        textDrawGlyphString(x + 0x86, y + 9, D_0069D710, MENU_PARTY_TEXT_COLOR);
        textDrawDecimal(x + 0xB1, y + 9, context->limits->limit_b, 5, MENU_PARTY_TEXT_COLOR);
        textDrawGlyphString(x + 0xA, y + 0x1C, D_0068AB08, MENU_PARTY_TEXT_COLOR);
        textDrawDecimal(x + 0x60, y + 0x1C, context->buffer->flags[1], 3, MENU_PARTY_TEXT_COLOR);
        textDrawGlyphString(x + 0x86, y + 0x1C, D_0069D718, MENU_PARTY_TEXT_COLOR);
        textDrawDecimal(x + 0xB1, y + 0x1C, context->limits->limit_a, 5, MENU_PARTY_TEXT_COLOR);
    }
}

int func_00229EC0(struct ControlContext *context, int group, int slot);
int func_00229F30(struct ControlContext *context, int group, int slot, int sub);
int selectionRecordIndex(int group, int slot, int sub);
extern unsigned char D_0068AB18[];
extern unsigned char D_0069D720[];
extern unsigned char D_0069D728[];
extern unsigned char D_0069D730[];
extern unsigned char D_0069D738[];
extern unsigned short D_003EC0E8[4][8];

#define LICENSE_ROW_PITCH 0x17 /* rows of the group and sub pages */
#define LICENSE_ROW_PITCH_COMPACT 0x12 /* rows of the slot page */
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
    struct ControlBuffer *board = func_0022A560()->buffer;
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
            if (func_00229EC0(func_0022A560(), groupCursor->index, i)
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
            if (func_00229F30(func_0022A560(), groupCursor->index, slotCursor->index, i)
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
                if (D_003EC0E8[groupCursor->index][i] > func_0022A560()->limits->limit_b) {
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
                        if (selectionGetValue(groupCursor->index, slotCursor->index, i) > func_0022A560()->limits->limit_a) {
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

/* The save settings the option page shows (saveDataSetDefaults sets them: +0x28 = 1, +0x3A = 2, the rest 0). */
typedef struct ConfigSaveSettings {
    unsigned char unmodeled_00[0x28];
    unsigned char flag;                 /* +0x28 */
    unsigned char dolby_digital;        /* +0x29 */
    unsigned char unmodeled_2a[0x32 - 0x2A];
    short pairFirst;                    /* +0x32: horizontal display offset (tyaSetDisplayOffset) */
    short pairSecond;                   /* +0x34: vertical display offset (tyaSetDisplayOffset) */
    unsigned char unmodeled_36[0x3A - 0x36];
    unsigned char caption_all;          /* +0x3A */
    unsigned char unmodeled_3b[0x3E - 0x3B];
    unsigned char copyByte;             /* +0x3E: copied to both pads' actuatorMode */
    unsigned char unmodeled_3f[0x46 - 0x3F];
    unsigned char cursor_memory;        /* +0x46 */
    unsigned char cursor_memory_alt;    /* +0x47 */
} ConfigSaveSettings;

extern ConfigSaveSettings SaveData;
extern signed char D_0069E268[8];           /* the shown value of each option row */
extern signed char D_003EB3F8[];            /* per option: value label group, 0 when it shows none */
extern unsigned char D_0068AB28[];
extern unsigned char D_0068AB38[];

/* Option page: eight option rows (items holds their ids), the default row (index 8) and the cursor. */
void menuDrawConfig(int x, int y, LicenseCursor *cursor, unsigned char *items)
{
    int rowY;
    int i;
    int selected;
    int labelX;
    int defaultY;
    int hiddenRow; /* cursor index for which this page draws no cursor */

    panelDrawNineSlice(D_003EB290, x - 0x28, y, 0x15E, 0xC4);
    func_0021CEF8(x - 0x24, y + 5, 0x156, 0xBA, D_0068AB28);
    panelDrawNineSlice(D_003EB290, x - 0x28, y + 0xCE, 0x15E, 0x36);
    func_0021CEF8(x - 0x24, y + 0xD3, 0x156, 0x2C, D_0068AB38);
    D_0069E268[0] = SaveData.cursor_memory;
    D_0069E268[1] = SaveData.cursor_memory_alt;
    D_0069E268[2] = SaveData.flag;
    D_0069E268[3] = SaveData.copyByte == 0;
    D_0069E268[4] = SaveData.caption_all != 0;
    D_0069E268[7] = SaveData.dolby_digital;
    D_0069E268[6] = 0;
    D_0069E268[5] = 0;
    textDrawSegment(0, 0, msgStyleInit, 0);
    rowY = y + 0x16;
    labelX = x - 0x10;
    for (i = 0; i < 8; i++) {
        selected = i == cursor->index;
        textDrawSegment(labelX, rowY, menuMainTxdEntry(items[i] + 0x156), selected);
        if (D_003EB3F8[items[i]] != 0) {
            int label = D_003EB3F8[items[i]] * 2 + 0x161;

            textDrawSegment(x + 0x94, rowY, menuMainTxdEntry(label + D_0069E268[items[i]]), selected);
        }
        rowY += 0x14;
    }
    defaultY = y + 0xE4;
    selected = cursor->index == 8;
    textDrawSegment(labelX, defaultY, menuMainTxdEntry(0x15E), selected);
    hiddenRow = 9;
    if (cursor->index != hiddenRow && (cursor->flags & 1)) {
        if (selected) {
            func_0021F2F0(x - 0x22, defaultY, 0x152, 1);
        } else {
            func_0021F2F0(x - 0x22, y + 0x16 + cursor->index * 0x14, 0x152, 1);
        }
        menuDrawCursor();
    }
}

extern unsigned char D_0068AB48[];

/* Controller page: the pad picture with its shaded parts and the four button labels. */
void menuDrawControllerConfig(void)
{
    panelDrawNineSlice(D_003EB290, 9, 0x5C, 0x1EE, 0x121);
    func_0021CEF8(0xD, 0x61, 0x1E6, 0x117, D_0068AB48);
    packetSubmitRequest(0x2BF, 0, 0xAC, 0);
    packetSubmitRequest(0x2C0, 0, 0xAC, 1);
    packetSubmitRequest(0x2C1, 0xFA, 0x10B, 1);
    fontObjSetColor(0x3F800000802E2E2ELL);
    packetSubmitRequest(0x2CE, 0, 0x5C, 0x45);
    fontObjSetColor(LICENSE_MARK_SHADOW);
    packetSubmitRequest(0x2C2, 1, 0x5E, 0x44);
    fontObjSetColor(0x3F80000080787850LL);
    packetSubmitRequest(0x2C2, 0, 0x5B, 0x44);
    fontObjSetColor(LICENSE_MARK_SHADOW);
    packetSubmitRequest(0x2C3, 1, 0x5F, 0x44);
    fontObjSetColor(0x3F80000080535353LL);
    packetSubmitRequest(0x2C3, 0, 0x5C, 0x40);
    fontObjSetColor(0x3F8000008051464ALL);
    packetSubmitRequest(0x2CA, 0xAB, 0x72, 0x44);
    packetSubmitRequest(0x2C9, 0xAB, 0x91, 0x44);
    packetSubmitRequest(0x2CC, 0x12F, 0x72, 0x44);
    packetSubmitRequest(0x2CB, 0x12F, 0x91, 0x44);
    fontObjSetColor(LICENSE_MARK_SHADOW);
    packetSubmitRequest(0x83, 0x194, 0x72, 4);
    packetSubmitRequest(0x83, 0x194, 0x9A, 4);
    packetSubmitRequest(0x83, 0x194, 0xC2, 4);
    packetSubmitRequest(0x83, 0x194, 0xEA, 4);
    fontObjResetColor();
    packetSubmitRequest(0x85, 0x194, 0x72, 1);
    packetSubmitRequest(0x87, 0x194, 0x9A, 1);
    packetSubmitRequest(0x84, 0x194, 0xC2, 1);
    packetSubmitRequest(0x86, 0x194, 0xEA, 1);
}

/* tu092.c's play time record: hours (+3, drawn without padding), minutes (+2) and seconds (+1).
   menuDrawPlayTime reads minutes and seconds as unsigned bytes. */
struct PlayTime {
    unsigned char unmodeled_00;
    signed char seconds;
    signed char minutes;
    unsigned char hours;
    unsigned char steps;
};

void playTimeClamp(struct PlayTime *playTime);
void xglClockRead(void *clock_time);
unsigned int xglClockDayTime2UInt(void *clock_time);
void xglClockUInt2DayTime(void *clock_time, unsigned int elapsed);

extern unsigned char D_0069D740[];


void menuDrawPlayTime(int x, int y)
{
    struct PlayTime time;
    unsigned int now;
    int rowY;

    xglClockRead(&time);
    now = xglClockDayTime2UInt(&time);
    rowY = y + 0xF;
    xglClockUInt2DayTime(&time, now - ptState.progress->playTime);
    playTimeClamp(&time);
    func_0021CEF8(x, y, 0x6B, 0x21, D_0069D740);
    textDrawGlyphString(x + 0x23, rowY, D_0069D680, MENU_PARTY_TEXT_COLOR);
    textDrawGlyphString(x + 0x3F, rowY, D_0069D680, MENU_PARTY_TEXT_COLOR);
    textDrawDecimal(x + 0x10, rowY, time.hours, 2, MENU_PARTY_TEXT_COLOR);
    textDrawDecimalZero(x + 0x2D, rowY, (unsigned char) time.minutes, 2, MENU_PARTY_TEXT_COLOR);
    textDrawDecimalZero(x + 0x49, rowY, (unsigned char) time.seconds, 2, MENU_PARTY_TEXT_COLOR);
}

void menuResetRecord(Tu104Record *record)
{
    if (record != 0) {
        record->status = -1;
        fontTrackSetBoth(&record->first_block, 0, 0);
        fontTrackSetBoth(&record->second_block, 0, 0);
        record->pending = 0;
    }
}

extern int D_003EC634[];
extern int D_003EC60C[];

/* The member entry of a menu item, returned as a word: none for status -1, else
   D_003EC634[status] (the address of ptState.roster) when pending bit 0x100 marks a party
   item (as in menuDraw), D_003EC60C[status] (ptState.slots) otherwise. */
int func_002245B8(Tu104Record *record)
{
    int index = record->status;

    if (index == -1) {
        return 0;
    }
    if (record->pending & 0x100) {
        return D_003EC634[index];
    }
    return D_003EC60C[index];
}

/* Icon pair at (x, y): piece 0xA2 + kind in `colour`, then piece 0xAA + kind in the font colour
   that was set on entry. Only the low word of that colour is kept (the asm sign-extends it, then
   passes it on zero-extended), so the second piece is drawn with a zero upper half (Q). */
void func_00224608(int kind, int x, int y, u64 colour)
{
    int savedColourLow;

    savedColourLow = (int) fontObjGetColor();
    kind += 0xA2;
    fontObjSetColor(colour);
    packetSubmitRequest(kind, x, y, 4);
    fontObjSetColor((unsigned int) savedColourLow);
    packetSubmitRequest(kind + 8, x, y, 4);
}


void func_002246A0(int x, int y, int level)
{
    fontObjSetColor(0x3F80000080805939LL);
    func_00224608(4, x, y + 1, MENU_PARTY_TEXT_COLOR);
    fontObjResetColor();
    textDrawDecimal(x + 0x14, y + 4, level, 2, MENU_PARTY_TEXT_COLOR);
}

/* Text colours of an HP value against its maximum. The callers pass MenuPartyRecord's value5C and
   gauge, which Opok's src/ov01/tu026.c comments identify (calcHpAdjust: "HP at +0x5C ... max is the
   unaligned int at +0x2"; calcEpAdjust: "EP halfword at +0x60 ... max is the halfword at +0x6"): the
   hp/maxHp and ep/maxEp parameter names of this helper and of the draw helpers below come from there. */
void func_00224740(int hp, int maxHp, u64 *shadowColour, u64 *faceColour)
{
    *faceColour = 0x3F80000080808080LL;
    if (hp <= 0) {
        *shadowColour = LICENSE_COLOR_SELECTED;
        *faceColour = 0x3F80000080575480LL;
    } else if (maxHp >= hp * 4) {
        *shadowColour = 0x3F8000008029224ALL;
        *faceColour = 0x3F8000008060FFFFLL;
    } else {
        *shadowColour = MENU_PARTY_TEXT_COLOR;
        if (hp != maxHp) {
            *faceColour = 0x3F80000080787878LL;
        }
    }
}

void func_00224810(int x, int y, int hp, int maxHp)
{
    u64 shadowColour;
    u64 faceColour;

    fontObjSetColor(0x3F800000807D763FLL);
    func_00224608(0, x, y, 0x3F80000080572000LL);
    fontObjResetColor();
    func_00224740(hp, maxHp, &shadowColour, &faceColour);
    textDrawGlyphString(x + 0x3C, y + 4, D_0069D6E0, MENU_PARTY_TEXT_COLOR);
    textDrawDecimal(x + 0x45, y + 4, maxHp, 4, MENU_PARTY_TEXT_COLOR);
    fontObjSetColor(faceColour);
    textDrawDecimal(x + 0x18, y + 4, hp, 4, shadowColour);
    fontObjResetColor();
}

void func_00224928(int x, int y, int ep, int maxEp)
{
    fontObjSetColor(0x3F800000807D763FLL);
    func_00224608(1, x, y, 0x3F80000080572000LL);
    fontObjResetColor();
    textDrawGlyphString(x + 0x3C, y + 4, D_0069D6E0, MENU_PARTY_TEXT_COLOR);
    textDrawDecimal(x + 0x21, y + 4, ep, 3, MENU_PARTY_TEXT_COLOR);
    textDrawDecimal(x + 0x45, y + 4, maxEp, 3, MENU_PARTY_TEXT_COLOR);
}

void func_00224A28(int x, int y, int hp, int maxHp)
{
    u64 shadowColour;
    u64 faceColour;

    fontObjSetColor(0x3F800000807D763FLL);
    func_00224608(0, x, y, 0x3F80000080572000LL);
    fontObjResetColor();
    func_00224740(hp, maxHp, &shadowColour, &faceColour);
    textDrawGlyphString(x + 0x44, y + 4, D_0069D6E0, MENU_PARTY_TEXT_COLOR);
    textDrawDecimal(x + 0x4D, y + 4, maxHp, 5, MENU_PARTY_TEXT_COLOR);
    fontObjSetColor(faceColour);
    textDrawDecimal(x + 0x18, y + 4, hp, 5, shadowColour);
    fontObjResetColor();
}

void func_00224B40(int x, int y, int ep, int maxEp)
{
    fontObjSetColor(0x3F800000807D763FLL);
    func_00224608(1, x, y, 0x3F80000080572000LL);
    fontObjResetColor();
    textDrawGlyphString(x + 0x44, y + 4, D_0069D6E0, MENU_PARTY_TEXT_COLOR);
    textDrawDecimal(x + 0x29, y + 4, ep, 3, MENU_PARTY_TEXT_COLOR);
    textDrawDecimal(x + 0x4D, y + 4, maxEp, 3, MENU_PARTY_TEXT_COLOR);
}

void func_00224C40(int x, int y, int level)
{
    fontObjSetColor(LICENSE_MARK_SHADOW);
    func_00224608(4, x + 1, y + 3, LICENSE_COLOR_IDLE);
    fontObjSetColor(0x3F80000080805939LL);
    func_00224608(4, x, y, LICENSE_COLOR_IDLE);
    fontObjResetColor();
    textDrawDecimalShadow(x + 0x14, y + 4, level, 2, LICENSE_COLOR_IDLE);
}

void func_00224CF8(int x, int y, int hp, int maxHp)
{
    u64 shadowColour;
    u64 faceColour;

    fontObjSetColor(LICENSE_MARK_SHADOW);
    func_00224608(0, x + 1, y + 3, LICENSE_COLOR_IDLE);
    fontObjSetColor(0x3F800000807D763FLL);
    func_00224608(0, x, y, LICENSE_COLOR_IDLE);
    fontObjResetColor();
    func_00224740(hp, maxHp, &shadowColour, &faceColour);
    textDrawShadow(x + 0x3C, y + 4, D_0069D6E0, LICENSE_COLOR_IDLE);
    textDrawDecimalShadow(x + 0x45, y + 4, maxHp, 4, LICENSE_COLOR_IDLE);
    fontObjSetColor(faceColour);
    textDrawDecimalShadow(x + 0x18, y + 4, hp, 4, LICENSE_COLOR_IDLE);
    fontObjResetColor();
}

void func_00224E20(int x, int y, int ep, int maxEp)
{
    fontObjSetColor(LICENSE_MARK_SHADOW);
    func_00224608(1, x + 1, y + 3, LICENSE_COLOR_IDLE);
    fontObjSetColor(0x3F800000807D763FLL);
    func_00224608(1, x, y, LICENSE_COLOR_IDLE);
    fontObjResetColor();
    textDrawShadow(x + 0x3C, y + 4, D_0069D6E0, LICENSE_COLOR_IDLE);
    textDrawDecimalShadow(x + 0x21, y + 4, ep, 3, LICENSE_COLOR_IDLE);
    textDrawDecimalShadow(x + 0x45, y + 4, maxEp, 3, LICENSE_COLOR_IDLE);
}

void func_00224F20(int x, int y, int hp, int maxHp)
{
    u64 shadowColour;
    u64 faceColour;

    fontObjSetColor(LICENSE_MARK_SHADOW);
    func_00224608(0, x + 1, y + 3, LICENSE_COLOR_IDLE);
    fontObjSetColor(0x3F800000807D763FLL);
    func_00224608(0, x, y, LICENSE_COLOR_IDLE);
    fontObjResetColor();
    func_00224740(hp, maxHp, &shadowColour, &faceColour);
    textDrawShadow(x + 0x44, y + 4, D_0069D6E0, LICENSE_COLOR_IDLE);
    textDrawDecimalShadow(x + 0x4D, y + 4, maxHp, 5, LICENSE_COLOR_IDLE);
    fontObjSetColor(faceColour);
    textDrawDecimalShadow(x + 0x18, y + 4, hp, 5, LICENSE_COLOR_IDLE);
    fontObjResetColor();
}

void func_00225048(int x, int y, int ep, int maxEp)
{
    fontObjSetColor(LICENSE_MARK_SHADOW);
    func_00224608(1, x + 1, y + 3, LICENSE_COLOR_IDLE);
    fontObjSetColor(0x3F800000807D763FLL);
    func_00224608(1, x, y, LICENSE_COLOR_IDLE);
    fontObjResetColor();
    textDrawShadow(x + 0x44, y + 4, D_0069D6E0, LICENSE_COLOR_IDLE);
    textDrawDecimalShadow(x + 0x29, y + 4, ep, 3, LICENSE_COLOR_IDLE);
    textDrawDecimalShadow(x + 0x4D, y + 4, maxEp, 3, LICENSE_COLOR_IDLE);
}

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

/* A framed panel from three tracks: pos gives the top-left corner, size the outer width and
   height, tabs the widths of the top (current_pair[0]) and bottom (current_pair[1]) tabs. The request commands
   are command + 0..11 for the pieces, 0xC1 for the top tab mark. */
void func_002251A0(int command, struct FontTrack *pos, struct FontTrack *size, struct FontTrack *tabs)
{
    int y = pos->current_pair[1];
    int x = pos->current_pair[0];
    int width;

    packetSubmitRequest(command + 7, x, y, 1);
    x += 0x10;
    width = size->current_pair[0] - tabs->current_pair[0] - 0x30;
    packetSubmitRequestEx(command + 8, x, y, width, 0x16, 1);
    x += width;
    packetSubmitRequest(command + 9, x, y, 1);
    x += 0x10;
    width = tabs->current_pair[0];
    packetSubmitRequestEx(command + 10, x, y, width, 0x16, 1);
    packetSubmitRequest(0xC1, x + ((width - fontObjCellOffsetX(0xC1)) >> 1) + 4, y + 4, 1);
    packetSubmitRequest(command + 11, x + width, y, 1);
    y += 0x16;

    x = pos->current_pair[0];
    width = size->current_pair[1] - 0x2C;
    packetSubmitRequestEx(command + 5, x, y, 0x10, width, 1);
    packetSubmitRequestEx(command + 6, x + size->current_pair[0] - 0x10, y, 0x10, width, 1);
    y += width;

    x = pos->current_pair[0];
    packetSubmitRequest(command, x, y, 1);
    x += 0x10;
    width = tabs->current_pair[1];
    packetSubmitRequestEx(command + 1, x, y, width, 0x16, 1);
    x += width;
    packetSubmitRequest(command + 2, x, y, 1);
    x += 0x10;
    width = size->current_pair[0] - tabs->current_pair[1] - 0x30;
    packetSubmitRequestEx(command + 3, x, y, width, 0x16, 1);
    packetSubmitRequest(command + 4, x + width, y, 1);
}

extern struct FontTrack D_006A5638;
extern struct FontTrack D_006A5658;
extern struct FontTrack D_006A5678;
extern int D_003EBA18[2][4][2]; /* size and tab widths of the four panel kinds */

/* Places the shared panel tracks at (x, y) with the sizes of kind (0..3) and draws it with commands 0x94... */
void func_002253A8(int x, int y, int kind)
{
    kind &= 3;
    fontTrackSetBoth(&D_006A5638, x, y);
    fontTrackSetBoth(&D_006A5658, D_003EBA18[0][kind][0], D_003EBA18[0][kind][1]);
    fontTrackSetBoth(&D_006A5678, D_003EBA18[1][kind][0], D_003EBA18[1][kind][1]);
    func_002251A0(0x94, &D_006A5638, &D_006A5658, &D_006A5678);
}

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
/* With PARTY_PANEL_MARK_A: func_00225FE0 adds the 0x158/0x159 marks and the D_0069D748 label. */
#define PARTY_PANEL_LABEL 0x100000

extern char D_0069D748[];

void func_0021C6F8(int x, int y, int width, int height);
void func_0021D550(int x, int y, int height);

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
            panelDrawNineSlice(D_003EB290, x, y, 0xE2, 0x2B);
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
            panelDrawNineSlice(D_003EB290, x, y + 0x19, 0x9D, 0x27);
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
            panelDrawNineSlice(D_003EB290, x, y + 0x19, 0xAF, 0x27);
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
        panelDrawNineSlice(D_003EB290, x, y + 0x29, 0xAE, 0x27);
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


typedef signed char s8;

typedef int s32;

/* A cursor menu. Partial layout: only the fields this TU's functions touch.
 * The object starts with its own track (state_words: position and target), then the cursor frame
 * tracks (0 position, 1 and 2 size, 3 glow) and up to 13 items. An item's pending word is its flags
 * (0x1 selected, 0x4/0x10000 frame variants, 0x8 disabled, 0x100 party roster item); its status is the
 * party index (-1 empty). cursor is the selected index, wrapped modulo item_count; prev_cursor is the
 * one before it. layout selects the party-panel grid shape menuPlaceItemCursor jumps on
 * (menuDrawPartyPanel's layout argument); prev_layout is the one before it.
 * flags: 0x1 cursor glow, 0x2 hidden, 0x4 locks the cursor, 0x10000 the full 13-item grid (func_00226140). */
typedef struct MenuObject {
    s32 state_words[4];
    unsigned char unmodeled_10[0x1C - 0x10];
    struct FontTrack tracks[4];
    Tu104Record items[13];
    s32 cursor;
    s32 prev_cursor;
    s32 reset_word; /* only written, to 0, by func_00226140 */
    s32 item_count;
    s32 layout;
    s32 prev_layout;
    s32 flags;
} MenuObject;

void menuPlaceItemCursor(MenuObject *object, s32 index);

/* Draws an empty party slot: background, the mark icon for panel->flags, and (PARTY_PANEL_LABEL)
 * the 0x158/0x159 marks and the D_0069D748 label; fades the mark the same way menuDrawPartyPanel does. */
void func_00225FE0(MenuPartyPanel *panel, int x, int y) {
    int cornerX;
    int cornerY;

    cornerX = x + panel->x;
    cornerY = y + panel->y;
    fontObjSetOpacity(panel->opacity);
    func_0021D550(cornerX, cornerY, 0x5E);
    if (panel->flags & PARTY_PANEL_MARK_A) {
        packetSubmitRequest(0x13D, cornerX + 0x1C, cornerY - 0xC, 1);
        if (panel->flags & PARTY_PANEL_LABEL) {
            packetSubmitRequest(0x158, cornerX + 8, cornerY - 9, 0x10);
            packetSubmitRequest(0x159, cornerX + 8, cornerY - 9, 0x11);
            textDrawStringAlt(cornerX + 0x24, cornerY + 5, D_0069D748, MENU_PARTY_TEXT_COLOR);
        }
    } else if (panel->flags & PARTY_PANEL_MARK_B) {
        packetSubmitRequest(0x13F, cornerX + 0x1C, cornerY - 0xC, 1);
    } else {
        packetSubmitRequest(0x13E, cornerX + 0x1C, cornerY - 0xC, 1);
    }
    if (panel->fade != 0) {
        fontObjSetOpacity((panel->opacity * panel->fade) >> MENU_PARTY_FADE_SHIFT);
        func_002253A8(cornerX - 8, cornerY - 0x12, 3);
    }
    fontObjResetOpacity();
}

/* Resets the menu: full (the 13-item party grid, items 0-2 the slots 7-9, items 3-12 the roster) or the
 * short list of ptState.counts[1] items. */
void func_00226140(MenuObject *object, s32 full) {
    s32 index;

    for (index = 0; index < 13; index++) {
        menuResetRecord(&object->items[index]);
        if (full != 0) {
            if (index < 3) {
                object->items[index].pending = 0x10000;
                object->items[index].status = index + 7;
                if (index == 2) {
                    object->items[index].pending |= 4;
                }
            } else {
                object->items[index].status = index - 3;
                object->items[index].pending |= 0x100;
                if (index >= 9) {
                    object->items[index].pending |= 0x14;
                } else {
                    if (index < 6) {
                        object->items[index].pending |= 0x200;
                    }
                    if (index % 3 == 2) {
                        object->items[index].pending |= 4;
                    }
                }
            }
        } else if (index < 7) {
            object->items[index].status = index;
            if (index >= 3) {
                object->items[index].pending |= 4;
            }
        }
    }
    fontTrackSetBoth(object, 0, 0);
    fontTrackSetBoth(&object->tracks[0], 0, 0);
    fontTrackSetBoth(&object->tracks[3], 0, 0);
    object->layout = 0;
    object->prev_layout = 0;
    object->cursor = 0;
    object->prev_cursor = 0;
    object->reset_word = 0;
    if (full != 0) {
        object->prev_layout = object->layout = 5;
        object->item_count = 13;
        object->flags = 0x10000;
        for (index = 3; index < 13; index++) {
            object->items[index].pending |= 8;
        }
    } else {
        object->item_count = ptState.counts[1];
        object->flags = 0;
    }
}

/* Moves the cursor frame (tracks 0-2) onto item index for the current layout: track 0 to the item's first
 * track target offset by the menu's own target, tracks 1 and 2 to the frame size for that layout. */
void menuPlaceItemCursor(MenuObject *object, s32 index) {
    if (index < 0) {
        return;
    }
    if (index >= object->item_count) {
        return;
    }
    switch (object->layout) {
    case 0:
    case 1:
        if (object->items[index].pending & 4) {
            fontTrackSetTarget(&object->tracks[0],
                               object->state_words[2] + object->items[index].first_block.target_pair[0] - 0xA,
                               object->state_words[3] + object->items[index].first_block.target_pair[1] - 0xE);
            fontTrackSetTarget(&object->tracks[1], 0xF6, 0x43);
        } else {
            fontTrackSetTarget(&object->tracks[0],
                               object->state_words[2] + object->items[index].first_block.target_pair[0] - 0xA,
                               object->state_words[3] + object->items[index].first_block.target_pair[1] - 0xA);
            fontTrackSetTarget(&object->tracks[1], 0xB1, 0x62);
        }
        fontTrackSetTarget(&object->tracks[2], 0x4E, 0x20);
        break;
    case 4:
        fontTrackSetTarget(&object->tracks[0],
                           object->state_words[2] + object->items[index].first_block.target_pair[0] - 0x14,
                           object->state_words[3] + object->items[index].first_block.target_pair[1] + 9);
        break;
    case 8:
        fontTrackSetTarget(&object->tracks[0],
                           object->state_words[2] + object->items[index].first_block.target_pair[0] - 0x14,
                           object->state_words[3] + object->items[index].first_block.target_pair[1] + 0x1E);
        break;
    case 5:
    case 6:
        if (object->items[index].pending & 0x10000) {
            fontTrackSetTarget(&object->tracks[0],
                               object->state_words[2] + object->items[index].first_block.target_pair[0] - 9,
                               object->state_words[3] + object->items[index].first_block.target_pair[1] - 0xA);
            fontTrackSetTarget(&object->tracks[1], 0xC2, 0x62);
            fontTrackSetTarget(&object->tracks[2], 0x5F, 0x20);
        } else {
            fontTrackSetTarget(&object->tracks[0],
                               object->state_words[2] + object->items[index].first_block.target_pair[0] - 8,
                               object->state_words[3] + object->items[index].first_block.target_pair[1] - 0x12);
            fontTrackSetTarget(&object->tracks[1], 0x6F, 0x31);
            fontTrackSetTarget(&object->tracks[2], 0x2B, 0x12);
        }
        break;
    default:
        fontTrackSetTarget(&object->tracks[0], object->state_words[2] + object->items[index].first_block.target_pair[0],
                           object->state_words[3] + object->items[index].first_block.target_pair[1]);
        fontTrackSetTarget(&object->tracks[1], 0x12, 0xA);
        fontTrackSetTarget(&object->tracks[2], 0x12, 0xA);
        fontTrackSetTarget(&object->tracks[3], 0, 0);
        fontTrackAdvance(&object->tracks[3]);
        break;
    }
}

/* Where an item sits in a layout: D_003EB400[layout][slot], 13 slots per layout. mode picks how
 * menuRefreshItems lights the item: 0 off, 1 on, 2 only under the cursor. */
typedef struct MenuLayoutSlot {
    unsigned char mode;
    unsigned char unmodeled_01[3];
    int x;
    int y;
} MenuLayoutSlot;

extern MenuLayoutSlot D_003EB400[][13];

/* Lights each item for the current layout by its D_003EB400 mode: the second track's target is x 0x80 when
 * lit, else 0, and y 0x80 when selected (pending 0x1). A party item (pending 0x100) with a roster entry is
 * lit only under the cursor in layout 7; one without, in rows 3-8 of layouts 5 and 6, is lit when slot
 * 7 + column holds a member. With object flags 0x1 a disabled item's x target is quartered. */
void menuRefreshItems(MenuObject *object) {
    s32 index;
    s32 lit;
    s32 layout;
    s32 mode;
    s32 status;

    lit = 0;
    for (index = 0; index < object->item_count; index++) {
        layout = object->layout;
        mode = D_003EB400[layout][index].mode;
        switch (mode) {
        case 0:
        case 1:
            lit = mode;
            break;
        case 2:
            lit = index == object->cursor;
            break;
        }
        status = object->items[index].status;
        if (status != -1 && (object->items[index].pending & 0x100)) {
            if (ptState.roster[status] != 0) {
                if (layout == 7) {
                    lit = index == object->cursor;
                }
            } else if ((layout >= 5 && layout < 7) && (index >= 3 && index < 9)) {
                lit = ptState.slots[index % 3 + 7] != 0;
            }
        }
        if (lit) {
            fontTrackSetTarget(&object->items[index].second_block, 0x80,
                               (object->items[index].pending & 1) ? 0x80 : 0);
        } else {
            fontTrackSetTarget(&object->items[index].second_block, 0,
                               (object->items[index].pending & 1) ? 0x80 : 0);
        }
        if ((object->flags & 1) && (object->items[index].pending & 8)) {
            object->items[index].second_block.target_pair[0] >>= 2;
        }
    }
}

/* Advances every item's tracks, or only redraws the first track of an item whose second track has
 * a nonzero x target; draws every item's second track, then sets the glow track's target from flags
 * 0x1, draws the frame tracks, moves the cursor frame onto the cursor item and draws the menu's own track. */
void menuDraw(MenuObject *object) {
    s32 index;

    index = 0;
    menuRefreshItems(object);
    if (object->item_count > 0) {
        do {
            if (object->items[index].second_block.target_pair[0] != 0) {
                fontTrackEase(&object->items[index].first_block);
            } else {
                fontTrackAdvance(&object->items[index].second_block);
                fontTrackAdvance(&object->items[index].first_block);
            }
            fontTrackEase(&object->items[index].second_block);
            index += 1;
        } while (index < object->item_count);
    }
    if (object->flags & 1) {
        fontTrackSetTarget(&object->tracks[3], 0x80, 0);
    } else {
        fontTrackSetTarget(&object->tracks[3], 0, 0);
    }
    fontTrackEase(&object->tracks[0]);
    fontTrackEase(&object->tracks[3]);
    fontTrackEase(&object->tracks[1]);
    fontTrackEase(&object->tracks[2]);
    menuPlaceItemCursor(object, object->cursor);
    fontTrackEase(object);
}

/* Draws the items: party items from their roster entry (layout 7 draws them as layout 3), empty grid slots
 * of layouts 5 and 6 as empty panels, the rest from their slot. flags 0x2 hides the menu; flags 0x4 draws
 * the blinking band on every lit filled slot (restoring the cursor track afterwards); layouts 4 and 8 draw
 * the band at the cursor while flags 0x1 is set, the others draw the frame 0x88. */
void func_00226948(MenuObject *object) {
    s32 index;
    MenuPartyEntry *entry;
    struct FontTrack savedTrack;

    if (object->flags & 2) {
        return;
    }
    for (index = 0; index < object->item_count; index++) {
        if (object->items[index].status == -1) {
            continue;
        }
        if (object->items[index].pending & 0x100) {
            entry = (MenuPartyEntry *) ptState.roster[object->items[index].status];
            if (entry != 0) {
                if (object->layout == 7) {
                    menuDrawPartyPanel((MenuPartyPanel *) &object->items[index], entry, object->state_words[0],
                                       object->state_words[1], 3);
                } else {
                    menuDrawPartyPanel((MenuPartyPanel *) &object->items[index], entry, object->state_words[0],
                                       object->state_words[1], object->layout);
                }
            } else if (object->layout == 5 || object->layout == 6) {
                if (index >= 3 && index < 9) {
                    if (ptState.slots[index % 3 + 7] != 0) {
                        func_00225FE0((MenuPartyPanel *) &object->items[index], object->state_words[0],
                                      object->state_words[1]);
                    }
                } else {
                    func_00225FE0((MenuPartyPanel *) &object->items[index], object->state_words[0],
                                  object->state_words[1]);
                }
            }
        } else {
            menuDrawPartyPanel((MenuPartyPanel *) &object->items[index],
                               (MenuPartyEntry *) ptState.slots[object->items[index].status],
                               object->state_words[0], object->state_words[1], object->layout);
        }
    }
    if (object->flags & 4) {
        savedTrack = object->tracks[0];
        if (xglTickGet() & 1) {
            fontObjSetOpacity(0x20);
        }
        for (index = 0; index < object->item_count; index++) {
            if (object->items[index].status != -1 && object->items[index].second_block.target_pair[0] > 0
                && ptState.slots[object->items[index].status] != 0) {
                menuPlaceItemCursor(object, index);
                fontTrackAdvance(&object->tracks[0]);
                menuDrawAnimatedBand(object->tracks[0].current_pair[0], object->tracks[0].current_pair[1]);
            }
        }
        fontObjResetOpacity();
        object->tracks[0] = savedTrack;
    } else if (object->layout == 4 || object->layout == 8) {
        if (object->flags & 1) {
            menuDrawAnimatedBand(object->tracks[0].current_pair[0], object->tracks[0].current_pair[1]);
        }
    } else {
        fontObjSetOpacity(object->tracks[3].current_pair[0]);
        func_002251A0(0x88, &object->tracks[0], &object->tracks[1], &object->tracks[2]);
        fontObjResetOpacity();
    }
}

void func_002275D0(MenuObject *object, s32 enableMask);

/* Switches the grid to layout (the grid variant 5-8 of layouts 0, 1, 3 and 4 when flags 0x10000 is set),
 * sends every item's first track toward its slot (layout 2 rotates the slots so the cursor item takes
 * slot 1 and the item before it slot 0), and re-enables the items for the new layout.
 * Defined without a prototype: menuHide calls it
 * with no arguments and the asm passes menuHide's own a0/a1 through. */
void menuSetMode(object, layout)
    MenuObject *object;
    s32 layout;
{
    s32 index;
    s32 slot;

    if (object->flags & 0x10000) {
        switch (layout) {
        case 0:
            layout = 5;
            break;
        case 1:
            layout = 6;
            break;
        case 3:
            layout = 7;
            break;
        case 4:
            layout = 8;
            break;
        }
    }
    object->prev_layout = object->layout;
    object->layout = layout;
    for (index = 0; index < object->item_count; index++) {
        slot = index;
        if (object->item_count != 0 && layout == 2) {
            slot = index - object->cursor + 1;
            slot += object->item_count;
            slot %= object->item_count;
        }
        fontTrackSetTarget(&object->items[index].first_block, D_003EB400[layout][slot].x,
                           D_003EB400[layout][slot].y);
        if (object->items[index].second_block.target_pair[0] == 0) {
            fontTrackAdvance(&object->items[index].first_block);
        }
    }
    menuRefreshItems(object);
    if ((object->flags & 0x10000) && object->layout != 6 && object->layout != 5) {
        func_002275D0(object, 0x401);
    } else {
        func_002275D0(object, 0xF0F);
    }
    menuPlaceItemCursor(object, object->cursor);
    fontTrackAdvance(&object->tracks[0]);
    fontTrackAdvance(&object->tracks[1]);
    fontTrackAdvance(&object->tracks[2]);
}

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

extern unsigned char D_0069D74F;

/* Wraps the cursor into 0..item_count-1 and, while it lands on a disabled item (pending bit 0x8), keeps
 * stepping in the direction it last moved. D_0069D74F is incremented before each recursive step and
 * cleared on return. */
void menuSkipDisabled(MenuObject *object) {
    s32 count;
    s32 cursor;

    count = object->item_count;
    if (count <= 0) {
        object->prev_cursor = 0;
        object->cursor = 0;
    } else {
        cursor = object->cursor;
        while (object->cursor < 0) {
            object->cursor += object->item_count;
        }
        object->cursor %= count;
        if (object->items[object->cursor].pending & 8) {
            if (cursor - object->prev_cursor < 0) {
                object->cursor = cursor - 1;
                D_0069D74F++;
                menuSkipDisabled(object);
                D_0069D74F = 0;
                return;
            }
            object->cursor = cursor + 1;
            D_0069D74F++;
            menuSkipDisabled(object);
        }
    }
    D_0069D74F = 0;
}

void seVoiceStartDefault(int);

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

/* Moves the cursor to the nearest enabled item in direction (0 up, 1 right, 2 down, 3 left), measured
 * between the items' first tracks; the cursor sound plays when the cursor moved. Vertical moves take the
 * item nearest along the axis within 45 degrees; horizontal moves the one nearest across the axis within
 * about 63 degrees, then the nearest along it. */
void menuNavigate(MenuObject *object, s32 direction) {
    s32 cursorAdjX;
    s32 cursorAdjY;
    s32 best;
    s32 bestAcross;
    s32 bestAlong;
    s32 index;
    s32 itemAdjX;
    s32 itemAdjY;
    s32 along;
    s32 across;
    s32 old_cursor;

    if ((object->flags & 0x10000) && object->cursor >= 3) {
        cursorAdjX = 0;
        cursorAdjY = 0;
    } else {
        cursorAdjX = 0x28;
        cursorAdjY = 0x14;
    }
    best = -1;
    bestAcross = 9999;
    bestAlong = 9999;
    for (index = 0; index < object->item_count; index++) {
        if (object->cursor == index) {
            continue;
        }
        if (object->items[index].pending & 8) {
            continue;
        }
        if ((object->flags & 0x10000) && index >= 3) {
            itemAdjX = 0;
            itemAdjY = 0;
        } else {
            itemAdjX = 0x1E;
            itemAdjY = 0x1E;
        }
        switch (direction) {
        default:
            continue;
        case 0:
            along = object->items[index].first_block.current_pair[1] + itemAdjY
                    - object->items[object->cursor].first_block.current_pair[1] - cursorAdjY;
            if (along >= 0) {
                continue;
            }
            across = object->items[index].first_block.current_pair[0] + itemAdjX
                     - object->items[object->cursor].first_block.current_pair[0] - cursorAdjX;
            along = (along < 0) ? -along : along;
            across = (across < 0) ? -across : across;
            if (along < across) {
                continue;
            }
            if (along >= bestAlong) {
                continue;
            }
            bestAcross = across;
            bestAlong = along;
            best = index;
            break;
        case 2:
            along = object->items[index].first_block.current_pair[1] + itemAdjY
                    - object->items[object->cursor].first_block.current_pair[1] - cursorAdjY;
            if (along <= 0) {
                continue;
            }
            across = object->items[index].first_block.current_pair[0] + itemAdjX
                     - object->items[object->cursor].first_block.current_pair[0] - cursorAdjX;
            along = (along < 0) ? -along : along;
            across = (across < 0) ? -across : across;
            if (along < across) {
                continue;
            }
            if (along >= bestAlong) {
                continue;
            }
            bestAcross = across;
            bestAlong = along;
            best = index;
            break;
        case 3:
            along = object->items[index].first_block.current_pair[0] + itemAdjX
                    - object->items[object->cursor].first_block.current_pair[0] - cursorAdjX;
            if (along >= 0) {
                continue;
            }
            across = object->items[index].first_block.current_pair[1] + itemAdjY
                     - object->items[object->cursor].first_block.current_pair[1] - cursorAdjY;
            along = (along < 0) ? -along : along;
            across = (across < 0) ? -across : across;
            if (along * 2 < across) {
                continue;
            }
            if (across > bestAcross) {
                continue;
            }
            if (across == bestAcross && along >= bestAlong) {
                continue;
            }
            bestAcross = across;
            bestAlong = along;
            best = index;
            break;
        case 1:
            along = object->items[index].first_block.current_pair[0] + itemAdjX
                    - object->items[object->cursor].first_block.current_pair[0] - cursorAdjX;
            if (along <= 0) {
                continue;
            }
            across = object->items[index].first_block.current_pair[1] + itemAdjY
                     - object->items[object->cursor].first_block.current_pair[1] - cursorAdjY;
            along = (along < 0) ? -along : along;
            across = (across < 0) ? -across : across;
            if (along * 2 < across) {
                continue;
            }
            if (across > bestAcross) {
                continue;
            }
            if (across == bestAcross && along >= bestAlong) {
                continue;
            }
            bestAcross = across;
            bestAlong = along;
            best = index;
            break;
        }
    }
    if (best != -1) {
        old_cursor = object->cursor;
        menuSetCursor(object, best);
        if (old_cursor != object->cursor) {
            seVoiceStartDefault(0x01000001);
        }
    }
}

/* Highlights item selected (flags bit 0x1 set, second track's y target 0x80) and clears every other item. */
void menuSetSelected(MenuObject *object, s32 selected) {
    s32 index;

    for (index = 0; index < object->item_count; index++) {
        if (index == selected) {
            object->items[index].second_block.target_pair[1] = 0x80;
            object->items[index].pending |= 1;
        } else {
            object->items[index].second_block.target_pair[1] = 0;
            object->items[index].pending &= ~1;
        }
    }
}

/* Sets or clears flags bit 0x400 on every item's pending word. */
void func_00227408(MenuObject *object, s32 enable) {
    s32 index;

    for (index = 0; index < object->item_count; index++) {
        if (enable != 0) {
            object->items[index].pending |= 0x400;
        } else {
            object->items[index].pending &= ~0x400;
        }
    }
}

/* Draws item index highlighted: its first track is moved to (0x11, 0x100) or, from item 6 on, (0x11, 0x121)
 * and its opacity set to 0x80 for the draw, then both are restored. An item whose status (the party
 * roster index) is negative is skipped; an empty entry draws the empty slot. */
void func_00227458(MenuObject *object, s32 index) {
    struct FontTrack savedTrack;
    s32 savedOpacity;
    MenuPartyEntry *entry;

    if (object->items[index].status < 0) {
        return;
    }
    savedTrack = object->items[index].first_block;
    savedOpacity = object->items[index].second_block.current_pair[0];
    if (index >= 6) {
        fontTrackSetBoth(&object->items[index].first_block, 0x11, 0x121);
    } else {
        fontTrackSetBoth(&object->items[index].first_block, 0x11, 0x100);
    }
    object->items[index].second_block.current_pair[0] = 0x80;
    entry = (MenuPartyEntry *) ptState.roster[object->items[index].status];
    if (entry != 0) {
        menuDrawPartyPanel((MenuPartyPanel *) &object->items[index], entry, object->state_words[0],
                           object->state_words[1], object->layout);
    } else {
        func_00225FE0((MenuPartyPanel *) &object->items[index], object->state_words[0], object->state_words[1]);
    }
    object->items[index].first_block = savedTrack;
    object->items[index].second_block.current_pair[0] = savedOpacity;
}

/* Disables every item (pending bit 0x8), then enables the groups selected by enableMask and moves the
 * cursor off disabled items. Grid (object flags 0x10000): 0x100 items 0-1, 0x200 item 2, 0x1 items 0-2,
 * 0x2/0x4/0x8 items 3-5/6-8/9-12; 0x800 disables items 3-8 whose column has no member in slot 7 + column.
 * List: 0x100 items 0-2, 0x200 the rest. 0x400 disables items whose member (the roster entry for items
 * with pending 0x100, else the slot) is empty. */
#define MENU_ENABLE_NEED_BACK_MEMBER 0x800 /* grid items 3-8 need a member in slot 7 + column */

void func_002275D0(MenuObject *object, s32 enableMask) {
    s32 index;

    for (index = 0; index < object->item_count; index++) {
        object->items[index].pending |= 8;
        if (object->flags & 0x10000) {
            if (index < 3) {
                if (index < 2) {
                    if (enableMask & 0x100) {
                        object->items[index].pending &= ~8;
                    }
                } else if (enableMask & 0x200) {
                    object->items[index].pending &= ~8;
                }
                if (enableMask & 1) {
                    object->items[index].pending &= ~8;
                }
            } else if (index < 6) {
                if (enableMask & 2) {
                    object->items[index].pending &= ~8;
                }
            } else if (index < 9) {
                if (enableMask & 4) {
                    object->items[index].pending &= ~8;
                }
            } else if (enableMask & 8) {
                object->items[index].pending &= ~8;
            }
            if ((enableMask & MENU_ENABLE_NEED_BACK_MEMBER) && (index >= 3 && index < 9)
                && ptState.slots[index % 3 + 7] == 0) {
                object->items[index].pending |= 8;
            }
        } else if (index < 3) {
            if (enableMask & 0x100) {
                object->items[index].pending &= ~8;
            }
        } else if (enableMask & 0x200) {
            object->items[index].pending &= ~8;
        }
        if (enableMask & 0x400) {
            if (object->items[index].pending & 0x100) {
                if (ptState.roster[object->items[index].status] == 0) {
                    object->items[index].pending |= 8;
                }
            } else if (ptState.slots[object->items[index].status] == 0) {
                object->items[index].pending |= 8;
            }
        }
    }
    menuSkipDisabled(object);
}

void menuResetState(MenuObject *object) {
    if (object != 0) {
        object->state_words[0] = 0;
        object->state_words[1] = 0;
        object->state_words[2] = 0;
        object->state_words[3] = 0;
    }
}

/* A message being typed out in a window. msgRangeSetText stores the escape-coded text and counts its
 * visible glyphs into glyph_count; msgRangeDrawRevealed advances reveal_pos (starts at -8, a short lead-in)
 * and drawn (the token count its draw loop reaches) toward glyph_count as well. */
typedef struct MsgRange {
    unsigned char *text;
    int glyph_count;
    int reveal_pos;
    int drawn;
} MsgRange;

void msgRangeReset(MsgRange *range);

/* Stores text in the range and counts its tokens, the units msgRangeDrawRevealed reveals. Two-byte
 * characters count once; code 7 counts none; a '$' to ';' span counts once (its characters count none, the
 * closing ';' is counted); a "\\n" pair is rewritten in place to a newline (counted) plus code 7; an '&'
 * followed by two digits counts once. */
void msgRangeSetText(MsgRange *range, unsigned char *text)
{
    unsigned char *p;
    int hidden;

    if (range != 0) {
        msgRangeReset(range);
        if (text == 0) {
            range->text = 0;
            range->glyph_count = 0;
            return;
        }
        range->text = text;
        range->glyph_count = 0;
        p = text;
        hidden = 0;
        for (; *p != 0; p++) {
            if ((signed char)*p < 0) {
                p++;
            } else if (*p == 7) {
                continue;
            } else if (*p == '\\') {
                if (p[1] == 'n') {
                    p[1] = 7;
                    p[0] = '\n';
                    p++;
                }
            } else if (hidden && *p == ';') {
                hidden = 0;
            } else if (*p == '$') {
                hidden = 1;
            } else if (*p == '&') {
                if ((unsigned char)(p[1] - '0') < 10 && p[2] >= '0' && p[2] <= '9') {
                    p += 2;
                }
            }
            if (!hidden) {
                range->glyph_count++;
            }
        }
    }
}

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

/* Draws the inline icon of message code `code` at (x, y) with the given alpha and returns the width it
 * advances: codes 1-4 are kind icons on a shadow plate (0x14 wide), codes 10-12 the icons 0x131-0x133
 * (0x12 wide); any other code draws nothing and returns 0. */
int msgRangeDrawIcon(int x, int y, int code, int alpha)
{
    int width = 0;
    unsigned int index = code - 1;

    if (index < 4) {
        width = 0x14;
        fontObjSetColor(((u64)alpha << 24) | 0x3F80000000000000LL);
        packetSubmitRequest(0x44, x + 1, y - 1, 4);
    }
    fontObjSetColor(((u64)alpha << 24) | 0x3F80000000808080LL);
    switch (index) {
    case 0:
        packetSubmitRequest(0x49, x + 1, y - 1, 5);
        break;
    case 1:
        packetSubmitRequest(0x48, x + 1, y - 1, 5);
        break;
    case 2:
        packetSubmitRequest(0x4A, x + 1, y - 1, 5);
        break;
    case 3:
        packetSubmitRequest(0x47, x + 1, y - 1, 5);
        break;
    case 9:
        packetSubmitRequest(0x131, x + 1, y + 1, 4);
        width = 0x12;
        break;
    case 10:
        packetSubmitRequest(0x132, x + 1, y + 1, 4);
        width = 0x12;
        break;
    case 11:
        packetSubmitRequest(0x133, x + 1, y + 1, 4);
        width = 0x12;
        break;
    }
    fontObjResetColor();
    return width;
}

int xglFontGetStringWidth(const char *text);
/* The width probe: font tag 0x19 0x03 followed by the glyph copied into bytes 2..4. */
extern unsigned char D_0069D750[8];
/* The 0x0F 'D' alpha command; byte 3 is the alpha of the fading token. */
extern unsigned char D_0069D758[4];

/* Draws the first `drawn` tokens of the message at (x, y), fading in the ones past reveal_pos (alpha
 * 0x80 less 0x10 per token through the 0x0F 'D' font command, each drawn one more pixel to the right
 * of its place), then advances drawn and reveal_pos by up to three tokens. A newline returns to x one
 * 0x14 line down; code 7 is skipped; a "$...;" span is drawn as one escape string; "&NN" draws inline
 * icon NN; a glyph advances by 13/16 of its font width plus 1. */
void msgRangeDrawRevealed(MsgRange *range, int x, int y)
{
    unsigned char glyph[3];
    unsigned char *escape;
    unsigned char *p;
    int penX;
    int advance;
    int icon;
    int index;
    int fade;

    if (range == 0 || range->text == 0) {
        return;
    }
    textDrawSegment(0, 0, msgStyleInit, 0);
    penX = x;
    textDrawSegment(0, 0, D_0068ABF0, 0);
    p = range->text;
    advance = 0;
    escape = 0;
    icon = 0;
    for (index = 0; index < range->drawn; index++) {
        if ((signed char)*p < 0) {
            glyph[0] = *p++;
            advance = 0x10;
            glyph[1] = *p++;
            glyph[2] = 0;
        } else {
            switch (*p) {
            case '\n':
                glyph[0] = 0;
                advance = -1;
                p++;
                break;
            case 7:
                glyph[0] = 0;
                advance = 0;
                p++;
                index--;
                continue;
            case '$':
                advance = 0;
                escape = p;
                glyph[0] = 0;
                for (; *p != 0; p++) {
                    if ((signed char)*p < 0) {
                        p++;
                    } else if (*p == ';') {
                        p++;
                        glyph[0] = *p;
                        *p = 0;
                        break;
                    }
                }
                break;
            case '&':
                if ((unsigned char)(p[1] - '0') < 10 && p[2] >= '0' && p[2] <= '9') {
                    icon = (unsigned char)((p[1] - '0') * 10 + p[2] - '0');
                    p += 3;
                    break;
                }
                glyph[0] = *p++;
                glyph[1] = 0;
                advance = 0xF;
                break;
            case ' ':
                glyph[0] = *p++;
                glyph[1] = 0;
                advance = 8;
                break;
            default:
                glyph[0] = *p++;
                glyph[1] = 0;
                advance = 0xF;
                break;
            }
        }
        if (range->reveal_pos < index) {
            fade = index - range->reveal_pos;
            D_0069D758[3] = 0x80 - fade * 16;
            xglFontPrint(0, 0, 0xFFFFF0, D_0069D758);
            if (escape != 0) {
                textDrawSegment(penX + fade, y, escape, 0);
            } else if (icon != 0) {
                advance = msgRangeDrawIcon(penX + fade, y, icon, D_0069D758[3]);
            } else {
                textDrawSegment(penX + fade, y, glyph, 0);
                D_0069D750[2] = glyph[0];
                D_0069D750[3] = glyph[1];
                D_0069D750[4] = glyph[2];
                if (advance != -1) {
                    advance = (xglFontGetStringWidth(D_0069D750) * 13 >> 4) + 1;
                }
            }
        } else if (escape != 0) {
            textDrawSegment(penX, y, escape, 0);
        } else if (icon != 0) {
            advance = msgRangeDrawIcon(penX, y, icon, 0x80);
        } else {
            textDrawSegment(penX, y, glyph, 0);
            D_0069D750[2] = glyph[0];
            D_0069D750[3] = glyph[1];
            D_0069D750[4] = glyph[2];
            if (advance != -1) {
                advance = (xglFontGetStringWidth(D_0069D750) * 13 >> 4) + 1;
            }
        }
        if (escape != 0) {
            escape = 0;
            *p = glyph[0];
        }
        if (icon != 0) {
            icon = 0;
        }
        if (advance == -1) {
            penX = x;
            y += 0x14;
        } else {
            penX += advance;
        }
    }
    for (index = 0; index < 3; index++) {
        if (range->drawn < range->glyph_count) {
            range->drawn++;
        }
        if (range->reveal_pos < range->glyph_count) {
            range->reveal_pos++;
        }
    }
    textDrawSegment(0, 0, msgStyleReset, 0);
}

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

extern unsigned char msgBuffer[];

/* Appends text to msgBuffer, keeping two-byte (high-bit) characters whole, turning a "\\n" pair into a
 * newline and stopping once the buffer holds 0xFF bytes. */
void msgBufferAppend(unsigned char *text)
{
    unsigned char *dst;
    int count;
    unsigned char c;

    if (text != 0) {
        dst = msgBuffer;
        count = 0;
        while (*dst != 0) {
            if ((signed char)*dst < 0) {
                dst++;
                count++;
            }
            dst++;
            count++;
        }
        c = *text;
        while (c != 0 && count < 0xFF) {
            *dst = c;
            if ((signed char)*text < 0) {
                text++;
                dst++;
                count++;
                *dst = *text;
            } else if (*text == '\\' && text[1] == 'n') {
                text++;
                *dst = '\n';
            }
            text++;
            count++;
            c = *text;
            dst++;
        }
        *dst = 0;
    }
}

/* The two-choice (yes/no) prompt at main 0x006a57d8. +0x04 is the row the highlight is placed at (choiceUpdate
 * passes it to the placement call); +0x38 is the selected option (0 or 1, 2 once closed); the words at +0x3C
 * (msg buffer), +0x40 and +0x44 are the prompt text and the two option labels drawn by choiceDraw. */
typedef struct ChoiceState {
    int x;                                   /* 0x00: current x of the highlight-position track */
    int id;
    unsigned char unmodeled_08[0x04];
    int targetRow;                          /* 0x0C */
    unsigned char unmodeled_10[0x04];
    int rowPixelY;                          /* 0x14 */
    unsigned char unmodeled_18[0x04];
    struct FontTrack fade;                  /* 0x1C: current_pair[0] is the opacity choiceDraw applies */
    unsigned char state;                    /* 0x38 */
    unsigned char unmodeled_39[0x3C - 0x39];
    unsigned char *buffer;                  /* 0x3C */
    void *firstOptionText;                  /* 0x40: the word choiceFirstOptionText names */
    void *secondOptionText;                 /* 0x44: the word choiceSecondOptionText names */
} ChoiceState;

extern ChoiceState choiceState;

/* Starts the yes/no prompt: the horizontal track jumps straight to initialOption's slot (the
   fixed row y 0xF8), the opacity track fades in from 0 to 0x7F, and the two option labels
   (choiceSetFirstOptionText/choiceSetSecondOptionText use the same words) come from the
   menu string table. */
void choiceInit(unsigned char *text, int initialOption)
{
    fontTrackSetBoth(&choiceState, initialOption * 0x78 + 0xA0, 0xF8);
    fontTrackSetBoth(&choiceState.fade, 0, 0x7F);
    fontTrackSetTarget(&choiceState.fade, 0x7F, 0);
    choiceState.buffer = msgBuffer;
    choiceState.state = initialOption;
    msgBuffer[0] = 0;
    msgBufferAppend(text);

    choiceState.firstOptionText = menuMainTxdEntry(0x45);
    choiceState.secondOptionText = menuMainTxdEntry(0x46);
}

extern PadRecord PadData[2];

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

/* Sets the highlight track to an absolute row (no animation): the row index, the target
   the track is already at (+0x0C) and its pixel y (+0x14, row * 16). */
void func_002280E0(int row)
{
    choiceState.targetRow = row;
    choiceState.id = row;
    choiceState.rowPixelY = row * 16;
}

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

void xglFontFlush(void);

/* Draws the yes/no prompt: panel frame, prompt text, the two option labels (glyph kind 5 for
   the currently selected option, 4 for the other) and, while state read as a signed byte is
   not negative, the moving highlight band. fontTrackEase runs twice on each track, as in the asm. */
void choiceDraw(void)
{
    /* The asm addresses the opacity track first and reaches the record through it. */
    struct FontTrack *fade = &choiceState.fade;
    ChoiceState *state = &choiceState;

    fontTrackEase(fade);
    fontTrackEase(fade);
    fontObjSetOpacity(state->fade.current_pair[0]);
    func_0021DE60(0x78, 0xA0, 0x110, 0x78);
    func_0021E338(0x7C, 0xA4, 0x108, 0x70);
    textDrawSegment(0, 0, msgStyleInit, 4);
    if (state->buffer != 0) {
        textDrawSegment(0x8C, 0xAC, state->buffer, 4);
    }
    textDrawSegment(0xB4, state->id - 2, state->firstOptionText,
                    (signed char) state->state == 0 ? 5 : 4);
    textDrawSegment(0x12C, state->id - 2, state->secondOptionText,
                    (signed char) state->state == 1 ? 5 : 4);
    xglFontFlush();
    if ((signed char) state->state >= 0) {
        fontTrackEase(state);
        fontTrackEase(state);
        fontObjResetAndBuild();
        packetRequest.first = state->x;
        packetRequest.command = 0x134;
        packetRequest.second = state->id;
        packetRequest.phase = (xglTickGet() & 7) * 0x10;
        packetRequest.flags = 0x21;
        packetRequest.reset_value = 0;
        fontObjDraw(&packetRequest);
    }
    fontObjResetOpacity();
}

/* The dispatch value of the choice prompt's state byte: a negative byte is returned as-is,
   otherwise only its low two bits. */
int func_00228340(void)
{
    if ((signed char) choiceState.state < 0) {
        return (signed char) choiceState.state;
    }
    return choiceState.state & 3;
}

/* Sets the countdown's displayed text and starting count. The track at +0x04 starts at
   (0, 0x7F) and heads to (0x7F, 0): its first word, the opacity countdownDraw applies, fades
   in from 0 to 0x7F. */
void countdownInit(void *text, int count)
{
    countdownState.count = count;
    countdownState.text = msgBuffer;
    msgBuffer[0] = 0;
    msgBufferAppend(text);
    fontTrackSetBoth(&countdownState.value, 0, 0x7F);
    fontTrackSetTarget(&countdownState.value, 0x7F, 0);
}

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
