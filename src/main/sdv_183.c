#include "common.h"
#include "shared.h"
#include "main/gifState.h"
#include "main/sdv_183.h"
#include "main/sef_169.h"
#include "vu0_macro.h"

void sdvClearSpecialWork(void);
extern short _sdvSpecialBuf[8];
extern void GameDefocusSet(int index, int, const int *);

/* A packed RGBA colour; the channels are written one by one as bit-fields. */
typedef union SdvColor {
    struct {
        unsigned int red : 8;
        unsigned int green : 8;
        unsigned int blue : 8;
        unsigned int alpha : 8;
    } channel;
    int word;
} SdvColor;

/* One key of a special table (26 bytes). */
typedef struct SdvSpecialKey {
    unsigned short time;      /* +0x00 */
    unsigned short depth;     /* +0x02 */
    unsigned char color[4];   /* +0x04 */
    short x;                  /* +0x08 */
    short y;                  /* +0x0A */
    short z;                  /* +0x0C */
    short width;              /* +0x0E */
    short height;             /* +0x10 */
    short param12;            /* +0x12 */
    short param14;            /* +0x14 */
    short intensity;          /* +0x16 */
    unsigned short decrease;  /* +0x18 */
} SdvSpecialKey;

/* A key viewed as raw bytes: the char member makes the key copies alias every type. */
typedef union SdvKeyBytes {
    SdvSpecialKey key;
    char byte[sizeof(SdvSpecialKey)];
} SdvKeyBytes;

/* A special table: the kind in the low byte of the first halfword, a signed argument in its high byte. */
typedef struct SdvSpecialTable {
    unsigned short kind;      /* +0x00 */
    short part;               /* +0x02 */
    unsigned short unmodeled_04;
    SdvKeyBytes keys[1];      /* +0x06 */
} SdvSpecialTable;

/* One special record of sdvSpecialTable (32 of them, indexed by special slot). */
typedef struct SdvSpecialWork {
    SdvSpecialTable *table;   /* +0x00 */
    SdvSpecialKey key;        /* +0x04 the interpolated key */
    short frame;              /* +0x1E */
    short keyIndex;           /* +0x20 */
    unsigned char flags;      /* +0x22 bits 0-1 alpha type, bit 4 fbp, bit 6 mode set */
    signed char decreaseWait; /* +0x23 */
    unsigned short clip;      /* +0x24 */
    unsigned char unmodeled_26[10];
    unsigned char matrix[0x40]; /* +0x30 */
} SdvSpecialWork;

extern void *memset(void *dest, int value, unsigned int size);
extern SdvSpecialWork sdvSpecialTable[32];

INCLUDE_ASM("asm/main/nonmatchings/sdv_183", sdvSelectCamera);

INCLUDE_ASM("asm/main/nonmatchings/sdv_183", sdvSetCameraParam);

INCLUDE_ASM("asm/main/nonmatchings/sdv_183", func_00268010);

INCLUDE_ASM("asm/main/nonmatchings/sdv_183", sdvWrapAngle);

INCLUDE_ASM("asm/main/nonmatchings/sdv_183", func_002680A8);

INCLUDE_ASM("asm/main/nonmatchings/sdv_183", sdvProgressPrm);

INCLUDE_ASM("asm/main/nonmatchings/sdv_183", sdvScheduleCamera);

INCLUDE_ASM("asm/main/nonmatchings/sdv_183", sdvTransOffset);

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
INCLUDE_ASM("asm/main/nonmatchings/sdv_183", sdvFindFreeSpecialWork);

/* Clears the record of one special slot; slot 0 is kept. */
void sdvClearSpecialRecord(int slot)
{
    if (slot > 0) {
        memset(&sdvSpecialTable[slot], 0, sizeof(SdvSpecialWork));
    }
}

/* Frees the special slots listed by an object (count at +6, slot ids from +8). */
INCLUDE_ASM("asm/main/nonmatchings/sdv_183", sdvFreeSpecialSlots);

/* Runs one special: its sequence step, slot setup, then the keys of each slot. */
INCLUDE_ASM("asm/main/nonmatchings/sdv_183", sdvExecSpecial);

/* The owner of a special: its slot count at +6 and the slot ids from +8. */
typedef struct SdvSpecialOwner {
    unsigned char unmodeled_00[6];
    signed char slotCount;
    unsigned char unmodeled_07;
    signed char slotIds[6];
} SdvSpecialOwner;

/* D_0069DBAC value while a battle runs: the clip matrix is used and boxes stay square. */
#define SDV_BATTLE_MODE 0x1012F003

