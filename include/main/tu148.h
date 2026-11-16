#ifndef INCLUDE_MAIN_TU148_H
#define INCLUDE_MAIN_TU148_H

/* Functions of src/main/tu148.c that other files call. */

/* Flag bits of a message slot (msgSlotFlags, msgSlotTestCurrent). */
#define MSG_SLOT_NO_CIRCLE 0x1       /* CIRCLE does not advance the window */
#define MSG_SLOT_KEEP_GAME_FLAG 0x4  /* closing the slot leaves the game flag set (func_002512B0) */
#define MSG_SLOT_CROSS_CLOSES 0x40   /* CROSS closes the slot */

/* Flag bits of the message window (msgWindowTestCurrentFlags). */
#define MSG_WINDOW_LOCKED 0x4        /* CROSS cannot close it */
#define MSG_WINDOW_TYPING 0x10       /* the text is still being typed; CIRCLE shows the rest at once */
/* msgWindowBusy's mask: bit 0x2 (not recovered), locked or typing. */
#define MSG_WINDOW_BUSY (0x2 | MSG_WINDOW_LOCKED | MSG_WINDOW_TYPING)

/* Flag bit of the choice object (msgChoiceTestFlags): a choice is open. */
#define MSG_CHOICE_OPEN 0x8

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
