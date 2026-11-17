/*
 * OV01 original TU 44: 0x00a9c928..0x00a9ee28 (91 functions).
 *
 * The SB script VM of overlay 01: thread records, operand stacks, image
 * headers ("SB  " / "SBL " magic), a 46-entry opcode table and a native
 * name table. A thread is one 0x40-byte SbThread record of sbThreadTable.
 */
#include "common.h"
#include "shared.h"
#include "ov01/tu044.h"

/* One 0x10-byte message of a thread's chain (sbThreadEnqueue): the caller's
 * words, with sender replaced by the sending thread's value (-1 for none). */
typedef struct SbMessage {
    int kind;                           /* +0x00 */
    int sender;                         /* +0x04 */
    int args[2];                        /* +0x08 */
} SbMessage;

/* The SB image header: magic at +0, the native name count at +0xC, the code
 * offset at +0x14, the global table offset at +0x18 and the name table offset
 * at +0x20 (offsets count from the header). */
struct SbImageHeader {
    unsigned char unmodeled_00[0x08];
    int entry;                          /* +0x08: the code entry sbStartImage starts */
    int name_count;                     /* +0x0C */
    unsigned char unmodeled_10[0x04];
    int code_offset;                    /* +0x14 */
    int global_offset;                  /* +0x18 */
    unsigned char unmodeled_1c[0x04];
    int name_offset;                    /* +0x20 */
};

/* One entry of an image's name table: the name (offset from the header) and its value. */
typedef struct SbNameEntry {
    int name_offset;
    int value;
} SbNameEntry;

/* One 0x10-byte slot of a thread's slot table: key 0 marks a free slot.
 * sbSlotStore copies whole slots; the rest of the slot is not read here. */
typedef struct SbSlot {
    int key;                           /* +0x00 */
    int target;                        /* +0x04: code position sbSlotJump jumps to */
    int low;                           /* +0x08: lowest position the slot covers */
    int high;                          /* +0x0C: highest position the slot covers */
} SbSlot;

/* Slots in a thread's slot table (sbSlotFind and sbSlotStore scan 8). */
#define SB_SLOT_COUNT 8

/* Messages in a thread's queue (sbChainReset: first 0, last 0xF). */
#define SB_QUEUE_LAST 0xF

/* A thread's message queue (sbChainGet, 0x114 bytes): a ring of message indices from
 * first to last, the read index head, the write index tail and the count used;
 * sbThreadEnqueue writes messages[tail], sbThreadDequeue reads messages[head]. */
typedef struct SbQueue {
    int first;                          /* +0x00 */
    int last;                           /* +0x04 */
    int head;                           /* +0x08: read index (sbCursorPop) */
    int tail;                           /* +0x0C: write index (sbCursorPush, sbChainSetValue) */
    int used;                           /* +0x10 */
    SbMessage messages[SB_QUEUE_LAST + 1]; /* +0x14 */
} SbQueue;

/*
 * One 0x40-byte record of sbThreadTable: a script thread. image (+0x14) is its key
 * (0 marks a free record; sbFindThread matches it with value), pc counts halfwords
 * from the image's code, the operand stack (+0x1C, stackLimit words) holds sp words
 * and the thread's locals, slots are its jump slots and queue its messages.
 */
typedef struct SbThread {
    int pc;                             /* +0x00 */
    int sp;                             /* +0x04: stack words in use */
    int frame;                          /* +0x08: stack index of the local frame (3 at start; sbOpCall saves it) */
    int jumpKey;                        /* +0x0C: pending slot jump (sbSlotJump) */
    int jumpPosition;                   /* +0x10 */
    struct SbImageHeader *image;        /* +0x14 */
    int value;                          /* +0x18: the entry it started at */
    int *stack;                         /* +0x1C */
    int stackLimit;                     /* +0x20 */
    SbSlot *slots;                      /* +0x24 */
    SbQueue *queue;                     /* +0x28 */
    int status;                         /* +0x2C: SB_THREAD_SUSPENDED; 0 on the first run of a wait */
    int wait[4];                        /* +0x30: wait[0] counts sbOpWaitFrames down; sbThreadStart clears it */
} SbThread;


extern SbThread sbThreadTable[80];

SbThread *sbThreadTableGet(void) {
    return sbThreadTable;
}

extern int sbFlags;
extern unsigned char sbStacks[];
extern unsigned char sbSlotTables[];
extern unsigned char sbChains[];
void sbResetNativeCounts(void);
void *memset(void *destination, int value, unsigned int count);