/* The 512x448 screen in pixels. */
#define SDV_SCREEN_WIDTH 0x200
#define SDV_SCREEN_HEIGHT 0x1C0

/* Bits of SdvSpecialWork.flags. */
#define SDV_FLAG_ALPHA_TYPE 0x03
#define SDV_FLAG_FBP_SHIFT 4
#define SDV_FLAG_MODE_SET_SHIFT 6
#define SDV_FLAG_MODE_SET (1 << SDV_FLAG_MODE_SET_SHIFT)

/* Depth of a kind-6 key without a depth: the far plane. */
#define SDV_DEPTH_FAR 0xFFFFF8

extern int D_0069DBAC;
extern float D_0069B378;
extern float D_0069B37C;
extern short D_005E6D60[9];

int sdvTransOffset(int *screen, int *offset, int part, int depth, float *scale, int argument, void *matrix);
void nmlModelUseSubWindow(unsigned int index, int enabled);
void *MMathNormalizeVector2(void *destination, const void *source);
void sefApplyPartTransform(float *destination, float *source, int part);
void nmlModelSetMulColor(float *color);

#define SDV_SET_COLOR(rgba, source) \
    ((rgba).channel.red = (source)->color[0], \
     (rgba).channel.green = (source)->color[1], \
     (rgba).channel.blue = (source)->color[2], \
     (rgba).channel.alpha = (source)->color[3], (rgba).word)

/* A float vector that the VU0 code also stores as a whole quadword. */
typedef union SdvScale {
    float f[4];
    unsigned int word[4];
} SdvScale;

/* Normalizes a vector in place. */
static inline void *sdvNormalize(float *vector)
{
    return MMathNormalizeVector2(vector, vector);
}

