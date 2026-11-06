/*
 * OV01 original TU 44: 0x00a9c928..0x00a9ee28 (91 functions).
 *
 * The SB script VM of overlay 01: thread records, operand stacks, image
 * headers ("SB  " / "SBL " magic), a 46-entry opcode table and a native
 * name table. The same 0x40-byte record is viewed through the Tu044 structs
 * below (control, object, record); the views are named for their use here.
 */
#include "common.h"
#include "shared.h"
#include "ov01/tu044.h"

struct SlotEntry {
    unsigned char unmodeled_00[0x10];
};

/*
 * entries is a table of 0x10-byte records that starts at +0x14; its length is
 * not evidenced here, so only the first record is declared.
 */
struct SlotChain {
    int state;
    int mask;
    int pending;
    int value;
    int previous_state;
    struct SlotEntry entries[1];
};

/* The SB image header: magic at +0, the code offset at +0x14, the global table offset at +0x18. */
struct SbImageHeader {
    unsigned char unmodeled_00[0x14];
    int code_offset;
    int global_offset;
};

struct SlotTable {
    unsigned char unmodeled_00[0x8];
    int count;
    unsigned char unmodeled_0c[0x14 - 0xC];
    struct SbImageHeader *layout;
    unsigned char unmodeled_18[0x1C - 0x18];
    int *slots;
    unsigned char unmodeled_20[0x28 - 0x20];
    struct SlotChain *chain;
};

/*
 * One 0x40-byte record of sbThreadTable. Only the fields these functions use
 * are named; the gaps keep the record's size and field offsets. The word at
 * +0x14 is the lookup key of a record (the link word of the in-use test);
 * the word at +0x18 is its value, which the update path also reads as the
 * replacement total.
 */
typedef struct SbThread {
    int total;
    int count;
    int limit;
    unsigned char unmodeled_0C[0x8];
    int key;
    int value;
    unsigned char unmodeled_1C[0x10];
    int status;
    unsigned char unmodeled_30[0x10];
} SbThread;

extern SbThread sbThreadTable[80];

SbThread *sbThreadTableGet(void) {
    return sbThreadTable;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbInit);

extern int sbNativeOwnerCount;
extern int sbNativeMethodCount;

void sbResetNativeCounts(void) {
    sbNativeMethodCount = 0;
    sbNativeOwnerCount = 0;
}



extern int (*sbOpHandlers[])(SbThread *, unsigned int);
/* The span a control points at: begin (0x14) and end (0x18) are read together as 32-bit values. */
typedef struct SbCodeRange {
    unsigned char unmodeled_00[0x14];
    int begin;
    int end;
} SbCodeRange;

typedef struct SbRunner {
    int count;
    unsigned char unmodeled_04[0x10];
    SbCodeRange *span;
    unsigned char unmodeled_18[0x14];
    int state;
    int remaining;
} SbRunner;

void sbRun(SbRunner *control);
void sbFatal(SbThread *record);
u16 sbCodeU16(struct SlotTable *table, int index);
int sbCodeS32(SbThread *record, int value);
int sbLocalSet(struct SlotTable *table, int index, int value);
void sbStackPush(SbThread *record, int value);
typedef struct SbCursor {
    int first;                                  /* +0x00 */
    int last;                                   /* +0x04 */
    unsigned char unmodeled_08[0x0C - 0x08];
    int index;                                  /* +0x0C */
    int used;                                   /* +0x10 */
} SbCursor;

typedef struct SbFrame {
    int sum;                                    /* +0x00 */
    int count;                                  /* +0x04 */
    int tag;                                    /* +0x08 */
    int state;                                  /* +0x0C */
    int counter;                                /* +0x10 */
    unsigned char unmodeled_14[0x1C - 0x14];
    int *stack;                                 /* +0x1C */
    unsigned char unmodeled_20[0x28 - 0x20];
    SbCursor *cursor;                        /* +0x28 */
} SbFrame;

int sbStackPop(SbFrame *obj);
int sbOpLength(int id, int index);

int sbRunAll(void)
{
  SbThread *first_record;
  int *status_field;
  SbThread *record;
  int index;
  int more;

  first_record = sbThreadTable;
  status_field = &sbThreadTable[0].status;
  record = first_record;
  index = 0;
  do
  {
    /* status_field[-6] is the key word of the same record (offset 0x14). */
    if (status_field[-6] != 0 && !(*status_field & 0x80000000))
    {
      sbRun(record);
    }
    index += 1;
    status_field += 16;
    more = index < 80;
    record += 1;
  }
  while (more != 0);
  return more;
}





