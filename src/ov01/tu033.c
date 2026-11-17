/*
 * OV01 original TU 33: 0x00a92150..0x00a97110 (77 functions)
 *
 * Naming pass (proposals/ov01-tu033.json, applied later; globals stay func_/D_ here):
 *   cmdSystemInit cmdSystemInit      cmdListPtrGet cmdListPtrGet      cmdListSet cmdListSet
 *   cmdListRunHead cmdListRunHead     cmdExecute cmdExecute         eventQueueNext eventQueueNext
 *   eventQueueReserve eventQueueReserve  eventQueueFindKind4 eventQueueFindKind4  eventQueueFindKind5 eventQueueFindKind5
 *   eventQueueAddKind1 eventQueueAddKind1 eventQueueAddKind4 eventQueueAddKind4  eventQueueAddKind5 eventQueueAddKind5
 *   eventQueueAdd eventQueueAdd      eventQueueRun eventQueueRun      itemRandomPick itemRandomPick
 *   unitSlotResume unitSlotResume     unitSlotPause unitSlotPause
 * Data: cmdList cmdList, cmdFifo cmdFifo, eventQueueTable eventQueueTable,
 *       eventQueueCount eventQueueCount, eventQueueCursor eventQueueCursor.
 */
#include "common.h"
#include "ov01/tu005.h"
#include "ov01/tu008.h"
#include "main/tu051.h"
#include "ov01/tu012.h"
#include "ov01/tu023.h"
#include "ov01/tu033.h"
#include "ov01/tu042.h"
#include "ov01/tu036.h"
#include "ov01/tu038.h"
#include "ov01/tu054.h"
#include "ov01/tu066.h"
#include "ov01/tu070.h"

extern char cmdList[];   /* cmdList: 0x20-byte entries, unit then command body */
extern char cmdFifo[];   /* cmdFifo: fifo control block */

/* Command list entries (0x20 bytes each); the fifo ring runs over them. */
#define CMD_LIST_COUNT 0x20
#define CMD_LIST_ENTRY_SIZE 0x20

/* Payloads the appenders copy whole into a record (four, six and eleven words). */

/* The four-word payload of eventQueueAdd and eventQueueAddKind4. */
typedef struct EventQueueArgs {
    unsigned char unmodeled_00[0x4];
    int tecIndex;                       /* +0x04: the tec whose EP cost cmdStockRun charges */
    unsigned char unmodeled_08[0x4];
    Ov01Object *target;                 /* +0x0C: the partner / stocked unit of events 0x10 and 0x12 */
} EventQueueArgs;

typedef struct EventQueueArgs11 {
    int words[11];
} EventQueueArgs11;

/* One 0x30-byte record of eventQueueTable (eventQueueTable); kind is its first word,
 * the payload layout depends on the appender. */
typedef struct EventQueueEntry {
    int kind;
    union {
        EventQueueArgs args;          /* eventQueueAdd, eventQueueAddKind4 */
        struct {
            void *object;
            CmdListBody args;
        } kind1;                      /* eventQueueAddKind1 */
        EventQueueArgs11 kind5;       /* eventQueueAddKind5 */
    } data;
} EventQueueEntry;

/* Record kinds by their handler in eventQueueHandlers (the kinds the code here queues). */
enum {
    EVENT_KIND_WAIT_UNIT_QUEUE = 2,     /* eventWaitUnitQueue */
    EVENT_KIND_LOADS_IDLE = 7,          /* ov01LoadsIdle */
    EVENT_KIND_STREAM_OPEN = 14,        /* ov01StreamOpen */
    EVENT_KIND_STREAM_BIND = 15,        /* ov01StreamBind */
    EVENT_KIND_PAIR_BEGIN = 16,         /* func_00A96AA8: unbinds the pair and hides its monster slots */
    EVENT_KIND_PAIR_END = 17,           /* func_00A96E38: shows and releases the monster slots again */
    EVENT_KIND_STOCK_BEGIN = 18,        /* func_00A97110 */
    EVENT_KIND_STOCK_BOOST_CLEAR = 19   /* eventActStockBoostClear */
};

/* Records of eventQueueTable (eventQueueReserve stops at this count). */
#define EVENT_QUEUE_COUNT 0x40

extern EventQueueEntry eventQueueTable[EVENT_QUEUE_COUNT];
extern int eventQueueCount;
extern int eventQueueCursor;
/* Cleared with the event queue: eventRetryCount counts the runs of a record whose
 * handler is not done yet, eventStepStarted is a handler's own flag (unitBindRequest
 * sets it once the bind load runs); both are cleared when a record is done. */
extern int eventRetryCount;
extern int eventStepStarted;

/* cmdSystemInit: clears the command list and fifo, the event table and its counters. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu033", cmdSystemInit);

/* cmdListPtrGet: the command fifo and the command list, through two out pointers. */
void cmdListPtrGet(void **fifo_out, void **list_out) {
    *fifo_out = cmdFifo;
    *list_out = cmdList;
}

/* Ov01Object.flags bit of a held (paused) unit. */

/* One 0x20-byte cmdList entry: the unit, then the command body at +0x08. The
 * unit is kept as a word, as XS1's CmdListEntry (battle_init.c) keeps it. */
