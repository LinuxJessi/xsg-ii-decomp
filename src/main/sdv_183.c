#include "common.h"
#include "shared.h"
#include "main/gifState.h"
#include "main/sdv_183.h"
#include "main/sef.h"
#include "main/tu051.h"
#include "main/tu152.h"
#include "vu0_macro.h"

void sdvClearSpecialWork(void);
extern short _sdvSpecialBuf[8];
extern void GameDefocusSet(int index, int, const int *);

/* A packed RGBA colour; the channels are written one by one as bit-fields. */
typedef union SdvColor {
    struct {
        unsigned int red : 8;
        unsigned int green : 8;
        unsigned int blue : 8;
        unsigned int alpha : 8;
    } channel;
    int word;
} SdvColor;

/* One key of a special table (26 bytes). */
typedef struct SdvSpecialKey {
    unsigned short time;      /* +0x00 */
    unsigned short depth;     /* +0x02 */
    unsigned char color[4];   /* +0x04 */
    short x;                  /* +0x08 */
    short y;                  /* +0x0A */
    short z;                  /* +0x0C */
    short width;              /* +0x0E */
    short height;             /* +0x10 */
    short param12;            /* +0x12 */
    short param14;            /* +0x14 */
    short intensity;          /* +0x16 */
    unsigned short decrease;  /* +0x18 */
} SdvSpecialKey;

/* A key viewed as raw bytes: the char member makes the key copies alias every type. */
typedef union SdvKeyBytes {
    SdvSpecialKey key;
    char byte[sizeof(SdvSpecialKey)];
} SdvKeyBytes;

/* A special table: the kind in the low byte of the first halfword, a signed argument in its high byte. */
typedef struct SdvSpecialTable {
    unsigned short kind;      /* +0x00 */
    short part;               /* +0x02 */
    unsigned short modes;     /* +0x04 sdvExecSpecial: ((short)modes >> 4) | (modes & 0xF) become the work flags */
    SdvKeyBytes keys[1];      /* +0x06 */
} SdvSpecialTable;

/* One special record of sdvSpecialTable (32 of them, indexed by special slot). */
typedef struct SdvSpecialWork {
    SdvSpecialTable *table;   /* +0x00 */
    SdvSpecialKey key;        /* +0x04 the interpolated key */
    short frame;              /* +0x1E */
    short keyIndex;           /* +0x20 */
    unsigned char flags;      /* +0x22 bits 0-1 alpha type, bit 4 fbp, bit 6 mode set */
    signed char decreaseWait; /* +0x23 */
    unsigned short clip;      /* +0x24 */
    unsigned char unmodeled_26[10];
    unsigned char matrix[0x40]; /* +0x30 */
} SdvSpecialWork;

extern void *memset(void *dest, int value, unsigned int size);
extern SdvSpecialWork sdvSpecialTable[32];

/* One key track of a camera schedule; sdvProgressPrm reads its key table at +0x04. */
typedef struct SdvScheduleChannel {
    short keyIndex;           /* +0x00: the key sdvProgressKey is at; sdvProgressPrm steps it onto the end marker */
    unsigned char unmodeled_02[2];
    void *keys;               /* +0x04 */
} SdvScheduleChannel;

/* A camera schedule (sefEffectRecords + 0xA1C): the selected camera and four key tracks. */
typedef struct SdvCameraSchedule {
    int camera;                     /* +0x00 the selected camera's script address, 0 when none */
    unsigned char unmodeled_04[3];
    unsigned char unmodeled_07;     /* +0x07 cleared on selection; no reader found yet */
    SdvScheduleChannel channels[4]; /* +0x08 */
} SdvCameraSchedule;

int func_002AA130(void); /* newlib rand() */
int scGetImmAdrImmIdx2(int base, unsigned short *table, int index);
short scGetImmNumIdx(short *table, int index);

/* Picks a camera of table by weighted chance (weights in percent) and loads its four tracks; table holds five
   entries, all five are read, but only the first four can be picked. */
int sdvSelectCamera(SdvCameraSchedule *schedule, int base, unsigned short *table)
{
    int weights[5];
    int cameras[5];
    int roll;
    int camera;
    int total;
    int i;

    camera = 0;
    roll = func_002AA130() % 100;
    if (table == 0 || base == 0) {
        return 0;
    }
    for (i = 0; i < 5; i++) {
        cameras[i] = scGetImmAdrImmIdx2(base, table, i * 2);
        weights[i] = scGetImmNumIdx((short *)table, i * 2 + 1);
    }
    total = 0;
    for (i = 0; i < 4; i++) {
        total += weights[i];
        if (roll < total && cameras[i] != 0) {
            camera = cameras[i];
            break;
        }
    }
    if (camera != 0) {
        schedule->channels[0].keys = (void *)scGetImmAdrImmIdx2(base, (unsigned short *)camera, 0);
        schedule->channels[1].keys = (void *)scGetImmAdrImmIdx2(base, (unsigned short *)camera, 1);
        schedule->channels[2].keys = (void *)scGetImmAdrImmIdx2(base, (unsigned short *)camera, 2);
        schedule->channels[3].keys = (void *)scGetImmAdrImmIdx2(base, (unsigned short *)camera, 3);
    }
    schedule->camera = camera;
    schedule->unmodeled_07 = 0;
    return camera;
}

/* A camera point request: sefGetPosition resolves offset (relative to object) into point. */
typedef struct SdvCameraParam {
    Vector4 point;      /* +0x00 */
    /* A one-member union: with a plain Vector4 member gcc hoists the three angle loads of
       sdvSetCameraParam above the offset stores; the original keeps load, store, load, store. */
    union {
        Vector4 vector;
    } offset;           /* +0x10 */
    int object;         /* +0x20 */
    int part;           /* +0x24 */
    unsigned char unmodeled_28[0x30 - 0x28];
} SdvCameraParam;

/* sefGlobalTransform as sef.c spells it: sefInitPartTransform fills it; scale is used when scaled is set. */
typedef struct SefGlobalTransform {
    Matrix4 matrix;                                  /* +0x000 */
    Vector4 position;                                /* +0x040 */
    float scale;                                     /* +0x050 */
    short scaled;                                    /* +0x054 */
} SefGlobalTransform;


/* Returns its second argument. kind is int: declared short, a sll/sra pair extends it before the call. */
typedef struct SefPartTransform SefPartTransform;
int sefInitPartTransform(SefPartTransform *part, int kind, int scaleKey40, int scaleKey80, int scaleKey100);
void vu0RotateCurrentX(float value);
int sefGetKindVector(Vector4 *destination, int object, int count, int kind);
void sefGetPosition(Vector4 *destination, Vector4 *offset, int kind);
extern SefGlobalTransform sefGlobalTransform;

/* Camera kinds: the part (low bits, from 1) of one of the three request objects; the bare
   kind takes part 1 and a height correction from the object's kind vector. */
