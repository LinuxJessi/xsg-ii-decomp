#include "common.h"
#include "shared.h"

/* The request block of mapProbeInit and mapMoveProbe (get.c's UnduParam): the header word and the attribute word.
   local view: the shared UnduParam of get.h (0x2C bytes, same members) changed the .text bytes of uwamonoStartAreaBlend in a probe; this 0x40-byte view stays. */
typedef struct UnduParam {
    unsigned char unmodeled_00[0x18];
    int header;                         /* +0x18: set from mapFindBlockEntry before the probe */
    unsigned char unmodeled_1C[0x4];
    int attribute;                      /* +0x20: written by the probe */
    unsigned char unmodeled_24[0x40 - 0x24];
} UnduParam;

/* The object a record points at (+0x330): its probe origin and the midpoint tracks of uwamonoUpdateMidpoints. */
typedef struct UwamonoObject {
    unsigned char unmodeled_00[0x10];
    Vector4 position;                   /* +0x10: origin passed to mapMoveProbe */
    unsigned char unmodeled_20[0xCC - 0x20];
    float blend;                        /* +0xCC */
    unsigned char unmodeled_D0[0x190 - 0xD0];
    float midpointA[3][4];              /* +0x190 */
    unsigned char unmodeled_1C0[0x200 - 0x1C0];
    float midpointB[3][4];              /* +0x200 */
    unsigned char unmodeled_230[0x260 - 0x230];
    float midpointC[4];                 /* +0x260 */
    unsigned char unmodeled_270[0x280 - 0x270];
    float midpointD[4];                 /* +0x280 */
} UwamonoObject;

/* One 0x870-byte record of uwamonoRecords: the fields this unit reads and writes. */
typedef struct UwamonoRecord {
    unsigned char unmodeled_00[0xF0];
    float startA[3][4];                 /* +0xF0 */
    float followA[3][4];                /* +0x120 */
    union { float startB[3][4]; };      /* +0x150: alias set 0, so uwamonoStartAreaBlend's cursor stores stay behind these copies */
    float followB[3][4];                /* +0x180 */
    float startD[4];                    /* +0x1B0 */
    float followD[4];                   /* +0x1C0 */
    float blendStart;                   /* +0x1D0 */
    float blendEnd;                     /* +0x1D4 */
    unsigned char unmodeled_1D8[0x1E0 - 0x1D8];
    float startC[4];                    /* +0x1E0 */
    float followC[4];                   /* +0x1F0 */
    unsigned char unmodeled_200[0x222 - 0x200];
    short cursorA[3];                   /* +0x222 */
    short countA[3];                    /* +0x228 */
    short cursorB[3];                   /* +0x22E */
    short countB[3];                    /* +0x234 */
    short cursorD;                      /* +0x23A */
    short countD;                       /* +0x23C */
    short cursorE;                      /* +0x23E */
    short totalE;                       /* +0x240 */
    short cursorC;                      /* +0x242 */
    short countC;                       /* +0x244 */
    unsigned char unmodeled_246[0x248 - 0x246];
    int frameCount;                     /* +0x248 */
    unsigned char unmodeled_24C[0x330 - 0x24C];
    UwamonoObject *object;              /* +0x330 */
    unsigned char unmodeled_334[0x870 - 0x334];
} UwamonoRecord;

extern UwamonoRecord uwamonoRecords[];

/* A midpoint track whose cursor is UWAMONO_TRACK_IDLE is not running; a started track takes UWAMONO_BLEND_STEPS steps. */
#define UWAMONO_TRACK_IDLE (-1)
#define UWAMONO_BLEND_STEPS 5
/* Probe attribute: both bits set means the low byte holds the area index. */
#define UWAMONO_PROBE_AREA_FLAGS 0x500
#define UWAMONO_PROBE_AREA_MASK 0xff
/* gameWork: the midpoint tables are float rows indexed by the frame count (tbl[224 + 12 * count], [416 + ...], [608 + 4 * count], [672 + ...], [736 + count]). */
typedef struct GameWork { unsigned char unmodeled_00[4]; float tbl[1024]; } GameWork;
extern GameWork gameWork;
extern const char D_00685760[];
extern const char D_00685778[];

