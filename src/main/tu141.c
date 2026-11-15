#include "common.h"
#include "shared.h"
#include "main/TiGlobalColl.h"
#include "main/cfePlugIn.h"
#include "main/tu022.h"
#include "main/res_get_path.h"
#include "main/tu146.h"
#include "main/xgl_2.h"
#include "main/xgl_studio.h"
#include "quadword.h"


void cfeNodePoolInit(void) {
    cfeNodePoolSet(GameResBufferAlloc(0x48000));
    cfeNodePoolClear();
}

void *memset(void *, int, unsigned int);


/* One 0x1B0-byte node of cfeEntries as cfeEvalRecordsInit sets it up. */
typedef struct CfeNodeInit {
    unsigned char unmodeled_000[0xD0];
    AlignedVector4 vectorD0;    /* +0x0D0 */
    AlignedVector4 vectorE0;    /* +0x0E0 */
    unsigned char unmodeled_0f0[0x10C - 0xF0];
    int parent;                 /* +0x10C: -1, no parent */
    unsigned char unmodeled_110[4];
    float scaleX;               /* +0x114 */
    float scaleY;               /* +0x118 */
    unsigned char unmodeled_11c[4];
    int enabled;                /* +0x120 */
    unsigned char unmodeled_124[0x1B0 - 0x124];
} CfeNodeInit;

/* Number of nodes in cfeEntries. */
#define CFE_NODE_COUNT 64

extern CfeNodeInit cfeEntries[];
extern const AlignedVector4 D_0068D070;

/* Clears the 64 nodes of cfeEntries and gives each the vector D_0068D070 at +0xD0 and +0xE0, no
 * parent, scale 1 and the enabled flag. */
void cfeEvalRecordsInit(void)
{
    AlignedVector4 initial = D_0068D070;
    const AlignedVector4 *source;
    int i;

    memset(cfeEntries, 0, CFE_NODE_COUNT * sizeof(CfeNodeInit));
    for (i = 0; i < CFE_NODE_COUNT; i++) {
        source = &initial;
        QuadwordCopy(&cfeEntries[i].vectorD0, source);
        QuadwordCopy(&cfeEntries[i].vectorE0, source);
        cfeEntries[i].parent = -1;
        cfeEntries[i].scaleX = 1.0f;
        cfeEntries[i].scaleY = 1.0f;
        cfeEntries[i].enabled = 1;
    }
}

int xglCameraSetPadPort(void *, int);
void *xglStudioGetCamera2(int);
void xglStudioInit(void);
void xglStudioMainCameraInit(void);
extern void *D_006384C0;

/* Studio setup: clears the 0x70-byte block at D_006384C0, starts xglStudio and the main camera,
   then keeps the object xglStudioGetCamera2 creates there (its word 1 is set to 2). */
void cfeStudioInit(void) {
    memset(&D_006384C0, 0, 0x70);
    xglStudioInit();
    xglStudioChange(0);
    xglStudioMainCameraInit();
    D_006384C0 = xglStudioGetCamera2(0);
    ((int *)D_006384C0)[1] = 2;
    xglCameraSetPadPort(D_006384C0, 1);
}

extern int cfeCommandCount[];
extern unsigned char cfeCommands[];

void cfeCommandTableInit(void) {
    int *entry;
    int i;

    cfeCommandCount[0] = 0;
    memset(cfeCommands, 0, 0x2000);
    entry = (int *)(cfeCommands + 0x1FC8);
    for (i = 0; i < 64; i++) {
        *entry = -1;
        entry -= 0x20;
    }
}

/* pi / 2: the angle the unused lights 3 to 5 are given. */
#define CFE_LIGHT_HALF_PI 1.5707964f

#include "main/nml_model_set.h"

/* Number of angle vectors turned into directions: directions[0..5] and viewDirection. */
#define CFE_LIGHT_ANGLE_VECTORS 7

