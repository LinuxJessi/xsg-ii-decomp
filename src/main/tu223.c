#include "common.h"
#include "shared.h"
#include "main/tu223.h"
#include "main/tu244.h"

void timecodeInit();
void timecodeTickIfActive(void *, int, int, int, int);
void timecodeSetFlag04(void *);
void timecodeSetValueB(void *, int);
void timecodeSetMode(void *, int);
void timecodeSetIndex();
void timecodeSetFrames(void *, long long);

/* Starts the timeline at a frame count (30 per second); returns 0 while the busy bit is set. */
int timecodeStartAtFrame(int *state, int frame) {
    if (*state & 1) {
        return 0;
    }
    timecodeInit();
    timecodeTickIfActive(state, 0xB8, 0xB6, 0x90, 0x54);
    sizedRecordSetValue(state, 0x7FFFFF);
    timecodeSetFrames(state, frame);
    timecodeSetFlag04(state);
    timecodeSetMode(state, 0);
    timecodeSetIndex(state, 4);
    timecodeSetValueB(state, 2);
    return 1;
}

void msgChoiceSetIndex(void) {
    timecodeSetIndex();
}

void func_00285B18();

void func_0027D9D8(void) {
    func_00285B18();
}

void func_00285BF8();

void func_0027D9F0(void) {
    func_00285BF8();
}

long long timecodeGetFrames();

void msgChoiceGetFrames(void) {
    timecodeGetFrames();
}
