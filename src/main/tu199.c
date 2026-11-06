#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tu199", ColorSet);

int func_A9F0A8(void *call);
int func_A9F0C0(void *call, int index);
void fadeStartIn(int frames);

/* Fadein: the overlay alpha fades from 255 to 0 over the frame count (fadeStartIn). */
int Fadein(void *call) {
    int frames;

    if (func_A9F0A8(call) != 1) {
        return 0;
    }
    frames = func_A9F0C0(call, 1);
    fadeStartIn(frames);
    return 1;
}

void fadeStartOut(int frames);

/* Fadeout: the overlay alpha fades from 0 to 255 over the frame count (fadeStartOut). */
int Fadeout(void *call) {
    int frames;

    if (func_A9F0A8(call) != 1) {
        return 0;
    }
    frames = func_A9F0C0(call, 1);
    fadeStartOut(frames);
    return 1;
}

void fadeStartColor(int frames, float red, float green, float blue);

/* Colorfade: fades the colour to red, green, blue over the frame count (fadeStartColor). */
int Colorfade(void *call) {
    float red;
    float green;
    float blue;
    int frames;

    if (func_A9F0A8(call) != 4) {
        return 0;
    }
    red = (float) func_A9F0C0(call, 1);
    green = (float) func_A9F0C0(call, 2);
    blue = (float) func_A9F0C0(call, 3);
    frames = func_A9F0C0(call, 4);
    fadeStartColor(frames, red, green, blue);
    return 1;
}

void fadeStartColorAlpha(int frames, float red, float green, float blue, float alpha);

/* Five-argument fade: colour and alpha over the frame count (fadeStartColorAlpha). */
int ColorfadeAlpha(void *call) {
    float red;
    float green;
    float blue;
    float alpha;
    int frames;

    if (func_A9F0A8(call) != 5) {
        return 0;
    }
    red = (float) func_A9F0C0(call, 1);
    green = (float) func_A9F0C0(call, 2);
    blue = (float) func_A9F0C0(call, 3);
    alpha = (float) func_A9F0C0(call, 4);
    frames = func_A9F0C0(call, 5);
    fadeStartColorAlpha(frames, red, green, blue, alpha);
    return 1;
}

void func_A9CDB0(void *name, void *table);
extern unsigned char fadeCommandTable[];
extern unsigned char D_0068D2A0[];

/* Registers the fade script commands (command table fadeCommandTable, module name string D_0068D2A0). */
void fadeRegisterCommands(void) {
    func_A9CDB0(D_0068D2A0, fadeCommandTable);
}
