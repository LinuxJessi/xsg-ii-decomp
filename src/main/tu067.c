#include "common.h"
#include "main/cfePlugIn.h"
#include "main/elevator.h"
#include "main/tu072.h"
#include "main/tu074.h"
#include "main/tu075.h"
#include "main/tu077.h"
#include "main/tu079.h"
#include "main/tu082.h"
#include "main/tu114.h"

INCLUDE_ASM("asm/main/nonmatchings/tu067", uwamonoFindIndexByShortKey);

INCLUDE_ASM("asm/main/nonmatchings/tu067", func_001EF650);

#include "shared.h"

/* Block that D_0069B968 points to; its rows start entry_offset bytes after the block. */
typedef struct ObjTable {
    unsigned char unmodeled_00[0x68];
    int entry_offset;           /* +0x68 */
} ObjTable;

/* One 0x40-byte row of the table at D_0069B968. */
typedef struct ObjTableEntry {
    unsigned char unmodeled_00[0x30];
    Vector4 position;           /* +0x30 */
} ObjTableEntry;

extern ObjTable *D_0069B968;

void func_001EF6A0(Vector4 *destination, int index)
{
    ObjTableEntry *entry = (ObjTableEntry *)((unsigned char *)D_0069B968 + D_0069B968->entry_offset) + index;

    destination->x = entry->position.x;
    destination->y = entry->position.y;
    destination->z = entry->position.z;
    destination->w = 1.0f;
}

INCLUDE_ASM("asm/main/nonmatchings/tu067", func_001EF6E0);

void objxRegisterNatives(void);
void func_AA1B10(void);

/* Registers each script native module in turn: objx, enepc, runtime, ladder, camera, math, menu, uwamono, cfe, talk. */
void scriptRegisterModules(void)
{
    objxRegisterNatives();
    enepcRegisterNatives();
    runtimeRegisterNatives();
    ladderRegisterNatives();
    cameraRegisterNatives();
    mathInit();
    menuInit();
    uwamonoRegisterNatives();
    cfeRegisterPlugIn();
    elvReset();
    scriptRegisterNatives();
    func_AA1B10();
}

INCLUDE_ASM("asm/main/nonmatchings/tu067", objx_001EF898);

INCLUDE_ASM("asm/main/nonmatchings/tu067", objx_001EFCA0);

INCLUDE_ASM("asm/main/nonmatchings/tu067", uwamonoStartMove);

INCLUDE_ASM("asm/main/nonmatchings/tu067", objx_001F0180);

INCLUDE_ASM("asm/main/nonmatchings/tu067", objx_001F0568);

INCLUDE_ASM("asm/main/nonmatchings/tu067", func_001F0750);

INCLUDE_ASM("asm/main/nonmatchings/tu067", objx_001F08B0);

INCLUDE_ASM("asm/main/nonmatchings/tu067", func_001F09B8);

INCLUDE_ASM("asm/main/nonmatchings/tu067", objx_001F0A10);

INCLUDE_ASM("asm/main/nonmatchings/tu067", objx_001F0B78);

INCLUDE_ASM("asm/main/nonmatchings/tu067", objx_001F0CE0);

int uwamonoFindIndexByShortKey(short key);
void func_A9CC70(int source, int code);
int func_A9F0A8(void);
short func_A9F0C0(int source, int which);
typedef struct UwamonoFlagRecord UwamonoFlagRecord;

/* One 0x870-byte record of D_00340020, indexed like uwamonoRecords. */
struct UwamonoFlagRecord {
    unsigned char flag;         /* +0x00 */
    unsigned char unmodeled_01[0x870 - 1];
};

extern UwamonoFlagRecord D_00340020[];

int objx_001F0E90(int source)
{
    int key;
    int index;

    if (func_A9F0A8() != 2) {
        func_A9CC70(source, 2);
        return 0;
    }
    key = func_A9F0C0(source, 1);
    index = uwamonoFindIndexByShortKey((short) key);
    D_00340020[index].flag = (unsigned char) func_A9F0C0(source, 2);
    return 1;
}

INCLUDE_ASM("asm/main/nonmatchings/tu067", objx_001F0F28);

INCLUDE_ASM("asm/main/nonmatchings/tu067", uwamonoSetTarget);

INCLUDE_ASM("asm/main/nonmatchings/tu067", objx_001F1458);

INCLUDE_ASM("asm/main/nonmatchings/tu067", setStepMove);

INCLUDE_ASM("asm/main/nonmatchings/tu067", setStepRotate);

typedef struct UwamonoBinding UwamonoBinding;

typedef struct UwamonoMapping UwamonoMapping;

typedef struct UwamonoLevel UwamonoLevel;

/* Each 0x870-byte record of uwamonoSlotTable starts with two pointers to blocks
 * this file reads; the rest of the record is not touched here. */
struct UwamonoBinding {
    UwamonoMapping *mapping;    /* +0x00 */
    UwamonoLevel *level_block;  /* +0x04 */
    unsigned char unmodeled_08[0x870 - 8];
};

struct UwamonoMapping {
    unsigned char unmodeled_000[0x758];
    int mapped_value;           /* +0x758 */
};

struct UwamonoLevel {
    unsigned char unmodeled_00[0x80];
    short level;                /* +0x80 */
};

int mapFindBlockEntry(int scale, short value);
extern UwamonoBinding uwamonoSlotTable[];

int objx_001F1778(int source)
{
    int key;
    int index;
    short level;
    int mapped;
    UwamonoBinding *binding;

    if (func_A9F0A8() != 2) {
        func_A9CC70(source, 2);
        return 0;
    }
    key = func_A9F0C0(source, 1);
    index = uwamonoFindIndexByShortKey((short) key);
    level = func_A9F0C0(source, 2);
    mapped = mapFindBlockEntry(0x100, level);
    binding = &uwamonoSlotTable[index];
    binding->mapping->mapped_value = mapped;
    binding->level_block->level = level;
    return 1;
}

INCLUDE_ASM("asm/main/nonmatchings/tu067", objx_001F1830);

void func_A9CDB0(const void *first, const void *second);
/* Native entries of the objx module: (name, function) pairs of 8 bytes. */
extern unsigned char objxNatives[];
/* Module name "objx". */
extern unsigned char objxModuleName[];

/* Registers the objx script natives; called by the module registration in scriptRegisterModules. */
void objxRegisterNatives(void)
{
    func_A9CDB0(objxModuleName, objxNatives);
}

INCLUDE_ASM("asm/main/nonmatchings/tu067", runtime_001F18F0);

INCLUDE_ASM("asm/main/nonmatchings/tu067", uwamonoCreateRecord);

INCLUDE_ASM("asm/main/nonmatchings/tu067", runtime_001F23A8);

void func_00246FF8(int value);
extern int uwamonoCurrentIndex;

int runtimeSetLastDan(void)
{
    func_00246FF8((int) uwamonoSlotTable[uwamonoCurrentIndex].mapping);
    return 1;
}

INCLUDE_ASM("asm/main/nonmatchings/tu067", runtime_001F2860);
