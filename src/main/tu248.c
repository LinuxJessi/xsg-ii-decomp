#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tu248", cfeDampDirOffset);

INCLUDE_ASM("asm/main/nonmatchings/tu248", func_0028B4A8);

/* Partial view of struct XenoActor (full definition in src/main/xenoActor.c): only the
 * flag word of the channel block at +0x650 and the slot at +0x79C are used here.
 * XenoActorSetChannelFlag80 runs on the XenoActor that unitCreateSide allocates (stored at unit +0x1D0). */
struct XenoActor {
    unsigned char unmodeled_000[0x650];
    int channelFlags;                       /* 0x650: flag word of the channel block */
    unsigned char unmodeled_654[0x79c - 0x654];
    int state;                              /* 0x79C: written here; XenoActorDrawMotion reads it and clears it */
};

void XenoActorSetChannelFlag80(struct XenoActor *actor) {
    actor->channelFlags |= 0x80;
}

void func_0028B568(struct XenoActor *actor, int state) {
    actor->state = state;
}
