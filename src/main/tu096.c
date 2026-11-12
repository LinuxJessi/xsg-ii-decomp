#include "common.h"
#include "shared.h"
#include "main/tu096.h"

void func_0020CCC0(int *left, int *right) {
    int leftValue = *left;

    *left = *right;
    *right = leftValue;
}

/* 16-bit tick counter (D_0069D498): zeroed, read and incremented by the functions below. */
extern int xglTickCounter;

/* Zeroes the tick counter; called from the init sequence menuSceneReset (main/tu100). */
void xglTickReset(void) {
    xglTickCounter = 0;
}

/* Returns the tick counter; six callers, among them menuDrawSelectArrows (main/tu101). */
int xglTickGet(void) {
    return xglTickCounter;
}

/* Adds one to the tick counter, wrapping at 16 bits. */
void xglTickIncrement(void) {
    xglTickCounter = (xglTickCounter + 1) & 0xFFFF;
}

float xglSin(float angle);
/* xglPi is the float pi (3.1415927). */
extern float xglPi;

/* sin(tick * pi / divisor) * amplitude, truncated; textDrawSegment (main/tu101) passes 32.0 and 24.0. */
int xglTickWave(float divisor, float amplitude) {
    return (int) (xglSin(((float) xglTickCounter * xglPi) / divisor) * amplitude);
}

INCLUDE_ASM("asm/main/nonmatchings/tu096", func_0020CD50);

/*
 * Stores the target pair (+0x8, +0xC) of a FontTrack, struct FontTrack in main/tu097.c:
 * target_pair.x and target_pair.y. func_0020CD50 moves the current pair (+0x0, +0x4)
 * toward it and fontTrackAtTarget tests for arrival.
 */
void fontTrackSetTarget(void *track, int targetX, int targetY) {
    int *track_words = (int *) track;

    track_words[2] = targetX;
    track_words[3] = targetY;
}