typedef struct CmdListEntry {
    int unit;                         /* +0x00 */
    unsigned char unmodeled_04[0x04];
    CmdListBody body;                 /* +0x08 */
} CmdListEntry;

/* cmdListSet: queues one unit's command record at the fifo's write position. */
void cmdListSet(void *unit, CmdListBody *command)
{
    void *fifo;
    void *list;
    CmdListEntry *entry;

    cmdListPtrGet(&fifo, &list);
    /* XS1 cmdListSet's form: the byte offset plus the list address as integers.
     * With pointer arithmetic (list + index) the unit store is scheduled after
     * the body copy. */
    entry = (CmdListEntry *)((char *)(ov01FifoGetTail(fifo) * sizeof(CmdListEntry)) + (int)list);
    entry->unit = (int)unit;
    entry->body = *command;
    ov01FifoPush(fifo);
}

/* cmdListRunHead: runs the head command of the list unless its unit is held; pops it on success. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu033", cmdListRunHead);

/* cmdExecute: runs one command body, a switch on its kind, queueing events on the way. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu033", cmdExecute);

void eventQueueAdd(int kind, EventQueueArgs *args);
void eventQueueAddKind1(void *object, CmdListBody *args);
void func_00A92C18(Ov01Object *unit, CmdListBody *command);
Ov01Object *unitPairPartnerFind(Ov01Object *unit, int value);

/* Unit command indices (CmdListBody.command) the two command handlers below queue. */
#define CMD_EVENT_MOTION_3 3
#define CMD_EVENT_MOTION_9 9

/* A pair command: queues stream open, loads idle, stream bind and pair begin (args target
 * set to the partner unitPairPartnerFind finds for the command's pair record), the command
 * for the unit and the partner, motion 9 for the partner, the follow-up and the pair end. */
void cmdPairRun(Ov01Object *unit, CmdListBody *command, EventQueueArgs *args)
{
    Ov01Object *partner;
    CmdListBody motion;

    partner = unitPairPartnerFind(unit, command->pairIndex);
    eventQueueAdd(EVENT_KIND_STREAM_OPEN, args);
    eventQueueAdd(EVENT_KIND_LOADS_IDLE, args);
    eventQueueAdd(EVENT_KIND_STREAM_BIND, args);
    args->target = partner;
    eventQueueAdd(EVENT_KIND_PAIR_BEGIN, args);
    eventQueueAddKind1(unit, command);
    eventQueueAddKind1(partner, command);
    motion.command = CMD_EVENT_MOTION_9;
    eventQueueAddKind1(partner, &motion);
    func_00A92C18(unit, command);
    eventQueueAdd(EVENT_KIND_PAIR_END, args);
}

int calcEpCost(Ov01Object *unit, int value);
int calcEpAdjust(Ov01Object *unit, int delta);

/* A stock command: the stocked unit (args target) gets the stock begin and motion 3,
 * pays the EP calcEpCost gives for the args tec, then the command runs for the unit
 * (wait for its queue, then the follow-up) and the stocked unit ends with motion 9 and
 * the boost clear. */
void cmdStockRun(Ov01Object *unit, CmdListBody *command, EventQueueArgs *args)
{
    Ov01Object *stocked;
    CmdListBody motion;

    stocked = actStockGet()->boost[1];
    args->target = stocked;
    eventQueueAdd(EVENT_KIND_STOCK_BEGIN, args);
    motion.command = CMD_EVENT_MOTION_3;
    eventQueueAddKind1(stocked, &motion);
    calcEpAdjust(stocked, -calcEpCost(stocked, args->tecIndex));
    eventQueueAddKind1(unit, command);
    eventQueueAdd(EVENT_KIND_WAIT_UNIT_QUEUE, args);
    func_00A92C18(unit, command);
    motion.command = CMD_EVENT_MOTION_9;
    eventQueueAddKind1(stocked, &motion);
    eventQueueAdd(EVENT_KIND_STOCK_BOOST_CLEAR, args);
}

struct Tu033CallSource {
    unsigned char unmodeled_00[8];
    int tec_id;
    int from_0c;
    int flags;
    int from_14;
};

/* The task record tgtFindFirst reads (tu031 struct TaskRecord): target at 0, tec_id at 8, flags at 0xC. */
struct CmdTaskRecord {
    int target;
    int from_14;
    int tec_id;
    int flags;
};

int tgtFindFirst();

/*
 * cmdFindFirstTarget (proposed cmdFindFirstTarget): builds the task record of a command
 * body and returns the first target tgtFindFirst finds for it. Only cmdExecute calls it.
 */
int cmdFindFirstTarget(int target, struct Tu033CallSource *source)
{
    struct CmdTaskRecord record;
    int flags;
    int from_14;

    record.target = target;
    record.tec_id = source->tec_id;
    flags = source->flags;
    from_14 = source->from_14;
    record.flags = flags;
    record.from_14 = from_14;
    return tgtFindFirst(&record, source->from_0c);
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A92C18);


int func_00A92E30(struct AttackCommand *owner, int value, TecRecord *tec);

int func_00A92D98(struct AttackCommand *owner, int value) {
    TecRecord *entry;
    int slot;
    int result;

    entry = tecRecordGet(owner->actionIdx);
    slot = entry->kindBits.slot;
    result = slot < 8;
    if (result != 0 && slot >= 0) {
        result = func_00A92E30(owner, value, entry);
    }
    return result;
}

