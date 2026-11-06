#include "common.h"
#include "shared.h"
#include "ov01/tu005.h"
#include "ov01/tu063.h"

/*
 * grGpInit's argument, restated from src/ov01/gr_gp_init.c (the owner keeps
 * it TU-local; header proposal in the PR): a data pointer and a length. A
 * local packet must be the whole record, because grGpInit writes both words.
 */
typedef struct GrPacket {
    void *data;
    int count;
} GrPacket;

typedef struct MenuRGBA {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} MenuRGBA;

/* The sprite colour: menuStatSpriteRGB copies a caller's MenuRGBA into it
 * byte-wise (lwl/lwr, swl/swr), while menuStatSprite/menuStatSprite2 copy the
 * whole word from an aligned local (lw/sw), so the field is word-aligned. */
typedef union MenuColor {
    unsigned int word;
    MenuRGBA rgba;
} MenuColor;

/* grPutSpr's sprite record: two corner vertices, a colour and the texel rectangle. */
typedef struct GrSprite {
    float x0, y0, z0, w0; /* +0x00 */
    float x1, y1, z1, w1; /* +0x10 */
    MenuColor color;      /* +0x20 */
    short u0;             /* +0x24 */
    short u1;             /* +0x26 */
    short v0;             /* +0x28 */
    short v1;             /* +0x2A */
} GrSprite;

#define SPRITE_Z 16777212.0f

extern void grPutSpr(GrPacket *packet, GrSprite *sprite, int flags);

/* Allocates a timed panel with the countdown step menuTimedPanelStep and the draw of menuTimedPanelDraw (proposed menuTimedPanelNew). */
INCLUDE_ASM("asm/nonmatchings/ov01/tu062", menuTimedPanelNew);

/* The menu state object that menuPanelCurrent points to; menuPanelCreate allocates
 * it. Only the offsets these functions touch are named. */
typedef struct MenuState {
    unsigned char unmodeled_00[4];
    void (*update)(struct MenuState *); /* +0x04: the state step, menuPanelStep */
    void (*draw)(struct MenuState *);   /* +0x08: the draw, menuPanelDraw */
    unsigned char unmodeled_0C[4];
    float origin[2];            /* +0x10: x and y; the panel slides toward it from off-screen */
    unsigned char unmodeled_18[0x58];
    float aux[2];               /* +0x70: (16, row) on a panel shown in a slot; role not evidenced yet */
    unsigned char unmodeled_78[0x18];
    int state;                  /* +0x90: 0 idle or counting down, 1 sliding in */
    int flag;                   /* +0x94: visible flag of a panel from menuPanelShow; the kind of a stat panel from menuStatPanelNew */
    int wait;                   /* +0x98: ticks down while state is 0 (menuStatPanelStep) */
    int value;                  /* +0x9C: the number a stat panel prints at kind 7 (menuStatPanelDraw) */
    int countdown;              /* +0xA0: ticks down while state is 0 (menuTimedPanelStep) */
} MenuState;

extern MenuState *menuPanelCurrent;

typedef signed char s8;

typedef int s32;

/* Partial layout: only the flags menuNumDrawTarget tests. */
typedef struct MenuTarget {
    unsigned char unmodeled_00[0xC];
    s32 flags;
} MenuTarget;

s32 objSlotRelease(void *);
s32 objStdMove(void *);
s32 slideSetStep(void *, s32);
s32 menuPanelRowFree(s32);

/* Update step of the timed panel (proposed menuTimedPanelStep): state 1 slides
 * the origin with slideSetStep until objStdMove reports arrival; state 0
 * counts countdown down and, at zero, frees the slot row and calls objSlotRelease. */
s32 menuTimedPanelStep(MenuState *menu)
{
  int state;
  int status;
  float *origin;

  state = menu->state;
  status = 1;
  if (state != 0) {
    if (state == 1) {
      origin = menu->origin;
      slideSetStep(origin, 2);
      status = objStdMove(origin);
      if (status != 0) {
        menu->state = 0;
      }
    }
  } else {
    status = menu->countdown - 1;
    menu->countdown = status;
    if (status <= 0) {
      menuPanelRowFree((s32) menu->origin[1]);
      status = objSlotRelease(menu);
    }
  }
  return status;
}

