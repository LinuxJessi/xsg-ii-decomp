#include "common.h"
#include "shared.h"
#include "quadword.h"
#include "main/cfePlugIn.h"
#include "main/control_entry.h"
#include "main/tu017.h"
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

/* The modes of cfeListSetKeyMode / cfeListSetAngleKeyMode: set the key plain or flagged (0x2), or plain and mark the
 * list's tail to reset its first (0x8), second (0x10) or both direction pairs (cfeEntryResetDirs). */
#define CFE_KEY_MODE_PLAIN 0
#define CFE_KEY_MODE_FLAGGED 1
#define CFE_KEY_MODE_RESET_BOTH 2
#define CFE_KEY_MODE_RESET_FIRST 3
#define CFE_KEY_MODE_RESET_SECOND 4
#define CFE_KEY_RESET_FIRST 0x8
#define CFE_KEY_RESET_SECOND 0x10

extern unsigned char cfeAssertZero[];

/* Sets key `key` of the value list to `value` in a CFE_KEY_MODE_* mode: plain (cfeListSetValue) or
 * flagged (cfeListSetValueLinear), or plain with reset flags on the list's tail, which is then updated
 * (cfeTrackUpdateDirections). */
void cfeListSetKeyMode(CfeTrackNode **list, int mode, int key, float value)
{
    CfeTrackNode *tail;

    switch (mode) {
    case CFE_KEY_MODE_PLAIN:
        cfeListSetValue(list, key, value);
        break;
    case CFE_KEY_MODE_FLAGGED:
        cfeListSetValueLinear(list, key, value);
        break;
    case CFE_KEY_MODE_RESET_BOTH:
        cfeListSetValue(list, key, value);
        tail = cfeListTail(*list);
        if (tail != 0) {
            tail->flags |= CFE_KEY_RESET_FIRST | CFE_KEY_RESET_SECOND;
            cfeTrackUpdateDirections(*list);
        }
        break;
    case CFE_KEY_MODE_RESET_FIRST:
        cfeListSetValue(list, key, value);
        tail = cfeListTail(*list);
        if (tail != 0) {
            tail->flags |= CFE_KEY_RESET_FIRST;
            cfeTrackUpdateDirections(*list);
        }
        break;
    case CFE_KEY_MODE_RESET_SECOND:
        cfeListSetValue(list, key, value);
        tail = cfeListTail(*list);
        if (tail != 0) {
            tail->flags |= CFE_KEY_RESET_SECOND;
            cfeTrackUpdateDirections(*list);
        }
        break;
    default:
        assertFailed(cfeAssertZero, cfeAssertFile, 96);
        break;
    }
}

/* Sets key `key` of the angle list to `value` in a CFE_KEY_MODE_* mode: plain (cfeListSetAngle) or
 * flagged (cfeListSetAngleLinear), or plain with reset flags on the list's tail, which is then updated
 * (cfeTrackUpdateDirections). */
void cfeListSetAngleKeyMode(CfeTrackNode **list, int mode, int key, float value)
{
    CfeTrackNode *tail;

    switch (mode) {
    case CFE_KEY_MODE_PLAIN:
        cfeListSetAngle(list, key, value);
        break;
    case CFE_KEY_MODE_FLAGGED:
        cfeListSetAngleLinear(list, key, value);
        break;
    case CFE_KEY_MODE_RESET_BOTH:
        cfeListSetAngle(list, key, value);
        tail = cfeListTail(*list);
        if (tail != 0) {
            tail->flags |= CFE_KEY_RESET_FIRST | CFE_KEY_RESET_SECOND;
            cfeTrackUpdateDirections(*list);
        }
        break;
    case CFE_KEY_MODE_RESET_FIRST:
        cfeListSetAngle(list, key, value);
        tail = cfeListTail(*list);
        if (tail != 0) {
            tail->flags |= CFE_KEY_RESET_FIRST;
            cfeTrackUpdateDirections(*list);
        }
        break;
    case CFE_KEY_MODE_RESET_SECOND:
        cfeListSetAngle(list, key, value);
        tail = cfeListTail(*list);
        if (tail != 0) {
            tail->flags |= CFE_KEY_RESET_SECOND;
            cfeTrackUpdateDirections(*list);
        }
        break;
    default:
        assertFailed(cfeAssertZero, cfeAssertFile, 140);
        break;
    }
}

void xglMatrixStackRotX(float angle);
void xglMatrixStackRotY(float angle);
void xglMatrixStackSave(Matrix4 matrix);
extern const AlignedVector4 cfeForwardVector;

/* The vector cfeForwardVector rotated by angles[0] about X and angles[1] about Y. */
void cfeRotateVectorXY(Vector4 *result, const float *angles)
{
    Matrix4 matrix;
    AlignedVector4 vector = cfeForwardVector;

    xglMatrixStackUnit();
    xglMatrixStackRotX(angles[0]);
    xglMatrixStackRotY(angles[1]);
    xglMatrixStackSave(matrix);
    xglVectorMulMat(&vector, matrix, &vector);
    QuadwordCopy(result, &vector);
}

