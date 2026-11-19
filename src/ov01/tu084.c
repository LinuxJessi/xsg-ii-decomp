/*
 * OV01 original TU 84 (re-cut 2026-11-18: effect/menu files start at their Create)
 */

#include "common.h"

#include "quadword.h"

#include "ov01/tu075.h"

#include "shared.h"
#include "main/tu187.h"
#include "main/tu194.h"

#include "vu0_macro.h"

#include "main/tu197.h"
#include "main/tu190.h"
#include "main/tu196.h"
#include "main/m_math_191.h"

/* The flag window: one status word and one position per history tick, sixteen strips around it. */
#define FLAG_WINDOW_POINTS 25
#define FLAG_WINDOW_STRIPS 16
/* Status bits of a history tick: a live tick, the tick 0 sample (START), the tick 29 sample (END). */
#define FLAG_WINDOW_VALID 0x01U
#define FLAG_WINDOW_START 0x10
#define FLAG_WINDOW_END 0x20
/* The tick a status word is set from is valid below ACTIVE_TICKS, END_TICK marks END, the effect frees at LIFETIME_TICKS. */
#define FLAG_WINDOW_ACTIVE_TICKS 0x1E
#define FLAG_WINDOW_END_TICK 0x1D
#define FLAG_WINDOW_LIFETIME_TICKS 0x37

/* A packed GS vertex or colour: the projected point, or its words (w carries ADC). */
typedef union MEfGsVertex {
    AlignedVector4 vector;
    Quadword whole;
    int words[4];
} MEfGsVertex;

/* Writes a GIF tag for regCount registers and returns its first data qword. */
static inline MEfGsQword *OpenTag(MGsPacket *packet, int pre, int prim, int regCount, u64 registers)
{
    MEfGsQword *qword = packet->current;

    qword->gif.tag = 0x8000 | ((u64) pre << 46) | ((u64) prim << 47) | ((u64) regCount << 60);
    qword->gif.registers = registers;
    packet->regCount = regCount;
    return qword + 1;
}

/* Writes one A+D register setting. */
static inline void SetRegister(MEfGsQword *qword, unsigned char address, u64 data)
{
    qword->ad.data = data;
    qword->ad.address = address;
}

/* Stores NLOOP into the open GIF tag and moves past its data. */
static inline void CloseTag(MGsPacket *packet, int nloop)
{
    int size;

    if (nloop > 0) {
        size = packet->regCount * nloop + 1;
        packet->current->nloop |= nloop;
        packet->current += size;
        packet->count += size;
    }
}

/*
 * The trail block at object+0x20: the actor pair and their coordinates at
 * +0x0C..+0x34, the GS packet at +0x40, a 16-bit tick count at +0x54, two
 * 16-byte samples at +0x60 and +0x70, and a ring of samples from +0x80 indexed
 * by the tick count.
 */
typedef struct TrailBlock {
    unsigned char unmodeled_00[0x0C];
    struct Actor *firstActor;               /* +0x0C */
    unsigned int firstCoordinate;           /* +0x10 */
    unsigned char unmodeled_14[0x1C];
    struct Actor *secondActor;              /* +0x30 */
    unsigned int secondCoordinate;          /* +0x34 */
    unsigned char unmodeled_38[0x08];
    MGsPacket packet;                       /* +0x40 */
    unsigned short ticks;                   /* +0x54 */
    unsigned char unmodeled_56[0x60 - 0x56];
    AlignedVector4 sample;                  /* +0x60 */
    AlignedVector4 prevSample;              /* +0x70 */
    AlignedVector4 ring[58];                /* +0x80 */
} TrailBlock;

typedef struct TrailHost {
    unsigned char unmodeled_00[0x4];
    int resourceType;
    void *step;
    unsigned char unmodeled_0C[0x4];
    void *draw;
    void *advance;
    unsigned char unmodeled_18[0x8];
    TrailBlock block;
} TrailHost;

/* The path-curve record (overlay data mefPathCurve): four key vectors at 0x10 steps. */
typedef struct MefPathCurve {
    unsigned char unmodeled_00[0x10];
    AlignedVector4 key1;
    AlignedVector4 key2;
    AlignedVector4 key3;
    AlignedVector4 key4;
} MefPathCurve;

/*
 * The flag-window block at object+0x20. Only the members these functions touch
 * are named; the gaps stay unmodeled. statusWords[0] holds the status word of
 * the current tick and [1..24] its history.
 */
