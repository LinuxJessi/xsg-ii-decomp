#include "common.h"
#include "main/tu022.h"

extern int func_A9F0C0(int a, int b);
extern int func_A9F238(int a, int b);
extern void GameMovieCaptionPlay(int value);
extern void adxSetSlotFile(int a, int b);

int mpeg2(int call) {
    int handle;
    int result;

    handle = func_A9F0C0(call, 1);
    result = func_A9F238(call, handle);
    GameMovieCaptionPlay(result);
    return 1;
}

int adx(int call) {
    int handle;
    int result;

    soundSlotSetVolume(0, -1, 0);
    handle = func_A9F0C0(call, 1);
    result = func_A9F238(call, handle);
    adxSetSlotFile(0, result);
    return 1;
}

int adxLoad(int call) {
    int handle;
    int fileName;

    soundSlotSetLevel(0, 1);
    soundSlotSetVolume(0, -1, 0);
    handle = func_A9F0C0(call, 1);
    fileName = func_A9F238(call, handle);
    adxSetSlotFile(0, fileName);
    return 1;
}

int adxPlay(void) {
    soundSlotSetLevel(0, 0);
    return 1;
}
