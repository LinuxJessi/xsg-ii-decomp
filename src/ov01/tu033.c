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
#include "main/sef.h"
#include "main/xenoActor.h"
#include "ov01/tu002.h"
#include "ov01/tu005.h"
#include "ov01/tu008.h"
#include "main/tu051.h"
#include "ov01/tu009.h"
#include "ov01/tu010.h"
#include "ov01/tu011.h"
#include "ov01/tu012.h"
#include "ov01/tu019.h"
#include "ov01/tu017.h"
#include "ov01/tu018.h"
#include "ov01/tu020.h"
#include "ov01/tu023.h"
#include "ov01/tu033.h"
#include "ov01/tu042.h"
#include "ov01/tu043.h"
#include "ov01/tu036.h"
#include "ov01/tu038.h"
#include "ov01/tu054.h"
#include "ov01/tu066.h"
#include "ov01/tu068.h"
#include "ov01/tu070.h"

extern char cmdList[];   /* cmdList: 0x20-byte entries, unit then command body */
extern struct FifoRing cmdFifo;   /* the fifo over the cmdList entries */

/* Command list entries (0x20 bytes each); the fifo ring runs over them. */
#define CMD_LIST_COUNT 0x20
#define CMD_LIST_ENTRY_SIZE 0x20

/* Payloads the appenders copy whole into a record (four, six and eleven words). */

/* The four-word payload of eventQueueAdd. */
typedef struct EventQueueArgs {
    int actions;                        /* +0x00: kind 3: the kind-4 records of the attack */
    union {
        int tecIndex;                   /* the tec whose EP cost cmdStockRun charges */
        int hits;                       /* kind 3: the kind-5 records (targets) of the attack */
    };                                  /* +0x04 */
    unsigned char unmodeled_08[0x4];
    Ov01Object *target;                 /* +0x0C: the partner / stocked unit of events 0x10 and 0x12 */
} EventQueueArgs;

/* One 0x30-byte record of eventQueueTable (eventQueueTable); kind is its first word,
 * the payload layout depends on the appender. */
typedef struct EventQueueEntry {
    int kind;
    union {
        EventQueueArgs args;          /* eventQueueAdd */
        struct AttackCommand attack;  /* eventQueueAddKind4: the attack (attackQueue) */
        struct {
            void *object;
            CmdListBody args;
        } kind1;                      /* eventQueueAddKind1 */
        struct HitRecord kind5;       /* eventQueueAddKind5: the hit record of one target */
    } data;
} EventQueueEntry;