/* Draws the slots of one special: sdvTransOffset per slot, GameDefocusSet when allocated. */
void sdvDrawSpecial(SdvSpecialOwner *special)
{
    int defocusParams[16];
    SdvScale scale;
    int offset[4];
    int screen[4];
    int slotIndex;

    for (slotIndex = 0; slotIndex < special->slotCount && slotIndex < 6; slotIndex++) {
        SdvSpecialWork *record = &sdvSpecialTable[special->slotIds[slotIndex]];
        SdvSpecialTable *table = record->table;
        SdvSpecialKey *key;
        SdvKeyBytes *nextKey;
        SdvKeyBytes *keys;
        int inBattle;
        void *matrix;
        int kind;
        int argument;
        SdvColor color;
        int defocusSlot;
        int overshoot;
        int sizeStep;
        unsigned int i;
        SdvScale *sizeVector;
        int depth;

        if (table == 0) {
            continue;
        }
        kind = table->kind & 0xFF;
        argument = (short)table->kind >> 8;
        keys = table->keys;
        nextKey = &keys[record->keyIndex];
        inBattle = D_0069DBAC == SDV_BATTLE_MODE;
        if (record->clip != 0 && inBattle) {
            matrix = record->matrix;
        } else {
            matrix = 0;
        }
        defocusParams[0] = 0;
        key = &record->key;
        if (record->frame < nextKey->key.time) {
            continue;
        }
        switch (kind) {
        case 7: /* box drawn at the projected point, sized by width/height */
            if (key->color[3] == 0) {
                break;
            }
            sizeVector = &scale;
            offset[0] = key->x;
            offset[1] = key->y;
            offset[2] = key->z;
            /* sizeVector = (0, 0, 0, 1): VU0 vf0 stored through the pointer */
            __asm__ __volatile__("sqc2 vf0, 0(%1)" : "=m"(*sizeVector) : "r"(sizeVector));
            scale.f[0] = key->width * 0.01;
            scale.f[1] = key->height * 0.01;
            if (sdvTransOffset(screen, offset, table->part, key->depth, sizeVector->f, argument, matrix) == 0) {
                break;
            }
            if (screen[0] < 0 || screen[1] < 0 || screen[0] >= SDV_SCREEN_WIDTH || screen[1] >= SDV_SCREEN_HEIGHT) {
                break;
            }
            defocusParams[1] = 1;
            defocusParams[0] = 0;
            if (sdvAllocSpecialWork(0) >= 0) {
                GameDefocusSet(0, 2, defocusParams);
            }
            SDV_SET_COLOR(color, key);
            defocusParams[0] = screen[2];
            defocusParams[1] = color.word;
            defocusParams[2] = screen[0];
            defocusParams[3] = screen[1];
            if (record->table->part == 8) {
                defocusParams[4] = key->width;
                defocusParams[5] = key->height;
            } else {
                defocusParams[4] = scale.f[0];
                defocusParams[5] = scale.f[1];
                if (record->clip != 0) {
                    /* keep the box inside the screen */
                    overshoot = defocusParams[2] - defocusParams[4];
                    if (overshoot < 0) {
                        defocusParams[4] += overshoot;
                    }
                    overshoot = defocusParams[2] + defocusParams[4] - SDV_SCREEN_WIDTH;
                    if (overshoot > 0) {
                        defocusParams[4] -= overshoot;
                    }
                    overshoot = defocusParams[3] - defocusParams[5];
                    if (overshoot < 0) {
                        defocusParams[5] += overshoot;
                    }
                    overshoot = defocusParams[3] + defocusParams[5] - SDV_SCREEN_HEIGHT;
                    if (overshoot > 0) {
                        defocusParams[5] -= overshoot;
                    }
                    if (D_0069DBAC == SDV_BATTLE_MODE) {
                        if ((unsigned int)defocusParams[4] < (unsigned int)defocusParams[5]) {
                            defocusParams[5] = defocusParams[4];
                        }
                        if ((unsigned int)defocusParams[5] < (unsigned int)defocusParams[4]) {
                            defocusParams[4] = defocusParams[5];
                        }
                    }
                }
            }
            defocusParams[6] = key->intensity;
            defocusSlot = sdvAllocSpecialWork(1);
            if (defocusSlot > 0) {
                GameDefocusSet(defocusSlot, 7, defocusParams);
            }
            break;
        case 6:
            if (key->color[3] == 0) {
                break;
            }
            defocusParams[1] = 1;
            defocusParams[0] = 0;
            if (sdvAllocSpecialWork(0) >= 0) {
                GameDefocusSet(0, 2, defocusParams);
            }
            depth = key->depth != 0 ? key->depth << 8 : SDV_DEPTH_FAR;
            SDV_SET_COLOR(color, key);
            defocusParams[0] = depth;
            defocusParams[2] = key->x;
            defocusParams[3] = key->y;
            defocusParams[4] = key->width;
            defocusParams[5] = key->height;
            defocusParams[1] = color.word;
            defocusSlot = sdvAllocSpecialWork(1);
            if (defocusSlot > 0) {
                GameDefocusSet(defocusSlot, 6, defocusParams);
            }
            break;
        case 5:
            if (key->color[3] == 0) {
                break;
            }
            defocusParams[0] = key->depth << 8;
            SDV_SET_COLOR(color, key);
            sizeStep = 1;
            defocusParams[1] = color.word;
            defocusParams[2] = key->x;
            defocusParams[3] = key->y;
            defocusParams[4] = key->width;
            defocusParams[5] = key->height;
            for (i = 0; i < 9; i++) {
                if (D_005E6D60[i] >= key->intensity) {
                    sizeStep = D_005E6D60[i];
                    break;
                }
            }
            defocusParams[6] = sizeStep;
            defocusParams[7] = sizeStep;
            defocusSlot = sdvAllocSpecialWork(1);
            if (defocusSlot > 0) {
                GameDefocusSet(defocusSlot, 5, defocusParams);
            }
            break;
        case 3:
            if (key->color[3] == 0) {
                break;
            }
            GameDefocusSet(0, 2, defocusParams);
            defocusParams[0] = key->intensity;
            SDV_SET_COLOR(color, key);
            defocusParams[2] = key->x;
            defocusParams[1] = color.word;
            defocusParams[3] = key->y;
            defocusParams[4] = key->width;
            defocusParams[5] = key->height;
            defocusSlot = sdvAllocSpecialWork(1);
            if (defocusSlot > 0) {
                GameDefocusSet(defocusSlot, 3, defocusParams);
            }
            break;
        case 1:
            if (key->color[3] == 0) {
                break;
            }
            defocusParams[0] = 0;
            defocusParams[3] = SDV_SET_COLOR(color, key);
            defocusParams[1] = key->intensity;
            defocusParams[2] = key->depth << 8;
            defocusParams[4] = key->x;
            defocusParams[5] = key->y;
            defocusParams[6] = key->width;
            defocusParams[7] = key->height;
            defocusSlot = sdvAllocSpecialWork(1);
            if (defocusSlot > 0) {
                GameDefocusSet(defocusSlot, 1, defocusParams);
            }
            break;
        case 4:
            if (key->color[3] == 0) {
                break;
            }
            defocusParams[0] = 1;
            defocusParams[3] = SDV_SET_COLOR(color, key);
            defocusParams[4] = 0;
            defocusParams[5] = 0;
            defocusParams[6] = 0;
            defocusParams[7] = 0;
            defocusParams[1] = 1;
            defocusParams[2] = key->depth << 8;
            defocusSlot = sdvAllocSpecialWork(1);
            if (defocusSlot > 0) {
                GameDefocusSet(defocusSlot, 1, defocusParams);
            }
            break;
        case 9:
            defocusSlot = sdvAllocSpecialWork(1);
            if (defocusSlot > 0) {
                GameDefocusSet(defocusSlot, 9, defocusParams);
            }
            break;
        /* kinds 10-14: screen effects, handed to gifState (kind 10 + mode) */
        case 10:
        case 11:
        case 12:
        case 13:
        case 14: {
            unsigned char modeSet;
            float *vector = scale.f;

            offset[0] = key->x;
            offset[1] = key->y;
            offset[2] = key->z;
            Vu0SetOrigin(vector);
            scale.f[0] = key->height * D_0069B378;
            scale.f[1] = key->param12 * D_0069B378;
            modeSet = (record->flags >> SDV_FLAG_MODE_SET_SHIFT) & 1;
            if (record->decreaseWait > 0 && (modeSet == 0 || key->decrease != 0)) {
                gifStateSetDecrease(key->decrease * (1.0f / 256.0f));
                record->decreaseWait = 0;
            }
            if (modeSet == 0) {
                gifStateSetMode(kind - 10);
                record->flags |= SDV_FLAG_MODE_SET;
            }
            if (sdvTransOffset(screen, offset, record->table->part, key->depth, vector, argument, 0) == 0) {
                break;
            }
            gifStateSetCenter(screen[0], screen[1]);
            gifStateSetEffectAlpha(record->flags & SDV_FLAG_ALPHA_TYPE, key->color[3]);
            gifStateSetFbp((record->flags >> SDV_FLAG_FBP_SHIFT) & 1);
            gifStateSetBlur(key->intensity);
            func_00246DD8(screen[2]);
            nmlModelUseSubWindow(0, key->width);
            if (kind == 11 && record->table->part != 8) {
                nmlModelUseSubWindow(2, scale.f[0]);
                nmlModelUseSubWindow(3, scale.f[1]);
                scale.f[0] = key->param14 * D_0069B37C;
                sdvTransOffset(screen, offset, record->table->part, key->depth, vector, argument, 0);
                nmlModelUseSubWindow(1, scale.f[0]);
            } else {
                nmlModelUseSubWindow(1, key->height);
                nmlModelUseSubWindow(2, key->param12);
                nmlModelUseSubWindow(3, key->param14);
            }
            if (kind == 12) {
                signed char *direction = (signed char *)key->color;

                scale.f[0] = direction[0];
                scale.f[1] = direction[1];
                scale.f[2] = direction[2];
                scale.f[3] = 1.0f;
                sdvNormalize(scale.f);
                sefApplyPartTransform(vector, vector, record->table->part);
                nmlModelSetMulColor(vector);
            }
            break;
        }
        }
    }
}

