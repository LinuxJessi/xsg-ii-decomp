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
    u8 unmodeled_00[0x08];
    /* A one-member union: the original reads this field with alias set 0, after the stores before it. */
    union {
        int flags;              /* +0x08: 0x01 pauses the map objects, 0x20 hides the riders */
    };
    u8 unmodeled_0C[0x08];
    int state;
    u8 unmodeled_18[0x04];
    int stateArg;               /* +0x1C */
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

/* A one-member union: the original accesses these vectors with alias set 0. */
typedef union MapVector {
    Vector4 vector;
} MapVector;

typedef union AlignedMapVector {
    AlignedVector4 vector;
} AlignedMapVector;

/* The XenoActor of a map object; only the fields used below. */
typedef struct MapActor {
    u8 unmodeled_000[0x10];
    Vector4 position;           /* 0x010 */
    u8 unmodeled_020[0x20];
    Vector4 velocity;           /* 0x040 */
    Vector4 rotation;           /* 0x050 */
    Vector4 direction;          /* 0x060: y is the moving angle */
    u8 unmodeled_070[0x20];
    Vector4 scale;              /* 0x090 */
    u8 unmodeled_0a0[0x1C];
    int drawFlags;              /* 0x0BC: 0x4 hides the actor, 0x10000 fades it */
    float alpha;                /* 0x0C0 */
    int motion;                 /* 0x0C4 */
    u8 unmodeled_0c8[0x780 - 0xC8];
    Vector4 previous;           /* 0x780: the position before this frame */
} MapActor;

/* A rider model carried by a map object (+0x63C, +0x640). */
typedef struct MapRider {
    DoublewordVector4 position; /* 0x00 */
    u8 unmodeled_10[0x10];
    DoublewordVector4 scale;    /* 0x20 */
    u8 unmodeled_30[0x14];
    int flags;                  /* 0x44 */
    u8 unmodeled_48[0x26];
    short visible;              /* 0x6E */
} MapRider;

typedef struct MapMotionInfo {
    u8 unmodeled_00[0x7C];
    int id;                     /* 0x7C */
    short frame;                /* 0x80 */
} MapMotionInfo;

/* MapObjectRecord.kind. */
enum {
    MAP_KIND_FREE = 0,
    MAP_KIND_ACTOR = 1,
    MAP_KIND_MODEL = 2,
    MAP_KIND_FLAG = 3,
    MAP_KIND_ACTOR_STATIC = 4, /* an actor that is not moved by uwamonoUpdateRecord */
    MAP_KIND_ELEVATOR = 6,
    MAP_KIND_MOVER = 7
};

/* MainWork.flags bits. */
#define GAME_PAUSE_MAP_OBJECTS 0x01
#define GAME_HIDE_RIDERS_DELAY 0x10 /* when set, one frame passes before GAME_HIDE_RIDERS is cleared */
#define GAME_HIDE_RIDERS 0x20

/* MapObjectRecord.flags bits. */
#define MAP_OBJ_SHOWN 0x02

/* Bit `key` of a map flag set (an array of 32-bit words). */
#define MAP_BIT_CLEAR(set, key) ((set)[(key) >> 5] &= ~(1 << ((key) & 0x1F)))
#define MAP_BIT_SET(set, key) ((set)[(key) >> 5] |= 1 << ((key) & 0x1F))

#define MAP_RECORD_COUNT 0x100

