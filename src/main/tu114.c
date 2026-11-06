#include "common.h"
#include "shared.h"
#include "main/tu114.h"
#include "main/xenoActor.h"

int msgChoicePoll(void);

/*
 * A map object, reached through mapActiveObject (func_0022DE00) and passed by the
 * talk and seg natives (func_0022DDA8) and DispMapObject (mapObjectClearFlags).
 * The word at 0x330 is a handle for XenoActorEnableChannel/XenoActorSetMode in
 * func_0022DDA8, and a subobject pointer in func_0022DE00, which reads float
 * positions at +0x10 and +0x18 through it and writes a heading at +0x64.
 * The flag word at 0x570 gets
 * 0x400400 in func_0022DE00 and loses it in mapObjectClearFlags.
 * Everything else in it is not modelled.
 */
typedef struct MapObject {
    u8 unmodeled_000[0x330];
    void *handle;
    u8 unmodeled_334[0x570 - 0x334];
    u32 flags;
} MapObject;

/*
 * Global block whose word at offset 8 is set to 0x8000 after a successful
 * menu, slot or message-window update (func_0022DDA8 and the natives in tu113).
 */
typedef struct WindowState {
    u8 unmodeled_00[8];
    u32 flags;
} WindowState;

/* Script native slotWinGetResult (table 0x3ec8e0). */
int slotWinGetResult(int unused_arg0, int *failure_flag)
{
    int status;

    status = msgChoicePoll();
    if (status < 0) {
        *failure_flag = 1;
    }
    return status;
}

extern unsigned char D_0069D7B8[];
extern unsigned char D_0069D7C0[];
extern unsigned char D_0069D7C8[];
/* The three tables below hold 8-byte rows: a name pointer, then a native's address. */
extern const char talkNativeTable[];
extern const char g2pathNativeTable[];
extern const char segNativeTable[];
void func_A9CDB0(void *target, const char *name);

/*
 * Registers three native groups with the script VM: "talk" (table 0x3ec868),
 * "g2path" (table 0x3ec8f0) and "seg" (table 0x3ec918); the group names sit at
 * D_0069D7B8, D_0069D7C0 and D_0069D7C8.
 */
void scriptRegisterNatives(void)
{
    func_A9CDB0(D_0069D7B8, talkNativeTable);
    func_A9CDB0(D_0069D7C0, g2pathNativeTable);
    func_A9CDB0(D_0069D7C8, segNativeTable);
}

void XenoActorSetMode(int handle, int value);
extern WindowState gameWork;

void func_0022DDA8(MapObject *object)
{
    if (object != 0) {
        XenoActorEnableChannel(object->handle);
        XenoActorSetMode(object->handle, 0);
        gameWork.flags |= 0x8000;
    }
}

extern int mapActiveObject;

void mapObjectSetActive(int unused_arg0, int value)
{
    mapActiveObject = value;
}

INCLUDE_ASM("asm/main/nonmatchings/tu114", func_0022DE00);

int mapObjectClearFlags(MapObject *object)
{
    int masked;
    int flags;

    masked = object->flags & 0xFFBFFFFF;
    flags = masked & ~0x400;
    object->flags = flags;
    return flags;
}

extern int talkResult;

void func_0022DEE0(void)
{
    mapActiveObject = 0;
    talkResult = -1;
}

void talkSetResult(int value)
{
    talkResult = value;
}
