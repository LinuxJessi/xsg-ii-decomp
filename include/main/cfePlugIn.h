#ifndef INCLUDE_MAIN_CFEPLUGIN_H
#define INCLUDE_MAIN_CFEPLUGIN_H

/* Functions of src/main/cfePlugIn.c that other files call. */

#include "main/tu144.h"

void cfeRegisterPlugIn(void);
void cfeRotateVectorXY(Vector4 *result, const float *angles);

/* One 0x50-byte sound record of cfeSndRecords: the name is copied to offset 0 and the tween
   fields sit at +0x40..+0x4C (cfeEvaluate.c's cfeSndRecordEvaluate tweens them). */
typedef struct CfeSndRecord {
    char name[0x40];
    int value;                   /* +0x40: moves toward the target */
    int duration;                /* +0x44 */
    CfeTrackNode *valueTrack;    /* +0x48: the track the value is sampled from */
    CfeTrackNode *durationTrack; /* +0x4C: the track the duration is sampled from */
} CfeSndRecord;

/* Sound records in use: cfeSndRecordCount of them. */
extern CfeSndRecord cfeSndRecords[];
extern int cfeSndRecordCount;

#endif /* INCLUDE_MAIN_CFEPLUGIN_H */
