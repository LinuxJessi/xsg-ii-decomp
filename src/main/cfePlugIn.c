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
#include "main/sdv_033.h"
#include "main/xenoActor.h"
#include "main/xgl_2.h"
#include "main/xgl_cd.h"
#include "main/tu167.h"
#include "cfeEvaluate.h"

extern unsigned char cfeAssertFile[];

/* The modes of func_0024B0E8 / func_0024B230: set the key plain or flagged (0x2), or plain and mark the
 * list's tail to reset its first (0x8), second (0x10) or both direction pairs (cfeEntryResetDirs). */
#define CFE_KEY_MODE_PLAIN 0
#define CFE_KEY_MODE_FLAGGED 1
#define CFE_KEY_MODE_RESET_BOTH 2
#define CFE_KEY_MODE_RESET_FIRST 3
#define CFE_KEY_MODE_RESET_SECOND 4
#define CFE_KEY_RESET_FIRST 0x8
#define CFE_KEY_RESET_SECOND 0x10

CfeListNode *cfeListTail(CfeListNode *node);
void func_0024FC90(CfeListNode *list);
extern unsigned char D_0068CD10[];

/* Sets key `key` of the value list to `value` in a CFE_KEY_MODE_* mode: plain (func_0024F698) or
 * flagged (func_0024F730), or plain with reset flags on the list's tail, which is then updated
 * (func_0024FC90). */
void func_0024B0E8(CfeListNode **list, int mode, int key, float value)
{
    CfeListNode *tail;

    switch (mode) {
    case CFE_KEY_MODE_PLAIN:
        func_0024F698(list, key, value);
        break;
    case CFE_KEY_MODE_FLAGGED:
        func_0024F730(list, key, value);
        break;
    case CFE_KEY_MODE_RESET_BOTH:
        func_0024F698(list, key, value);
        tail = cfeListTail(*list);
        if (tail != 0) {
            tail->flags |= CFE_KEY_RESET_FIRST | CFE_KEY_RESET_SECOND;
            func_0024FC90(*list);
        }
        break;
    case CFE_KEY_MODE_RESET_FIRST:
        func_0024F698(list, key, value);
        tail = cfeListTail(*list);
        if (tail != 0) {
            tail->flags |= CFE_KEY_RESET_FIRST;
            func_0024FC90(*list);
        }
        break;
    case CFE_KEY_MODE_RESET_SECOND:
        func_0024F698(list, key, value);
        tail = cfeListTail(*list);
        if (tail != 0) {
            tail->flags |= CFE_KEY_RESET_SECOND;
            func_0024FC90(*list);
        }
        break;
    default:
        assertFailed(D_0068CD10, cfeAssertFile, 96);
        break;
    }
}

/* Sets key `key` of the angle list to `value` in a CFE_KEY_MODE_* mode: plain (func_0024F7C0) or
 * flagged (func_0024F860), or plain with reset flags on the list's tail, which is then updated
 * (func_0024FC90). */
