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
#include "main/tu051.h"
#include "ov01/tu012.h"
#include "ov01/tu023.h"
#include "ov01/tu033.h"
#include "ov01/tu036.h"
#include "ov01/tu066.h"
#include "ov01/tu070.h"

/* cmdSystemInit: clears the command list and fifo, the event table and its counters. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu033", cmdSystemInit);

extern char cmdList[];   /* cmdList: 0x20-byte entries, unit then command body */
extern char cmdFifo[];   /* cmdFifo: fifo control block */

/* cmdListPtrGet: the command fifo and the command list, through two out pointers. */
void cmdListPtrGet(void **fifo_out, void **list_out) {
    *fifo_out = cmdFifo;
    *list_out = cmdList;
}

/* cmdListSet: queues one unit's command record at the fifo's write position. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu033", cmdListSet);

/* cmdListRunHead: runs the head command of the list unless its unit is held; pops it on success. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu033", cmdListRunHead);

/* cmdExecute: runs one command body, a switch on its kind, queueing events on the way. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu033", cmdExecute);

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A92A48);

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A92B10);

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

struct Tu033Owner {
    int key;
    unsigned char unmodeled_04[4];
    int handle;
};

struct SlotEntry {
    unsigned char flags;
    unsigned char unmodeled_01[6];
    signed char slot;
    int index;
    unsigned char unmodeled_0c[0xE];
    short extent;
    signed char state;
};

int func_00A92E30(void *, int, void *);
struct SlotEntry *func_00A9B3B8(int);

int func_00A92D98(struct Tu033Owner *owner, int value) {
    struct SlotEntry *entry;
    int slot;
    int result;

    entry = func_00A9B3B8(owner->handle);
    slot = entry->slot;
    result = slot < 8;
    if (result != 0 && slot >= 0) {
        result = func_00A92E30(owner, value, entry);
    }
    return result;
}

void func_00A92DF0(struct Tu033Owner *owner, int value) {
    func_00A92E30(owner, value, func_00A9B3B8(owner->handle));
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A92E30);

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A93080);

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A93128);

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A93168);

int func_00A951D8(int, void *);

int func_00A93288(struct Tu033Owner *owner) {
    struct SlotEntry *entry;
    int status;

    entry = func_00A9B3B8(owner->handle);
    status = func_00A951D8(owner->key, entry);
    return unitEffectKeyFree(owner->key, status, entry->extent) != 0;
}

struct Tu033Request {
    Ov01Object *object;
    unsigned char unmodeled_04[4];
    int second;
    unsigned char unmodeled_0c[0xC];
    int flags;
};

int func_00AA4DB8(int, int, int, int);

/* func_00A932E0: forwards a request's object, second word and flags to func_00AA4DB8 (fourth argument 0). */
void func_00A932E0(int unused_arg0, struct Tu033Request *request) {
    func_00AA4DB8((int) request->object, request->second, request->flags, 0);
}

/* One 0x30-byte record of eventQueueTable (eventQueueTable); kind is its first word. */
typedef struct EventQueueEntry {
    int kind;
    unsigned char unmodeled_04[0x2C];
} EventQueueEntry;

extern EventQueueEntry eventQueueTable[];
extern int eventQueueCount;
extern int eventQueueCursor;

/* eventQueueNext: the record at the read cursor, or 0 when the queue is drained. */
void *eventQueueNext(void) {
    if (eventQueueCursor < eventQueueCount) {
        return eventQueueTable + eventQueueCursor;
    }
    return 0;
}

/* eventQueueReserve: the next free record (the appenders fill it), or 0 when the table is full. */
void *eventQueueReserve(void) {
    if (eventQueueCount < 0x40) {
        return eventQueueTable + eventQueueCount;
    }
    return 0;
}

#include "shared.h"

/* Slot from func_00A9B3B8; its state byte at +0x1C is compared with 1 in unitAttachRequest. */
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
int dataUnitLoad(int);
int unitEffectSlotAcquire(int);
int unitRemove(Ov01Object *);
int unitAttachChild(int, int, int);
int func_00A93080(int, int);
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
INCLUDE_ASM("asm/nonmatchings/ov01/tu033", eventQueueAddKind1);

/* eventQueueAddKind4: appends a kind-4 record with an 0x10-byte payload. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu033", eventQueueAddKind4);

/* eventQueueAddKind5: appends a kind-5 record with an 0x20-byte payload and one more word. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu033", eventQueueAddKind5);

/* eventQueueAdd: appends a record of the given kind, copying 0x10 bytes when a source is given. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu033", eventQueueAdd);

/* eventQueueRun: calls the handler of each queued record's kind; a failing handler stops the drain. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu033", eventQueueRun);

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
 * D_00AB5AE0. The handler's result is unitQueueIsEmpty's result, which the drain
 * reads (nonzero: the queue is empty and the event is done). The return type stays
 * void: the value the drain reads is the one left by the call.
 */
void eventWaitUnitQueue(struct EventWaitRecord *record) {
    unitQueueIsEmpty(record->object);
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A938A0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A93B70);

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
int unitTryActStock(struct SlotEntry *entry, struct Tu033Request *request) {
    Ov01Object *object;
    int chanceHit;
    int flags;
    int requestFlags;

    object = request->object;
    if (func_00A9B3B8(entry->index)->flags & 8) {
        return 0;
    }
    if (object->stats->key < 0x10) {
        chanceHit = actLevelChanceRoll(object, entry->index) != 0;
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
        chanceHit = actLevelChanceRoll(object, entry->index) != 0;
    }
    if (chanceHit == 1 && calcBoostOk(object, 0) != 0) {
        calcActStockSet(object, 0);
        func_00A954D8(object, 0x140, 7);
        seVoiceStartTagged(2, 1);
    }
    return 1;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A94260);

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A94500);

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A946C0);

/* itemRandomPick: rnd() picks one item from four weighted groups of D_00AB5A80; its name is shown. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu033", itemRandomPick);

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A94C20);

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A94D28);

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A94EB0);

int func_00A94260();

void func_00A95018(int unused_arg0, struct Tu033Owner *owner) {
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

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A95198);

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A951D8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A95268);

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A954D8);

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

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A95C00);

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A95C60);

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

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A95E40);

/*
 * unitAttachRequest (proposed unitAttachRequest): the kind-8 event handler. It attaches
 * the requested unit to the model entry in ov01ModelEntry; when the slot's state is 1 it
 * also attaches the handle that func_00A93080 matches for the request's value.
 */
int unitAttachRequest(Ov01Tu033Request *request)
{
    int id;
    int matchedHandle;

    if (ov01ModelEntry != 0) {
        id = request->id;
        unitAttachChild(id, ov01ModelEntry, 0);
        if (func_00A9B3B8(request->index)->state == 1) {
            matchedHandle = func_00A93080(id, request->value);
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

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A95FB0);

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
                    func_00A954D8(units[i], 0x140, 7);
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

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A96810);

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A968C8);

void *dataStreamOpen(int, int);
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

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A96A70);

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A96AA8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu033", func_00A96E38);
