#include "common.h"
#include "main/tu021.h"
#include "main/tu022.h"

INCLUDE_ASM("asm/main/nonmatchings/tu021", seBankLoadFile);

extern int seBankDirectory;
int seBankLoadFile(int, int, int, int);

/* Loads the bank `name` into `slot`, its files taken from the global directory seBankDirectory. */
int seBankLoad(int slot, int name, int option) {
    return seBankLoadFile(slot, name, seBankDirectory, option);
}

void seBankLoadDefault(int slot, int name) {
    seBankLoad(slot, name, 0);
}

INCLUDE_ASM("asm/main/nonmatchings/tu021", func_001A33F8);

INCLUDE_ASM("asm/main/nonmatchings/tu021", seBgmLoad);

void seBgmLoad(int, int, int);

/* Loads song `name` (three digits, then .mid) with no option. */
void seBgmLoadDefault(int packed, int name) {
    seBgmLoad(packed, name, 0);
}

void func_00139AF0(void);
void func_0013A1B8(int, int);
int func_001A33F8(int, int, int, int);

void func_001A3668(int first, int second, int third, int fourth, int fifth) {
    func_00139AF0();
    func_001A33F8(0, third, fourth, fifth);
    if (second != 0) {
        func_0013A1B8(first, second);
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu021", func_001A3700);

void func_001394B8(int, int);
void func_00139C20(int, int);
void func_001A3700(void);
void func_001A3900(int, int);
void seVolumeSet(int, int);

/* `packed`: the high 16 bits select the mode, the low 16 bits are the id.
 * `name` is the song name for seBgmLoad; a non-null name also starts playback. */
void func_001A37E0(int packed, int name, int option) {
    int *mode_ref;
    int mode;
    int id;
    int next_id;

    id = packed;
    mode = id >> 16;
    id &= 0xffff;
    next_id = id + 0x100;
    switch (mode) {
    case 2:
    case 0:
        func_00139C20(0, id);
        func_001394B8(1, 0);
        mode_ref = &mode;
        seBgmLoad(next_id, name, option);
        if (name != 0) {
            func_001A3900(id, next_id);
            /* the address taken here makes this test reload mode */
            if (*mode_ref == 0) {
                seVolumeSet(-1, 0);
                return;
            }
        }
        return;

    case 1:
        seBgmLoad(0x10100 + id, name, option);
        if (name != 0) {
            seVolumeSet(0, 0);
            seSetMuted(id, 0);
            seVolumeSet(-1, 16);
            func_001A3700();
        }
        break;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu021", func_001A3900);

void func_00139430(int, int);
void func_00139CA8(void);

void func_001A3A18(void) {
    func_00139CA8();
    func_00139430(1, 0);
    func_001394B8(1, 0);
}

void func_0013A618(int, int);

void seSetMuted(int first, int flag) {
    func_0013A618(first, flag);
    soundSetMuted(flag);
    if (flag != 0) {
        func_001394B8(1, 0);
        return;
    }
    func_001A3700();
}

INCLUDE_ASM("asm/main/nonmatchings/tu021", seVolumeSet);

void seVoiceStart(int, int, int);

/* Starts `voice` at the default volume (-1) and centre pan (0x40). */
void seVoiceStartDefault(int voice) {
    seVoiceStart(voice, -1, 0x40);
}

INCLUDE_ASM("asm/main/nonmatchings/tu021", func_001A3B38);

INCLUDE_ASM("asm/main/nonmatchings/tu021", seVoiceStart);

INCLUDE_ASM("asm/main/nonmatchings/tu021", seVoiceSetPanVolume);

INCLUDE_ASM("asm/main/nonmatchings/tu021", func_001A3D88);

INCLUDE_ASM("asm/main/nonmatchings/tu021", func_001A3E10);