/* Record kinds by their handler in eventQueueHandlers (the kinds the code here queues). */
enum {
    EVENT_KIND_WAIT_UNIT_QUEUE = 2,     /* eventWaitUnitQueue */
    EVENT_KIND_ATTACK = 3,              /* func_00A938A0: shows the attack (unitsMarkUninvolved) */
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

void *memset(void *destination, int value, unsigned int count);

/* cmdSystemInit: clears the command list and fifo, the event table and its counters. The four counters are
 * cleared in one chained assignment: as four statements gcc loads their addresses in the other order. */
void cmdSystemInit(void)
{
    memset(cmdList, 0, CMD_LIST_COUNT * CMD_LIST_ENTRY_SIZE);
    ov01FifoInit(&cmdFifo, CMD_LIST_COUNT);
    memset(eventQueueTable, 0, sizeof(eventQueueTable));
    eventStepStarted = eventRetryCount = eventQueueCursor = eventQueueCount = 0;
}

/* cmdListPtrGet: the command fifo and the command list, through two out pointers. */
void cmdListPtrGet(void **fifo_out, void **list_out) {
    *fifo_out = &cmdFifo;
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

int cmdExecute(Ov01Object *unit, CmdListBody *command);

/* cmdListRunHead: clears the event queue counters, then runs the head command of the list unless its unit is
 * down and pops it; returns 0 when the list is empty, else 1. */
int cmdListRunHead(void)
{
    void *fifo;
    void *list;
    CmdListEntry *entry;
    Ov01Object *unit;
    CmdListBody *body;

    /* One chained assignment: four separate stores give the same stores with the lui order reversed. */
    eventStepStarted = eventRetryCount = eventQueueCursor = eventQueueCount = 0;
    cmdListPtrGet(&fifo, &list);
    if (ov01FifoGetUsed(fifo) == 0) {
        return 0;
    }
    /* cmdListSet's integer form; indexing the list gives list + offset instead of offset + list. */
    entry = (CmdListEntry *)((char *)(ov01FifoGetHead(fifo) * sizeof(CmdListEntry)) + (int)list);
    unit = (Ov01Object *)entry->unit;
    body = &entry->body;
    if (!(unit->flags & UNIT_FLAG_DOWN)) {
        cmdExecute(unit, body);
    }
    ov01FifoPop(fifo);
    return 1;
}

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


int attackQueue(struct AttackCommand *command, struct Ov01Object *target, TecRecord *tec);

int func_00A92D98(struct AttackCommand *owner, struct Ov01Object *value) {
    TecRecord *entry;
    int slot;
    int result;

    entry = tecRecordGet(owner->actionIdx);
    slot = entry->kindBits.slot;
    result = slot < 8;
    if (result != 0 && slot >= 0) {
        result = attackQueue(owner, value, entry);
    }
    return result;
}

void func_00A92DF0(struct AttackCommand *owner, struct Ov01Object *value) {
    attackQueue(owner, value, tecRecordGet(owner->actionIdx));
}

void *memset(void *destination, int value, unsigned int count);
struct Ov01Object *tgtFindNext(void);
signed char unitRecordStateGet(struct Ov01Object *unit, int index);
int actionTransformSlotGet(int id);
void eventQueueAddKind4(struct AttackCommand *command);
void eventQueueAddKind5(struct HitRecord *args);
extern int D_00AC3AFC;

/* Queues an attack: a kind-5 record per target (tgtFindNext walks them) with its hit record and the unit command
 * the target answers with, the attack itself as a kind-4 record (twice with the partner for a pair technique), and
 * a kind-3 record with both counts. Returns 0 for an unknown hit kind. */
int attackQueue(struct AttackCommand *command, struct Ov01Object *target, TecRecord *tec)
{
    struct HitRecord hit;
    EventQueueArgs counts;
    struct AttackCommand pair;
    int kind;
    int flags;
    int hits;

    hits = 0;
    do {
        if (tec->kindBits.kind != 0) {
            hit.target = target;
            calcHitRecordFill(command, &hit);
            kind = hit.hitMode.word & 0xFF;
            flags = (hit.hitMode.word >> 8) & 0xFF;
            battleStateGet()->hitDamage = hit.damage;
            battleStateGet()->hitMode = hit.hitMode.word;
            switch (kind) {
            case 0:
                hit.reaction = UNIT_CMD_JUMP_EFFECT;
                break;
            case 1:
            case 3:
            case 7:
            case 8:
            case 9:
            case 10:
                if (flags & 2) {
                    hit.reaction = UNIT_CMD_HOVER_EFFECT;
                } else {
                    hit.reaction = UNIT_CMD_MODE_DISPATCH;
                }
                break;
            case 2:
            case 4:
                if (flags & 8) {
                    hit.reaction = UNIT_CMD_PHASE_2B_WAIT;
                } else {
                    hit.reaction = UNIT_CMD_CLEAR_ACTIVE;
                }
                break;
            case 5:
            case 6:
                hit.reaction = UNIT_CMD_DEACTIVATE;
                break;
            default:
                return 0;
            }
            target->stats->source = command->attacker;
            target->stats->sourceAction = command->actionIdx;
        } else {
            memset(&hit, 0, sizeof(hit));
            hit.target = target;
            hit.reaction = 0;
            hit.hitMode.word = (hit.hitMode.word & 0xFF00) | 0xB;
        }
        eventQueueAddKind5(&hit);
        hits++;
    } while ((target = tgtFindNext()) != 0);
    eventQueueAddKind4(command);
    kind = 1;
    command->attacker->stats->valueLevel += unitRecordStateGet(command->attacker, command->actionIdx);
    if (tecRecordGet(command->actionIdx)->gateFlag == 1) {
        kind = 2;
        pair = *command;
        pair.attacker = unitPairPartnerFind(command->attacker, command->id);
        eventQueueAddKind4(&pair);
    }
    if (actionTransformSlotGet(command->actionIdx) != 0) {
        D_00AC3AFC = 1;
    } else {
        D_00AC3AFC = 0;
    }
    counts.actions = kind;
    counts.hits = hits;
    eventQueueAdd(EVENT_KIND_ATTACK, &counts);
    return 1;
}

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

/* For the attack kinds 1, 3 and 4: whether the sef entry of the attacker's actor has flag 1; 0 otherwise. */
int func_00A93128(Ov01Object *attacker, int kind)
{
    if (kind != 1 && kind != 3 && kind != 4) {
        return 0;
    }
    return sefEntryHasFlag1((int)attacker->unit) != 0;
}

/* Queues the reaction command of hit for its target (the mode, effect and activity commands; others are
 * ignored). The mode dispatch takes the hit's words at +0x0C and +0x10 as its code and wait, and the face
 * target. When the running command and the new one are both mode or effect commands, the running one is dropped
 * first. */
void unitCmdRequestQueue(Ov01Object **faceTarget, struct HitRecord *hit)
{
    Ov01Command command;
    Ov01Command *current;

    switch (hit->reaction) {
    case UNIT_CMD_MODE_DISPATCH:
        command.command = UNIT_CMD_MODE_DISPATCH;
        command.code = hit->followResult;
        command.arg.wait = hit->backAttack;
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
    current = dataLookupCurrent(&hit->target->commandQueue);
    if (current != 0
        && (current->command == UNIT_CMD_MODE_DISPATCH || current->command == UNIT_CMD_MODE_START
            || current->command == UNIT_CMD_HOVER_EFFECT || current->command == UNIT_CMD_MOVE_RATIO)) {
        if (hit->reaction == UNIT_CMD_MODE_DISPATCH || hit->reaction == UNIT_CMD_MODE_START
            || hit->reaction == UNIT_CMD_HOVER_EFFECT) {
            dataLookupNext(&hit->target->commandQueue);
        }
    }
    unitQueueAppend(hit->target, &command);
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
#include "main/tu125.h"

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
extern struct DataEntry *ov01ModelEntry;
int dataSlotRelease(s16);
int dataSlotFlagClear(s16);
int unitEffectSlotAcquire(int);
int unitRemove(Ov01Object *);
int sefApplyRecordMode(struct XenoActor *, int, int);
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

/* eventQueueAddKind4: appends a kind-4 record holding the attack command. */
void eventQueueAddKind4(struct AttackCommand *command)
{
    EventQueueEntry *entry = eventQueueReserve();

    entry->kind = 4;
    entry->data.attack = *command;
    eventQueueCount++;
}

/* eventQueueAddKind5: appends a kind-5 record with a 0x2C-byte payload. */
void eventQueueAddKind5(struct HitRecord *args)
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

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", eventUnitCommand);


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

/* The steps of a transforming action (actionTransformSlotGet gives its model): attackStep runs through them. */
enum {
    ATTACK_STEP_TRANSFORM,              /* swap the attacker's model to the transform slot */
    ATTACK_STEP_FILL_IN,                /* hold the screen fill for ATTACK_FILL_FRAMES */
    ATTACK_STEP_RESOLVE,                /* resolve the attack (func_00A95018), start the screen fill */
    ATTACK_STEP_RECREATE,               /* recreate the unit */
    ATTACK_STEP_RESTORE,                /* bind the battle slot's data again, show the monster slots */
    ATTACK_STEP_RESET,                  /* reset the battle units */
    ATTACK_STEP_FILL_OUT                /* hold the screen fill again, then done */
};
#define ATTACK_FILL_FRAMES 3

/* The percent chance of the target's first drop for the two stealing actions (unitDataBoxDrop): 0x2C9 takes it
 * half the time, 0x2F6 always (else the second drop). */
#define STEAL_CHANCE_HALF 50
#define STEAL_CHANCE_SURE 100

extern int attackStep;
extern int attackFillWait;
extern int D_00AC3AFC;
void unitBindActors(Ov01Object *object, struct DataSlot *slot);
void unitDataBoxDrop(Ov01Object *target, int value);
int func_00A95018(EventQueueEntry *entry, struct AttackCommand *owner);
void unitRecreate(Ov01Object *owner);
void battleUnitsReset(void);
void itemRandomPick(void);

/*
 * One step of the special actions of a queued attack (attackStep counts their steps); returns 1 when the action
 * is done. Plain actions resolve at once (func_00A93288); 0x2C4 and 0x2C9 free their effect key first,
 * 0x2D5..0x2D7 transform the attacker, 0x2F6 waits for the text block. Unknown actions return 0.
 */
int attackSpecialStep(EventQueueEntry *entry, struct AttackCommand *command)
{
    struct MonsSlot **slots;
    struct HitRecord *hit;
    TecRecord *tec;
    struct DataSlot *slot;
    int done;

    done = 0;
    hit = &eventQueueFindKind5(0)->data.kind5;
    tec = tecRecordGet(command->actionIdx);
    switch (command->actionIdx) {
    case 0x38:
    case 0x2A4:
    case 0x2C3:
    case 0x2C5:
    case 0x2C6:
        done = func_00A93288(command);
        break;
    case 0x2C4:
        if (attackStep == 0) {
            if (unitEffectKeyFree(command->attacker, tecEffectGroupGet(command->attacker, tec), tec->extent) != 0) {
                itemRandomPick();
                attackStep++;
            }
        } else if (msgIsIdle() != 0) {
            done = 1;
        }
        break;
    case 0x2C8:
        if (textBlockIsActive() == 0) {
            done = 1;
        }
        break;
    case 0x2C9:
        if (attackStep == 0) {
            if (unitEffectKeyFree(command->attacker, tecEffectGroupGet(command->attacker, tec), tec->extent) != 0) {
                unitDataBoxDrop(hit->target, STEAL_CHANCE_HALF);
                attackStep++;
            }
        } else if (msgIsIdle() != 0) {
            done = 1;
        }
        break;
    case 0x2D5:
    case 0x2D6:
    case 0x2D7:
        switch (attackStep) {
        case ATTACK_STEP_TRANSFORM:
            if (ov01CdPendingReads() != 0) {
                break;
            }
            if (loadWaitPending() != 0) {
                break;
            }
            slot = dataSlotFind(actionTransformSlotGet(command->actionIdx));
            if (slot == 0) {
                return 1;
            }
            unitBindActors(ov01LastUnit, slot);
            unitUnbind(ov01LastUnit);
            D_00AC3AFC = 0;
            command->attacker = ov01LastUnit;
            unitAttachChild(ov01LastUnit, ov01ModelEntry, 0);
            unitSetTimedPhase(ov01LastUnit, 1);
            attackFillWait = 0;
            attackStep++;
            break;
        case ATTACK_STEP_FILL_IN:
            if (attackFillWait >= ATTACK_FILL_FRAMES) {
                screenFillEnd();
                attackStep++;
            }
            attackFillWait++;
            break;
        case ATTACK_STEP_RESOLVE:
            if (func_00A95018(entry, command) == 0) {
                break;
            }
            screenFillStart();
            attackStep++;
            break;
        case ATTACK_STEP_RECREATE:
            unitRecreate(ov01LastUnit);
            attackStep++;
            break;
        case ATTACK_STEP_RESTORE:
            if (ov01CdPendingReads() != 0) {
                break;
            }
            if (loadWaitPending() != 0) {
                break;
            }
            slot = dataSlotFind(battleStateGet()->slotIndex);
            if (slot == 0) {
                return 1;
            }
            unitBindActors(ov01LastUnit, slot);
            func_00A955C8(ov01LastUnit);
            command->attacker = ov01LastUnit;
            monsActiveSlotsGet(&slots);
            monsSlotShow(slots[0]);
            attackStep++;
            break;
        case ATTACK_STEP_RESET:
            battleUnitsReset();
            attackFillWait = 0;
            attackStep++;
            break;
        case ATTACK_STEP_FILL_OUT:
            if (attackFillWait >= ATTACK_FILL_FRAMES) {
                screenFillEnd();
                done = 1;
            }
            attackFillWait++;
            break;
        }
        break;
    case 0x2F6:
        switch (attackStep) {
        case 0:
            if (textBlockIsActive() == 0) {
                attackStep++;
            }
            break;
        case 1:
            unitDataBoxDrop(hit->target, STEAL_CHANCE_SURE);
            attackStep++;
            break;
        default:
            done = msgIsIdle() != 0;
            break;
        }
        break;
    }
    return done;
}

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
    strcat(itemPickText, hitActionTextGet(item, 0));
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

/* Ov01Object.flags bits of the data box drops already taken from a unit: the first and the second pair of its
 * TableKeyRecord. */
#define UNIT_FLAG_DROP_FIRST 0x10000
#define UNIT_FLAG_DROP_SECOND 0x8000
/* The data box category whose names itemTextGet gives (the others: hitActionTextGet). */
#define DATA_BOX_CATEGORY_2 2


/*
 * The data box drop of a unit: with chance percent its first pair, else (or when that is empty or taken) its
 * second; a pair is taken once. Composes the drop message ("<name> ..." or the nothing text) in itemPickText and
 * shows it.
 */
void unitDataBoxDrop(Ov01Object *unit, int percent)
{
    struct TableKeyRecord *record;
    char *name;
    int category;
    int id;

    record = tableKeyRecordGet(unit->stats->key);
    if (ov01RollPercent(percent) != 0 && record->firstId != 0 && !(unit->flags & UNIT_FLAG_DROP_FIRST)) {
        category = record->firstCategory;
        id = record->firstId;
        unit->flags |= UNIT_FLAG_DROP_FIRST;
    } else if (record->secondId != 0 && !(unit->flags & UNIT_FLAG_DROP_SECOND)) {
        category = record->secondCategory;
        id = record->secondId;
        unit->flags |= UNIT_FLAG_DROP_SECOND;
    } else {
        category = -1;
        id = 0;
    }
    if (id != 0) {
        if (category == DATA_BOX_CATEGORY_2) {
            name = itemTextGet(id, 0);
        } else {
            name = hitActionTextGet(id, 0);
        }
        strcpy(itemPickText, tableStringGet(2, 1));
        strcat(itemPickText, name);
        strcat(itemPickText, itemPickTextEnd);
        dataBoxInc(category, id);
    } else {
        strcpy(itemPickText, tableStringGet(3, 1));
    }
    msgPutObject(0x14, 0x120, itemPickText, 0x2D, 0x1AC, 0x28);
}

/* The sef gate the transform closes (owner 0). */
#define TRANSFORM_SEF_GATE 0x37

/*
 * Starts the transform of unit by action: keeps its key as the battle slot index and its facing as the battle
 * scale, rebinds the side-2 units, removes the unit and creates the transformed one (actionTransformSlotGet) as
 * ov01LastUnit facing pi, loads its data and hides the first monster slot.
 */
void unitTransformBegin(Ov01Object *unit, int action)
{
    struct MonsSlot **slots;
    Ov01Object **units;
    Ov01Object *other;
    int count;
    int i;
    int key;
    int posIndex;
    int model;

    battleStateGet()->slotIndex = unit->stats->key;
    count = unitTblGet(2, &units);
    for (i = 0; i < count; i++) {
        other = units[i];
        if (other != 0) {
            other->unit->drawFlags &= ~ACTOR_STATE_UNBOUND;
            unitBindSetup(other);
        }
    }
    posIndex = unit->stats->posIndex;
    key = unit->stats->key;
    battleStateGet()->scale = unit->stats->facing;
    sefReleaseWaitSlot(unit->stats->sequenceIndex);
    unitRemove(unit);
    model = actionTransformSlotGet(action);
    ov01LastUnit = unitPlCreate(model);
    unitClearSnapshot(unit);
    unit->stats->posIndex = posIndex;
    unitSetAngleRadians(ov01LastUnit, 3.1415927f);
    dataUnitLoad(model);
    dataSlotFlagClear(key);
    monsActiveSlotsGet(&slots);
    monsSlotHide(slots[0]);
    sefSetGateForId(0, TRANSFORM_SEF_GATE);
    decoFlagsSetAll();
}

int func_00A94260();

int func_00A95018(EventQueueEntry *entry, struct AttackCommand *owner) {
    func_00A94260();
    return func_00A93288(owner);
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
                owner->unit->drawFlags |= ACTOR_STATE_UNBOUND;
                unitUnbind(owner);
            }
        } while (index < count);
    }
    battleStateGet()->slotIndex = 0;
    sefSetGateForId(1, 0x37);
    decoFlagsClearAll();
}

/* Maps the ids 0x2D5..0x2D7 to 0x8D..0x8F, any other id to 0. */
int actionTransformSlotGet(int id)
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

/* Party slot keys 11..15 (the slots with linkedSlots) start their effects in mode 3, the others in mode 2. */
#define EFFECT_KEY_MODE3_FIRST 11
#define EFFECT_KEY_MODE3_COUNT 5

/* Starts effect (keyHigh, keyLow) on the unit's actor, in mode 3 for unit keys 11..15, else 2. */
void func_00A95540(Ov01Object *object, int keyHigh, int keyLow)
{
    struct EffectRequest request;
    int mode;

    memset(&request, 0, sizeof(request));
    mode = ((unsigned int)(object->stats->keyUnsigned - EFFECT_KEY_MODE3_FIRST) < EFFECT_KEY_MODE3_COUNT) ? 3 : 2;
    request.key = (keyHigh << 0xC) | keyLow;
    request.head = object->unit;
    request.mode = mode;
    request.target = object->unit;
    sefStartEffectDefault(&request);
}

void func_00A955C8(Ov01Object *object) {
    sefApplyRecordMode(object->unit, (object->stats->sequenceIndex << 0xc) | 8, 3);
    unitStatusAuraUpdate(object);
}

void func_00A95608(Ov01Object *object)
{
    sefApplyRecordMode(object->unit, (object->stats->sequenceIndex << 0xC) | 8, 2);
    sefApplyRecordMode(object->unit, 0x12C001, 1);
    sefApplyRecordMode(object->unit, 0x12C002, 1);
    sefApplyRecordMode(object->unit, 0x12C003, 1);
}

/* Status aura effects of a unit: effect group 0x12C, records 1..3 (one per state unitStatusAuraUpdate tells apart). */
#define AURA_GROUP 0x12C
#define AURA_KEY(record) ((AURA_GROUP << 12) | (record))

/*
 * Shows the status aura that matches the unit's state (stat 2 at level 1, stat 2 at any level, stat 1 at any
 * level), starting it when it does not run yet, and switches the other two auras off (mode 1). Nothing for a unit
 * with flag 2.
 */
void unitStatusAuraUpdate(Ov01Object *object)
{
    if (object->flags & UNIT_FLAG_HIDDEN) {
        return;
    }
    if (calcStatGet(object, 2, 1)) {
        if (!sefOwnerPairInUse((int)object->unit, AURA_KEY(3))) {
            unitEffectStart(object, AURA_GROUP, 3);
        }
        sefApplyRecordMode(object->unit, AURA_KEY(2), 1);
        sefApplyRecordMode(object->unit, AURA_KEY(1), 1);
    } else if (calcStatGet(object, 2, 0xFFFF)) {
        if (!sefOwnerPairInUse((int)object->unit, AURA_KEY(2))) {
            unitEffectStart(object, AURA_GROUP, 2);
        }
        sefApplyRecordMode(object->unit, AURA_KEY(3), 1);
        sefApplyRecordMode(object->unit, AURA_KEY(1), 1);
    } else if (calcStatGet(object, 1, 0xFFFF)) {
        if (!sefOwnerPairInUse((int)object->unit, AURA_KEY(1))) {
            unitEffectStart(object, AURA_GROUP, 1);
        }
        sefApplyRecordMode(object->unit, AURA_KEY(3), 1);
        sefApplyRecordMode(object->unit, AURA_KEY(2), 1);
    } else {
        sefApplyRecordMode(object->unit, AURA_KEY(1), 1);
        sefApplyRecordMode(object->unit, AURA_KEY(2), 1);
        sefApplyRecordMode(object->unit, AURA_KEY(3), 1);
    }
}

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

extern const char *ov01Slot1Part;

/* unitSlotShowPart (proposed unitSlotShowPart): shows part ov01Slot1Part of slot 1 when the slot exists. */
void unitSlotShowPart(Ov01Object *object) {
    struct XenoActor *unit;

    unit = object->children[1];
    if (unit != 0) {
        XenoActorShowPart(unit, ov01Slot1Part);
    }
}


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
extern const char *ov01UnitSlotPart;

/* Shows slot index of the unit (clears the pause bit, alpha 1) and hides part ov01UnitSlotPart of its actor. */
void unitSlotShowHidePart(Ov01Object *object, int index) {
    struct XenoActor *unit;

    unit = object->children[index];
    if (unit != 0) {
        unit->drawFlags &= ~ACTOR_STATE_HIDDEN;
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
        unit->drawFlags |= ACTOR_STATE_HIDDEN;
        XenoActorShowPart(object->unit, ov01UnitSlotPart);
    }
}

/* Fades slot index of the unit in (unitSlotFadeIn). */
struct FadeTask *func_00A95CB0(Ov01Object *object, int index) {
    return unitSlotFadeIn(object, index);
}

/* Fades slot index of the unit out (unitSlotFadeOut). */
struct FadeTask *func_00A95CC8(Ov01Object *object, int index) {
    return unitSlotFadeOut(object, index);
}

int func_00A95CE0(void) {
    return 1;
}

int func_00A95CE8(void) {
    return 1;
}

/* sefLoadEffect (main) as ov01 calls it: the effect key and its group (tu046 loadEffect passes both too). */
int sefLoadEffect(int effectKey, int group);

/*
 * Loads what the action of request needs: the motion entries of its technique (one, or two for a pair technique,
 * the partner's first when the unit is not the pair's first slot), the model of a gateFlag-5 technique's target
 * record (joint in pairSecond[2]) and its effect group. Returns 1.
 */
int func_00A95CF0(Ov01Tu033Request *request)
{
    Ov01Object *unit;
    int value;
    TecRecord *tec;
    TgtIdRecord *record;
    int first;
    int second;
    int group;

    unit = (Ov01Object *) request->id;
    tec = tecRecordGet(request->index);
    value = request->value;
    ov01ModelEntry = 0;
    if (tec->timer != 0) {
        if (tec->gateFlag == 1) {
            if (pairLinkGet(value)->slotFirst == unit->stats->key) {
                first = 0;
                second = 1;
            } else {
                first = 1;
                second = 0;
            }
            ov01ModelEntry = dataMotEntryLoad(tec->timer, first, 0);
            ov01ModelEntry = dataMotEntryLoad(tec->timer, second, 1);
        } else {
            ov01ModelEntry = dataMotEntryLoad(tec->timer, 0, 0);
        }
    }
    if (tec->gateFlag == 5) {
        record = tgtIdRecordGet(value);
        if (record->modelIndex != 0) {
            unit->stats->pairSecond[2] = record->modelJoint;
            ov01ModelEntry = dataModelEntryLoad(record->modelIndex);
        }
    }
    group = tecEffectGroupGet(unit, tec);
    sefLoadEffect((group << 12) | tec->extent, group);
    return 1;
}

int ov01CdPendingReads(void);

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
    Ov01Object *unit;

