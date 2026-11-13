#include "common.h"
#include "shared.h"
#include "main/tu051.h"

/* Two endpoints of the sector at angle +/- half its width, both one unit out from source. */
typedef struct UwamonoSector {
    unsigned char unmodeled_0000[0xA0];
    float angle;                        /* +0xA0 */
    unsigned char unmodeled_00A4[0xC];
    float width;                        /* +0xB0 */
} UwamonoSector;

void Get_Point_By_AngleLength(const Vector4 *source, Vector4 *destination, float angle, float length);
void func_00202E60(void *first, void *second, int point);

void uwamonoGetSectorEnds(Vector4 *source, UwamonoSector *sector) {
    Vector4 leftEnd;
    Vector4 rightEnd;

    Get_Point_By_AngleLength(source, &leftEnd, sector->angle - sector->width * 0.5f, 1.0f);
    Get_Point_By_AngleLength(source, &rightEnd, sector->angle + sector->width * 0.5f, 1.0f);
    func_00202E60(source, &leftEnd, 2);
    func_00202E60(source, &rightEnd, 3);
}

extern const char D_00689F00[];
int Check_CrossingOver(const Vector4 *first_start, const Vector4 *first_end,
                       const Vector4 *second_start, const Vector4 *second_end);
void Get_MiddlePoint(const float *start, const float *following,
                     short point_index, short point_count, float *result);
float Get_Distance(const Vector4 *first, const Vector4 *second);
int uwamonoQuadEdgesVisible();
void DispPillar(float *start, int index, float length, float angle);
void uwamonoDrawSegmentBox(float *start, float *following, Vector4 *out);
void xglFontDebugPrintf(int x, int y, const char *format, ...);

int uwamonoCheckCollision(int moverIndex, int sectorIndex, int otherIndex)
{
    UwamonoObject *mover = &uwamonoRecords[moverIndex];
    UwamonoObject *sector = &uwamonoRecords[sectorIndex];
    UwamonoObject *other = &uwamonoRecords[otherIndex];
    Vector4 moverAhead;
    Vector4 rangeMin;
    Vector4 rangeMax;
    double moverDistance;
    float segmentStart[4];
    float segmentEnd[4];
    float distance;
    float flatHeight;
    DoublewordVector4 sectorEnd;
    union {
        int limitWords[4];
        float limitFloats[4];
    } limitBlock;
    AlignedVector4 moverPosition;
    Vector4 *previousPosition;
    struct XenoActor *moverActor;

    moverActor = uwamonoRecords[moverIndex].actor;
    moverPosition = moverActor->position;
    limitBlock.limitWords[0] = 0xFF;
    limitBlock.limitWords[1] = 0x7F;
    limitBlock.limitWords[2] = 0;
    limitBlock.limitWords[3] = 0x7F;
    segmentStart[0] = sector->position.x;
    segmentStart[1] = sector->position.y;
    segmentStart[2] = sector->position.z;
    segmentStart[3] = 1.0f;
    segmentEnd[0] = other->position.x;
    segmentEnd[1] = other->position.y;
    segmentEnd[2] = other->position.z;
    segmentEnd[3] = 1.0f;

    switch (sector->search.shape - 1) {
    case 0:
        previousPosition = &moverActor->previous.vector;
        func_00202E60(segmentStart, segmentEnd, 3);
        Get_MiddlePoint(segmentStart, segmentEnd, 1, 2, (float *)&sectorEnd.vector);
        uwamonoGetSectorEnds(&sectorEnd.vector, (UwamonoSector *)sector);
        if (Check_CrossingOver(&sector->position, &other->position, previousPosition, &moverPosition)) {
            return 1;
        }
        if (uwamonoQuadEdgesVisible(&moverPosition, sector->unmodeled_20, &sector->position, &other->position, other->unmodeled_20)) {
            return 1;
        }
        if (uwamonoQuadEdgesVisible(previousPosition, sector->unmodeled_20, &sector->position, &other->position, other->unmodeled_20)) {
            return 1;
        }
        if (uwamonoQuadEdgesVisible(&moverPosition, other->unmodeled_20, &other->position, &sector->position, sector->unmodeled_20)) {
            return 1;
        }
        return uwamonoQuadEdgesVisible(previousPosition, other->unmodeled_20, &other->position, &sector->position, sector->unmodeled_20) != 0;
    case 1:
        Get_Point_By_AngleLength(&moverPosition, &moverAhead, moverActor->rotation.y, mover->form);
        func_00202E60(segmentStart, segmentEnd, 3);
        Get_MiddlePoint(segmentStart, segmentEnd, 1, 2, (float *)&sectorEnd.vector);
        uwamonoGetSectorEnds(&sectorEnd.vector, (UwamonoSector *)sector);
        return Check_CrossingOver(&sector->position, &other->position, &moverAhead, &moverPosition);
    case 2:
    case 3:
    case 4:
    case 5:
        distance = Get_Distance(&sector->position, &other->position);
        segmentEnd[1] = 0.0f;
        flatHeight = segmentEnd[1];
        segmentEnd[0] = 0.1f;
        DispPillar(segmentStart, 1, 0.1f, flatHeight);
        segmentEnd[0] = distance;
        segmentEnd[1] = flatHeight;
        DispPillar(segmentStart, 1, distance, flatHeight);
        sectorEnd.words[0] = *(u64 *)segmentStart;
        sectorEnd.words[1] = *(u64 *)(segmentStart + 2);
        moverDistance = Get_Distance(&sector->position, &moverPosition);
        xglFontDebugPrintf(8, 0x6C, D_00689F00, (double)distance, moverDistance);
        uwamonoGetSectorEnds(&sectorEnd.vector, (UwamonoSector *)sector);
        if (distance < Get_Distance(&sector->position, &moverPosition)) {
            return 0;
        }
        return 1;
    case 6:
    case 7:
    case 8:
        distance = Get_Distance(&sector->position, &other->position);
        sectorEnd.words[0] = *(u64 *)segmentStart;
        sectorEnd.words[1] = *(u64 *)(segmentStart + 2);
        segmentEnd[0] = segmentStart[0] + distance;
        segmentEnd[2] = segmentStart[2] + distance;
        segmentStart[0] -= distance;
        segmentStart[2] -= distance;
        uwamonoDrawSegmentBox(segmentStart, segmentEnd, (Vector4 *)limitBlock.limitWords);
        rangeMin.z = sector->position.z - distance;
        rangeMax.z = sector->position.z + distance;
        rangeMin.x = sector->position.x - distance;
        rangeMax.x = sector->position.x + distance;
        uwamonoGetSectorEnds(&sectorEnd.vector, (UwamonoSector *)sector);
        goto checkRange;
    case 10: {
        float sideSpan;
        distance = Get_Distance(&sector->position, &other->position);
        sideSpan = 0.3f;
        sectorEnd.words[0] = *(u64 *)segmentStart;
        sectorEnd.words[1] = *(u64 *)(segmentStart + 2);
        segmentEnd[0] = segmentStart[0] + sideSpan;
        segmentEnd[2] = segmentStart[2];
        segmentStart[0] -= sideSpan;
        segmentStart[2] -= distance;
        uwamonoDrawSegmentBox(segmentStart, segmentEnd, (Vector4 *)limitBlock.limitWords);
        rangeMin.x = sector->position.x - sideSpan;
        rangeMax.x = sector->position.x + sideSpan;
        rangeMin.z = sector->position.z - distance;
        rangeMax.z = sector->position.z;
        uwamonoGetSectorEnds(&sectorEnd.vector, (UwamonoSector *)sector);
        goto checkRange;
    }
    default:
        return 0;
    }

checkRange:
    if (moverPosition.x < rangeMin.x || rangeMax.x < moverPosition.x || moverPosition.z < rangeMin.z || rangeMax.z < moverPosition.z) {
        goto missed;
    }
    return 1;

missed:
    return 0;
}

