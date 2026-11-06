/*
 * OV01 original TU 70: 0x00aa9058..0x00aa90c8 (3 functions)
 */
#include "common.h"
#include "ov01/tu070.h"

/*
 * Partial layout of one 12-byte handle record (the table of six records that
 * tu069 clears and allocates). Only the state word is read here, the third
 * word of the record, which starts 8 bytes into that table.
 */
struct HandleSlotState {
    int state;
    unsigned char unmodeled_04[8];
};

/* Handle state words, indexed by handle - 3. */
extern struct HandleSlotState D_00AE0F20[];

/* Sets the state word of handle to 1 and returns the byte offset of its record. */
int handleStateSet(int handle) {
    int handleIndex;
    int slotOffset;

    handleIndex = handle - 3;
    slotOffset = handleIndex * 0xC;
    D_00AE0F20[handleIndex].state = 1;
    return slotOffset;
}

/* Returns the state word of handle. */
int handleStateGet(unsigned int handle) {
    return D_00AE0F20[handle - 3].state;
}

int seVoiceStartDefault(int);

int seVoiceStartTagged(int tag, int id) {
    return seVoiceStartDefault((tag << 0x18) + id);
}