int mapFindBlockEntry(int, int);
void mapProbeInit(UnduParam *request);
float mapMoveProbe(const Vector4 *position, int mode, UnduParam *request);
void xglFontDebugPrintf(int, int, const char *, ...);
/* local view: the shared Get_MiddlePoint of get.h takes Vector4 pointers; the Vector4 record views change the code of uwamonoUpdateMidpoints. */
void Get_MiddlePoint(float *start, float *follow, int cursor, int count, float *out);

/*
 * Per uwamonoRecords[index] (called from DispMapObject): three Get_MiddlePoint pairs write midpoints to the record's
 * object (the pointer at +0x330) at +0x190 and +0x200; short counters at +0x222..+0x244 pace them, and the float at
 * +0xCC is blended over a frame count.
 */
void uwamonoUpdateMidpoints(int index)
{
    UwamonoRecord *record = &uwamonoRecords[index];
    UwamonoObject *object = record->object;
    union {
        float value[4];   /* alias set 0: the midpoint loads stay behind the cursor store */
    } out;
    int i;

    for (i = 0; i < 3; i++) {
        if (record->cursorA[i] != UWAMONO_TRACK_IDLE) {
            unsigned short cursor;
            Get_MiddlePoint(record->startA[i], record->followA[i], record->cursorA[i], record->countA[i], out.value);
            cursor = record->cursorA[i];
            record->cursorA[i] = (short)(cursor + 1);
            object->midpointA[i][0] = out.value[0];
            object->midpointA[i][1] = out.value[1];
            object->midpointA[i][2] = out.value[2];
            object->midpointA[i][3] = out.value[3];
            if ((short)cursor >= record->countA[i]) {
                record->cursorA[i] = UWAMONO_TRACK_IDLE;
                record->countA[i] = 0;
            }
        }
        if (record->cursorB[i] != UWAMONO_TRACK_IDLE) {
            unsigned short cursor;
            Get_MiddlePoint(record->startB[i], record->followB[i], record->cursorB[i], record->countB[i], out.value);
            cursor = record->cursorB[i];
            record->cursorB[i] = (short)(cursor + 1);
            object->midpointB[i][0] = out.value[0];
            object->midpointB[i][1] = out.value[1];
            object->midpointB[i][2] = out.value[2];
            object->midpointB[i][3] = out.value[3];
            if ((short)cursor >= record->countB[i]) {
                record->cursorB[i] = UWAMONO_TRACK_IDLE;
                record->countB[i] = 0;
            }
        }
    }
    if (record->cursorC != UWAMONO_TRACK_IDLE) {
        unsigned short cursor;
        Get_MiddlePoint(record->startC, record->followC, record->cursorC, record->countC, out.value);
        cursor = record->cursorC;
        record->cursorC = (short)(cursor + 1);
        object->midpointC[0] = out.value[0];
        object->midpointC[1] = out.value[1];
        object->midpointC[2] = out.value[2];
        object->midpointC[3] = out.value[3];
        if ((short)cursor >= record->countC) {
            record->cursorC = UWAMONO_TRACK_IDLE;
            record->countC = 0;
        }
    }
    if (record->cursorD != UWAMONO_TRACK_IDLE) {
        unsigned short cursor;
        Get_MiddlePoint(record->startD, record->followD, record->cursorD, record->countD, out.value);
        cursor = record->cursorD;
        record->cursorD = (short)(cursor + 1);
        object->midpointD[0] = out.value[0];
        object->midpointD[1] = out.value[1];
        object->midpointD[2] = out.value[2];
        object->midpointD[3] = record->followD[3];
        if ((short)cursor >= record->countD) {
            record->cursorD = UWAMONO_TRACK_IDLE;
            record->countD = 0;
        }
    }
    if (record->cursorE != UWAMONO_TRACK_IDLE) {
        short cursor;
        cursor = record->cursorE;
        object->blend = record->blendStart + (record->blendEnd - record->blendStart) * cursor / record->totalE;
        record->cursorE = record->cursorE + 1;
        if (cursor >= record->totalE) {
            record->cursorE = UWAMONO_TRACK_IDLE;
            record->totalE = 0;
        }
    }
}