/* Thread records and the per-thread stacks (0x200 bytes), slot tables (0x80) and chains (0x114). */
#define SB_THREAD_COUNT 80
#define SB_STACK_SIZE 0x200
#define SB_SLOT_TABLE_SIZE 0x80
#define SB_CHAIN_SIZE 0x114

/* Frees every thread record and clears the per-thread areas and the native counts. */
void sbInit(void)
{
    int index;

    sbFlags = 0;
    for (index = SB_THREAD_COUNT - 1; index >= 0; index--) {
        sbThreadTable[index].image = 0;
    }
    memset(sbStacks, 0, SB_THREAD_COUNT * SB_STACK_SIZE);
    memset(sbSlotTables, 0, SB_THREAD_COUNT * SB_SLOT_TABLE_SIZE);
    memset(sbChains, 0, SB_THREAD_COUNT * SB_CHAIN_SIZE);
    sbResetNativeCounts();
}

/* A native method scripts call by owner and method index (sbOpCallNative); a
 * table of them ends with a null name. status 1 from func keeps the frame waiting. */
typedef struct SbNativeMethod {
    const char *name;
    int (*func)(SbThread *record, int *status);
} SbNativeMethod;

/* A named table of native methods, registered by sbNativeOwnerAdd. */
typedef struct SbNativeOwner {
    const char *name;
    SbNativeMethod *methods;
    int count;
} SbNativeOwner;

#define SB_NATIVE_OWNER_MAX 0x600
#define SB_NATIVE_METHOD_MAX 0x800

extern SbNativeOwner sbNativeOwners[SB_NATIVE_OWNER_MAX];
extern int sbNativeOwnerCount;
extern SbNativeMethod sbNativeMethods[SB_NATIVE_METHOD_MAX];
extern int sbNativeMethodCount;

void sbResetNativeCounts(void) {
    sbNativeMethodCount = 0;
    sbNativeOwnerCount = 0;
}



extern int (*sbOpHandlers[])(SbThread *, unsigned int);
void sbRun(SbThread *control);
void sbFatal(SbThread *record);
u16 sbCodeU16(SbThread *table, int index);
int sbCodeS32(SbThread *record, int value);
void sbLocalSet(SbThread *table, int index, int value);
void sbStackPush(SbThread *obj, int value);
int sbStackPop(SbThread *obj);
int sbOpLength(unsigned short opcode, int count);

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





int sbSlotJump(SbThread *obj);
int sbStep(SbThread *record);

void sbRun(SbThread *control)
{
    unsigned int available;

    available = (unsigned int) (control->image->global_offset - control->image->code_offset) >> 1;
    do {
        if ((int) available < control->pc) {
            sbFatal(control);
        }
        sbSlotJump(control);
    } while (sbStep(control) == 0);
}

int sbIsSbImage(int *header);
int sbThreadStart(struct SbImageHeader *key, int entry);

/* sbStartImage starts a thread at the image's entry and stops for good on an image without the SB
 * magic; it stays asm: the original calls sbThreadStart with jal, every plain C form gives a tail call. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbStartImage);

extern unsigned char sbSlotTables[];

void *sbSlotTableGet(int index) {
    return sbSlotTables + (index << 7);
}

/* The thread's slot with this key, or 0. */
SbSlot *sbSlotFind(SbThread *thread, int key)
{
    SbSlot *slot = thread->slots;
    int i;

    for (i = 0; i < SB_SLOT_COUNT; i++, slot++) {
        if (slot->key == key) {
            return slot;
        }
    }
    return 0;
}

/* Copies entry into the slot with its key, else into the first free slot;
 * returns 0 when the table is full. */
int sbSlotStore(SbThread *thread, SbSlot *entry)
{
    SbSlot *slot = sbSlotFind(thread, entry->key);
    SbSlot *slots;
    int i;

    if (slot == 0) {
        slots = thread->slots;
        for (i = 0; i < SB_SLOT_COUNT; i++) {
            if (slots[i].key == 0) {
                slot = &slots[i];
                break;
            }
        }
    }
    if (slot != 0) {
        *slot = *entry;
        return 1;
    }
    return 0;
}

/* Frees the slot with this key; returns 0 when there is none. */
int sbSlotRemove(SbThread *thread, int key)
{
    SbSlot *slot = sbSlotFind(thread, key);

    if (slot == 0) {
        return 0;
    }
    slot->key = 0;
    return 1;
}