/* One 0x870-byte record of uwamonoRecords. */
typedef struct MapObjectRecord {
    short id;
    unsigned char kind;         /* +0x002: 0 free, 1/4 actor, 2 model, 3 flag, 6 elevator, 7 mover */
    unsigned char unmodeled_003;
    int key;                    /* +0x004: bit number in the map flag sets */
    int model;                  /* +0x008: index into mapModelResources, -1 for none */
    u8 unmodeled_00c[4];
    AlignedMapVector position;  /* +0x010 */
    MapVector previous;         /* +0x020 */
    u8 unmodeled_030[0x10];
    MapVector rotation;         /* +0x040 */
    MapVector scale;            /* +0x050 */
    MapVector pivot;            /* +0x060 */
    float fade;                 /* +0x070: 0..100 */
    u8 unmodeled_074[0xC];
    MapVector moveFrom;         /* +0x080 */
    MapVector moveTo;           /* +0x090 */
    MapVector turnFrom;         /* +0x0A0 */
    MapVector turnTo;           /* +0x0B0 */
    MapVector scaleFrom;        /* +0x0C0 */
    MapVector scaleTo;          /* +0x0D0 */
    u8 unmodeled_0e0[0x120];
    MapVector tilt;             /* +0x200 */
    u8 unmodeled_210[2];
    short moveStep;             /* +0x212 */
    short moveCount;            /* +0x214 */
    short turnStep;             /* +0x216 */
    short turnCount;            /* +0x218 */
    short scaleStep;            /* +0x21A */
    short scaleCount;           /* +0x21C */
    u8 unmodeled_21e[0x2E];
    short eventTimer;           /* +0x24C */
    u8 unmodeled_24e[2];
    int event;                  /* +0x250 */
    int eventState;             /* +0x254 */
    int eventStateArg;          /* +0x258 */
    short tiltStep;             /* +0x25C */
    short tiltCount;            /* +0x25E */
    signed char rotationOrder;  /* +0x260 */
    u8 unmodeled_261[3];
    float (*transform)[4];      /* +0x264 */
    u8 unmodeled_268[0x88];
    float matrix[4][4];         /* +0x2F0 */
    MapActor *actor;            /* +0x330 */
    MapMotionInfo *motions;     /* +0x334 */
    float size;                 /* +0x338 */
    short motion;               /* +0x33C */
    u8 unmodeled_33e[2];
    u8 moveCurve[2][0x80];      /* +0x340 */
    short moveCurveType;        /* +0x440 */
    u8 unmodeled_442[2];
    u8 turnCurve[2][0x80];      /* +0x444 */
    short turnCurveType;        /* +0x544 */
    short parent;               /* +0x546 */
    short attached;             /* +0x548 */
    u8 unmodeled_54a[6];
    MapVector savedPosition;    /* +0x550 */
    MapVector savedRotation;    /* +0x560 */
    int flags;                  /* +0x570 */
    u8 unmodeled_574[0xC];
    float extent;               /* +0x580 */
    float height;               /* +0x584 */
    u8 unmodeled_588[8];
    signed char searchMode;     /* +0x590 */
    u8 unmodeled_591;
    short elevatorMotion;       /* +0x592 */
    u8 unmodeled_594[0x1C];
    MapVector riderOffset;      /* +0x5B0 */
    signed char talkMode;       /* +0x5C0 */
    signed char searchState;    /* +0x5C1 */
    signed char busy;           /* +0x5C2 */
    u8 unmodeled_5c3[0xD];
    MapVector lookPoint;        /* +0x5D0 */
    u8 unmodeled_5e0[0x38];
    int lookLink;               /* +0x618 */
    u8 unmodeled_61c[0x10];
    signed char riderSide;      /* +0x62C */
    u8 unmodeled_62d[0xB];
    void *effect;               /* +0x638 */
    MapRider *riders[2];        /* +0x63C */
    u8 unmodeled_644[0x870 - 0x644];
} MapObjectRecord;

/* The header block of a map model; its matrix is the model's base transform. */
typedef struct MapModelHeader {
    unsigned long long unmodeled_00[8];
    float matrix[4][4];         /* +0x40 */
} MapModelHeader;

typedef struct MapModelData {
    u8 unmodeled_00[0x68];
    int headerOffset;           /* +0x68: byte offset of the MapModelHeader */
} MapModelData;

/* One pair of D_002B7BB0, the map resource table func_001D9280 loads. */
typedef struct MapModelResource {
    MapModelData *data;
    int texture;
} MapModelResource;