/* Sets the light set the CFE preview uses: three directional lights given as angles (turned into
 * directions here), ambient 0.2 and no point lights, then makes it the pxy default light. */
void func_0024EB48(XglLightSet *light)
{
    Vector4 *direction;
    int i;

    light->colors[0][0] = light->colors[0][1] = light->colors[0][2] = 0.9f;
    light->colors[1][0] = light->colors[1][1] = light->colors[1][2] = 0.0f;
    light->colors[2][0] = light->colors[2][1] = light->colors[2][2] = 0.0f;
    light->ambientColor.x = light->ambientColor.y = light->ambientColor.z = 0.2f;
    light->ambientColorLower.x = light->ambientColorLower.y = light->ambientColorLower.z = 0.2f;
    light->directions[3][0] = light->directions[4][0] = light->directions[5][0] = CFE_LIGHT_HALF_PI;
    light->viewDirection.x = 0.0f;
    light->directions[0][0] = 0.0f;
    light->directions[0][1] = 0.0f;
    light->directions[0][3] = 1.0f;
    light->directions[1][0] = 0.0f;
    light->directions[1][1] = 0.0f;
    light->directions[1][3] = 1.0f;
    light->directions[2][0] = 0.0f;
    light->directions[2][1] = 0.0f;
    light->directions[2][3] = 1.0f;
    /* directions[0..5] and viewDirection are seven consecutive angle vectors */
    direction = (Vector4 *) light->directions;
    for (i = 0; i < CFE_LIGHT_ANGLE_VECTORS; i++, direction++) {
        xglDirectionFromAngles(direction, direction);
    }
    light->points[0][0] = light->points[0][1] = light->points[0][2] = light->points[0][3] = 0.0f;
    light->points[1][0] = light->points[1][1] = light->points[1][2] = light->points[1][3] = 0.0f;
    light->points[2][0] = light->points[2][1] = light->points[2][2] = light->points[2][3] = 0.0f;
    light->pointDirections[0][0] = light->pointDirections[0][1] = light->pointDirections[0][2]
        = light->pointDirections[0][3] = 0.0f;
    light->pointDirections[1][0] = light->pointDirections[1][1] = light->pointDirections[1][2]
        = light->pointDirections[1][3] = 0.0f;
    light->pointDirections[2][0] = light->pointDirections[2][1] = light->pointDirections[2][2]
        = light->pointDirections[2][3] = 0.0f;
    /* nml_model_set.c types the light block as its own PxyShadowLight */
    pxySetDefaultLight((struct PxyShadowLight *) light);
}

/* A row of cfeDirections: the direction and the colour of one light of cfeDirectionTable (row 0 has only
 * the ambient colour). */
typedef struct CfeLightRow {
    unsigned char unmodeled_00[0x10];
    const float *direction;     /* +0x10 */
    const float *color;         /* +0x14 */
    unsigned char unmodeled_18[0x28];
} CfeLightRow;

extern CfeLightRow cfeDirections[4];
extern XglLightSet cfeDirectionTable;

/* Sets up the light set cfeDirectionTable, clears cfeDirections and points its rows at the ambient colour
 * and at the direction and colour of lights 0 to 2. */
void func_0024ECA0(void)
{
    func_0024EB48(&cfeDirectionTable);
    memset(cfeDirections, 0, sizeof(cfeDirections));
    cfeDirections[0].direction = 0;
    cfeDirections[0].color = &cfeDirectionTable.ambientColor.x;
    cfeDirections[1].direction = cfeDirectionTable.directions[0];
    cfeDirections[1].color = cfeDirectionTable.colors[0];
    cfeDirections[2].direction = cfeDirectionTable.directions[1];
    cfeDirections[2].color = cfeDirectionTable.colors[1];
    cfeDirections[3].direction = cfeDirectionTable.directions[2];
    cfeDirections[3].color = cfeDirectionTable.colors[2];
}

extern int cfeGroupCount;
extern unsigned char cfeGroups[];