/*
 * Per uwamonoRecords[index] (called from DispMapObject before uwamonoUpdateMidpoints): copies the object's midpoints back into
 * the record (+0xF0, +0x1B0, +0x1E0, +0xCC) and stores the frame count at +0x248. A debug line goes out when the flags
 * word returned through the stack has 0x500 set.
 */
void uwamonoStartAreaBlend(int index)
{
    UwamonoRecord *record = &uwamonoRecords[index];
    UwamonoObject *object = record->object;
    UnduParam request;
    int attribute;
    int count;
    int i;

    mapProbeInit(&request);
    request.header = mapFindBlockEntry(2560, -1);
    mapMoveProbe(&object->position, 0, &request);
    attribute = request.attribute;
    if ((attribute & UWAMONO_PROBE_AREA_FLAGS) == UWAMONO_PROBE_AREA_FLAGS) {
        xglFontDebugPrintf(192, 20, D_00685760, attribute);
        attribute = request.attribute;
        count = (attribute & UWAMONO_PROBE_AREA_MASK) + 1;
    } else {
        count = 0;
    }
    xglFontDebugPrintf(192, 30, D_00685778, record->frameCount, count, attribute);
    if (record->frameCount != count) {
        for (i = 0; i < 3; i++) {
            record->startA[i][0] = object->midpointA[i][0];
            record->startA[i][1] = object->midpointA[i][1];
            record->startA[i][2] = object->midpointA[i][2];
            record->startA[i][3] = object->midpointA[i][3];
            record->followA[i][0] = gameWork.tbl[224 + 12 * count + 4 * i + 0];
            record->followA[i][1] = gameWork.tbl[224 + 12 * count + 4 * i + 1];
            record->followA[i][2] = gameWork.tbl[224 + 12 * count + 4 * i + 2];
            record->followA[i][3] = gameWork.tbl[224 + 12 * count + 4 * i + 3];
            record->startB[i][0] = object->midpointB[i][0];
            record->startB[i][1] = object->midpointB[i][1];
            record->startB[i][2] = object->midpointB[i][2];
            record->startB[i][3] = object->midpointB[i][3];
            record->followB[i][0] = gameWork.tbl[416 + 12 * count + 4 * i + 0];
            record->followB[i][1] = gameWork.tbl[416 + 12 * count + 4 * i + 1];
            record->followB[i][2] = gameWork.tbl[416 + 12 * count + 4 * i + 2];
            record->followB[i][3] = gameWork.tbl[416 + 12 * count + 4 * i + 3];
            record->cursorA[i] = record->cursorB[i] = 0;
            record->countB[i] = UWAMONO_BLEND_STEPS;
            record->countA[i] = UWAMONO_BLEND_STEPS;
        }
        record->startD[0] = object->midpointD[0];
        record->startD[1] = object->midpointD[1];
        record->startD[2] = object->midpointD[2];
        record->startD[3] = object->midpointD[3];
        record->followD[0] = gameWork.tbl[672 + 4 * count + 0];
        record->followD[1] = gameWork.tbl[672 + 4 * count + 1];
        record->followD[2] = gameWork.tbl[672 + 4 * count + 2];
        record->followD[3] = gameWork.tbl[672 + 4 * count + 3];
        record->blendStart = object->blend;
        record->blendEnd = gameWork.tbl[736 + count];
        record->startC[0] = object->midpointC[0];
        record->startC[1] = object->midpointC[1];
        record->startC[2] = object->midpointC[2];
        record->startC[3] = object->midpointC[3];
        record->followC[0] = gameWork.tbl[608 + 4 * count + 0];
        record->followC[1] = gameWork.tbl[608 + 4 * count + 1];
        record->followC[2] = gameWork.tbl[608 + 4 * count + 2];
        record->followC[3] = gameWork.tbl[608 + 4 * count + 3];
        record->cursorC = 0;
        record->cursorE = 0;
        record->cursorD = 0;
        record->countC = UWAMONO_BLEND_STEPS;
        record->totalE = UWAMONO_BLEND_STEPS;
        record->countD = UWAMONO_BLEND_STEPS;
        record->frameCount = count;
    }
}

/* Returns 0. No caller inside this unit. */
int func_001DF088(void) {
    return 0;
}
