#include "common.h"

int func_A9F0A8(int ctx);
int func_A9F0C0(int ctx, int index);
int msgSlotCurrent(void);
void msgSlotSet(int index, int value);
/* Script natives: type of argument index, string of a string token. */
int func_A9F140(int ctx, int index);
char *func_A9F238(int ctx, int handle);
int *func_A9F298(int ctx, int handle);
int strcmp(const char *left, const char *right);
void msgBegin(void);
void msgWindowSetScriptText(int text);
void msgWindowSetSlot(int slot, int value);
void *uwamonoCurrentRecord(void);
void mapObjectBeginMessage(void *object);
void mapObjectFaceActive(void *object);

/* func_A9F140's type of a string argument. */
#define SCRIPT_ARG_STRING 3

/* "NULL" is the entry that ends a script string list. */

/* Script native talk.smsg (table 0x3ec878): begins a message. Argument 1 is one
 * string, or a list of strings ended by "NULL", for the window text; arguments 2
 * and up fill the message slots 1 and up. Clears the open slot's flag word and
 * updates the current map object. */
int smsg(int ctx)
{
    int index;
    int *list;
    int arg;
    void *object;

    /* The argument type is compared as a byte: without the cast gcc drops the andi 0xFF. */
    if ((unsigned char)func_A9F140(ctx, 1) != SCRIPT_ARG_STRING) {
        list = func_A9F298(ctx, func_A9F0C0(ctx, 1));
        msgBegin();
        for (index = 0; strcmp("NULL", func_A9F238(ctx, list[index])) != 0; index++) {
            msgWindowSetScriptText((int)func_A9F238(ctx, list[index]));
        }
    } else {
        msgBegin();
        msgWindowSetScriptText((int)func_A9F238(ctx, func_A9F0C0(ctx, 1)));
    }
    for (arg = 2; arg <= func_A9F0A8(ctx); arg++) {
        msgWindowSetSlot(arg - 1, func_A9F0C0(ctx, arg));
    }
    msgSlotSet(msgSlotCurrent(), 0);
    object = uwamonoCurrentRecord();
    mapObjectBeginMessage(object);
    mapObjectFaceActive(object);
    return 1;
}

/* Store script argument 1 as the flag word of the open message slot (none open:
 * nothing stored). No call or native table entry references it. */
int talkSetSlotFlags(int ctx)
{
    int flags = func_A9F0C0(ctx, 1);

    if (msgSlotCurrent() < 0) {
        return 1;
    }
    msgSlotSet(msgSlotCurrent(), flags);
    return 1;
}

/* Script native talk.msgDtl (table 0x3ec880): talk.smsg with a leading argument.
 * Argument 1 is the flag word stored for the open slot, argument 2 the string or
 * "NULL"-ended string list for the window text, arguments 3 and up fill the
 * message slots 1 and up. */
int msgDtl(int ctx)
{
    int index;
    int *list;
    int arg;
    int flags;
    void *object;

    /* The argument type is compared as a byte, as in smsg. */
    if ((unsigned char)func_A9F140(ctx, 2) != SCRIPT_ARG_STRING) {
        list = func_A9F298(ctx, func_A9F0C0(ctx, 2));
        msgBegin();
        for (index = 0; strcmp("NULL", func_A9F238(ctx, list[index])) != 0; index++) {
            msgWindowSetScriptText((int)func_A9F238(ctx, list[index]));
        }
    } else {
        msgBegin();
        msgWindowSetScriptText((int)func_A9F238(ctx, func_A9F0C0(ctx, 2)));
    }
    for (arg = 3; arg <= func_A9F0A8(ctx); arg++) {
        msgWindowSetSlot(arg - 2, func_A9F0C0(ctx, arg));
    }
    flags = func_A9F0C0(ctx, 1);
    msgSlotSet(msgSlotCurrent(), flags);
    object = uwamonoCurrentRecord();
    mapObjectBeginMessage(object);
    mapObjectFaceActive(object);
    return 1;
}

/* Names of script natives (.sdata, address order). The native tables of tu114.c point at them; no code of this
 * file does. */
char D_0069D780[8] = "wait";
char D_0069D788[8] = "resume";
char D_0069D790[8] = "msgDtl";
char D_0069D798[8] = "smsg";
char D_0069D7A0[8] = "reset";
char D_0069D7A8[8] = "check";
char D_0069D7B0[8] = "addrOn";
char D_0069D7B8[8] = "talk";
char D_0069D7C0[8] = "g2path";
char D_0069D7C8[8] = "seg";