void func_001D74E0(int index, void *buffer, unsigned int size, int value);
int GameResLoadID(int id, void **buffer, unsigned int *size, void (*callback)(int index, void *buffer, unsigned int size, int value));
int GameResBlockFreeByOwner(int owner);
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
            GameResBlockFreeByOwner(id);
        }
        uwamonoResetRecords(-1, 1);
        sefResetHandleSlots();
    }
    gameWork.current_id = id;
    while (GameResLoadID(id, D_002B5DB8, 0, func_001D74E0) == -1) {
    }
    if (id != prev_id) {
        GameResBlockFreeByOwner(prev_id);
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

extern MainObject *mainObject;
extern int D_0069B964;
void pxySetTxyBlock(int);
void pxySubmitModel(void *);
extern MapObjectRecord uwamonoRecords[];
extern int uwamonoCurrentIndex;
extern MapModelResource D_002B7BB0[];
extern int D_002B7C88[];
extern int D_002B7CA8[];
extern int D_002B7CC8[];

void xglMatrixUnit(float matrix[4][4]);
void xglMatrixStackLoad(float matrix[4][4]);
void xglMatrixStackSave(float matrix[4][4]);
void xglMatrixStackRotX(float angle);
void xglMatrixStackRotY(float angle);
void xglMatrixStackRotZ(float angle);
void xglMatrixStackTrans(Vector4 *translation);
void xglMatrixStackScale(Vector4 *scale);
int func_001D7A78(MapObjectRecord *record);
void func_001D7D18(MapObjectRecord *record);
void func_001D7EB8(MainActive *active);
void func_001DC860(int value);
void fadeStartIn(int frames);
void func_00207028(int index);
void func_001DEDD8(int index);
float func_0020A968(int index, Vector4 *position, int motion, int frame, int flags);
void uwamonoUpdateRecord(int index);
float cursolGetByReduceSpeedAngleLoop(float current, float target, float speed);
void mapObjectClearFlags(MapObjectRecord *record);
void uwamonoUpdateMidpoints(int index);
void func_00202CD0(Vector4 *position, float extent, float scale, int mode);
void uwamonoUpdateTransform(MapObjectRecord *record);
void elvUpdateMotion(MapObjectRecord *record);
void *xglPacketReserve(int size);
void Get_MiddlePoint(const float *start, const float *following, short point_index, short point_count, float *result);
void func_00209458(const float *start, const float *following, short point_index, short point_count,
                   void *curveStart, void *curveFollowing, short curveType, float *result);
void func_001BAE58(MapModelHeader *header);
void func_001BA8F0(void);
float Get_Decimal_Surplus_for_Radius(float angle);
float Get_Distance(const Vector4 *a, const Vector4 *b);
float Get_Distance3D(const Vector4 *a, const Vector4 *b);
void *sefSpawnEffectRecordById(int id, Vector4 *position, int flags);
void sefReleaseRecordHandle(void *record);
void seVoiceStartDefault(int id);
void battleSlotsRefresh(void);
void func_00202FA0(float *point);
void XenoActorPushMatrix(MapActor *actor);
void func_001FDA18(MapActor *actor, int index);
void func_001FE5B0(int index, int value);
void func_001FFA60(void);
void func_001FEF78(void);
void func_002025B0(void);

int DispMapObject(void)
{
    Vector4 point;
    MapRider *riders[2];
    float matrix[4][4];
    MapObjectRecord *record;
    MapActor *actor;
    MapModelHeader *header;
    MapModelHeader *source;
    MapModelData *data;
    MainActive *active;
    float size;
    int i;
    int j;
    int k;

    if (gameWork.flags & GAME_HIDE_RIDERS) {
        if (gameWork.flags & GAME_HIDE_RIDERS_DELAY) {
            gameWork.flags &= ~GAME_HIDE_RIDERS_DELAY;
        } else {
            gameWork.flags &= ~GAME_HIDE_RIDERS;
        }
    }
    xglMatrixUnit(matrix);
    for (i = 0; i < MAP_RECORD_COUNT; i++) {
        record = &uwamonoRecords[i];
        if (record->kind != MAP_KIND_FREE) {
        if (record->eventTimer != -1) {
            if (record->eventTimer-- <= 0) {
                switch (record->event) {
                case 0:
                    gameWork.state = record->eventState;
                    gameWork.stateArg = record->eventStateArg;
                    break;
                case 1:
                    func_001DC860(-1000);
                    fadeStartIn(30);
                    break;
                }
            }
        }
        switch (record->kind) {
        case MAP_KIND_ACTOR:
        case MAP_KIND_ACTOR_STATIC:
            actor = record->actor;
            func_00207028(i);
            if (record->flags & MAP_OBJ_SHOWN) {
                actor->drawFlags &= ~4;
                actor->motion = record->motion;
                func_001DEDD8(i);
            } else {
                actor->drawFlags |= 4;
            }
            if (record->kind != MAP_KIND_ACTOR_STATIC) {
                actor->previous.x = actor->position.x;
                actor->previous.y = actor->position.y;
                actor->previous.z = actor->position.z;
                actor->previous.w = actor->position.w;
            }
            if (func_001D7A78(record) == 1) {
                if (record->flags & 0x08000000) {
                    record->position.vector.y = func_0020A968(i, &record->actor->position, record->motions->id,
                                                       record->motions->frame, 0);
                }
                actor->velocity.x = record->position.vector.x - actor->position.x;
                /* The original stores all three differences into velocity.x; y and z are left alone. */
                actor->velocity.x = record->position.vector.y - actor->position.y;
                actor->velocity.x = record->position.vector.z - actor->position.z;
                actor->velocity.w = 1.0f;
                actor->position.x = record->position.vector.x;
                actor->position.y = record->position.vector.y;
                actor->position.z = record->position.vector.z;
                actor->position.w = 1.0f;
                actor->rotation.x = record->rotation.vector.x;
                actor->rotation.y = record->rotation.vector.y;
                actor->rotation.z = record->rotation.vector.z;
                actor->rotation.w = 1.0f;
            }
            actor->scale.x = record->scale.vector.x;
            actor->scale.y = record->scale.vector.y;
            actor->scale.z = record->scale.vector.z;
            actor->scale.w = 1.0f;
            if (record->fade == 0.0f) {
                actor->drawFlags &= ~0x10000;
            } else {
                actor->drawFlags |= 0x10000;
                actor->alpha = (100.0f - record->fade) / 100.0f;
            }
            if (record->kind != MAP_KIND_ACTOR_STATIC) {
                uwamonoUpdateRecord(i);
            }
            if ((record->flags & 0x400) && (record->flags & 0x400000)) {
                record->rotation.vector.y = cursolGetByReduceSpeedAngleLoop(record->rotation.vector.y, actor->direction.y, 4.0f);
                if (!(gameWork.flags & 0x8000)) {
                    mapObjectClearFlags(record);
                }
            }
            uwamonoUpdateMidpoints(i);
            func_00202CD0(&record->position.vector, record->extent, 0.1f, 7);
        }
        }
    }
    if (D_0069B964 == 0 || mainObject == 0) {
        active = gameWork.active;
        if (active != 0 && active->pending != 0) {
            for (k = 0; k < MAP_RECORD_COUNT; k++) {
                record = &uwamonoRecords[k];
                if (record->kind == MAP_KIND_FREE) {
                    continue;
                }
                if (record->kind == MAP_KIND_ELEVATOR) {
                    func_001D7D18(record);
                }
                if (record->actor != 0) {
                    XenoActorPushMatrix(record->actor);
                }
            }
            func_001D7EB8(active);
        }
        return 0;
    }
    for (j = 0; j < MAP_RECORD_COUNT; j++) {
        record = &uwamonoRecords[j];
        if (record->kind == MAP_KIND_FREE) {
            continue;
        }
        switch (record->kind) {
        case MAP_KIND_MODEL:
            if (record->key != -1) {
                if (record->flags & MAP_OBJ_SHOWN) {
                    MAP_BIT_CLEAR(D_002B7CA8, record->key);
                } else {
                    MAP_BIT_SET(D_002B7CA8, record->key);
                }
                if (record->flags & 0x1000) {
                    MAP_BIT_CLEAR(D_002B7CC8, record->key);
                } else {
                    MAP_BIT_SET(D_002B7CC8, record->key);
                }
            }
            if (!(gameWork.flags & GAME_PAUSE_MAP_OBJECTS)) {
                if (record->parent == -1 || record->attached != 1) {
                    func_001D7A78(record);
                    if (record->tiltStep != -1) {
                        record->tiltStep++;
                        if (record->tiltStep > record->tiltCount) {
                            record->tiltStep = -1;
                        }
                    }
                } else {
                    record->position.vector.x = record->savedPosition.vector.x;
                    record->position.vector.y = record->savedPosition.vector.y;
                    record->position.vector.z = record->savedPosition.vector.z;
                    record->rotation.vector.x = record->savedRotation.vector.x;
                    record->rotation.vector.y = record->savedRotation.vector.y;
                    record->rotation.vector.z = record->savedRotation.vector.z;
                    record->position.vector.w = 1.0f;
                    record->rotation.vector.w = 1.0f;
                }
            }
            if (record->talkMode == 2) {
                if (record->busy == 0 && record->tiltStep == -1) {
                    record->searchMode = 0;
                } else {
                    record->searchMode = 1;
                }
            }
            uwamonoUpdateTransform(record);
            if (record->elevatorMotion != -1) {
                elvUpdateMotion(record);
            }
            if (record->parent == -1 || record->attached != 1) {
                xglMatrixStackUnit();
            } else {
                xglMatrixStackLoad(uwamonoRecords[record->parent].matrix);
            }
            xglMatrixStackTrans(&record->position.vector);
            xglMatrixStackTrans(&record->pivot.vector);
            switch (record->rotationOrder) {
            case 0:
                xglMatrixStackRotX(record->rotation.vector.x);
                xglMatrixStackRotY(record->rotation.vector.y);
                xglMatrixStackRotZ(record->rotation.vector.z);
                break;
            case 1:
                xglMatrixStackRotX(record->rotation.vector.x);
                xglMatrixStackRotZ(record->rotation.vector.z);
                xglMatrixStackRotY(record->rotation.vector.y);
                break;
            case 2:
                xglMatrixStackRotY(record->rotation.vector.y);
                xglMatrixStackRotX(record->rotation.vector.x);
                xglMatrixStackRotZ(record->rotation.vector.z);
                break;
            case 3:
                xglMatrixStackRotY(record->rotation.vector.y);
                xglMatrixStackRotZ(record->rotation.vector.z);
                xglMatrixStackRotX(record->rotation.vector.x);
                break;
            case 4:
                xglMatrixStackRotZ(record->rotation.vector.z);
                xglMatrixStackRotX(record->rotation.vector.x);
                xglMatrixStackRotY(record->rotation.vector.y);
                break;
            case 5:
                xglMatrixStackRotZ(record->rotation.vector.z);
                xglMatrixStackRotY(record->rotation.vector.y);
                xglMatrixStackRotX(record->rotation.vector.x);
                break;
            }
            point.x = -record->pivot.vector.x;
            point.y = -record->pivot.vector.y;
            point.z = -record->pivot.vector.z;
            point.w = 1.0f;
            xglMatrixStackTrans(&point);
            xglMatrixStackScale(&record->scale.vector);
            xglMatrixStackSave(record->transform);
            xglMatrixStackSave(record->matrix);
            if (record->model != -1) {
                header = xglPacketReserve(8);
                data = D_002B7BB0[record->model + 2].data;
                source = (MapModelHeader *) ((u8 *) data + data->headerOffset);
                if (record->tiltStep != -1) {
                    Get_MiddlePoint(&record->turnFrom.vector.x, &record->turnTo.vector.x, record->tiltStep, record->tiltCount,
                                    &record->tilt.vector.x);
                    if (record->tiltCount < record->tiltStep) {
                        record->tiltStep = -1;
                    }
                }
                if (record->flags & MAP_OBJ_SHOWN) {
                    *header = *source;
                    xglMatrixStackLoad(header->matrix);
                    xglMatrixStackRotX(record->tilt.vector.x);
                    xglMatrixStackSave(header->matrix);
                    nmlModelSetDrawMatrix(record->transform);
                    func_001BAE58(header);
                    func_001BA8F0();
                    pxySetTxyBlock(D_002B7BB0[record->model + 2].texture);
                    pxySubmitModel(D_002B7BB0[record->model + 2].data);
                }
                if (record->model >= 7 && !(gameWork.flags & GAME_PAUSE_MAP_OBJECTS)) {
                    record->rotation.vector.y = Get_Decimal_Surplus_for_Radius(record->rotation.vector.y + 0.1f);
                }
                if (record->model >= 7 && record->model <= 9) {
                    if (Get_Distance3D(&uwamonoRecords[uwamonoCurrentIndex].position.vector, &record->position.vector)
                        <= (size = uwamonoRecords[uwamonoCurrentIndex].size) * 0.5f) {
                        if (record->effect == 0) {
                            record->effect = sefSpawnEffectRecordById(size == 1.0f ? 0x100C8032 : 0x100C8064,
                                                                      &record->position.vector, 0);
                            seVoiceStartDefault(0x02000010);
                            battleSlotsRefresh();
                        }
                    } else {
                        if (record->effect != 0) {
                            sefReleaseRecordHandle(record->effect);
                        }
                        record->effect = 0;
                    }
                }
            }
            point.x = record->position.vector.x + record->pivot.vector.x;
            point.y = record->position.vector.y + record->pivot.vector.y;
            point.z = record->position.vector.z + record->pivot.vector.z;
            point.w = 1.0f;
            func_00202FA0(&point.x);
            func_00202FA0(record->transform[3]);
            break;
        case MAP_KIND_FLAG:
            if (record->flags & MAP_OBJ_SHOWN) {
                MAP_BIT_CLEAR(D_002B7C88, record->key);
            } else {
                MAP_BIT_SET(D_002B7C88, record->key);
            }
            break;
        case MAP_KIND_ELEVATOR:
            func_001D7D18(record);
            break;
        case MAP_KIND_MOVER:
            record->previous.vector.x = record->position.vector.x;
            record->previous.vector.y = record->position.vector.y;
            record->previous.vector.z = record->position.vector.z;
            record->previous.vector.w = record->position.vector.w;
            if (gameWork.flags & GAME_PAUSE_MAP_OBJECTS) {
                break;
            }
            if (record->moveStep != -1) {
                if (record->moveCurveType == 0) {
                    Get_MiddlePoint(&record->moveFrom.vector.x, &record->moveTo.vector.x, record->moveStep, record->moveCount,
                                    &record->position.vector.x);
                } else {
                    func_00209458(&record->moveFrom.vector.x, &record->moveTo.vector.x, record->moveStep, record->moveCount,
                                  record->moveCurve[0], record->moveCurve[1], record->moveCurveType,
                                  &record->position.vector.x);
                }
                record->moveStep++;
                if (record->moveStep > record->moveCount) {
                    record->moveStep = -1;
                }
            }
            if (record->turnStep != -1) {
                if (record->turnCurveType == 0) {
                    Get_MiddlePoint(&record->turnFrom.vector.x, &record->turnTo.vector.x, record->turnStep, record->turnCount,
                                    &record->rotation.vector.x);
                } else {
                    func_00209458(&record->turnFrom.vector.x, &record->turnTo.vector.x, record->turnStep, record->turnCount,
                                  record->turnCurve[0], record->turnCurve[1], record->turnCurveType,
                                  &record->rotation.vector.x);
                }
                record->turnStep++;
                if (record->turnStep > record->turnCount) {
                    record->turnStep = -1;
                }
            }
            if (record->scaleStep != -1) {
                record->scale.vector.x = record->scaleFrom.vector.x
                                  + (record->scaleTo.vector.x - record->scaleFrom.vector.x) * record->scaleStep / record->scaleCount;
                record->scale.vector.y = record->scaleFrom.vector.y
                                  + (record->scaleTo.vector.y - record->scaleFrom.vector.y) * record->scaleStep / record->scaleCount;
                record->scale.vector.z = record->scaleFrom.vector.z
                                  + (record->scaleTo.vector.z - record->scaleFrom.vector.z) * record->scaleStep / record->scaleCount;
                record->scaleStep++;
                if (record->scaleStep > record->scaleCount) {
                    record->scaleStep = -1;
                }
            }
            break;
        }
        if (record->actor != 0) {
            XenoActorPushMatrix(record->actor);
        }
    }
    if (!(gameWork.flags & GAME_HIDE_RIDERS)) {
        for (j = 0; j < MAP_RECORD_COUNT; j++) {
            record = &uwamonoRecords[j];
            if (record->kind < MAP_KIND_ACTOR || record->kind > MAP_KIND_FLAG) {
                continue;
            }
            func_001FDA18(uwamonoRecords[uwamonoCurrentIndex].actor, j);
            if (record->riderSide != 0) {
                riders[0] = record->riders[0];
                riders[1] = record->riders[1];
                if (record->flags & 0x80000) {
                    riders[1]->visible = 0;
                    riders[0]->visible = 0;
                } else {
                    if (record->riderSide == 1) {
                        riders[0]->visible = 1;
                        riders[1]->visible = 0;
                        riders[0]->position.vector.x = record->position.vector.x + record->riderOffset.vector.x;
                        riders[0]->position.vector.y = record->position.vector.y + record->riderOffset.vector.y + record->height * 0.5f;
                        riders[0]->position.vector.z = record->position.vector.z + record->riderOffset.vector.z;
                        riders[0]->scale.vector.x = riders[0]->scale.vector.y = riders[0]->scale.vector.z
                            = uwamonoRecords[uwamonoCurrentIndex].size;
                    } else {
                        riders[0]->visible = 0;
                        riders[1]->visible = 1;
                        riders[1]->position.vector.x = record->position.vector.x + record->riderOffset.vector.x;
                        riders[1]->position.vector.y = record->position.vector.y + record->riderOffset.vector.y + record->height * 0.5f;
                        riders[1]->position.vector.z = record->position.vector.z + record->riderOffset.vector.z;
                        riders[1]->scale.vector.x = riders[1]->scale.vector.y = riders[1]->scale.vector.z
                            = uwamonoRecords[uwamonoCurrentIndex].size;
                    }
                    riders[0]->flags |= 4;
                    riders[1]->flags |= 4;
                }
            }
            if (record->talkMode != 0 && record->searchState != 1 && record->busy == 0
                && uwamonoCurrentIndex != -1) {
                if (record->lookLink == -1) {
                    point = record->lookPoint.vector;
                } else {
                    Get_MiddlePoint(&record->lookPoint.vector.x, &uwamonoRecords[record->lookLink].lookPoint.vector.x, 1, 2, &point.x);
                }
                if (uwamonoRecords[uwamonoCurrentIndex].size * 5.0f
                    <= Get_Distance(&uwamonoRecords[uwamonoCurrentIndex].actor->position, &point)) {
                    func_001FE5B0(j, 1);
                }
            }
        }
    }
    func_001FFA60();
    func_001FEF78();
    func_002025B0();
    return 1;
}

void pxySetFogColor(void *);
void pxySetFogDist(f32, f32, f32, f32);
void func_001BB3FC(void *);
extern unsigned char D_002B5E50[];
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
        pxySetFogDist(gameWork.extent.x, gameWork.extent.y, gameWork.extent.z, gameWork.extent.w);
        pxySetFogColor(gameWork.unmodeled_360);
    }
    func_001BB44C(D_002B7C88);
    if (D_0069B964 != 0) {
        pxySetTxyBlock(D_0069B964);
    }
    func_001BB3FC(D_002B5E50);
    pxySubmitModel(mainObject);
    if (mainObject->flags != 0 && D_0069B968 != 0) {
        func_001BB678(mainObject);
        func_001BB44C(D_002B7CA8);
        func_001BB47C(D_002B7CC8);
        pxySetTxyBlock(D_0069B964);
        func_001BB3FC(D_002B5E50);
        pxySubmitModel(D_0069B968);
    }
}
