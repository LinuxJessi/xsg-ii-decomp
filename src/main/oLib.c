#include "common.h"
#include "main/res_get_path.h"
#include "main/test_128.h"
#include "main/tu026.h"
#include "main/xenoActor.h"

/* Source file name passed with the allocation and free calls. */
extern char D_0068D5E8[];
extern int oLibHandle;
int InstanceOfRgHeapById(int count);
int RgHeapAllocById(int count, int flags, char *file, int line);
void resResetSlotTable(void);
void RgHeapFree(int handle, char *file, int line);

/* Object library setup: the handle for the oLib file goes into oLibHandle. */
void oLibInit(void)
{
    int setup_handle;
    int handle;

    setup_handle = InstanceOfRgHeapById(4);
    InitRgHeap(setup_handle, 0x01000000, 0x01200000);
    handle = RgHeapAllocById(4, 0x800010, D_0068D5E8, 118);
    oLibHandle = handle;
    resInitPathHeader(handle, 0x800000);
}

/* Object library teardown: frees the handle in oLibHandle. */
void oLibExit(void)
{
    resResetSlotTable();
    RgHeapFree(oLibHandle, D_0068D5E8, 134);
}

/* A resource id: RES_KIND_* (bits 16-23) | file number (res_get_path.c). Declared as an enumeration type:
 * an int store of it to the actor's kind is scheduled after the first resource-table load (int loads share
 * its alias set), and tu051.h's one-member union (alias set 0) orders it the same way. */
typedef enum ResourceId { RESOURCE_ID_NONE = 0 } ResourceId;

/* The XenoActor of an object (xenoActor.c); only the fields oLibLoadObject fills, named as in tu051.h. */
struct XenoActor {
    unsigned char unmodeled_000[0xC4];
    int drawMode;                           /* 0x0C4: oLibLoadObject sets it to 1 */
    unsigned char unmodeled_0c8[0x5EC - 0xC8];
    int pack;                               /* 0x5EC: the model pack (xenoActor.c reads it as PxyHeader *) */
    int effect;                             /* 0x5F0 */
    int hierarchy;                          /* 0x5F4: XenoActorAllocMatrices builds the matrices from it */
    unsigned char unmodeled_5f8[4];
    int slotTable;                          /* 0x5FC: XenoActorGetSlotRange's table */
    unsigned char unmodeled_600[0x618 - 0x600];
    int lookLink;                           /* 0x618 */
    unsigned char unmodeled_61c[0x798 - 0x61C];
    ResourceId kind;                        /* 0x798: the resource id the object was loaded from (tu051.h: int) */
};

/* The assert expression of oLibLoadObject: "GameResLoadID(Id, &SrcData, NULL, NULL) ". */
extern char D_0068D5F8[];
int GameResLoadID(int id, void **buffer, unsigned int *size,
                  void (*callback)(int index, void *buffer, unsigned int size, int value));
struct XenoActor *XenoActorAlloc(void);
void XenoActorAllocMatrices(struct XenoActor *actor);
void XenoActorSetModeInSlot(struct XenoActor *actor, int slot, int mode);
float XenoActorSetSelector(struct XenoActor *actor, int selector);

/* Loads the object resource Id (asserted at line 157) and creates the actor from its
 * resource table (pack, effect, hierarchy, lookLink, slotTable) as uwamonoLoadPlayer does. */
void oLibLoadObject(struct XenoActor **object, int Id)
{
    void *SrcData;
    int *values;
    struct XenoActor *actor;

    SrcData = 0;
    if (GameResLoadID(Id, &SrcData, 0, 0) == -1) {
        assertFailed(D_0068D5F8, D_0068D5E8, 157);
    }
    values = SrcData;
    actor = XenoActorAlloc();
    actor->kind = Id;
    actor->pack = values[0];
    actor->drawMode = 1;
    *object = actor;
    actor->effect = values[1];
    actor->hierarchy = values[2];
    actor->slotTable = values[4];
    actor->lookLink = values[3];
    XenoActorAllocMatrices(actor);
    XenoActorSetChannelValue(*object, 0.0f);
    XenoActorSetModeInSlot(*object, 0, 0);
    XenoActorSetChannelValueB(*object, 0.0f);
    XenoActorSetSelector(*object, 0);
}
