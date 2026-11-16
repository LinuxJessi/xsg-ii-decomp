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
extern u8 pauseBackdropPacket[128];
/* "Pause" title text, in the game's string encoding (control bytes precede the glyphs). */
extern char pauseTitleText[16];
/* "button: Skip and proceed" hint. */
extern char pauseSkipHintText[48];
/* "START button: Cancel Pause" hint. */
extern char pauseCancelHintText[48];
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

char pauseTitleText[16] = "\013\r\003\031\003Pause\031";

u8 pauseBackdropPacket[128] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 81, 1, 128, 0, 0, 0, 64, 35, 96, 238, 30, 85, 0, 0, 0, 0, 0, 0, 0, 255, 1, 0, 0, 255, 1, 64, 0, 0, 0, 0, 0, 0, 0, 1, 16, 3, 0, 0, 0, 0, 0, 71, 0, 0, 0, 0, 0, 0, 0, 68, 0, 0, 0, 0, 0, 0, 0, 66, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 64, 0, 0, 0, 248, 111, 0, 0, 247, 113, 0, 0, 0, 0, 0, 64, 0, 0, 0, 0, 248, 143, 0, 0, 247, 141, 0, 0, 0, 0, 0, 64, 0, 0, 0, 0 };

char pauseSkipHintText[48] = "\013\r\003\014 \200 \242\244\013\r\003\014\200\200\200\031\003 button: Skip and proceed";

char pauseCancelHintText[48] = "\013\r\003\031\003START button: Cancel Pause\031";

extern char D_006856F0[];
extern char D_006856D0[];
extern char D_006856B8[];
extern char D_006856A0[];
extern char D_00685688[];
extern char D_00685670[];
extern char D_00685658[];
extern char D_00685640[];
extern char D_00685628[];
extern char D_00685610[];
extern char D_006855F8[];
extern char D_006855E0[];
extern char D_006855C8[];
extern char D_006855B0[];
extern char D_00685598[];
extern char D_00685580[];
extern char D_00685568[];
extern char D_00685550[];
extern char D_00685538[];
extern char D_00685520[];
extern char D_00685508[];
extern char D_006854F0[];
extern char D_006854D8[];
extern char D_006854C0[];
extern char D_006854A8[];
extern char D_00685490[];
extern char D_00685478[];
extern char D_00685460[];
extern char D_00685448[];
extern char D_00685430[];
extern char D_00685418[];
extern char D_00685400[];
extern char D_006853E8[];
extern char D_006853D0[];
extern char D_006853B8[];
extern char D_006853A0[];
extern char D_00685388[];
extern char D_00685370[];
extern char D_00685358[];
extern char D_00685340[];
extern char D_00685328[];
extern char D_00685310[];
extern char D_006852F8[];
extern char D_006852E0[];
extern char D_006852C8[];
extern char D_006852B0[];
extern char D_00685298[];
extern char D_00685280[];
extern char D_00685268[];
extern char D_00685250[];
extern char D_00685238[];
extern char D_00685220[];
extern char D_00685208[];
extern char D_006851F0[];
char *mapObjectResourceNames[56] = {
    (char *) D_006856F0,
    (char *) D_006856D0,
    (char *) D_006856B8,
    (char *) D_006856A0,
    (char *) D_00685688,
    (char *) D_00685670,
    (char *) D_00685658,
    (char *) D_00685640,
    (char *) D_00685628,
    (char *) D_00685610,
    (char *) D_006855F8,
    (char *) D_006855E0,
    (char *) D_006855C8,
    (char *) D_006855B0,
    (char *) D_00685598,
    (char *) D_00685580,
    (char *) D_00685568,
    (char *) D_00685550,
    (char *) D_00685538,
    (char *) D_00685520,
    (char *) D_00685508,
    (char *) D_006854F0,
    (char *) D_006854D8,
    (char *) D_006854C0,
    (char *) D_006854A8,
    (char *) D_00685490,
    (char *) D_00685478,
    (char *) D_00685460,
    (char *) D_00685448,
    (char *) D_00685430,
    (char *) D_00685418,
    (char *) D_00685400,
    (char *) D_006853E8,
    (char *) D_006853D0,
    (char *) D_006853B8,
    (char *) D_006853A0,
    (char *) D_00685388,
    (char *) D_00685370,
    (char *) D_00685358,
    (char *) D_00685340,
    (char *) D_00685328,
    (char *) D_00685310,
    (char *) D_006852F8,
    (char *) D_006852E0,
    (char *) D_006852C8,
    (char *) D_006852B0,
    (char *) D_00685298,
    (char *) D_00685280,
    (char *) D_00685268,
    (char *) D_00685250,
    (char *) D_00685238,
    (char *) D_00685220,
    (char *) D_00685208,
    (char *) D_006851F0,
    0,
    0
};
