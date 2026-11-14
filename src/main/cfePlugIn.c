#include "common.h"
#include "shared.h"
#include "quadword.h"
#include "main/cfePlugIn.h"
#include "main/control_entry.h"
#include "main/tu022.h"
#include "main/tu026.h"
#include "main/tu051.h"
#include "main/tu138.h"
#include "main/sef.h"
#include "main/tu144.h"
#include "main/xenoActor.h"
#include "main/xgl_2.h"
#include "main/xgl_cd.h"

extern unsigned char cfeAssertFile[];

CfeListNode *cfeListTail(CfeListNode *node);
void func_0024FC90(CfeListNode *list);
extern unsigned char D_0068CD10[];

/* Sets key `key` of the value list to `value` by mode: 0 plain (func_0024F698), 1 flagged (func_0024F730), 2 to 4
 * plain with flag 0x18, 0x8 or 0x10 on the list's tail, which is then updated (func_0024FC90). */
void func_0024B0E8(CfeListNode **list, int mode, int key, float value)
{
    CfeListNode *tail;

    switch (mode) {
    case 0:
        func_0024F698(list, key, value);
        break;
    case 1:
        func_0024F730(list, key, value);
        break;
    case 2:
        func_0024F698(list, key, value);
        tail = cfeListTail(*list);
        if (tail != 0) {
            tail->flags |= 0x18;
            func_0024FC90(*list);
        }
        break;
    case 3:
        func_0024F698(list, key, value);
        tail = cfeListTail(*list);
        if (tail != 0) {
            tail->flags |= 0x8;
            func_0024FC90(*list);
        }
        break;
    case 4:
        func_0024F698(list, key, value);
        tail = cfeListTail(*list);
        if (tail != 0) {
            tail->flags |= 0x10;
            func_0024FC90(*list);
        }
        break;
    default:
        assertFailed(D_0068CD10, cfeAssertFile, 96);
        break;
    }
}

/* Sets key `key` of the angle list to `value` by mode: 0 plain (func_0024F7C0), 1 flagged (func_0024F860), 2 to 4
 * plain with flag 0x18, 0x8 or 0x10 on the list's tail, which is then updated (func_0024FC90). */
void func_0024B230(CfeListNode **list, int mode, int key, float value)
{
    CfeListNode *tail;

    switch (mode) {
    case 0:
        func_0024F7C0(list, key, value);
        break;
    case 1:
        func_0024F860(list, key, value);
        break;
    case 2:
        func_0024F7C0(list, key, value);
        tail = cfeListTail(*list);
        if (tail != 0) {
            tail->flags |= 0x18;
            func_0024FC90(*list);
        }
        break;
    case 3:
        func_0024F7C0(list, key, value);
        tail = cfeListTail(*list);
        if (tail != 0) {
            tail->flags |= 0x8;
            func_0024FC90(*list);
        }
        break;
    case 4:
        func_0024F7C0(list, key, value);
        tail = cfeListTail(*list);
        if (tail != 0) {
            tail->flags |= 0x10;
            func_0024FC90(*list);
        }
        break;
    default:
        assertFailed(D_0068CD10, cfeAssertFile, 140);
        break;
    }
}

void xglMatrixStackRotX(float angle);
void xglMatrixStackRotY(float angle);
void xglMatrixStackSave(Matrix4 matrix);
void xglVectorMulMat(Vector4 *destination, Matrix4 matrix, const Vector4 *vector);
extern const AlignedVector4 D_0068CD70;

/* The vector D_0068CD70 rotated by angles[0] about X and angles[1] about Y. */
void cfeRotateVectorXY(Vector4 *result, const float *angles)
{
    Matrix4 matrix;
    AlignedVector4 vector = D_0068CD70;

    xglMatrixStackUnit();
    xglMatrixStackRotX(angles[0]);
    xglMatrixStackRotY(angles[1]);
    xglMatrixStackSave(matrix);
    xglVectorMulMat(&vector, matrix, &vector);
    QuadwordCopy(result, &vector);
}

/* Number of characters of `text` equal to `character`; 0 for no text. */
int cfeStrLastCharPos(const char *text, char character)
{
    int count = 0;

    if (text == 0) {
        return 0;
    }
    while (*text != 0) {
        if (*text == character) {
            count++;
        }
        text++;
    }
    return count;
}

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfeMesLayoutText);