/* Number of characters of `text` equal to `character`; 0 for no text. */
int cfeStrCountChar(const char *text, char character)
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
char *strcat(char *destination, const char *source);
extern const char cfeMesEmpty[];
extern const char cfeMesFontSettings[];

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
    result[0] = cfeMesEmpty[0];
    memset(result + 1, 0, sizeof(result) - 1);
    xglFontGetStringWidth(cfeMesFontSettings);
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
        xglFontPrint(0, CFE_MES_SETUP_Y, CFE_MES_SETUP_COLOR, cfeMesFontSettings);
        line[0] = 1;
        width = xglFontGetStringWidth(line);
        line[1] = -1 - width / 2;
        if (line[1] == 0) {
            line[1] = 1;
        }
        strcat(result, CFE_MES_SCRATCHPAD);
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

/* XenoActor.kind (+0x798) as cfeLoadResourceObject writes it: a pointer-typed store outside tu051.h's
 * one-member union (alias set 0), so the pack load after it may move above it as in the original. Through
 * actor->kind (0.91) or an int view field (gate fails) it cannot. */
typedef struct CfeActorKindView {
    unsigned char unmodeled_000[0x798];
    void *kind;
} CfeActorKindView;

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
    ((CfeActorKindView *) actor)->kind = (void *) id;
    actor->pack = resource->pack;
    actor->effect = resource->effect;
    if (resource->pack != 0) {
        xglCullingMapSet(resource->pack);
    }
    return actor;
}

extern int cfeGroupCount;

extern CfeGroup cfeGroups[];

float asinf(float value);
double sqrt(double value);
float atan2f(float y, float x);

/* The record of part `index` with its address computed index first (index * size + records), as the
 * original computes it for the two quadword copies below; `&group->records[index]` adds in the other
 * order (addu operands swapped). */
#define CFE_GROUP_RECORD(group, index) \
    ((CfeEvalRecord *) ((index) * sizeof(CfeEvalRecord) + (int) (group)->records))

/* Starts the records of every part of the group from the part's matrix: the position (w = 1), the Euler
 * angles (x from the pitch row, y from the heading of the third row, z the roll, reflected across pi when the
 * up row points down) and unit scale; both are kept as the initial values. */
void cfeMapInitRecords(CfeGroup *group)
{
    float *matrix;
    int i;

    if (group->parts == 0) {
        return;
    }
    for (i = 0; i < group->parts->geometryCount; i++) {
        matrix = &group->matrices[i][0][0];
        group->records[i].evaluated[0].vector.x = matrix[12];
        group->records[i].evaluated[0].vector.y = matrix[13];
        group->records[i].evaluated[0].vector.z = matrix[14];
        group->records[i].evaluated[0].vector.w = 1.0f;
        group->records[i].evaluated[1].vector.x = asinf(matrix[9]);
        group->records[i].evaluated[1].vector.y = atan2f(matrix[8], matrix[10]);
        if (matrix[5] >= 0.0f) {
            double sine = matrix[1];

            group->records[i].evaluated[1].vector.z = asinf(sine / sqrt(1.0f - matrix[9] * matrix[9]));
        } else {
            double sine = matrix[1];

            group->records[i].evaluated[1].vector.z
                = 3.1415927f - asinf(sine / sqrt(1.0f - matrix[9] * matrix[9]));
        }
        group->records[i].evaluated[1].vector.w = 1.0f;
        group->records[i].evaluated[2].vector.x = 1.0f;
        group->records[i].evaluated[2].vector.y = 1.0f;
        group->records[i].evaluated[2].vector.z = 1.0f;
        group->records[i].evaluated[2].vector.w = 1.0f;
        QuadwordCopy(&CFE_GROUP_RECORD(group, i)->initial[0], &CFE_GROUP_RECORD(group, i)->evaluated[0]);
        QuadwordCopy(&CFE_GROUP_RECORD(group, i)->initial[1], &CFE_GROUP_RECORD(group, i)->evaluated[1]);
    }
}

void XenoActorSetModeInSlot(void *, int, int);
float XenoActorSetSelector(void *, int);
int VM_PLUGIN_CNT_GET(int handle);
int vmArgGet(int, int);
char *vmKeyedEntryGet(int, int);
extern unsigned char cfeArgCount3Assert[];
extern unsigned char cfeArgCount1Assert[];

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

extern unsigned char cfeActorAssert[];
extern int cfeEntryCount;
extern CfeNode cfeEntries[];

/* Script command (1 argument): loads actor argument 1 into the next entry of cfeEntries, copies the play
 * range of its first motion state, sets draw mode 6 and allocates a cleared record block of two words per
 * pack geometry. */