/* Requests a jump to slot key at the current position (sbSlotJump runs it); natives
 * call it with key 2 when they get too few arguments. */
void sbChainSetValue(SbThread *thread, int key) {
    thread->jumpKey = key;
    thread->jumpPosition = thread->pc;
}

void func_00A9CD30(SbThread *obj);

/* Pending slot jump of a frame (state = slot key, counter = position): an unknown
 * key is fatal; a position inside the slot's range jumps to its target. */
int sbSlotJump(SbThread *obj)
{
    int key;
    SbSlot *slot;
    int position;

    key = obj->jumpKey;
    if (key == 0) {
        return 0;
    }
    slot = sbSlotFind(obj, key);
    if (slot == 0) {
        obj->pc = obj->jumpPosition;
        if (key < 3) {
            func_00A9CD30(obj);
        }
        sbFatal(obj);
        return 1;
    }
    position = obj->jumpPosition;
    obj->jumpKey = 0;
    obj->jumpPosition = 0;
    if (position >= slot->low) {
        if (slot->high >= position) {
            obj->pc = slot->target;
            return 1;
        }
    }
    return 0;
}





void func_00A9CD30(SbThread *obj)
{
    if (obj->jumpKey == 1) {
        return;
    }
    if (obj->jumpKey == 2) {
        sbCodeS32(obj, obj->jumpPosition + 1);
    }
}

/* Empty in this build (tu046); the thread and its first word are passed as for a debug print. */
void func_00A9F2E0(SbThread *record, int total);

/* Fatal VM error (stack overflow in sbRun): reports the thread, then stops for good. */
void sbFatal(SbThread *record)
{
    func_00A9F2E0(record, record->pc);
    for (;;) {
    }
}

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

int strcmp(const char *a, const char *b);

/* Native id of a method: owner index in the high half, method index in the low
 * half; -1 when the owner or the method is not registered. */
int sbFindNative(const char *ownerName, const char *methodName)
{
    int owner;
    int method;

    for (owner = 0; owner < sbNativeOwnerCount; owner++) {
        if (strcmp(ownerName, sbNativeOwners[owner].name) == 0) {
            for (method = 0; method < sbNativeOwners[owner].count; method++) {
                if (strcmp(methodName, sbNativeOwners[owner].methods[method].name) == 0) {
                    return (owner << 16) + method;
                }
            }
        }
    }
    return -1;
}

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

void *sbStackGet(int index);
void *sbSlotTableGet(int index);
void *sbChainGet(int index);
void sbChainReset(SbThread *table);

/* Starts a thread in the first free record: key and entry name it, the pc starts at entry. */
int sbThreadStart(struct SbImageHeader *key, int entry)
{
    SbThread *thread;
    int index;

    thread = sbThreadTable;
    for (index = 0; index < SB_THREAD_COUNT; index++) {
        if (thread->image == 0) {
            memset(thread->wait, 0, sizeof(thread->wait));
            thread->pc = entry;
            thread->frame = 3;
            thread->image = key;
            thread->value = entry;
            thread->sp = 0;
            thread->jumpKey = 0;
            thread->jumpPosition = 0;
            thread->stack = sbStackGet(index);
            thread->stackLimit = SB_STACK_SIZE / 4;
            thread->slots = sbSlotTableGet(index);
            thread->queue = sbChainGet(index);
            memset(thread->slots, 0, SB_SLOT_TABLE_SIZE);
            sbChainReset(thread);
            thread->status = 0;
            return 1;
        }
        thread++;
    }
    return 0;
}

/* sbThreadTable records with a set suspended bit in status are skipped by sbRunAll. */
#define SB_THREAD_SUSPENDED 0x80000000

/* The thread record with this key and value, or 0. */
SbThread *sbFindThread(struct SbImageHeader *key, int value)
{
    int index;

    for (index = 0; index < 80; index++) {
        if (sbThreadTable[index].image == key && sbThreadTable[index].value == value) {
            return &sbThreadTable[index];
        }
    }
    return 0;
}

/* Frees the thread record with this key and value; 0 when there is none. */
int sbThreadRelease(struct SbImageHeader *key, int value)
{
    SbThread *thread = sbFindThread(key, value);

    if (thread == 0) {
        return 0;
    }
    thread->image = 0;
    return 1;
}

int sbCursorPush(SbThread *obj);
SbMessage *sbChainEntryGet(SbThread *table, int index);

