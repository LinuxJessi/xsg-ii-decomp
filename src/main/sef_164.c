#include "common.h"
#include "shared.h"
#include "main/sef_164.h"
#include "main/sef_169.h"
#include "main/tu151.h"
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
#define SEF_LOCAL_DATA_POINT_HANDLES 0x6A0
#define SEF_LINE_DATA_STRIDE 0x8B0
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

/* One line-data record (SEF_LINE_DATA_STRIDE bytes): the particle-allocator
 * handle table sefFreeLocalData walks at +0x6A0, and in_use at +0x8A4 (sefFreeLineData). */
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
    unsigned char unmodeled_1e8[SEF_LOCAL_DATA_POINT_HANDLES - 0x1e8];
    short pointHandles[SEF_LINE_PARTICLE_COUNT];                         /* +0x6a0 */
    unsigned char unmodeled_8a0[0x8a4 - 0x8a0];
    short in_use;                                      /* +0x8a4 */
    unsigned char unmodeled_8a6[SEF_LINE_DATA_STRIDE - 0x8a6];
} SefLocalData;

typedef struct SefPtAllocatorState {
    unsigned char records[SEF_PT_ALLOC_COUNT * SEF_PT_ALLOC_RECORD_SIZE];
    short freeSlots[SEF_PT_ALLOC_COUNT];
    unsigned int head;
    unsigned int tail;
    unsigned char unmodeled_a0808[8];
} SefPtAllocatorState;

extern unsigned char _lineData[];
void sefFreeLineData(int line_index);
void sefFreeEffectData(EffectData *effect, unsigned int index);
/* Frees particle handle `index` of the record when it is in range and * returns -1; out of range, returns the handle's byte offset in the record. */ int sefFreeLocalData(SefLocalData *data, unsigned int index);
/* The original local pointer stores the KSEG0 alias of the named table
 * sefAtanTbl at 0x0040FAC0 (main symbol map: 0x0040FAC0, size 0x1004). */
extern float *atanTbl_0;
extern unsigned char _ptAlloc[0xA0810];
void sefDestroyLocalData(SefLocalData *data);

#include "vu0_macro.h"
#define SEF_TABLE_RECORD_SIZE 0xA70

/* One record of the table sefEffectRecords (SEF_TABLE_RECORD_SIZE bytes); in_use
 * is zero while the record is free. */
typedef struct SefTableRecord {
    unsigned char unmodeled_000[0x48];
    int in_use;                                        /* +0x48 */
    unsigned char unmodeled_04c[SEF_TABLE_RECORD_SIZE - 0x4c];
} SefTableRecord;

/* One 8-byte key of a keyframe track: frame and next are the key index
 * the cursor steps to; value is the sample the cursor takes from it. */
typedef struct SefKeyframe {
    short frame;                                     /* +0x00 */
    short next;                                      /* +0x02 */
    unsigned short value;                            /* +0x04 */
    unsigned char unmodeled_006[2];
} SefKeyframe;

/* A track's cursor: the current key index, the sampled value and the
 * playback frame (float). */
typedef struct SefKeyCursor {
    short index;                                     /* +0x00 */
    unsigned short value;                            /* +0x02 */
    float frame;                                     /* +0x04 */
} SefKeyCursor;

/* The 2D draw flag word: sefDrawEffect2D reads it before its camera and end-of-3D-mode calls. */
extern int sefDraw2DFlag[];

int sefGetDraw2DFlag(void) {
    return sefDraw2DFlag[0];
}

/* The 3D draw flag word: sefDrawEffect3D draws the 3D scene only while it is non-zero. */
extern int sefDraw3DFlag[];

int sefGetDraw3DFlag(void) {
    return sefDraw3DFlag[0];
}

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

/* vector takes the fraction of vector + addend, all four lanes:
 * sum - (float) (int) sum. */
