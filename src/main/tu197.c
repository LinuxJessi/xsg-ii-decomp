#include "common.h"
#include "main/tu197.h"
#include "main/sdv_033.h"
#include "main/tu187.h"
#include "quadword.h"

/* Builds in destination the rotation that turns the unit vector from onto
 * the unit vector to: the half-angle quaternion (cross(from, to) / s, s / 2)
 * with s = sqrt(2 * (1 + dot(from, to))), expanded to a 4x4 matrix. When s is
 * below 0.01 (opposite vectors) it uses the half turn about y instead and
 * returns 0; otherwise returns 1. A hand-written VU0 macro-mode block, like
 * MMathIsVectorEqual it keeps an untouched 16-byte frame (the unused scratch
 * quadword); the 0.01 threshold is the assembler's li.s literal. */
int MMathMatrixBetweenVectors(Matrix4 *destination, const Vector4 *from, const Vector4 *to)
{
    int rotated;
    Vector4 scratch;

    __asm__ __volatile__(
        ".set noreorder\n"
        "lqc2 vf2, 0(%2)\n"
        "lqc2 vf3, 0(%3)\n"
        "vaddw.xyz vf7, vf0, vf0w\n"
        "vsub.xyzw vf1, vf0, vf0\n"
        "li.s $f9, 0.01\n"
        "vmula.xyz ACC, vf2, vf3\n"
        "vmaddaw.xy ACC, vf7, vf0w\n"
        "vmadd.xyz vf4, vf2, vf3\n"
        "viaddi vi1, vi0, 8\n"
        "vaddw.y vf1, vf0, vf0w\n"
        "addi %0, $0, 1\n"
        "vmulax.z ACC, vf7, vf4x\n"
        "vmadday.z ACC, vf7, vf4y\n"
        "vmadd.z vf4, vf7, vf4\n"
        "vopmula.xyz ACC, vf2, vf3\n"
        "vmfir.w vf6, vi1\n"
        "vsqrt Q, vf4z\n"
        "vopmsub.xyz vf5, vf3, vf2\n"
        "vitof4.w vf6, vf6\n"
        "vwaitq\n"
        "vaddq.x vf4, vf0, Q\n"
        "vrsqrt Q, vf0w, vf4z\n"
        "qmfc2.ni $8, vf4\n"
        "mtc1 $8, $f8\n"
        "abs.s $f8, $f8\n"
        "c.lt.s $f8, $f9\n"
        "vwaitq\n"
        "vaddq.y vf4, vf0, Q\n"
        "bc1tl 1f\n"
        "neg %0, $0\n"
        "vmulx.w vf1, vf6, vf4x\n"
        "vmuly.xyz vf1, vf5, vf4y\n"
        "1:\n"
        "vadd.xyzw vf2, vf1, vf1\n"
        "vaddw.xyzw vf9, vf0, vf0w\n"
        "vsub.xyzw vf8, vf0, vf0\n"
        "vmulz.xyzw vf5, vf2, vf1z\n"
        "vmuly.xyzw vf4, vf2, vf1y\n"
        "vmulx.xyzw vf3, vf2, vf1x\n"
        "vsub.xyzw vf7, vf0, vf0\n"
        "vsub.xyzw vf6, vf0, vf0\n"
        "vaddz.y vf10, vf4, vf5z\n"
        "vaddx.z vf11, vf5, vf3x\n"
        "vaddy.x vf12, vf3, vf4y\n"
        "vaddw.x vf8, vf5, vf4w\n"
        "vsubw.x vf7, vf4, vf5w\n"
        "vsuby.x vf6, vf9, vf10y\n"
        "vsubw.y vf8, vf5, vf3w\n"
        "vsubz.y vf7, vf9, vf11z\n"
        "vaddw.y vf6, vf3, vf5w\n"
        "vsubx.z vf8, vf9, vf12x\n"
        "vaddw.z vf7, vf4, vf3w\n"
        "vsubw.z vf6, vf3, vf4w\n"
        "sqc2 vf0, 0x30(%1)\n"
        "sqc2 vf8, 0x20(%1)\n"
        "sqc2 vf7, 0x10(%1)\n"
        "sqc2 vf6, 0(%1)\n"
        ".set reorder\n"
        : "=r"(rotated)
        : "r"(destination), "r"(from), "r"(to)
        : "$8", "$f8", "$f9", "memory");
    return rotated;
}

extern void *memset(void *destination, int value, unsigned int count);
extern unsigned short MSeObjReady; /* unsigned: MSeObjProc tests it with lhu, andi 1, andi 0xffff (as short, fd.sh
                                     MSeObjProc 14/172 in place) */
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
extern char D_0068C6D8[];
XglCamera *xglStudioGetCamera2(int camera_index);
Matrix4 *MMathRotateMatrixYXZ(Matrix4 *out, const Matrix4 *matrix, const Vector4 *angles);
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
        MMathRotateMatrixYXZ(&D_0061B2C0.rotationMatrix, 0, &D_0061B2C0.rotation);
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
        MOutputDebugStringWarn(D_0068C6D8, mode);
        break;
    }
}

/* Takes the first free slot of the pool: its flags become MSE_OBJ_BUSY, the mapper and the four callbacks 0.
 * Returns 0 when the pool already holds MSE_OBJ_SLOTS objects or no slot is free. */
MSeObj *MSeObjAlloc(void) {
    MSeObj *object = (MSeObj *)MSeObjPool;
    int slot = MSE_OBJ_SLOTS;

    if (MSeObjCount < MSE_OBJ_SLOTS) {
        while (slot-- != 0) {
            if (!(object->flags & MSE_OBJ_BUSY)) {
                /* Volatile keeps the two header stores in source order (fd.sh: plain stores
                 * issue the flags sh before the mapper sw, 26/28 in place; this view 28/28). */
                volatile MSeObj *header = object;

                header->imageMapper = 0;
                header->flags = MSE_OBJ_BUSY;
                MSeObjCount++;
                object->modeProc[0] = 0;
                object->modeProc[1] = 0;
                object->modeProc[2] = 0;
                object->modeProc[3] = 0;
                return object;
            }
            object++;
        }
    }
    return 0;
}

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