typedef struct FlagWindowBlock FlagWindowBlock;

struct FlagWindowBlock {
    unsigned char unmodeled_00[0x40];
    MGsPacket packet;                       /* +0x40 */
    s16 ticks;                              /* +0x54 */
    unsigned char unmodeled_56[0xC0 - 0x56];
    u16 statusWords[FLAG_WINDOW_POINTS];                   /* +0xC0 */
    unsigned char unmodeled_F2[0x100 - 0xF2];
    AlignedVector4 positions[FLAG_WINDOW_POINTS];           /* +0x100 */
    AlignedVector4 angles[FLAG_WINDOW_POINTS];              /* +0x290 */
};

typedef struct FlagWindowHost FlagWindowHost;

struct FlagWindowHost {
    unsigned char unmodeled_00[0x20];
    FlagWindowBlock block;
};


extern MefPathCurve mefPathCurve;
extern AlignedVector4 MMathUnitVector;

short ov01LastValueGet(void);
/* The rotation helpers stay local views: this file passes AlignedVector4 rows (frame, pointFrame) as the
 * result matrix, which is not the Matrix4 * of main/m_math_193.h. */
void MMathRotateMatrixYX(void *result, int angle, const void *vector);
/* ov01RecordInit: the definition is tu076.c; this file keeps its own view (void * record). */
void ov01RecordInit(void *record, int count);
void MEfGetActorCoord(void *result, struct Actor *actor, unsigned int coordinate);
/* MSeGetActorMatrix: the tu077.c definition returns Matrix4 *; this file keeps its void view. */
void MSeGetActorMatrix(void *result, struct Actor *actor, unsigned int coordinate);
void MEfSetStepFlags30(FlagWindowHost *object);
void MEfDrawFlagWindow(FlagWindowHost *object);
void MEfAdvanceFlagWindow(FlagWindowHost *object);
void *memset(void *dst, int value, unsigned int count);
void MEfFillFlagWindowSamples(TrailBlock *block);
void MEfMakeFlagWindowPath(TrailBlock *block);
/* MMathCalcHermiteKey(Prm): the tu076.c definitions take other argument types, so this file keeps its own view. */
void MMathCalcHermiteKeyPrm(const void *base, int segment);
void MMathCalcHermiteKey(AlignedVector4 *destination, const void *base, int index, float parameter);
void *MMathRotateMatrixYXZ(void *result, int mode, const void *angles);

int MEfCreateFlagWindowEffect30(TrailHost *object)
{
    TrailBlock *block;

    block = &object->block;
    block->ticks = 0;
    MEfGetActorCoord(&block->sample, block->firstActor, block->firstCoordinate);
    MEfGetActorCoord(&block->prevSample, block->secondActor, block->secondCoordinate);
    MSeGetActorMatrix(&block->ring[0], block->firstActor, block->firstCoordinate);
    memset(&block->ring[4], 0, 0x32);
    /* positions and angles of the FlagWindowBlock view of this object (+0x120, +0x2B0): TrailBlock has no such members. */
    memset((unsigned char *) object + 0x120, 0, 0x190);
    memset((unsigned char *) object + 0x2B0, 0, 0x190);
    MEfMakeFlagWindowPath(block);
    MEfFillFlagWindowSamples(block);
    MGsGPInit(&block->packet, 0, 0);
    object->step = MEfSetStepFlags30;
    object->draw = MEfDrawFlagWindow;
    object->advance = MEfAdvanceFlagWindow;
    return 1;
}

