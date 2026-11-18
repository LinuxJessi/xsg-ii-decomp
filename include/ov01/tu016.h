#ifndef INCLUDE_OV01_TU016_H
#define INCLUDE_OV01_TU016_H

#include "ov01/tu012.h"

/* Functions of src/ov01/tu016.c that other files call: the unit pose, goal and move commands. */

void objPosePublish(Ov01Object *object);
void objPoseSetSteps(Ov01Object *object, int mode, int unusedFlag);
int objPoseStep(Ov01Object *object);
int objPoseHeadingUpdate(Ov01Object *object, int faceAway);
void objMotionCopyLookup(Ov01Object *object, int action);
float unitPoseRamp(int index, int reverse);
int func_00A86FF0(Ov01Object *object);
void unitCmdDstSet(Ov01Object *object, int cmdId, int mode);
int unitCmdRun(Ov01Object *object, int cmdId, int flip);
/* The third argument is unused: unitCmdMoveChaJump and unitCmdMovePosJump (tu014) pass the move flag as the sibling run and hover calls do. */
int unitCmdMoveJump(Ov01Object *object, int phase, int unusedFlag);
int unitCmdHoverFade(Ov01Object *object, int cmdId, int flip);
float unitDefArea(Ov01Object *object);

#endif /* INCLUDE_OV01_TU016_H */
