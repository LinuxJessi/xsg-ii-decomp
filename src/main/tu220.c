#include "common.h"
#include "main/tu220.h"

/*
 * The message window (main/tu148, msgWindow) as these functions read it: a
 * table of 0x98-byte records. Word 0 of the table holds the index of the record
 * to check (-1 for none); the flags word at record +0x08 and the state word at
 * record +0x24 are the only other fields these functions read.
 */
struct MsgEntryStatus {
    int flags;
    unsigned char unmodeled_04[0x18];
    int state;
};

struct MsgEntry {
    int current_index;
    unsigned char unmodeled_04[4];
    struct MsgEntryStatus status;
    unsigned char unmodeled_28[0x70];
};

int msgWindowGetFirstWord(int *word)
{
    return *word;
}

void msgWindowRunRecord(struct MsgEntry *window, int index);

/* Steps the current record unless it is already done or paused in state 2. */
void msgWindowUpdateCurrent(struct MsgEntry *window)
{
    int current;
    struct MsgEntryStatus *status;

    current = window[0].current_index;
    if (current < 0) {
        return;
    }
    status = &window[current].status;
    if (status->state == 2) {
        if (status->flags & 0x20) {
            return;
        }
        if (status->flags & 0x100) {
            return;
        }
    }
    msgWindowRunRecord(window, current);
}

INCLUDE_ASM("asm/main/nonmatchings/tu220", msgWindowFindLastActive);

INCLUDE_ASM("asm/main/nonmatchings/tu220", msgWindowCompact);

INCLUDE_ASM("asm/main/nonmatchings/tu220", msgWindowRunRecord);

INCLUDE_ASM("asm/main/nonmatchings/tu220", msgWindowRunAll);
