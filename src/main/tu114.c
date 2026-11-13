#include "common.h"
#include "shared.h"
#include "main/tu114.h"
#include "main/xenoActor.h"

int msgChoicePoll(void);

typedef struct MapModeBlock {
    u8 unmodeled_0000[0x4084];
    int mode;
} MapModeBlock;

typedef struct MapObject {
    u8 unmodeled_000[0x330];
    void *handle;
    MapModeBlock *modeBlock;
    u8 unmodeled_338[0x570 - 0x338];
    u32 flags;
} MapObject;

/*
 * Global block whose word at offset 8 is set to 0x8000 after a successful
 * menu, slot or message-window update (mapObjectBeginMessage and the natives in tu113).
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

void XenoActorSetMode(struct XenoActor *actor, int value);
extern WindowState gameWork;

void mapObjectBeginMessage(MapObject *object)
{
    if (object != 0) {
        XenoActorEnableChannel(object->handle);
        XenoActorSetMode(object->handle, 0);
        gameWork.flags |= 0x8000;
    }
}

extern MapObject *mapActiveObject;

void mapObjectSetActive(int unused_arg0, MapObject *value)
{
    mapActiveObject = value;
}

int msgSlotTestCurrent(int slot);
float xglAtan2Angle(float x, float y);

typedef struct MapActorPose {
    u8 unmodeled_00[0x10];
    float x;
    u8 unmodeled_14[0x18 - 0x14];
    float z;
    u8 unmodeled_1C[0x64 - 0x1C];
    float heading;
} MapActorPose;

extern float mapHeadingOffset;

void mapObjectFaceActive(MapObject *object)
{
    if (object != 0 && mapActiveObject != 0) {
        mapActiveObject->flags |= 0x400400;
        if (msgSlotTestCurrent(16) == 0) {
            /* The active object turns to face `object`: heading = offset - atan2 of the pose difference. */
            MapActorPose *targetPose = object->handle;
            MapActorPose *activePose = mapActiveObject->handle;
            ((MapActorPose *)mapActiveObject->handle)->heading = mapHeadingOffset - xglAtan2Angle(targetPose->x - activePose->x, targetPose->z - activePose->z);
        }
        if (msgSlotTestCurrent(8) == 0) {
            XenoActorEnableChannel(mapActiveObject->handle);
            XenoActorSetMode(mapActiveObject->handle, mapActiveObject->modeBlock->mode);
        }
        mapActiveObject = 0;
    }
}

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

void talkStateReset(void)
{
    mapActiveObject = 0;
    talkResult = -1;
}

void talkSetResult(int value)
{
    talkResult = value;
}
