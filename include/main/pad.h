#ifndef INCLUDE_MAIN_PAD_H
#define INCLUDE_MAIN_PAD_H

#include "shared.h"

/* Button bits of the pad halfwords (PadRecord.held / pressed / repeated, standard PS2 layout). */
#define PAD_L2 0x0001
#define PAD_R2 0x0002
#define PAD_L1 0x0004
#define PAD_R1 0x0008
#define PAD_TRIANGLE 0x0010
#define PAD_CIRCLE 0x0020
#define PAD_CROSS 0x0040
#define PAD_SQUARE 0x0080
#define PAD_SELECT 0x0100
#define PAD_L3 0x0200
#define PAD_R3 0x0400
#define PAD_START 0x0800
#define PAD_UP 0x1000
#define PAD_RIGHT 0x2000
#define PAD_DOWN 0x4000
#define PAD_LEFT 0x8000

/* The first 0x68 bytes of a port record of PadData (the full record is 0x80 bytes: xglPadRead
 * scales the port index by 128). Only the members the game code reads are named. */
typedef struct PadRecord {
    unsigned char unmodeled_00[0x28];
    union {
        struct {
            unsigned short held;     /* +0x28 */
            unsigned short pressed;  /* +0x2A */
            unsigned short repeated; /* +0x2C */
            unsigned short unmodeled_2e;
        };
        unsigned long long heldAndPressed; /* +0x28: held and pressed read as one doubleword */
    };
    unsigned char unmodeled_30[2];
    unsigned short directionPressed;  /* +0x32 */
    unsigned short directionRepeated; /* +0x34 */
    unsigned char unmodeled_36[6];
    short stickX;                     /* +0x3C */
    short stickY;                     /* +0x3E */
    unsigned char unmodeled_40[0x26];
    signed char directionX;           /* +0x66 */
    signed char directionY;           /* +0x67 */
} PadRecord;

#endif /* INCLUDE_MAIN_PAD_H */
