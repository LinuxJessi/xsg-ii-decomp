#include "common.h"
#include "main/control_entry.h"
#include "main/elevator.h"
#include "main/tu022.h"
#include "main/tu063.h"
#include "main/xgl_menu.h"
#include "main/xgl_render.h"

#define NULL ((void *) 0)

void tyaDisplaySetting(int framebuffer);
typedef struct TyaGaugeArgs TyaGaugeArgs;
void tyaDrawGauge(TyaGaugeArgs *gauge);

/* Calls tyaDrawGauge, registers the elevator "id" module and resets the
 * elevator records (elvReset), then runs the display adjustment screen into
 * the framebuffer at 0x01000000. */
void tyaRunDisplaySetting(void) {
    /* passes a null gauge: tyaDrawGauge reads its arguments from address 0
     * (no test in tyaDrawGauge.s) */
    tyaDrawGauge(NULL);
    elvReset();
    tyaDisplaySetting(0x01000000);
}

/* Analog dead zones of the port-1 pad record (PadData); the rest of the
 * record is not read here. */
typedef struct YajimaTestPad {
    unsigned char unmodeled_00[0x4E];
    unsigned char horizontal_dead_zone;
    unsigned char vertical_dead_zone;
} YajimaTestPad;

extern YajimaTestPad PadData;

/* The clock reading xglClockRead fills (src/main/xgl_cd.c). */
typedef struct {
    unsigned char status;
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char day;
    unsigned char month;
    unsigned short year;
} XglClock;

void FontTest(int selection);
void MemcardTest(int selection);
void CdSlotTest(int selection);
void IpuMovieTest(int selection);
void SoundTest(int selection);
void MovieListTest(int selection);
void func_001E9B28(int selection);
void func_001EAB08(int selection);
void func_001ED1C0(int selection);
void func_001ED568(int selection);

extern void (*yajimaTestActions[10])(int selection);
extern XglMenuRow yajimaTestRows[];
extern XglMenuNode D_002BD440;
extern XglMenuParams yajimaTestMenu;
extern const char D_0069D198[];
extern const char D_00689490[];

void xglRenderClearColor(unsigned int color);
void xglFontDebugPrintf(int x, int y, const char *format, ...);
void xglClockRead(XglClock *clock);
float xglFRand(void);

/* Debug menu loop ("SELECT" title, clock line); the selected row runs its
 * entry of yajimaTestActions with the 1-based selection still in $a0. */
void YajimaTest(void) {
    int selection;
    XglClock clock;

    PadData.horizontal_dead_zone = 0x40;
    PadData.vertical_dead_zone = 0x40;
    xglRenderClearFrame();
    xglRenderClearColor(0x80004000);

    for (;;) {
        xglMenuInitial();
        selection = 0;
        while (yajimaTestRows[selection].text != 0) {
            selection++;
        }
        D_002BD440.item_count = selection;
        selection = 0;
        yajimaTestMenu.result_out = &selection;
        xglMenuOpen(XGL_MENU_SLOT_ANY, &yajimaTestMenu);
        while (selection == 0) {
            xglFontDebugPrintf(0, 0, D_0069D198);
            xglMenuDraw();
            xglClockRead(&clock);
            xglFontDebugPrintf(0x80, 0x10, D_00689490, clock.year, clock.month,
                               clock.day, clock.hour, clock.minute, clock.second);
            /* Draws a random number each frame and drops it: only the
             * generator's state advances. */
            xglFRand();
            xglSleep();
        }
        if (selection == -1) {
            break;
        }
        yajimaTestActions[selection - 1](selection);
        xglSleep();
    }
    xglRenderClearDepth();
}


/* Channel level 0x5A (bgm fade). Byte-identical to func_001EDC40. */
void func_001EDC20(int index) {
    soundSlotSetVolume(index, 0x5A, 0);
}

/* Channel level 0x5A; same body as func_001EDC20. */
void func_001EDC40(int index) {
    soundSlotSetVolume(index, 0x5A, 0);
}

/* Channel level 0x7A; same body as streamSetLevel. */
void func_001EDC60(int index) {
    soundSlotSetVolume(index, 0x7A, 0);
}

/* Channel level 0x7A (stream play). Byte-identical to func_001EDC60. */
int streamSetLevel(int index) {
    return soundSlotSetVolume(index, 0x7A, 0);
}

void (*yajimaTestActions[10])(int selection) = {
    FontTest,
    func_001E9B28,
    func_001EAB08,
    MemcardTest,
    CdSlotTest,
    IpuMovieTest,
    SoundTest,
    MovieListTest,
    func_001ED1C0,
    func_001ED568
};

extern const char D_0069D190[];
extern const char D_0069D188[];
extern const char D_0069D180[];
extern const char D_0069D178[];
extern const char D_00689480[];
extern const char D_0069D170[];
extern const char D_0069D168[];
extern const char D_0069D160[];
extern const char D_0069D158[];
extern const char D_0069D150[];

XglMenuRow yajimaTestRows[11] = {
    { (const char *) D_0069D190, { .result = 1 } },
    { (const char *) D_0069D188, { .result = 2 } },
    { (const char *) D_0069D180, { .result = 3 } },
    { (const char *) D_0069D178, { .result = 4 } },
    { (const char *) D_00689480, { .result = 5 } },
    { (const char *) D_0069D170, { .result = 6 } },
    { (const char *) D_0069D168, { .result = 7 } },
    { (const char *) D_0069D160, { .result = 8 } },
    { (const char *) D_0069D158, { .result = 9 } },
    { (const char *) D_0069D150, { .result = 10 } },
    { 0, { .result = -1 } }
};

const char D_00689490[24] = "%4d.%2d.%2d\n%2d.%2d.%2d";

XglMenuNode D_002BD440 = { 0, { -1, 1, 8 }, 0, { 0, 0, 0 }, 0, 0, { 0 }, yajimaTestRows };

XglMenuParams yajimaTestMenu = { 1, 0, { 0, 0 }, 8, 16, 0, { 0, 0 }, -1, &D_002BD440, 0 };

const char D_0069D198[8] = "SELECT";
const char D_0069D1A0[8] = "preload";
const char D_0069D1A8[8] = "preren";
const char D_0069D1B0[8] = "keepoff";
const char D_0069D1B8[8] = "elvMove";