void *objSlotAlloc();
void menuPanelStep(MenuState *menu);
void menuPanelDraw(MenuState *menu);

/* Creates the menu panel and makes it current (proposed menuPanelCreate). */
void menuPanelCreate(void)
{
  MenuState *menu;

  menu = objSlotAlloc();
  if (menu != 0) {
    menu->update = menuPanelStep;
    menu->draw = menuPanelDraw;
    menuPanelCurrent = menu;
    ov01MatrixBlockInit(menu->origin);
    menu->state = 0;
    menu->flag = 0;
  }
}

/* Update step of the menu panel (proposed menuPanelStep): slides in while state is 1. */
void menuPanelStep(MenuState *menu)
{
  int state;
  float *origin;

  state = menu->state;
  if (state != 0) {
    if (state == 1) {
      /* separate address keeps menu->origin in its own register across both calls */
      origin = menu->origin;
      slideSetStep(origin, 2);
      if (objStdMove(origin) != 0) {
        menu->state = 0;
      }
    }
  }
}

s32 grPutSprOpen();
void grPutSprClose(void);
s32 menuStatSprite();
void menuTexFlush(void);
void grGpInit(GrPacket *);
void grPacketSend(GrPacket *);

/* Draws the panel while flag is set (proposed menuPanelDraw): a bar from the left
 * edge at y + 10 and a 128 x 20 box at the origin, both inside one grGpInit packet. */
void menuPanelDraw(MenuState *menu)
{
  int packet[4];
  int x;
  int y;

  if (menu->flag != 0) {
    x = (int) menu->origin[0];
    y = (int) menu->origin[1];
    menuTexFlush();
    grGpInit(packet);
    menuPanelTexState(packet);
    grPutSprOpen(packet);
    menuStatSprite(packet, 0, y + 0xA, x + 0x8A, 0xA, 0x12F - x, 0x4A);
    menuStatSprite(packet, x, y, 0x80, 0x14, 0x180, 0x36);
    grPutSprClose();
    grPacketSend(packet);
  }
}

s32 menuPanelRowAlloc();

/* Shows the menu panel (proposed menuPanelShow): slides in from x = -152 on a row
 * taken from menuPanelRowAlloc. */
void menuPanelShow(void)
{
  float origin_y;
  menuPanelCurrent->state = 1;
  menuPanelCurrent->flag = 1;
  origin_y = (float) menuPanelRowAlloc();
  menuPanelCurrent->origin[0] = -152.0f;
  menuPanelCurrent->aux[0] = 16.0f;
  menuPanelCurrent->aux[1] = (menuPanelCurrent->origin[1] = origin_y);
}

/* Hides the menu panel and frees its row (proposed menuPanelHide). */
void menuPanelHide(void) {
    menuPanelCurrent->state = 0;
    menuPanelCurrent->flag = 0;
    menuPanelRowFree((s32) menuPanelCurrent->origin[1]);
}

/* Allocates a stat panel of `kind`: its origin.x is minus the width word of row kind of
 * menuStatRects; kind 0 keeps y 82, other kinds take a row from menuPanelRowAlloc (proposed menuStatPanelNew). */
INCLUDE_ASM("asm/nonmatchings/ov01/tu062", menuStatPanelNew);

/* Update step of the stat panel (proposed menuStatPanelStep): slides in at speed 1
 * for kind 0 and 2 otherwise; counts wait down and frees its row at zero. */
void menuStatPanelStep(MenuState *menu)
{
  MenuState **slot;
  int state;
  int remaining;
  float *origin;

  /* The argument's address is taken, as in the original frame: the state
   * step reloads the menu through this slot. */
  slot = &menu;
  state = menu->state;
  if (state != 0) {
    if (state == 1) {
      origin = (*slot)->origin;
      slideSetStep(origin, ((*slot)->flag == 0) ? 1 : 2);
      if (objStdMove(origin) != 0) {
        (*slot)->state = 0;
      }
    }
  } else {
    remaining = menu->wait - 1;
    menu->wait = remaining;
    if (remaining <= 0) {
      menuPanelRowFree((s32) menu->origin[1]);
      objSlotRelease(menu);
    }
  }
}

