#include "common.h"
#include "main/sdv_180.h"
#include "main/sdv_183.h"
#include "main/sef_164.h"
#include "main/srs.h"
#include "main/tu045.h"
#include "main/tu155.h"
#include "main/tu165.h"
#include "main/tu170.h"
#include "main/tu172.h"
#include "main/tu173.h"
#include "main/tu174.h"
#include "main/tu182.h"
#include "main/tu185.h"

/* Returns the record of sefEntryTable whose key (word at +0x1630, records of 0x1670 bytes, count word at +0x21A90)
 * equals the argument; key 0 and an absent key return NULL. */
INCLUDE_ASM("asm/main/nonmatchings/tu165", sefFindRecordById);

/* Looks up the record for the key with sefFindRecordById and returns its float at +0x1638, or 1.0f when there is none.
 * When the word at sefRequestBuffer +0x1C is 0x140008 it returns 4.0f if the halfword at +0x2C is 3, else 1.0f. */
INCLUDE_ASM("asm/main/nonmatchings/tu165", func_0025FBF0);

#include "shared.h"

extern unsigned char sefGroupTable[];
extern unsigned char sefRequestBuffer[];
extern unsigned char sefSlotTable[];
extern unsigned char D_00804970[];
extern void *memset(void *destination, int value, unsigned int count);
void sevInitPtAllocator(void);
void sefInitEffectRecords(void);

/* Clears the four work tables (sefSlotTable, sefRequestBuffer, sefGroupTable, D_00804970), then runs the init chain of the
 * CD reader, the ambient, and the effect subsystems. */
void sefInitWorkTables(void) {
    sevInitPtAllocator();
    srsInitCdRead();
    sefInitEffectRecords();
    sefClearLineDataRecords();
    memset(sefSlotTable, 0, 0x30);
    memset(sefRequestBuffer, 0, 0x340);
    memset(sefGroupTable, 0, 0x100);
    memset(D_00804970, 0, 0x488);
    sdvInitAmbient();
    sefResetResourceTable();
    sefClearHandleTable();
    sefResetSlotTable();
}

/* Common start: the init chain (sefInitWorkTables among it), then sets the initialized flag sefInitialized = 1 (the gate of
 * the draw passes below) and the time scale sefTimeScale = 1.0f. */
INCLUDE_ASM("asm/main/nonmatchings/tu165", sefStartSystem);

extern int sefEffectRecordCount;
extern int sefLineDataCount;
extern int sefPtAllocCount;
extern f32 D_0069DB94;
extern unsigned char D_00493B80[];
void sefStartSystem(void);
void smInitilize(void *, u32);

/* Start with the built-in block D_00493B80 (0x152800 bytes): record limits 0x40 (sefEffectRecordCount), 0xC0 and 0x44C,
 * step D_0069DB94 = 1.0f, then the common start. */
void sefStartWithDefaultMemory(void) {
    smInitilize(D_00493B80, 0x152800);
    srsResetKeyTable();
    sefEffectRecordCount = 0x40;
    sefLineDataCount = 0xC0;
    sefPtAllocCount = 0x44C;
    D_0069DB94 = 1.0f;
    sefStartSystem();
    sefNoiseInit();
}

/* Start with a caller block of 0x152800 bytes split into three pools: 0xBD120 bytes at the start (sefPtAllocatorState),
 * 0x68400 bytes after them (sefLineDataRecords), and 0x29C00 bytes after that (sefEffectRecords, 64 records of 0xA70 bytes,
 * the table sef_164 reads with in_use at +0x48). Then the common start. */
INCLUDE_ASM("asm/main/nonmatchings/tu165", sefStartWithMemory);

extern s16 sefClipTestDisabled;
extern f32 sefTimeScale;
extern int sefEffectRecordHead;
void sefResetWorkTables(void);
void sefSetOverrideVectors(void *, void *);
void srsLoadEspFile(int, int);

/* Second start variant: sets the mode flag sefClipTestDisabled = 1, then the reset of sefResetWorkTables and the subsystem
 * starts. The mirror is sefStartClipTestEnabled, which sets sefClipTestDisabled = 0 and uses func_00260200. */
void sefStartClipTestDisabled(void) {
    do {
        sefClipTestDisabled = 1;
        sefTimeScale = 1.0f;
    } while (0);
    sefEffectRecordHead = 0;
    sdvInitAlters();
    sefResetWorkTables();
    sefInitEffectRecords();
    sefClearLineDataRecords();
    tiWindClearFirstSlot();
    func_00252E20(1);
    srsLoadEspFile(1, 0);
    sefSetOverrideVectors(0, 0);
}

void func_00260200(void);

void sefStartClipTestEnabled(void) {
    sefTimeScale = 1.0f;
    sefEffectRecordHead = 0;
    sefClipTestDisabled = 0;
    func_00260200();
    sefInitWorkTables();
    func_00252E10(1);
    srsLoadEspFile(0, 1);
    xglCdWaitUntilIdle();
    sefSetOverrideVectors(0, 0);
}

/* Slot table sefSlotTable: eight shorts of ids (+0x00..+0x0E) with a flag short per slot at +0x20..+0x2E.
 * This claims a slot for the key among the first three entries when the key is absent, sets its flag, then calls
 * sefLoadPendingEffects. func_0025FFE8 claims among entries 3..7 the same way, for a positive key only. */
INCLUDE_ASM("asm/main/nonmatchings/tu165", sefAcquireSlot);

INCLUDE_ASM("asm/main/nonmatchings/tu165", func_0025FFE8);