void sefAddVectorFraction(void *vector, void *addend)
{
    __asm__ __volatile__(
        "lqc2 vf1, 0(%0)\n"
        "lqc2 vf2, 0(%1)\n"
        "vadd.xyzw vf1, vf1, vf2\n"
        "vftoi0.xyzw vf3, vf1\n"
        "vitof0.xyzw vf3, vf3\n"
        "vsub.xyzw vf3, vf1, vf3\n"
        "sqc2 vf3, 0(%0)\n"
        :
        : "r"(vector), "r"(addend)
        : "memory");
}

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sevInitPtAllocator);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sevAllocPtAllocator);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sevFreePtAllocator);

/* One 8-byte record of a keyed track: the key is compared with the integer
 * time, 0x400 ends the track, link is the index of the next key (-1 when the
 * track stops there), value is the sample the key holds. */
typedef struct SefTrackKey {
    short key;
    unsigned short link;
    float value;
} SefTrackKey;

/* A scheduled-effect slot as sefResetEffectSlot re-initializes it: the first 0x20
 * bytes are line handles, the next 0x20 are one tag byte per handle (written
 * by sefAddSlotLine), the rest of the fields are only the ones the reset
 * writes. */
typedef struct SefEffectSlot {
    unsigned char lineHandles[0x20];
    unsigned char lineTags[0x40 - 0x20];
    int id;
    unsigned int flags;
    short kind;
    unsigned char lineCount;
} SefEffectSlot;

/* A track cursor: the key index it is on, the running time and the
 * interpolated samples the track readers store. */
typedef struct SefTrackCursor {
    short index;
    short sampled_s16;
    float time;
    float sampled;
} SefTrackCursor;

/* The 16-byte keyed-track record of the wide tracks: only the key and the
 * signed link are read by the track walker. */
typedef struct SefTrackKey16 {
    unsigned char unmodeled_000[0xc];
    short key;
    signed char link;
    unsigned char unmodeled_00f[1];
} SefTrackKey16;

extern f32 sefTrackTimeStep;

/* Advances a track cursor by one time step, and returns the key it now sits on. */
SefTrackKey *sefAdvanceTrackCursor(SefTrackKey *keys, SefTrackCursor *cursor)
{
    f32 time;
    int frame;
    SefTrackKey *key;
    short next_index;

    time = cursor->time + sefTrackTimeStep;
    key = &keys[cursor->index];
    frame = (int)time;
    cursor->time = time;
    if (frame < 0x400 && key->key != 0x400 && frame >= (key + 1)->key) {
        key++;
        next_index = (short)key->link;
        if (next_index >= 0) {
            cursor->index = key->link;
            key = &keys[next_index];
            cursor->time = (f32)key->key;
        } else {
            cursor->index = (s16)((u16)cursor->index + 1);
        }
    }
    return key;
}

/* The wide-track form of sefAdvanceTrackCursor: advances a cursor over 16-byte records
 * and returns the record it sits on. */
SefTrackKey16 *sefAdvanceTrackCursor16(SefTrackKey16 *keys, SefTrackCursor *cursor)
{
    f32 time;
    int frame;
    SefTrackKey16 *key;
    signed char next_index;

    time = cursor->time + sefTrackTimeStep;
    key = &keys[cursor->index];
    frame = (int)time;
    cursor->time = time;
    if (frame < 0x400 && key->key != 0x400 && frame >= (key + 1)->key) {
        key++;
        next_index = key->link;
        if (next_index >= 0) {
            cursor->index = next_index;
            key = &keys[next_index];
            cursor->time = (f32)key->key;
        } else {
            cursor->index = (s16)((u16)cursor->index + 1);
        }
    }
    return key;
}

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sefLerpPositionXyz);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sefSampleVectorTrack);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sefSampleColorTrack);

/* Samples a track at the cursor's time into cursor->sampled. Past the last
 * key the sample holds that key's value; otherwise the cursor advances and
 * the sample is interpolated between the two keys it now spans. */
