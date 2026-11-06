#include "common.h"
#include "main/tu174.h"

INCLUDE_ASM("asm/main/nonmatchings/tu174", sefSetOverrideVectors);

#include "shared.h"

extern unsigned char sefOverrideFlag[];

u8 *sefOverrideFlagPtr(void) {
    return sefOverrideFlag;
}

/* sefFlagWord is one flag word; sefExecEffect tests bit 1 of it through sefFlagTest. */
extern int sefFlagWord;

void sefFlagSet(int mask) {
    sefFlagWord |= mask;
}

void sefFlagClear(int mask) {
    sefFlagWord &= ~mask;
}

int sefFlagTest(int mask) {
    return sefFlagWord & mask;
}

void sefFlagToggle(int mask) {
    sefFlagWord ^= mask;
}

/* Layout shared with the effect records of tu175: flags at +0x44, enabled at +0x48 (the callee tu185 sefObjectClearFlags reads FlaggedObject.enabled at +0x188; reconcile in the integrator). */
typedef struct FlagRequest {
    u8 unmodeled_00[0x40];
    int request;
    u32 flags;
    int enabled;
} FlagRequest;

void sefObjectClearFlags(void *state, int bit);

void sefClearEnabledFlag(FlagRequest *state) {
    if (state != 0) {
        if (state->enabled != 0) {
            if (state->request != 0) {
                sefObjectClearFlags(state, 0x400000);
            } else {
                state->flags &= ~0x400000;
            }
        }
    }
}

void sefFlagUpdateBit1(int selector) {
    if (selector < 0) {
        sefFlagToggle(1);
        return;
    }
    if (selector > 0) {
        sefFlagSet(1);
        return;
    }
    sefFlagClear(1);
}
