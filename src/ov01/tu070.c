/*
 * OV01 original TU 70: 0x00aa9058..0x00aa90c8 (3 functions)
 */
#include "common.h"
#include "ov01/tu069.h"
#include "ov01/tu070.h"

/*
 * Partial layout of one HandleSlot record (tu069's table): the state word is its
 * first word (HandleSlot.inUse, typed int here); the other 8 bytes are not used by these functions.
 */
struct HandleSlotState {
    int state;
    unsigned char unmodeled_04[8];
};

/* Handle state words, indexed by handle - HANDLE_BASE. */
extern struct HandleSlotState D_00AE0F20[];

/* Sets the state word of handle to 1 and returns the byte offset of its record. */
int handleStateSet(int handle) {
    int handleIndex;
    int slotOffset;

    handleIndex = handle - HANDLE_BASE;
    slotOffset = handleIndex * HANDLE_RECORD_SIZE;
    D_00AE0F20[handleIndex].state = 1;
    return slotOffset;
}

/* Returns the state word of handle. */
int handleStateGet(unsigned int handle) {
    return D_00AE0F20[handle - HANDLE_BASE].state;
}

int seVoiceStartDefault(int);

/* Starts the SE voice `code` tagged with voiceTag (the owner's handle, in the top byte). */
int seVoiceStartTagged(int voiceTag, int code) {
    return seVoiceStartDefault((voiceTag << SE_VOICE_TAG_SHIFT) + code);
}