/* Builds an alter request from a sequence entry, then creates it through sdvCreateAlter. */
INCLUDE_ASM("asm/main/nonmatchings/sdv_183", sdvScheduleAlter);


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
INCLUDE_ASM("asm/main/nonmatchings/sdv_183", sdvCreateAlter);

/* Zeroes one alter record (same code as sdvInitAlter; the callers tell them apart). */
void sdvDestroyAlter(SdvAlter *alter)
{
    sefMemZero(alter, sizeof(*alter));
}

/* Kills alter id (index | serial << 16) when it is active and the serial matches. */
INCLUDE_ASM("asm/main/nonmatchings/sdv_183", sdvKillAlter);

INCLUDE_ASM("asm/main/nonmatchings/sdv_183", func_0026A718);

INCLUDE_ASM("asm/main/nonmatchings/sdv_183", func_0026A7A0);

/* Advances one alter (calls sdvDestroyAlter among others). */
INCLUDE_ASM("asm/main/nonmatchings/sdv_183", sdvExecAlter);

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

INCLUDE_ASM("asm/main/nonmatchings/sdv_183", func_0026AFB0);

/* Draws one alter's key frames into the packet from xglPacketGetCurrent (sdvDrawAlter). */
INCLUDE_ASM("asm/main/nonmatchings/sdv_183", sdvDrawAlter);

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
INCLUDE_ASM("asm/main/nonmatchings/sdv_183", sdvExecAlters);

INCLUDE_ASM("asm/main/nonmatchings/sdv_183", sdvDrawAlters);

INCLUDE_ASM("asm/main/nonmatchings/sdv_183", func_0026BB78);
