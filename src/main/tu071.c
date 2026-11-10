#include "common.h"

/* Native "se" (its name pointer is at 0x3e6f00).  Argument 1 is the handle; argument 2 the
   volume, read from seDefaultVolume (the byte seVol writes) when absent; argument 3 the pan, 0x40 when
   absent.  A volume above 0 calls seVoiceStart with the same three values sePos uses;
   otherwise func_001A3D88 is called with the handle. */
int func_A9F0C0(void *call, int index);
int func_A9F1C0(void *call, int index);
float I2F(int value);
unsigned int seVoicePanVolumeFromPosition(float *position);
void seVoiceStart(int handle, unsigned int high, int low);
void func_001A3D88(int handle);
extern unsigned char seDefaultVolume[];

int seSetVolumePan(void *call) {
    int handle;
    int volume;
    int pan;

    handle = func_A9F0C0(call, 1);
    if (func_A9F1C0(call, 2) != 0) {
        volume = seDefaultVolume[0];
    } else {
        volume = func_A9F0C0(call, 2);
    }
    if (func_A9F1C0(call, 3) == 0) {
        pan = func_A9F0C0(call, 3);
    } else {
        pan = 0x40;
    }
    if (volume > 0) {
        seVoiceStart(handle, volume, pan);
    } else {
        func_001A3D88(handle);
    }
    return 1;
}

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

void seVoiceRecordSet(unsigned int voice, unsigned int volume, unsigned int pan);

int runtime_001F4BE0(void *call) {
    int handle;
    int volume;
    int pan;

    handle = func_A9F0C0(call, 1);
    if (func_A9F1C0(call, 2) == 0) {
        volume = func_A9F0C0(call, 2);
    } else {
        volume = -1;
    }
    if (func_A9F1C0(call, 3) == 0) {
        pan = func_A9F0C0(call, 3);
    } else {
        pan = 0x40;
    }
    seVoiceRecordSet(handle, volume, pan);
    return 1;
}

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

void seVolumeSet(int handle, int volume);

int runtime_001F4D48(void *call) {
    int handle;
    int volume;

    handle = func_A9F0C0(call, 1);
    if (func_A9F1C0(call, 2) == 0) {
        volume = func_A9F0C0(call, 2);
    } else {
        volume = 0;
    }
    seVolumeSet(handle, volume);
    return 1;
}

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