void func_00A92DF0(struct AttackCommand *owner, int value) {
    func_00A92E30(owner, value, tecRecordGet(owner->actionIdx));
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A92E30);

/* The partner of unit in pair record value: the first unit of side 0 that is
 * not held and has the other key of the pair; 0 when there is none. */
Ov01Object *unitPairPartnerFind(Ov01Object *unit, int value)
{
    PairLink *pair;
    Ov01Object **units;
    Ov01Object *other;
    int key;
    int count;
    int i;

    pair = pairLinkGet(value);
    if (pair->slotFirst == unit->stats->key) {
        key = pair->slotSecond;
    } else {
        key = pair->slotFirst;
    }
    count = unitTblGet(0, &units);
    for (i = 0; i < count; i++) {
        other = units[i];
        if (other != 0 && !(other->flags & UNIT_FLAG_DOWN)) {
            if (other->stats->key == key) {
                return other;
            }
        }
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A93128);

/* A unit command request (func_00A96510, func_00A94260): the unit, the command and its words. */
typedef struct UnitCmdRequest {
    Ov01Object *unit;
    int command;
    int words[3];
} UnitCmdRequest;

/* Queues the unit command of request (the mode, effect and activity commands; others are ignored). The mode
 * dispatch takes words 1 and 2 of the request and the face target. When the running command and the new one
 * are both mode or effect commands, the running one is dropped first. */
void unitCmdRequestQueue(Ov01Object **faceTarget, UnitCmdRequest *request)
{
    Ov01Command command;
    Ov01Command *current;

    switch (request->command) {
    case UNIT_CMD_MODE_DISPATCH:
        command.command = UNIT_CMD_MODE_DISPATCH;
        command.code = request->words[1];
        command.arg.wait = request->words[2];
        if (faceTarget != 0) {
            command.faceTarget = *faceTarget;
        } else {
            command.faceTarget = 0;
        }
        break;
    case UNIT_CMD_HOVER_EFFECT:
        command.command = UNIT_CMD_HOVER_EFFECT;
        break;
    case UNIT_CMD_JUMP_EFFECT:
        command.command = UNIT_CMD_JUMP_EFFECT;
        break;
    case UNIT_CMD_CLEAR_ACTIVE:
        command.command = UNIT_CMD_CLEAR_ACTIVE;
        break;
    case UNIT_CMD_PHASE_2B_WAIT:
        command.command = UNIT_CMD_PHASE_2B_WAIT;
        break;
    case UNIT_CMD_DEACTIVATE:
        command.command = UNIT_CMD_DEACTIVATE;
        break;
    default:
        return;
    }
    current = dataLookupCurrent(&request->unit->commandQueue);
    if (current != 0
        && (current->command == UNIT_CMD_MODE_DISPATCH || current->command == UNIT_CMD_MODE_START
            || current->command == UNIT_CMD_HOVER_EFFECT || current->command == UNIT_CMD_MOVE_RATIO)) {
        if (request->command == UNIT_CMD_MODE_DISPATCH || request->command == UNIT_CMD_MODE_START
            || request->command == UNIT_CMD_HOVER_EFFECT) {
            dataLookupNext(&request->unit->commandQueue);
        }
    }
    unitQueueAppend(request->unit, &command);
}

int tecEffectGroupGet(Ov01Object *unit, TecRecord *tec);

int func_00A93288(struct AttackCommand *owner) {
    TecRecord *entry;
    int status;

    entry = tecRecordGet(owner->actionIdx);
    status = tecEffectGroupGet(owner->attacker, entry);
    return unitEffectKeyFree(owner->attacker, status, entry->extent) != 0;
}

struct Tu033Request {
    Ov01Object *object;
    unsigned char unmodeled_04[4];
    int second;
    unsigned char unmodeled_0c[0xC];
    int flags;
};


/* func_00A932E0: forwards a request's object, second word and flags to func_00AA4DB8 (fourth argument 0). */
void func_00A932E0(int unused_arg0, struct Tu033Request *request) {
    func_00AA4DB8(request->object, request->second, request->flags, 0);
}

/* eventQueueNext: the record at the read cursor, or 0 when the queue is drained. */
void *eventQueueNext(void) {
    if (eventQueueCursor < eventQueueCount) {
        return eventQueueTable + eventQueueCursor;
    }
    return 0;
}

/* eventQueueReserve: the next free record (the appenders fill it), or 0 when the table is full. */
void *eventQueueReserve(void) {
    if (eventQueueCount < EVENT_QUEUE_COUNT) {
        return eventQueueTable + eventQueueCount;
    }
    return 0;
}

#include "shared.h"

/* Slot from tecRecordGet; its state byte at +0x1C is compared with 1 in unitAttachRequest. */
typedef struct Ov01Tu033Slot {
    unsigned char unmodeled_00[0x1C];
    signed char state;
} Ov01Tu033Slot;



/* Request record: the four-word argument of unitAttachRequest and unitSpawnFromRequest. */
typedef struct Ov01Tu033Request {
    unsigned char unmodeled_00[4];
    int id;
    int index;
    int value;
} Ov01Tu033Request;


extern Ov01Object *ov01LastUnit;
extern int ov01ModelEntry;
int dataSlotRelease(s16);
int dataSlotFlagClear(s16);
int unitEffectSlotAcquire(int);
int unitRemove(Ov01Object *);
int unitAttachChild(int, int, int);
int sefApplyRecordMode(struct XenoActor *, int, int);
int sefSetGateForId(int, int);
int unitTblGet(int, Ov01Object ***);
Ov01Object *unitPlCreate(int);
Ov01Object *unitEnCreate(int);

/*
 * eventQueueFindKind4: the ordinal'th kind-4 record counting back from the last
 * one, or 0 when the table holds fewer.
 */
EventQueueEntry *eventQueueFindKind4(int ordinal)
{
    int remaining;
    int matches;
    EventQueueEntry *entry;

    remaining = eventQueueCursor - 1;
    matches = 0;
    if (remaining >= 0) {
        entry = eventQueueTable + remaining;
        for (;;) {
            if (entry->kind == 4) {
                if (matches == ordinal) {
                    return entry;
                }
                matches += 1;
            }
            remaining -= 1;
            entry -= 1;
            if (remaining < 0) {
                break;
            }
        }
    }
    return 0;
}

/*
 * eventQueueFindKind5: the ordinal'th kind-5 record counting back from the last
 * one, or 0 when the table holds fewer.
 */
EventQueueEntry *eventQueueFindKind5(int ordinal)
{
    int remaining;
    int matches;
    EventQueueEntry *entry;

    remaining = eventQueueCursor - 1;
    matches = 0;
    if (remaining >= 0) {
        entry = eventQueueTable + remaining;
        for (;;) {
            if (entry->kind == 5) {
                if (matches == ordinal) {
                    return entry;
                }
                matches += 1;
            }
            remaining -= 1;
            entry -= 1;
            if (remaining < 0) {
                break;
            }
        }
    }
    return 0;
}

/* eventQueueAddKind1: appends a kind-1 record holding a pointer and an 0x18-byte payload. */
void eventQueueAddKind1(void *object, CmdListBody *args)
{
    EventQueueEntry *entry = eventQueueReserve();

    entry->kind = 1;
    entry->data.kind1.object = object;
    entry->data.kind1.args = *args;
    eventQueueCount++;
}

/* eventQueueAddKind4: appends a kind-4 record with an 0x10-byte payload. */
void eventQueueAddKind4(EventQueueArgs *args)
{
    EventQueueEntry *entry = eventQueueReserve();

    entry->kind = 4;
    entry->data.args = *args;
    eventQueueCount++;
}

/* eventQueueAddKind5: appends a kind-5 record with a 0x2C-byte payload. */
void eventQueueAddKind5(EventQueueArgs11 *args)
{
    EventQueueEntry *entry = eventQueueReserve();

    entry->kind = 5;
    entry->data.kind5 = *args;
    eventQueueCount++;
}

/* eventQueueAdd: appends a record of the given kind, copying 0x10 bytes when a source is given. */
void eventQueueAdd(int kind, EventQueueArgs *args)
{
    EventQueueEntry *entry = eventQueueReserve();

    entry->kind = kind;
    if (args != 0) {
        entry->data.args = *args;
    }
    eventQueueCount++;
}

/* Event kinds with a handler in eventQueueHandlers. */
#define EVENT_KIND_COUNT 0x15

/* Handlers by record kind: nonzero when the record is done. */
extern int (*eventQueueHandlers[EVENT_KIND_COUNT])(EventQueueEntry *entry);

/* eventQueueRun: calls the handler of each queued record's kind. A record whose
 * handler is not done yet stops the drain (eventRetryCount counts the retries); a done
 * record clears both handler counters and is consumed. 0 also for an unknown kind. */
int eventQueueRun(void)
{
    EventQueueEntry *entry;

    for (entry = eventQueueNext(); entry != 0; entry = eventQueueNext()) {
        if (!(entry->kind < EVENT_KIND_COUNT)) {
            return 0;
        }
        if (eventQueueHandlers[entry->kind](entry) == 0) {
            eventRetryCount++;
            return 0;
        }
        eventRetryCount = 0;
        eventStepStarted = 0;
        eventQueueCursor++;
    }
    return 1;
}

int func_00A93708(void) {
    return 1;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A93710);


/* Event record of kind 2: the object whose command queue the event waits on (+4). */
struct EventWaitRecord {
    unsigned char unmodeled_00[4];
    Ov01Object *object;
};

/*
 * eventWaitUnitQueue (proposed eventWaitUnitQueue): the kind-2 handler of the event table
 * eventQueueHandlers. The handler's result is unitQueueIsEmpty's result, which the drain
 * reads (nonzero: the queue is empty and the event is done). The return type stays
 * void: the value the drain reads is the one left by the call.
 */
void eventWaitUnitQueue(struct EventWaitRecord *record) {
    unitQueueIsEmpty(record->object);
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A938A0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", unitsMarkUninvolved);

void func_00A93CF0(void) {
    Ov01Object **actors;
    Ov01Object **cursor;
    Ov01Object *actor;
    struct XenoActor *unit;
    int count;
    int remaining;

    count = unitTblGet(2, &actors);
    if (count > 0) {
        cursor = actors;
        remaining = count;
        do {
            actor = *cursor;
            cursor += 1;
            if (actor != 0) {
                unit = actor->unit;
                unit->drawFlags &= 0xFBFFFFFF;
            }
            remaining -= 1;
        } while (remaining != 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A93D50);

int unitHasEffectCode(void *, int);


/*
 * unitTryActStock (proposed unitTryActStock): rolls the act-stock chance of the unit
 * of an entry, with two effect-code rolls for the low keys, and grants act stock
 * when a roll hits and calcBoostOk accepts the unit.
 */
int unitTryActStock(struct Tu033CallSource *source, struct Tu033Request *request) {
    Ov01Object *object;
    int chanceHit;
    int flags;
    int requestFlags;

    object = request->object;
    if (tecRecordGet(source->tec_id)->flags & 8) {
        return 0;
    }
    if (object->stats->key < 0x10) {
        chanceHit = actLevelChanceRoll(object, source->tec_id) != 0;
        if (unitHasEffectCode(object, 0x2B) != 0) {
            flags = request->flags;
            if ((flags >> 8) & 1) {
                if (ov01RollPercent(0x19) != 0) {
                    chanceHit = 1;
                }
            }
        }
        if (unitHasEffectCode(object, 0x33) != 0 && (object->flags & 0x4000)) {
            requestFlags = request->flags;
            if (((requestFlags >> 8) & 2) != 0 || !(requestFlags & 0xFF)) {
                if (ov01RollPercent(0x32) != 0) {
                    chanceHit = 1;
                }
            }
        }
    } else {
        chanceHit = actLevelChanceRoll(object, source->tec_id) != 0;
    }
    if (chanceHit == 1 && calcBoostOk(object, 0) != 0) {
        calcActStockSet(object, 0);
        unitEffectStart(object, 0x140, 7);
        seVoiceStartTagged(2, 1);
    }
    return 1;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A94260);

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A94500);

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A946C0);

/* One weighted group of itemPickGroups: a roll below threshold picks one of its count items. */
typedef struct ItemPickGroup {
    int threshold;
    int count;
    int *items;
} ItemPickGroup;

#define ITEM_PICK_GROUPS 4

extern ItemPickGroup itemPickGroups[ITEM_PICK_GROUPS];
/* The 0x80-byte message line itemRandomPick builds, and the text "!" appended after the item name. */
extern char itemPickText[];
extern char itemPickTextEnd[];

void dataBoxIncCategory1(int item);
char *func_00A9B8D8(int item, int mode);
char *strcpy(char *destination, const char *source);
char *strcat(char *destination, const char *source);
void msgPutObject(int index, int x, char *text, int y, int width, int size);

/* itemRandomPick: rnd() picks one item from four weighted groups of itemPickGroups;
 * the item is added to the data box and its name is shown. */
void itemRandomPick(void)
{
    int roll;
    int item;
    int i;

    roll = rnd(99);
    item = 0;
    for (i = 0; i < ITEM_PICK_GROUPS; i++) {
        if (roll < itemPickGroups[i].threshold) {
            item = itemPickGroups[i].items[rnd(itemPickGroups[i].count - 1)];
            break;
        }
    }
    dataBoxIncCategory1(item);
    strcpy(itemPickText, tableStringGet(1, 1));
    strcat(itemPickText, func_00A9B8D8(item, 0));
    strcat(itemPickText, itemPickTextEnd);
    msgPutObject(0x14, 0x120, itemPickText, 0x2D, 0x1AC, 0x28);
}

/* StatBlock.flags bits unitStatusTextShow turns into the text mode (1 or 2). */
#define UNIT_TEXT_MODE_1 0x200
#define UNIT_TEXT_MODE_2 0x400
/* StatBlock.slotValue entries unitStatusTextShow marks (values from 0x15 on). */
#define UNIT_TEXT_SLOT_COUNT 8
#define UNIT_TEXT_SLOT_MARKED 0x15

/* The unit status block unitStatusTextShow hands to textBlockStart: HP and EP with their caps
 * and the unit's table key record. */
typedef struct UnitTextBlock {
    int hp;                              /* +0x00 */
    int maxHp;                           /* +0x04 */
    int ep;                              /* +0x08 */
    int maxEp;                           /* +0x0C */
    int points;                          /* +0x10 */
    int stepRate;                        /* +0x14 */
    int flat;                            /* +0x18 */
    int firstId;                         /* +0x1C */
    int secondId;                        /* +0x20 */
    unsigned char mode;                  /* +0x24 */
    unsigned char marked;                /* +0x25: bit i set when slotValue[i] is marked */
    unsigned char firstCategory;         /* +0x26 */
    unsigned char secondCategory;        /* +0x27 */
} UnitTextBlock;

struct TableKeyRecord *tableKeyRecordGet(int key);
void textBlockStart(int index, int x, UnitTextBlock *block);

/* Shows the status text block of unit: HP and EP with their caps, its table key
 * record, the text mode from the stat flags and the marked status slots as bits. */
void unitStatusTextShow(Ov01Object *unit)
{
    UnitTextBlock block;
    StatBlock *stats;
    struct TableKeyRecord *record;
    int mode;
    int bits;
    int i;

    stats = unit->stats;
    record = tableKeyRecordGet(stats->key);
    block.hp = stats->hp;
    block.maxHp = stats->maxHp;
    block.ep = stats->ep;
    block.maxEp = stats->maxEp;
    block.points = record->points;
    block.stepRate = record->stepRate;
    block.flat = record->flat;
    block.firstCategory = record->firstCategory;
    block.firstId = record->firstId;
    block.secondCategory = record->secondCategory;
    block.secondId = record->secondId;
    mode = 1;
    if (!(stats->flags & UNIT_TEXT_MODE_1)) {
        mode = 0;
        if (stats->flags & UNIT_TEXT_MODE_2) {
            mode = 2;
        }
    }
    block.mode = mode;
    bits = 0;
    for (i = 0; i < UNIT_TEXT_SLOT_COUNT; i++) {
        if (unit->stats->slotValue[i] >= UNIT_TEXT_SLOT_MARKED) {
            bits |= 1 << i;
        }
    }
    block.marked = bits;
    textBlockStart(0x14, 0x64, &block);
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A94D28);

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A94EB0);

int func_00A94260();

void func_00A95018(int unused_arg0, struct AttackCommand *owner) {
    func_00A94260();
    func_00A93288(owner);
}

/*
 * unitRecreate (proposed unitRecreate): replaces the owner unit by a new one created at
 * the battle index, keeping the owner's level and the battle scale; the data slot of
 * the owner's key is released first.
 */
void unitRecreate(Ov01Object *owner)
{
    f32 scale;
    int level;
    int index;

    level = owner->stats->posIndex;
    scale = battleStateGet()->scale;
    dataSlotFlagClear(owner->stats->key);
    dataSlotRelease(owner->stats->key);
    unitRemove(owner);
    index = battleStateGet()->slotIndex;
    ov01LastUnit = unitPlCreate(index);
    unitLoadSnapshot(owner, level);
    unitSetAngleRadians(ov01LastUnit, scale);
    dataUnitLoad(index);
    unitEffectSlotAcquire(index);
}

/*
 * battleUnitsReset (proposed battleUnitsReset): sets bit 1 of every unit's object flags,
 * resets the battle index to 0, calls sefSetGateForId(1, 0x37) and clears the deco flags.
 */
void battleUnitsReset(void)
{
    Ov01Object **items;
    int count;
    int index;
    Ov01Object *owner;

    index = 0;
    count = unitTblGet(2, &items);
    if (count > 0) {
        do {
            owner = items[index];
            index += 1;
            if (owner != 0) {
                owner->unit->drawFlags |= 1;
                unitUnbind(owner);
            }
        } while (index < count);
    }
    battleStateGet()->slotIndex = 0;
    sefSetGateForId(1, 0x37);
    decoFlagsClearAll();
}

/* Maps the ids 0x2D5..0x2D7 to 0x8D..0x8F, any other id to 0. */
int func_00A95198(int id)
{
    int value;

    switch (id) {
    case 0x2D5:
        value = 0x8D;
        break;
    case 0x2D6:
        value = 0x8E;
        break;
    case 0x2D7:
        value = 0x8F;
        break;
    default:
        value = 0;
        break;
    }
    return value;
}

/* Effect key ranges of the unit's own sequence: keys 700..733 move up by 100 for a technique of extent below 8. */
#define TEC_EFFECT_SEQUENCE_FIRST 700
#define TEC_EFFECT_SEQUENCE_COUNT 34
#define TEC_EFFECT_SEQUENCE_SHIFT 100

/* The effect group a technique uses for the unit (the high half of the key unitEffectKeyFree tests), by the
 * technique's gateFlag: fixed groups, the unit's sequence + 10, or the unit's sequence. */
int tecEffectGroupGet(Ov01Object *unit, TecRecord *tec)
{
    int group;

    switch (tec->gateFlag) {
    case 2:
    case 3:
        group = 0;
        break;
    case 9:
        group = 200;
        break;
    case 4:
        group = unit->stats->sequenceIndex + 10;
        break;
    case 1:
        group = 50;
        break;
    case 5:
        group = 14;
        break;
    case 6:
    case 7:
    case 8:
    default:
        group = unit->stats->sequenceIndex;
        if ((unsigned int)(group - TEC_EFFECT_SEQUENCE_FIRST) < TEC_EFFECT_SEQUENCE_COUNT) {
            if (tec->extent < 8) {
                group += TEC_EFFECT_SEQUENCE_SHIFT;
            }
        }
        break;
    }
    return group;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A95268);

void *memset(void *destination, int value, unsigned int count);
void sefStartEffectDefault(void *object);

/* Starts effect (keyHigh, keyLow) on the unit's actor. */
void unitEffectStart(Ov01Object *object, int keyHigh, int keyLow)
{
    struct EffectRequest request;

    memset(&request, 0, sizeof(request));
    request.key = (keyHigh << 0xC) | keyLow;
    request.head = object->unit;
    request.target = object->unit;
    sefStartEffectDefault(&request);
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A95540);

void func_00A955C8(Ov01Object *object) {
    sefApplyRecordMode(object->unit, (object->stats->sequenceIndex << 0xc) | 8, 3);
    func_00A95680(object);
}

void func_00A95608(Ov01Object *object)
{
    sefApplyRecordMode(object->unit, (object->stats->sequenceIndex << 0xC) | 8, 2);
    sefApplyRecordMode(object->unit, 0x12C001, 1);
    sefApplyRecordMode(object->unit, 0x12C002, 1);
    sefApplyRecordMode(object->unit, 0x12C003, 1);
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A95680);

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A95810);

