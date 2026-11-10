#include "common.h"
#include "main/elevator.h"
#include "main/tu022.h"
#include "main/tu063.h"

void tyaDisplaySetting(int);
typedef struct TyaGaugeArgs TyaGaugeArgs;
void tyaDrawGauge(TyaGaugeArgs *gauge);

/* Draws the gauge, resets the elevator records, then runs the display
 * adjustment screen into the framebuffer at 0x01000000. */
void tyaRunDisplaySetting(void) {
    tyaDrawGauge(0);
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

/* Menu rows and nodes, spelled as in src/main/xgl_menu.c. */
typedef struct XglMenuNode XglMenuNode;

typedef union XglMenuRowLink {
    XglMenuNode *node;
    int result;
} XglMenuRowLink;

typedef struct XglMenuRow {
    const char *text;
    XglMenuRowLink next;
} XglMenuRow;

struct XglMenuNode {
    unsigned char item_count;
    struct { signed char initial_selected_item; unsigned char flags; unsigned char row_spacing; } initial;
    signed char selected_item;
    unsigned char unmodeled_05[3];
    int enter_result;
    int cancel_result;
    unsigned char unmodeled_10[8];
    XglMenuRow *rows;
};

/* The menu description xglMenuOpen copies into a menutbl entry. */
typedef struct XglMenuOpenParams {
    unsigned char type;
    unsigned char select_flags;
    unsigned char unmodeled_02[0xA];
    int default_result;
    XglMenuNode *root;
    int *result_out;
} XglMenuOpenParams;

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

extern void (*yajimaTestActions[])(int selection);
extern XglMenuRow yajimaTestRows[];
extern XglMenuNode D_002BD440;
extern XglMenuOpenParams yajimaTestMenu;
extern const char D_0069D198[];
extern const char D_00689490[];

void xglRenderClearColor(unsigned int color);
void xglMenuInitial(void);
int xglMenuOpen(int index, XglMenuOpenParams *params);
void xglMenuDraw(void);
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
        xglMenuOpen(-1, &yajimaTestMenu);
        while (selection == 0) {
            xglFontDebugPrintf(0, 0, D_0069D198);
            xglMenuDraw();
            xglClockRead(&clock);
            xglFontDebugPrintf(0x80, 0x10, D_00689490, clock.year, clock.month,
                               clock.day, clock.hour, clock.minute, clock.second);
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
