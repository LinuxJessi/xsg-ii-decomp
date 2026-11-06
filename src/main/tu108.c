#include "common.h"
#include "main/tu108.h"
#include "main/tu148.h"
#include "main/xgl_cd.h"
#include "main/xgl_flags.h"

INCLUDE_ASM("asm/main/nonmatchings/tu108", saveDataSetDefaults);

extern unsigned char menuLoadPending;

/* Sets the load-pending flag that func_002285C8 reads (proposed name: menuRequestLoad). */
void menuRequestLoad(void) {
    menuLoadPending = 1;
}

extern const char menuMainTxdPath[];
extern const char menuTutorialTxdPath[];
extern unsigned char menuMainTxdBuffer[];
extern unsigned char menuTutorialTxdBuffer[];

/* Reads yosikawa\MainMenu.txd and yosikawa\Tutorial.txd from the disc into their buffers, then polls
 * xglCdServiceIfPending until it returns 0. */
void menuLoadTxds(void) {
    xglCdReadFile(menuMainTxdPath, menuMainTxdBuffer, 1, 0);
    xglCdReadFile(menuTutorialTxdPath, menuTutorialTxdBuffer, 1, 0);
    do {
    } while (xglCdServiceIfPending() != 0);
}

INCLUDE_ASM("asm/main/nonmatchings/tu108", func_002285C8);

/* One 16-byte record of the 10-entry table ptMemberRecords, keyed by its id. */
struct IdSlot {
    int id;
    unsigned char unmodeled_04[12];
};

extern struct IdSlot ptMemberRecords[];

struct IdSlot *ptMemberRecordAt(int index) {
    return &ptMemberRecords[index];
}

/* Returns the first of the 10 slots whose id matches, or NULL. */
struct IdSlot *ptMemberRecordFindById(int id) {
    struct IdSlot *found = 0;
    int entry_index;

    for (entry_index = 0; entry_index < 10; entry_index++) {
        if (ptMemberRecords[entry_index].id == id) {
            found = &ptMemberRecords[entry_index];
            break;
        }
    }
    return found;
}

INCLUDE_ASM("asm/main/nonmatchings/tu108", uwamonoAttachCurrent);

/* The object attached to the current uwamono. id is -1 while nothing is attached. */
struct UwamonoContext {
    unsigned char unmodeled_00[8];
    int id;
    unsigned char unmodeled_0C[20];
    unsigned char state;                /* +0x20 */
};

/* The global binding ptState: the attached object, and the flag copied into its state on attach. */
struct UwamonoBinding {
    struct UwamonoContext *obj;
    unsigned char unmodeled_04[80];
    unsigned char attachState;          /* +0x54 */
};

/* gameWork as used here: its flags word. Shared with other TUs under their own types. */
struct SharedFlags {
    unsigned char unmodeled_00[8];
    int flags;                          /* +0x08; bit 0x800 is set on attach and cleared on detach */
};

extern struct UwamonoBinding ptState;
extern struct SharedFlags gameWork;
void uwamonoSetTransition(int, int, int, int);
void ptEnterSplit();
void ptLeaveSplit();

/* Detaches the current uwamono object: stops its effect, clears its flags and frees the id. */
void uwamonoDetachCurrent(void) {
    if (ptState.obj->id != -1) {
        if (ptState.obj->state != 0) {
            if (ptState.attachState == 0) {
                ptEnterSplit();
            }
        } else if (ptState.attachState != 0) {
            ptLeaveSplit();
        }
        xglFlagsSet1(0x2F0, 0);
        gameWork.flags &= ~0x800;
        uwamonoSetTransition(0, ptState.obj->id, -2, 0);
        msgResetAll();
        ptState.obj->id = -1;
    }
}
