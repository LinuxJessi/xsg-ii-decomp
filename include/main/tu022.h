#ifndef INCLUDE_MAIN_TU022_H
#define INCLUDE_MAIN_TU022_H

/* Functions of src/main/tu022.c that other files call. */

int func_001A4010(void);
void func_001A4400(int flag);
void soundApplySettings(void);
void soundSetMuted(int muted);
void soundSlotSetLevel(int index, int value);
int soundSlotSetVolume(int index, int level, int extra);
void soundSlotStop(int index);
int soundSlotsRestart(void);

#endif /* INCLUDE_MAIN_TU022_H */