    if (ov01ModelEntry != 0) {
        /* The kind-8 request carries the unit as its word. */
        unit = (Ov01Object *) request->id;
        unitAttachChild(unit, ov01ModelEntry, 0);
        if (tecRecordGet(request->index)->gateFlag == 1) {
            unitAttachChild(unitPairPartnerFind(unit, request->value), ov01ModelEntry, 1);
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

int func_00A96510(Ov01Object *unit, int commandKind, int hitDamage, int hitMode);

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
    int commandKind;
    int hitDamage;
    int hitMode;
    int count;
    int i;
    int result;

    object = battleStateGet()->object;
    commandKind = battleStateGet()->commandKind;
    hitDamage = battleStateGet()->hitDamage;
    hitMode = battleStateGet()->hitMode;
    if (object != 0 && (calcStatGet(object, 1, 0xFFFF) != 0 || calcStatGet(object, 2, 0xFFFF) != 0 || calcStatGet(object, 3, 0xFFFF) != 0)) {
        result = unitQueueIsEmpty(object);
        if (result == 0) {
            return 0;
        }
        func_00A96510(object, commandKind, hitDamage, hitMode);
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

/* Brings back every down side-2 unit with a revive status: clears the status, queues
 * UNIT_CMD_PHASE_2B_WAIT, restores maxHp (or half of it), clears UNIT_FLAG_DOWN and shows the amount
 * (func_00AA4DB8 with flags 0x802). Returns 1. */
int unitsReviveDown(void)
{
    Ov01Command command;
    Ov01Object **units;
    Ov01Object *unit;
    int count;
    int i;
    int amount;
    int shown;

    count = unitTblGet(2, &units);
    for (i = 0; i < count; i++) {
        unit = units[i];
        amount = 0;
        if (unit == 0) {
            continue;
        }
        if (unit->flags & UNIT_FLAG_DOWN) {
            if (calcStatGet(unit, 3, STAT3_REVIVE_FULL) != 0) {
                amount = unit->stats->maxHp;
                calcStatReset(unit, 3, STAT3_REVIVE_FULL);
                calcStatAltReset(unit, 3, STAT3_REVIVE_FULL);
            } else if (calcStatGet(unit, 3, STAT3_REVIVE_HALF) != 0) {
                amount = unit->stats->maxHp / 2;
                calcStatReset(unit, 3, STAT3_REVIVE_HALF);
                calcStatAltReset(unit, 3, STAT3_REVIVE_HALF);
            }
        }
        if (amount != 0) {
            command.command = UNIT_CMD_PHASE_2B_WAIT;
            unitQueueAppend(unit, &command);
            calcHpAdjust(unit, amount);
            unit->flags &= ~UNIT_FLAG_DOWN;
            shown = -amount;
            if (amount >= 0) {
                shown = amount;
            }
            func_00AA4DB8(unit, shown, 0x802, 0);
        }
    }
    return 1;
}

extern struct MonsSlot *ov01StreamSlot;

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
