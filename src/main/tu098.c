#include "common.h"
#include "main/tu097.h"

INCLUDE_ASM("asm/main/nonmatchings/tu098", cursorBufferHandlePad);

typedef struct CursorBuffer CursorBuffer;

/*
 * Fields named from the loads and stores of the functions in this file.
 * Roles are inferred from access patterns only. cursorBufferHandlePad reads a second
 * step record at offset 0x34: its fields sit at 0x48-0x59, the same layout as
 * 0x14-0x25 here. Offsets 0x26-0x47 are not accessed in this file.
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
    unsigned char unmodeled_26[0x36];  /* 0x26-0x5B: not accessed in this file */
    /*
     * 0x5C: the track object handed to func_0020DCF0, fontObjStartClose and
     * fontObjAdvanceTracks (main/tu097 treats such a block as struct Tu097Object).
     * Only its start offset is evidenced; its extent is not, so the one-byte
     * size is a placeholder and sizeof(CursorBuffer) must not be used.
     */
    unsigned char track_object[1];
};

int func_0020DCF0(void *object);

/* Activates the object: sets the active flag in state and starts the track object. */
void cursorBufferActivate(CursorBuffer *buffer) {
    if (buffer != 0) {
        buffer->state = buffer->state | 0x10000;
        func_0020DCF0(buffer->track_object);
    }
}


/* Advances the track object unless state bit 0x1 is set. */
void cursorBufferUpdateTracks(CursorBuffer *buffer) {
    if (buffer != 0) {
        if (!(buffer->state & 1)) {
            fontObjStartClose(buffer->track_object);
        }
    }
}


/* Deactivates the object: clears the active flag in state and stops the track object. */
void cursorBufferDeactivate(CursorBuffer *buffer) {
    if (buffer != 0) {
        buffer->state = buffer->state & 0xFFFEFFFF;
        fontObjAdvanceTracks(buffer->track_object);
    }
}

/* Resets the record: status bit 0x10000 cleared, state takes mode, aux_value and count zeroed. */
void cursorBufferReset(CursorBuffer *buffer) {
    int mode;

    mode = buffer->mode;
    buffer->status_flags &= 0xFFFEFFFF;
    buffer->state = mode;
    buffer->aux_value = 0;
    buffer->count = 0;
}

/*
 * One tick: count grows by step, state takes mode, status bit 0x20000 is
 * cleared. cursorBufferHandlePad inlines the same body on the record at 0x34.
 */
void cursorBufferTick(CursorBuffer *buffer) {
    int status;
    int mode;
    int count;

    status = buffer->status_flags & 0xFFFDFFFF;
    mode = buffer->mode;
    count = buffer->count + buffer->step;
    buffer->state = mode;
    buffer->count = count;
    buffer->status_flags = status;
}

/* Initializes the cursor over a buffer of capacity bytes with unit_size-byte elements; sets mode bit 0x100. */
void cursorBufferInit(CursorBuffer *buffer, int capacity, int unit_size, int mode_flags) {
    buffer->mode_flags = mode_flags | 0x100;
    buffer->unit_size = unit_size;
    buffer->cursor = 0;
    buffer->previous_cursor = 0;
    buffer->capacity = capacity;
}

INCLUDE_ASM("asm/main/nonmatchings/tu098", cursorBufferWrapOrClamp);

/* Keeps cursor inside [0, capacity): a held direction clamps, a fresh one wraps (mode bit 0x100). */
int cursorBufferWrapOrClamp(CursorBuffer *buffer);

/* Sets the cursor to an absolute byte offset, then clamps or wraps it. */
void cursorBufferSetOffset(CursorBuffer *buffer, int cursor) {
    int old_cursor;

    old_cursor = buffer->cursor;
    buffer->cursor = cursor;
    buffer->previous_cursor = old_cursor;
    cursorBufferWrapOrClamp(buffer);
}

int seVoiceStartDefault(int code);

/* Moves the cursor by delta bytes, clamps or wraps it, and requests sound 0x01000001 when it moved. */
void cursorBufferMoveBy(CursorBuffer *buffer, int delta) {
    int old_cursor;

    old_cursor = buffer->cursor;
    buffer->previous_cursor = old_cursor;
    buffer->cursor = old_cursor + delta;
    cursorBufferWrapOrClamp(buffer);
    if (buffer->previous_cursor != buffer->cursor) {
        seVoiceStartDefault(0x01000001);
    }
}

/* Moves the cursor by units whole elements (units * unit_size bytes), then clamps or wraps it. */
void cursorBufferMoveUnits(CursorBuffer *buffer, int units) {
    int old_cursor;
    int delta;

    old_cursor = buffer->cursor;
    delta = units * buffer->unit_size;
    buffer->previous_cursor = old_cursor;
    buffer->cursor = delta + old_cursor;
    cursorBufferWrapOrClamp(buffer);
    if (buffer->previous_cursor != buffer->cursor) {
        seVoiceStartDefault(0x01000001);
    }
}

/* Sets the buffer capacity and pulls the cursor back inside it. */
void cursorBufferSetCapacity(CursorBuffer *buffer, int capacity) {
    int cursor;
    int last;

    cursor = buffer->cursor;
    last = capacity - 1;
    buffer->capacity = capacity;
    if (cursor >= capacity) {
        buffer->cursor = last;
        cursor = last;
    }
    if (cursor < 0) {
        buffer->cursor = 0;
    }
}

/* Element index under the cursor: cursor / unit_size, or 0 without a unit size. */
unsigned int cursorBufferGetIndex(CursorBuffer *buffer) {
    unsigned int unit_size;

    unit_size = buffer->unit_size;
    if (!(unit_size != 0)) {
        return 0;
    }
    return (unsigned int) buffer->cursor / unit_size;
}

/* Cursor rounded down to a whole element: index * unit_size, or 0 without a unit size. */
int cursorBufferGetAlignedOffset(CursorBuffer *buffer) {
    unsigned int unit_size;

    unit_size = buffer->unit_size;
    if (!(unit_size != 0)) {
        return 0;
    }
    return ((unsigned int) buffer->cursor / unit_size) * unit_size;
}