#define SDV_KEY_SOURCE 0x40         /* the request's source actor (the same select bits as tu181.c) */
#define SDV_KEY_TARGET 0x80         /* the request's target actor */
#define SDV_KEY_OBJECT1 0x100       /* the request's second object */

/* Builds a camera point from a key: angles (pitch, yaw, distance) orbit around the object that kind selects. */
void sdvSetCameraParam(SdvCameraParam *camera, int kind, short *angles, int scaled, int mode)
{
    Vector4 orbit;
    Vector4 vector;
    int *objects;
    Vector4 *offset;
    float scale;
    float radians;
    int vectorKind;

    vectorKind = -1;
    objects = (int *)sefGetRequestBuffer();
    scale = 1.0f;
    memset(camera, 0, sizeof(*camera));
    if (scaled != 0) {
        sefInitPartTransform((SefPartTransform *)&sefGlobalTransform, kind, objects[0], objects[2], objects[1]);
        if (sefGlobalTransform.scaled != 0) {
            scale = sefGlobalTransform.scale;
        }
    }
    mode &= 0xF;
    camera->offset.vector.x = angles[0];
    camera->offset.vector.y = angles[1];
    camera->offset.vector.z = angles[2];
    camera->offset.vector.w = 1.0f;
    if (mode == 1) {
        /* orbit = (0, 0, distance, 1) turned by yaw and pitch through the current matrix (vf26-vf29) */
        __asm__ __volatile__("sqc2 vf0, 0(%1)" : "=m"(orbit) : "r"(&orbit));
        orbit.z = camera->offset.vector.z * 0.01f;
        radians = 0.017453292f;
        vu0SetCurrentRotationY(camera->offset.vector.y * radians);
        vu0RotateCurrentX(camera->offset.vector.x * radians);
        __asm__ __volatile__("lqc2 vf1, 0(%0)" : : "r"(&orbit) : "memory");
        __asm__ __volatile__(
            "vmulax.xyzw ACC, vf26, vf1x\n"
            "vmadday.xyzw ACC, vf27, vf1y\n"
            "vmaddaz.xyzw ACC, vf28, vf1z\n"
            "vmaddw.xyzw vf20, vf29, vf1w\n"
            : : : "memory");
        offset = &camera->offset.vector;
        __asm__ __volatile__("sqc2 vf20, 0(%0)" : : "r"(offset) : "memory");
    } else {
        offset = &camera->offset.vector;
        Vu0SefScaleVectorXYZNI(offset, offset, 0.01f);
    }
    if ((kind & SDV_KEY_SOURCE) && kind != SDV_KEY_SOURCE) {
        camera->object = objects[0];
        camera->part = kind - (SDV_KEY_SOURCE + 1);
    } else if ((kind & SDV_KEY_TARGET) && kind != SDV_KEY_TARGET) {
        camera->object = objects[2];
        camera->part = kind - (SDV_KEY_TARGET + 1);
    } else if ((kind & SDV_KEY_OBJECT1) && kind != SDV_KEY_OBJECT1) {
        camera->object = objects[1];
        camera->part = kind - (SDV_KEY_OBJECT1 + 1);
    } else if (kind == SDV_KEY_SOURCE) {
        camera->object = objects[0];
        camera->part = 1;
        kind = SDV_KEY_SOURCE + 2;
        vectorKind = 1;
    } else if (kind == SDV_KEY_TARGET) {
        camera->object = objects[2];
        camera->part = 1;
        kind = SDV_KEY_TARGET + 2;
        vectorKind = 0;
    } else if (kind == SDV_KEY_OBJECT1) {
        camera->object = objects[1];
        camera->part = 1;
        kind = SDV_KEY_OBJECT1 + 2;
        vectorKind = 5;
    }
    if (vectorKind >= 0) {
        sefGetKindVector(&vector, camera->object, 1, vectorKind);
        if (vector.y != 0.0f) {
            camera->offset.vector.y -= 1.3f;
        } else {
            camera->offset.vector.y -= 7.8f;
        }
    }
    Vu0SefScaleVectorXYZNI(offset, offset, scale);
    sefGetPosition(&camera->point, offset, kind);
}

/* The camera block of gameWork (struct CameraState in tu051.c, private there) from its flags word at +0xD0:
   the 432 bytes cameraInit clears. The original addresses it through this symbol, so the field names of
   CameraState are kept at their offsets minus 0xD0. */
typedef struct SdvCameraWork {
    unsigned short flags;               /* +0x00: bit CAMERA_FLAG_SLOTS_APPLIED (0x10) set by cameraApplySlots */
    unsigned char unmodeled_02[0x20 - 0x02];
    unsigned char shakeFlags;           /* +0x20: the wave mask of the current shake key */
    unsigned char unmodeled_21[0x30 - 0x21];
    CameraWave waves[3];         /* +0x30 */
    float sway[4];                      /* +0x60 */
    CameraSlot slot[4];          /* +0x70 */
    int anchorIndex;                    /* +0xB0: the object the look-at follows, -1 for none */
    short anchorSubIndex;               /* +0xB4: its part */
    unsigned char unmodeled_B6[0xD0 - 0xB6];
    float offsetX;                      /* +0xD0: the anchor offset */
    float offsetY;
    float offsetZ;
    float offsetW;                      /* +0xDC: 1.0f */
    unsigned char unmodeled_E0[0xF0 - 0xE0];
    float positionX;                    /* +0xF0 */
    float positionY;
    float positionZ;
    float perspective;                  /* +0xFC */
    float lookAtX;                      /* +0x100 */
    float lookAtY;
    float lookAtZ;
    float roll;                         /* +0x10C */
    float pitch;                        /* +0x110 */
    float yaw;                          /* +0x114 */
    unsigned char unmodeled_118[4];
    float distance;                     /* +0x11C */
    float fromPositionX;                /* +0x120 */
    float fromPositionY;
    float fromPositionZ;
    float fromPerspective;
    float fromLookAtX;                  /* +0x130 */
    float fromLookAtY;
    float fromLookAtZ;
    float fromRoll;
    float fromPitch;                    /* +0x140 */
    float fromYaw;
    unsigned char unmodeled_148[4];
    float fromDistance;                 /* +0x14C */
    float toPositionX;                  /* +0x150 */
    float toPositionY;
    float toPositionZ;
    float toPerspective;                /* +0x15C */
    float toLookAtX;                    /* +0x160 */
    float toLookAtY;
    float toLookAtZ;
    float toRoll;                       /* +0x16C */
    float toPitch;                      /* +0x170 */
    float toYaw;                        /* +0x174 */
    unsigned char unmodeled_178[4];
    float toDistance;                   /* +0x17C */
    float fromOffsetX;                  /* +0x180 */
    float fromOffsetY;
    float fromOffsetZ;
    float fromOffsetW;                  /* +0x18C: 1.0f */
    Vector4 anchorOffset;               /* +0x190 */
    float anchorBase[3];                /* +0x1A0 */
    unsigned char unmodeled_1AC[4];
} SdvCameraWork;

extern SdvCameraWork D_002B5E60;

