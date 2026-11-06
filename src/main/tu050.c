#include "common.h"
#include "main/nml_model_set.h"
#include "main/tu050.h"

INCLUDE_ASM("asm/main/nonmatchings/tu050", func_001D74E0);

#include "shared.h"

typedef struct MainActive {
    u8 unmodeled_00[0x24];
    int pending;
} MainActive;

/* Work block at gameWork. Members follow the offsets the functions access;
 * the member at 0x360 is only passed by address, its bytes are not modeled. */
typedef struct MainWork {
    u8 unmodeled_00[0x14];
    int state;
    u8 unmodeled_18[0x08];
    int current_id;
    u8 unmodeled_24[0x04];
    MainActive *active;
    u8 unmodeled_2C[0x31C];
    Vector4 extent;
    u8 unmodeled_358[0x08];
    u8 unmodeled_360[0x10];
} MainWork;

typedef struct MainObject {
    u8 unmodeled_00[0xD0];
    int flags;
} MainObject;

void func_001D74E0(int index, void *buffer, unsigned int size, int value);
int GameResLoadID(int id, void **buffer, unsigned int *size, void (*callback)(int index, void *buffer, unsigned int size, int value));
void func_001DF858(int id);
void uwamonoResetRecords(int, int);
void sefResetHandleSlots(void);
extern MainWork gameWork;
extern void *D_002B5DB8[];

/* Makes id the current resource. With mode 1, a reload of the current id releases it first and then
 * uwamonoResetRecords(-1, 1) and sefResetHandleSlots() run. id is loaded (retried until GameResLoadID stops
 * returning -1) and a different previous id is released. */
void mainSwitchId(int id, signed char mode)
{
    int prev_id;

    prev_id = gameWork.current_id;
    if (mode == 1) {
        if (prev_id == id) {
            func_001DF858(id);
        }
        uwamonoResetRecords(-1, 1);
        sefResetHandleSlots();
    }
    gameWork.current_id = id;
    while (GameResLoadID(id, D_002B5DB8, 0, func_001D74E0) == -1) {
    }
    if (id != prev_id) {
        func_001DF858(prev_id);
    }
}

void GameDefocusSet(int index, int type, const int *settings);

void GameDefocusReset(void)
{
    GameDefocusSet(0, 0, 0);
}

INCLUDE_ASM("asm/main/nonmatchings/tu050", func_001D7A78);

INCLUDE_ASM("asm/main/nonmatchings/tu050", func_001D7D18);

INCLUDE_ASM("asm/main/nonmatchings/tu050", func_001D7EB8);

INCLUDE_ASM("asm/main/nonmatchings/tu050", DispMapObject);

void func_001BAE88(void *);
void func_001BB0D8(f32, f32, f32, f32);
void func_001BB3FC(void *);
void func_001BB4AC(int);
void func_001BFD48(void *);
extern unsigned char D_002B5E50[];
extern unsigned char D_002B7C88[];
extern unsigned char D_002B7CA8[];
extern unsigned char D_002B7CC8[];
extern MainObject *mainObject;
extern int D_0069B964;
extern void *D_0069B968;

void mainDrawObjects(void)
{
    if (mainObject == 0) {
        return;
    }
    if (gameWork.active != 0 && gameWork.active->pending != 0) {
        return;
    }
    if (gameWork.state != -1) {
        return;
    }
    if (gameWork.extent.x != 0.0f && gameWork.extent.y != 0.0f) {
        func_001BB0D8(gameWork.extent.x, gameWork.extent.y, gameWork.extent.z, gameWork.extent.w);
        func_001BAE88(gameWork.unmodeled_360);
    }
    func_001BB44C(D_002B7C88);
    if (D_0069B964 != 0) {
        func_001BB4AC(D_0069B964);
    }
    func_001BB3FC(D_002B5E50);
    func_001BFD48(mainObject);
    if (mainObject->flags != 0 && D_0069B968 != 0) {
        func_001BB678(mainObject);
        func_001BB44C(D_002B7CA8);
        func_001BB47C(D_002B7CC8);
        func_001BB4AC(D_0069B964);
        func_001BB3FC(D_002B5E50);
        func_001BFD48(D_0069B968);
    }
}
