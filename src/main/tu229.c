#include "common.h"
#include "gs.h"
#include "shared.h"
#include "main/tu229.h"
#include "main/xgl_cd.h"

/* (value * 30) - 30; the unit is not read yet (caller interpAdvance, tu227). */
int interpCursorFrame(int value) {
    return (value * 30) - 30;
}

/* value - 80; the constant's meaning is not read yet (caller func_0027E048, tu225). */
int func_0027E7F8(int value) {
    return value - 80;
}

void talkLoadOmiyaScript(void);

/* Entry that loads the talk script; delegates to talkLoadOmiyaScript. */
void talkStartScriptLoad(void) {
    talkLoadOmiyaScript();
}

void talkCloseScript(void);
extern int talkActive;

/* Closes the loaded script, then clears the talk-active flag talkActive. */
void talkEndScript(void) {
    talkCloseScript();
    talkActive = 0;
}

int GameResLoadName(char *name, void **data, unsigned int *size, void (*callback)(int index, void *buffer, unsigned int size, int value));
extern void *talkScriptHandle;
extern char talkOmiyaScriptPath[];

/*
 * Starts loading "omiya\talk01.xtx" (talkOmiyaScriptPath) into the script handle
 * talkScriptHandle, then waits while the CD service reports work pending
 * (xglCdServiceIfPending, xglCdServiceIfPending in tu011).
 */
void talkLoadOmiyaScript(void) {
    GameResLoadName(talkOmiyaScriptPath, &talkScriptHandle, 0, 0);
    while (xglCdServiceIfPending() != 0) {
    }
}

int GameResBlockFreeByBuffer(void *buffer);

/* Releases the script handle talkScriptHandle and clears it. */
void talkCloseScript(void) {
    GameResBlockFreeByBuffer(talkScriptHandle);
    talkScriptHandle = 0;
}

/* Clears the talk-active flag talkActive. */
void talkDeactivate(void) {
    talkActive = 0;
}

extern int talkPendingValue;

/* Stores a pending value in talkPendingValue. */
void talkSetPending(int value) {
    talkPendingValue = value;
}

/* Returns the pending value in talkPendingValue and clears it. */
int talkTakePending(void) {
    int value;

    value = talkPendingValue;
    talkPendingValue = 0;
    return value;
}

INCLUDE_ASM("asm/main/nonmatchings/tu229", func_0027E8B8);

INCLUDE_ASM("asm/main/nonmatchings/tu229", func_0027E960);

INCLUDE_ASM("asm/main/nonmatchings/tu229", func_0027EA80);

INCLUDE_ASM("asm/main/nonmatchings/tu229", func_0027EB40);

INCLUDE_ASM("asm/main/nonmatchings/tu229", func_0027EC18);

/* Sets the talk-active flag talkActive. */
void talkSetActive(void) {
    talkActive = 1;
}

INCLUDE_ASM("asm/main/nonmatchings/tu229", func_0027ED28);

/* Screen to GS primitive coordinates: 12.4 fixed point around the 2048 centre. */
#define TALK_GS_X(x) ((int) (((float) (x) + 1792.0f) * 16.0f))
#define TALK_GS_Y(y) ((int) (((float) (y) + 1824.0f) * 16.0f))
#define TALK_ABS(value) ((value) < 0 ? -(value) : (value))

/* One A+D pair of the window packet, built in the EE scratchpad. */
typedef union TalkGsPair {
    struct {
        u64 data;
        u64 reg;
    } ad;
    long qword[2];
} TalkGsPair;

/* The talk window drawn here (a TalkWork, tu228; 0x98 bytes, func_0027ED28
 * builds one on its stack). mode is 1 for the window that shows the page
 * cursor; talkSetFlag10 sets its upper bit. The structure is 8-byte aligned:
 * func_0027ED28 copies it with ld/sd and the flag word is read with ld. */