/* Applies the slots, then clears CAMERA_FLAG_SLOTS_APPLIED so the next camera update re-blends. */
void sdvApplyCameraSlots(int slot_mask, int blend_mode, int ease_mode, int frames)
{
    unsigned short *flags = &D_002B5E60.flags;

    cameraApplySlots(slot_mask, blend_mode, ease_mode, frames);
    *flags &= ~0x10;
}

/* The angle unit's scale and full-circle range (no other spelling exists yet). */
extern float D_0069B348;
extern float D_0069B34C;

/* Wraps angle into [0, D_0069B34C) by subtracting whole D_0069B34C-sized turns. */
float sdvWrapAngle(float angle)
{
    float scale = D_0069B348;
    float range = D_0069B34C;
    int quotient;
    float wrapped;

    quotient = (int)(angle * scale) / 360;
    wrapped = angle - (float)quotient * range;
    if (wrapped < 0.0f) {
        wrapped += range;
    }
    return wrapped;
}

/* Returns angle moved by whole turns to lie within half a turn of current (radians). */
float sdvNearestAngle(float angle, float current)
{
    float wrappedCurrent = sdvWrapAngle(current);
    float delta = sdvWrapAngle(angle) - wrappedCurrent;
    float degrees = __builtin_fabs(delta) * 57.29578f;

    if (degrees == 0.0f) {
        return current;
    }
    if ((int)degrees % 360 > 180) {
        /* the other way round is shorter */
        if (delta > 0.0f) {
            return current - (6.2831855f - degrees * 0.017453292f);
        }
        return current + (6.2831855f - degrees * 0.017453292f);
    }
    return current + delta;
}

#include "main/sdv_033.h"

/* The track kinds sdvScheduleCamera passes to sdvProgressPrm. */
#define SDV_TRACK_POSITION 0
#define SDV_TRACK_LOOK_AT 1
#define SDV_TRACK_LENS 3
#define SDV_TRACK_SHAKE 0x41

/* The time a key table ends with, and the bit that makes sdvProgressKey wait for the hit signal (the key's time then
   counts from the frame the hit arrives); the same key bits as tu181.c. */
#define SDV_KEY_END 0x400
#define SDV_KEY_WAIT_HIT 0x1000

/* The second halfword of a camera key: the ease and blend modes of the slot transition to it; the request log
   copies it whole (the union: a struct copy comes out as two byte moves, the original copies the halfword). */
typedef union SdvKeyModes {
    short word;
    struct {
        unsigned char ease;             /* +0x02: the ease mode cameraSlotSetup stores */
        signed char blend;              /* +0x03: the blend mode; nonzero also drives the angles from the positions */
    } mode;
} SdvKeyModes;

/* A key of the look-at (kind 1) and position (kind 0) tracks: 8 halfwords. Every track's key starts with time. */
typedef struct SdvCameraKey {
    short time;                         /* +0x00: the key's frame, or a hit wait (SDV_KEY_WAIT_HIT) */
    SdvKeyModes modes;                  /* +0x02 */
    short mode;                         /* +0x04: sdvSetCameraParam's mode; bit 0x10 anchors to the object */
    short scaled;                       /* +0x06 */
    short angles[3];                    /* +0x08: pitch, yaw, distance */
    short kind;                         /* +0x0E: the object and part the camera orbits */
} SdvCameraKey;

/* A key of the perspective and roll track (kind 3): 4 halfwords, both angles in degrees. */
typedef struct SdvLensKey {
    short time;
    SdvKeyModes modes;
    short perspective;                  /* +0x04 */
    short roll;                         /* +0x06 */
} SdvLensKey;

/* A key of the shake track (kind 0x41): 6 halfwords; a negative mask ends the track. */
typedef struct SdvShakeKey {
    short time;
    short waveMask;                     /* +0x02: bits 0-2 select the waves; its low byte becomes shakeFlags */
    short amplitude;                    /* +0x04: degrees */
    short period;                       /* +0x06 */
    short hold;                         /* +0x08 */
    short cycles;                       /* +0x0A */
} SdvShakeKey;

/* The last look-at and position keys applied: kind, scale flag and modes of each. */
typedef struct SdvCameraKeyLog {
    short lookAtKind;                   /* +0x00 */
    short lookAtScaled;                 /* +0x02 */
    short positionKind;                 /* +0x04 */
    short positionScaled;               /* +0x06 */
    SdvKeyModes lookAtModes;            /* +0x08 */
    SdvKeyModes positionModes;          /* +0x0A */
    unsigned char unmodeled_0C[0x28 - 0x0C];
} SdvCameraKeyLog;

extern SdvCameraKeyLog D_005E7A98;
/* Minus the distance from the camera position to its look-at point. */
extern float D_0069DC38;

/* Finds the key of keys (stride halfwords each) that elapsed reaches, counting *position up; returns its index,
   or SDV_KEY_END when the table is over (defined in tu181.c). */
int sdvProgressKey(short *keys, short *position, int stride, int elapsed);
XglCamera *xglStudioGetActiveCamera(void);
void cameraInit(void);
/* Returns the distance between two points (tu153.c declares it void: the original reads $f0 here). */
float MMathCalcDist(const Vector4 *from, const Vector4 *to);
void cameraAnglesBetween(Vector4 *angles, const Vector4 *from, const Vector4 *to);

/* Advances one track to frame: applies the key frame reaches (look-at, position, perspective and roll, or the
   shake waves), then starts the slot transition to the next key; counts keyIndex past the table's end marker.
   Returns the key index, SDV_KEY_END when the track is over.
   The key after next is an integer sum with the key size first: with `(short *)next + keySize` (or
   `(unsigned char *)next + keyBytes`) gcc emits `addu $2,$20,$3` for the original's `addu $2,$3,$20`. */
