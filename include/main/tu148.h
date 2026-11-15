#ifndef INCLUDE_MAIN_TU148_H
#define INCLUDE_MAIN_TU148_H

/* Functions of src/main/tu148.c that other files call. */

unsigned int msgWindowInUse(void);
void msgChoiceOpen(int frame, int second);
int msgChoicePoll(void);
void * msgPollSelect(void);
void msgPanelShow(int first, int second);
void msgResetAll(void);
void msgStateReset(void);
int msgWindowBusy(void);
void msgWindowCloseForTalk(void);
int msgWindowHasMore(void);

#endif /* INCLUDE_MAIN_TU148_H */