typedef struct TalkWindow {
    int kind : 3;
    int mode : 2;
    int x;                                 /* 0x04 */
    int y;                                 /* 0x08 */
    int z;                                 /* 0x0C */
    int width;                             /* 0x10 */
    int height;                            /* 0x14 */
    unsigned char unmodeled_18[4];
    int state;                             /* 0x1C, 1 while active */
    unsigned int color;                    /* 0x20, RGBA before the fade */
    unsigned char unmodeled_24[4];
    long unmodeled_28[12];                 /* the scroll block of TalkWork */
    unsigned char unmodeled_88[4];
    unsigned int frame_counter;            /* 0x8C, 0..15 */
    unsigned char unmodeled_90[8];
} TalkWindow;

/* One piece of the frame: textured corners and edges (kind 0..13), the
 * gradient-masked middle strips (14..18) and blend changes (19..23); kind
 * TALK_PIECE_END closes the list. */
typedef struct TalkPiece {
    int kind;
    int texture_offset;                    /* TBP0 above the font texture base */
    int clut_offset;                       /* CBP above the font texture base */
    int repeat;                            /* region repeat instead of region clamp */
    int u_limit;
    int v_limit;
    int u;
    int v;
} TalkPiece;

enum {
    TALK_PIECE_STRIP_FIRST = 14,
    TALK_PIECE_STRIP_LEFT_INNER = 15,
    TALK_PIECE_MIDDLE = 16,
    TALK_PIECE_STRIP_RIGHT_INNER = 17,
    TALK_PIECE_COLOR = 19,
    TALK_PIECE_BLEND_ONE_A = 20,       /* alpha mode 1 */
    TALK_PIECE_BLEND_ZERO = 21,        /* alpha mode 0 */
    TALK_PIECE_BLEND_ONE_B = 22,       /* alpha mode 1 */
    TALK_PIECE_BLEND_ONE_C = 23,       /* alpha mode 1 */
    TALK_PIECE_END = 24,
    TALK_RECT_COUNT = 19,
};

/* The EE scratchpad, where the packet is built before it is sent. */
#define TALK_SCRATCHPAD 0x70000000
/* The font texture base, in words, plus a piece offset. */
#define TALK_FONT_WORDS(offset) ((sRender.font_texture_base << 5) + (offset))
/* Scissor of the whole 512x448 screen. */
#define TALK_SCISSOR_FULL GS_SET_SCISSOR(0, 0x1FF, 0, 0x1BF)

/* Screen rectangle of a piece (a mirrored piece has right < left). */
typedef struct TalkRect {
    long top;
    long left;
    long right;
    long bottom;
} TalkRect;

typedef struct XglPacket XglPacket;

typedef struct RenderState {
    unsigned char unmodeled_00[0x1A];
    unsigned short font_texture_base;      /* 0x1A, in 32-word units */
    unsigned char unmodeled_1c[4];
    unsigned short frame_base;             /* 0x20 */
} RenderState;

extern RenderState sRender;
extern TalkPiece talkWindowPieces[];
extern int talkActive;

XglPacket *xglPacketGetCurrent(void);
void sceVif1PkCnt(XglPacket *packet, int count);
void sceVif1PkOpenDirectHLCode(XglPacket *packet, int mode);
void sceVif1PkAddDirectDataN(XglPacket *packet, const void *data, int qwc);
void sceVif1PkCloseDirectHLCode(XglPacket *packet);
u64 func_002745A8(int buffer, int flag);
u64 func_00274650(int buffer, int flag);
int func_002746F8(int blend);
void func_002803D8(int *left, int *top, int *bottom, int *right, TalkWindow *window, int mode, float scale);
unsigned int func_002805C8(unsigned int color, float scale);
void func_00280688(int *x, int *y, TalkWindow *window);

/*
 * Draws a talk window as one direct GS packet: the frame pieces of talkWindowPieces
 * around the rectangles below, then the page cursor when the window shows one.
 */