int sdvProgressPrm(int kind, SdvScheduleChannel *channel, int keySize, int frame)
{
    SdvCameraParam param;
    Vector4 angles;
    SdvCameraWork *camera;
    short *keys = channel->keys;
    SdvCameraKey *key;
    int keyBytes;                       /* a key's size in bytes */
    SdvCameraKey *next;
    SdvCameraKey *after;
    SdvLensKey *lensKey;
    SdvLensKey *lensNext;
    SdvShakeKey *shakeKey;
    unsigned int index;
    int remaining;                      /* frames from key to next */
    signed char blend;                  /* key's blend mode while applying it */
    signed char blendMode;              /* key's blend and ease modes, read again for the slot transition */
    unsigned char easeMode;
    CameraWave *wave;
    float amplitude;                    /* the shake key's amplitude in radians */
    int bit;
    int i;

    index = sdvProgressKey(keys, &channel->keyIndex, keySize, frame);
    if (index < SDV_KEY_END) {
        camera = &D_002B5E60;
        key = (SdvCameraKey *)&keys[index * keySize];
        keyBytes = keySize * sizeof(short);
        next = (SdvCameraKey *)((unsigned char *)key + keyBytes);
        remaining = next->time - key->time;
        if (key->time & SDV_KEY_WAIT_HIT) {
            if ((next->time & SDV_KEY_WAIT_HIT) == 0) {
                remaining = next->time - frame + 1;
            } else {
                remaining = 0;
            }
        }
        if (kind == SDV_TRACK_SHAKE) {
            shakeKey = (SdvShakeKey *)key;
            if (shakeKey->waveMask >= 0) {
                camera->shakeFlags = shakeKey->waveMask;
                for (i = 0; i < 3; i++) {
                    bit = 1 << i;
                    if (shakeKey->waveMask & bit) {
                        if (bit & 1) {
                            wave = &camera->waves[0];
                        } else if (bit & 2) {
                            wave = &camera->waves[1];
                        } else if (bit & 4) {
                            wave = &camera->waves[2];
                        } else {
                            wave = 0;
                        }
                        if (wave != 0) {
                            amplitude = (float)shakeKey->amplitude * 0.017453292f * 0.05f;
                            wave->period = shakeKey->period;
                            wave->hold = shakeKey->hold;
                            wave->remaining = shakeKey->cycles;
                            wave->value = amplitude;
                            if (shakeKey->cycles != 0) {
                                wave->step = amplitude / (float)shakeKey->cycles;
                            } else {
                                wave->step = 0.0f;
                            }
                        }
                    }
                }
            }
        } else {
            blend = key->modes.mode.blend;
            if (kind == SDV_TRACK_LOOK_AT) {
                if (index == 0) {
                    /* the first key resets the camera, keeping the studio camera's state */
                    XglCamera *studio = xglStudioGetActiveCamera();
                    unsigned int state = studio->state;

                    if (state == 4) {
                        cameraInit();
                    }
                    studio->state = state;
                }
                D_005E7A98.lookAtKind = key->kind;
                D_005E7A98.lookAtScaled = key->scaled;
                D_005E7A98.lookAtModes.word = key->modes.word;
                sdvSetCameraParam(&param, key->kind, key->angles, key->scaled, key->mode);
                camera->slot[0].duration = 0.0f;
                camera->slot[0].progress = 0.0f;
                camera->slot[0].fraction = 0.0f;
                if ((key->mode & 0x10) && param.object != 0) {
                    camera->anchorIndex = param.object;
                    camera->anchorSubIndex = param.part;
                    camera->lookAtX = param.point.x;
                    camera->lookAtY = param.point.y;
                    camera->lookAtZ = param.point.z;
                    camera->offsetX = param.offset.vector.x;
                    camera->offsetY = param.offset.vector.y;
                    camera->offsetZ = param.offset.vector.z;
                    camera->offsetW = 1.0f;
                    camera->toLookAtX = param.point.x;
                    camera->toLookAtY = param.point.y;
                    camera->toLookAtZ = param.point.z;
                    camera->fromOffsetX = param.offset.vector.x;
                    camera->fromOffsetY = param.offset.vector.y;
                    camera->fromOffsetZ = param.offset.vector.z;
                    camera->fromOffsetW = 1.0f;
                } else {
                    camera->anchorIndex = -1;
                    camera->lookAtX = param.point.x;
                    camera->lookAtY = param.point.y;
                    camera->lookAtZ = param.point.z;
                    camera->toLookAtX = param.point.x;
                    camera->toLookAtY = param.point.y;
                    camera->toLookAtZ = param.point.z;
                }
            } else if (kind == SDV_TRACK_POSITION) {
                D_005E7A98.positionKind = key->kind;
                D_005E7A98.positionScaled = key->scaled;
                D_005E7A98.positionModes.word = key->modes.word;
                sdvSetCameraParam(&param, key->kind, key->angles, key->scaled, key->mode);
                camera->slot[1].duration = 0.0f;
                camera->slot[1].progress = 0.0f;
                camera->positionX = param.point.x;
                camera->positionY = param.point.y;
                camera->positionZ = param.point.z;
                camera->slot[1].fraction = 0.0f;
                D_0069DC38 = -MMathCalcDist((Vector4 *)&camera->positionX, (Vector4 *)&camera->lookAtX);
                if (blend != 0) {
                    cameraAnglesBetween(&angles, (Vector4 *)&camera->lookAtX, (Vector4 *)&camera->positionX);
                    camera->pitch = angles.x;
                    camera->yaw = angles.y;
                    camera->distance = angles.w;
                } else {
                    camera->pitch = 0.0f;
                    camera->yaw = 0.0f;
                    camera->distance = 0.0f;
                }
            } else if (kind == SDV_TRACK_LENS) {
                lensKey = (SdvLensKey *)key;
                camera->slot[2].duration = 0.0f;
                camera->slot[2].progress = 0.0f;
                camera->slot[2].fraction = 0.0f;
                camera->slot[3].duration = 0.0f;
                camera->slot[3].progress = 0.0f;
                camera->slot[3].fraction = 0.0f;
                camera->perspective = (float)lensKey->perspective * 0.017453292f;
                camera->roll = (float)lensKey->roll * 0.017453292f;
            }
            if (remaining >= 2 && next->time < SDV_KEY_END) {
                blendMode = key->modes.mode.blend;
                easeMode = key->modes.mode.ease;
                if (kind == SDV_TRACK_LOOK_AT) {
                    sdvSetCameraParam(&param, next->kind, next->angles, key->scaled, next->mode);
                    if ((next->mode & 0x10) && param.object != 0) {
                        camera->toLookAtX = param.point.x;
                        camera->toLookAtY = param.point.y;
                        camera->toLookAtZ = param.point.z;
                        camera->fromOffsetX = param.offset.vector.x;
                        camera->fromOffsetY = param.offset.vector.y;
                        camera->fromOffsetZ = param.offset.vector.z;
                        camera->fromOffsetW = 1.0f;
                    } else {
                        camera->toLookAtX = param.point.x;
                        camera->toLookAtY = param.point.y;
                        camera->toLookAtZ = param.point.z;
                        camera->anchorIndex = -1;
                        sdvApplyCameraSlots(1, blendMode, easeMode, remaining);
                    }
                } else if (kind == SDV_TRACK_POSITION) {
                    sdvSetCameraParam(&param, next->kind, next->angles, key->scaled, next->mode);
                    camera->toPositionX = param.point.x;
                    camera->toPositionY = param.point.y;
                    camera->toPositionZ = param.point.z;
                    if (blendMode != 0) {
                        cameraAnglesBetween(&angles, (Vector4 *)&camera->toLookAtX, (Vector4 *)&camera->toPositionX);
                        camera->pitch = sdvWrapAngle(camera->pitch);
                        camera->yaw = sdvWrapAngle(camera->yaw);
                        camera->toPitch = sdvNearestAngle(angles.x, camera->pitch);
                        camera->toYaw = sdvNearestAngle(angles.y, camera->yaw);
                        camera->toDistance = angles.w;
                    } else {
                        camera->toPitch = 0.0f;
                        camera->toYaw = 0.0f;
                        camera->toDistance = 0.0f;
                    }
                    sdvApplyCameraSlots(2, blendMode, easeMode, remaining);
                } else if (kind == SDV_TRACK_LENS) {
                    lensKey = (SdvLensKey *)key;
                    lensNext = (SdvLensKey *)next;
                    if (lensNext->perspective != lensKey->perspective) {
                        camera->toPerspective = (float)lensNext->perspective * 0.017453292f;
                        sdvApplyCameraSlots(4, blendMode, easeMode, remaining);
                    }
                    if (lensNext->roll != lensKey->roll) {
                        camera->toRoll = (float)lensNext->roll * 0.017453292f;
                        sdvApplyCameraSlots(8, blendMode, easeMode, remaining);
                    }
                }
                after = (SdvCameraKey *)(keyBytes + (int)next);
                if (after->time == SDV_KEY_END) {
                    /* the key after next is the end marker: step onto it now */
                    channel->keyIndex++;
                }
            }
        }
    }
    return index;
}

