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
 * The model entry (DataEntry, ov01/tu008.h) as unitAttachChild reads it: from +0xC the buffer words of its file
 * records, 12 bytes apart. Entries 0 to 3 (the model files) give the new actor's resources; entry index + 4 (the
 * two motions, the callers pass indexes 0 and 1) gives its rsrc[XAct_Rsrc_5].
 */
struct AttachRecord {
    unsigned char unmodeled_00[0xC];
    struct AttachEntry entries[6];
};

struct DataEntry;

/* Attaches a new child actor to the unit from the model entry (read through AttachRecord). */
int unitAttachChild(struct Ov01Object *unit, struct DataEntry *entry, int index);

#endif /* INCLUDE_OV01_TU011_H */
