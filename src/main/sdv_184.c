#include "common.h"
#include "shared.h"
#include "main/gifState.h"
#include "main/sdv_184.h"
#include "main/sef_169.h"

void sdvClearSpecialWork(void);
extern short _sdvSpecialBuf[8];
extern void GameDefocusSet(int index, int, const int *);

/* One special record of sdvSpecialTable (32 of them, indexed by special slot). */
typedef struct SdvSpecialWork {
    unsigned char unmodeled_00[0x70];
} SdvSpecialWork;

extern void *memset(void *dest, int value, unsigned int size);
extern SdvSpecialWork sdvSpecialTable[32];

/* Reset: zeroes the slot flags and every special record, then gifStateClearModeValue. */
void sdvResetSpecialWork(void)
{
    memset(_sdvSpecialBuf, 0, sizeof(_sdvSpecialBuf));
    memset(sdvSpecialTable, 0, sizeof(sdvSpecialTable));
    gifStateClearModeValue();
}

void sdvClearSpecialWork(void)
{
    int i;

    for (i = 0; i < 8; i++) {
        if (_sdvSpecialBuf[i] != 0) {
            GameDefocusSet(i, 0, 0);
            _sdvSpecialBuf[i] = 0;
        }
    }
}

int sdvAllocSpecialWork(int kind)
{
    int slot;

    if (kind == 0) {
        if (_sdvSpecialBuf[0] == 0) {
            _sdvSpecialBuf[0] = 1;
            return 0;
        }
        return -1;
    }

    for (slot = 1; slot < 8; slot++) {
        if (_sdvSpecialBuf[slot] == 0) {
            _sdvSpecialBuf[slot] = 1;
            return slot;
        }
    }
    return -1;
}

/* Finds a free special record (index 1..31); 0 when none is free. */
INCLUDE_ASM("asm/main/nonmatchings/sdv_184", sdvFindFreeSpecialWork);

/* Clears the record of one special slot; slot 0 is kept. */
void sdvClearSpecialRecord(int slot)
{
    if (slot > 0) {
        memset(&sdvSpecialTable[slot], 0, sizeof(SdvSpecialWork));
    }
}

/* Frees the special slots listed by an object (count at +6, slot ids from +8). */
INCLUDE_ASM("asm/main/nonmatchings/sdv_184", sdvFreeSpecialSlots);

/* Runs one special: its sequence step, slot setup, then the keys of each slot. */
INCLUDE_ASM("asm/main/nonmatchings/sdv_184", sdvExecSpecial);

/* Draws the slots of one special: sdvTransOffset per slot, GameDefocusSet when allocated. */
INCLUDE_ASM("asm/main/nonmatchings/sdv_184", sdvDrawSpecial);

/* Builds an alter request from a sequence entry, then creates it through sdvCreateAlter. */
INCLUDE_ASM("asm/main/nonmatchings/sdv_184", sdvScheduleAlter);


/* One alter record: 0xD40 bytes; the active halfword is at +0xD38, the serial at +0xD3C. */
typedef struct SdvAlter {
    unsigned char unmodeled_00[0xD40];
} SdvAlter;

/* Zeroes one alter record (sdvInitAlter). */
void sdvInitAlter(SdvAlter *alter)
{
    sefMemZero(alter, sizeof(*alter));
}

/* Takes the first free alter record, stamps a serial and copies the request in. */
INCLUDE_ASM("asm/main/nonmatchings/sdv_184", sdvCreateAlter);

/* Zeroes one alter record (same code as sdvInitAlter; the callers tell them apart). */
void sdvDestroyAlter(SdvAlter *alter)
{
    sefMemZero(alter, sizeof(*alter));
}

/* Kills alter id (index | serial << 16) when it is active and the serial matches. */
INCLUDE_ASM("asm/main/nonmatchings/sdv_184", sdvKillAlter);

INCLUDE_ASM("asm/main/nonmatchings/sdv_184", func_0026A718);

INCLUDE_ASM("asm/main/nonmatchings/sdv_184", func_0026A7A0);

/* Advances one alter (calls sdvDestroyAlter among others). */
INCLUDE_ASM("asm/main/nonmatchings/sdv_184", sdvExecAlter);

typedef struct XglPacket XglPacket;

extern void func_0012B9A0(XglPacket *packet, u32 tag);
extern void sceVif1PkCnt(XglPacket *packet, int count);
extern XglPacket *xglPacketGetCurrent(void);

void sdvPacketFlushMscnt(void)
{
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    sceVif1PkCnt(packet, 0);
    func_0012B9A0(packet, 0x11000000);
    func_0012B9A0(packet, 0x17000000);
}

INCLUDE_ASM("asm/main/nonmatchings/sdv_184", func_0026AFB0);

/* Draws one alter's key frames into the packet from xglPacketGetCurrent (sdvDrawAlter). */
INCLUDE_ASM("asm/main/nonmatchings/sdv_184", sdvDrawAlter);

extern SdvAlter sdvAlterTable[49];

/* Zeroes all 49 alter records (sdvInitAlters). */
void sdvInitAlters(void)
{
    SdvAlter *alter;

    alter = sdvAlterTable;
    do {
        sdvInitAlter(alter);
        alter++;
    } while ((int) alter < (int) (sdvAlterTable + 49));
}

/* Zeroes all 49 alter records through sdvDestroyAlter (sdvDestroyAlters). */
void sdvDestroyAlters(void)
{
    SdvAlter *alter;

    alter = sdvAlterTable;
    do {
        sdvDestroyAlter(alter);
        alter++;
    } while ((int) alter < (int) (sdvAlterTable + 49));
}

/* Runs every active alter record (sdvExecAlters). */
INCLUDE_ASM("asm/main/nonmatchings/sdv_184", sdvExecAlters);

INCLUDE_ASM("asm/main/nonmatchings/sdv_184", sdvDrawAlters);

INCLUDE_ASM("asm/main/nonmatchings/sdv_184", func_0026BB78);