/*
 * The byte fade block at gameWork. vibrationStartFade starts a fade from currentValue
 * towards targetValue over framesTotal steps; vibrationStepFade steps it and writes
 * currentValue to PadData[0].+0x51, with PadData[0].+0x50 as the running flag.
 */
typedef struct ByteFade {
    unsigned char unmodeled_0000[0x332];
    unsigned char targetValue;          /* +0x332 */
    unsigned char currentValue;         /* +0x333 */
    int holdCountdown;                  /* +0x334: hold countdown, read by vibrationStepFade */
    int framesLeft;                     /* +0x338: steps left in the fade */
    int framesTotal;                    /* +0x33C: steps the fade was started with */
} ByteFade;

extern ByteFade gameWork;
enum {
    VIBRATION_RUNNING_FLAG = 0x50,
    VIBRATION_VALUE_BYTE = 0x51
};
/* Clears the fade state: the hold countdown, the fade value and the running flag. */
extern unsigned char PadData[];

void vibrationResetFade(void) {
    do {
        gameWork.holdCountdown = 0;
        PadData[VIBRATION_VALUE_BYTE] = 0;
    } while (0);
    do {
        gameWork.framesLeft = 0;
        PadData[VIBRATION_RUNNING_FLAG] = 0;
    } while (0);
}

/* Stores into the word at D_002B60C4; what that word means is not known yet. */
extern int D_002B60C4[];

void func_00208B50(int value) {
    D_002B60C4[0] = value;
}

void vibrationStartFade(unsigned char targetValue, unsigned char currentValue, int framesTotal) {
    gameWork.targetValue = targetValue;
    gameWork.currentValue = currentValue;
    gameWork.framesLeft = framesTotal;
    gameWork.framesTotal = framesTotal;
}

void vibrationStepFade(void) {
    if (gameWork.holdCountdown != 0) {
        int remaining;

        remaining = 0;
        if (gameWork.holdCountdown - 1 >= 0) {
            remaining = gameWork.holdCountdown - 1;
        }
        PadData[VIBRATION_RUNNING_FLAG] = 1;
        gameWork.holdCountdown = remaining;
    }
    if (gameWork.framesLeft != 0) {
        int remaining;
        int fadeStep;

        fadeStep = (gameWork.targetValue - gameWork.currentValue) * gameWork.framesLeft / gameWork.framesTotal;
        remaining = 0;
        if (gameWork.framesLeft - 1 >= 0) {
            remaining = gameWork.framesLeft - 1;
        }
        gameWork.framesLeft = remaining;
        PadData[VIBRATION_VALUE_BYTE] = gameWork.currentValue + fadeStep;
    }
}