/* unitSlotResume: clears the pause bit of the slot and sets its alpha to 1.0. */
void unitSlotResume(Ov01Object *object, int index)
{
    struct XenoActor *unit;

    unit = object->children[index];
    if (unit != 0)
    {
        unit->drawFlags &= ~4;
        unit->alpha = 1.0f;
    }
}

/* unitSlotPause: sets the pause bit of the slot and its alpha to 0. */
void unitSlotPause(Ov01Object *object, int index) {
    struct XenoActor *unit;

    unit = object->children[index];
    if (unit != 0) {
        unit->alpha = 0;
        unit->drawFlags |= 4;
    }
}

int XenoActorShowPart(struct XenoActor *, int);
extern int ov01Slot1Part;

/* unitSlotShowPart (proposed unitSlotShowPart): shows part ov01Slot1Part of slot 1 when the slot exists. */
void unitSlotShowPart(Ov01Object *object) {
    struct XenoActor *unit;

    unit = object->children[1];
    if (unit != 0) {
        XenoActorShowPart(unit, ov01Slot1Part);
    }
}

int XenoActorHidePart(struct XenoActor *, int);

/* unitSlotHidePart (proposed unitSlotHidePart): hides part ov01Slot1Part of slot 1 when the slot exists. */
void unitSlotHidePart(Ov01Object *object) {
    struct XenoActor *unit;

    unit = object->children[1];
    if (unit != 0) {
        XenoActorHidePart(unit, ov01Slot1Part);
    }
}