void sefSampleTrack(SefTrackKey *keys, SefTrackCursor *cursor)
{
    SefTrackKey *key;
    f32 sample;

    key = &keys[cursor->index];
    if ((key + 1)->key == 0x400) {
        sample = key->value;
    } else {
        key = sefAdvanceTrackCursor(keys, cursor);
        if (key->key == (key + 1)->key) {
            return;
        }
        sample = ((key + 1)->value - key->value)
               * ((cursor->time - (f32)key->key) / (f32)((key + 1)->key - key->key))
               + key->value;
    }
    cursor->sampled = sample;
}

/* The truncated-sample reader: like sefSampleTrack but the interpolated value
 * goes to cursor->sampled_s16, and nothing is written while either key of the
 * current span is the 0x400 end marker. */
void sefSampleTrackTruncated(SefTrackKey *keys, SefTrackCursor *cursor)
{
    SefTrackKey *key;
    f32 sample;

    key = &keys[cursor->index];
    if (key->key == 0x400 || (key + 1)->key == 0x400) {
        return;
    }
    key = sefAdvanceTrackCursor(keys, cursor);
    if (key->key == (key + 1)->key) {
        return;
    }
    sample = ((key + 1)->value - key->value)
           * ((cursor->time - (f32)key->key) / (f32)((key + 1)->key - key->key))
           + key->value;
    cursor->sampled_s16 = (short)sample;
}

/* Advances a track cursor by one time step, and returns the key it now sits on. */ SefTrackKey *sefAdvanceTrackCursor(SefTrackKey *keys, SefTrackCursor *cursor);

void sefSampleKeyframeValue(SefKeyframe *keys, SefKeyCursor *cursor) {
    cursor->value = ((SefKeyframe *)sefAdvanceTrackCursor((SefTrackKey *)keys, (SefTrackCursor *)cursor))->value;
}

void *MMathNormalizeVector2(void *destination, const Vector4 *source);
void MMathMatrixBetweenVectors(void *destination, int slot, const Vector4 *vector);

/* Normalizes a non-zero direction and hands it to the slot-indexed sink;
 * returns 0 without calling it for a zero direction. */
int sefSetSlotDirection(void *destination, const Vector4 *direction, int slot)
{
    Vector4 normalized;
    int is_zero;

    is_zero = ((*(int *)&direction->z | *(u64 *)direction) == 0);
    if (is_zero) {
        return 0;
    }
    MMathNormalizeVector2(&normalized, direction);
    MMathMatrixBetweenVectors(destination, slot, &normalized);
    return 1;
}

/* Normalizes a non-zero direction and hands it to the slot-indexed sink; * returns 0 without calling it for a zero direction. */ int sefSetSlotDirection(void *destination, const Vector4 *direction, int slot);

void sefSetSlotDirectionBetween(void *state, Vector4 *minuend, Vector4 *subtrahend, int flags) {
    Vector4 delta;

    Vu0SefSubVectorXYZ(&delta, minuend, subtrahend);
    sefSetSlotDirection(state, &delta, flags);
}


/* result = the normalized (first - second) when flags has 0x400, else the
 * normalized (second - first); xyz only. The normalizing subroutine works on
 * vf20 in place. */
void sefNormalizeDifference(Vector4 *result, Vector4 *first, Vector4 *second, int flags) {
    __asm__ __volatile__(
        "lqc2 vf20, 0(%0)\n"
        "lqc2 vf1, 0(%1)\n"
        :
        : "r"(first), "r"(second)
        : "memory");
    if ((flags & 0x400) != 0) {
        __asm__ __volatile__("vsub.xyz vf20, vf20, vf1" : : : "memory");
    } else {
        __asm__ __volatile__("vsub.xyz vf20, vf1, vf20" : : : "memory");
    }
    vu0NormalizeVf20();
    __asm__ __volatile__("sqc2 vf20, 0(%0)" : : "r"(result) : "memory");
}

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_00259F78);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025A388);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025A540);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025A888);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025A970);