void func_00A9CC80(SbRunner *);
int sbStep(SbThread *record);

void sbRun(SbRunner *control)
{
    unsigned int available;

    available = (unsigned int) (control->span->end - control->span->begin) >> 1;
    do {
        if ((int) available < control->count) {
            sbFatal(control);
        }
        func_00A9CC80(control);
    } while (sbStep(control) == 0);
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbStartImage);

extern unsigned char sbSlotTables[];

void *sbSlotTableGet(int index) {
    return sbSlotTables + (index << 7);
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbSlotFind);

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbSlotStore);

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbSlotRemove);

void sbChainSetValue(struct SlotChain *chain, int value) {
    chain->value = value;
    chain->previous_state = chain->state;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", func_00A9CC80);





void func_00A9CD30(SbFrame *obj)
{
    if (obj->state == 1) {
        return;
    }
    if (obj->state == 2) {
        sbCodeS32(obj, obj->counter + 1);
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbFatal);

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbNativeOwnerAdd);

int sbIsSbImage(int *header) {
    if (header == 0) {
        return 0;
    }
    return *header == 0x20204253;
}

int sbIsSblImage(int *header) {
    if (header == 0) {
        return 0;
    }
    return *header == 0x204C4253;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", func_00A9CEE8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbLinkImage);

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbFindNative);

extern int sbFlags;

int sbFlagsGet(void) {
    return sbFlags;
}

void sbFlagsSet(int bits) {
    sbFlags |= bits;
}

void sbFlagsClear(int bits) {
    sbFlags &= ~bits;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbThreadStart);

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbFindThread);

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbThreadRelease);

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbThreadEnqueue);

int sbFindThread(void);
int sbCursorPop(int);
int sbChainPreviousState(struct SlotTable *table);
int sbChainEntryGet(struct SlotTable *table, int index);

/* Dequeue side of sbThreadEnqueue: pops the next cursor index of the thread and takes its chain entry. */
void sbThreadDequeue(void)
{
    int thread;
    int index;

    thread = sbFindThread();
    if (thread != 0 && sbChainPreviousState(thread) != 0) {
        index = sbCursorPop(thread);
        sbChainEntryGet(thread, index);
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbNameTableFind);

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbNameTableNameOf);

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", func_00A9D608);

#define TU044_RECORD_COUNT 80

void sbThreadRelease(int, int);