/* The part of the unit's own actor that stands in for a slot actor (unitSlotShowHidePart hides it while the slot
 * is shown, unitSlotHideShowPart shows it while the slot is paused). */
extern int ov01UnitSlotPart;

/* Shows slot index of the unit (clears the pause bit, alpha 1) and hides part ov01UnitSlotPart of its actor. */
void unitSlotShowHidePart(Ov01Object *object, int index) {
    struct XenoActor *unit;

    unit = object->children[index];
    if (unit != 0) {
        unit->drawFlags &= ~4;
        unit->alpha = 1.0f;
        XenoActorHidePart(object->unit, ov01UnitSlotPart);
    }
}

/* Pauses slot index of the unit (alpha 0, pause bit) and shows part ov01UnitSlotPart of its actor. */
void unitSlotHideShowPart(Ov01Object *object, int index) {
    struct XenoActor *unit;

    unit = object->children[index];
    if (unit != 0) {
        unit->alpha = 0;
        unit->drawFlags |= 4;
        XenoActorShowPart(object->unit, ov01UnitSlotPart);
    }
}

int unitSlotFadeIn(void);

int func_00A95CB0(void) {
    return unitSlotFadeIn();
}

int unitSlotFadeOut(void);

int func_00A95CC8(void) {
    return unitSlotFadeOut();
}