extern s16 sefClipTestDisabled;

/* Advances the four tracks of a schedule to frame; returns how many of tracks 0 and 1 finished. */
int sdvScheduleCamera(SdvCameraSchedule *schedule, int frame)
{
    int finished = 0;

    if (sefClipTestDisabled == 0) {
        return 0;
    }
    if (schedule == 0 || schedule->camera == 0) {
        return 0;
    }
    if (schedule->channels[1].keys != 0) {
        if (sdvProgressPrm(1, &schedule->channels[1], 8, frame) == 0) {
            finished++;
        }
    }
    if (schedule->channels[0].keys != 0) {
        if (sdvProgressPrm(0, &schedule->channels[0], 8, frame) == 0) {
            finished++;
        }
    }
    if (schedule->channels[2].keys != 0) {
        sdvProgressPrm(3, &schedule->channels[2], 4, frame);
    }
    if (schedule->channels[3].keys != 0) {
        sdvProgressPrm(0x41, &schedule->channels[3], 6, frame);
    }
    return finished;
}

INCLUDE_ASM("asm/main/nonmatchings/sdv_183", sdvTransOffset);

/* Reset: zeroes the slot flags and every special record, then gifStateClearModeValue. */
void sdvResetSpecialWork(void)
{
    memset(_sdvSpecialBuf, 0, sizeof(_sdvSpecialBuf));
    memset(sdvSpecialTable, 0, sizeof(sdvSpecialTable));
    gifStateClearModeValue();
}

void sdvClearSpecialWork(void)
{
    int i;

    for (i = 0; i < 8; i++) {
        if (_sdvSpecialBuf[i] != 0) {
            GameDefocusSet(i, 0, 0);
            _sdvSpecialBuf[i] = 0;
        }
    }
}

int sdvAllocSpecialWork(int kind)
{
    int slot;

    if (kind == 0) {
        if (_sdvSpecialBuf[0] == 0) {
            _sdvSpecialBuf[0] = 1;
            return 0;
        }
        return -1;
    }

    for (slot = 1; slot < 8; slot++) {
        if (_sdvSpecialBuf[slot] == 0) {
            _sdvSpecialBuf[slot] = 1;
            return slot;
        }
    }
    return -1;
}

/* Finds a free special record (index 1..31); 0 when none is free. */
int sdvFindFreeSpecialWork(void)
{
    int slot;

    for (slot = 1; slot < 0x20; slot++) {
        if (sdvSpecialTable[slot].table == 0) {
            return slot;
        }
    }
    return 0;
}

/* Clears the record of one special slot; slot 0 is kept. */
void sdvClearSpecialRecord(int slot)
{
    if (slot > 0) {
        memset(&sdvSpecialTable[slot], 0, sizeof(SdvSpecialWork));
    }
}

/* Bits of SdvSpecialWork.flags. */
#define SDV_FLAG_ALPHA_TYPE 0x03
#define SDV_FLAG_FBP_SHIFT 4
#define SDV_FLAG_MODE_SET_SHIFT 6
#define SDV_FLAG_MODE_SET (1 << SDV_FLAG_MODE_SET_SHIFT)

/* The owner of a special: the sequence it runs (sdvExecSeqTbl steps it), then its slot ids. */
typedef struct SdvSpecialOwner {
    short *script;            /* +0x00 the sequence table, 0 when none */
    unsigned short time;      /* +0x04 */
    signed char slotCount;    /* +0x06 */
    signed char stride;       /* +0x07 halfwords per sequence entry */
    signed char slotIds[6];   /* +0x08 */
} SdvSpecialOwner;

/* Frees the special slots an owner lists; a screen-effect record (kinds 10-15) with no key decrease, or of
   kind 13, also clears the gifState mode value. */
INCLUDE_ASM("asm/main/nonmatchings/sdv_183", sdvFreeSpecialSlots);

/* Runs one special: its sequence step, slot setup, then the keys of each slot. */
INCLUDE_ASM("asm/main/nonmatchings/sdv_183", sdvExecSpecial);

/* D_0069DBAC value while a battle runs: the clip matrix is used and boxes stay square. */
#define SDV_BATTLE_MODE 0x1012F003

/* The 512x448 screen in pixels. */
#define SDV_SCREEN_WIDTH 0x200
#define SDV_SCREEN_HEIGHT 0x1C0

/* Depth of a kind-6 key without a depth: the far plane. */
#define SDV_DEPTH_FAR 0xFFFFF8

extern int D_0069DBAC;
extern float D_0069B378;
extern float D_0069B37C;
extern short D_005E6D60[9];

int sdvTransOffset(int *screen, int *offset, int part, int depth, float *scale, int argument, void *matrix);
void nmlModelUseSubWindow(unsigned int index, int enabled);
void *MMathNormalizeVector2(void *destination, const void *source);
void sefApplyPartTransform(float *destination, float *source, int part);

#define SDV_SET_COLOR(rgba, source) \
    ((rgba).channel.red = (source)->color[0], \
     (rgba).channel.green = (source)->color[1], \
     (rgba).channel.blue = (source)->color[2], \
     (rgba).channel.alpha = (source)->color[3], (rgba).word)

/* A float vector that the VU0 code also stores as a whole quadword. */
typedef union SdvScale {
    float f[4];
    unsigned int word[4];
    Vector4 vector;
} SdvScale;

/* Normalizes a vector in place. */
static inline void *sdvNormalize(float *vector)
{
    return MMathNormalizeVector2(vector, vector);
}

