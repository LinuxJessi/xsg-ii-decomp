#include "common.h"
#include "shared.h"
#include "main/tu110.h"
#include "main/tu114.h"
#include "main/tu148.h"
#include "main/xenoActor.h"
#include "main/tu051.h"
#include "main/tu048.h"


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
extern void *talkNativeTable[];
extern void *g2pathNativeTable[];
extern void *segNativeTable[];
void func_A9CDB0(void *target, void *name);

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

void mapObjectBeginMessage(MapObject *object)
{
    if (object != 0) {
        XenoActorEnableChannel(object->handle);
        XenoActorSetMode(object->handle, 0);
        gameWork.control |= CONTROL_SCRIPTED;
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

void mapObjectFaceActive(MapObject *object)
{
    if (object != 0 && mapActiveObject != 0) {
        mapActiveObject->flags |= 0x400400;
        if (msgSlotTestCurrent(16) == 0) {
            /* The active object turns to face `object`: heading = offset - atan2 of the pose difference. */
            MapActorPose *targetPose = object->handle;
            MapActorPose *activePose = mapActiveObject->handle;
            ((MapActorPose *)mapActiveObject->handle)->heading = 1.5707963705062866f - xglAtan2Angle(targetPose->x - activePose->x, targetPose->z - activePose->z);
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

void getCitizenFlag();
void getQuestFlag();
void menuAddItem();
void menuDeptGetResult();
void menuDeptQuest();
void menuGetResult();
void msgDtl();
void objx_0022D540();
void objx_0022D548();
void resume();
void segAddrOnGetFlag();
void segAddrOnMessage();
void setCitizenFlag();
void setQuestFlag();
void slotWinOpen();
void smsg();
void talk_0022D170();
void talk_0022D1C8();
void talk_0022D4F0();
void talk_0022D550();
void talk_0022D568();

extern char D_0069D7A8[];
extern char D_0069D7A0[];
extern char D_0069D798[];
extern char D_0069D790[];
extern char D_0068ADB0[];
extern char D_0069D788[];
extern char D_0068ADA0[];
extern char D_0068AD90[];
extern char D_0068AD80[];
extern char D_0069D780[];
extern char D_0068AD70[];
extern char D_0068AD60[];
extern char D_0068AD50[];
extern char D_0068AD38[];
extern char D_0068AD28[];
extern char D_0068AD10[];
extern char D_0068ADF0[];
extern char D_0068ADE0[];
extern char D_0068ADD0[];
extern char D_0068ADC0[];
extern char D_0069D7B0[];
extern char D_0068AE00[];

void *talkNativeTable[34] = {
    (void *) D_0069D7A8,
    talk_0022D170,
    (void *) D_0069D7A0,
    talk_0022D1C8,
    (void *) D_0069D798,
    smsg,
    (void *) D_0069D790,
    msgDtl,
    (void *) D_0068ADB0,
    talk_0022D4F0,
    (void *) D_0069D788,
    resume,
    (void *) D_0068ADA0,
    objx_0022D540,
    (void *) D_0068AD90,
    objx_0022D548,
    (void *) D_0068AD80,
    talk_0022D550,
    (void *) D_0069D780,
    talk_0022D568,
    (void *) D_0068AD70,
    menuAddItem,
    (void *) D_0068AD60,
    menuGetResult,
    (void *) D_0068AD50,
    menuDeptQuest,
    (void *) D_0068AD38,
    menuDeptGetResult,
    (void *) D_0068AD28,
    slotWinOpen,
    (void *) D_0068AD10,
    slotWinGetResult,
    0,
    0
};

void *g2pathNativeTable[10] = {
    (void *) D_0068ADF0,
    setQuestFlag,
    (void *) D_0068ADE0,
    getQuestFlag,
    (void *) D_0068ADD0,
    setCitizenFlag,
    (void *) D_0068ADC0,
    getCitizenFlag,
    0,
    0
};

void *segNativeTable[6] = {
    (void *) D_0069D7B0,
    segAddrOnMessage,
    (void *) D_0068AE00,
    segAddrOnGetFlag,
    0,
    0
};

/* .sdata of this file (0x69D7B8-0x69D7D0): the module names of scriptRegisterNatives. */
unsigned char D_0069D7B8[8] = "talk";

unsigned char D_0069D7C0[8] = "g2path";

unsigned char D_0069D7C8[8] = "seg";
