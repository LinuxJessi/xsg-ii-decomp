/*
 * OV01 original TU 11: 0x00a84430..0x00a84830 (3 functions)
 */
#include "common.h"
#include "shared.h"
#include "main/tu051.h"
#include "main/xenoActor.h"
#include "ov01/tu012.h"
#include "ov01/tu023.h"
#include "ov01/tu033.h"

void sefReleaseUnitRecords(struct XenoActor *actor, int mode);
void XenoActorReleaseChild(struct XenoActor *actor, struct XenoActor *child);
void unitRelease(Ov01Object *unit);
int unitTblRemove(Ov01Object *unit);

/*
 * Removes a unit: clears its reset slots, releases its actor and child actors, then
 * clears its scheduler entry and the state's status word when that is 16 or more.
 */
void unitRemove(Ov01Object *unit)
{
    struct XenoActor *child;
    int i;

    for (i = 3; i >= 0; i--) {
        unit->stats->activeMask[i] = 0;
    }
    sefReleaseUnitRecords(unit->unit, 0);
    XenoActorRelease(unit->unit);
    for (i = 0; i < 3; i++) {
        child = unit->children[i];
        if (child != 0) {
            XenoActorReleaseChild(unit->unit, child);
            XenoActorRelease(child);
        }
    }
    unitRelease(unit);
    unitTblRemove(unit);
    if (unit->stats->key >= 16) {
        battleStateClearStatus(unit);
    }
}

/* One 12-byte entry of an attach record; only its first word is read here. */
struct AttachEntry {
    int value;
    unsigned char unmodeled_04[0x8];
};

/*
 * Attach record passed to unitAttachChild: its entries start at +0xC. Entries 0 to 3 give
 * the new actor's resources; entry index + 4 gives its rsrc[XAct_Rsrc_5]. The callers
 * pass indexes 0 and 1, so six entries are modelled.
 */
struct AttachRecord {
    unsigned char unmodeled_00[0xC];
    struct AttachEntry entries[6];
};

/*
 * Source record of unitBindActors: 12-byte entries from +0x14. Entries 0 to 3 give the unit
 * actor's resources and look link, entry 16 its rsrc[XAct_Rsrc_XAP], and entries 4 + 4i to
 * 7 + 4i give child i's; entry 18 + 2i gives child i's rsrc[XAct_Rsrc_XAP].
 */
struct SpawnRecord {
    unsigned char unmodeled_00[0xC];
    unsigned short flags; /* +0xC */
    unsigned char unmodeled_0E[0x14 - 0xE];
    struct AttachEntry entries[24]; /* +0x14 */
};

int XenoActorAllocMatrices(struct XenoActor *actor);
void XenoActorAttachChildToJoint(struct XenoActor *parent, struct XenoActor *child, short joint);
void XenoActorSetMode(struct XenoActor *actor, int mode);
void XenoActorAttachAtSlot(struct XenoActor *parent, struct XenoActor *child, int slot);
void *dataFaceHandleGet(void);

/*
 * Sets up the unit's actor from the spawn record, then its child actors that are present.
 * Each child is attached at a slot or to a joint by its child kind, then the unit's state
 * takes the record's flags.
 */
int unitBindActors(Ov01Object *unit, struct SpawnRecord *spawn)
{
    struct XenoActor *actor;
    struct XenoActor *child;
    int clear_mask;
    int i;

    if (unit == 0 || spawn == 0) {
        return 0;
    }
    actor = unit->unit;
    actor->rsrc[0] = spawn->entries[0].value;
    actor->rsrc[1] = spawn->entries[1].value;
    actor->rsrc[2] = spawn->entries[2].value;
    actor->lookLink = spawn->entries[3].value;
    actor->rsrc[XAct_Rsrc_XAP] = spawn->entries[16].value;
    actor->sharedResource = dataFaceHandleGet();
    XenoActorAllocMatrices(actor);
    XenoActorSetFirstWord(actor, 20);
    XenoActorSetChannelValue(actor, 0.0f);
    XenoActorSetMode(actor, 0);
    for (i = 0; i < 3; i++) {
        child = unit->children[i];
        clear_mask = ~0x4000;
        if (child != 0) {
            child->rsrc[0] = spawn->entries[4 + 4 * i].value;
            child->rsrc[1] = spawn->entries[5 + 4 * i].value;
            child->rsrc[2] = spawn->entries[6 + 4 * i].value;
            child->lookLink = spawn->entries[7 + 4 * i].value;
            child->rsrc[XAct_Rsrc_XAP] = spawn->entries[18 + 2 * i].value;
            XenoActorAllocMatrices(child);
            if (child->rsrc[XAct_Rsrc_XAP] != 0) {
                XenoActorSetChannelValue(child, 0.0f);
                XenoActorSetMode(child, 0);
            }
            if ((unsigned int)(unit->stats->pairFirstUnsigned[i] - 1) < 2) {
                XenoActorAttachAtSlot(unit->unit, child, 27);
            } else {
                XenoActorAttachChildToJoint(unit->unit, child, (short)(unit->stats->pairSecond[i] & clear_mask));
            }
        }
    }
    func_00A954D8(unit, unit->stats->sequenceIndex, 8);
    func_00A95608(unit);
    unit->stats->handle = spawn->flags;
    return 1;
}

/*
 * Attaches a new child actor to the unit's actor from the attach record's entries. The
 * record's entry index + 4 is written to every actor of the unit and to the new one.
 */
int unitAttachChild(Ov01Object *unit, struct AttachRecord *attach, int index)
{
    struct XenoActor *child;
    int i;

    if (unit == 0 || attach == 0) {
        return 0;
    }
    child = unit->unit;
    child->rsrc[XAct_Rsrc_5] = attach->entries[index + 4].value;
    for (i = 0; i < 3; i++) {
        child = unit->children[i];
        if (child != 0) {
            child->rsrc[XAct_Rsrc_5] = attach->entries[index + 4].value;
        }
    }
    if (attach->entries[0].value != 0) {
        child = unit->children[2] = XenoActorAlloc();
        child->rsrc[0] = attach->entries[0].value;
        child->rsrc[1] = attach->entries[1].value;
        child->rsrc[2] = attach->entries[2].value;
        child->lookLink = attach->entries[3].value;
        child->rsrc[XAct_Rsrc_XAP] = 0;
        child->rsrc[XAct_Rsrc_5] = attach->entries[index + 4].value;
        XenoActorAllocMatrices(child);
        XenoActorAttachChildToJoint(unit->unit, child, (short)(unit->stats->pairSecond[2] & 0xBFFF));
    }
    return 1;
}
