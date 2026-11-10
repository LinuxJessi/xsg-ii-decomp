#include "common.h"
#include "shared.h"
#include "main/game_snap_shot.h"
#include "main/xgl_font.h"

#define PAUSE_TEXT_COLOR 0xFFFFFF /* white */
#define PAUSE_SCREEN_CENTER_X 256

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
/* One-shot draw hook: adds the pause backdrop to the packet, draws the title, then clears itself. */ void pauseDrawTitleHook(XglPacket *packet);

void pauseTitleCallback(FontDrawCursor *cursor, void *argument)
{
    sceVif1PkAddDirectDataN(cursor->packet, pauseBackdropPacket, 8);
}

extern void sceVif1PkCnt(XglPacket *packet, int count);
extern void sceVif1PkOpenDirectHLCode(XglPacket *packet, int mode);
extern void sceVif1PkCloseDirectHLCode(XglPacket *packet);
extern void xglFontFlush(void);

/* One-shot draw hook: adds the pause backdrop to the packet, draws the title, then clears itself. */
void pauseDrawTitleHook(XglPacket *packet)
{
    int width;

    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    sceVif1PkAddDirectDataN(packet, pauseBackdropPacket, 8);
    sceVif1PkCloseDirectHLCode(packet);
    width = xglFontGetStringWidth(pauseTitleText);
    xglFontPrint(PAUSE_SCREEN_CENTER_X - width / 2, 200, PAUSE_TEXT_COLOR, pauseTitleText);
    xglFontFlush();
    pauseDrawHook = 0;
}

/* Draws the "Pause" title centred at the top of the screen when visible is nonzero. */
void pauseDrawTitle(int visible)
{
    int width;

    if (visible != 0) {
        xglFontPrintExtFunc(PAUSE_TEXT_COLOR, pauseTitleCallback, 0);
        width = xglFontGetStringWidth(pauseTitleText);
        xglFontPrint(PAUSE_SCREEN_CENTER_X - width / 2, 200, PAUSE_TEXT_COLOR, pauseTitleText);
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
        xglFontPrint(PAUSE_SCREEN_CENTER_X - width / 2, 256, PAUSE_TEXT_COLOR, pauseSkipHintText);
    }
    width = xglFontGetStringWidth(pauseCancelHintText);
    xglFontPrint(PAUSE_SCREEN_CENTER_X - width / 2, 288, PAUSE_TEXT_COLOR, pauseCancelHintText);
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
