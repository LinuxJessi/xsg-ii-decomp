#include "common.h"
#include "main/tu005.h"
#include "main/tu026.h"
#include "main/pad.h"
#include "main/xgl_font.h"
#include "main/TiWater.h"
#include "main/control_entry.h"
#include "main/test_129.h"

/* Debug test menu layout: entry count, the origin and line height of the list, and the slide offset at rest. */
#define TEST_MENU_ENTRY_COUNT 4
#define TEST_MENU_ORIGIN_X 20
#define TEST_MENU_ORIGIN_Y 20
#define TEST_MENU_LINE_HEIGHT 12
#define TEST_MENU_SLIDE_REST 10
/* Overlay loaded when the menu opens (its name is not known from this file). */
#define TEST_MENU_OVERLAY 98

extern const char D_0068C788[];
extern const char D_0068C790[];
extern PadRecord PadData;
extern int D_00631144;
void xglCdLoadOverlay(int overlay);
void xglFontSetStoredValue(int value);
void glowRunScene(void);
void func_00A89428(void);

int D_00631144 = 0x0068C728;
int D_00631148 = 0;
int D_0063114C = 0;

int tiWaterSubCount = 0;

int tiWaterDebugMode = 0;

float tiWaterEffectNum = 0.0f;

float tiWaterCx = 256.0f;

float tiWaterCy = 224.0f;

int tiWaterAlphaType = 0;

float tiWaterAlpha = 63.0f;

int tiWaterFbp = 0;

float tiWaterBlur = 10.0f;

float tiWaterDecrease = 0.0133400001f;

int tiWaterIntVals = 0;

int D_0063117C = 0;

float D_00631180 = 50.0f;

float D_00631184 = 100.0f;

/* Debug test menu: UP and DOWN move the cursor, CIRCLE runs the highlighted entry, SELECT with START leaves it. */
void testMenuRun(void)
{
    void (*callbacks[TEST_MENU_ENTRY_COUNT])(void) = {func_00A89428, glowRunScene, tiWaterSceneLoop, tiWaterSceneLoop};
    const char *labels[TEST_MENU_ENTRY_COUNT] = {"CFEventTool", "GlowTest", "WaterTest", "\xa4\xc6\xa4\xb9\xa4\xc8"};
    int selected;
    int slideOffset;
    unsigned short pressed;
    int i;

    selected = 0;
    slideOffset = TEST_MENU_SLIDE_REST;
    xglCdLoadOverlay(TEST_MENU_OVERLAY);
    for (;;) {
        xglFontSetStoredValue(D_00631144);
        if ((PadData.heldAndPressed & (PAD_SELECT | ((unsigned long long)PAD_START << 16))) ==
            (PAD_SELECT | ((unsigned long long)PAD_START << 16))) {
            break;
        }
        pressed = PadData.pressed;
        if (pressed & PAD_UP) {
            selected -= 1;
            slideOffset = 0;
        }
        if (pressed & PAD_DOWN) {
            selected += 1;
            slideOffset = 0;
        }
        if (selected < 0) {
            selected = TEST_MENU_ENTRY_COUNT - 1;
        }
        if (selected >= TEST_MENU_ENTRY_COUNT) {
            selected = 0;
        }
        if (pressed & PAD_CIRCLE) {
            callbacks[selected]();
        }
        for (i = 0; i < TEST_MENU_ENTRY_COUNT; i++) {
            if (selected == i) {
                xglFontDebugPrintf(TEST_MENU_ORIGIN_X - slideOffset, TEST_MENU_ORIGIN_Y + i * TEST_MENU_LINE_HEIGHT, labels[i]);
            } else {
                xglFontDebugPrintf(TEST_MENU_ORIGIN_X, TEST_MENU_ORIGIN_Y + i * TEST_MENU_LINE_HEIGHT, labels[i]);
            }
        }
        if (slideOffset < TEST_MENU_SLIDE_REST) {
            slideOffset += 2;
        }
        xglSleep();
    }
}

/*
 * Reads the debug file name into the next 16-byte aligned block of the test heap
 * cursor (*bufferCursor) and returns its address, or 0 for an empty file.
 * The NULL check is the assert on pData at ../test.euc.c line 95.
 */
int testLoadFile(int *bufferCursor, const char *name) {
    int buffer;
    int size;

    buffer = 0;
    size = xglDebFileSize(name);
    if (size > 0) {
        buffer = *bufferCursor;
        *bufferCursor = (buffer + size + 0xF) & ~0xF;
        if (buffer == 0) {
            assertFailed(D_0068C788, D_0068C790, 95);
        }
        /* The test heap keeps its cursor as an address word. */
        xglDebFileRead(name, (void *) buffer);
    }
    return buffer;
}

