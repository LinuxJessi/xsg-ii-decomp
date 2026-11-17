/*
 * OV01 original TU 42: 0x00a9b280..0x00a9c610 (49 functions)
 */
#include "common.h"
#include "shared.h"
#include "main/sef.h"
#include "ov01/tu001.h"
#include "ov01/tu033.h"
#include "ov01/tu035.h"
#include "ov01/tu023.h"
#include "ov01/tu042.h"

extern int dataModelKeyTbl[];

int dataModelKeyGet(int index) {
    return dataModelKeyTbl[index];
}

int func_00A9B298(int value) {
    return value;
}

/* Returns its argument unchanged. Caller: dataVoiceSetLoad. */
int func_00A9B2A0(int value) {
    return value;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9B2A8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9B338);

/* 32-byte record `index` of the self-relative table at D_61D050 (entries numbered from 1), or 0. */
extern int D_61D050;

void *func_00A9B3B8(int index)
{
    unsigned char *table;

    if (index <= 0) {
        return 0;
    }
    table = (unsigned char *)&D_61D050 - 16 + D_61D050;
    return table + (index - 1) * 32;
}

/* tableKeyRowsOffset holds a self-relative offset to a table of 20-byte records; entry numbers start at 1. */
extern int tableKeyRowsOffset;

/* Row of the table for a key: the key is halfword +0x62 of the unit sub record, and
 * the callers read halfword entries of the returned row (tableKeyEntryGet in tu036). */
void *tableKeyRowGet(int key)
{
    unsigned char *table = (unsigned char *)&tableKeyRowsOffset - 20 + tableKeyRowsOffset;

    return table + (key - 1) * 20;
}

/* 6-byte record `index` of the self-relative table at D_61D058 (entries numbered from 1), or 0. */
extern int D_61D058;

void *func_00A9B418(int index)
{
    unsigned char *table;

    if (index <= 0) {
        return 0;
    }
    table = (unsigned char *)&D_61D058 - 24 + D_61D058;
    return table + (index - 1) * 6;
}

/* 12-byte record `index` of the self-relative table at D_61D05C (entries 1..0xF), or 0. */
extern int D_61D05C;

ShortPair *func_00A9B450(int index)
{
    unsigned char *table;

    if (!(index < 0x10)) {
        return 0;
    }
    table = (unsigned char *)&D_61D05C - 28 + D_61D05C;
    return (ShortPair *) (table + (index - 1) * 12);
}

/* 12-byte record `index` of the self-relative table at D_61D060 (entries 1..0x6D), or 0. */
extern int D_61D060;

void *func_00A9B490(int index)
{
    unsigned char *table;

    if (!(index < 0x6E)) {
        return 0;
    }
    table = (unsigned char *)&D_61D060 - 32 + D_61D060;
    return table + (index - 1) * 12;
}

/* 16-byte record `index` of the self-relative table at D_61D068 (entries 1..0x28), or 0. */
extern int D_61D068;

void *func_00A9B4D0(int index)
{
    unsigned char *table;

    if (!(index < 0x29)) {
        return 0;
    }
    table = (unsigned char *)&D_61D068 - 40 + D_61D068;
    return table + (index - 1) * 16;
}

/* 12-byte record `index` of the self-relative table at D_61D064 (entries 1..0x68), or 0. */
extern int D_61D064;

void *func_00A9B508(int index)
{
    unsigned char *table;

    if (!(index < 0x69)) {
        return 0;
    }
    table = (unsigned char *)&D_61D064 - 36 + D_61D064;
    return table + (index - 1) * 12;
}

/* 16-byte record `index` of the self-relative table at D_61D06C (entries 1..0x28), or 0. */
extern int D_61D06C;

void *func_00A9B548(int index)
{
    unsigned char *table;

    if (!(index < 0x29)) {
        return 0;
    }
    table = (unsigned char *)&D_61D06C - 44 + D_61D06C;
    return table + (index - 1) * 16;
}

/* 4-byte record `index` of the self-relative table at D_61D070 (entries 1..0x63), or 0. */
extern int D_61D070;

void *func_00A9B580(int index)
{
    unsigned char *table;

    if (!(index < 0x64)) {
        return 0;
    }
    table = (unsigned char *)&D_61D070 - 48 + D_61D070;
    return table + (index - 1) * 4;
}

/* 8-byte record `index` of the self-relative table at D_61D074 (entries 1..0x63), or 0. */
extern int D_61D074;

void *func_00A9B5B8(int index)
{
    unsigned char *table;

    if (!(index < 0x64)) {
        return 0;
    }
    table = (unsigned char *)&D_61D074 - 52 + D_61D074;
    return table + (index - 1) * 8;
}