/* Queues a copy of message on the thread with this key and value; sender, when
 * given, is named in the copy by its value. 0 when there is no such thread or
 * its queue is full. */
int sbThreadEnqueue(struct SbImageHeader *key, int value, SbMessage *message, SbThread *sender)
{
    SbThread *thread;
    SbMessage *entry;
    int index;

    thread = sbFindThread(key, value);
    if (thread == 0) {
        return 0;
    }
    index = sbCursorPush(thread);
    if (index == -1) {
        return 0;
    }
    entry = sbChainEntryGet(thread, index);
    *entry = *message;
    if (sender != 0) {
        entry->sender = sender->value;
    } else {
        entry->sender = -1;
    }
    return 1;
}

int sbCursorPop(SbThread *thread);
int sbChainPreviousState(SbThread *table);
SbMessage *sbChainEntryGet(SbThread *table, int index);

/* Dequeue side of sbThreadEnqueue: pops the next cursor index of the thread and takes its chain entry. */
SbMessage *sbThreadDequeue(struct SbImageHeader *key, int value)
{
    SbThread *thread;
    int index;

    thread = sbFindThread(key, value);
    if (thread == 0) {
        return 0;
    }
    if (sbChainPreviousState(thread) == 0) {
        return 0;
    }
    index = sbCursorPop(thread);
    return sbChainEntryGet(thread, index);
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbNameTableFind);

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbNameTableNameOf);

int sbNameTableFind(struct SbImageHeader *image, const char *name);
int sbThreadEnqueue(struct SbImageHeader *key, int value, SbMessage *message, SbThread *sender);

/* Enqueues a thread at the entry the image's name table gives for name; 0 when
 * the name is not there. */
int sbThreadEnqueueByName(struct SbImageHeader *image, const char *name, SbMessage *message)
{
    int entry = sbNameTableFind(image, name);

    if (entry == -1) {
        return 0;
    }
    return sbThreadEnqueue(image, entry, message, 0);
}

#define TU044_RECORD_COUNT 80

void sbThreadsRelease(struct SbImageHeader *key)
{
    int index;

    for (index = 0; index < TU044_RECORD_COUNT; index++) {
        if (sbThreadTable[index].image == key) {
            sbThreadRelease(key, sbThreadTable[index].value);
        }
    }
}

/* Sets the suspended bit of the thread with this key and value; 0 when there is none. */
int sbThreadSuspend(struct SbImageHeader *key, int value)
{
    SbThread *thread = sbFindThread(key, value);

    if (thread == 0) {
        return 0;
    }
    thread->status |= SB_THREAD_SUSPENDED;
    return 1;
}

void sbThreadsSuspend(struct SbImageHeader *key)
{
    int index;

    for (index = 0; index < TU044_RECORD_COUNT; index++) {
        if (sbThreadTable[index].image == key) {
            sbThreadSuspend(key, sbThreadTable[index].value);
        }
    }
}

/* Clears the suspended bit of the thread with this key and value; 0 when there is none. */
int sbThreadResume(struct SbImageHeader *key, int value)
{
    SbThread *thread = sbFindThread(key, value);

    if (thread == 0) {
        return 0;
    }
    thread->status &= ~SB_THREAD_SUSPENDED;
    return 1;
}

void sbThreadsResume(struct SbImageHeader *key)
{
    int index;

    for (index = 0; index < TU044_RECORD_COUNT; index++) {
        if (sbThreadTable[index].image == key) {
            sbThreadResume(key, sbThreadTable[index].value);
        }
    }
}

extern unsigned char sbChains[];

void *sbChainGet(int index) {
    return sbChains + index * 0x114;
}

void sbChainReset(SbThread *table) {
    SbQueue *chain;

    chain = table->queue;
    chain->last = 0xF;
    chain->first = 0;
    chain->tail = 0;
    chain->used = (chain->head = 0);
}

int sbCursorPush(SbThread *obj)
{
    SbQueue *cursor;
    int next;

    cursor = obj->queue;
    if (!(cursor->used < (cursor->last - cursor->first) + 1)) {
        return -1;
    }
    cursor->used = cursor->used + 1;
    next = cursor->tail + 1;
    cursor->tail = next;
    if (cursor->last < next) {
        cursor->tail = cursor->first;
    }
    return cursor->tail;
}