f32 MMathVectorDotProduct(f32 *, f32 *);

/* Segment-sphere test: segment is end - start, offset is start - anchor, and
 * the discriminant of the quadratic in the segment parameter decides whether
 * the segment reaches the sphere of the given radius. */
int sefSegmentHitsSphere(void *anchor, void *start, void *end, f32 radius)
{
    f32 offset[4];
    f32 segment[4];
    f32 *segment_ptr;
    f32 segment_sq;
    f32 segment_dot_offset;

    segment_ptr = &segment[0];
    Vu0SefSubVectorXYZ(segment_ptr, end, start);
    Vu0SefSubVectorXYZ(offset, start, anchor);
    segment_sq = MMathVectorDotProduct(segment_ptr, segment_ptr);
    segment_dot_offset = MMathVectorDotProduct(segment_ptr, offset);
    if (((segment_dot_offset * segment_dot_offset)
         - (segment_sq * 4.0f * (MMathVectorDotProduct(offset, offset) - (radius * radius)))) >= 0.0f) {
        return 1;
    }
    return 0;
}

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025ABE0);

void sevFreePtAllocator(short handle);

/* Frees particle handle `index` of the record when it is in range and
 * returns -1; out of range, returns the handle's byte offset in the record. */
int sefFreeLocalData(SefLocalData *data, unsigned int index) {
    int in_range = index < SEF_LINE_PARTICLE_COUNT;
    int offset = (int)(index * sizeof(short)) + SEF_LOCAL_DATA_POINT_HANDLES;

    if (in_range != 0) {
        short *handle = (short *)((unsigned char *)data + offset);

        sevFreePtAllocator(*handle);
        offset = -1;
        *handle = -1;
    }
    return offset;
}

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sefAllocLocalData);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sefDestroyLocalData);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025B628);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sefAllocLineData);

extern SefLocalData *sefLineDataRecords;
extern int sefLineDataCount;

void sefFreeLineData(int line_index) {
    if (line_index >= 0) {
        if (line_index < sefLineDataCount) {
            SefLocalData *line = 0;

            if (line_index >= 0) {
                line = sefLineDataRecords + line_index;
            }
            sefDestroyLocalData(line);
            line->in_use = 0;
        }
    }
}

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025BD68);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025C0D8);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025C648);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025CAB8);

void *memset(void *destination, int value, unsigned int size);

/* Re-initializes a slot: the line handles go to 0xff (free), the id and kind
 * are set from the arguments, bit 31 of flags is cleared and lineCount is zeroed. */
void sefResetEffectSlot(SefEffectSlot *slot, int id, int kind)
{
    int flags;

    memset(slot, -1, 0x20);
    slot->id = id;
    flags = slot->flags & 0x7FFFFFFF;
    slot->kind = (short)kind;
    slot->flags = flags;
    slot->lineCount = 0;
}

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sefDestroyEffectData);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sefAddSlotLine);

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

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sefApplySlotFlags);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025D180);

extern unsigned char sefLineDataPool[];

/* Clears the static 0x68400-byte block sefLineDataPool (192 records of the 0x8B0 line-data stride); both start chains run it
   right after sefInitEffectRecords. */
void sefClearLineDataRecords(void) {
    sefMemClear(sefLineDataPool, 0x68400);
}

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sefInitEffectRecords);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", uwamonoOwnsRecord);

/* The owner whose 0x74 word is non-zero while it has entries in the table. */
typedef struct SefOwner {
    unsigned char unmodeled_000[0x74];
    int tracked;
} SefOwner;

/* One 0x870-byte entry of the 256-entry table: its kind byte and the owner
 * it was registered under. */
typedef struct SefOwnedRecord {
    unsigned char unmodeled_000[2];
    unsigned char kind;
    unsigned char unmodeled_003[0x638 - 0x3];
    void *owner;
    unsigned char unmodeled_63c[0x870 - 0x63c];
} SefOwnedRecord;