int cfe_0024BA70(int handle)
{
    XenoActor *actor;
    CfePackHeader *pack;
    void *block;

    if (VM_PLUGIN_CNT_GET(handle) != 1) {
        assertFailed(cfeArgCount1Assert, cfeAssertFile, 394);
    }
    cfeEntries[cfeEntryCount].actor = cfeLoadActorObject(vmArgGet(handle, 1));
    if (cfeEntries[cfeEntryCount].actor == 0) {
        assertFailed(cfeActorAssert, cfeAssertFile, 398);
    }
    actor = cfeEntries[cfeEntryCount].actor;
    /* XenoActor keeps the pack as an int. */
    pack = (CfePackHeader *) actor->pack;
    cfeEntries[cfeEntryCount].playStart = actor->motion[0].range[0];
    cfeEntries[cfeEntryCount].playEnd = actor->motion[0].range[1];
    cfeEntries[cfeEntryCount].drawMode = 6;
    if (pack != 0 && pack->geometryCount != 0) {
        block = GameResBufferAlloc(pack->geometryCount * 8);
        cfeEntries[cfeEntryCount].records = block;
        memset(block, 0, pack->geometryCount * 8);
    } else {
        cfeEntries[cfeEntryCount].records = 0;
    }
    cfeEntryCount++;
    return 1;
}

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
    if (VM_PLUGIN_CNT_GET(handle) != 3) {
        assertFailed(cfeArgCount3Assert, cfeAssertFile, 430);
    }
    slot = vmArgGet(handle, 1);
    index = vmArgGet(handle, 2);
    kind = vmArgGet(handle, 3);
    strcpy(buffer, vmKeyedEntryGet(handle, kind));
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

extern unsigned char cfeArgCount4Assert[];
extern unsigned char cfeArgCount5Assert[];

extern int cfeCommandCount;
extern CfeEvalCommand cfeCommands[];

/* The initial scale of a command (1, 1, 1). */
extern const AlignedVector4 cfeUnitScale;

/* Script command (2 or 3 arguments): starts the effect of the next command of cfeCommands (page index
 * and offset; the 3-argument form also gives the bank flag) and sets the command's position and
 * rotation to the origin and its scale to cfeUnitScale. */
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
    scale = cfeUnitScale;
    command = &cfeCommands[cfeCommandCount];
    if (VM_PLUGIN_CNT_GET(handle) == 2) {
        command->page_index = vmArgGet(handle, 1);
        command->page_offset = vmArgGet(handle, 2);
        sefStartEffectKeyDefault(0, (command->page_index << CFE_PAGE_SHIFT) | command->page_offset | CFE_BANK_PLAIN);
    } else if (VM_PLUGIN_CNT_GET(handle) == 3) {
        command->flag = vmArgGet(handle, 1);
        command->page_index = vmArgGet(handle, 2);
        command->page_offset = vmArgGet(handle, 3);
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

    if (VM_PLUGIN_CNT_GET(handle) != 1) {
        assertFailed(cfeArgCount1Assert, cfeAssertFile, 502);
    }
    object = cfeLoadResourceObject(vmArgGet(handle, 1));
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
    cfeMapInitRecords(&cfeGroups[cfeGroupCount]);
    cfeGroupCount++;
    return 1;
}

void sprintf(char *, unsigned char *, char *);
extern int D_0063A868[];
extern unsigned char cfeMotionPathFormat[];

int cfe_0024C030(int handle)
{
    char buffer[0x40];
    void *data;
    int slot;
    char *text;

    data = 0;
    slot = vmArgGet(handle, 1);
    text = vmKeyedEntryGet(handle, vmArgGet(handle, 2));
    sprintf(buffer, cfeMotionPathFormat, text);
    GameResLoadName(buffer, &data, 0, 0);
    do {
    } while (xglCdServiceIfPending() != 0);
    D_0063A868[slot * 0x30] = (int) data;
    return 1;
}

extern int cfeEvalEntryCount;
extern CfeEvalEntry cfeEvalEntries[];
extern char cfeMovieOpenFailedText[];
extern unsigned char cfeMovieOpenAssert[];

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

    strcpy(entry->name, vmKeyedEntryGet(handle, vmArgGet(handle, 1)));
    xglMovieInfoInit(&entry->movie);
    do {
    } while (xglCdServiceIfPending() != 0);
    entry->movie.sourceSize = xglCdGetFileSize(entry->name);
    entry->movie.source = GameResBufferAlloc(entry->movie.sourceSize);
    if (entry->movie.source == 0) {
        /* An empty walk to the first unused slot; the result is not used. */
        for (slot = 1; slot < CFE_CD_SLOT_LAST && xglCdSlotTable[slot + 1].owner != -1; slot++) {
        }
    }
    do {
    } while (xglCdServiceIfPending() != 0);
    xglCdReadFile(entry->name, entry->movie.source, 0, 1);
    entry->movie.loop = 1;
    /* The original's assert macro also tests its message string cfeMovieOpenFailedText. */
    if (xglMovieOpen(&entry->movie, 0) != 0 && cfeMovieOpenFailedText != 0) {
        assertFailed(cfeMovieOpenAssert, cfeAssertFile, 604);
    }
    entry->movie.imageAddress[0] = GameResBufferAlloc(entry->movie.imageQwords * 16 + 0x20);
    entry->movie.imageAddress[1] = GameResBufferAlloc(entry->movie.imageQwords * 16 + 0x20);
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

    text = vmKeyedEntryGet(handle, vmArgGet(handle, 1));
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
void cfeInit(void);
void tiGlobalCollClearHandle(void);
void func_001B2724(void);
void XenoActorRelease(struct XenoActor *actor);
void fadeClear(void);
XglCamera *xglStudioGetCamera2(int index);
extern signed char pxyTickScaleDouble;
extern int cfePaused;
extern int cfeStartDelay;
extern int cfeEntryCount;

