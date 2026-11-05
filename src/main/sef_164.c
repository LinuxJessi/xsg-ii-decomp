#include "common.h"
#include "shared.h"

#define SEF_EFFECT_LINE_COUNT 32

/*
 * One of a scheduler's 32 effect-table records (sefInitEffectData): the
 * first 0x20 bytes are line-data handles, filled with 0xff by memset (a
 * negative handle is a free slot; svDrawSchedulerEffect draws the others);
 * owner, effect_no and category are set from sefInitEffectData's
 * parameters, flags and frame are reset to 0.
 */
typedef struct EffectData {
    signed char lineHandles[SEF_EFFECT_LINE_COUNT];
    int owner;
    short category;
    short effect_no;
    short flags;
    short active;
    short frame;
} EffectData;

/* 128 records of the evidenced 0x820-byte line-data stride. */
extern unsigned char _lineData[0x41000];

/* Particle allocator (_ptAlloc): 1024 particle records of
 * SEF_PT_ALLOC_RECORD_SIZE bytes, then a 1024-entry ring of free handles. */
#define SEF_PT_ALLOC_COUNT 0x400

/* Particles per line-data record. */
#define SEF_LINE_PARTICLE_COUNT 0x100

#define SEF_LINE_DATA_RECORD_SIZE 0x820
#define SEF_LINE_DATA_COUNT 0x80
#define SEF_LOCAL_DATA_PT_TABLE 0x600

/* _ptAlloc's per-record byte size, the same 640 sefCalcLocalMatrix already
 * multiplies _nowParentLocal by. */
#define SEF_PT_ALLOC_RECORD_SIZE 640

#define SEF_PT_ALLOCATOR ((SefPtAllocatorState *)(void *)_ptAlloc)

typedef struct SefLineDataEntry {
    unsigned char unmodeled_000[2];
    unsigned short inUse;
    unsigned char unmodeled_004[SEF_LINE_DATA_RECORD_SIZE - 4];
} SefLineDataEntry;

typedef struct SefLineDataTable {
    unsigned char unmodeled_000[0x810];
    SefLineDataEntry entries[SEF_LINE_DATA_COUNT];
} SefLineDataTable;

/* One line-data record (SEF_LINE_DATA_RECORD_SIZE bytes): the particle-allocator
 * handle table sefFreeLocalData walks and the cursor sefAllocLocalData scans from. */
typedef struct SefLocalData {
    unsigned char *position_keys;                    /* +0x000 */
    short *spread_rotation_keys;                     /* +0x004 */
    unsigned char *size_keys;                        /* +0x008 */
    unsigned char *signal_keys;                      /* +0x00c */
    unsigned char unmodeled_010[0x14 - 0x10];
    unsigned char *rotation_keys;                    /* +0x014 */
    unsigned char unmodeled_018[0x20 - 0x18];
    unsigned short initial_frame;                    /* +0x020 */
    short frame_count;                               /* +0x022 */
    unsigned char unmodeled_024[0x1e0 - 0x24];
    short *color_keys;                               /* +0x1e0 */
    short *uniform_scale_keys;                       /* +0x1e4 */
    unsigned char unmodeled_1e8[SEF_LOCAL_DATA_PT_TABLE - 0x1e8];
    short pointHandles[SEF_LINE_PARTICLE_COUNT];                         /* +0x600 */
    unsigned char unmodeled_800[0x80e - 0x800];
    short cursor;                                      /* +0x80e */
} SefLocalData;

typedef struct SefPtAllocatorState {
    unsigned char records[SEF_PT_ALLOC_COUNT * SEF_PT_ALLOC_RECORD_SIZE];
    short freeSlots[SEF_PT_ALLOC_COUNT];
    unsigned int head;
    unsigned int tail;
    unsigned char unmodeled_a0808[8];
} SefPtAllocatorState;

extern unsigned char _lineData[];

void sevFreePtAllocator(unsigned int handle);
void sefFreeLineData(int line_index);
void sefFreeEffectData(EffectData *effect, unsigned int index);

int sefFreeLocalData(unsigned char *line_data, unsigned int index);

/* The original local pointer stores the KSEG0 alias of the named table
 * sefAtanTbl at 0x0040FAC0 (main symbol map: 0x0040FAC0, size 0x1004). */
extern float *atanTbl_0;

extern unsigned char _ptAlloc[0xA0810];

void sefDestroyLocalData(SefLocalData *data);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_00259658);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_00259668);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_00259678);

float srsAtan2(float x, float y)
{
    int y_is_positive = y >= 0.0f;
    int x_is_positive = x >= 0.0f;
    float result = 0.0f;
    float angle;

    if (y == 0.0f) {
        if (x != 0.0f) {
            if (x_is_positive) {
                result = 1.5707964f;
            } else {
                result = -1.5707964f;
            }
        }
    } else if (x == 0.0f) {
        if (!y_is_positive) {
            result = 3.1415927f;
        }
    } else {
        if (!y_is_positive) {
            y = -y;
        }
        if (!x_is_positive) {
            x = -x;
        }

        if (x <= y) {
            angle = atanTbl_0[(int)(x * 1024.0f / y)];
            if (y_is_positive) {
                if (!x_is_positive) {
                    angle = -angle;
                }
            } else if (x_is_positive) {
                angle = 3.1415927f - angle;
            } else {
                angle += 3.1415927f;
            }
        } else {
            angle = atanTbl_0[(int)(y * 1024.0f / x)];
            if (y_is_positive) {
                if (x_is_positive) {
                    angle = 1.5707964f - angle;
                } else {
                    angle = -(1.5707964f - angle);
                }
            } else if (x_is_positive) {
                angle += 1.5707964f;
            } else {
                angle = 4.712389f - angle;
            }
        }
        result = angle;
    }
    return result;
}

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_002598D8);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_00259900);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_00259960);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_002599A0);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_002599F8);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_00259A90);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_00259B28);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_00259B58);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_00259C18);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_00259CF8);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_00259DA0);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_00259E58);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_00259E88);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_00259EF8);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_00259F28);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_00259F78);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025A388);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025A540);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025A888);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025A970);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025AB20);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025ABE0);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sefFreeLocalData);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sefAllocLocalData);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sefDestroyLocalData);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025B628);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025BCC8);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sefFreeLineData);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025BD68);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025C0D8);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025C648);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025CAB8);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025CF20);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sefDestroyEffectData);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025CFE0);

void sefFreeEffectData(EffectData *effect, unsigned int index)
{
    if (index < SEF_EFFECT_LINE_COUNT) {
        signed char *handle = &effect->lineHandles[index];

        if (*handle >= 0) {
            sefFreeLineData(*handle);
            *handle = -1;
        }
    }
}

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025D0C0);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025D180);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025D948);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025D970);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025DA30);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025DA90);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025DB18);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025DBC0);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025DCB8);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025DDE0);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025DEC0);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025DEC8);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025DED0);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025E050);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025E4A8);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025E4C8);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025E600);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025E660);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025E678);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025EA30);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025EBE0);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025F298);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025F2D0);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025F2F8);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025F358);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025F398);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025F4E8);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025F568);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025F6E0);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025F768);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025F7E8);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025F848);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025F868);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025F888);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025F8F0);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025F958);