int func_00A95CE0(void) {
    return 1;
}

int func_00A95CE8(void) {
    return 1;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A95CF0);

int ov01CdPendingReads(void);
int loadWaitPending(void);

/* 1 once no CD read and no load wait is pending. */
int ov01LoadsIdle(void)
{
    if (ov01CdPendingReads() == 0) {
        if (loadWaitPending() == 0) {
            return 1;
        }
    }
    return 0;
}

/*
 * unitAttachRequest (proposed unitAttachRequest): the kind-8 event handler. It attaches
 * the requested unit to the model entry in ov01ModelEntry; when the slot's state is 1 it
 * also attaches the handle that unitPairPartnerFind matches for the request's value.
 */
int unitAttachRequest(Ov01Tu033Request *request)
{
    int id;
    int matchedHandle;

    if (ov01ModelEntry != 0) {
        id = request->id;
        unitAttachChild(id, ov01ModelEntry, 0);
        if (tecRecordGet(request->index)->gateFlag == 1) {
            /* The kind-8 request carries the unit as its word, and unitAttachChild takes words. */
            matchedHandle = (int) unitPairPartnerFind((Ov01Object *) id, request->value);
            unitAttachChild(matchedHandle, ov01ModelEntry, 1);
        }
    }
    return 1;
}

