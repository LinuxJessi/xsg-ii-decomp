#ifndef INCLUDE_OV01_TU022_H
#define INCLUDE_OV01_TU022_H

/* Functions of src/ov01/tu022.c that other files call. */

struct DataObject;
struct Ov01Object;

/* Sound ids played by the slot and cursor code and the battle menu (SE_CURSOR_MOVE is the same id as tu098.c). */
#define SE_CURSOR_MOVE 0x01000001
#define SE_CONFIRM 0x01000002
#define SE_REFUSE 0x01000003
#define SE_MENU_OPEN 0x01000004                 /* battleSelectStep: the command panel opens */
#define SE_MENU_CLOSE 0x01000005                /* battleSelectStep: the command panel is closed with PAD_CROSS */
#define SE_CANCEL 0x01000008

/* Key codes of the key functions: DATA_KEY_CANCEL is the cancel press (PAD_CROSS), DATA_KEY_NONE a key that
 * is not registered, not found or refused. */
enum DataKeyCode {
    DATA_KEY_CANCEL = 20,
    DATA_KEY_NONE = 21
};

/* Data object and state reset: dataObjRemove releases the data object, dataStateReset is the state step of the reset chain (tu037). */
void dataObjDraw(struct DataObject *obj);
void dataObjUpdate(struct DataObject *obj);
void dataObjRemove(void);
void dataStateReset(void);

/* Counters and virtual pad. */
void dataCountersClear(void);
void dataVPadModeSet(int mode);
void dataVPadReset(void);
void dataVPadSet(int virtualPad);
void dataObjCreateAtOrigin(void);

/* State of a command key for the unit (0 usable, 1 shown, 2 hidden; negative skips the icons). */
int dataKeyCheck(struct Ov01Object *unit, int key, void *tgt);

/* Pad, key input and the battle menu panels (battleSelectStep). */
unsigned short dataPadRead(int port);
int dataKeyInputStep(struct Ov01Object *unit);
void func_00A88E98(struct Ov01Object *unit);
void menuWepSpPanelReset(void);
int menuWepSpPanelUpdate(struct Ov01Object *unit);
void menuTargetListCursorReset(void);
int menuTargetListUpdate(struct Ov01Object *unit);

/* Target and entry cursors of the battle menu. */
void dataTargetShow(struct Ov01Object *unit, unsigned char *targetFlags);
void dataTargetHide(void);
void dataObjShow(void);
void dataObjHide(void);
void dataStateSetMode(unsigned int value, int flags);
int dataEntryListInit(void *obj);
int dataEntryCursorStep(struct Ov01Object *unit, int *out);
int dataTargetListInit(struct Ov01Object **selected, struct Ov01Object *target, int flags);
int dataUnitCursorCycle(struct Ov01Object **out, int mode);

#endif /* INCLUDE_OV01_TU022_H */