/* Draws the slots of one special: sdvTransOffset per slot, GameDefocusSet when allocated. */
void sdvDrawSpecial(SdvSpecialOwner *special)
{
    int defocusParams[16];
    SdvScale scale;
    int offset[4];
    int screen[4];
    int slotIndex;

    for (slotIndex = 0; slotIndex < special->slotCount && slotIndex < 6; slotIndex++) {
        SdvSpecialWork *record = &sdvSpecialTable[special->slotIds[slotIndex]];
        SdvSpecialTable *table = record->table;
        SdvSpecialKey *key;
        SdvKeyBytes *nextKey;
        SdvKeyBytes *keys;
        int inBattle;
        void *matrix;
        int kind;
        int argument;
        SdvColor color;
        int defocusSlot;
        int overshoot;
        int sizeStep;
        unsigned int i;
        SdvScale *sizeVector;
        int depth;

        if (table == 0) {
            continue;
        }
        kind = table->kind & 0xFF;
        argument = (short)table->kind >> 8;
        keys = table->keys;
        nextKey = &keys[record->keyIndex];
        inBattle = D_0069DBAC == SDV_BATTLE_MODE;
        if (record->clip != 0 && inBattle) {
            matrix = record->matrix;
        } else {
            matrix = 0;
        }
        defocusParams[0] = 0;
        key = &record->key;
        if (record->frame < nextKey->key.time) {
            continue;
        }
        switch (kind) {
        case 7: /* box drawn at the projected point, sized by width/height */
            if (key->color[3] == 0) {
                break;
            }
            sizeVector = &scale;
            offset[0] = key->x;
            offset[1] = key->y;
            offset[2] = key->z;
            /* sizeVector = (0, 0, 0, 1): VU0 vf0 stored through the pointer */
            __asm__ __volatile__("sqc2 vf0, 0(%1)" : "=m"(*sizeVector) : "r"(sizeVector));
            scale.f[0] = key->width * 0.01;
            scale.f[1] = key->height * 0.01;
            if (sdvTransOffset(screen, offset, table->part, key->depth, sizeVector->f, argument, matrix) == 0) {
                break;
            }
            if (screen[0] < 0 || screen[1] < 0 || screen[0] >= SDV_SCREEN_WIDTH || screen[1] >= SDV_SCREEN_HEIGHT) {
                break;
            }
            defocusParams[1] = 1;
            defocusParams[0] = 0;
            if (sdvAllocSpecialWork(0) >= 0) {
                GameDefocusSet(0, 2, defocusParams);
            }
            SDV_SET_COLOR(color, key);
            defocusParams[0] = screen[2];
            defocusParams[1] = color.word;
            defocusParams[2] = screen[0];
            defocusParams[3] = screen[1];
            if (record->table->part == 8) {
                defocusParams[4] = key->width;
                defocusParams[5] = key->height;
            } else {
                defocusParams[4] = scale.f[0];
                defocusParams[5] = scale.f[1];
                if (record->clip != 0) {
                    /* keep the box inside the screen */
                    overshoot = defocusParams[2] - defocusParams[4];
                    if (overshoot < 0) {
                        defocusParams[4] += overshoot;
                    }
                    overshoot = defocusParams[2] + defocusParams[4] - SDV_SCREEN_WIDTH;
                    if (overshoot > 0) {
                        defocusParams[4] -= overshoot;
                    }
                    overshoot = defocusParams[3] - defocusParams[5];
                    if (overshoot < 0) {
                        defocusParams[5] += overshoot;
                    }
                    overshoot = defocusParams[3] + defocusParams[5] - SDV_SCREEN_HEIGHT;
                    if (overshoot > 0) {
                        defocusParams[5] -= overshoot;
                    }
                    if (D_0069DBAC == SDV_BATTLE_MODE) {
                        if ((unsigned int)defocusParams[4] < (unsigned int)defocusParams[5]) {
                            defocusParams[5] = defocusParams[4];
                        }
                        if ((unsigned int)defocusParams[5] < (unsigned int)defocusParams[4]) {
                            defocusParams[4] = defocusParams[5];
                        }
                    }
                }
            }
            defocusParams[6] = key->intensity;
            defocusSlot = sdvAllocSpecialWork(1);
            if (defocusSlot > 0) {
                GameDefocusSet(defocusSlot, 7, defocusParams);
            }
            break;
        case 6:
            if (key->color[3] == 0) {
                break;
            }
            defocusParams[1] = 1;
            defocusParams[0] = 0;
            if (sdvAllocSpecialWork(0) >= 0) {
                GameDefocusSet(0, 2, defocusParams);
            }
            depth = key->depth != 0 ? key->depth << 8 : SDV_DEPTH_FAR;
            SDV_SET_COLOR(color, key);
            defocusParams[0] = depth;
            defocusParams[2] = key->x;
            defocusParams[3] = key->y;
            defocusParams[4] = key->width;
            defocusParams[5] = key->height;
            defocusParams[1] = color.word;
            defocusSlot = sdvAllocSpecialWork(1);
            if (defocusSlot > 0) {
                GameDefocusSet(defocusSlot, 6, defocusParams);
            }
            break;
        case 5:
            if (key->color[3] == 0) {
                break;
            }
            defocusParams[0] = key->depth << 8;
            SDV_SET_COLOR(color, key);
            sizeStep = 1;
            defocusParams[1] = color.word;
            defocusParams[2] = key->x;
            defocusParams[3] = key->y;
            defocusParams[4] = key->width;
            defocusParams[5] = key->height;
            for (i = 0; i < 9; i++) {
                if (D_005E6D60[i] >= key->intensity) {
                    sizeStep = D_005E6D60[i];
                    break;
                }
            }
            defocusParams[6] = sizeStep;
            defocusParams[7] = sizeStep;
            defocusSlot = sdvAllocSpecialWork(1);
            if (defocusSlot > 0) {
                GameDefocusSet(defocusSlot, 5, defocusParams);
            }
            break;
        case 3:
            if (key->color[3] == 0) {
                break;
            }
            GameDefocusSet(0, 2, defocusParams);
            defocusParams[0] = key->intensity;
            SDV_SET_COLOR(color, key);
            defocusParams[2] = key->x;
            defocusParams[1] = color.word;
            defocusParams[3] = key->y;
            defocusParams[4] = key->width;
            defocusParams[5] = key->height;
            defocusSlot = sdvAllocSpecialWork(1);
            if (defocusSlot > 0) {
                GameDefocusSet(defocusSlot, 3, defocusParams);
            }
            break;
        case 1:
            if (key->color[3] == 0) {
                break;
            }
            defocusParams[0] = 0;
            defocusParams[3] = SDV_SET_COLOR(color, key);
            defocusParams[1] = key->intensity;
            defocusParams[2] = key->depth << 8;
            defocusParams[4] = key->x;
            defocusParams[5] = key->y;
            defocusParams[6] = key->width;
            defocusParams[7] = key->height;
            defocusSlot = sdvAllocSpecialWork(1);
            if (defocusSlot > 0) {
                GameDefocusSet(defocusSlot, 1, defocusParams);
            }
            break;
        case 4:
            if (key->color[3] == 0) {
                break;
            }
            defocusParams[0] = 1;
            defocusParams[3] = SDV_SET_COLOR(color, key);
            defocusParams[4] = 0;
            defocusParams[5] = 0;
            defocusParams[6] = 0;
            defocusParams[7] = 0;
            defocusParams[1] = 1;
            defocusParams[2] = key->depth << 8;
            defocusSlot = sdvAllocSpecialWork(1);
            if (defocusSlot > 0) {
                GameDefocusSet(defocusSlot, 1, defocusParams);
            }
            break;
        case 9:
            defocusSlot = sdvAllocSpecialWork(1);
            if (defocusSlot > 0) {
                GameDefocusSet(defocusSlot, 9, defocusParams);
            }
            break;
        /* kinds 10-14: screen effects, handed to gifState (kind 10 + mode) */
        case 10:
        case 11:
        case 12:
        case 13:
        case 14: {
            unsigned char modeSet;
            float *vector = scale.f;

            offset[0] = key->x;
            offset[1] = key->y;
            offset[2] = key->z;
            Vu0SetOrigin(vector);
            scale.f[0] = key->height * D_0069B378;
            scale.f[1] = key->param12 * D_0069B378;
            modeSet = (record->flags >> SDV_FLAG_MODE_SET_SHIFT) & 1;
            if (record->decreaseWait > 0 && (modeSet == 0 || key->decrease != 0)) {
                gifStateSetDecrease(key->decrease * (1.0f / 256.0f));
                record->decreaseWait = 0;
            }
            if (modeSet == 0) {
                gifStateSetMode(kind - 10);
                record->flags |= SDV_FLAG_MODE_SET;
            }
            if (sdvTransOffset(screen, offset, record->table->part, key->depth, vector, argument, 0) == 0) {
                break;
            }
            gifStateSetCenter(screen[0], screen[1]);
            gifStateSetEffectAlpha(record->flags & SDV_FLAG_ALPHA_TYPE, key->color[3]);
            gifStateSetFbp((record->flags >> SDV_FLAG_FBP_SHIFT) & 1);
            gifStateSetBlur(key->intensity);
            func_00246DD8(screen[2]);
            nmlModelUseSubWindow(0, key->width);
            if (kind == 11 && record->table->part != 8) {
                nmlModelUseSubWindow(2, scale.f[0]);
                nmlModelUseSubWindow(3, scale.f[1]);
                scale.f[0] = key->param14 * D_0069B37C;
                sdvTransOffset(screen, offset, record->table->part, key->depth, vector, argument, 0);
                nmlModelUseSubWindow(1, scale.f[0]);
            } else {
                nmlModelUseSubWindow(1, key->height);
                nmlModelUseSubWindow(2, key->param12);
                nmlModelUseSubWindow(3, key->param14);
            }
            if (kind == 12) {
                signed char *direction = (signed char *)key->color;

                scale.f[0] = direction[0];
                scale.f[1] = direction[1];
                scale.f[2] = direction[2];
                scale.f[3] = 1.0f;
                sdvNormalize(scale.f);
                sefApplyPartTransform(vector, vector, record->table->part);
                nmlModelSetMulColor(&scale.vector);
            }
            break;
        }
        }
    }
}