/* Copies a message, turning each "\\n" into a newline; another escaped character is dropped (its place
 * is skipped). */
void cfeMesUnescape(char *destination, const char *source)
{
    while (*source != 0) {
        if (*source == '\\') {
            if (source[1] == 'n') {
                *destination = '\n';
            }
            source += 2;
            destination++;
        } else {
            *destination++ = *source++;
        }
    }
    *destination = 0;
}

typedef struct XenoActor XenoActor;
XenoActor *XenoActorAlloc(void);
void xglCullingMapSet(int map);
extern unsigned char cfeResLoadAssertMsg[];

/* The resource block the cfe object loaders read. */
typedef struct CfeResourceBlock {
    int pack;
    int effect;
    int hierarchy;
    int lookLink;
    int slotTable;
} CfeResourceBlock;

/* Loads resource `id` (waiting for the CD first), creates an actor for it with the block's pack and
 * effect and sets the pack as the culling map. */
XenoActor *cfeLoadResourceObject(int id)
{
    void *data;
    CfeResourceBlock *resource;
    XenoActor *actor;

    data = 0;
    do {
    } while (xglCdServiceIfPending() != 0);
    if (GameResLoadID(id, &data, 0, 0) == -1) {
        assertFailed(cfeResLoadAssertMsg, cfeAssertFile, 283);
    }
    resource = data;
    actor = XenoActorAlloc();
    /* The id goes to XenoActor.kind as a pointer store: through the member (alias set 0) the pack load
     * below cannot move above it as in the original (0.91, also with a local for the pack). */
    *(void **) &actor->kind = (void *) id;
    actor->pack = resource->pack;
    actor->effect = resource->effect;
    if (resource->pack != 0) {
        xglCullingMapSet(resource->pack);
    }
    return actor;
}

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", func_0024B6D8);

void XenoActorSetModeInSlot(void *, int, int);
float XenoActorSetSelector(void *, int);
int func_A9F0A8(void);
int func_A9F0C0(int, int);
char *func_A9F238(int, int);
char *strcpy(char *, const char *);
extern unsigned char D_0068CE20[];
extern unsigned char D_0068CDE8[];
void *GameResBufferAlloc(int size);
void *memset(void *, int, unsigned int);

void XenoActorAllocMatrices(XenoActor *actor);

/* The head of an actor's hierarchy as cfeLoadActorObject reads it. */
typedef struct CfeHierarchyHeader {
    unsigned char unmodeled_00[0x1C];
    int needsShared;            /* +0x1C: set when the hierarchy needs the shared resource */
} CfeHierarchyHeader;

/* The shared resource the hierarchy loads (0x30041). */
#define CFE_SHARED_RESOURCE_ID 0x30041

/* Loads resource `id` as a full actor: pack, effect, hierarchy, slot table and look link from the
 * block (draw mode 6). A hierarchy that needs it also gets the shared resource. The matrices are
 * built and both channels start at 0. */
XenoActor *cfeLoadActorObject(int id)
{
    void *data;
    CfeResourceBlock *resource;
    XenoActor *actor;
    int pack;

    data = 0;
    do {
    } while (xglCdServiceIfPending() != 0);
    if (GameResLoadID(id, &data, 0, 0) == -1) {
        assertFailed(cfeResLoadAssertMsg, cfeAssertFile, 346);
    }
    resource = data;
    actor = XenoActorAlloc();
    do {
    } while (xglCdServiceIfPending() != 0);
    pack = resource->pack;
    actor->drawMode = 6;
    actor->pack = pack;
    actor->kind = id;
    actor->effect = resource->effect;
    actor->hierarchy = resource->hierarchy;
    actor->slotTable = resource->slotTable;
    actor->lookLink = resource->lookLink;
    if (actor->hierarchy != 0 && ((CfeHierarchyHeader *) actor->hierarchy)->needsShared != 0) {
        do {
        } while (xglCdServiceIfPending() != 0);
        GameResLoadID(CFE_SHARED_RESOURCE_ID, &data, 0, 0);
        actor->sharedResource = data;
        XenoActorSetChannelValueB(actor, 0.0f);
        XenoActorSetSelector(actor, 0);
    }
    XenoActorAllocMatrices(actor);
    XenoActorSetChannelValue(actor, 0.0f);
    XenoActorSetChannelValueB(actor, 0.0f);
    XenoActorSetModeInSlot(actor, 0, 0);
    return actor;
}

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfe_0024BA70);

