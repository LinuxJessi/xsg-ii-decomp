#ifndef INCLUDE_OV01_TU033_H
#define INCLUDE_OV01_TU033_H

/* Functions of src/ov01/tu033.c that other files call. */

struct Ov01Object;

/* A queued command record: six words, copied whole into the list. */
typedef struct CmdListBody {
    int words[6];
} CmdListBody;

/* Command record kind thinkSendEnd queues: the end of a unit's think step. */
#define CMD_KIND_THINK_END 9

void cmdListSet(void *unit, CmdListBody *command);

/* The unit effects the object starts or stops by effect key (keyHigh, keyLow); the object is the unit of ov01/tu012.h. */
void unitEffectStart(struct Ov01Object *object, int keyHigh, int keyLow);
void func_00A955C8(struct Ov01Object *object);
void func_00A95608(struct Ov01Object *object);
int func_00A95680(struct Ov01Object *object);

#endif /* INCLUDE_OV01_TU033_H */