/* gameWork as the scene end sees it: the control word at +0x08 (CONTROL_* in tu051.h). gameWork has no
 * header type; tu069.c keeps its own view as well. */
typedef struct CfeGameWork {
    unsigned char unmodeled_00[8];
    int control;
} CfeGameWork;

extern CfeGameWork gameWork;
/* The position the parked record actors are moved to before they are released. */
extern const AlignedVector4 cfeParkedPosition;

/* The uwamonoRecords entries and the two flags this command sets on every used record. */
#define CFE_RECORD_COUNT 256
#define CFE_RECORD_FLAG_400000 0x400000
#define CFE_RECORD_FLAG_800000 0x800000

/* Script command: ends the map scene for a cut scene. Waits for the CD, resets the messages, the
 * font, the path slot and the sound levels, restarts the cut-scene state, releases the actors of the
 * player and part records (moved to cfeParkedPosition first) and marks every used record, then frees all
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
    xglMovieInit();
    cfePaused = 0;
    cfeStartDelay = 0;
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
            parked = cfeParkedPosition;
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
 * Lists 3 to 5 hold angles (cfeListSetAngleLinear / cfeListSetAngle), the others plain values (cfeListSetValueLinear /
 * cfeListSetValue); the flagged forms are used when `flagged` is set. */
int cfe_0024C500(int handle)
{
    int index;
    int sub;
    int key;
    int flagged;
    int value;

    if (VM_PLUGIN_CNT_GET(handle) != 5) {
        assertFailed(cfeArgCount5Assert, cfeAssertFile, 756);
    }
    index = vmArgGet(handle, 1);
    sub = vmArgGet(handle, 2);
    key = vmArgGet(handle, 3);
    flagged = vmArgGet(handle, 4);
    value = vmArgGet(handle, 5);
    if (sub >= 3 && sub < 6) {
        if (flagged != 0) {
            cfeListSetAngleLinear(&D_006319F0[index][sub], key, CFE_FIXED_TO_FLOAT(value));
        } else {
            cfeListSetAngle(&D_006319F0[index][sub], key, CFE_FIXED_TO_FLOAT(value));
        }
    } else {
        if (flagged != 0) {
            cfeListSetValueLinear(&D_006319F0[index][sub], key, CFE_FIXED_TO_FLOAT(value));
        } else {
            cfeListSetValue(&D_006319F0[index][sub], key, CFE_FIXED_TO_FLOAT(value));
        }
    }
    return 1;
}

extern CfeTrackNode *D_00631A54[][108];

/* Script command (5 arguments): sets key `key` of list [index][sub] of D_00631A54
 * (the lists at +0x194 of each 0x1B0-byte entry of cfeEntries) to the 8.8 value, with flag
 * 0x2 (cfeListSetValueLinear) when `flagged` is set. */
int cfe_0024C7B8(int handle)
{
    int index;
    int sub;
    int key;
    int flagged;
    int value;

    if (VM_PLUGIN_CNT_GET(handle) != 5) {
        assertFailed(cfeArgCount5Assert, cfeAssertFile, 795);
    }
    index = vmArgGet(handle, 1);
    sub = vmArgGet(handle, 2);
    key = vmArgGet(handle, 3);
    flagged = vmArgGet(handle, 4);
    value = vmArgGet(handle, 5);
    if (flagged != 0) {
        cfeListSetValueLinear(&D_00631A54[index][sub], key, CFE_FIXED_TO_FLOAT(value));
    } else {
        cfeListSetValue(&D_00631A54[index][sub], key, CFE_FIXED_TO_FLOAT(value));
    }
    return 1;
}

extern unsigned char cfeArgCount6Assert[];
extern CfeNode cfeEntries[];

/* Script command (6 arguments): sets key `key` of track `column` of flag record `row` of entry `entry`
 * to the 8.8 value (flagged through cfeListSetValueLinear when `flagged` is set), then walks that track to its
 * end. */