/* 16-byte record `index` of the self-relative table at D_61D078 (entries 1..0x10), or 0. */
extern int D_61D078;

void *func_00A9B5F0(int index)
{
    unsigned char *table;

    if (!(index < 0x11)) {
        return 0;
    }
    table = (unsigned char *)&D_61D078 - 56 + D_61D078;
    return table + (index - 1) * 16;
}

/* 16-byte record `index` of the self-relative table at D_61D07C (entries 1..0x15), or 0. */
extern int D_61D07C;

void *func_00A9B628(int index)
{
    unsigned char *table;

    if (!(index < 0x16)) {
        return 0;
    }
    table = (unsigned char *)&D_61D07C - 60 + D_61D07C;
    return table + (index - 1) * 16;
}

/* 16-byte record `index` of the self-relative table at D_61D080 (entries 1..0xA), or 0. */
extern int D_61D080;

void *func_00A9B660(int index)
{
    unsigned char *table;

    if (!(index < 0xB)) {
        return 0;
    }
    table = (unsigned char *)&D_61D080 - 64 + D_61D080;
    return table + (index - 1) * 16;
}

/* Self-relative table at tableKeyRecordsOffset: 16-byte entries, valid for index 16..143. */
extern int tableKeyRecordsOffset;

struct TableKeyRecord *tableKeyRecordGet(int key)
{
    u32 offset = key - 16;
    unsigned char *table;
    void *result;

    if (!(offset < 0x80U)) {
        return 0;
    }
    table = (unsigned char *)&tableKeyRecordsOffset - 68;
    result = &table[tableKeyRecordsOffset] + key * 16 - 256;
    return result;
}

/* Base of the self-relative table at unitExpTableOffset. */
extern int unitExpTableOffset;

UnitExpEntry *unitExpTblGet(void) {
    return (UnitExpEntry *)((u8 *)&unitExpTableOffset - 72 + unitExpTableOffset);
}

/* 32-byte record `index` of the self-relative table at D_61D08C (entries 1..0xF), or 0. */
extern int D_61D08C;

GrowthRecord *func_00A9B6E8(int index)
{
    unsigned char *table;

    if (!(index < 0x10)) {
        return 0;
    }
    table = (unsigned char *)&D_61D08C - 76 + D_61D08C;
    return (GrowthRecord *) (table + (index - 1) * 32);
}

/* 2-byte record `index` of the self-relative table at D_61D090 (entries numbered from 1), or 0. */
extern int D_61D090;

short *func_00A9B720(int index)
{
    unsigned char *table;

    if (index <= 0) {
        return 0;
    }
    table = (unsigned char *)&D_61D090 - 80 + D_61D090;
    return (short *) (table + (index - 1) * 2);
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9B750);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9B7D8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9B850);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9B8D8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9B950);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9B9C8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9BA40);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9BAB8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9BB30);

/* Message object made by msgPutObject: +0x4 release callback (msgObjRelease), +0xC ref count, +0x10 mode, +0x14 handle. */
struct MsgObject {
    unsigned char unmodeled_00[0xC];
    int ref_count;
    int mode;
    int handle;
};

/* The string table: record 1 of the table at D_61D090 is its offset list; each offset counts from the list. */
int tableStringGet(int index, int mode)
{
    int base;

    base = (int)func_00A9B720(1);
    if (mode == 1) {
        return base + *(short *)(((index - 1) << 1) + base);
    }
    return 0;
}

/* Message globals: D_00AC51B0 is the two-word request block {arg, 0} that msgPutObject passes to textEntryAlloc;
 * msgBusy is the busy flag (1 while a message is outstanding). */
extern int D_00AC51B0;
extern int msgBusy;

/* Clears the message request block and the busy flag. */
void msgReset(void) {
    D_00AC51B0 = 0;
    msgBusy = 0;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9BC20);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9BCC8);

/* thinkDispatch: switch over the think opcode (argument 2, 38 cases) on the unit's +0x144 sub-struct;
 * shared by natives thinkSet and thinkSet2. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu042", thinkDispatch);

/* thinkSendEnd: queues the think-end record for the unit (cmdListSet), then ends the
 * target selection mode (tgtSelectModeEnd); used by thinkSet and thinkSetEnd. */
void thinkSendEnd(Ov01Object *unit)
{
    CmdListBody command;

    command.words[0] = CMD_KIND_THINK_END;
    cmdListSet(unit, &command);
    tgtSelectModeEnd();
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9BF08);