/* Takes the next index from the thread's cursor ring; -1 when it is empty. */
int sbCursorPop(SbThread *thread)
{
    SbQueue *cursor;
    int next;

    cursor = thread->queue;
    if (cursor->used <= 0) {
        return -1;
    }
    next = cursor->head + 1;
    cursor->used = cursor->used - 1;
    cursor->head = next;
    if (cursor->last < next) {
        cursor->head = cursor->first;
    }
    return cursor->head;
}

int sbChainPreviousState(SbThread *table) {
    return table->queue->used;
}

SbMessage *sbChainEntryGet(SbThread *table, int index) {
    return &table->queue->messages[index];
}

extern unsigned char sbStacks[];

void *sbStackGet(int index) {
    return sbStacks + (index << 9);
}

u16 sbCodeU16(SbThread *table, int index) {
    struct SbImageHeader *layout;

    layout = table->image;
    return ((u16 *) ((unsigned char *) layout + layout->code_offset))[index];
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbCodeS32);

int sbGlobalGet(SbThread *table, int index) {
    struct SbImageHeader *layout;

    layout = table->image;
    return ((int *) ((unsigned char *) layout + layout->global_offset))[index];
}

void sbGlobalSet(SbThread *table, int index, int value) {
    struct SbImageHeader *layout;

    layout = table->image;
    ((int *) ((unsigned char *) layout + layout->global_offset))[index] = value;
}

/* Local slot read: the locals sit above the operand stack, at table->slots + count words. */
int sbLocalGet(SbThread *table, int index)
{
    int *locals;

    locals = table->stack + table->frame;
    return locals[index];
}

/* Local slot write; returns the locals' byte offset (count * 4). */
void sbLocalSet(SbThread *table, int index, int value) {
    int *locals;

    locals = table->stack + table->frame;
    locals[index] = value;
}

/* Pushes value on the frame's operand stack; a full stack is fatal. */
void sbStackPush(SbThread *obj, int value)
{
    if (obj->sp >= obj->stackLimit) {
        sbFatal(obj);
    }
    obj->stack[obj->sp++] = value;
}

int sbStackPop(SbThread *obj)
{
    int top;

    if (obj->sp <= 0) {
        sbFatal(obj);
    }
    top = obj->sp - 1;
    obj->sp = top;
    return obj->stack[top];
}

/* Opcode with an operand list: one byte opcode word plus `count` words. */
#define SB_OP_SWITCH 0x25

extern signed char sbOpLengths[];

/* Length of an instruction: from sbOpLengths, or 4 * count + 5 for SB_OP_SWITCH. */
int sbOpLength(unsigned short opcode, int count)
{
    if (opcode == SB_OP_SWITCH) {
        return count * 4 + 5;
    }
    return sbOpLengths[opcode];
}

/* One step of the VM: fetch the opcode at pc (record->total) and run its handler. */
int sbStep(SbThread *record)
{
    unsigned int opcode;
    int original_total;
    int result;

    opcode = sbCodeU16(record, record->pc);
    if (opcode < 0x2E) {
        original_total = record->pc;
        result = sbOpHandlers[opcode](record, opcode);
        if (result == 1) {
            record->status = result;
            record->pc = original_total;
        } else {
            record->status = 0;
        }
        if (result == 2) {
            record->pc = record->value;
        }
    } else {
        result = 2;
        sbFatal(record);
    }
    return result;
}