/* The seven words of an sef request: sdvExecAlter copies them into the sef request buffer and hands them to
   sefLoadRequestBuffer. */
typedef struct SdvSefRequest {
    struct SefActor *actor;             /* +0x00 the SefActor of sef_164.c */
    int params[6];                      /* +0x04 */
} SdvSefRequest;

/*
 * The 0x58-byte request an alter is created from: sdvScheduleAlter fills it from an event, sdvCreateAlter
 * copies it into the record at +0xAD0 and sdvExecAlter/sdvDrawAlter read it there.
 */
typedef struct SdvAlterRequest {
    unsigned long long tex0[2];   /* +0x00 GS TEX0 of the two images */
    SdvSefRequest *sefRequest;    /* +0x10 the seven words sdvExecAlter copies into the sef request buffer */
    unsigned char rgba[4];        /* +0x14 */
    unsigned char unmodeled_18[4];
    float length;                 /* +0x1C progress added over maxSamples points (each frame length / maxSamples) */
    unsigned char unmodeled_20[4];
    float imageFrames;            /* +0x24 */
    float life;                   /* +0x28 frames left; 2048 never runs out */
    float width;                  /* +0x2C */
    float height;                 /* +0x30 */
    float minDelta;               /* +0x34 a shorter move keeps the previous delta */
    short parts[2];               /* +0x38 sefGetPosition kinds; with follow set, parts[1] is the heading turn
                                     rate in 1/100 */
    short maxSamples;             /* +0x3C points the record grows to: at most 64, 32 with follow set */
    unsigned short shrink;        /* +0x3E frames taken per frame once the life is spent */
    unsigned short blend;         /* +0x40 */
    short imageCount;             /* +0x42 */
    unsigned short mode;          /* +0x44 */
    unsigned char unmodeled_46[2];
    short layer;                  /* +0x48 sdvDrawAlters draws the records of one layer */
    short owner;                  /* +0x4A index of the owning effect record; negative for none */
    short follow;                 /* +0x4C nonzero: the record trails the part of parts[0] */
    short spread;                 /* +0x4E */
    short spin;                   /* +0x50 degrees the point angles turn per frame */
    short timeBase;               /* +0x52 */
    short timeStep;               /* +0x54 */
    unsigned char unmodeled_56[2];
} SdvAlterRequest;

/* Builds an alter request from each event the sequence steps onto, then creates it through sdvCreateAlter. */
INCLUDE_ASM("asm/main/nonmatchings/sdv_183", sdvScheduleAlter);


/*
 * One alter record: 0xD40 bytes. The quadword members make the record 16-byte aligned, which is why
 * gcc addresses +0xD38 as +0xD30 + 8 and +0xB18 as +0xB10 + 8.
 */
typedef struct SdvAlter {
    Matrix4 worldMatrix;                /* +0x000 */
    Matrix4 matrix;                     /* +0x040 */
    /* +0x080 two 64-point rings, one per sef part; when the record trails a part (follow), [0] holds the
       sampled anchors and [1] the ring swung around them */
    AlignedVector4 points[2][64];
    AlignedVector4 quats[33];           /* +0x880 heading quaternions, newest first */
    AlignedVector4 heading;             /* +0xA90 */
    AlignedVector4 lastDelta;           /* +0xAA0 */
    AlignedVector4 position;            /* +0xAB0 sdvCreateAlter stores the sefGetPosition result here (lq/sq) */
    AlignedVector4 previousPosition;    /* +0xAC0 */
    SdvAlterRequest request;            /* +0xAD0 */
    float widths[64];                   /* +0xB28 */
    float progress;                     /* +0xC28 */
    float angles[32];                   /* +0xC2C */
    unsigned char unmodeled_CAC[0xCB0 - 0xCAC];
    float radii[32];                    /* +0xCB0 */
    unsigned char unmodeled_D30[0xD34 - 0xD30];
    short frame;                        /* +0xD34 */
    unsigned short index;               /* +0xD36 the point written this frame, 0..63 */
    unsigned short active;              /* +0xD38 */
    unsigned short wrapped;             /* +0xD3A set once index has wrapped */
    short serial;                       /* +0xD3C */
    unsigned char unmodeled_D3E[0xD40 - 0xD3E];
} SdvAlter;

extern SdvAlter sdvAlterTable[49];