void talkWindowDrawFrame(TalkWindow *window, int mode, float scale)
{
    int left = window->x;
    int top = window->y;
    int right = left + window->width;
    int bottom = top + window->height;
    int z = window->z;
    unsigned int color = window->color;
    TalkPiece *piece = talkWindowPieces;
    int middleFits = 1;
    TalkGsPair *pairs;
    XglPacket *packet;
    int pairCount;
    int cursor_x;
    int cursor_y;
    int scissor_left;
    int scissor_top;
    int scissor_bottom;
    int scissor_right;
    /* Screen rectangle of each piece. */
    TalkRect rect[TALK_RECT_COUNT] = {
        { top + 7, left + 4, left + 0x20, top + 0x20 },
        { bottom - 4, left + 4, left + 0x20, bottom - 0x20 },
        { top + 7, right - 4, right - 0x20, top + 0x20 },
        { bottom - 4, right - 4, right - 0x20, bottom - 0x20 },
        { top + 7, left + 0x20, right - 0x20, top + 0x20 },
        { bottom - 4, left + 0x20, right - 0x20, bottom - 0x20 },
        { top + 0x20, left + 4, left + 0x20, bottom - 0x20 },
        { top + 0x20, right - 4, right - 0x20, bottom - 0x20 },
        { top + 0x20, left + 0x20, right - 0x20, bottom - 0x20 },
        { bottom - 0x20, left + 4, left + 0x20, bottom - 4 },
        { top + 7, right - 0x20, right - 4, top + 0x20 },
        { bottom - 0x20, right - 0x20, right - 4, bottom - 4 },
        { top + 0x20, right - 0x20, right - 4, bottom - 0x20 },
        { bottom - 0x20, left + 0x20, right - 0x20, bottom - 4 },
        { top + 0x14, left + 4, left + 0x20, bottom - 0x14 },
        { top + 0x14, left + 0x20, left + 0x40, bottom - 0x14 },
        { top + 0x14, left + 0x40, right - 0x3E, bottom - 0x14 },
        { top + 0x14, right - 0x20, right - 0x3E, bottom - 0x14 },
        { top + 0x14, right - 4, right - 0x20, bottom - 0x14 },
    };

    cursor_x = 0;
    cursor_y = 0;
    /* Too narrow for the middle piece: the pieces beside it meet halfway. */
    if (rect[TALK_PIECE_MIDDLE].left >= rect[TALK_PIECE_MIDDLE].right) {
        middleFits = 0;
        rect[TALK_PIECE_STRIP_LEFT_INNER].right = rect[TALK_PIECE_STRIP_RIGHT_INNER].right = (left + right) >> 1;
    }
    if (window->state == 1 && window->mode == 1 && talkActive == 0) {
        func_00280688(&cursor_x, &cursor_y, window);
    }
    packet = xglPacketGetCurrent();
    pairs = (TalkGsPair *) TALK_SCRATCHPAD;
    pairCount = 0;
    pairs[pairCount].ad.reg = GIF_AD;
    pairs[pairCount].ad.data = ((u64) 0x10000000 << 32) | 0x8000;
    pairCount++;
    pairs[pairCount].ad.reg = GS_TEXFLUSH;
    pairs[pairCount].ad.data = 0;
    pairCount++;
    pairs[pairCount].ad.reg = GS_PABE;
    pairs[pairCount].ad.data = 0;
    pairCount++;
    pairs[pairCount].ad.reg = GS_COLCLAMP;
    pairs[pairCount].ad.data = 1;
    pairCount++;
    pairs[pairCount].ad.reg = GS_ZBUF_1;
    pairs[pairCount].ad.data = (u64) 0x9880 << 17;
    pairCount++;
    func_002803D8(&scissor_left, &scissor_top, &scissor_bottom, &scissor_right, window, mode, scale);
    color = func_002805C8(color, scale);
    pairs[pairCount].ad.reg = GS_TEX1_1;
    pairs[pairCount].ad.data = 0;
    pairCount++;
    pairs[pairCount].ad.reg = GS_SCISSOR_1;
    pairs[pairCount].ad.data = GS_SET_SCISSOR(scissor_left, scissor_right, scissor_top, scissor_bottom);
    pairCount++;
    pairs[pairCount].ad.reg = GS_TEST_1;
    pairs[pairCount].ad.data = 0;
    pairCount++;
    pairs[pairCount].ad.reg = GS_TEXA;
    pairs[pairCount].ad.data = 0;
    pairCount++;
    pairs[pairCount].ad.reg = GS_ALPHA_1;
    pairs[pairCount].ad.data = 0x44;
    pairCount++;
    pairs[pairCount].ad.reg = GS_FRAME_1;
    pairs[pairCount].ad.data = GS_SET_FRAME(sRender.frame_base, 8, 0, 0);
    pairCount++;
    pairs[pairCount].ad.data = 0x15E;
    pairs[pairCount].ad.reg = GS_PRIM;
    pairCount++;

    if (piece->kind != TALK_PIECE_END) {
        do {
            pairs[pairCount].ad.reg = GS_TEXFLUSH;
            pairs[pairCount].ad.data = 0;
            pairCount++;
            if (piece->kind < TALK_PIECE_STRIP_FIRST) {
                int u = piece->u;
                int v = piece->v;
                int r_top;
                int r_right;
                int r_left;
                int r_bottom;
                int dx;
                int dy;
                int width;
                int height;

                r_left = rect[piece->kind].left;
                r_top = rect[piece->kind].top;
                r_right = rect[piece->kind].right;
                r_bottom = rect[piece->kind].bottom;
                dx = r_right - r_left;
                dy = r_bottom - r_top;
                width = TALK_ABS(dx);
                height = TALK_ABS(dy);

                pairs[pairCount].ad.reg = GS_TEX0_1;
                pairs[pairCount].ad.data = GS_SET_TEX0(TALK_FONT_WORDS(piece->texture_offset), 8, GS_PSMT8, 5, 9, 1, 0,
                                                  TALK_FONT_WORDS(piece->clut_offset), 0, 0, 0, 1);
                pairCount++;
                if (piece->repeat != 0) {
                    pairs[pairCount].ad.reg = GS_CLAMP_1;
                    pairs[pairCount].ad.data = GS_SET_CLAMP(GS_REGION_REPEAT, GS_REGION_REPEAT, piece->u_limit, 0, piece->v_limit, 0);
                    pairCount++;
                } else {
                    pairs[pairCount].ad.reg = GS_CLAMP_1;
                    pairs[pairCount].ad.data = GS_SET_CLAMP(GS_REGION_CLAMP, GS_REGION_CLAMP, u, piece->u_limit, v, piece->v_limit);
                    pairCount++;
                }
                pairs[pairCount].ad.data = GS_SET_UV(u * 16, v * 16);
                pairs[pairCount].ad.reg = GS_UV;
                pairCount++;
                pairs[pairCount].ad.data = GS_SET_XYZ(TALK_GS_X(r_left), TALK_GS_Y(r_top), z);
                pairs[pairCount].ad.reg = GS_XYZF2;
                pairCount++;
                pairs[pairCount].ad.reg = GS_UV;
                pairs[pairCount].ad.data = GS_SET_UV((u + width) * 16, (v + height) * 16);
                pairCount++;
                pairs[pairCount].ad.reg = GS_XYZF2;
                pairs[pairCount].ad.data = GS_SET_XYZ(TALK_GS_X(r_right), TALK_GS_Y(r_bottom), z);
                pairCount++;
            } else if ((unsigned int) (piece->kind - TALK_PIECE_STRIP_FIRST) < 5) {
                if (piece->kind != TALK_PIECE_MIDDLE || middleFits == 1) {
                    int s_left;
                    int s_top;
                    int s_right;
                    int s_bottom;
                    int fade_top;
                    int middle;
                    int eighth;
                    int fade_bottom;
                    int width;
                    int height;

                    s_left = rect[piece->kind].left;
                    if (s_left < 0) {
                        s_left = 0;
                    }
                    s_top = rect[piece->kind].top;
                    s_right = rect[piece->kind].right;
                    s_bottom = rect[piece->kind].bottom;
                    fade_top = (top + bottom) >> 1;
                    middle = fade_top;
                    eighth = (bottom - top) >> 3;
                    fade_top -= eighth;
                    fade_bottom = middle + eighth;
                    pairs[pairCount].ad.data = TALK_SCISSOR_FULL;
                    pairs[pairCount].ad.reg = GS_SCISSOR_1;
                    pairCount++;
                    pairs[pairCount].ad.reg = GS_ALPHA_1;
                    pairs[pairCount].ad.data = func_002746F8(0);
                    pairCount++;
                    pairs[pairCount].ad.reg = GS_FRAME_1;
                    pairs[pairCount].ad.data = func_00274650(2, 0);
                    pairCount++;
                    /* A black alpha mask into the work frame: opaque at the edges, clear in the middle. */
                    pairs[pairCount].ad.data = 0x14E;
                    pairs[pairCount].ad.reg = GS_PRIM;
                    pairCount++;
                    pairs[pairCount].ad.reg = GS_RGBAQ;
                    pairs[pairCount].ad.data = 0x80000000;
                    pairCount++;
                    pairs[pairCount].ad.data = GS_SET_XYZ(TALK_GS_X(s_left), TALK_GS_Y(s_top), 1);
                    pairs[pairCount].ad.reg = GS_XYZ3;
                    pairCount++;
                    pairs[pairCount].ad.data = GS_SET_XYZ(TALK_GS_X(s_right), TALK_GS_Y(s_bottom), 1);
                    pairs[pairCount].ad.reg = GS_XYZF2;
                    pairCount++;
                    pairs[pairCount].ad.data = 0;
                    pairs[pairCount].ad.reg = GS_TEXFLUSH;
                    pairCount++;
                    pairs[pairCount].ad.reg = GS_TEX0_1;
                    pairs[pairCount].ad.data = GS_SET_TEX0(TALK_FONT_WORDS(piece->texture_offset), 8, GS_PSMT8, 5, 9, 1, 0,
                                                      TALK_FONT_WORDS(piece->clut_offset), 0, 0, 0, 1);
                    pairCount++;
                    pairs[pairCount].ad.reg = GS_CLAMP_1;
                    pairs[pairCount].ad.data = GS_SET_CLAMP(GS_REGION_REPEAT, GS_REGION_REPEAT, piece->u_limit, 0, piece->v_limit, 0);
                    pairCount++;
                    pairs[pairCount].ad.reg = GS_PRIM;
                    pairs[pairCount].ad.data = 0x15E;
                    pairCount++;
                    pairs[pairCount].ad.data = 0x80808080;
                    pairs[pairCount].ad.reg = GS_RGBAQ;
                    pairCount++;
                    pairs[pairCount].ad.reg = GS_UV;
                    pairs[pairCount].ad.data = 0;
                    pairCount++;
                    width = s_right - s_left;
                    height = s_bottom - s_top;
                    width = TALK_ABS(width);
                    height = TALK_ABS(height);
                    pairs[pairCount].ad.reg = GS_XYZ3;
                    pairs[pairCount].ad.data = GS_SET_XYZ(TALK_GS_X(s_left), TALK_GS_Y(s_top), z);
                    pairCount++;
                    pairs[pairCount].ad.reg = GS_UV;
                    pairs[pairCount].ad.data = GS_SET_UV(width * 16, height * 16);
                    pairCount++;
                    pairs[pairCount].ad.reg = GS_XYZF2;
                    pairs[pairCount].ad.data = GS_SET_XYZ(TALK_GS_X(s_right), TALK_GS_Y(s_bottom), z);
                    pairCount++;
                    pairs[pairCount].ad.reg = GS_TEXFLUSH;
                    pairs[pairCount].ad.data = 0;
                    pairCount++;
                    pairs[pairCount].ad.data = 0x14C;
                    pairs[pairCount].ad.reg = GS_PRIM;
                    pairCount++;
                    pairs[pairCount].ad.reg = GS_RGBAQ;
                    pairs[pairCount].ad.data = 0x80000000;
                    pairCount++;
                    pairs[pairCount].ad.reg = GS_XYZ3;
                    pairs[pairCount].ad.data = GS_SET_XYZ(TALK_GS_X(s_left), TALK_GS_Y(s_top), z);
                    pairCount++;
                    pairs[pairCount].ad.reg = GS_XYZ3;
                    pairs[pairCount].ad.data = GS_SET_XYZ(TALK_GS_X(s_right), TALK_GS_Y(s_top), z);
                    pairCount++;
                    pairs[pairCount].ad.reg = GS_RGBAQ;
                    pairs[pairCount].ad.data = 0;
                    pairCount++;
                    pairs[pairCount].ad.reg = GS_XYZF2;
                    pairs[pairCount].ad.data = GS_SET_XYZ(TALK_GS_X(s_left), TALK_GS_Y(fade_top), z);
                    pairCount++;
                    pairs[pairCount].ad.reg = GS_XYZF2;
                    pairs[pairCount].ad.data = GS_SET_XYZ(TALK_GS_X(s_right), TALK_GS_Y(fade_top), z);
                    pairCount++;
                    pairs[pairCount].ad.data = GS_SET_XYZ(TALK_GS_X(s_left), TALK_GS_Y(fade_bottom), z);
                    pairs[pairCount].ad.reg = GS_XYZ3;
                    pairCount++;
                    pairs[pairCount].ad.data = GS_SET_XYZ(TALK_GS_X(s_right), TALK_GS_Y(fade_bottom), z);
                    pairs[pairCount].ad.reg = GS_XYZ3;
                    pairCount++;
                    pairs[pairCount].ad.data = 0x80000000;
                    pairs[pairCount].ad.reg = GS_RGBAQ;
                    pairCount++;
                    pairs[pairCount].ad.data = GS_SET_XYZ(TALK_GS_X(s_left), TALK_GS_Y(s_bottom), z);
                    pairs[pairCount].ad.reg = GS_XYZF2;
                    pairCount++;
                    pairs[pairCount].ad.reg = GS_XYZF2;
                    pairs[pairCount].ad.data = GS_SET_XYZ(TALK_GS_X(s_right), TALK_GS_Y(s_bottom), z);
                    pairCount++;
                    pairs[pairCount].ad.reg = GS_TEXFLUSH;
                    pairs[pairCount].ad.data = 0;
                    pairCount++;
                    pairs[pairCount].ad.reg = GS_SCISSOR_1;
                    pairs[pairCount].ad.data = GS_SET_SCISSOR(scissor_left, scissor_right, scissor_top, scissor_bottom);
                    pairCount++;
                    /* Then the work frame, masked, through the scissor of the window. */
                    pairs[pairCount].ad.reg = GS_FRAME_1;
                    pairs[pairCount].ad.data = func_00274650(0, 0);
                    pairCount++;
                    pairs[pairCount].ad.reg = GS_TEX0_1;
                    pairs[pairCount].ad.data = func_002745A8(2, 0);
                    pairCount++;
                    pairs[pairCount].ad.reg = GS_ALPHA_1;
                    pairs[pairCount].ad.data = func_002746F8(1);
                    pairCount++;
                    pairs[pairCount].ad.reg = GS_CLAMP_1;
                    pairs[pairCount].ad.data = 0;
                    pairCount++;
                    pairs[pairCount].ad.data = 0x15E;
                    pairs[pairCount].ad.reg = GS_PRIM;
                    pairCount++;
                    pairs[pairCount].ad.reg = GS_RGBAQ;
                    pairs[pairCount].ad.data = color;
                    pairCount++;
                    pairs[pairCount].ad.data = GS_SET_UV(s_left * 16, s_top * 16);
                    pairs[pairCount].ad.reg = GS_UV;
                    pairCount++;
                    pairs[pairCount].ad.reg = GS_XYZ3;
                    pairs[pairCount].ad.data = GS_SET_XYZ(TALK_GS_X(s_left), TALK_GS_Y(s_top), z);
                    pairCount++;
                    pairs[pairCount].ad.reg = GS_UV;
                    pairs[pairCount].ad.data = GS_SET_UV(s_right * 16, s_bottom * 16);
                    pairCount++;
                    pairs[pairCount].ad.reg = GS_XYZF2;
                    pairs[pairCount].ad.data = GS_SET_XYZ(TALK_GS_X(s_right), TALK_GS_Y(s_bottom), z);
                    pairCount++;
                }
            } else {
                switch (piece->kind) {
                case TALK_PIECE_COLOR:
                    pairs[pairCount].ad.reg = GS_RGBAQ;
                    pairs[pairCount].ad.data = color;
                    pairCount++;
                    pairs[pairCount].ad.reg = GS_ALPHA_1;
                    pairs[pairCount++].ad.data = func_002746F8(0);
                    break;
                case TALK_PIECE_BLEND_ZERO:
                    pairs[pairCount].ad.reg = GS_ALPHA_1;
                    pairs[pairCount++].ad.data = func_002746F8(0);
                    break;
                case TALK_PIECE_BLEND_ONE_A:
                case TALK_PIECE_BLEND_ONE_B:
                case TALK_PIECE_BLEND_ONE_C:
                    pairs[pairCount].ad.reg = GS_ALPHA_1;
                    pairs[pairCount++].ad.data = func_002746F8(1);
                    break;
                }
            }
            piece++;
        } while (piece->kind != TALK_PIECE_END);
    }
    if (window->state == 1 && window->mode == 1 && talkActive == 0) {
        pairs[pairCount].ad.reg = GS_TEXFLUSH;
        pairs[pairCount].ad.data = 0;
        pairCount++;
        pairs[pairCount].ad.reg = GS_SCISSOR_1;
        pairs[pairCount].ad.data = GS_SET_SCISSOR(0, 0x1FF, window->y + 0x21, window->y + window->height - 0x20);
        pairCount++;
        pairs[pairCount].ad.reg = GS_RGBAQ;
        pairs[pairCount].ad.data = 0x80808080;
        pairCount++;
        pairs[pairCount].ad.reg = GS_TEX0_1;
        pairs[pairCount].ad.data = GS_SET_TEX0(TALK_FONT_WORDS(0x208), 8, GS_PSMT8, 6, 5, 1, 0,
                                              TALK_FONT_WORDS(0x228), 0, 0, 0, 1);
        pairCount++;
        pairs[pairCount].ad.reg = GS_PRIM;
        pairs[pairCount].ad.data = 0x15E;
        pairCount++;
        pairs[pairCount].ad.data = 0x0A500140;
        pairs[pairCount].ad.reg = GS_UV;
        pairCount++;
        pairs[pairCount].ad.reg = GS_XYZF2;
        pairs[pairCount].ad.data = GS_SET_XYZ(TALK_GS_X(cursor_x), TALK_GS_Y(cursor_y), z);
        pairCount++;
        pairs[pairCount].ad.data = 0x0B500280;
        pairs[pairCount].ad.reg = GS_UV;
        pairCount++;
        pairs[pairCount].ad.reg = GS_XYZF2;
        pairs[pairCount].ad.data = GS_SET_XYZ(TALK_GS_X(cursor_x + 0x14), TALK_GS_Y(cursor_y + 0xF), z);
        pairCount++;
        if (window->frame_counter >= 8) {
            cursor_y -= 4;
        }
        pairs[pairCount].ad.reg = GS_TEXFLUSH;
        pairs[pairCount].ad.data = 0;
        pairCount++;
        pairs[pairCount].ad.reg = GS_TEX0_1;
        pairs[pairCount].ad.data = GS_SET_TEX0(TALK_FONT_WORDS(0x208), 8, GS_PSMT8, 5, 5, 1, 0,
                                              TALK_FONT_WORDS(0x228), 0, 0, 0, 1);
        pairCount++;
        pairs[pairCount].ad.data = 0x15E;
        pairs[pairCount].ad.reg = GS_PRIM;
        pairCount++;
        pairs[pairCount].ad.data = 0x0A500000;
        pairs[pairCount].ad.reg = GS_UV;
        pairCount++;
        pairs[pairCount].ad.reg = GS_XYZF2;
        pairs[pairCount].ad.data = GS_SET_XYZ(TALK_GS_X(cursor_x), TALK_GS_Y(cursor_y), z);
        pairCount++;
        pairs[pairCount].ad.reg = GS_UV;
        pairs[pairCount].ad.data = 0x0B500140;
        pairCount++;
        pairs[pairCount].ad.reg = GS_XYZF2;
        pairs[pairCount].ad.data = GS_SET_XYZ(TALK_GS_X(cursor_x + 0x14), TALK_GS_Y(cursor_y + 0xF), z);
        pairCount++;
    }
    pairs[pairCount].ad.reg = GS_TEXFLUSH;
    pairs[pairCount].ad.data = 0;
    pairCount++;
    pairs[pairCount].ad.reg = GS_SCISSOR_1;
    pairs[pairCount].ad.data = TALK_SCISSOR_FULL;
    pairCount++;
    pairs[pairCount].ad.reg = GS_CLAMP_1;
    pairs[pairCount].ad.data = 0;
    pairCount++;
    pairs[0].ad.data += pairCount - 1;
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    sceVif1PkAddDirectDataN(packet, pairs, pairCount);
    sceVif1PkCloseDirectHLCode(packet);
}