void uwamonoReleaseRecord(int index);
extern SefOwnedRecord uwamonoRecords[];

/* Detaches every kind-6 entry registered under owner and releases it by index. */
void uwamonoReleaseOwnedRecords(SefOwner *owner)
{
    SefOwnedRecord *record;
    int index;

    if (owner != 0 && owner->tracked != 0) {
        record = uwamonoRecords;
        for (index = 0; index < 0x100; index++) {
            if (record->kind == 6 && record->owner == owner) {
                record->owner = 0;
                uwamonoReleaseRecord(index);
            }
            record++;
        }
    }
}

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sefAllocEffectRecord);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sefAllocUnownedEffectRecord);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sefFreeEffectRecord);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sefReleaseEffectRecord);

void func_0025DEC0(void) {
}

void func_0025DEC8(void) {
}

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sefLoadEffectSlots);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sefCreateEffectRecord);

int sefCreateEffectRecord(int first, int second, int third, int fourth, int mode, int fifth, int flag);

int sefCreateEffectRecordDefault(int first, int second, int third, int fourth, int fifth) {
    return sefCreateEffectRecord(first, second, third, fourth, 0, fifth, 1);
}

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025E4C8);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sefCopyRecordMatrix);

void MMathCalcLength(void);

void func_0025E660(void) {
    MMathCalcLength();
}

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025E678);

typedef struct SefEffectEntry {
    unsigned char unmodeled_000[0x40];
    int state;
    int flags;
    short key;
    unsigned char unmodeled_04a[1];
    signed char signal;
} SefEffectEntry;

typedef struct SefEffectRecord {
    unsigned char unmodeled_000[0x44];
    int flags;
    int state;
    unsigned char unmodeled_04c[0x6e - 0x4c];
    short gate;
    float coord_a;
    unsigned char unmodeled_074[0x78 - 0x74];
    float coord_b;
    unsigned char unmodeled_07c[0x84 - 0x7c];
    SefEffectEntry entries[32];
    unsigned char unmodeled_a04[0xa66 - 0xa04];
    short remaining;
    unsigned char unmodeled_a68[0xa70 - 0xa68];
} SefEffectRecord;

typedef struct SefEffectOwner {
    unsigned char unmodeled_000[0x5f4];
    float signal_scale;
} SefEffectOwner;

SefEffectOwner *xglStudioGetCamera2(int index);
void func_0025E4C8(SefEffectRecord *record);
extern float D_0069B304;
extern int sefEffectRecordCount;
extern float sefTimeScale;

void sefMatchEffectEntries(SefEffectRecord *records)
{
    int i;
    int j;
    int key;
    int matched;
    float scale;

    for (i = 0; i < sefEffectRecordCount; i++) {
        SefEffectRecord *record = &records[i];

        matched = 0;
        if (record->state != 0 && record->gate != 0 && !(record->flags & 0x400502) && record->remaining <= 0) {
            scale = D_0069B304;
            key = (int)((record->coord_a + record->coord_b * sefTimeScale) * 1000.0f);
            if (key % 10 == 9) {
                key = key + 1;
            }
            key = key / 1000;
            if (key >= 0x3FF) {
                key = 0x3FC;
            }
            for (j = 0; j < 32; j++) {
                SefEffectEntry *entry = &record->entries[j];

                if (entry->state != 0 && key == entry->key) {
                    matched += entry->flags & 1;
                    if (entry->signal >= 0) {
                        xglStudioGetCamera2(0)->signal_scale = (float)entry->signal * scale;
                    }
                }
            }
            if (matched != 0) {
                func_0025E4C8(record);
            }
        }
    }
}

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025EBE0);

extern SefTableRecord *sefEffectRecords;