void sbThreadsRelease(int key)
{
    int index;

    for (index = 0; index < TU044_RECORD_COUNT; index++) {
        if (sbThreadTable[index].key == key) {
            sbThreadRelease(key, sbThreadTable[index].value);
        }
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbThreadSuspend);

void sbThreadSuspend(int, int);

void sbThreadsSuspend(int key)
{
    int index;

    for (index = 0; index < TU044_RECORD_COUNT; index++) {
        if (sbThreadTable[index].key == key) {
            sbThreadSuspend(key, sbThreadTable[index].value);
        }
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbThreadResume);

void sbThreadResume(int, int);

void sbThreadsResume(int key)
{
    int index;

    for (index = 0; index < TU044_RECORD_COUNT; index++) {
        if (sbThreadTable[index].key == key) {
            sbThreadResume(key, sbThreadTable[index].value);
        }
    }
}

extern unsigned char sbChains[];

void *sbChainGet(int index) {
    return sbChains + index * 0x114;
}

void sbChainReset(struct SlotTable *table) {
    struct SlotChain *chain;

    chain = table->chain;
    chain->mask = 0xF;
    chain->state = 0;
    chain->value = 0;
    chain->previous_state = (chain->pending = 0);
}

int sbCursorPush(SbFrame *obj)
{
    SbCursor *cursor;
    int next;

    cursor = obj->cursor;
    if (!(cursor->used < (cursor->last - cursor->first) + 1)) {
        return -1;
    }
    cursor->used = cursor->used + 1;
    next = cursor->index + 1;
    cursor->index = next;
    if (cursor->last < next) {
        cursor->index = cursor->first;
    }
    return cursor->index;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbCursorPop);

int sbChainPreviousState(struct SlotTable *table) {
    return table->chain->previous_state;
}

int sbChainEntryGet(struct SlotTable *table, int index) {
    return (int) &table->chain->entries[index];
}

extern unsigned char sbStacks[];

void *sbStackGet(int index) {
    return sbStacks + (index << 9);
}

u16 sbCodeU16(struct SlotTable *table, int index) {
    struct SbImageHeader *layout;

    layout = table->layout;
    return ((u16 *) ((unsigned char *) layout + layout->code_offset))[index];
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbCodeS32);

int sbGlobalGet(struct SlotTable *table, int index) {
    struct SbImageHeader *layout;

    layout = table->layout;
    return ((int *) ((unsigned char *) layout + layout->global_offset))[index];
}

void sbGlobalSet(struct SlotTable *table, int index, int value) {
    struct SbImageHeader *layout;

    layout = table->layout;
    ((int *) ((unsigned char *) layout + layout->global_offset))[index] = value;
}

/* Local slot read: the locals sit above the operand stack, at table->slots + count words. */
int sbLocalGet(struct SlotTable *table, int index)
{
    int *locals;

    locals = table->slots + table->count;
    return locals[index];
}

/* Local slot write; returns the locals' byte offset (count * 4). */
int sbLocalSet(struct SlotTable *table, int index, int value) {
    int used_bytes;
    int *locals;

    used_bytes = table->count * 4;
    locals = table->slots + table->count;
    locals[index] = value;
    return used_bytes;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbStackPush);

int sbStackPop(SbFrame *obj)
{
    int top;

    if (obj->count <= 0) {
        sbFatal(obj);
    }
    top = obj->count - 1;
    obj->count = top;
    return obj->stack[top];
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbOpLength);

/* One step of the VM: fetch the opcode at pc (record->total) and run its handler. */
int sbStep(SbThread *record)
{
    unsigned int opcode;
    int original_total;
    int result;

    opcode = sbCodeU16(record, record->total);
    if (opcode < 0x2E) {
        original_total = record->total;
        result = sbOpHandlers[opcode](record, opcode);
        if (result == 1) {
            record->status = result;
            record->total = original_total;
        } else {
            record->status = 0;
        }
        if (result == 2) {
            record->total = record->value;
        }
    } else {
        result = 2;
        sbFatal(record);
    }
    return result;
}

/* Opcode 0x00: no effect; advances past the opcode. */
int sbOpNop(SbFrame *obj, int opcode)
{
    obj->sum += sbOpLength(opcode & 0xFFFF, 0);
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbOpBinary);

/* Opcode 0x02: no effect; skips the opcode and its operand word. */
int func_00A9DDE0(SbFrame *obj, int opcode)
{
    obj->sum += sbOpLength(opcode & 0xFFFF, 0);
    return 0;
}

int sbOpPushWord(SbRunner *control, int amount)
{
    unsigned short amount16;
    int slot;

    amount16 = (unsigned short) amount;
    slot = sbCodeS32(control, control->count + 1);
    sbStackPush(control, slot);
    control->count += sbOpLength(amount16, 0);
    return 0;
}

/* Opcode 0x04: pushes the operand word after the opcode, as sbOpPushWord does. */
int func_00A9DE80(SbRunner *control, int opcode)
{
    unsigned short opcode16;
    int operand;

    opcode16 = (unsigned short) opcode;
    operand = sbCodeS32(control, control->count + 1);
    sbStackPush(control, operand);
    control->count += sbOpLength(opcode16, 0);
    return 0;
}

/* Opcode 0x05: no effect; skips the opcode and its operand word. */
int func_00A9DEE0(SbFrame *obj, int opcode)
{
    obj->sum += sbOpLength(opcode & 0xFFFF, 0);
    return 0;
}

/* Opcode 0x06: pushes the operand word after the opcode, as sbOpPushWord does. */
int func_00A9DF20(SbRunner *control, int opcode)
{
    unsigned short opcode16;
    int operand;

    opcode16 = (unsigned short) opcode;
    operand = sbCodeS32(control, control->count + 1);
    sbStackPush(control, operand);
    control->count += sbOpLength(opcode16, 0);
    return 0;
}

/* Opcode 0x07: pushes the operand word after the opcode, reading the pc through the SbFrame view (sum). */
int func_00A9DF80(SbFrame *obj, int opcode)
{
    int opcode16;
    int operand;

    opcode16 = opcode & 0xFFFF;
    operand = sbCodeS32(obj, obj->sum + 1);
    sbStackPush(obj, operand);
    obj->sum += sbOpLength(opcode16, 0);
    return 0;
}

int sbGlobalGet(struct SlotTable *table, int index);

int sbOpPushGlobal(SbRunner *control, int amount)
{
    unsigned short amount16;
    int slot;
    int next_slot;

    amount16 = (unsigned short) amount;
    slot = sbCodeS32(control, control->count + 1);
    next_slot = sbGlobalGet(control, slot);
    sbStackPush(control, next_slot);
    control->count += sbOpLength(amount16, 0);
    return 0;
}

int sbLocalGet(struct SlotTable *table, int index);

int sbOpPushLocal(SbRunner *control, int amount)
{
    unsigned short amount16;
    int slot;
    int next_slot;

    amount16 = (unsigned short) amount;
    slot = sbCodeS32(control, control->count + 1);
    next_slot = sbLocalGet(control, slot);
    sbStackPush(control, next_slot);
    control->count += sbOpLength(amount16, 0);
    return 0;
}

void sbGlobalSet(struct SlotTable *table, int index, int value);

int sbOpStoreGlobal(SbRunner *control, int amount)
{
    unsigned short amount16;
    int first;
    int second;

    amount16 = (unsigned short) amount;
    first = sbStackPop(control);
    second = sbStackPop(control);
    sbGlobalSet(control, first, second);
    control->count += sbOpLength(amount16, 0);
    return 0;
}

int sbOpStoreLocal(SbRunner *control, int amount)
{
    unsigned short amount16;
    int first;
    int second;

    amount16 = (unsigned short) amount;
    first = sbStackPop(control);
    second = sbStackPop(control);
    sbLocalSet(control, first, second);
    control->count += sbOpLength(amount16, 0);
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", func_00A9E1A0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", func_00A9E220);

int sbOpJump(SbFrame *obj)
{
    obj->sum = sbCodeS32(obj, obj->sum + 1);
    return 0;
}

int sbOpJumpIfZero(SbThread *record, unsigned short id)
{
    int next;

    next = sbCodeS32(record, record->total + 1);
    if (sbStackPop(record) == 0) {
        record->total = next;
    } else {
        record->total += sbOpLength(id, 0);
    }
    return 0;
}

int sbOpJumpIfNotZero(SbThread *record, unsigned short id)
{
    int next;

    next = sbCodeS32(record, record->total + 1);
    if (sbStackPop(record) != 0) {
        record->total = next;
    } else {
        record->total += sbOpLength(id, 0);
    }
    return 0;
}

/*
 * Opcode 0x11: pushes a call frame header (return slot, saved limit, frame
 * marker) and records the marker in the local at -1.
 */
int sbOpEnterFrame(SbThread *record, unsigned short opcode)
{
    sbStackPush(record, 0);
    sbStackPush(record, record->limit);
    sbStackPush(record, 0);
    sbLocalSet(record, -1, record->count);
    record->total += sbOpLength(opcode, 0);
    return 0;
}

/* Opcode 0x12: call. Stores the return position in the frame's return slot and jumps to the operand. */
int sbOpCall(SbFrame *obj)
{
    obj->tag = sbLocalGet(obj, -1);
    sbLocalSet(obj, -3, obj->sum + 2);
    obj->sum = sbCodeS32(obj, obj->sum + 1);
    return 0;
}

/* Opcode 0x13: return. Restores the stack depth, the saved limit and the pc from the frame header. */
int sbOpReturn(SbThread *record, unsigned short opcode)
{
    if (record->count == 0) {
        sbFatal(record);
    }
    record->count = record->limit;
    sbStackPop(record);
    record->limit = sbStackPop(record);
    record->total = sbStackPop(record);
    record->total += sbOpLength(opcode, 0);
    return 0;
}

/* Opcode 0x14: return with a value. The top of the stack survives the frame restore. */
int sbOpReturnValue(SbThread *record, unsigned short opcode)
{
    int returnValue;

    returnValue = sbStackPop(record);
    if (record->count == 0) {
        sbFatal(record);
    }
    record->count = record->limit;
    sbStackPop(record);
    record->limit = sbStackPop(record);
    record->total = sbStackPop(record);
    sbStackPush(record, returnValue);
    record->total += sbOpLength(opcode, 0);
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbOpCallNative);

int sbThreadStart(SbCodeRange *, int);

int sbOpSpawn(SbRunner *control, int amount)
{
    unsigned short amount16;
    int slot;

    amount16 = (unsigned short) amount;
    slot = sbStackPop(control);
    control->count += sbOpLength(amount16, 0);
    return (sbThreadStart(control->span, slot) != 0) ? 0 : 2;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbOpKillThread);

int sbOpDup(SbFrame *obj, int value)
{
    int operand;

    operand = value & 0xFFFF;
    sbStackPush(obj, obj->stack[obj->count - 1]);
    obj->sum += sbOpLength(operand, 0);
    return 0;
}

int sbOpDrop(SbFrame *obj, int value)
{
    obj->count -= 1;
    obj->sum += sbOpLength(value & 0xFFFF, 0);
    return 0;
}

int sbOpLoadGlobal(SbRunner *control, int amount)
{
    unsigned short amount16;
    int slot;
    int next_slot;

    amount16 = (unsigned short) amount;
    slot = sbStackPop(control);
    next_slot = sbGlobalGet(control, slot);
    sbStackPush(control, next_slot);
    control->count += sbOpLength(amount16, 0);
    return 0;
}

int sbOpLoadLocal(SbRunner *control, int amount)
{
    unsigned short amount16;
    int slot;
    int next_slot;

    amount16 = (unsigned short) amount;
    slot = sbStackPop(control);
    next_slot = sbLocalGet(control, slot);
    sbStackPush(control, next_slot);
    control->count += sbOpLength(amount16, 0);
    return 0;
}

/* Opcode 0x1C: returns 2, which sbStep takes as a rewind of the pc to the thread entry. */
int sbOpEnd(SbFrame *obj, int opcode)
{
    obj->sum += sbOpLength(opcode & 0xFFFF, 0);
    return 2;
}

/* Opcode 0x1D: returns 3; sbStep keeps the advanced pc and the run stops for this frame. */
int sbOpYield(SbFrame *obj, int opcode)
{
    obj->sum += sbOpLength(opcode & 0xFFFF, 0);
    return 3;
}

int sbOpWaitFrames(SbRunner *control, int amount)
{
    unsigned short amount16;
    int result;

    amount16 = (unsigned short) amount;
    if (control->state == 0) {
        control->remaining = sbStackPop(control);
        result = 1;
    } else {
        control->remaining -= 1;
        if (control->remaining <= 0) {
            result = 0;
            control->count += sbOpLength(amount16, 0);
        } else {
            result = 1;
        }
    }
    return result;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", func_00A9E9E0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", func_00A9EA80);

int sbOpIncGlobal(SbRunner *control, int amount)
{
    unsigned short amount16;
    int first;
    int second;

    amount16 = (unsigned short) amount;
    first = sbStackPop(control);
    second = sbGlobalGet(control, first);
    sbGlobalSet(control, first, second + 1);
    control->count += sbOpLength(amount16, 0);
    return 0;
}

int sbOpIncLocal(SbRunner *control, int amount)
{
    unsigned short amount16;
    int first;
    int second;

    amount16 = (unsigned short) amount;
    first = sbStackPop(control);
    second = sbLocalGet(control, first);
    sbLocalSet(control, first, second + 1);
    control->count += sbOpLength(amount16, 0);
    return 0;
}

int sbOpDecGlobal(SbRunner *control, int amount)
{
    unsigned short amount16;
    int first;
    int second;

    amount16 = (unsigned short) amount;
    first = sbStackPop(control);
    second = sbGlobalGet(control, first);
    sbGlobalSet(control, first, second - 1);
    control->count += sbOpLength(amount16, 0);
    return 0;
}

int sbOpDecLocal(SbRunner *control, int amount)
{
    unsigned short amount16;
    int first;
    int second;

    amount16 = (unsigned short) amount;
    first = sbStackPop(control);
    second = sbLocalGet(control, first);
    sbLocalSet(control, first, second - 1);
    control->count += sbOpLength(amount16, 0);
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbOpSwitch);