/* Converts the two-byte character at first/second in place: the lead byte is doubled and both bytes are
 * moved into the Shift-JIS ranges. Only yg2PathReadTaggedText calls it, for each high-bit character. */
void sjisToEuc(unsigned char *first, unsigned char *second)
{
    unsigned int leadDoubled;
    unsigned char trailCode;

    leadDoubled = *first << 1;
    *first = leadDoubled;
    trailCode = *second;
    if (trailCode < 0x9F) {
        if ((leadDoubled & 0xFE) < 0x3F) {
            *first = leadDoubled + 0x9F;
        } else {
            *first = leadDoubled + 0x1F;
        }
        if (*second >= 0x7F) {
            *second = *second + 0x60;
        } else {
            *second = *second + 0x61;
        }
    } else {
        if ((leadDoubled & 0xFE) < 0x3F) {
            *first = leadDoubled + 0xA0;
        } else {
            *first = leadDoubled + 0x20;
        }
        *second = *second + 2;
    }
}

/* Bump pointer of the test heap. */
static int testHeapCursor;

/* Sets the test heap cursor. */
void testHeapSetCursor(int newCursor) {
    testHeapCursor = newCursor;
}

/* Bump allocation: returns the next 16-byte aligned address and moves the cursor past size bytes. */
int testHeapAlloc(int size) {
    int buffer;
    int cursorAfter;

    buffer = (testHeapCursor + 0xF) & ~0xF;
    cursorAfter = buffer + size;
    testHeapCursor = cursorAfter;
    return buffer;
}

const char D_0068C788[8] = "pData";

const char D_0068C790[16] = "../test.euc.c";

const char D_0068C7A0[16] = "\015\002IntVal4   %f";

const char D_0068C7B0[16] = "\015\002IntVal3   %f";

const char D_0068C7C0[16] = "\015\002IntVal2   %f";

const char D_0068C7D0[16] = "\015\002IntVal1   %f";

const char D_0068C7E0[16] = "\015\002Decrease  %f";

const char D_0068C7F0[16] = "\015\002Blur      %f";

const char D_0068C800[16] = "\015\002FBP       %f";

const char D_0068C810[16] = "\015\002Alpha     %f";

const char D_0068C820[16] = "\015\002AlphaType %f";

const char D_0068C830[16] = "\015\002cy        %f";

const char D_0068C840[16] = "\015\002cx        %f";

const char D_0068C850[16] = "\015\002EffectNum %f";

const char *tiWaterDebugLabels[12] = {
    D_0068C850, D_0068C840, D_0068C830, D_0068C820, D_0068C810, D_0068C800,
    D_0068C7F0, D_0068C7E0, D_0068C7D0, D_0068C7C0, D_0068C7B0, D_0068C7A0,
};

unsigned int tiWaterDebugSteps[12] = { 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000, 0x3c23d70a, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000 };

void *tiWaterDebugValues[14] = {
    &tiWaterEffectNum, &tiWaterCx, &tiWaterCy, &tiWaterAlphaType, &tiWaterAlpha, &tiWaterFbp, &tiWaterBlur,
    &tiWaterDecrease, &tiWaterIntVals, &D_0063117C, &D_00631180, &D_00631184, 0, 0,
};

unsigned int tiWaterStateTemplate[60] = {
    0, 0, 0, 0x5100000Eu, 32769, 0xD08B4000u, 0x31EEEEEEu, 0xEE535u,
    0x31000000u, 1, 78, 0, 0x71001u, 0, 71, 0,
    0, 0, 20, 0, 1, 0, 74, 0,
    0, 0, 76, 0, 0, 0, 6, 0,
    128, 128, 128, 0, 0, 0, 0, 0,
    32760, 29175, 0xFFFFFFFFu, 0, 8192, 7168, 0, 0,
    36856, 32759, 0xFFFFFFFFu, 0, 0, 0, 63, 0,
    0, 0, 74, 0
};
unsigned int tiWaterDepthProgramSelect[2] = { 0 };
unsigned int tiWaterSurfaceGridHeader[4] = {
    32768, 0x300E4000u, 1298, 0
};
unsigned int tiWaterDepthGridHeader[4] = {
    32768, 0x302E4000u, 1298, 0
};
unsigned int tiWaterSubList[66] = { 0 };