int cfe_0024C940(int handle)
{
    int entry;
    int row;
    int column;
    int key;
    int flagged;
    int value;
    CfeTrackNode *node;

    if (VM_PLUGIN_CNT_GET(handle) != 6) {
        assertFailed(cfeArgCount6Assert, cfeAssertFile, 821);
    }
    entry = vmArgGet(handle, 1);
    row = vmArgGet(handle, 2);
    column = vmArgGet(handle, 3);
    key = vmArgGet(handle, 4);
    flagged = vmArgGet(handle, 5);
    value = vmArgGet(handle, 6);
    if (flagged != 0) {
        cfeListSetValueLinear(&cfeEntries[entry].records[row].track[column], key, CFE_FIXED_TO_FLOAT(value));
    } else {
        cfeListSetValue(&cfeEntries[entry].records[row].track[column], key, CFE_FIXED_TO_FLOAT(value));
    }
    for (node = cfeEntries[entry].records[row].track[column]; node != 0; node = node->next) {
    }
    return 1;
}


/* The studio block (0x70 bytes, cleared by cfeStudioInit): the camera and its key lists. */
typedef struct CfeStudioBlock {
    XglCamera *camera;
    CfeTrackNode *lists[27];
} CfeStudioBlock;

extern CfeStudioBlock cfeCamera;

/* Script command (4 arguments): fetches the studio camera and sets key `key` of camera list `index`
 * to the 8.8 value in `mode`; lists 3 to 5 go through cfeListSetAngleKeyMode, the others through
 * cfeListSetKeyMode. */
int cfe_0024CB80(int handle)
{
    int index;
    int mode;
    int key;
    int value;

    if (VM_PLUGIN_CNT_GET(handle) != 4) {
        assertFailed(cfeArgCount4Assert, cfeAssertFile, 856);
    }
    index = vmArgGet(handle, 1);
    key = vmArgGet(handle, 2);
    mode = vmArgGet(handle, 3);
    value = vmArgGet(handle, 4);
    xglStudioGetCamera(&cfeCamera.camera, 0);
    if (index >= 3 && index < 6) {
        cfeListSetAngleKeyMode(&cfeCamera.lists[index], mode, key, CFE_FIXED_TO_FLOAT(value));
    } else {
        cfeListSetKeyMode(&cfeCamera.lists[index], mode, key, CFE_FIXED_TO_FLOAT(value));
    }
    return 1;
}

extern CfeTrackNode *D_0063857C[][32];

/* Script command (5 arguments): sets key `key` of list [index][sub] of D_0063857C to the 8.8 value.
 * Lists 3 to 5 hold angles (cfeListSetAngleLinear / cfeListSetAngle), the others plain values (cfeListSetValueLinear /
 * cfeListSetValue); the flagged forms are used when `flagged` is set. */
int cfe_0024CCF8(int handle)
{
    int index;
    int sub;
    int key;
    int flagged;
    int value;

    if (VM_PLUGIN_CNT_GET(handle) != 5) {
        assertFailed(cfeArgCount5Assert, cfeAssertFile, 896);
    }
    index = vmArgGet(handle, 1);
    sub = vmArgGet(handle, 2);
    key = vmArgGet(handle, 3);
    flagged = vmArgGet(handle, 4);
    value = vmArgGet(handle, 5);
    if (sub >= 3 && sub < 6) {
        if (flagged != 0) {
            cfeListSetAngleLinear(&D_0063857C[index][sub], key, CFE_FIXED_TO_FLOAT(value));
        } else {
            cfeListSetAngle(&D_0063857C[index][sub], key, CFE_FIXED_TO_FLOAT(value));
        }
    } else {
        if (flagged != 0) {
            cfeListSetValueLinear(&D_0063857C[index][sub], key, CFE_FIXED_TO_FLOAT(value));
        } else {
            cfeListSetValue(&D_0063857C[index][sub], key, CFE_FIXED_TO_FLOAT(value));
        }
    }
    return 1;
}

extern CfeTrackNode *D_0063A54C[][16];

/* Script command (5 arguments): sets key `key` of list [index][sub] of D_0063A54C to the 8.8 value.
 * Lists 3 to 5 hold angles (cfeListSetAngleLinear / cfeListSetAngle), the others plain values (cfeListSetValueLinear /
 * cfeListSetValue); the flagged forms are used when `flagged` is set. */