int GameResLoadName(char *name, void **data, unsigned int *size, void (*callback)(int index, void *buffer, unsigned int size, int value));

/* One 0x1B0-byte entry; word 0 is the pointer to its object. The rest of
   the entry is not recovered. */
typedef struct CfePlugInEntry {
    unsigned char *object;
    unsigned char unmodeled_004[0x1B0 - 4];
} CfePlugInEntry;

extern CfePlugInEntry cfePlugInSlots[];

/* Offset of the int result table inside the object; the element is added
   as index * 4. The object's layout is not recovered. */
#define CFE_OBJECT_RESULTS_OFFSET 0x5FC

int cfe_0024BC10(int handle)
{
    char buffer[0x80];
    void *data;
    int slot;
    int index;
    int kind;
    CfePlugInEntry *entry;
    unsigned char *object;

    data = 0;
    if (func_A9F0A8() != 3) {
        assertFailed(D_0068CE20, cfeAssertFile, 430);
    }
    slot = func_A9F0C0(handle, 1);
    index = func_A9F0C0(handle, 2);
    kind = func_A9F0C0(handle, 3);
    strcpy(buffer, func_A9F238(handle, kind));
    GameResLoadName(buffer, &data, 0, 0);
    do {
    } while (xglCdServiceIfPending() != 0);
    if (data != 0) {
        entry = &cfePlugInSlots[slot];
        object = entry->object;
        *(int *) (object + CFE_OBJECT_RESULTS_OFFSET + index * 4) = (int) data;
        if (index == 0) {
            XenoActorSetModeInSlot(object, 0, 0);
            XenoActorSetSelector(entry->object, 0);
        }
    }
    return 1;
}

extern unsigned char D_0068CF10[];
extern unsigned char D_0068CED0[];

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfe_0024BD28);

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfe_0024BEF0);

void sprintf(char *, unsigned char *, char *);
extern int D_0063A868[];
extern unsigned char D_0068CE50[];

int cfe_0024C030(int handle)
{
    char buffer[0x40];
    void *data;
    int slot;
    char *text;

    data = 0;
    slot = func_A9F0C0(handle, 1);
    text = func_A9F238(handle, func_A9F0C0(handle, 2));
    sprintf(buffer, D_0068CE50, text);
    GameResLoadName(buffer, &data, 0, 0);
    do {
    } while (xglCdServiceIfPending() != 0);
    D_0063A868[slot * 0x30] = (int) data;
    return 1;
}

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfe_0024C0D8);

int ldDev(void)
{
    return 1;
}

void seBgmNameSet(void *);

int cfe_0024C278(int handle)
{
    int count;
    CfeSndRecord *record;
    char *text;

    text = func_A9F238(handle, func_A9F0C0(handle, 1));
    strcpy(cfeSndRecords[cfeSndRecordCount].name, text);
    count = cfeSndRecordCount;
    record = &cfeSndRecords[count];
    count = count + 1;
    cfeSndRecordCount = count;
    seBgmNameSet(record);
    return 1;
}

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfe_0024C308);

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfe_0024C500);

extern CfeListNode *D_00631A54[][108];

/* Script command (5 arguments): sets key `key` of list [index][sub] of D_00631A54
 * (the lists at +0x194 of each 0x1B0-byte entry of cfeEntries) to the 8.8 value, with flag
 * 0x2 (func_0024F730) when `flagged` is set. */
int cfe_0024C7B8(int handle)
{
    int index;
    int sub;
    int key;
    int flagged;
    int value;

    if (func_A9F0A8() != 5) {
        assertFailed(D_0068CED0, cfeAssertFile, 795);
    }
    index = func_A9F0C0(handle, 1);
    sub = func_A9F0C0(handle, 2);
    key = func_A9F0C0(handle, 3);
    flagged = func_A9F0C0(handle, 4);
    value = func_A9F0C0(handle, 5);
    if (flagged != 0) {
        func_0024F730(&D_00631A54[index][sub], key, CFE_FIXED_TO_FLOAT(value));
    } else {
        func_0024F698(&D_00631A54[index][sub], key, CFE_FIXED_TO_FLOAT(value));
    }
    return 1;
}

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfe_0024C940);

typedef struct XglCamera XglCamera;
void xglStudioGetCamera(XglCamera **camera, int index);