/* Returns 1 when the index is outside the table or its record is free, else 0. */
int sefEffectRecordIsFree(int index) {
    if ((index < 0) || (index >= sefEffectRecordCount)) {
        return 1;
    }
    return sefEffectRecords[index].in_use == 0;
}

extern int sefHandleTable[];

void sefClearHandleTable(void) {
    memset(sefHandleTable, 0, 0x80);
}

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sefAddHandle);

/* Clears every entry of the 32-entry table sefHandleTable that holds handle. */
void sefRemoveHandle(int handle) {
    int i;
    int *entry = sefHandleTable;

    for (i = 0; i < 32; i++) {
        if (*entry == handle) {
            *entry = 0;
        }
        entry++;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025F398);

/* An entry of the list func_A87E38 returns; its handle sits at 0x1d0. */
typedef struct SefListEntry {
    unsigned char unmodeled_000[0x1d0];
    int handle;
} SefListEntry;

void func_0025F398(int handle, int flag);
int func_A87E38(int kind, SefListEntry ***entries);
extern s16 sefClipTestDisabled;

/* Hands the handle of every listed entry to func_0025F398 when the gate is set. */
void sefRegisterListEntries(void)
{
    SefListEntry **entries;
    SefListEntry *entry;
    int count;
    int index;
    int handle;

    if (sefClipTestDisabled != 0) {
        count = func_A87E38(2, &entries);
        index = 0;
        if (count > 0) {
            do {
                entry = entries[index];
                index++;
                if (entry != 0) {
                    handle = entry->handle;
                    if (handle != 0) {
                        func_0025F398(handle, 1);
                    }
                }
            } while (index < count);
        }
    }
}

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025F568);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sefOrEntryFlags);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sefClearEntryFlags);

INCLUDE_ASM("asm/main/nonmatchings/sef_164", sefGetEntryFlags);

int sefGetEntryFlags(void);

int sefEntryHasFlag1(void) {
    return sefGetEntryFlags() & 1;
}

int sefEntryHasFlag2(void) {
    return sefGetEntryFlags() & 2;
}

/* The actor the wrappers below address, with the offsets of XenoActor in xenoActor.c:
   its local matrix at 0x100 (the 4x4 matrix is the first 0x40 bytes) and the handle
   channel at 0x640, whose address is passed to the callees. */
typedef struct SefActor {
    unsigned char unmodeled_000[0x100];
    unsigned char localMatrix[0x640 - 0x100];
    unsigned char handle640[0];
} SefActor;

int func_0029A6D0(void *operand, void *input, void *output, int flag);
int func_0029A980(void *operand, void *input, void *output, int flag);
void *MMathMulMatrix(void *destination, const void *matrix, const void *source);
Vector4 *MMathApplyMatrix(Vector4 *destination, const void *matrix, const Vector4 *source);

/* Fetches the actor's output for input through its handle channel (func_0029A6D0); on success, when apply is set,
   multiplies the output by the actor's local matrix. Same shape as XenoActorMulMatrix without its override matrix. */
int sefActorMulLocalMatrix(SefActor *actor, void *input, void *output, int apply)
{
    int ok;

    ok = func_0029A6D0(actor->handle640, input, output, 1);
    if (ok != 0) {
        if (apply != 0) {
            MMathMulMatrix(output, actor->localMatrix, output);
        }
        return 1;
    }
    return 0;
}

/* Same as sefActorMulLocalMatrix with the second callee (func_0029A980) and MMathApplyMatrix: the vector form, as
   XenoActorMulVector in xenoActor.c, here without its override matrix. */
int sefActorMulLocalVector(SefActor *actor, void *input, void *output, int apply)
{
    int ok;

    ok = func_0029A980(actor->handle640, input, output, 1);
    if (ok != 0) {
        if (apply != 0) {
            MMathApplyMatrix(output, actor->localMatrix, output);
        }
        return 1;
    }
    return 0;
}

INCLUDE_ASM("asm/main/nonmatchings/sef_164", func_0025F958);