/*
 * unitSpawnFromRequest (proposed unitSpawnFromRequest): the kind-9 event handler. It creates
 * the requested unit (unitPlCreate for ids outside 0x10..0x8C, else unitEnCreate),
 * loads its snapshot and angle, loads its data and keeps it in ov01LastUnit with flag 8.
 */
int unitSpawnFromRequest(Ov01Tu033Request *request)
{
    int id;
    int index;
    int value;

    id = request->id;
    index = request->index;
    value = request->value;
    if ((u32) (id - 0x10) >= 0x7D) {
        ov01LastUnit = unitPlCreate(id);
    } else {
        ov01LastUnit = unitEnCreate(id);
    }
    unitLoadSnapshot(ov01LastUnit, index);
    unitSetAngleDegrees(ov01LastUnit, value);
    dataUnitLoad(id);
    unitEffectSlotAcquire(id);
    ov01LastUnit->flags |= 8;
    return 1;
}

void unitBindActors(Ov01Object *object, struct DataSlot *slot);

/* Bind event handler: binds the data slot of request id to ov01LastUnit, fades
 * it in and starts the load (returns 0); on the next calls returns 1 once the
 * load is done. 0 also when the slot is not found. */
int unitBindRequest(Ov01Tu033Request *request)
{
    struct DataSlot *slot;

    if (eventStepStarted == 0) {
        slot = dataSlotFind(request->id);
        if (slot == 0) {
            return 0;
        }
        unitBindActors(ov01LastUnit, slot);
        unitFadeIn(ov01LastUnit);
        func_00A955C8(ov01LastUnit);
        ov01LoadRequest(slot);
        eventStepStarted = 1;
    } else if (ov01LoadIsDone() != 0) {
        return 1;
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A96050);

void unitLevelAdd(void *, int);
void calcHpAdjust(void *, int);
int calcStatGet(void *, int, int);
void calcStatReset(void *, int, int);
int calcStatAltReset(void *, int, int);
void func_00A96510(void *, int, int, int);

/*
 * battleResolveStatus (proposed battleResolveStatus): the kind-12 event handler. While the
 * battle object has status bits in rows 1-3 it waits for its queue (returns 0); then it
 * resolves the object and each unit with state 0: HP adjust, level add and act stock
 * for bit 0x4000 of row 3, HP adjust for bit 0x400, and both bits are cleared.
 */
int battleResolveStatus(void) {
    Ov01Object **units;
    Ov01Object *object;
    Ov01Object *unit;
    int first;
    int second;
    int third;
    int count;
    int i;
    int result;

    object = battleStateGet()->object;
    first = battleStateGet()->first;
    second = battleStateGet()->second;
    third = battleStateGet()->third;
    if (object != 0 && (calcStatGet(object, 1, 0xFFFF) != 0 || calcStatGet(object, 2, 0xFFFF) != 0 || calcStatGet(object, 3, 0xFFFF) != 0)) {
        result = unitQueueIsEmpty(object);
        if (result == 0) {
            return 0;
        }
        func_00A96510(object, first, second, third);
    }
    count = unitTblGet(2, &units);
    for (i = 0; i < count; i++) {
        unit = units[i];
        if (unit != 0 && unit->stats->hp == 0) {
            if (calcStatGet(unit, 3, 0x4000) != 0) {
                calcHpAdjust(units[i], 1);
                unitLevelAdd(units[i], 3);
                if (calcBoostOk(units[i], 0) != 0) {
                    calcActStockSet(units[i], 0);
                    unitEffectStart(units[i], 0x140, 7);
                    seVoiceStartTagged(2, 1);
                }
                calcStatReset(units[i], 3, 0x4000);
                calcStatAltReset(units[i], 3, 0x4000);
            } else if (calcStatGet(units[i], 3, 0x400) != 0) {
                calcHpAdjust(units[i], 1);
                calcStatReset(units[i], 3, 0x400);
                calcStatAltReset(units[i], 3, 0x400);
            }
        }
    }
    return 1;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A96510);

/* Row-3 statuses that bring a down unit back when the battle step ends (unitsReviveDown): with its full
 * maxHp, or with half of it. */
#define STAT3_REVIVE_FULL 0x800
#define STAT3_REVIVE_HALF 0x2000

int unitsReviveDown(void);

/* Waits (returns 0) while a side-2 unit with a revive status (row 3) still has queued commands; then runs
 * unitsReviveDown and returns 1. */
int battleStatusQueueWait(void)
{
    Ov01Object **units;
    Ov01Object *unit;
    int count;
    int i;

    count = unitTblGet(2, &units);
    for (i = 0; i < count; i++) {
        unit = units[i];
        if (unit != 0) {
            if (calcStatGet(unit, 3, STAT3_REVIVE_FULL) != 0 || calcStatGet(unit, 3, STAT3_REVIVE_HALF) != 0) {
                if (unitQueueIsEmpty(unit) == 0) {
                    return 0;
                }
            }
        }
    }
    unitsReviveDown();
    return 1;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", unitsReviveDown);

/* monsSlotCreate (tu019.c) returns a record pointer; this caller keeps its int view (the int global), see tu019.h. */
int monsSlotCreate(void);
extern int ov01StreamSlot;

/*
 * ov01StreamOpen (proposed ov01StreamOpen): the kind-14 event handler. It stores the
 * MonsSlot that monsSlotCreate creates (monsSlotCreate, tu019) in ov01StreamSlot and opens
 * data stream 0x43.
 */
int ov01StreamOpen(void) {
    ov01StreamSlot = monsSlotCreate();
    dataStreamOpen(0x43, 1);
    return 1;
}

void *dataStreamFind(int id);
void monsSlotCopyParams(int slot, void *stream);

/* Once data stream 0x43 (opened by ov01StreamOpen) is found, hands it to the
 * MonsSlot in ov01StreamSlot (monsSlotCopyParams) and returns 1; 0 until then. */
int ov01StreamBind(void)
{
    void *stream = dataStreamFind(0x43);

    if (stream == 0) {
        return 0;
    }
    monsSlotCopyParams(ov01StreamSlot, stream);
    return 1;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A96AA8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A96E38);
