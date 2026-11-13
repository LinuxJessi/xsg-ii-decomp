#ifndef INCLUDE_MAIN_TU098_H
#define INCLUDE_MAIN_TU098_H

/* Cursor buffers of src/main/tu098.c, used by tu099, tu100 and tu097. */

/* CursorBuffer.status_flags and state bits. CURSOR_FLAG_ACTIVE is the active bit of a cursor record
 * (the text cursor entries of tu097 and the text records of tu099 test the same bit). */
#define CURSOR_FLAG_ACTIVE 0x10000
#define CURSOR_FLAG_TICK_PENDING 0x20000
#define CURSOR_STATE_PAUSE 0x1
#define CURSOR_MODE_WRAP 0x100

/* A step record: 0x14 bytes of header (the first five CursorBuffer words), then the step fields. */
typedef struct CursorStep {
    unsigned char unmodeled_00[0x14]; /* header, not accessed */
    int status_flags;          /* +0x14: bit 0x10000 active, bit 0x20000 pending a tick */
    int count;                 /* +0x18 */
    int state;                 /* +0x1C */
    int aux_value;             /* +0x20 */
    unsigned char mode;        /* +0x24 */
    unsigned char step;        /* +0x25 */
} CursorStep;

/*
 * Fields named from the loads and stores of the functions in tu098.c.
 * Roles are inferred from access patterns only. cursorBufferHandlePad reads a second
 * step record at offset 0x34: its fields sit at 0x48-0x59, the same layout as
 * 0x14-0x25 here. Offsets 0x26-0x47 are not accessed in tu098.c.
 */
struct CursorBuffer {
    int cursor;                        /* 0x00: byte offset into the buffer */
    int previous_cursor;               /* 0x04: the cursor before the last move */
    int capacity;                      /* 0x08: size of the buffer in bytes */
    int unit_size;                     /* 0x0C: size of one element; 0 means none */
    int mode_flags;                    /* 0x10: bit 0x100 set by cursorBufferInit */
    int status_flags;                  /* 0x14: bit 0x10000 cleared by cursorBufferReset, bit 0x20000 by cursorBufferTick */
    int count;                         /* 0x18: advanced by step in cursorBufferTick */
    int state;                         /* 0x1C: bit 0x10000 is the active flag, bit 0x1 pauses the tracks */
    int aux_value;                     /* 0x20: cleared by cursorBufferReset */
    unsigned char mode;                /* 0x24: copied into state by cursorBufferReset and cursorBufferTick */
    unsigned char step;                /* 0x25: added to count by cursorBufferTick */
    unsigned char unmodeled_26[0x0E];  /* 0x26-0x33: not accessed in tu098.c */
    CursorStep second_step;            /* 0x34-0x5B: the second record (cursorBufferHandlePad) */
    /*
     * 0x5C: the track object handed to fontObjStartOpen, fontObjStartClose and
     * fontObjAdvanceTracks (main/tu097). Only its start offset is evidenced; its
     * extent is not, so the one-byte size is a placeholder and sizeof(CursorBuffer)
     * must not be used. The calls cast it to struct FontObject *.
     */
    unsigned char track_object[1];
};
typedef struct CursorBuffer CursorBuffer;

void cursorBufferInit(CursorBuffer *buffer, int capacity, int unit_size, int mode_flags);
void cursorBufferHandlePad(CursorBuffer *buffer);
void cursorBufferActivate(CursorBuffer *buffer);
void cursorBufferUpdateTracks(CursorBuffer *buffer);
void cursorBufferDeactivate(CursorBuffer *buffer);
void cursorBufferReset(CursorBuffer *buffer);
void cursorBufferTick(CursorBuffer *buffer);
void cursorBufferWrapOrClamp(CursorBuffer *buffer);
void cursorBufferSetOffset(CursorBuffer *buffer, int cursor);
void cursorBufferMoveBy(CursorBuffer *buffer, int delta);
void cursorBufferMoveUnits(CursorBuffer *buffer, int units);
void cursorBufferSetCapacity(CursorBuffer *buffer, int capacity);
unsigned int cursorBufferGetIndex(CursorBuffer *buffer);
int cursorBufferGetAlignedOffset(CursorBuffer *buffer);

#endif /* INCLUDE_MAIN_TU098_H */