/* Zeroes one alter record (sdvInitAlter). */
void sdvInitAlter(SdvAlter *alter)
{
    sefMemZero(alter, sizeof(*alter));
}

/* Takes the first free alter record, stamps a serial and copies the request in. */
INCLUDE_ASM("asm/main/nonmatchings/sdv_183", sdvCreateAlter);

/* Zeroes one alter record (same code as sdvInitAlter; the callers tell them apart). */
void sdvDestroyAlter(SdvAlter *alter)
{
    sefMemZero(alter, sizeof(*alter));
}

/* Kills alter id (index | serial << 16) when it is active and the serial matches. */
void sdvKillAlter(int id)
{
    if (id >= 0) {
        int index = id & 0xFFFF;
        int serial = id >> 16;

        if (sdvAlterTable[index].active != 0 && sdvAlterTable[index].serial == serial) {
            sdvDestroyAlter(&sdvAlterTable[index]);
        }
    }
}

/*
 * Destroys every active alter record that belongs to owner; a negative owner destroys nothing.
 * With "active != 0 && owner == owner" gcc reads owner through the active pointer (bnel slots;
 * fndiff 23/34 in place); the owner test on a separate record pointer keeps the original's beq/bne (34/34).
 */
void sdvDestroyOwnerAlters(int owner)
{
    int i;

    if (owner >= 0) {
        for (i = 0; i < 49; i++) {
            if (sdvAlterTable[i].active != 0) {
                SdvAlter *alter = &sdvAlterTable[i];

                if (alter->request.owner == owner) {
                    sdvDestroyAlter(alter);
                }
            }
        }
    }
}

INCLUDE_ASM("asm/main/nonmatchings/sdv_183", func_0026A7A0);

/*
 * Advances one alter record by a frame: the lifetime and frame counters, the sef request, then either the
 * trailing ring (follow) or the next point of each part; a spent record is destroyed.
 */
INCLUDE_ASM("asm/main/nonmatchings/sdv_183", sdvExecAlter);

extern void sceVif1PkAddCode(XglPacket *packet, unsigned int tag);
extern void sceVif1PkCnt(XglPacket *packet, int count);
extern XglPacket *xglPacketGetCurrent(void);

void sdvPacketFlushMscnt(void)
{
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    sceVif1PkCnt(packet, 0);
    sceVif1PkAddCode(packet, 0x11000000);
    sceVif1PkAddCode(packet, 0x17000000);
}

/* A 128-bit quadword (one lq/sq), as the VIF1 packet writers take it. */
typedef unsigned int XglQuadword __attribute__((mode(TI)));

/* The header sdvDrawAlter puts in front of each strip it sends by reference: sdvSetStripHeader clears its first
   two quadwords, fills the UNPACK code, primitive count and strip flag, and ORs the primitive count into the
   loop count at +0x20 (the third quadword is not cleared). */
typedef union SdvStripHeader {
    XglQuadword quad[3];
    struct {
        unsigned int unmodeled_00[3];
        unsigned int unpack;           /* +0x0C VIF UNPACK code for the whole strip */
        unsigned int primitiveCount;   /* +0x10 */
        unsigned int strip;            /* +0x14 sdvDrawAlter passes 1 with its count / 2, else 0 with count / 3 */
        unsigned int unmodeled_18[2];
        unsigned short loopCount;      /* +0x20 ORed with the primitive count */
    } layout;
} SdvStripHeader;

extern void sceVif1PkRef(XglPacket *packet, const void *environment,
                         int count, int mode, int offset, int flags);

/* Keeps the physical address of an EE pointer: DMA references take physical addresses. */
#define SDV_PHYSICAL_ADDRESS_MASK 0x0FFFFFFF

/*
 * Clears and fills a strip header, then references its qwc quadwords (physical address) in the current packet.
 * Masking the address inside the sceVif1PkRef call loads the mask after the calls instead of before them
 * (fndiff 44 insns, 5/48 in place; this form 48/48).
 */
void sdvSetStripHeader(SdvStripHeader *header, int unused, int qwc, int primitiveCount, int strip)
{
    XglPacket *packet;
    void *physical;

    physical = (void *)((unsigned int)header & SDV_PHYSICAL_ADDRESS_MASK);
    packet = xglPacketGetCurrent();
    sdvPacketFlushMscnt();
    header->quad[0] = 0;
    header->quad[1] = 0;
    header->layout.unpack = ((qwc - 1) << 16) | 0x6C008000;
    header->layout.primitiveCount = primitiveCount;
    header->layout.strip = strip;
    header->layout.loopCount |= primitiveCount;
    sceVif1PkRef(packet, physical, qwc, 0, 0, 0);
}

/* Draws one alter's key frames into the packet from xglPacketGetCurrent (sdvDrawAlter). */
INCLUDE_ASM("asm/main/nonmatchings/sdv_183", sdvDrawAlter);

/* Zeroes all 49 alter records (sdvInitAlters). */
void sdvInitAlters(void)
{
    SdvAlter *alter;

    alter = sdvAlterTable;
    do {
        sdvInitAlter(alter);
        alter++;
    } while ((int) alter < (int) (sdvAlterTable + 49));
}

/* Zeroes all 49 alter records through sdvDestroyAlter (sdvDestroyAlters). */
void sdvDestroyAlters(void)
{
    SdvAlter *alter;

    alter = sdvAlterTable;
    do {
        sdvDestroyAlter(alter);
        alter++;
    } while ((int) alter < (int) (sdvAlterTable + 49));
}

/* Runs every active alter record (sdvExecAlters). */
void sdvExecAlters(void)
{
    int i;

    for (i = 0; i < 49; i++) {
        if (sdvAlterTable[i].active != 0) {
            sdvExecAlter(&sdvAlterTable[i]);
        }
    }
}

void svLoadDrawViewMatrix(void);
void svBuildDrawStatePacket(void);
void sdvDrawAlter(SdvAlter *alter);

/* Draws every active alter record of one layer; the draw state is set up before the first one. */
void sdvDrawAlters(int layer)
{
    int i;
    int stateBuilt = 0;

    for (i = 0; i < 49; i++) {
        if (sdvAlterTable[i].active != 0 && sdvAlterTable[i].request.layer == layer) {
            if (!stateBuilt) {
                stateBuilt = 1;
                svLoadDrawViewMatrix();
                svBuildDrawStatePacket();
            }
            sdvDrawAlter(&sdvAlterTable[i]);
        }
    }
}

/* Draws every active alter record that belongs to owner, like sdvDrawAlters. */
void sdvDrawOwnerAlters(int owner)
{
    int i;
    int stateBuilt = 0;

    for (i = 0; i < 49; i++) {
        if (sdvAlterTable[i].active != 0 && sdvAlterTable[i].request.owner == owner) {
            if (!stateBuilt) {
                stateBuilt = 1;
                svLoadDrawViewMatrix();
                svBuildDrawStatePacket();
            }
            sdvDrawAlter(&sdvAlterTable[i]);
        }
    }
}
