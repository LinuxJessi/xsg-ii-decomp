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
#include "main/xgl_cd.h"

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", func_0024B0E8);

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", func_0024B230);

void xglMatrixStackUnit(void);
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
extern unsigned char cfeAssertFile[];

/* The resource block the cfe object loaders read. */
typedef struct CfeResourceBlock {
    int pack;
    int effect;
} CfeResourceBlock;

/* Offset of the resource id inside the actor (a pointer-sized field XenoActor does not model). */
#define CFE_ACTOR_RESOURCE_ID 0x798

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
    *(void **) ((char *) actor + CFE_ACTOR_RESOURCE_ID) = (void *) id;
    actor->pack = resource->pack;
    actor->effect = resource->effect;
    if (resource->pack != 0) {
        xglCullingMapSet(resource->pack);
    }
    return actor;
}

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", func_0024B6D8);

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfeLoadActorObject);

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfe_0024BA70);

int GameResLoadName(char *name, void **data, unsigned int *size, void (*callback)(int index, void *buffer, unsigned int size, int value));
void XenoActorSetModeInSlot(void *, int, int);
float XenoActorSetSelector(void *, int);
int func_A9F0A8(void);
int func_A9F0C0(int, int);
char *func_A9F238(int, int);
char *strcpy(char *, const char *);
extern unsigned char D_0068CE20[];

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

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfe_0024C7B8);

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfe_0024C940);

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfe_0024CB80);

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfe_0024CCF8);

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfe_0024CF70);

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfe_0024D1E8);

typedef struct CfeListNode CfeListNode;
CfeListNode *func_0024F698(CfeListNode **list, int key, float value);
CfeListNode *func_0024F730(CfeListNode **list, int key, float value);
extern CfeListNode *D_0063CDE0[];
extern unsigned char D_0068CF10[];

/* An 8.8 fixed-point script value as a float. */
#define CFE_FIXED_TO_FLOAT(value) ((float) ((value) / 256) + (float) ((value) % 256) * (1.0f / 256))
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

extern unsigned char D_0068CED0[];
extern CfeListNode *D_0063ACB4[][140];

/* Script command (5 arguments): sets key `key` of list [index][sub] of D_0063ACB4 (140 per row) to the 8.8
 * value, with flag 0x2 (func_0024F730) when `flagged` is set. */
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

/* Script command (5 arguments): sets key `key` of list [index][sub] of D_0063CE0C (6 per row) to the 8.8
 * value, with flag 0x2 (func_0024F730) when `flagged` is set. */
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

/* Script command (5 arguments): sets key `key` of list [index][sub] of D_0063CE90 (20 per row) to the 8.8
 * value, with flag 0x2 (func_0024F730) when `flagged` is set. */
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

extern unsigned char D_0068CDE8[];

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
