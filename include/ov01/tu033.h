#ifndef INCLUDE_OV01_TU033_H
#define INCLUDE_OV01_TU033_H

/* Functions of src/ov01/tu033.c that other files call. */

struct Ov01Object;

/* A unit command: six words, copied whole into the command list (cmdListSet) and into kind-1 events
 * (eventQueueAddKind1). command is its index; the pair command of cmdPairRun names its pair record in pairIndex. */
typedef struct CmdListBody {
    int command;                        /* +0x00 */
    unsigned char unmodeled_04[0x4];
    int code;                           /* +0x08: the key-row entry, technique or argument thinkDispatch passes */
    union {
        struct Ov01Object *target;      /* the target unit of the think commands */
        int flag;                       /* 1 for think command 14 */
    } arg;                              /* +0x0C */
    int pairIndex;                      /* +0x10: the pair record unitPairPartnerFind reads; the battle lookup of
                                         * think command 3 */
    unsigned char unmodeled_14[0x4];
} CmdListBody;

/* Command record kind thinkSendEnd queues: the end of a unit's think step. */
#define CMD_KIND_THINK_END 9

void cmdListSet(void *unit, CmdListBody *command);

/* The unit effects the object starts or stops by effect key (keyHigh, keyLow); the object is the unit of ov01/tu012.h. */
void unitEffectStart(struct Ov01Object *object, int keyHigh, int keyLow);
void func_00A955C8(struct Ov01Object *object);
void func_00A95608(struct Ov01Object *object);
void unitStatusAuraUpdate(struct Ov01Object *object);

#endif /* INCLUDE_OV01_TU033_H */
