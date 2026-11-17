#ifndef INCLUDE_OV01_TU011_H
#define INCLUDE_OV01_TU011_H

/* Functions of src/ov01/tu011.c that other files call. */

struct Ov01Object;

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

int unitAttachChild(struct Ov01Object *unit, struct AttachRecord *attach, int index);

#endif /* INCLUDE_OV01_TU011_H */
