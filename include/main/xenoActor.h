#ifndef INCLUDE_MAIN_XENOACTOR_H
#define INCLUDE_MAIN_XENOACTOR_H

/* Functions of src/main/xenoActor.c that other files call. */

struct XenoActor;

void XenoActorEnableChannel(struct XenoActor *actor);
void XenoActorEnableChannelB(struct XenoActor *actor);
void XenoActorPoolInitDefault(void);
void XenoActorReleaseAll(void);
void XenoActorSetChannelValue(struct XenoActor *actor, float value);
void XenoActorSetChannelValueB(struct XenoActor *actor, float value);
int XenoActorIsDone(struct XenoActor *actor);
void XenoActorApplySourceValue(struct XenoActor *actor);
void XenoActorSetPlayRange(struct XenoActor *actor, int start, int end);
void XenoActorSetFirstWord(struct XenoActor *actor, int value);
void XenoActorSetModeInSlot(struct XenoActor *actor, int slot, int mode);
float XenoActorSetSelector(struct XenoActor *actor, int selector);
void XenoActorSetHoldFlag(void);
void XenoActorClearHoldFlag(void);
void XenoActorDoit(void);
void XenoActorDisp(void);

#endif /* INCLUDE_MAIN_XENOACTOR_H */