void MEfMakeFlagWindowPath(TrailBlock *block)
{
    MefPathCurve *curve;
    AlignedVector4 *key1;
    AlignedVector4 *key2;
    AlignedVector4 *key3;
    AlignedVector4 direction;
    AlignedVector4 previousAngle;
    AlignedVector4 nextAngle;
    AlignedVector4 frame[4];
    float segmentLength;
    float turnScale;

    curve = &mefPathCurve;
    key1 = &curve->key1;
    segmentLength = MMathCalcDist(&block->sample, &block->prevSample) * 0.5f;
    ov01RecordInit(curve, 4);
    __asm__ __volatile__(
        "lq $8, 0(%3)\n"
        "sq $8, 0(%2)\n"
        : "=m" (*key1), "=m" (*key1)
        : "r" (key1), "r" (&block->sample)
        : "$8");
    key3 = &curve->key3;
    __asm__ __volatile__(
        "lq $8, 0(%2)\n"
        "sq $8, 0(%1)\n"
        : "=m" (*key3)
        : "r" (key3), "r" (&block->prevSample)
        : "$8");
    __asm__ __volatile__("sqc2 vf0, 0(%0)\n" : : "r" (&direction) : "memory");
    direction.z = segmentLength;
    key2 = &curve->key2;
    MMathApplyMatrix(key2, block->ring, &direction);
    MMathCalcAngle(&previousAngle, key1, key2);
    MMathCalcAngle(&nextAngle, key2, key3);
    Vu0SefSubVectorXYZ(&direction, &nextAngle, &previousAngle);
    turnScale = MMathMakeRandom() + 1.0f;
    Vu0SefScaleVectorXYZNI(&direction, &direction, turnScale);
    Vu0SefAddVectorXYZ(&direction, &nextAngle, &direction);
    MMathRotateMatrixYX(frame, 0, &direction);
    __asm__ __volatile__(
        "lqc2 vf1, 0(%0)\n"
        "vmove.w vf1, vf0\n"
        "sqc2 vf1, 48(%1)\n"
        : : "r" (key3), "r" (&frame[0]), "m" (*key3) : "memory");
    __asm__ __volatile__("sqc2 vf0, 0(%0)\n" : : "r" (&direction) : "memory");
    direction.z = segmentLength;
    MMathApplyMatrix(&curve->key4, &frame[0], &direction);
}

void MEfFillFlagWindowSamples(TrailBlock *block)
{
    AlignedVector4 frame[4];
    AlignedVector4 sample;
    MefPathCurve *curve;
    float parameter;
    int index;
    int segment;
    int step;

    curve = &mefPathCurve;
    QuadwordCopyMatrixT0(frame, &block->ring[0]);
    __asm__ __volatile__(
        "lqc2 vf1, 0(%0)\n"
        "vmove.w vf1, vf0\n"
        "sqc2 vf1, 48(%1)\n"
        : : "r" (&MMathUnitVector), "r" (frame) : "memory");
    index = 0;
    do {
        segment = index / 8;
        step = index - segment * 8;
        if (segment == 3) {
            segment = 2;
            step = 8;
        }
        MMathCalcHermiteKeyPrm(curve, segment);
        parameter = step * 0.125f;
        MMathCalcHermiteKey(&sample, curve, segment, parameter);
        __asm__ __volatile__(
            "lq $8, 0(%4)\n"
            "sq $8, 0(%5)\n"
            : "=m" (block->ring[8 + index].w), "=m" (block->ring[8 + index].w), "=m" (block->ring[8 + index].w), "=m" (block->ring[8 + index].w)
            : "r" (&sample), "r" (&block->ring[8 + index])
            : "$8");
        index++;
    } while (index < FLAG_WINDOW_POINTS);
    for (index = 0; index < 24; index++) {
        MMathCalcAngle(&block->ring[33 + index], &block->ring[8 + index], &block->ring[9 + index]);
    }
    QuadwordCopyT0(&block->ring[57], &block->ring[56]);
}

void MEfSetStepFlags30(FlagWindowHost *object) {
    FlagWindowBlock *block;
    s16 ticks;

    block = &object->block;
    if (ov01LastValueGet() == 0) {
        ticks = block->ticks;
        block->statusWords[0] = 0U;
        if (ticks < FLAG_WINDOW_ACTIVE_TICKS) {
            block->statusWords[0] = FLAG_WINDOW_VALID;
            if (ticks == 0) {
                block->statusWords[0] = 0x11U;
            }
            if (ticks == FLAG_WINDOW_END_TICK) {
                block->statusWords[0] |= FLAG_WINDOW_END;
            }
        }
    }
}

/* Copies a point into a matrix row and sets its w to 1 (lqc2, vmove, sqc2; as tu078). */
#define Vu0MMathCopyPointDst(destination, source) \
    __asm__ __volatile__( \
        "lqc2 vf1, 0(%1)\n" \
        "vmove.w vf1, vf0\n" \
        "sqc2 vf1, %0\n" \
        : "=m" (*(Vector4 *) (destination)) \
        : "r" (source) \
        : "memory")

/*
 * Draw callback: writes the GS state, then sixteen line strips around the
 * window. Each strip takes the 25 history points whose status word is set,
 * rotated by the strip angle and pushed out by a cosine of their age.
 */
