#include "common.h"

/* Native "se" (its name pointer is at 0x3e6f00).  Argument 1 is the handle; argument 2 the
   volume, read from seDefaultVolume (the byte seVol writes) when absent; argument 3 the second value, 0x40 when
   absent.  A volume above 0 calls seVoiceStart with the same three values sePos uses;
   otherwise func_001A3D88 is called with the handle. */
INCLUDE_ASM("asm/main/nonmatchings/tu071", seSetVolumePan);

int func_A9F0C0(void *call, int index);
float I2F(int value);
unsigned int seVoicePanVolumeFromPosition(float *position);
void seVoiceStart(int handle, unsigned int high, int low);
extern unsigned char seDefaultVolume[];

int seVol(void *call) {
    seDefaultVolume[0] = (unsigned char) func_A9F0C0(call, 1);
    return 1;
}

int sePos(void *call) {
    float position[3];
    int handle;
    unsigned int volumePan;

    handle = func_A9F0C0(call, 1);
    position[0] = I2F(func_A9F0C0(call, 2)) / 100.0f;
    position[1] = I2F(func_A9F0C0(call, 3)) / 100.0f;
    position[2] = I2F(func_A9F0C0(call, 4)) / 100.0f;
    volumePan = seVoicePanVolumeFromPosition(position);
    if (volumePan != 0) {
        seVoiceStart(handle, volumePan >> 8, volumePan & 0xFF);
    }
    return 1;
}

INCLUDE_ASM("asm/main/nonmatchings/tu071", runtime_001F4BE0);

void seVoiceRecordAddAt(int handle, float *position);

int bgmSePos(void *call) {
    float position[3];
    int handle;

    handle = func_A9F0C0(call, 1);
    position[0] = I2F(func_A9F0C0(call, 2)) / 100.0f;
    position[1] = I2F(func_A9F0C0(call, 3)) / 100.0f;
    position[2] = I2F(func_A9F0C0(call, 4)) / 100.0f;
    seVoiceRecordAddAt(handle, position);
    return 1;
}

INCLUDE_ASM("asm/main/nonmatchings/tu071", runtime_001F4D48);

extern signed char D_0069CFD0;

int nextBgm(void *call) {
    D_0069CFD0 = (signed char) func_A9F0C0(call, 1);
    return 1;
}

void seBgmNameSet(int result);
int func_A9F238(void *call, int value);

/* Native "bgm": argument 1 names the track; the name is resolved through func_A9F238 and
   passed to seBgmNameSet, which starts it (an empty name stops the music). */
int bgmSetTrack(void *call) {
    int trackRef;
    int trackName;

    trackRef = func_A9F0C0(call, 1);
    trackName = func_A9F238(call, trackRef);
    seBgmNameSet(trackName);
    return 1;
}