/* Opcode 0x00: no effect; advances past the opcode. */
int sbOpNop(SbThread *obj, int opcode)
{
    obj->pc += sbOpLength(opcode & 0xFFFF, 0);
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu044", sbOpBinary);

/* Opcode 0x02: no effect; skips the opcode and its operand word. */
int func_00A9DDE0(SbThread *obj, int opcode)
{
    obj->pc += sbOpLength(opcode & 0xFFFF, 0);
    return 0;
}

int sbOpPushWord(SbThread *control, int amount)
{
    unsigned short amount16;
    int slot;

    amount16 = (unsigned short) amount;
    slot = sbCodeS32(control, control->pc + 1);
    sbStackPush(control, slot);
    control->pc += sbOpLength(amount16, 0);
    return 0;
}

/* Opcode 0x04: pushes the operand word after the opcode, as sbOpPushWord does. */
int func_00A9DE80(SbThread *control, int opcode)
{
    unsigned short opcode16;
    int operand;

    opcode16 = (unsigned short) opcode;
    operand = sbCodeS32(control, control->pc + 1);
    sbStackPush(control, operand);
    control->pc += sbOpLength(opcode16, 0);
    return 0;
}

/* Opcode 0x05: no effect; skips the opcode and its operand word. */
int func_00A9DEE0(SbThread *obj, int opcode)
{
    obj->pc += sbOpLength(opcode & 0xFFFF, 0);
    return 0;
}

/* Opcode 0x06: pushes the operand word after the opcode, as sbOpPushWord does. */
int func_00A9DF20(SbThread *control, int opcode)
{
    unsigned short opcode16;
    int operand;

    opcode16 = (unsigned short) opcode;
    operand = sbCodeS32(control, control->pc + 1);
    sbStackPush(control, operand);
    control->pc += sbOpLength(opcode16, 0);
    return 0;
}

/* Opcode 0x07: pushes the operand word after the opcode, reading the pc through the SbThread view (sum). */
int func_00A9DF80(SbThread *obj, int opcode)
{
    int opcode16;
    int operand;

    opcode16 = opcode & 0xFFFF;
    operand = sbCodeS32(obj, obj->pc + 1);
    sbStackPush(obj, operand);
    obj->pc += sbOpLength(opcode16, 0);
    return 0;
}

int sbGlobalGet(SbThread *table, int index);

int sbOpPushGlobal(SbThread *control, int amount)
{
    unsigned short amount16;
    int slot;
    int next_slot;

    amount16 = (unsigned short) amount;
    slot = sbCodeS32(control, control->pc + 1);
    next_slot = sbGlobalGet(control, slot);
    sbStackPush(control, next_slot);
    control->pc += sbOpLength(amount16, 0);
    return 0;
}

int sbLocalGet(SbThread *table, int index);

int sbOpPushLocal(SbThread *control, int amount)
{
    unsigned short amount16;
    int slot;
    int next_slot;

    amount16 = (unsigned short) amount;
    slot = sbCodeS32(control, control->pc + 1);
    next_slot = sbLocalGet(control, slot);
    sbStackPush(control, next_slot);
    control->pc += sbOpLength(amount16, 0);
    return 0;
}

void sbGlobalSet(SbThread *table, int index, int value);

int sbOpStoreGlobal(SbThread *control, int amount)
{
    unsigned short amount16;
    int first;
    int second;

    amount16 = (unsigned short) amount;
    first = sbStackPop(control);
    second = sbStackPop(control);
    sbGlobalSet(control, first, second);
    control->pc += sbOpLength(amount16, 0);
    return 0;
}

int sbOpStoreLocal(SbThread *control, int amount)
{
    unsigned short amount16;
    int first;
    int second;

    amount16 = (unsigned short) amount;
    first = sbStackPop(control);
    second = sbStackPop(control);
    sbLocalSet(control, first, second);
    control->pc += sbOpLength(amount16, 0);
    return 0;
}

/* Stores into a global like the plain store but leaves the value on the stack. */
int sbOpStoreGlobalKeep(SbThread *obj, int opcode)
{
    unsigned short opcode16;
    int first;
    int second;

    opcode16 = (unsigned short) opcode;
    first = sbStackPop(obj);
    second = sbStackPop(obj);
    sbGlobalSet(obj, first, second);
    obj->sp++;
    obj->pc += sbOpLength(opcode16, 0);
    return 0;
}

/* Stores into a local like the plain store but leaves the value on the stack. */
int sbOpStoreLocalKeep(SbThread *obj, int opcode)
{
    unsigned short opcode16;
    int first;
    int second;

    opcode16 = (unsigned short) opcode;
    first = sbStackPop(obj);
    second = sbStackPop(obj);
    sbLocalSet(obj, first, second);
    obj->sp++;
    obj->pc += sbOpLength(opcode16, 0);
    return 0;
}

int sbOpJump(SbThread *obj)
{
    obj->pc = sbCodeS32(obj, obj->pc + 1);
    return 0;
}

int sbOpJumpIfZero(SbThread *record, unsigned short id)
{
    int next;

    next = sbCodeS32(record, record->pc + 1);
    if (sbStackPop(record) == 0) {
        record->pc = next;
    } else {
        record->pc += sbOpLength(id, 0);
    }
    return 0;
}

int sbOpJumpIfNotZero(SbThread *record, unsigned short id)
{
    int next;

    next = sbCodeS32(record, record->pc + 1);
    if (sbStackPop(record) != 0) {
        record->pc = next;
    } else {
        record->pc += sbOpLength(id, 0);
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
    sbStackPush(record, record->frame);
    sbStackPush(record, 0);
    sbLocalSet(record, -1, record->sp);
    record->pc += sbOpLength(opcode, 0);
    return 0;
}

/* Opcode 0x12: call. Stores the return position in the frame's return slot and jumps to the operand. */
int sbOpCall(SbThread *obj)
{
    obj->frame = sbLocalGet(obj, -1);
    sbLocalSet(obj, -3, obj->pc + 2);
    obj->pc = sbCodeS32(obj, obj->pc + 1);
    return 0;
}

/* Opcode 0x13: return. Restores the stack depth, the saved limit and the pc from the frame header. */
int sbOpReturn(SbThread *record, unsigned short opcode)
{
    if (record->sp == 0) {
        sbFatal(record);
    }
    record->sp = record->frame;
    sbStackPop(record);
    record->frame = sbStackPop(record);
    record->pc = sbStackPop(record);
    record->pc += sbOpLength(opcode, 0);
    return 0;
}

/* Opcode 0x14: return with a value. The top of the stack survives the frame restore. */
int sbOpReturnValue(SbThread *record, unsigned short opcode)
{
    int returnValue;

    returnValue = sbStackPop(record);
    if (record->sp == 0) {
        sbFatal(record);
    }
    record->sp = record->frame;
    sbStackPop(record);
    record->frame = sbStackPop(record);
    record->pc = sbStackPop(record);
    sbStackPush(record, returnValue);
    record->pc += sbOpLength(opcode, 0);
    return 0;
}

/* Native call opcode: the word after the opcode is the native id (sbFindNative).
 * Status 1 from the method repeats the opcode on the next run; otherwise the
 * arguments (a count word over two words per argument) are dropped and the
 * result is pushed. */
int sbOpCallNative(SbThread *record, int opcode)
{
    unsigned short opcode16;
    int id;
    int status;
    int result;
    int words;

    opcode16 = opcode;
    id = sbCodeS32(record, record->pc + 1);
    status = 0;
    result = sbNativeOwners[id >> 16].methods[id & 0xFFFF].func(record, &status);
    if (status == 1) {
        return 1;
    }
    words = record->stack[record->sp - 1] * 2 + 1;
    record->sp -= words;
    sbStackPush(record, result);
    record->pc += sbOpLength(opcode16, 0);
    return 0;
}

int sbOpSpawn(SbThread *control, int amount)
{
    unsigned short amount16;
    int slot;

    amount16 = (unsigned short) amount;
    slot = sbStackPop(control);
    control->pc += sbOpLength(amount16, 0);
    return (sbThreadStart(control->image, slot) != 0) ? 0 : 2;
}

/* Kills the thread whose value is popped; 2 (stop) when the thread killed itself. */
int sbOpKillThread(SbThread *thread, int opcode)
{
    unsigned short opcode16;
    int value;
    int result;

    opcode16 = (unsigned short) opcode;
    value = sbStackPop(thread);
    thread->pc += sbOpLength(opcode16, 0);
    result = sbThreadRelease(thread->image, value);
    if (result != 0) {
        result = 2;
        if (thread->value != value) {
            result = 0;
        }
        return result;
    }
    return 2;
}

int sbOpDup(SbThread *obj, int value)
{
    int operand;

    operand = value & 0xFFFF;
    sbStackPush(obj, obj->stack[obj->sp - 1]);
    obj->pc += sbOpLength(operand, 0);
    return 0;
}

int sbOpDrop(SbThread *obj, int value)
{
    obj->sp -= 1;
    obj->pc += sbOpLength(value & 0xFFFF, 0);
    return 0;
}

int sbOpLoadGlobal(SbThread *control, int amount)
{
    unsigned short amount16;
    int slot;
    int next_slot;

    amount16 = (unsigned short) amount;
    slot = sbStackPop(control);
    next_slot = sbGlobalGet(control, slot);
    sbStackPush(control, next_slot);
    control->pc += sbOpLength(amount16, 0);
    return 0;
}

int sbOpLoadLocal(SbThread *control, int amount)
{
    unsigned short amount16;
    int slot;
    int next_slot;

    amount16 = (unsigned short) amount;
    slot = sbStackPop(control);
    next_slot = sbLocalGet(control, slot);
    sbStackPush(control, next_slot);
    control->pc += sbOpLength(amount16, 0);
    return 0;
}

/* Opcode 0x1C: returns 2, which sbStep takes as a rewind of the pc to the thread entry. */
int sbOpEnd(SbThread *obj, int opcode)
{
    obj->pc += sbOpLength(opcode & 0xFFFF, 0);
    return 2;
}

/* Opcode 0x1D: returns 3; sbStep keeps the advanced pc and the run stops for this frame. */
int sbOpYield(SbThread *obj, int opcode)
{
    obj->pc += sbOpLength(opcode & 0xFFFF, 0);
    return 3;
}

int sbOpWaitFrames(SbThread *control, int amount)
{
    unsigned short amount16;
    int result;

    amount16 = (unsigned short) amount;
    if (control->status == 0) {
        control->wait[0] = sbStackPop(control);
        result = 1;
    } else {
        control->wait[0] -= 1;
        if (control->wait[0] <= 0) {
            result = 0;
            control->pc += sbOpLength(amount16, 0);
        } else {
            result = 1;
        }
    }
    return result;
}

/* The keyed entry table of the running image (tu046); an entry here is a message. */
SbMessage *vmEntryGet(SbThread *record, int index);
SbMessage *sbThreadDequeue(struct SbImageHeader *key, int value);

/* Send opcode: pops a thread value and an entry index and queues that entry as a
 * message from this thread to the thread with the same key and the popped value. */
int sbOpSend(SbThread *record, int opcode)
{
    unsigned short opcode16;
    int value;
    SbMessage message;

    opcode16 = opcode;
    value = sbStackPop(record);
    message = *vmEntryGet(record, sbStackPop(record));
    sbThreadEnqueue(record->image, value, &message, record);
    record->pc += sbOpLength(opcode16, 0);
    return 0;
}

/* Receive opcode: pops a thread value and an entry index, takes the next message
 * of the thread with the same key and that value into the entry and pushes 1, or
 * pushes 0 when there is none. */
int sbOpReceive(SbThread *record, int opcode)
{
    unsigned short opcode16;
    int value;
    SbMessage *out;
    SbMessage *message;
    int found;

    opcode16 = opcode;
    value = sbStackPop(record);
    out = vmEntryGet(record, sbStackPop(record));
    message = sbThreadDequeue(record->image, value);
    if (message != 0) {
        *out = *message;
        found = 1;
    } else {
        found = 0;
    }
    sbStackPush(record, found);
    record->pc += sbOpLength(opcode16, 0);
    return 0;
}

int sbOpIncGlobal(SbThread *control, int amount)
{
    unsigned short amount16;
    int first;
    int second;

    amount16 = (unsigned short) amount;
    first = sbStackPop(control);
    second = sbGlobalGet(control, first);
    sbGlobalSet(control, first, second + 1);
    control->pc += sbOpLength(amount16, 0);
    return 0;
}

int sbOpIncLocal(SbThread *control, int amount)
{
    unsigned short amount16;
    int first;
    int second;

    amount16 = (unsigned short) amount;
    first = sbStackPop(control);
    second = sbLocalGet(control, first);
    sbLocalSet(control, first, second + 1);
    control->pc += sbOpLength(amount16, 0);
    return 0;
}

int sbOpDecGlobal(SbThread *control, int amount)
{
    unsigned short amount16;
    int first;
    int second;

    amount16 = (unsigned short) amount;
    first = sbStackPop(control);
    second = sbGlobalGet(control, first);
    sbGlobalSet(control, first, second - 1);
    control->pc += sbOpLength(amount16, 0);
    return 0;
}

int sbOpDecLocal(SbThread *control, int amount)
{
    unsigned short amount16;
    int first;
    int second;

    amount16 = (unsigned short) amount;
    first = sbStackPop(control);
    second = sbLocalGet(control, first);
    sbLocalSet(control, first, second - 1);
    control->pc += sbOpLength(amount16, 0);
    return 0;
}

/* Switch opcode: pops a value and looks it up by binary search in the sorted
 * (value, target) pairs after the opcode (count at +1, default target at +3,
 * pair i at +5 + 4 * i); the pc moves to the matching or the default target. */
int sbOpSwitch(SbThread *record)
{
    int low;
    int high;
    int mid;
    int key;
    int value;

    low = 0;
    key = sbStackPop(record);
    high = sbCodeS32(record, record->pc + 1);
    while (low < high) {
        mid = (low + high) / 2;
        value = sbCodeS32(record, record->pc + mid * 4 + 5);
        if (value < key) {
            low = mid + 1;
        } else if (key < value) {
            high = mid;
        } else {
            record->pc = sbCodeS32(record, record->pc + mid * 4 + 7);
            return 0;
        }
    }
    record->pc = sbCodeS32(record, record->pc + 3);
    return 0;
}