/* Releases the slot of the key: for the matching entry calls sefDeleteWaitEffect (delete the wait effect) and
 * srsReleaseSlotsByIndex, then clears the id and its flag. */
INCLUDE_ASM("asm/main/nonmatchings/tu165", sefReleaseWaitSlot);

int srsLoadPendingEffects();

void sefLoadPendingEffects(void) {
    srsLoadPendingEffects(sefSlotTable);
}

/* Reset: sefReleaseOwnedRecords(-1) frees every record, memset clears sefSlotTable, sefRequestBuffer, sefEntryTable (0x21AA0 bytes) and
 * D_00804970, then the quadword copy of MMathUnitMatrix into D_00471E60 (two passes of four vectors). */
INCLUDE_ASM("asm/main/nonmatchings/tu165", sefResetWorkTables);

/* The same reset as sefResetWorkTables with sefResetAll in place of srsReleaseRecords and tiWindResetSlots. */
INCLUDE_ASM("asm/main/nonmatchings/tu165", func_00260200);

void func_002602F8(void) {
}

/* Loads n records: when n > 0 tail-calls srsLoadEffectSlot with the buffer D_0069DBA0. The dossier lists the
 * string "load" for this function; the bytes of this function do not show it. */
INCLUDE_ASM("asm/main/nonmatchings/tu165", func_00260300);

extern s16 D_0069DB80;
extern s16 D_0069DB82;
extern s16 D_0069DB84;
void func_00259678(void);
void sdvExecAlters(void);
void func_002708E0(int);

/* One step of the effect update. sefExecEffect calls it when sefFlagTest(1) returns 0, and sefExecEffectSteps calls
 * it per step for id 0. Clears D_00804970 first and D_0069DB80/82/84 last. */
void sefExecStep(void) {
    memset(D_00804970, 0, 0x488);
    sefAdvanceGroupKey();
    sdvExecAlters();
    func_002708E0(0);
    func_00259678();
    D_0069DB80 = 0;
    D_0069DB82 = 0;
    D_0069DB84 = 0;
}

/* Installs the two render callbacks in sRender: sefDrawEffect2D at +0x30 and sefDrawEffect3DCallback at +0x40. */
INCLUDE_ASM("asm/main/nonmatchings/tu165", sefSetRenderHooks);

void sefSetRenderHooks(void);

/* gameWork state read by sefExecEffect. vector348 is the Vector4 at +0x348. The second argument passed to
 * sefSetOverrideVectors is gameWork + 0x360, one past the end of this struct, a Vector4 this struct does not model. */
struct SefExecData {
    unsigned char unmodeled_000[0x348];
    Vector4 vector348;
    unsigned char unmodeled_358[0x08];
};

extern struct SefExecData gameWork;

void sefExecEffect(void)
{
    if (sefClipTestDisabled == 0) {
        if (gameWork.vector348.x != 0.0f && gameWork.vector348.y != 0.0f) {
            Vector4 vector;

            vector.x = gameWork.vector348.x;
            vector.y = gameWork.vector348.y;
            vector.z = gameWork.vector348.z;
            vector.w = gameWork.vector348.w;
            sefSetOverrideVectors(&vector, gameWork.unmodeled_358 + 0x08);
        } else {
            sefSetOverrideVectors(0, 0);
        }
    }
    if (sefFlagTest(1) == 0) {
        sefExecStep();
    }
    sefSetRenderHooks();
}

/* Runs the frame steps for id: stores step (argument 3) in D_0069DB94 and loops with a float time accumulator
 * from 0 up to frames (argument 2). id 0 runs sefExecStep per step, then func_002708E0(0); other ids clear
 * D_00804970 and call func_0025EBE0 or func_002708E0 with the id. */
INCLUDE_ASM("asm/main/nonmatchings/tu165", sefExecEffectSteps);

int sefExecEffectSteps(int, float, float);

/* One-frame-step form of sefExecEffectSteps (step 1.0f): runs the given number of frames for id. */
void sefExecEffectFrames(int id, float frames) {
    sefExecEffectSteps(id, frames, 1.0f);
}

void func_001CD1D0(int);

void sefFinishDrawPass(int value) {
    xglPacketModeReset();
    func_001CD134();
    func_001CD1AC();
    func_001CD160();
    func_001CD1D0(value);
}

/* Draw pass installed at sRender +0x30. Guarded by the initialized flag sefInitialized and by the pending bit
 * 0x04000000 of D_002B5D98 (the X1 SEF_DRAW_FLAG_PENDING value). Ends the 3D mode with svDrawScene2D (the
 * _draw3D flag of sv_get_image_item_161.c is cleared there). */
INCLUDE_ASM("asm/main/nonmatchings/tu165", sefDrawEffect2D);

void sefDrawEffect2D(void);

void sefDrawEffect2DAndFinish(int value) {
    sefDrawEffect2D();
    sefFinishDrawPass(value);
}

INCLUDE_ASM("asm/main/nonmatchings/tu165", func_00260720);

/* Draw pass guarded like sefDrawEffect2D (initialized flag sefInitialized, pending bit 0x04000000). When
 * sefGetDraw3DFlag reports yes, enters 3D mode with svDrawScene3D (sets _draw3D = 1, then draws). */
INCLUDE_ASM("asm/main/nonmatchings/tu165", sefDrawEffect3D);

void sefDrawEffect3D(void);

void sefDrawEffect3DAndFinish(int value) {
    sefDrawEffect3D();
    sefFinishDrawPass(value);
}

void sefDrawEffect3DCallback(void) {
    sefDrawEffect3D();
}
