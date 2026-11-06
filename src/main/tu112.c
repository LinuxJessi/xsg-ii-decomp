#include "common.h"
#include "shared.h"
#include "main/tu148.h"

void cursorBufferTick(void *);
void *cursorBufferGetCurrent(void);

/* Script native talk.winClose (table 0x3ec888): closes the message window. */
int talk_0022D4F0(void) {
    msgWindowCloseForTalk();
    return 1;
}

/* Script native resume (table 0x3ec890). */
int resume(void) {
    void *record = cursorBufferGetCurrent();

    if (record != 0) {
        /* 0x34: offset of the member the callee receives */
        cursorBufferTick(record + 0x34);
    }
    return 1;
}

/* Script native objx.enableFlags (table 0x3ec898): the body only returns 1. */
int objx_0022D540(void) {
    return 1;
}

/* Script native objx.disableFlags (table 0x3ec8a0): the body only returns 1. */
int objx_0022D548(void) {
    return 1;
}

/* Script native talk.isDispMsg (table 0x3ec8a8): the body only calls msgWindowInUse and returns nothing, so the name is not borne out here. */
void talk_0022D550(void) {
    msgWindowInUse();
}

/* Script native talk.wait (table 0x3ec8b0): *out is set to 1 when msgWindowInUse returns non-zero. */
int talk_0022D568(int unused_ctx, int *out) {
    if (msgWindowInUse() != 0) {
        *out = 1;
    }
    return 1;
}