void func_0024B230(CfeListNode **list, int mode, int key, float value)
{
    CfeListNode *tail;

    switch (mode) {
    case CFE_KEY_MODE_PLAIN:
        func_0024F7C0(list, key, value);
        break;
    case CFE_KEY_MODE_FLAGGED:
        func_0024F860(list, key, value);
        break;
    case CFE_KEY_MODE_RESET_BOTH:
        func_0024F7C0(list, key, value);
        tail = cfeListTail(*list);
        if (tail != 0) {
            tail->flags |= CFE_KEY_RESET_FIRST | CFE_KEY_RESET_SECOND;
            func_0024FC90(*list);
        }
        break;
    case CFE_KEY_MODE_RESET_FIRST:
        func_0024F7C0(list, key, value);
        tail = cfeListTail(*list);
        if (tail != 0) {
            tail->flags |= CFE_KEY_RESET_FIRST;
            func_0024FC90(*list);
        }
        break;
    case CFE_KEY_MODE_RESET_SECOND:
        func_0024F7C0(list, key, value);
        tail = cfeListTail(*list);
        if (tail != 0) {
            tail->flags |= CFE_KEY_RESET_SECOND;
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

extern int cfeGroupCount;

/* A group of cfeGroups (0xC0 bytes): an object, the part table of its pack with the part matrices and
 * one evaluation record per part, and the key lists of the group's own values (+0x64). */
typedef struct CfeGroup {
    XenoActor *object;          /* +0x00 */
    unsigned char *parts;       /* +0x04: the part table of the object's pack; +0x60 is the count */
    Matrix4 *matrices;          /* +0x08: one per part */
    CfeEvalRecord *records;     /* +0x0C: one per part */
    unsigned char unmodeled_10[0x64 - 0x10];
    CfeListNode *lists[4];      /* +0x64 */
    unsigned char unmodeled_74[0xC0 - 0x74];
} CfeGroup;

extern CfeGroup cfeGroups[];

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", func_0024B6D8);

void XenoActorSetModeInSlot(void *, int, int);
float XenoActorSetSelector(void *, int);
int func_A9F0A8(int handle);
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
    if (func_A9F0A8(handle) != 3) {
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

extern int cfeCommandCount;
extern CfeEvalCommand cfeCommands[];

/* The initial scale of a command (1, 1, 1). */
extern const AlignedVector4 D_0068CE40;

/* Script command (2 or 3 arguments): starts the effect of the next command of cfeCommands (page index
 * and offset; the 3-argument form also gives the bank flag) and sets the command's position and
 * rotation to the origin and its scale to D_0068CE40. */
int cfe_0024BD28(int handle)
{
    AlignedVector4 position;
    AlignedVector4 rotation;
    AlignedVector4 scale;
    CfeEvalCommand *command;

    memset(&position, 0, sizeof(position));
    position.w = 1.0f;
    memset(&rotation, 0, sizeof(rotation));
    rotation.w = 1.0f;
    scale = D_0068CE40;
    command = &cfeCommands[cfeCommandCount];
    if (func_A9F0A8(handle) == 2) {
        command->page_index = func_A9F0C0(handle, 1);
        command->page_offset = func_A9F0C0(handle, 2);
        sefStartEffectKeyDefault(0, (command->page_index << CFE_PAGE_SHIFT) | command->page_offset | CFE_BANK_PLAIN);
    } else if (func_A9F0A8(handle) == 3) {
        command->flag = func_A9F0C0(handle, 1);
        command->page_index = func_A9F0C0(handle, 2);
        command->page_offset = func_A9F0C0(handle, 3);
        if (command->flag != 0) {
            sefStartEffectKeyDefault(0, (command->page_index << CFE_PAGE_SHIFT) | command->page_offset
                                     | CFE_BANK_FLAGGED);
        } else {
            sefStartEffectKeyDefault(0, (command->page_index << CFE_PAGE_SHIFT) | command->page_offset
                                     | CFE_BANK_PLAIN);
        }
    }
    QuadwordCopy(&command->position, &position);
    QuadwordCopy(&command->rotation, &rotation);
    QuadwordCopy(command->scale, &scale);
    cfeCommandCount++;
    return 1;
}

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

/* The key lists at +0x130 of each 0x1B0-byte entry of cfeEntries. */
extern CfeListNode *D_006319F0[][108];

/* Script command (5 arguments): sets key `key` of list [index][sub] of D_006319F0 to the 8.8 value.
 * Lists 3 to 5 hold angles (func_0024F860 / func_0024F7C0), the others plain values (func_0024F730 /
 * func_0024F698); the flagged forms are used when `flagged` is set. */
int cfe_0024C500(int handle)
{
    int index;
    int sub;
    int key;
    int flagged;
    int value;

    if (func_A9F0A8(handle) != 5) {
        assertFailed(D_0068CED0, cfeAssertFile, 756);
    }
    index = func_A9F0C0(handle, 1);
    sub = func_A9F0C0(handle, 2);
    key = func_A9F0C0(handle, 3);
    flagged = func_A9F0C0(handle, 4);
    value = func_A9F0C0(handle, 5);
    if (sub >= 3 && sub < 6) {
        if (flagged != 0) {
            func_0024F860(&D_006319F0[index][sub], key, CFE_FIXED_TO_FLOAT(value));
        } else {
            func_0024F7C0(&D_006319F0[index][sub], key, CFE_FIXED_TO_FLOAT(value));
        }
    } else {
        if (flagged != 0) {
            func_0024F730(&D_006319F0[index][sub], key, CFE_FIXED_TO_FLOAT(value));
        } else {
            func_0024F698(&D_006319F0[index][sub], key, CFE_FIXED_TO_FLOAT(value));
        }
    }
    return 1;
}

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

    if (func_A9F0A8(handle) != 5) {
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

    if (func_A9F0A8(handle) != 4) {
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

    if (func_A9F0A8(handle) != 5) {
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

extern CfeListNode *D_0063A54C[][16];

/* Script command (5 arguments): sets key `key` of list [index][sub] of D_0063A54C to the 8.8 value.
 * Lists 3 to 5 hold angles (func_0024F860 / func_0024F7C0), the others plain values (func_0024F730 /
 * func_0024F698); the flagged forms are used when `flagged` is set. */
int cfe_0024CF70(int handle)
{
    int index;
    int sub;
    int key;
    int flagged;
    int value;

    if (func_A9F0A8(handle) != 5) {
        assertFailed(D_0068CED0, cfeAssertFile, 933);
    }
    index = func_A9F0C0(handle, 1);
    sub = func_A9F0C0(handle, 2);
    key = func_A9F0C0(handle, 3);
    flagged = func_A9F0C0(handle, 4);
    value = func_A9F0C0(handle, 5);
    if (sub >= 3 && sub < 6) {
        if (flagged != 0) {
            func_0024F860(&D_0063A54C[index][sub], key, CFE_FIXED_TO_FLOAT(value));
        } else {
            func_0024F7C0(&D_0063A54C[index][sub], key, CFE_FIXED_TO_FLOAT(value));
        }
    } else {
        if (flagged != 0) {
            func_0024F730(&D_0063A54C[index][sub], key, CFE_FIXED_TO_FLOAT(value));
        } else {
            func_0024F698(&D_0063A54C[index][sub], key, CFE_FIXED_TO_FLOAT(value));
        }
    }
    return 1;
}

/* Script command (6 or 5 arguments): sets key `key` of a list of group `group` to the 8.8 value. With
 * 6 arguments the list is source `sub` of the record of part `part` (sources 3 to 5 hold angles),
 * with 5 the group's own list `sub`; the flagged forms are used when `flagged` is set. */
int cfe_0024D1E8(int handle)
{
    int group;
    int part;
    int sub;
    int key;
    int flagged;
    int value;

    if (func_A9F0A8(handle) == 6) {
        group = func_A9F0C0(handle, 1);
        part = func_A9F0C0(handle, 2);
        sub = func_A9F0C0(handle, 3);
        key = func_A9F0C0(handle, 4);
        flagged = func_A9F0C0(handle, 5);
        value = func_A9F0C0(handle, 6);
        /* CfeEvalRecord.source holds the key lists as ints (cfeEvaluate.c samples them through int
         * prototypes), hence the casts. */
        if (sub >= 3 && sub < 6) {
            if (flagged != 0) {
                func_0024F860((CfeListNode **) &cfeGroups[group].records[part].source[sub], key,
                              CFE_FIXED_TO_FLOAT(value));
            } else {
                func_0024F7C0((CfeListNode **) &cfeGroups[group].records[part].source[sub], key,
                              CFE_FIXED_TO_FLOAT(value));
            }
        } else {
            if (flagged != 0) {
                func_0024F730((CfeListNode **) &cfeGroups[group].records[part].source[sub], key,
                              CFE_FIXED_TO_FLOAT(value));
            } else {
                func_0024F698((CfeListNode **) &cfeGroups[group].records[part].source[sub], key,
                              CFE_FIXED_TO_FLOAT(value));
            }
        }
    } else {
        group = func_A9F0C0(handle, 1);
        sub = func_A9F0C0(handle, 2);
        key = func_A9F0C0(handle, 3);
        flagged = func_A9F0C0(handle, 4);
        value = func_A9F0C0(handle, 5);
        if (flagged != 0) {
            func_0024F730(&cfeGroups[group].lists[sub], key, CFE_FIXED_TO_FLOAT(value));
        } else {
            func_0024F698(&cfeGroups[group].lists[sub], key, CFE_FIXED_TO_FLOAT(value));
        }
    }
    return 1;
}

extern CfeListNode *D_0063CDE0[];

/* Script command (4 arguments): sets key `key` of list `index` of D_0063CDE0 to the 8.8 value,
 * with flag 0x2 (func_0024F730) when `flagged` is set. */
int cfe_0024D5E0(int handle)
{
    int index;
    int key;
    int flagged;
    int value;

    if (func_A9F0A8(handle) != 4) {
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

    if (func_A9F0A8(handle) != 5) {
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

    if (func_A9F0A8(handle) != 5) {
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

    if (func_A9F0A8(handle) != 5) {
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

/* One 0x44C-byte message record of D_006316B4 (cfe_0024DBC0 allocates them). */
typedef struct CfeMessageRecord {
    int first;                  /* +0x000: script argument 2 */
    int second;                 /* +0x004: script argument 3 */
    int third;                  /* +0x008: script argument 5 (5-argument form only) */
    char text[0x400];           /* +0x00C */
    char name[0x40];            /* +0x40C: script argument 4 (5-argument form), else empty */
} CfeMessageRecord;

extern CfeMessageRecord *D_006316B4;
extern unsigned char D_0068CF30[];

/* Script command (1 argument): allocates and clears argument 1 message records for D_006316B4. */
int cfe_0024DBC0(int handle)
{
    int count;
    int size;

    if (func_A9F0A8(handle) != 1) {
        assertFailed(D_0068CDE8, cfeAssertFile, 1124);
    }
    count = func_A9F0C0(handle, 1);
    size = count * sizeof(CfeMessageRecord);
    if (count <= 0) {
        return 1;
    }
    D_006316B4 = GameResBufferAlloc(size);
    if (D_006316B4 == 0) {
        assertFailed(D_0068CF30, cfeAssertFile, 1131);
    }
    memset(D_006316B4, 0, size);
    return 1;
}

void cfeMesLayoutText(char *text);
extern int cfePlugInRecordCount;
extern const char D_0068CF38[];

/* Script command (3 or 5 arguments): fills the next message record with the unescaped, laid-out text
 * of argument 1 and arguments 2 and 3; the 5-argument form also sets the name (argument 4) and the
 * third value (argument 5). */
int cfe_0024DC78(int handle)
{
    CfeMessageRecord *record = &D_006316B4[cfePlugInRecordCount];

    if (func_A9F0A8(handle) == 3) {
        memset(record->text, 0, sizeof(record->text));
        cfeMesUnescape(record->text, func_A9F238(handle, func_A9F0C0(handle, 1)));
        cfeMesLayoutText(record->text);
        record->first = func_A9F0C0(handle, 2);
        record->second = func_A9F0C0(handle, 3);
        /* The empty name: the original copies the one byte of the empty string D_0068CF38 (an inlined
         * strcpy). */
        record->name[0] = D_0068CF38[0];
        cfePlugInRecordCount++;
    } else if (func_A9F0A8(handle) == 5) {
        memset(record->text, 0, sizeof(record->text));
        cfeMesUnescape(record->text, func_A9F238(handle, func_A9F0C0(handle, 1)));
        cfeMesLayoutText(record->text);
        record->first = func_A9F0C0(handle, 2);
        record->second = func_A9F0C0(handle, 3);
        cfeMesUnescape(record->name, func_A9F238(handle, func_A9F0C0(handle, 4)));
        record->third = func_A9F0C0(handle, 5);
        cfePlugInRecordCount++;
    }
    return 1;
}

int setMesScr(void)
{
    return 1;
}


int cfe_0024DE20(int handle)
{
    int value;
    if (func_A9F0A8(handle) != 1) {
        assertFailed(D_0068CDE8, cfeAssertFile, 1201);
    }
    value = func_A9F0C0(handle, 1);
    cfeRangeSetMin(value);
    return 1;
}


int cfe_0024DE80(int handle)
{
    int value;
    if (func_A9F0A8(handle) != 1) {
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


int cfe_0024E688(int handle)
{
    if (func_A9F0A8(handle) != 1) {
        assertFailed(D_0068CDE8, cfeAssertFile, 1456);
    }
    sefReleaseOwnedRecords(-1);
    return 1;
}

void fadeStartOut(int);

int cfe_0024E6D8(int handle)
{
    int value;
    if (func_A9F0A8(handle) != 1) {
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