/* The studio block (0x70 bytes, cleared by cfeStudioInit): the camera and its key lists. */
typedef struct CfeStudioBlock {
    XglCamera *camera;
    CfeListNode *lists[27];
} CfeStudioBlock;

extern CfeStudioBlock D_006384C0;

/* Script command (4 arguments): fetches the studio camera and sets key `key` of camera list `index`
 * to the 8.8 value in `mode`; lists 3 to 5 go through func_0024B230, the others through
 * func_0024B0E8. */
int cfe_0024CB80(int handle)
{
    int index;
    int mode;
    int key;
    int value;

    if (func_A9F0A8() != 4) {
        assertFailed(D_0068CF10, cfeAssertFile, 856);
    }
    index = func_A9F0C0(handle, 1);
    key = func_A9F0C0(handle, 2);
    mode = func_A9F0C0(handle, 3);
    value = func_A9F0C0(handle, 4);
    xglStudioGetCamera(&D_006384C0.camera, 0);
    if (index >= 3 && index < 6) {
        func_0024B230(&D_006384C0.lists[index], mode, key, CFE_FIXED_TO_FLOAT(value));
    } else {
        func_0024B0E8(&D_006384C0.lists[index], mode, key, CFE_FIXED_TO_FLOAT(value));
    }
    return 1;
}

extern CfeListNode *D_0063857C[][32];

/* Script command (5 arguments): sets key `key` of list [index][sub] of D_0063857C to the 8.8 value.
 * Lists 3 to 5 hold angles (func_0024F860 / func_0024F7C0), the others plain values (func_0024F730 /
 * func_0024F698); the flagged forms are used when `flagged` is set. */
int cfe_0024CCF8(int handle)
{
    int index;
    int sub;
    int key;
    int flagged;
    int value;

    if (func_A9F0A8() != 5) {
        assertFailed(D_0068CED0, cfeAssertFile, 896);
    }
    index = func_A9F0C0(handle, 1);
    sub = func_A9F0C0(handle, 2);
    key = func_A9F0C0(handle, 3);
    flagged = func_A9F0C0(handle, 4);
    value = func_A9F0C0(handle, 5);
    if (sub >= 3 && sub < 6) {
        if (flagged != 0) {
            func_0024F860(&D_0063857C[index][sub], key, CFE_FIXED_TO_FLOAT(value));
        } else {
            func_0024F7C0(&D_0063857C[index][sub], key, CFE_FIXED_TO_FLOAT(value));
        }
    } else {
        if (flagged != 0) {
            func_0024F730(&D_0063857C[index][sub], key, CFE_FIXED_TO_FLOAT(value));
        } else {
            func_0024F698(&D_0063857C[index][sub], key, CFE_FIXED_TO_FLOAT(value));
        }
    }
    return 1;
}

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfe_0024CF70);

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfe_0024D1E8);

extern CfeListNode *D_0063CDE0[];

/* Script command (4 arguments): sets key `key` of list `index` of D_0063CDE0 to the 8.8 value,
 * with flag 0x2 (func_0024F730) when `flagged` is set. */
int cfe_0024D5E0(int handle)
{
    int index;
    int key;
    int flagged;
    int value;

    if (func_A9F0A8() != 4) {
        assertFailed(D_0068CF10, cfeAssertFile, 1021);
    }
    index = func_A9F0C0(handle, 1);
    key = func_A9F0C0(handle, 2);
    flagged = func_A9F0C0(handle, 3);
    value = func_A9F0C0(handle, 4);
    if (flagged != 0) {
        func_0024F730(&D_0063CDE0[index], key, CFE_FIXED_TO_FLOAT(value));
    } else {
        func_0024F698(&D_0063CDE0[index], key, CFE_FIXED_TO_FLOAT(value));
    }
    return 1;
}

extern CfeListNode *D_0063ACB4[][140];

/* Script command (5 arguments): sets key `key` of list [index][sub] of D_0063ACB4
 * (140 per row) to the 8.8 value, with flag 0x2 (func_0024F730) when
 * `flagged` is set. */
