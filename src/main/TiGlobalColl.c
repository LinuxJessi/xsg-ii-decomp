#include "common.h"
#include "shared.h"
#include "main/TiGlobalColl.h"
#include "main/TiXacAnimCurve.h"
#include "main/tu026.h"

/* Offsets used by the functions of this unit; gaps are not modeled. */
typedef struct TiGlobalCollObject {
    unsigned char unmodeled_00[0x04];
    f32 level;               /* 0x04: compared with 1.5f in tiGlobalCollEvalSubobject */
    unsigned char unmodeled_08[0x0c];
    f32 scale;               /* 0x14: floored at 0.03f in tiGlobalCollClampScale */
    unsigned char unmodeled_18[0x0c];
    f32 accumulated;         /* 0x24: reduced by 0.2f times the old scale in tiGlobalCollClampScale */
    unsigned char unmodeled_28[0x28];
    unsigned char subobject; /* 0x50: its address is taken in tiGlobalCollEvalSubobject */
} TiGlobalCollObject;

void tiGlobalCollClampScale(TiGlobalCollObject *obj) {
    f32 previousScale;

    previousScale = obj->scale;
    if (previousScale < 0.03f) {
        obj->scale = 0.03f;
        obj->accumulated -= previousScale * 0.2f;
    }
}


void tiGlobalCollClearHandle(void) {
    tiGlobalCollSetHandle(0);
}

/* The current collision handle: set by tiGlobalCollSetHandle, cleared by tiGlobalCollClearHandle
 * and read by tiGlobalCollGetHandle. Zero means that no collision set is active. */
extern int tiGlobalCollHandle;

void tiGlobalCollSetHandle(int handle) {
    tiGlobalCollHandle = handle;
}

int tiGlobalCollGetHandle(void) {
    return tiGlobalCollHandle;
}

/* Pushes the body (context: 0x10 position, 0x20 velocity) out of each sphere of
 * the handle's list (count +0x30, entries of 0x20 at +0x3c: radius +0x0, centre +0x10).
 * pNode is not read by the body. */
INCLUDE_ASM("asm/main/nonmatchings/TiGlobalColl", tiGlobalCollPushOutSpheres);

/* Clamps the body above each plane of the handle's list (count +0x34, entries of
 * 0x80 at +0x40, matrix +0x40). */
INCLUDE_ASM("asm/main/nonmatchings/TiGlobalColl", tiGlobalCollClampAbovePlanes);

/* Pushes the body out of each cylinder of the handle's list (count +0x38, entries of
 * 0x90 at +0x44: radius +0x0, half height +0x4, matrices +0x10 and +0x50). */
INCLUDE_ASM("asm/main/nonmatchings/TiGlobalColl", tiGlobalCollPushOutCylinders);

/* assert expression "pNode" */
extern char tiGlobalCollAssertExpr[];
/* assert file "../TiGlobalColl.euc.c" */
extern char tiGlobalCollSourceFile[];
int tiGlobalCollGetHandle(void);
void tiGlobalCollPushOutSpheres(int handle, void *pNode, void *context);
void tiGlobalCollClampAbovePlanes(int handle, void *pNode, void *context);
void tiGlobalCollPushOutCylinders(int handle, void *pNode, void *context);

/* Solves one body (context, a pointer in the asm) against
 * the shapes of the current handle: spheres, planes, then cylinders. */
void tiGlobalCollSolveBody(void *pNode, void *context) {
    int handle;

    handle = tiGlobalCollGetHandle();
    if (pNode == 0) {
        assertFailed(tiGlobalCollAssertExpr, tiGlobalCollSourceFile, 138);
    }
    if (handle != 0) {
        tiGlobalCollPushOutSpheres(handle, pNode, context);
        tiGlobalCollClampAbovePlanes(handle, pNode, context);
        tiGlobalCollPushOutCylinders(handle, pNode, context);
    }
}

void func_00294EB0(int handle, void **subobjectPtr, f32 amount);

/* Chooses the sub-object routine by obj->level: below 1.5f or not. */
void tiGlobalCollEvalSubobject(int handle, TiGlobalCollObject *obj, f32 amount) {
    void *subobjectPtr;

    subobjectPtr = &obj->subobject;
    if (obj->level < 1.5f) {
        func_00294EB0(handle, &subobjectPtr, amount);
        return;
    }
    xacReadJointCurves15(handle, &subobjectPtr, amount);
}
