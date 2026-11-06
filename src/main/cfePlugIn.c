#include "common.h"
#include "main/cfePlugIn.h"
#include "main/control_entry.h"
#include "main/tu022.h"
#include "main/tu026.h"
#include "main/tu138.h"
#include "main/tu175.h"
#include "main/xgl_cd.h"

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", func_00249248);

/* Updates one 0x50-byte sound record: the value at +0x40 moves toward the
   target, and +0x44 holds the duration. */
void func_00249248(unsigned char *record, int target);
/* Sound records, 0x50 bytes each: the name is copied to offset 0 and the
   tween fields sit at +0x40..+0x4C (see cfe_0024C278). */
extern unsigned char cfeSndRecords[];

/* Sets the target of record 0. The argument's address is taken: the original
   keeps a frame for the call and does not tail-call func_00249248. */
void cfeSndSetTarget(int target)
{
  int *target_ref;
  target_ref = &target;
  func_00249248(cfeSndRecords, *target_ref);
}

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", func_00249340);

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", func_00249518);

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", func_00249ED0);

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", func_0024A8C0);

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfeUpdateEntries);

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", func_0024B0E8);

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", func_0024B230);

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfeRotateVectorXY);

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfeStrLastCharPos);

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfeMesLayoutText);

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfeMesUnescape);

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfeLoadResourceObject);

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
extern unsigned char cfeAssertFile[];
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

void func_002AA5D8(char *, unsigned char *, char *);
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
    func_002AA5D8(buffer, D_0068CE50, text);
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

void func_001E1218(void *);
/* Number of sound records in use; the next record is at count * 0x50. */
extern int cfeSndRecordCount;

int cfe_0024C278(int handle)
{
    int count;
    int slot;
    char *text;

    text = func_A9F238(handle, func_A9F0C0(handle, 1));
    strcpy(&cfeSndRecords[cfeSndRecordCount * 0x50], text);
    count = cfeSndRecordCount;
    slot = count * 5;
    count = count + 1;
    cfeSndRecordCount = count;
    func_001E1218(&cfeSndRecords[slot * 0x10]);
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

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfe_0024D5E0);

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfe_0024D738);

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfe_0024D8C0);

INCLUDE_ASM("asm/main/nonmatchings/cfePlugIn", cfe_0024DA40);

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
