#include "common.h"
#include "main/tu197.h"
#include "main/sdv_033.h"
#include "main/tu187.h"
#include "quadword.h"

INCLUDE_ASM("asm/main/nonmatchings/tu197", MMathMatrixBetweenVectors);

extern void *memset(void *destination, int value, unsigned int count);
extern unsigned short MSeObjReady; /* unsigned: MSeObjProc tests it with lhu, andi 1, andi 0xffff */
extern int MSeObjCount;
extern int MSeObjPool;
extern int MSeMode1Hook;
extern int MSeMode2Hook;

/* Initialises the MSe object pool: 127 slots of 0x800 bytes at 0x01FB0000. */
void MSeObjInit(void) {
    MSeObjPool = 0x01FB0000;
    MSeObjReady = 0;
    MSeObjCount = 0;
    MSeMode1Hook = 0;
    MSeMode2Hook = 0;
    memset((void *)0x01FB0000, 0, 0x3F800);
    MSeObjReady = 1;
}

/* Shutdown: clears the ready flag, the live count and the pool pointer. */
void MSeObjShutdown(void) {
    MSeObjReady = 0;
    MSeObjCount = 0;
    MSeObjPool = 0;
}

#define MSE_OBJ_SLOTS 127
#define MSE_OBJ_BUSY 0x1
#define MSE_OBJ_FREE_REQUESTED 0x100

/* One MSe object: a 0x800-byte slot of the pool MSeObjInit clears (MSeObjAlloc sets flags to 1 and the
 * mapper and the four callbacks to 0). */
typedef struct MSeObj MSeObj;
typedef void (*MSeObjCallback)(MSeObj *object);

struct MSeObj {
    unsigned short flags;           /* +0x0: MSE_OBJ_BUSY, MSE_OBJ_FREE_REQUESTED (MSeObjRequestFree) */
    unsigned char unmodeled_02[2];
    unsigned int imageMapper;       /* +0x4: mapper forced before the mode 2 callback; 0 keeps the current one */
    MSeObjCallback modeProc[4];     /* +0x8: the callback MSeObjProc runs for each mode */
    unsigned char unmodeled_18[0x800 - 0x18];
};

/* The camera snapshot MSeObjProc mode 0 takes before the object callbacks run. */
typedef struct MEfCameraParams {
    XglCamera *camera;              /* +0x0: xglStudioGetCamera2(0) */
    unsigned char unmodeled_04[0xC];
    Matrix4 viewMatrix;             /* +0x10: camera->volume[0].viewMatrix */
    Vector4 rotation;               /* +0x50: camera->travel.rotation */
    Matrix4 rotationMatrix;         /* +0x60: MMathRotateMatrixYXZ of rotation */
} MEfCameraParams;

typedef void (*MSeHook)(void);

/* MEfCamParams in the ov01 link (config/symbols/ov01.txt); main still links it as D_0061B2C0. */
extern MEfCameraParams D_0061B2C0;
extern const char D_0068C6D8[];
XglCamera *xglStudioGetCamera2(int camera_index);
void MMathRotateMatrixYXZ(Matrix4 matrix, int mode, const Vector4 *angles);
void svForceImageMapper(unsigned int index);

/* Runs one pass over the busy slots of the pool: mode 0 takes the camera snapshot first, mode 2 forces each
 * object's image mapper, mode 3 releases the slots MSeObjRequestFree marked; modes 1 and 2 end with their hook.
 * MSeObjPool and the two hooks are still declared int above, hence the casts. */
void MSeObjProc(unsigned int mode) {
    MSeObj *object = (MSeObj *)MSeObjPool;
    int count = MSeObjCount;
    int slot = MSE_OBJ_SLOTS;
    unsigned int mapper = 0;
    XglCamera *camera;

    if (!(MSeObjReady & 1)) {
        return;
    }
    switch (mode) {
    case 0:
        camera = xglStudioGetCamera2(0);
        D_0061B2C0.camera = camera;
        QuadwordCopyMatrixT0(D_0061B2C0.viewMatrix, camera->volume[0].viewMatrix);
        QuadwordCopyT0(&D_0061B2C0.rotation, &camera->travel.rotation);
        MMathRotateMatrixYXZ(D_0061B2C0.rotationMatrix, 0, &D_0061B2C0.rotation);
        while (slot-- != 0) {
            if (object->flags & MSE_OBJ_BUSY) {
                if (object->modeProc[0] != 0) {
                    object->modeProc[0](object);
                }
                if (--count == 0) {
                    break;
                }
            }
            object++;
        }
        break;
    case 1:
        while (slot-- != 0) {
            if (object->flags & MSE_OBJ_BUSY) {
                if (object->modeProc[1] != 0) {
                    object->modeProc[1](object);
                }
                if (--count == 0) {
                    break;
                }
            }
            object++;
        }
        if (MSeMode1Hook != 0) {
            ((MSeHook)MSeMode1Hook)();
        }
        break;
    case 2:
        svForceImageMapper(0);
        while (slot-- != 0) {
            if (object->flags & MSE_OBJ_BUSY) {
                if (object->modeProc[2] != 0) {
                    if (object->imageMapper != 0 && object->imageMapper != mapper) {
                        svForceImageMapper(object->imageMapper);
                        mapper = object->imageMapper;
                    }
                    object->modeProc[2](object);
                }
                if (--count == 0) {
                    break;
                }
            }
            object++;
        }
        if (MSeMode2Hook != 0) {
            ((MSeHook)MSeMode2Hook)();
        }
        break;
    case 3:
        while (slot-- != 0) {
            if (object->flags & MSE_OBJ_BUSY) {
                if (object->modeProc[3] != 0) {
                    object->modeProc[3](object);
                }
                if (object->flags & MSE_OBJ_FREE_REQUESTED) {
                    object->flags = 0;
                    MSeObjCount--;
                }
                if (--count == 0) {
                    break;
                }
            }
            object++;
        }
        break;
    default:
        MOutputDebugStringWarn((char *)D_0068C6D8, mode);
        break;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu197", MSeObjAlloc);

/* Marks the slot whose flags word is *flags for release: MSeObjProc mode 3 clears slots carrying 0x100. */
void MSeObjRequestFree(unsigned short *flags) {
    *flags |= 0x100;
}

/* Sets the callback that MSeObjProc calls with no arguments after its mode 1 pass (MSeMode1Hook). */
void MSeSetMode1Hook(int hook) {
    MSeMode1Hook = hook;
}

/* Sets the callback that MSeObjProc calls with no arguments after its mode 2 pass (MSeMode2Hook). */
void MSeSetMode2Hook(int hook) {
    MSeMode2Hook = hook;
}