void cfeGroupTableInit(void) {
    memset(cfeGroups, 0, 0x300);
    cfeGroupCount = 0;
}

extern int cfePlugInRecordCount;

void cfePlugInRecordCountReset(void) {
    cfePlugInRecordCount = 0;
}

#include "cfeEvaluate.h"
extern CfeEvalGlobal cfeEvalGlobal;

/* Clears the global block and sets its two default vectors (255, 255, 255, 1) and (5, 20, 0, 1). */
void cfeEvalGlobalInit(void)
{
    memset(&cfeEvalGlobal, 0, sizeof(cfeEvalGlobal));
    cfeEvalGlobal.result_int = 0;
    cfeEvalGlobal.evaluated[0].x = 255.0f;
    cfeEvalGlobal.evaluated[0].y = 255.0f;
    cfeEvalGlobal.evaluated[0].z = 255.0f;
    cfeEvalGlobal.evaluated[0].w = 1.0f;
    cfeEvalGlobal.evaluated[1].x = 5.0f;
    cfeEvalGlobal.evaluated[1].y = 20.0f;
    cfeEvalGlobal.evaluated[1].z = 0.0f;
    cfeEvalGlobal.evaluated[1].w = 1.0f;
}

INCLUDE_ASM("asm/main/nonmatchings/tu141", cfeEvalEntriesInit);

/* Resets the three CfeEvalSlot entries of cfeEvalSlots (cfeEvaluate.h): result 0, sample 0x80, sample_float 1.0. */
extern unsigned char cfeEvalSlots[];

void cfeEvalSlotsInit(void) {
    u8 *slot;
    int i;

    do { memset(cfeEvalSlots, 0, 0x48); slot = cfeEvalSlots; } while (0);
    for (i = 0; i < 3; i++) {
        ((int *)slot)[0] = 0;
        ((int *)slot)[1] = 0x80;
        ((f32 *)slot)[2] = 1.0f;
        slot += 0x18;
    }
}


/* Empties the text table: count cfeSndRecordCount = 0 and the first 0x50-byte text record of cfeSndRecords
   (cfePlugIn appends records of that size and bumps the count). */
void cfePlugInTextReset(void) {
    int *field;

    cfeSndRecordCount = 0;
    memset(cfeSndRecords, 0, sizeof(CfeSndRecord));
    field = &cfeSndRecords[0].duration;
    field[-1] = 0x7F;
    field[0] = 0;
}

int cfeEvalEntriesInit(void);

/* Module init: calls two helpers, then each table initializer of this file in order. */
void cfeInit(void) {
    func_001A4010();
    tiGlobalCollSetHandle(0);
    cfeNodePoolInit();
    cfeGroupTableInit();
    cfeEvalRecordsInit();
    cfeStudioInit();
    cfeCommandTableInit();
    func_0024ECA0();
    cfePlugInRecordCountReset();
    cfeEvalGlobalInit();
    cfeEvalEntriesInit();
    cfeEvalSlotsInit();
    cfePlugInTextReset();
}

int sceLseek(int, int, int);
int sceOpen(const char *, int, ...);

/* Opens `path` for writing, creating it (O_WRONLY | O_CREAT = 0x202, mode 0666 = 0x1B6). */
int cfeFileOpenWrite(const char *path) {
    int fd;

    fd = sceOpen(path, 0x202, 0x1B6);
    if (fd >= 0) {
        sceLseek(fd, 0, 0);
    }
    return fd;
}

void func_0024F090(const char *path) {
    sceOpen(path, 1);
}

int sceWrite(int, const void *, int);
int sceRead(int, void *, int);

/* Thin wrappers over the SDK file calls. */
int cfeFileWrite(int fd, const void *buf, int size) {
    return sceWrite(fd, buf, size);
}

int cfeFileRead(int fd, void *buf, int size) {
    return sceRead(fd, buf, size);
}
