#include "common.h"
#include "shared.h"
#include "main/tu223.h"
#include "main/tu240.h"
#include "main/tu241.h"
#include "main/tu244.h"

void timecodeSetFrames(MsgSlot *, long long); /* tu241 */

/* Starts the timeline at a frame count (30 per second); returns 0 while the busy bit is set. */
int timecodeStartAtFrame(MsgSlot *state, int frame) {
    if (state->flags & MSG_SLOT_ACTIVE) {
        return 0;
    }
    timecodeInit(state);
    timecodeTickIfActive(state, 0xB8, 0xB6, 0x90, 0x54);
    sizedRecordSetValue((struct SizedRecord *)state, 0x7FFFFF);
    timecodeSetFrames(state, frame);
    timecodeSetFlag04(state);
    timecodeSetMode(state, 0);
    timecodeSetIndex(state, 4);
    timecodeSetValueB(state, 2);
    return 1;
}

void msgChoiceSetIndex(MsgSlot *slot, int index) {
    timecodeSetIndex(slot, index);
}

/* Forwarders of tu148 (step is +1 or -1; see tu241 for the callees). */
void func_0027D9D8(MsgSlot *choice, int step) {
    timecodeStepPhase(choice, step);
}

void func_0027D9F0(MsgSlot *choice, int step) {
    timecodeStart(choice, step);
}

long long msgChoiceGetFrames(MsgSlot *choice) {
    return timecodeGetFrames(choice);
}
