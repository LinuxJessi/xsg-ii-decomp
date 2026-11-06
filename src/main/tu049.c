#include "common.h"
#include "shared.h"
#include "main/game_snap_shot.h"
#include "main/xgl_font.h"

typedef struct XglPacket XglPacket;

/* The draw cursor a queued font callback receives; only its packet word is read here. */
typedef struct FontDrawCursor {
    XglPacket *packet;
} FontDrawCursor;

extern void sceVif1PkAddDirectDataN(XglPacket *packet, const void *data, int count);
extern void sceVif1PkRef(XglPacket *packet, const void *environment, int count, int mode,
                         int offset, int flags);
extern XglPacket *xglPacketGetCurrent(void);
extern void xglFontPrintExtFunc(int ot, void (*function)(FontDrawCursor *cursor, void *argument),
                                void *argument);
/* 8 quadwords of direct GIF/VIF data: the pause backdrop state added to the packet. */
extern u8 pauseBackdropPacket[];
/* "Pause" title text, in the game's string encoding (control bytes precede the glyphs). */
extern const char pauseTitleText[];
/* "button: Skip and proceed" hint. */
extern const char pauseSkipHintText[];
/* "START button: Cancel Pause" hint. */
extern const char pauseCancelHintText[];
/* One-shot draw hook: set when the pause starts, cleared after the title is drawn. */
extern void (*pauseDrawHook)(void);
void pauseDrawTitleHook(void);

void pauseTitleCallback(FontDrawCursor *cursor, void *argument)
{
    sceVif1PkAddDirectDataN(cursor->packet, pauseBackdropPacket, 8);
}

INCLUDE_ASM("asm/main/nonmatchings/tu049", pauseDrawTitleHook);

/* Draws the "Pause" title centred at the top of the screen when visible is nonzero. */
void pauseDrawTitle(int visible)
{
    int width;

    if (visible != 0) {
        xglFontPrintExtFunc(0xFFFFFF, pauseTitleCallback, 0);
        width = xglFontGetStringWidth(pauseTitleText);
        xglFontPrint(256 - width / 2, 200, 0xFFFFFF, pauseTitleText);
    }
}

void pauseStart(void)
{
    pauseDrawHook = pauseDrawTitleHook;
    GameSnapShotCheck();
    pauseDrawTitle(0);
}

void pauseRefBackdrop(void)
{
    sceVif1PkRef(xglPacketGetCurrent(), pauseBackdropPacket, 8, 0, 0, 0);
}

/* Draws the two button hints; a zero skip_hidden also draws the skip-and-proceed hint above the cancel hint. */
void pauseDrawHints(int skip_hidden)
{
    int width;

    if (skip_hidden == 0) {
        width = xglFontGetStringWidth(pauseSkipHintText);
        xglFontPrint(256 - width / 2, 256, 0xFFFFFF, pauseSkipHintText);
    }
    width = xglFontGetStringWidth(pauseCancelHintText);
    xglFontPrint(256 - width / 2, 288, 0xFFFFFF, pauseCancelHintText);
}

void pauseDrawScreen(void)
{
    pauseDrawTitle(1);
    pauseRefBackdrop();
    pauseDrawHints(0);
}

void pauseDrawTitleAndHints(int skip_hidden)
{
    pauseDrawTitle(1);
    pauseDrawHints(skip_hidden);
}