/* Draws the stat panel of kind flag from its rectangle row in menuStatRects; at kind 7 it also prints value (proposed menuStatPanelDraw). */
INCLUDE_ASM("asm/nonmatchings/ov01/tu062", menuStatPanelDraw);

/* Maps menu kinds 1, 2, 3 to rows 8, 10, 9 and creates that stat panel through menuStatPanelNew;
 * other kinds do nothing (proposed menuStatPanelOpen, low confidence). */
INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA76F8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA7750);

INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA7770);

INCLUDE_ASM("asm/nonmatchings/ov01/tu062", func_00AA7900);

s32 menuNumDrawMode();

/* Prints `value` in `digits` decimal digits at style `style`, mode 0 (proposed menuNumDraw). */
void menuNumDraw(int *packet, s32 x, s32 y, s32 value, s32 digits, s32 style) {
    menuNumDrawMode(packet, x, y, value, digits, style, 0);
}

s32 func_00A907B0();

/* Same printer with the glyph mode taken from the target (proposed menuNumDrawTarget). */
void menuNumDrawTarget(s32 packet, s32 x, s32 y, s32 value, s32 digits, s32 style, MenuTarget *target) {
    s32 mode;

    mode = 2;
    if (!(target->flags & 4)) {
        mode = func_00A907B0(target) != 0;
    }
    menuNumDrawMode(packet, x, y, value, digits, style, mode);
}

/* Identical to menuNumDraw (kept as a separate function; no caller evidence separates them). */
void func_00AA7A48(s32 packet, s32 x, s32 y, s32 value, s32 digits, s32 style) {
    menuNumDrawMode(packet, x, y, value, digits, style, 0);
}

/* Decimal number printer (proposed menuNumDrawMode): packet, x, y, value, digits,
 * style and mode, the glyph set of D_00AB6708. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu062", menuNumDrawMode);

extern unsigned char menuStyleTable[];

/* Entry `index` of the 0x50-byte style table at menuStyleTable (proposed menuStyleEntry). */
void *menuStyleEntry(s32 index) {
    return (void *) (menuStyleTable + (index * 0x50));
}

/* Draws one 18 x 18 icon from the atlas at (0x48, 0x54), cell (col, row) (proposed menuIconPut). */
INCLUDE_ASM("asm/nonmatchings/ov01/tu062", menuIconPut);

/* Draws a status icon per set bit of the record's 16-bit mask, from menuStatusIconUVs (proposed menuStatusIconsDraw). */
INCLUDE_ASM("asm/nonmatchings/ov01/tu062", menuStatusIconsDraw);


void menuStatusIconPut(int *packet, s32 x, s32 y)
{
  menuStatusIconsTexState(packet);
  grPutSprOpen(packet);
  menuStatSprite(packet, x, y, 0x24, 0x11, 0x2C0, 0xAC);
  grPutSprClose();
}

/* Sprite with the colour word at menuStatSpriteColor (proposed menuStatSprite; same body as X1 menuStatSprite). */
INCLUDE_ASM("asm/nonmatchings/ov01/tu062", menuStatSprite);

/* Sprite with independent texel bounds and the colour word at menuStatSprite2Color (proposed menuStatSprite2). */
INCLUDE_ASM("asm/nonmatchings/ov01/tu062", menuStatSprite2);

void menuStatSpriteRGB(GrPacket *packet, int x, int y, int width, int height, int srcX, int srcY, const MenuRGBA *rgba) {
    GrSprite sprite;

    sprite.x0 = x;
    sprite.y0 = y;
    sprite.z0 = SPRITE_Z;
    sprite.x1 = x + width;
    sprite.y1 = y + height;
    sprite.z1 = SPRITE_Z;
    sprite.u0 = srcX;
    sprite.u1 = srcX + width + 1;
    sprite.v0 = srcY;
    sprite.v1 = srcY + height + 1;
    sprite.color.rgba = *rgba;
    grPutSpr(packet, &sprite, 0);
}

/* menuStatSprite2 with the colour copied from the caller's MenuRGBA (proposed menuStatSprite2RGB). */
INCLUDE_ASM("asm/nonmatchings/ov01/tu062", menuStatSprite2RGB);
