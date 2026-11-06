/*
 * OV01 original TU 42: 0x00a9b280..0x00a9c610 (49 functions)
 */
#include "common.h"
#include "shared.h"
#include "main/tu166.h"
#include "ov01/tu001.h"
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

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9B3B8);

/* tableKeyRowsOffset holds a self-relative offset to a table of 20-byte records; entry numbers start at 1. */
extern int tableKeyRowsOffset;

/* Row of the table for a key: the key is halfword +0x62 of the unit sub record, and
 * the callers read halfword entries of the returned row (tableKeyEntryGet in tu036). */
void *tableKeyRowGet(int key)
{
    unsigned char *table = (unsigned char *)&tableKeyRowsOffset - 20 + tableKeyRowsOffset;

    return table + (key - 1) * 20;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9B418);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9B450);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9B490);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9B4D0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9B508);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9B548);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9B580);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9B5B8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9B5F0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9B628);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9B660);

/* Self-relative table at tableKeyRecordsOffset: 16-byte entries, valid for index 16..143. */
extern int tableKeyRecordsOffset;

void *tableKeyRecordGet(int key)
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

u8 *unitExpTblGet(void) {
    return (u8 *)&unitExpTableOffset - 72 + unitExpTableOffset;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9B6E8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9B720);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9B750);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9B7D8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9B850);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9B8D8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9B950);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9B9C8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9BA40);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9BAB8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9BB30);

/* Effect request block sent to the main-side effect queue (sefStartEffectDefault / sefStartEffectWithValue). */
struct EffectRequest {
    int head;
    unsigned char unmodeled_04[0x18];
    int key; /* (high << 12) | low, see createEffect */
    unsigned char unmodeled_20[0x18];
};

/* Owner whose request word (+0x1D0) is copied into the head of an effect request. */
struct RequestOwner {
    unsigned char unmodeled_00[0x1D0];
    int request_word;
};

/* Message object made by msgPutObject: +0x4 release callback (msgObjRelease), +0xC ref count, +0x10 mode, +0x14 handle. */
struct MsgObject {
    unsigned char unmodeled_00[0xC];
    int ref_count;
    int mode;
    int handle;
};

int func_00A9B720(int);

int tableStringGet(int index, int mode)
{
    int base;

    base = func_00A9B720(1);
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

/* thinkSendEnd: sends opcode 9 through cmdListSet, then calls tgtSelectModeEnd; used by thinkSet and thinkSetEnd. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu042", thinkSendEnd);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9BF08);

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9C070);

/* unitParaFieldGet: field reader over the unit's +0x144 sub-struct, same 38 field ids as unitParaFieldSet; native unitParaGet. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu042", unitParaFieldGet);

/* unitParaFieldSet: field writer over the unit's +0x144 sub-struct (ids 0..37); native unitParaSet. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu042", unitParaFieldSet);

void *memset();

/* Effect request keyed by (high, low), head taken from the owner when one is given; used by native createEffect. */
void effectSendOwned(int high, int low, struct RequestOwner *owner) {
    struct EffectRequest request;

    memset(&request, 0, sizeof(request));
    if (owner != 0) {
        request.head = owner->request_word;
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

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", func_00A9C328);

/* msgPutObject (core of native msgPut): makes a MsgObject from func_00A802F8, installs msgObjRelease as its
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

INCLUDE_ASM("asm/nonmatchings/ov01/tu042", partyDataGetSlot);

void *func_00A91730(int id);

/* Writes one of the three slot bytes of record `id`; slot 0 is byte 1, slot 1 byte 2, slot 2 byte 0. */
void partyDataSetSlot(int id, int slot, int value) {
    signed char *slots;

    slots = func_00A91730(id);
    switch (slot) {
    case 0:
        slots[1] = (signed char) value;
        return;
    case 1:
        slots[2] = (signed char) value;
        break;
    case 2:
        slots[0] = (signed char) value;
        break;
    }
}