int cfe_0024D738(int handle)
{
    int index;
    int sub;
    int key;
    int flagged;
    int value;

    if (func_A9F0A8() != 5) {
        assertFailed(D_0068CED0, cfeAssertFile, 1047);
    }
    index = func_A9F0C0(handle, 1);
    sub = func_A9F0C0(handle, 2);
    key = func_A9F0C0(handle, 3);
    flagged = func_A9F0C0(handle, 4);
    value = func_A9F0C0(handle, 5);
    if (flagged != 0) {
        func_0024F730(&D_0063ACB4[index][sub], key, CFE_FIXED_TO_FLOAT(value));
    } else {
        func_0024F698(&D_0063ACB4[index][sub], key, CFE_FIXED_TO_FLOAT(value));
    }
    return 1;
}

extern CfeListNode *D_0063CE0C[][6];

/* Script command (5 arguments): sets key `key` of list [index][sub] of D_0063CE0C
 * (6 per row) to the 8.8 value, with flag 0x2 (func_0024F730) when
 * `flagged` is set. */
int cfe_0024D8C0(int handle)
{
    int index;
    int sub;
    int key;
    int flagged;
    int value;

    if (func_A9F0A8() != 5) {
        assertFailed(D_0068CED0, cfeAssertFile, 1073);
    }
    index = func_A9F0C0(handle, 1);
    sub = func_A9F0C0(handle, 2);
    key = func_A9F0C0(handle, 3);
    flagged = func_A9F0C0(handle, 4);
    value = func_A9F0C0(handle, 5);
    if (flagged != 0) {
        func_0024F730(&D_0063CE0C[index][sub], key, CFE_FIXED_TO_FLOAT(value));
    } else {
        func_0024F698(&D_0063CE0C[index][sub], key, CFE_FIXED_TO_FLOAT(value));
    }
    return 1;
}

extern CfeListNode *D_0063CE90[][20];

/* Script command (5 arguments): sets key `key` of list [index][sub] of D_0063CE90
 * (20 per row) to the 8.8 value, with flag 0x2 (func_0024F730) when
 * `flagged` is set. */
int cfe_0024DA40(int handle)
{
    int index;
    int sub;
    int key;
    int flagged;
    int value;

    if (func_A9F0A8() != 5) {
        assertFailed(D_0068CED0, cfeAssertFile, 1099);
    }
    index = func_A9F0C0(handle, 1);
    sub = func_A9F0C0(handle, 2);
    key = func_A9F0C0(handle, 3);
    flagged = func_A9F0C0(handle, 4);
    value = func_A9F0C0(handle, 5);
    if (flagged != 0) {
        func_0024F730(&D_0063CE90[index][sub], key, CFE_FIXED_TO_FLOAT(value));
    } else {
        func_0024F698(&D_0063CE90[index][sub], key, CFE_FIXED_TO_FLOAT(value));
    }
    return 1;
}

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfe_0024DBC0);

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfe_0024DC78);

int setMesScr(void)
{
    return 1;
}


int cfe_0024DE20(int handle)
{
    int value;
    if (func_A9F0A8() != 1) {
        assertFailed(D_0068CDE8, cfeAssertFile, 1201);
    }
    value = func_A9F0C0(handle, 1);
    cfeRangeSetMin(value);
    return 1;
}


int cfe_0024DE80(int handle)
{
    int value;
    if (func_A9F0A8() != 1) {
        assertFailed(D_0068CDE8, cfeAssertFile, 1217);
    }
    value = func_A9F0C0(handle, 1);
    cfeRangeSetMax(value);
    return 1;
}

void adxSetSlotFile(int, int);
int soundSlotStatus(int);

void cfeRunAndWait(int value, int id)
{
    soundSlotSetLevel(id, 1);
    soundSlotSetVolume(id, -1, 0);
    adxSetSlotFile(id, value);
    while (soundSlotStatus(id) != 3) {
        xglSleep();
    }
}

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfe_0024DF60);


int cfe_0024E688(void)
{
    if (func_A9F0A8() != 1) {
        assertFailed(D_0068CDE8, cfeAssertFile, 1456);
    }
    sefReleaseOwnedRecords(-1);
    return 1;
}

void fadeStartOut(int);

int cfe_0024E6D8(int handle)
{
    int value;
    if (func_A9F0A8() != 1) {
        assertFailed(D_0068CDE8, cfeAssertFile, 1475);
    }
    value = func_A9F0C0(handle, 1);
    fadeStartOut(value);
    return 1;
}

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfe_0024E738);

void func_A9CDB0(void *, void *);
extern unsigned char D_006316B8[];
extern unsigned char D_0068D060[];

void cfeRegisterPlugIn(void)
{
    func_A9CDB0(D_0068D060, D_006316B8);
}