void MEfDrawFlagWindow(FlagWindowHost *object)
{
    FlagWindowBlock *block;
    MGsPacket *packet;
    Matrix4 pointFrame;
    Vector4 worldPoint;
    MEfGsVertex vertex;
    AlignedVector4 localOffset;
    /* The strip colour: dark red, opaque. */
    static const Quadword stripColor = 0x000000FF000000000000000000000040;
    MEfGsVertex rgbaq;
    Vector4 cosine;
    AlignedVector4 *pointAngle;
    MEfGsQword *header;
    MEfGsQword *qword;
    struct XglCamera *camera;
    Matrix4 *cameraMatrix;
    int statusWord;
    short startFlag;
    short endFlag;
    int pointCount;
    int hiddenRun;
    int clipResult;
    int historyIndex;
    int stripIndex;
    float stripAngle;
    float phase;

    block = &object->block;
    packet = &object->block.packet;
    camera = MEfCamParams.camera;
    cameraMatrix = &MEfCamParams.viewMatrix;
    MGsGPInit(packet, 0, 0);

    header = packet->current;
    header[0].gif.tag = MEF_GS_SETUP_TAG;
    header[0].gif.registers = MEF_GS_SETUP_REGS;
    packet->regCount = 1;
    qword = header + 1;
    SetRegister(qword, MEF_GS_TEST_1, MEF_GS_TEST_1_ZGEQUAL);
    SetRegister(&header[2], MEF_GS_ZBUF_1, MEF_GS_ZBUF_1_NO_DEPTH_WRITE);
    SetRegister(&header[3], MEF_GS_ALPHA_1, MEF_GS_ALPHA_1_ADDITIVE);
    CloseTag(packet, 3);

    __asm__ __volatile__("sqc2 $vf0, 0(%2)\n" : "=m"(localOffset), "=m"(localOffset.w) : "r"(&localOffset), "r"(&localOffset));
    rgbaq.whole = stripColor;

    for (stripIndex = 0; stripIndex < FLAG_WINDOW_STRIPS; stripIndex++) {
        qword = OpenTag(&block->packet, 1, 0x4A, 2, 0x51);
        phase = (float) block->ticks * -0.10471976f;
        stripAngle = (float) stripIndex * 0.39269906f + phase;
        pointCount = 0;
        hiddenRun = 0;
        for (historyIndex = 0; historyIndex < FLAG_WINDOW_POINTS; historyIndex++) {
            statusWord = block->statusWords[historyIndex];
            if ((short) block->statusWords[historyIndex] == 0) {
                continue;
            }
            phase = (float) (historyIndex - block->ticks) * 1.0471976f;
            Vu0Cos(cosine.x, phase);
            localOffset.x = cosine.x * 0.2f + 0.25f;
            if ((startFlag = statusWord & FLAG_WINDOW_START) != 0 || (endFlag = statusWord & FLAG_WINDOW_END) != 0) {
                localOffset.x = 0.0f;
            }
            pointAngle = &block->angles[historyIndex];
            pointAngle->z = stripAngle;
            MMathRotateMatrixYXZ(pointFrame, 0, pointAngle);
            Vu0MMathCopyPointDst(pointFrame[3], &block->positions[historyIndex]);
            MMathApplyMatrix(&worldPoint, pointFrame, &localOffset);
            clipResult = MMathRotTransPersClip(&vertex.vector, camera, cameraMatrix, &worldPoint);
            pointCount++;
            if (clipResult != 0) {
                hiddenRun = 1;
            } else if (hiddenRun != 0) {
                hiddenRun--;
                vertex.words[3] = 0x8000;
            }
            qword[0].whole = rgbaq.whole;
            qword[1].whole = vertex.whole;
            qword += 2;
        }
        CloseTag(&block->packet, pointCount);
    }
    MGsGPTerm(&block->packet);
}

void MEfAdvanceFlagWindow(FlagWindowHost *object) {
    FlagWindowBlock *block;
    int historyIndex;
    s16 ticks;

    block = &object->block;
    if (ov01LastValueGet() != 0) {
        return;
    }
    for (historyIndex = FLAG_WINDOW_POINTS - 1; historyIndex > 0; historyIndex--) {
        block->statusWords[historyIndex] = block->statusWords[historyIndex - 1];
    }
    ticks = block->ticks + 1;
    block->ticks = block->ticks + 1;
    if (ticks >= FLAG_WINDOW_LIFETIME_TICKS) {
        MSeObjRequestFree(object);
    }
}
