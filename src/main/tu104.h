#ifndef MAIN_TU104_H
#define MAIN_TU104_H

#include "shared.h"

/* Record reset by menuResetRecord: status is set to -1, both blocks are
   cleared through fontTrackSetBoth and pending is zeroed. */
typedef struct Tu104Record {
    int status;
    unsigned char first_block[0x1C];
    unsigned char second_block[0x1C];
    int pending;
} Tu104Record;

/* Request filled by menuDrawAnimatedBand and handed to fontObjDraw. Only the
   members stored by that function are named. */
typedef struct Tu104Request {
    int length;
    int first;
    int second;
    int phase;
    int reset_value;
    unsigned char unmodeled_14[8];
    int kind;
} Tu104Request;

#endif /* MAIN_TU104_H */
