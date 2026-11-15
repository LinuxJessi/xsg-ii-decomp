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
#include "main/tu145.h"
#include "main/tu146.h"
#include "main/sdv_033.h"
#include "main/xenoActor.h"
#include "main/xgl_2.h"
#include "main/xgl_cd.h"
#include "main/xgl_studio.h"
#include "main/tu167.h"
#include "cfeEvaluate.h"
#include "main/tu148.h"
#include "main/TiGlobalColl.h"
#include "main/nml_model_set.h"
#include "main/pad.h"
#include "main/res_get_path.h"
#include "main/tu021.h"
#include "main/xgl_font.h"
#include "main/xgl_render.h"

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

extern unsigned char D_0068CD10[];

/* Sets key `key` of the value list to `value` in a CFE_KEY_MODE_* mode: plain (func_0024F698) or
 * flagged (func_0024F730), or plain with reset flags on the list's tail, which is then updated
 * (func_0024FC90). */
void func_0024B0E8(CfeTrackNode **list, int mode, int key, float value)
{
    CfeTrackNode *tail;

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
void func_0024B230(CfeTrackNode **list, int mode, int key, float value)
{
    CfeTrackNode *tail;

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

void *memset(void *destination, int value, unsigned int count);
char *strcpy(char *destination, const char *source);
char *func_002AA7F0(char *destination, const char *source);   /* strcat */
extern const char D_0068CD80[];
extern const char D_0068CD88[];

/* The scratchpad, where each line is laid out before it is appended. */
#define CFE_MES_SCRATCHPAD ((char *) 0x70000000)
/* The size of the scratchpad line and of the laid-out text. */
#define CFE_MES_BUFFER_SIZE 0x400
/* A leading '@' indents the first line by one space. */
#define CFE_MES_INDENT '@'
/* Where the font settings string is printed and its colour. */
#define CFE_MES_SETUP_Y 390
#define CFE_MES_SETUP_COLOR 0xFFFFFF

/* Lays the message text out line by line: each line gets a header byte of 1 and an x offset of
 * minus half its width (0 becomes 1) in front of it, built on the scratchpad and appended to a local
 * buffer, which then replaces the text. */
void cfeMesLayoutText(char *text)
{
    char result[CFE_MES_BUFFER_SIZE];
    char *start;
    int indent = 0;
    char *line;
    char *destination;
    int width;
    int last;

    start = text;
    result[0] = D_0068CD80[0];
    memset(result + 1, 0, sizeof(result) - 1);
    xglFontGetStringWidth(D_0068CD88);
    if (*start == CFE_MES_INDENT) {
        indent = 1;
        text = start + 1;
    }
    do {
        destination = CFE_MES_SCRATCHPAD;
        memset(CFE_MES_SCRATCHPAD, 0, CFE_MES_BUFFER_SIZE);
        if (indent) {
            CFE_MES_SCRATCHPAD[0] = ' ';
            destination = CFE_MES_SCRATCHPAD + 1;
            indent = 0;
        }
        line = destination;
        destination = line + 2;
        for (;;) {
            *destination++ = *text++;
            if (*text == '\n' || *text == 0) {
                *destination = *text;
                break;
            }
        }
        xglFontPrint(0, CFE_MES_SETUP_Y, CFE_MES_SETUP_COLOR, D_0068CD88);
        line[0] = 1;
        width = xglFontGetStringWidth(line);
        line[1] = -1 - width / 2;
        if (line[1] == 0) {
            line[1] = 1;
        }
        func_002AA7F0(result, CFE_MES_SCRATCHPAD);
        last = *text++;
    } while (last != 0);
    strcpy(start, result);
}

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

int GameResLoadID(int id, void **buffer, unsigned int *size,
                  void (*callback)(int index, void *buffer, unsigned int size, int value));

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

/* The model pack of an object as the group and object commands read it (xenoActor.c: PxyHeader). */
typedef struct CfePackHeader {
    unsigned char unmodeled_00[0x60];
    int geometryCount;          /* +0x60 */
    unsigned char unmodeled_64[4];
    int matrixOffset;           /* +0x68: bytes from the pack to its geometry matrices */
    unsigned char unmodeled_6c[0xD0 - 0x6C];
    int subPackOffset;          /* +0xD0: bytes from the pack to its part pack, 0 if none */
} CfePackHeader;

/* A group of cfeGroups (0xC0 bytes): an object, the part table of its pack with the part matrices and
 * one evaluation record per part, and the key lists of the group's own values (+0x64). */
typedef struct CfeGroup {
    XenoActor *object;          /* +0x00 */
    CfePackHeader *parts;       /* +0x04: the part pack of the object's pack (one record per geometry) */
    Matrix4 *matrices;          /* +0x08: one per part */
    CfeEvalRecord *records;     /* +0x0C: one per part */
    unsigned char unmodeled_10[0x64 - 0x10];
    CfeTrackNode *lists[4];      /* +0x64 */
    unsigned char unmodeled_74[0xC0 - 0x74];
} CfeGroup;

extern CfeGroup cfeGroups[];

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", func_0024B6D8);

void XenoActorSetModeInSlot(void *, int, int);
float XenoActorSetSelector(void *, int);
int func_A9F0A8(int handle);
int func_A9F0C0(int, int);
char *func_A9F238(int, int);
extern unsigned char D_0068CE20[];
extern unsigned char D_0068CDE8[];

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

int GameResLoadName(char *name, void **data, unsigned int *size,
                    void (*callback)(int index, void *buffer, unsigned int size, int value));

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

void func_0024B6D8(CfeGroup *group);

/* Script command (1 argument): loads resource argument 1 as the object of the next group; when its
 * pack has a part pack, the group gets it, its matrices and one cleared record per geometry. */
int cfe_0024BEF0(int handle)
{
    CfeGroup *group;
    XenoActor *object;
    CfePackHeader *pack;
    CfePackHeader *parts;
    int offset;
    CfeEvalRecord *records;

    if (func_A9F0A8(handle) != 1) {
        assertFailed(D_0068CDE8, cfeAssertFile, 502);
    }
    object = cfeLoadResourceObject(func_A9F0C0(handle, 1));
    /* XenoActor keeps the pack as an int. */
    pack = (CfePackHeader *) object->pack;
    offset = pack->subPackOffset;
    group = &cfeGroups[cfeGroupCount];
    group->object = object;
    /* subPackOffset is a byte offset from the pack. */
    parts = (CfePackHeader *) ((unsigned char *) pack + offset);
    if (offset != 0) {
        group->parts = parts;
        /* matrixOffset is a byte offset from the part pack. */
        group->matrices = (Matrix4 *) ((unsigned char *) parts + parts->matrixOffset);
        records = GameResBufferAlloc(parts->geometryCount * sizeof(CfeEvalRecord));
        group = &cfeGroups[cfeGroupCount];
        group->records = records;
        memset(records, 0, group->parts->geometryCount * sizeof(CfeEvalRecord));
    } else {
        group->parts = 0;
        group->matrices = 0;
        group->records = 0;
    }
    func_0024B6D8(&cfeGroups[cfeGroupCount]);
    cfeGroupCount++;
    return 1;
}

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

void xglMovieInfoInit(void *movie);
int xglMovieOpen(void *movie, const char *name);
int xglCdGetFileSize(const char *name);
extern int cfeEvalEntryCount;
extern CfeEvalEntry cfeEvalEntries[];
extern char D_0068CE68[];
extern unsigned char D_0068CE80[];

/* The block table of xgl_cd (res_get_path.c: CdSlot) as far as the free-slot walk reads it. */
typedef struct CfeCdSlot {
    void *buffer;
    unsigned int size;
    int owner;                  /* +0x08: -1 ends the walk */
    void *tag;
    void *value;
} CfeCdSlot;

extern CfeCdSlot xglCdSlotTable[];

/* Entries of xglCdSlotTable the walk may visit. */
#define CFE_CD_SLOT_LAST 0x7F

/* Script command (1 argument): opens the movie named by argument 1 in the next entry of
 * cfeEvalEntries: reads the file into a new block (walking the block table when the allocation
 * fails), opens it and allocates its two frame buffers. */
int cfe_0024C0D8(int handle)
{
    CfeEvalEntry *entry = &cfeEvalEntries[cfeEvalEntryCount];
    unsigned int slot;

    strcpy(entry->name, func_A9F238(handle, func_A9F0C0(handle, 1)));
    xglMovieInfoInit(entry);
    do {
    } while (xglCdServiceIfPending() != 0);
    entry->dataSize = xglCdGetFileSize(entry->name);
    entry->data = GameResBufferAlloc(entry->dataSize);
    if (entry->data == 0) {
        /* An empty walk to the first unused slot; the result is not used. */
        for (slot = 1; slot < CFE_CD_SLOT_LAST && xglCdSlotTable[slot + 1].owner != -1; slot++) {
        }
    }
    do {
    } while (xglCdServiceIfPending() != 0);
    xglCdReadFile(entry->name, entry->data, 0, 1);
    entry->opened = 1;
    /* The original's assert macro also tests its message string D_0068CE68. */
    if (xglMovieOpen(entry, 0) != 0 && D_0068CE68 != 0) {
        assertFailed(D_0068CE80, cfeAssertFile, 604);
    }
    entry->frameBuffers[0] = GameResBufferAlloc(entry->bufferQuads * 16 + 0x20);
    entry->frameBuffers[1] = GameResBufferAlloc(entry->bufferQuads * 16 + 0x20);
    cfeEvalEntryCount++;
    return 1;
}

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

void xglFontLoad(int first, int second);
void resUpdatePathSlot(int slot);
void func_001A0920(void);
void cfeInit(void);
void tiGlobalCollClearHandle(void);
void func_001B2724(void);
void XenoActorRelease(struct XenoActor *actor);
void fadeClear(void);
XglCamera *xglStudioGetCamera2(int index);
extern signed char pxyTickScaleDouble;
extern int D_006316A8;
extern int D_006316AC;
extern int cfeEntryCount;

/* gameWork as the scene end sees it: the control word at +0x08 (CONTROL_* in tu051.h). gameWork has no
 * header type; tu069.c keeps its own view as well. */
typedef struct CfeGameWork {
    unsigned char unmodeled_00[8];
    int control;
} CfeGameWork;

extern CfeGameWork gameWork;
/* The position the parked record actors are moved to before they are released. */
extern const AlignedVector4 D_0068CEA0;

/* The uwamonoRecords entries and the two flags this command sets on every used record. */
#define CFE_RECORD_COUNT 256
#define CFE_RECORD_FLAG_400000 0x400000
#define CFE_RECORD_FLAG_800000 0x800000

/* Script command: ends the map scene for a cut scene. Waits for the CD, resets the messages, the
 * font, the path slot and the sound levels, restarts the cut-scene state, releases the actors of the
 * player and part records (moved to D_0068CEA0 first) and marks every used record, then frees all
 * actors and resets the camera and the fade. */
int cfe_0024C308(int handle)
{
    AlignedVector4 parked;
    int i;

    do {
    } while (xglCdServiceIfPending() != 0);
    msgResetAll();
    xglFontLoad(0, 0);
    resUpdatePathSlot(-3);
    func_001A0920();
    D_006316A8 = 0;
    D_006316AC = 0;
    soundSlotSetLevel(0, 0);
    soundSlotSetLevel(1, 0);
    cfeInit();
    cfeEntryCount = 0;
    tiGlobalCollClearHandle();
    pxyTickScaleDouble = 0;
    func_001B2724();
    gameWork.control |= CONTROL_CFE_NO_FADE | CONTROL_CFE_SCENE;
    for (i = 0; i < CFE_RECORD_COUNT; i++) {
        switch (uwamonoRecords[i].kind) {
        case 0:
            break;
        case 4:
        case 5:
            parked = D_0068CEA0;
            QuadwordCopy(&uwamonoRecords[i].actor->position, &parked);
            QuadwordCopy(&uwamonoRecords[i].position, &parked);
            QuadwordCopy(&uwamonoRecords[i].moveFrom, &parked);
            QuadwordCopy(&uwamonoRecords[i].moveTo, &parked);
            XenoActorRelease(uwamonoRecords[i].actor);
            uwamonoRecords[i].actor = 0;
            /* fall through */
        case 1:
        case 2:
        case 3:
        case 6:
        default:
            uwamonoRecords[i].flags |= CFE_RECORD_FLAG_400000;
            uwamonoRecords[i].flags |= CFE_RECORD_FLAG_800000;
            break;
        }
    }
    XenoActorReleaseAll();
    XenoActorPoolInitDefault();
    xglStudioGetCamera2(0)->state = 1;
    fadeClear();
    return 1;
}

/* The key lists at +0x130 of each 0x1B0-byte entry of cfeEntries. */
extern CfeTrackNode *D_006319F0[][108];

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

extern CfeTrackNode *D_00631A54[][108];

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

extern unsigned char D_0068CEF0[];
extern CfeNode cfeEntries[];

/* Script command (6 arguments): sets key `key` of track `column` of flag record `row` of entry `entry`
 * to the 8.8 value (flagged through func_0024F730 when `flagged` is set), then walks that track to its
 * end. The record keeps its track as an int handle. */
int cfe_0024C940(int handle)
{
    int entry;
    int row;
    int column;
    int key;
    int flagged;
    int value;
    CfeTrackNode *node;

    if (func_A9F0A8(handle) != 6) {
        assertFailed(D_0068CEF0, cfeAssertFile, 821);
    }
    entry = func_A9F0C0(handle, 1);
    row = func_A9F0C0(handle, 2);
    column = func_A9F0C0(handle, 3);
    key = func_A9F0C0(handle, 4);
    flagged = func_A9F0C0(handle, 5);
    value = func_A9F0C0(handle, 6);
    if (flagged != 0) {
        func_0024F730((CfeTrackNode **) &cfeEntries[entry].records[row].track[column], key,
                      CFE_FIXED_TO_FLOAT(value));
    } else {
        func_0024F698((CfeTrackNode **) &cfeEntries[entry].records[row].track[column], key,
                      CFE_FIXED_TO_FLOAT(value));
    }
    for (node = (CfeTrackNode *) cfeEntries[entry].records[row].track[column]; node != 0; node = node->next) {
    }
    return 1;
}


/* The studio block (0x70 bytes, cleared by cfeStudioInit): the camera and its key lists. */
typedef struct CfeStudioBlock {
    XglCamera *camera;
    CfeTrackNode *lists[27];
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

extern CfeTrackNode *D_0063857C[][32];

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

extern CfeTrackNode *D_0063A54C[][16];

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
                func_0024F860((CfeTrackNode **) &cfeGroups[group].records[part].source[sub], key,
                              CFE_FIXED_TO_FLOAT(value));
            } else {
                func_0024F7C0((CfeTrackNode **) &cfeGroups[group].records[part].source[sub], key,
                              CFE_FIXED_TO_FLOAT(value));
            }
        } else {
            if (flagged != 0) {
                func_0024F730((CfeTrackNode **) &cfeGroups[group].records[part].source[sub], key,
                              CFE_FIXED_TO_FLOAT(value));
            } else {
                func_0024F698((CfeTrackNode **) &cfeGroups[group].records[part].source[sub], key,
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

extern CfeTrackNode *D_0063CDE0[];

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

extern CfeTrackNode *D_0063ACB4[][140];

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

extern CfeTrackNode *D_0063CE0C[][6];

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

extern CfeTrackNode *D_0063CE90[][20];

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

void adxSetSlotFile(int slot, const char *file);
int soundSlotStatus(int);

void cfeRunAndWait(const char *file, int id)
{
    soundSlotSetLevel(id, 1);
    soundSlotSetVolume(id, -1, 0);
    adxSetSlotFile(id, file);
    while (soundSlotStatus(id) != 3) {
        xglSleep();
    }
}

void func_0028B268(struct XenoActor *actor, Vector4 *target);
void cfeDampDirOffset(struct XenoActor *actor, int offset);
void xglClockSampleDayTime(void);
void xglClockAccumulate(void);
void pauseDrawScreen(void);
void sefExecEffect(void);
void func_0027C7D0(void);
void func_0027BFD0(int slot, int kind, unsigned int value, int count);
void fadeReset(void);
void fadeStartIn(int frames);
void xglFontPrintDirect(const char *text);
void seVolumeSet(int value, int flags);
/* The running fade (nonzero while a fade is under way). */
extern int fadeUpdateHook;
extern PadRecord PadData[];
extern XglLightSet cfeDirectionTable;
extern CfeEvalGlobal cfeEvalGlobal;

/* SaveData as the subtitles read it: +0x3A shows every subtitle (game_movie.c caption_all). */
typedef struct CfeSaveData {
    unsigned char unmodeled_00[0x3A];
    unsigned char captionAll;
} CfeSaveData;

extern CfeSaveData SaveData;
extern const char D_0068CD88[];
extern CfeEvalSlot cfeEvalSlots[3];
int strcmp(const char *first, const char *second);

/* Frames after the start of the cut scene before start can pause it, and the sound of pausing. */
#define CFE_PAUSE_DELAY 31
#define CFE_PAUSE_SOUND 0x2000011
/* Triangle while paused jumps this many frames ahead (past the end of any cut scene). */
#define CFE_SKIP_FRAMES 30000
/* The subtitle line: one line low on the screen, two or more lines higher up. */
#define CFE_SUBTITLE_Y 370
#define CFE_SUBTITLE_Y2 348
#define CFE_SUBTITLE_COLOR 0xFFFFFF
/* The slot value scale for func_0027BFD0; kinds 0x22 and 0x23 take 0.03 of it as kinds 0x1A and 0x1B. */
#define CFE_SLOT_SCALE 16777215.0f
#define CFE_SLOT_SHORT 0.03f


/* Script command (1 argument): one frame of the cut scene at the time in argument 1. Start pauses and resumes
 * (triangle while paused skips 30000 frames ahead), the cut scene is evaluated (camera, directions, nodes,
 * groups, commands, entries, slots, sound) and drawn with its subtitles, and the time to continue with is
 * returned: one frame later unless paused. */
int cfe_0024DF60(int handle)
{
    CfeNode *entry;
    int time;
    int i;
    int y;

    if (func_A9F0A8(handle) != 1) {
        assertFailed(D_0068CDE8, cfeAssertFile, 1250);
    }
    while (xglCdServiceIfPending() != 0) {
    }
    time = func_A9F0C0(handle, 1);
    cfeRangeSetValue(time);
    if (D_006316AC != 0) {
        D_006316AC--;
    }
    if (cfeRangeGetValue() - cfeRangeGetMin() >= CFE_PAUSE_DELAY && fadeUpdateHook == 0
        && (PadData[0].pressed & PAD_START)) {
        D_006316AC = 2;
        D_006316A8 = D_006316A8 == 0;
        seVoiceStartDefault(CFE_PAUSE_SOUND);
        if (D_006316A8 != 0) {
            xglClockSampleDayTime();
        }
    }
    if (cfePlugInRecordCount > 0 && time == cfeRangeGetMin() && strcmp(D_006316B4->name, D_0068CF38) != 0) {
        cfeRunAndWait(D_006316B4->name, D_006316B4->third);
    }
    if (D_006316A8 == 0) {
        func_00248390(time, 0);
    }
    cfeEvaluateDirections(time);
    cfeGlobalEvaluate(time);
    cfeEvaluateNodes(time);
    cfeGroupTableEvaluate(time);
    cfeCommandTableEvaluate(time);
    cfeGlobalEvaluate(time);
    cfeEntryTableEvaluate(time);
    cfeSlotTableEvaluate(time);
    cfeSndSetTarget(time);
    /* PxyShadowLight is nml_model_set.c's view of the light set. */
    pxySetDefaultLight((struct PxyShadowLight *) &cfeDirectionTable);
    if (cfeEvalGlobal.result_int != 0) {
        pxySetGlobalFogDist(cfeEvalGlobal.evaluated[1].x, cfeEvalGlobal.evaluated[1].y, cfeEvalGlobal.evaluated[1].z,
                            cfeEvalGlobal.evaluated[1].w);
        pxySetGlobalFogColor(&cfeEvalGlobal.evaluated[0]);
    }
    for (i = 0; i < cfeEntryCount; i++) {
        entry = &cfeEntries[i];
        if (entry->flags & CFE_NODE_SHOWN) {
            func_0028B268(entry->actor, &entry->position);
        }
        cfeDampDirOffset(entry->actor, entry->dampOffset);
    }
    if (time == cfeRangeGetMin()) {
        fadeReset();
        fadeStartIn(30);
    }
    for (i = 0; i < cfePlugInRecordCount; i++) {
        if (D_006316B4[i].first <= time && time < D_006316B4[i].second && D_006316B4[i].first == time
            && strcmp(D_006316B4[i].name, D_0068CF38) != 0 && D_006316A8 == 0) {
            soundSlotSetLevel(D_006316B4[i].third, 0);
        }
    }
    for (i = 0; i < cfePlugInRecordCount; i++) {
        if (D_006316B4[i].first <= time && time < D_006316B4[i].second
            && (SaveData.captionAll != 0 || D_006316B4[i].text[0] == ' ')) {
            y = CFE_SUBTITLE_Y;
            if (cfeStrLastCharPos(D_006316B4[i].text, '\n') >= 2) {
                y = CFE_SUBTITLE_Y2;
            }
            xglFontPrint(0, y, CFE_SUBTITLE_COLOR, D_0068CD88);
            xglFontPrintDirect(D_006316B4[i].text);
        }
    }
    if (D_006316A8 != 0) {
        xglClockAccumulate();
        func_0027C7D0();
        XenoActorSetHoldFlag();
        soundSlotSetLevel(0, 1);
        soundSlotSetLevel(1, 1);
        seSetMuted(1, 1);
        pauseDrawScreen();
        if ((PadData[0].pressed & PAD_TRIANGLE) && D_006316AC == 0) {
            PadData[0].pressed &= ~PAD_TRIANGLE;
            time += CFE_SKIP_FRAMES;
            soundSlotStop(0);
            soundSlotStop(1);
            soundSlotSetLevel(0, 0);
            soundSlotSetLevel(1, 0);
            seSetMuted(1, 0);
            seVolumeSet(0, 0);
            seVoiceStartDefault(CFE_PAUSE_SOUND);
            sefReleaseOwnedRecords(-1);
            return time;
        }
    } else {
        if (PadData[0].pressed & PAD_START) {
            soundSlotSetLevel(0, 0);
            soundSlotSetLevel(1, 0);
            seSetMuted(1, 0);
            XenoActorClearHoldFlag();
        }
        time++;
    }
    if (PadData[0].pressed & PAD_START) {
        if ((PadData[0].held & PAD_SOFT_RESET_HELD) == PAD_SOFT_RESET_HELD) {
            gameWork.control |= CONTROL_SOFT_RESET;
        }
    }
    for (i = 0; i < 3; i++) {
        if ((unsigned int) (cfeEvalSlots[i].result_int - 1) < 2 && cfeEvalSlots[i].sample_int > 0) {
            fadeClear();
        }
        if (cfeEvalSlots[i].result_int == 0x22) {
            func_0027BFD0(i, 0x1A, cfeEvalSlots[i].sample_float * CFE_SLOT_SCALE * CFE_SLOT_SHORT,
                          cfeEvalSlots[i].sample_int);
        } else if (cfeEvalSlots[i].result_int == 0x23) {
            func_0027BFD0(i, 0x1B, cfeEvalSlots[i].sample_float * CFE_SLOT_SCALE * CFE_SLOT_SHORT,
                          cfeEvalSlots[i].sample_int);
        } else {
            func_0027BFD0(i, cfeEvalSlots[i].result_int, cfeEvalSlots[i].sample_float * CFE_SLOT_SCALE,
                          cfeEvalSlots[i].sample_int);
        }
    }
    cfeUpdateEntries(D_006316A8);
    XenoActorDoit();
    XenoActorDisp();
    if (D_006316A8 == 0) {
        sefExecEffect();
    }
    return time;
}


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

int sceGsSyncPath(int mode, int timeout);
void xglRenderCopyDisp2Draw(void);
int GameResBlockFreeByBuffer(void *buffer);
int GameResBlockFreeByOwner(int owner);
void tiWindDisableAll(void);
void resResetSlotTable(void);
void resResetPathSlots(void);
void func_001BA320(void);
extern int cfeEvalEntryCount;
extern CfeEvalEntry cfeEvalEntries[];
extern int uwamonoCurrentIndex;
extern UwamonoObject uwamonoRecords[];

/* The record flag the cut-scene end clears on the current record. */
#define CFE_RECORD_FLAG_2 0x2

/* Script command: ends a cut scene. Stops both sound slots and the owned effects, copies the shown
 * frame to the drawn one, clears the scene control bits and the current record's flag 0x2, frees the
 * message records, the group and entry buffers, the collision, wind, actors and resource slots, and
 * gives the player and part records new actors. */
int cfe_0024E738(int handle)
{
    int i;
    int current;

    soundSlotStop(0);
    soundSlotStop(1);
    sefReleaseOwnedRecords(-1);
    xglRenderClearDepth();
    sceGsSyncPath(0, 0);
    xglRenderCopyDisp2Draw();
    xglSleep();
    xglSleep();
    xglRenderCopyDisp2Draw();
    current = uwamonoCurrentIndex;
    gameWork.control &= ~CONTROL_CFE_SCENE;
    uwamonoRecords[current].flags &= ~CFE_RECORD_FLAG_2;
    gameWork.control &= ~CONTROL_CFE_NO_FADE;
    if (D_006316B4 != 0) {
        GameResBlockFreeByBuffer(D_006316B4);
        D_006316B4 = 0;
    }
    for (i = 0; i < cfeGroupCount; i++) {
        if (cfeGroups[i].records != 0) {
            GameResBlockFreeByBuffer(cfeGroups[i].records);
        }
        GameResBlockFreeByOwner(cfeGroups[i].object->kind);
    }
    cfeGroupCount = 0;
    for (i = 0; i < cfeEvalEntryCount; i++) {
        if (cfeEvalEntries[i].data != 0) {
            GameResBlockFreeByBuffer(cfeEvalEntries[i].data);
        }
        if (cfeEvalEntries[i].frameBuffers[0] != 0) {
            GameResBlockFreeByBuffer(cfeEvalEntries[i].frameBuffers[0]);
        }
        if (cfeEvalEntries[i].frameBuffers[1] != 0) {
            GameResBlockFreeByBuffer(cfeEvalEntries[i].frameBuffers[1]);
        }
    }
    cfeEvalEntryCount = 0;
    tiGlobalCollClearHandle();
    tiWindDisableAll();
    XenoActorReleaseAll();
    resResetSlotTable();
    resResetPathSlots();
    for (i = 0; i < CFE_RECORD_COUNT; i++) {
        switch (uwamonoRecords[i].kind) {
        case 4:
        case 5:
            uwamonoRecords[i].actor = XenoActorAlloc();
            break;
        }
    }
    xglFontLoad(1, 0);
    func_001BA320();
    return 1;
}

void func_A9CDB0(void *, void *);
extern unsigned char D_006316B8[];
extern unsigned char D_0068D060[];

void cfeRegisterPlugIn(void)
{
    func_A9CDB0(D_0068D060, D_006316B8);
}