/* Field `field` of row `index` of the attack table (native atkTblGet). */
int atkTblFieldGet(int field, AtkTbl *table, int index)
{
    /* index[table->rows] rather than table->rows[index]: the plain form swaps the operands of the addu that forms
     * the row address (0x00A9C084 and 0x00A9C090); only the reversed form keeps the original order. */
    switch (field) {
    case 1:
        return index[table->rows].key;
    case 2:
        return index[table->rows].value;
    }
    return table->count;
}

/* unitParaFieldGet: field reader over the unit's +0x144 sub-struct, same 38 field ids as unitParaFieldSet; native unitParaGet. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu042", unitParaFieldGet);

/* unitParaFieldSet: field writer over the unit's +0x144 sub-struct (ids 0..37); native unitParaSet. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu042", unitParaFieldSet);

void *memset();

/* Effect request keyed by (high, low), head taken from the owner when one is given; used by native createEffect. */
void effectSendOwned(int high, int low, StatOwner *owner) {
    struct EffectRequest request;

    memset(&request, 0, sizeof(request));
    if (owner != 0) {
        request.head = owner->unit;
    }
    request.key = (high << 0xC) | low;
    sefStartEffectDefault(&request);
}


/* Effect request keyed by (high, low) that carries a value; used by native createEffectPos. */
void effectSendValue(int high, int low, Vector4 *position) {
    struct EffectRequest request;

    memset(&request, 0, sizeof(request));
    request.key = (high << 0xC) | low;
    sefStartEffectWithValue(&request, (int) position);
}

/* The 32-byte technique record of the func_00A9B3B8 table as the native
 * tecParaGet reads it (fields 0 to 3). */
typedef struct TecPara {
    unsigned char para0;               /* +0x00: tecParaGet field 0 */
    unsigned char unmodeled_01[0x05];
    signed char para1;                 /* +0x06: tecParaGet field 1 */
    signed char para2;                 /* +0x07: tecParaGet field 2 */
    unsigned char unmodeled_08[0x04];
    unsigned short para3;              /* +0x0C: tecParaGet field 3 */
    unsigned char unmodeled_0e[0x20 - 0x0E];
} TecPara;

/* Field `field` of technique `index` (native tecParaGet); 0 for technique 0
 * or another field. */
int tecParaFieldGet(int index, int field)
{
    TecPara *tec;

    if (index == 0) {
        return 0;
    }
    tec = func_00A9B3B8(index);
    switch (field) {
    case 0:
        return tec->para0;
    case 1:
        return tec->para1;
    case 2:
        return tec->para2;
    case 3:
        return tec->para3;
    }
    return 0;
}

/* msgPutObject (core of native msgPut): makes a MsgObject from ov01TaskClaim, installs msgObjRelease as its
 * release callback, submits the request block through textEntryAlloc and sets the busy flag. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu042", msgPutObject);

u8 textEntryPlayVoice(void *entry);
void objRemove(int handle);

/* Release callback of a MsgObject (stored at +0x4 by msgPutObject). A mode-1 object is freed when
 * textEntryPlayVoice reports its handle done; otherwise the reference count is dropped. The busy flag follows. */
void msgObjRelease(struct MsgObject *object) {
    int handle;
    int remaining;
    int remove;

    remove = 0;
    handle = object->handle;
    if (object->mode == 1) {
        if (textEntryPlayVoice(handle) != 0) {
            remove = 1;
        }
    } else {
        remaining = object->ref_count - 1;
        object->ref_count = remaining;
        remove = remaining < 1;
    }
    if (remove == 1) {
        objRemove(handle);
        ov01TaskRelease(object);
        msgBusy = 0;
        return;
    }
    msgBusy = 1;
}

/* 1 when no message is outstanding (native msgPutWait). */
int msgIsIdle(void) {
    return msgBusy == 0;
}

/* Reads one of the three bytes of gauge `id` (slot order as in partyDataSetSlot); 0 for another slot. */
int partyDataGetSlot(int id, int slot)
{
    struct BoostGauge *gauge;

    gauge = boostGaugeSelect(id);
    switch (slot) {
    case 0:
        return gauge->stock;
    case 1:
        return gauge->progress;
    case 2:
        return gauge->maxStock;
    }
    return 0;
}

/* Writes one of the three slot bytes of record `id`; slot 0 is stock, slot 1 progress, slot 2 maxStock. */
void partyDataSetSlot(int id, int slot, int value) {
    struct BoostGauge *gauge;

    gauge = boostGaugeSelect(id);
    switch (slot) {
    case 0:
        gauge->stock = (signed char) value;
        return;
    case 1:
        gauge->progress = (signed char) value;
        break;
    case 2:
        gauge->maxStock = (signed char) value;
        break;
    }
}