int cfe_0024CF70(int handle)
{
    int index;
    int sub;
    int key;
    int flagged;
    int value;

    if (VM_PLUGIN_CNT_GET(handle) != 5) {
        assertFailed(cfeArgCount5Assert, cfeAssertFile, 933);
    }
    index = vmArgGet(handle, 1);
    sub = vmArgGet(handle, 2);
    key = vmArgGet(handle, 3);
    flagged = vmArgGet(handle, 4);
    value = vmArgGet(handle, 5);
    if (sub >= 3 && sub < 6) {
        if (flagged != 0) {
            cfeListSetAngleLinear(&D_0063A54C[index][sub], key, CFE_FIXED_TO_FLOAT(value));
        } else {
            cfeListSetAngle(&D_0063A54C[index][sub], key, CFE_FIXED_TO_FLOAT(value));
        }
    } else {
        if (flagged != 0) {
            cfeListSetValueLinear(&D_0063A54C[index][sub], key, CFE_FIXED_TO_FLOAT(value));
        } else {
            cfeListSetValue(&D_0063A54C[index][sub], key, CFE_FIXED_TO_FLOAT(value));
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

    if (VM_PLUGIN_CNT_GET(handle) == 6) {
        group = vmArgGet(handle, 1);
        part = vmArgGet(handle, 2);
        sub = vmArgGet(handle, 3);
        key = vmArgGet(handle, 4);
        flagged = vmArgGet(handle, 5);
        value = vmArgGet(handle, 6);
        if (sub >= 3 && sub < 6) {
            if (flagged != 0) {
                cfeListSetAngleLinear(&cfeGroups[group].records[part].source[sub], key, CFE_FIXED_TO_FLOAT(value));
            } else {
                cfeListSetAngle(&cfeGroups[group].records[part].source[sub], key, CFE_FIXED_TO_FLOAT(value));
            }
        } else {
            if (flagged != 0) {
                cfeListSetValueLinear(&cfeGroups[group].records[part].source[sub], key, CFE_FIXED_TO_FLOAT(value));
            } else {
                cfeListSetValue(&cfeGroups[group].records[part].source[sub], key, CFE_FIXED_TO_FLOAT(value));
            }
        }
    } else {
        group = vmArgGet(handle, 1);
        sub = vmArgGet(handle, 2);
        key = vmArgGet(handle, 3);
        flagged = vmArgGet(handle, 4);
        value = vmArgGet(handle, 5);
        if (flagged != 0) {
            cfeListSetValueLinear(&cfeGroups[group].tracks[sub], key, CFE_FIXED_TO_FLOAT(value));
        } else {
            cfeListSetValue(&cfeGroups[group].tracks[sub], key, CFE_FIXED_TO_FLOAT(value));
        }
    }
    return 1;
}

extern CfeTrackNode *D_0063CDE0[];

/* Script command (4 arguments): sets key `key` of list `index` of D_0063CDE0 to the 8.8 value,
 * with flag 0x2 (cfeListSetValueLinear) when `flagged` is set. */
int cfe_0024D5E0(int handle)
{
    int index;
    int key;
    int flagged;
    int value;

    if (VM_PLUGIN_CNT_GET(handle) != 4) {
        assertFailed(cfeArgCount4Assert, cfeAssertFile, 1021);
    }
    index = vmArgGet(handle, 1);
    key = vmArgGet(handle, 2);
    flagged = vmArgGet(handle, 3);
    value = vmArgGet(handle, 4);
    if (flagged != 0) {
        cfeListSetValueLinear(&D_0063CDE0[index], key, CFE_FIXED_TO_FLOAT(value));
    } else {
        cfeListSetValue(&D_0063CDE0[index], key, CFE_FIXED_TO_FLOAT(value));
    }
    return 1;
}

extern CfeTrackNode *D_0063ACB4[][140];

/* Script command (5 arguments): sets key `key` of list [index][sub] of D_0063ACB4
 * (140 per row) to the 8.8 value, with flag 0x2 (cfeListSetValueLinear) when
 * `flagged` is set. */
int cfe_0024D738(int handle)
{
    int index;
    int sub;
    int key;
    int flagged;
    int value;

    if (VM_PLUGIN_CNT_GET(handle) != 5) {
        assertFailed(cfeArgCount5Assert, cfeAssertFile, 1047);
    }
    index = vmArgGet(handle, 1);
    sub = vmArgGet(handle, 2);
    key = vmArgGet(handle, 3);
    flagged = vmArgGet(handle, 4);
    value = vmArgGet(handle, 5);
    if (flagged != 0) {
        cfeListSetValueLinear(&D_0063ACB4[index][sub], key, CFE_FIXED_TO_FLOAT(value));
    } else {
        cfeListSetValue(&D_0063ACB4[index][sub], key, CFE_FIXED_TO_FLOAT(value));
    }
    return 1;
}

extern CfeTrackNode *D_0063CE0C[][6];

/* Script command (5 arguments): sets key `key` of list [index][sub] of D_0063CE0C
 * (6 per row) to the 8.8 value, with flag 0x2 (cfeListSetValueLinear) when
 * `flagged` is set. */
int cfe_0024D8C0(int handle)
{
    int index;
    int sub;
    int key;
    int flagged;
    int value;

    if (VM_PLUGIN_CNT_GET(handle) != 5) {
        assertFailed(cfeArgCount5Assert, cfeAssertFile, 1073);
    }
    index = vmArgGet(handle, 1);
    sub = vmArgGet(handle, 2);
    key = vmArgGet(handle, 3);
    flagged = vmArgGet(handle, 4);
    value = vmArgGet(handle, 5);
    if (flagged != 0) {
        cfeListSetValueLinear(&D_0063CE0C[index][sub], key, CFE_FIXED_TO_FLOAT(value));
    } else {
        cfeListSetValue(&D_0063CE0C[index][sub], key, CFE_FIXED_TO_FLOAT(value));
    }
    return 1;
}

extern CfeTrackNode *D_0063CE90[][20];

/* Script command (5 arguments): sets key `key` of list [index][sub] of D_0063CE90
 * (20 per row) to the 8.8 value, with flag 0x2 (cfeListSetValueLinear) when
 * `flagged` is set. */
int cfe_0024DA40(int handle)
{
    int index;
    int sub;
    int key;
    int flagged;
    int value;

    if (VM_PLUGIN_CNT_GET(handle) != 5) {
        assertFailed(cfeArgCount5Assert, cfeAssertFile, 1099);
    }
    index = vmArgGet(handle, 1);
    sub = vmArgGet(handle, 2);
    key = vmArgGet(handle, 3);
    flagged = vmArgGet(handle, 4);
    value = vmArgGet(handle, 5);
    if (flagged != 0) {
        cfeListSetValueLinear(&D_0063CE90[index][sub], key, CFE_FIXED_TO_FLOAT(value));
    } else {
        cfeListSetValue(&D_0063CE90[index][sub], key, CFE_FIXED_TO_FLOAT(value));
    }
    return 1;
}

/* One 0x44C-byte message record of cfeMessages (cfe_0024DBC0 allocates them). */
typedef struct CfeMessageRecord {
    int first;                  /* +0x000: script argument 2 */
    int second;                 /* +0x004: script argument 3 */
    int third;                  /* +0x008: script argument 5 (5-argument form only) */
    char text[0x400];           /* +0x00C */
    char name[0x40];            /* +0x40C: script argument 4 (5-argument form), else empty */
} CfeMessageRecord;

extern CfeMessageRecord *cfeMessages;
extern unsigned char cfeMessageAssert[];

/* Script command (1 argument): allocates and clears argument 1 message records for cfeMessages. */
int cfe_0024DBC0(int handle)
{
    int count;
    int size;

    if (VM_PLUGIN_CNT_GET(handle) != 1) {
        assertFailed(cfeArgCount1Assert, cfeAssertFile, 1124);
    }
    count = vmArgGet(handle, 1);
    size = count * sizeof(CfeMessageRecord);
    if (count <= 0) {
        return 1;
    }
    cfeMessages = GameResBufferAlloc(size);
    if (cfeMessages == 0) {
        assertFailed(cfeMessageAssert, cfeAssertFile, 1131);
    }
    memset(cfeMessages, 0, size);
    return 1;
}

extern int cfePlugInRecordCount;
extern const char cfeEmptyName[];

/* Script command (3 or 5 arguments): fills the next message record with the unescaped, laid-out text
 * of argument 1 and arguments 2 and 3; the 5-argument form also sets the name (argument 4) and the
 * third value (argument 5). */
int cfe_0024DC78(int handle)
{
    CfeMessageRecord *record = &cfeMessages[cfePlugInRecordCount];

    if (VM_PLUGIN_CNT_GET(handle) == 3) {
        memset(record->text, 0, sizeof(record->text));
        cfeMesUnescape(record->text, vmKeyedEntryGet(handle, vmArgGet(handle, 1)));
        cfeMesLayoutText(record->text);
        record->first = vmArgGet(handle, 2);
        record->second = vmArgGet(handle, 3);
        /* The empty name: the original copies the one byte of the empty string cfeEmptyName (an inlined
         * strcpy). */
        record->name[0] = cfeEmptyName[0];
        cfePlugInRecordCount++;
    } else if (VM_PLUGIN_CNT_GET(handle) == 5) {
        memset(record->text, 0, sizeof(record->text));
        cfeMesUnescape(record->text, vmKeyedEntryGet(handle, vmArgGet(handle, 1)));
        cfeMesLayoutText(record->text);
        record->first = vmArgGet(handle, 2);
        record->second = vmArgGet(handle, 3);
        cfeMesUnescape(record->name, vmKeyedEntryGet(handle, vmArgGet(handle, 4)));
        record->third = vmArgGet(handle, 5);
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
    if (VM_PLUGIN_CNT_GET(handle) != 1) {
        assertFailed(cfeArgCount1Assert, cfeAssertFile, 1201);
    }
    value = vmArgGet(handle, 1);
    cfeRangeSetMin(value);
    return 1;
}


int cfe_0024DE80(int handle)
{
    int value;
    if (VM_PLUGIN_CNT_GET(handle) != 1) {
        assertFailed(cfeArgCount1Assert, cfeAssertFile, 1217);
    }
    value = vmArgGet(handle, 1);
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
/* The running fade (nonzero while a fade is under way). */
extern int fadeUpdateHook;
extern PadRecord PadData[];
extern XglLightSet cfeLightSet;
extern CfeEvalGlobal cfeEvalGlobal;

/* SaveData as the subtitles read it: +0x3A shows every subtitle (game_movie.c caption_all). */
typedef struct CfeSaveData {
    unsigned char unmodeled_00[0x3A];
    unsigned char captionAll;
} CfeSaveData;

extern CfeSaveData SaveData;
extern const char cfeMesFontSettings[];
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

    if (VM_PLUGIN_CNT_GET(handle) != 1) {
        assertFailed(cfeArgCount1Assert, cfeAssertFile, 1250);
    }
    while (xglCdServiceIfPending() != 0) {
    }
    time = vmArgGet(handle, 1);
    cfeRangeSetValue(time);
    if (cfeStartDelay != 0) {
        cfeStartDelay--;
    }
    if (cfeRangeGetValue() - cfeRangeGetMin() >= CFE_PAUSE_DELAY && fadeUpdateHook == 0
        && (PadData[0].pressed & PAD_START)) {
        cfeStartDelay = 2;
        cfePaused = cfePaused == 0;
        seVoiceStartDefault(CFE_PAUSE_SOUND);
        if (cfePaused != 0) {
            xglClockSampleDayTime();
        }
    }
    if (cfePlugInRecordCount > 0 && time == cfeRangeGetMin() && strcmp(cfeMessages->name, cfeEmptyName) != 0) {
        cfeRunAndWait(cfeMessages->name, cfeMessages->third);
    }
    if (cfePaused == 0) {
        cfeCameraEvaluate(time, 0);
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
    pxySetDefaultLight((struct PxyShadowLight *) &cfeLightSet);
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
        if (cfeMessages[i].first <= time && time < cfeMessages[i].second && cfeMessages[i].first == time
            && strcmp(cfeMessages[i].name, cfeEmptyName) != 0 && cfePaused == 0) {
            soundSlotSetLevel(cfeMessages[i].third, 0);
        }
    }
    for (i = 0; i < cfePlugInRecordCount; i++) {
        if (cfeMessages[i].first <= time && time < cfeMessages[i].second
            && (SaveData.captionAll != 0 || cfeMessages[i].text[0] == ' ')) {
            y = CFE_SUBTITLE_Y;
            if (cfeStrCountChar(cfeMessages[i].text, '\n') >= 2) {
                y = CFE_SUBTITLE_Y2;
            }
            xglFontPrint(0, y, CFE_SUBTITLE_COLOR, cfeMesFontSettings);
            xglFontPrintDirect(cfeMessages[i].text);
        }
    }
    if (cfePaused != 0) {
        xglClockAccumulate();
        func_0027C7D0();
        XenoActorSetHoldFlag();
        soundSlotSetLevel(0, 1);
        soundSlotSetLevel(1, 1);
        seSetMuted(1, 1);
        pauseDrawScreen();
        if ((PadData[0].pressed & PAD_TRIANGLE) && cfeStartDelay == 0) {
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
    cfeUpdateEntries(cfePaused);
    XenoActorDoit();
    XenoActorDisp();
    if (cfePaused == 0) {
        sefExecEffect();
    }
    return time;
}


int cfe_0024E688(int handle)
{
    if (VM_PLUGIN_CNT_GET(handle) != 1) {
        assertFailed(cfeArgCount1Assert, cfeAssertFile, 1456);
    }
    sefReleaseOwnedRecords(-1);
    return 1;
}

void fadeStartOut(int);

int cfe_0024E6D8(int handle)
{
    int value;
    if (VM_PLUGIN_CNT_GET(handle) != 1) {
        assertFailed(cfeArgCount1Assert, cfeAssertFile, 1475);
    }
    value = vmArgGet(handle, 1);
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
    if (cfeMessages != 0) {
        GameResBlockFreeByBuffer(cfeMessages);
        cfeMessages = 0;
    }
    for (i = 0; i < cfeGroupCount; i++) {
        if (cfeGroups[i].records != 0) {
            GameResBlockFreeByBuffer(cfeGroups[i].records);
        }
        GameResBlockFreeByOwner(cfeGroups[i].object->kind);
    }
    cfeGroupCount = 0;
    for (i = 0; i < cfeEvalEntryCount; i++) {
        if (cfeEvalEntries[i].movie.source != 0) {
            GameResBlockFreeByBuffer(cfeEvalEntries[i].movie.source);
        }
        if (cfeEvalEntries[i].movie.imageAddress[0] != 0) {
            GameResBlockFreeByBuffer(cfeEvalEntries[i].movie.imageAddress[0]);
        }
        if (cfeEvalEntries[i].movie.imageAddress[1] != 0) {
            GameResBlockFreeByBuffer(cfeEvalEntries[i].movie.imageAddress[1]);
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

void sbNativeOwnerAdd(void *, void *);
extern unsigned char cfePlugInTable[];
extern unsigned char cfePlugInName[];

void cfeRegisterPlugIn(void)
{
    sbNativeOwnerAdd(cfePlugInName, cfePlugInTable);
}
