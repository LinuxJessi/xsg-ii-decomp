#ifndef INCLUDE_MAIN_TU245_H
#define INCLUDE_MAIN_TU245_H

/* Functions of src/main/tu245.c that other files call. */

struct MsgWindow;

void msgWindowDrawListIcons(struct MsgWindow *window);
void msgWindowLoadBotanXtx(void);
void msgWindowUnloadBotanXtx(void);
void textTagTableClear(void);

#endif /* INCLUDE_MAIN_TU245_H */
