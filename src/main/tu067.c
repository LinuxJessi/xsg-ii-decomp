#include "common.h"
#include "shared.h"
#include "main/cfePlugIn.h"
#include "main/elevator.h"
#include "main/tu072.h"
#include "main/tu074.h"
#include "main/tu075.h"
#include "main/tu077.h"
#include "main/tu079.h"
#include "main/tu082.h"
#include "main/tu114.h"
#include "main/xenoActor.h"

/* The XenoActor of a map object (xenoActor.c); only the fields this file touches. */
struct XenoActor {
    unsigned char unmodeled_000[0x10];
    Vector4 position;           /* +0x010 */
    unsigned char unmodeled_020[0x30];
    Vector4 rotation;           /* +0x050: y is the facing angle */
    Vector4 direction;          /* +0x060: y is the moving angle */
    unsigned char unmodeled_070[0x20];
    Vector4 scale;              /* +0x090 */
    unsigned char unmodeled_0a0[0x10];
    float radius;               /* +0x0B0 */
    unsigned char unmodeled_0b4[0x5D4 - 0xB4];
    int mode;                   /* +0x5D4: the motion XenoActorCheckMode compares against */
    unsigned char unmodeled_5d8[0x748 - 0x5D8];
    unsigned short hitFlags;    /* +0x748 */
    unsigned char unmodeled_74a[0x758 - 0x74A];
    int ground;                 /* +0x758: map block 0x100 entry of the ground (changeId) */
    unsigned char unmodeled_75c[0x768 - 0x75C];
    float probeRadius;          /* +0x768: set together with radius by setPlayer */
    unsigned char unmodeled_76c[0x798 - 0x76C];
    int kind;                   /* +0x798 */
};

/* The per-object state block of a map object record (record+0x334; tu085.c's UwamonoState). */
typedef struct UwamonoState {
    unsigned char unmodeled_00[0x5F];
    unsigned char mode;         /* +0x5F */
    unsigned char unmodeled_60[0x7C - 0x60];
    int id;                     /* +0x7C */
    short groundId;             /* +0x80: map block 0x100 entry of the actor's ground (changeId) */
    short footId;               /* +0x82: map block 0x700 entry for the footstep sound (footId native) */
    unsigned char unmodeled_84[0x37C0 - 0x84];
    Vector4 path[128];          /* +0x37C0: the points between the record and its target (uwamonoSetTarget) */
    short pathStep;             /* +0x3FC0 */
} UwamonoState;

/* A vector of a map object record (tu072's EnepcPoint): accessed through the union, with alias set 0,
 * so the record's stores keep their order. */
typedef union UwamonoVector {
    float coords[4];
    Vector4 vector;
} UwamonoVector;

/* One 0x870-byte record of uwamonoRecords (the table tu050.c declares as MapObjectRecord);
 * only the fields the natives below touch are named. */
typedef struct UwamonoRecord {
    short id;                   /* +0x000: the key uwamonoFindIndexByShortKey searches */
    unsigned char kind;         /* +0x002: MAP_KIND_* */
    unsigned char unmodeled_003;
    int key;                    /* +0x004 */
    unsigned char unmodeled_008[8];
    UwamonoVector position;     /* +0x010 */
    unsigned char unmodeled_020[0x20];
    float rotation[4];          /* +0x040: read as an array by objxRotate */
    Vector4 scale;              /* +0x050 */
    AlignedVector4 pivot;       /* +0x060: quadword-aligned, makes the record 16-aligned */
    float transparency;         /* +0x070: percent, 0 opaque (transparent native) */
    unsigned char unmodeled_074[0xC];
    Vector4 moveFrom;           /* +0x080 */
    Vector4 moveTo;             /* +0x090 */
    UwamonoVector turnFrom;     /* +0x0A0 */
    UwamonoVector turnTo;       /* +0x0B0 */
    Vector4 scaleFrom;          /* +0x0C0 */
    Vector4 scaleTo;            /* +0x0D0 */
    float transparencyFrom;     /* +0x0E0 */
    float transparencyTo;       /* +0x0E4 */
    unsigned char unmodeled_0e8[0x210 - 0xE8];
    short moveEndMode;          /* +0x210: mode for XenoActorCheckMode when the move ends, -1 for none */
    short moveStep;             /* +0x212 */
    short moveCount;            /* +0x214 */
    short turnStep;             /* +0x216 */
    short turnCount;            /* +0x218 */
    short scaleStep;            /* +0x21A */
    short scaleCount;           /* +0x21C */
    short transparencyStep;     /* +0x21E */
    short transparencyCount;    /* +0x220 */
    unsigned char unmodeled_222[0x260 - 0x222];
    signed char rotationOrder;  /* +0x260: the order the rotation angles are applied in (setRotateTurn) */
    unsigned char unmodeled_261[0x2F0 - 0x261];
    float matrix[4][4];         /* +0x2F0: the placement matrix, its last row the position */
    struct XenoActor *actor;    /* +0x330 */
    UwamonoState *motions;      /* +0x334 */
    unsigned char unmodeled_338[0x546 - 0x338];
    short parent;               /* +0x546: index of the record this one is attached to or looks at */
    short attached;             /* +0x548 */
    unsigned char unmodeled_54a[6];
    UwamonoVector savedPosition; /* +0x550: offset of the record from its parent */
    UwamonoVector savedRotation; /* +0x560 */
    int flags;                  /* +0x570 */
    unsigned char unmodeled_574[0xC];
    float extent;               /* +0x580: the player's radius (setPlayer) */
    float height;               /* +0x584 */
    unsigned char unmodeled_588[9];
    signed char locaterType;    /* +0x591: setLocater's type argument, 10 for the second locater */
    unsigned char unmodeled_592[0x618 - 0x592];
    int lookLink;               /* +0x618: index of the record this one looks at */
    unsigned char unmodeled_61c[0x10];
    signed char riderSide;      /* +0x62C */
    unsigned char unmodeled_62d[0xB];
    void *effect;               /* +0x638: sef effect record */
    void *riders[2];            /* +0x63C: released through sefReleaseRecordHandle */
    int effectOwner;            /* +0x644: index of the record whose effect this one carries */
    unsigned char unmodeled_648[0x870 - 0x648];
} UwamonoRecord;

/* UwamonoRecord.kind (tu050.c's MapObjectRecord.kind). */
enum {
    MAP_KIND_FREE = 0,
    MAP_KIND_ACTOR = 1,
    MAP_KIND_MODEL = 2,
    MAP_KIND_FLAG = 3,
    MAP_KIND_ACTOR_STATIC = 4,  /* an actor that is not moved by uwamonoUpdateRecord */
    MAP_KIND_PART = 5,          /* a part attached to the player (tu051.c) */
    MAP_KIND_EFFECT = 6,        /* the record carries a sef effect at +0x638 */
    MAP_KIND_MOVER = 7,
    MAP_KIND_MODEL_REQUEST = 8  /* create only: uwamonoCreateRecord turns it into a model or frees it */
};

#define MAP_RECORD_COUNT 0x100

extern UwamonoRecord uwamonoRecords[];
extern int uwamonoCurrentIndex;

/* A bit of gameWork.control; tu051.h cannot be included here, its record types clash with this file's. */
#define CONTROL_SCRIPTED 0x8000 /* include/main/tu051.h */

/* The game work block (tu050.c's MainWork, tu051.c's CameraState); only the fields this file uses. */
typedef struct GameWork {
    unsigned char unmodeled_000[6];
    unsigned short loadedFromSave;  /* +0x006: 1 when the game was loaded from a save (getParam 7) */
    int control;                    /* +0x008: CONTROL_SCRIPTED while a script controls the player */
    unsigned char unmodeled_00c[0x10];
    int stateArg;                   /* +0x01C: tu050.c's stateArg */
    unsigned char unmodeled_020[0x2D0 - 0x20];
    float frameScale;               /* +0x2D0: elapsed frames, scales the per-frame movement */
    unsigned char unmodeled_2d4[4];
    float playerParams[5][2];       /* +0x2D8: setPlayer arguments 4 to 8 (percent) in [i][0] */
    int climbFrames;                /* +0x300: setPlayer argument 9, the ladder-step divisor (tu075.c) */
    unsigned char unmodeled_304[0x374 - 0x304];
    float playerRate;               /* +0x374: setPlayer argument 10 (ten-thousandths) */
    unsigned char unmodeled_378[4];
    float playerScale;              /* +0x37C: setPlayer argument 11 (percent) */
} GameWork;

extern GameWork gameWork;

/* A one-member union: the original reads the vector it points at with alias set 0 (tu050.c's MapVector). */
typedef union MapVector {
    Vector4 vector;
} MapVector;

/* Script natives: argument count, n-th argument, nonzero when argument n is omitted, wrong-count error. */
int func_A9F0A8(int call);
int func_A9F0C0(int call, int index);
int func_A9F1C0(int call, int index);
void func_A9CC70(int call, int error_code);

int uwamonoFindIndexByShortKey(short key);
void uwamonoStartTransparency(int index, int frames, float value);
void func_001F0750(int index, MapVector *scale, int frames);
void uwamonoAddMoveStep(int index, float step, float speed);
void uwamonoAddRotateStep(int index, float step, float speed);
void uwamonoClearMoveResetWord(int index);
void uwamonoClearRotateResetWord(int index);
int uwamonoSetTarget(int index, int target, short flag);
void uwamonoStartMove(int index, MapVector *destination, int frames, signed char hasMotion, int motion, signed char mode);
int uwamonoCreateRecord(int key, int kind, int entry, int fourth, int fifth);
void uwamonoReleaseRecord(int index);
void sefSetRecordOwner(void *record, void *owner);
void sefSetRecordOwnerMarked(void *record, void *owner);
void sefReleaseRecordHandle(void *record);
void XenoActorPushMatrix(struct XenoActor *actor);
void XenoActorCheckMode(struct XenoActor *actor, int expected_mode);
void XenoActorSetMode(struct XenoActor *actor, int mode);
void XenoActorAttachChildToJoint(struct XenoActor *parent, struct XenoActor *child, int joint);
void XenoActorReleaseChild(struct XenoActor *parent, struct XenoActor *child);
void uwamonoSetMotion(int index, int motion, float speed);
float Get_Angle(const Vector4 *first, const Vector4 *second);
float func_0020A968(int index, Vector4 *position, int id, int groundId, int flags);
int srsEffectKeyFromName(signed char *name);
/* Script natives: type of argument n (3 is a string), string of a string token. */
int func_A9F140(int call, int index);
char *func_A9F238(int call, int index);

/* Index of the first used record (kind != MAP_KIND_FREE) whose id is key; -1 when there is none. */
int uwamonoFindIndexByShortKey(short key)
{
    UwamonoRecord *record = uwamonoRecords;
    int index;

    for (index = 0; index < MAP_RECORD_COUNT; index++, record++) {
        if (record->kind != MAP_KIND_FREE && record->id == key) {
            return index;
        }
    }
    return -1;
}

/* A second copy of uwamonoFindIndexByShortKey. */
int func_001EF650(short key)
{
    UwamonoRecord *record = uwamonoRecords;
    int index;

    for (index = 0; index < MAP_RECORD_COUNT; index++, record++) {
        if (record->kind != MAP_KIND_FREE && record->id == key) {
            return index;
        }
    }
    return -1;
}

/* The model pack mapObjectTable points to (xenoActor.c's PxyHeader); only the field this file reads. */
typedef struct PxyHeader {
    unsigned char unmodeled_00[0x68];
    int entry_offset;           /* +0x68: nml_model_set.c offset68: offset of the 0x40-byte entry matrices */
} PxyHeader;

/* One 0x40-byte row of the table at mapObjectTable: a placement matrix, its rotation rows then the position. */
typedef struct ObjTableEntry {
    float rotation[3][4];       /* +0x00 */
    Vector4 position;           /* +0x30 */
} ObjTableEntry;

extern PxyHeader *mapObjectTable;
int pxyFindEntryIndex(PxyHeader *header, const char *name);

void func_001EF6A0(Vector4 *destination, int index)
{
    ObjTableEntry *entry = (ObjTableEntry *)((unsigned char *)mapObjectTable + mapObjectTable->entry_offset) + index;

    destination->x = entry->position.x;
    destination->y = entry->position.y;
    destination->z = entry->position.z;
    destination->w = 1.0f;
}

float func_002A1DE0(float value);   /* asinf */
double func_002A1DB0(double value); /* sqrt */
float atan2f(float y, float x);

/* Euler angles of the rotation rows of table entry index: x from the pitch row, y from the
 * heading of the third row, z from the roll (reflected across pi when the up row points down). */
void func_001EF6E0(Vector4 *angles, int index)
{
    ObjTableEntry *entry = (ObjTableEntry *)((unsigned char *)mapObjectTable + mapObjectTable->entry_offset) + index;
    float roll;

    if (entry->rotation[1][1] >= 0.0f) {
        double sine = entry->rotation[0][1];

        roll = func_002A1DE0(sine / func_002A1DB0(1.0f - entry->rotation[2][1] * entry->rotation[2][1]));
    } else {
        double sine = entry->rotation[0][1];

        roll = 3.1415927f - func_002A1DE0(sine / func_002A1DB0(1.0f - entry->rotation[2][1] * entry->rotation[2][1]));
    }
    angles->x = func_002A1DE0(entry->rotation[2][1]);
    angles->y = atan2f(entry->rotation[2][0], entry->rotation[2][2]);
    angles->z = roll;
    angles->w = 1.0f;
}

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

/* Native "getParam": [key], param. With a key: the position (0..2, hundredths) or the rotation
 * (3..5, hundredths of a degree) of that record, for MAP_KIND_ACTOR, MODEL and ACTOR_STATIC.
 * Without a key: 6 the current actor's kind, 7 the state argument (-2 when the game was loaded
 * from a save), 8 bit 11 of the control word, 9 the area group of the current record's key.
 * -1 when there is no such value. */
int objxGetParam(int call)
{
    UwamonoRecord *current = &uwamonoRecords[uwamonoCurrentIndex];
    int result;
    int index;
    int key;
    int param;

    result = -1;
    if (func_A9F0A8(call) != 2) {
        func_A9CC70(call, 2);
        return 0;
    }
    if (func_A9F1C0(call, 1) == 0) {
        key = func_A9F0C0(call, 1);
        index = uwamonoFindIndexByShortKey((short) key);
    } else {
        index = -1;
    }
    param = func_A9F0C0(call, 2);
    if (index == -1) {
        switch (param) {
        case 6:
            result = uwamonoRecords[uwamonoCurrentIndex].actor->kind;
            break;
        case 7:
            result = gameWork.loadedFromSave != 0 ? -2 : gameWork.stateArg;
            break;
        case 8:
            result = (gameWork.control & 0x800) >> 11;
            break;
        case 9:
            key = current->key;
            if (key >= 0x10001 && key <= 0x10007) {
                result = 0;
            }
            if (key >= 0x10009 && key <= 0x1000D) {
                result = 1;
            }
            if (key >= 0x1000F && key <= 0x10014) {
                result = 2;
            }
            if (key >= 0x10015 && key <= 0x1001A) {
                result = 3;
            }
            if (key >= 0x1001B && key <= 0x10021) {
                result = 4;
            }
            if (key >= 0x10022 && key <= 0x10024) {
                result = 5;
            }
            if (key >= 0x10025 && key <= 0x1002B) {
                result = 6;
            }
            if (key == 0x14002) {
                result = 7;
            }
            if (key == 0x14003) {
                result = 8;
            }
            if (key == 0x14005) {
                result = 9;
            }
            break;
        }
    } else {
        switch (uwamonoRecords[index].kind) {
        case MAP_KIND_ACTOR:
        case MAP_KIND_MODEL:
        case MAP_KIND_ACTOR_STATIC:
            switch (param) {
            case 0:
                result = (int) (uwamonoRecords[index].position.coords[0] * 100.0f);
                break;
            case 1:
                result = (int) (uwamonoRecords[index].position.coords[1] * 100.0f);
                break;
            case 2:
                result = (int) (uwamonoRecords[index].position.coords[2] * 100.0f);
                break;
            case 3:
                result = (int) (uwamonoRecords[index].rotation[0] / 3.1415927f * 180.0f * 100.0f);
                break;
            case 4:
                result = (int) (uwamonoRecords[index].rotation[1] / 3.1415927f * 180.0f * 100.0f);
                break;
            case 5:
                result = (int) (uwamonoRecords[index].rotation[2] / 3.1415927f * 180.0f * 100.0f);
                break;
            }
            break;
        }
    }
    return result;
}

/* Native "move": key, frames, x, y, z (percent; an omitted component keeps the record's position),
 * [motion], [mode]: starts the record moving to the point. */
int objxMove(int call)
{
    int key;
    int index;
    int frames;
    int motion;
    signed char mode;
    MapVector destination;

    if (func_A9F0A8(call) < 5) {
        func_A9CC70(call, 2);
        return 0;
    }
    key = func_A9F0C0(call, 1);
    index = uwamonoFindIndexByShortKey((short) key);
    if (index == -1) {
        return 1;
    }
    frames = func_A9F1C0(call, 2) != 0 ? 0 : func_A9F0C0(call, 2);
    destination.vector.x = func_A9F1C0(call, 3) != 0 ? uwamonoRecords[index].position.coords[0] : (float) func_A9F0C0(call, 3) / 100.0f;
    destination.vector.y = func_A9F1C0(call, 4) != 0 ? uwamonoRecords[index].position.coords[1] : (float) func_A9F0C0(call, 4) / 100.0f;
    destination.vector.z = func_A9F1C0(call, 5) != 0 ? uwamonoRecords[index].position.coords[2] : (float) func_A9F0C0(call, 5) / 100.0f;
    motion = func_A9F1C0(call, 6) != 0 ? -1 : func_A9F0C0(call, 6);
    mode = func_A9F1C0(call, 7) != 0 ? 0 : (signed char) func_A9F0C0(call, 7);
    uwamonoStartMove(index, &destination, frames, motion != -1, motion, mode);
    return 1;
}

/* Moves record index to destination over frames (at once when frames is 0, also on the actor of
 * an actor record). With a motion an actor record also turns towards the destination over 15
 * frames, the shorter way round. Flag, part and free records are not moved. */
void uwamonoStartMove(int index, MapVector *destination, int frames, signed char hasMotion, int motion, signed char mode)
{
    UwamonoRecord *record = &uwamonoRecords[index];
    float angle;

    switch (record->kind) {
    case MAP_KIND_ACTOR:
    case MAP_KIND_MODEL:
    case MAP_KIND_ACTOR_STATIC:
    case MAP_KIND_EFFECT:
    case MAP_KIND_MOVER:
        if (frames == 0) {
            if (hasMotion == 1) {
                record->position.vector.y = func_0020A968(index, &destination->vector, record->motions->id, record->motions->groundId, 0);
            }
            if (record->kind == MAP_KIND_ACTOR_STATIC || record->kind == MAP_KIND_ACTOR) {
                struct XenoActor *actor = record->actor;

                actor->position.x = record->position.vector.x = destination->vector.x;
                actor->position.y = record->position.vector.y = destination->vector.y;
                actor->position.z = record->position.vector.z = destination->vector.z;
                XenoActorPushMatrix(actor);
            } else {
                record->position.vector.x = destination->vector.x;
                record->position.vector.y = destination->vector.y;
                record->position.vector.z = destination->vector.z;
            }
            record->moveCount = 0;
            record->moveStep = -1;
        } else {
            record->moveFrom.x = record->position.vector.x;
            record->moveFrom.y = record->position.vector.y;
            record->moveFrom.z = record->position.vector.z;
            record->moveTo.x = destination->vector.x;
            record->moveTo.y = destination->vector.y;
            record->moveTo.z = destination->vector.z;
            record->moveCount = frames;
            record->moveStep = 0;
            if (hasMotion == 1) {
                record->flags |= 0x8000000;
            } else {
                record->flags &= ~0x8000000;
            }
            if (record->kind == MAP_KIND_ACTOR_STATIC || record->kind == MAP_KIND_ACTOR) {
                if (motion != -1) {
                    XenoActorCheckMode(record->actor, motion);
                    record->turnTo.coords[0] = record->turnFrom.coords[0] = record->rotation[0];
                    record->turnFrom.coords[1] = record->rotation[1];
                    record->turnFrom.coords[2] = record->rotation[2];
                    angle = Get_Angle(&record->moveFrom, &record->moveTo);
                    record->turnTo.coords[1] = angle;
                    if (angle < record->turnFrom.coords[1]) {
                        if (record->turnFrom.coords[1] - angle >= 3.1415927f) {
                            record->turnTo.coords[1] = angle + 6.2831855f;
                        }
                    }
                    if (record->turnFrom.coords[1] < record->turnTo.coords[1]) {
                        if (record->turnTo.coords[1] - record->turnFrom.coords[1] >= 3.1415927f) {
                            record->turnTo.coords[1] = record->turnTo.coords[1] - 6.2831855f;
                        }
                    }
                    record->turnCount = 15;
                    record->turnTo.coords[2] = record->turnFrom.coords[2];
                    record->turnStep = 0;
                }
                record->moveEndMode = mode == 1 ? 0 : -1;
            }
        }
        break;
    }
}

/* Native "rotate": key, frames, x, y, z (degrees; an omitted component keeps the record's rotation):
 * turns the record to the angles over frames (at once when frames is 0, also on the actor of an
 * actor record). Flag, part, mover and free records are not turned. */
int objxRotate(int call)
{
    int key;
    int index;
    int frames;
    float angle[3];
    float current[3];

    if (func_A9F0A8(call) != 5) {
        func_A9CC70(call, 2);
        return 0;
    }
    key = func_A9F0C0(call, 1);
    index = uwamonoFindIndexByShortKey((short) key);
    frames = func_A9F1C0(call, 2) != 0 ? 0 : func_A9F0C0(call, 2);
    angle[0] = func_A9F1C0(call, 3) != 0 ? uwamonoRecords[index].rotation[0] : (float) func_A9F0C0(call, 3) / 100.0f / 180.0f * 3.1415927f;
    angle[1] = func_A9F1C0(call, 4) != 0 ? uwamonoRecords[index].rotation[1] : (float) func_A9F0C0(call, 4) / 100.0f / 180.0f * 3.1415927f;
    angle[2] = func_A9F1C0(call, 5) != 0 ? uwamonoRecords[index].rotation[2] : (float) func_A9F0C0(call, 5) / 100.0f / 180.0f * 3.1415927f;
    current[0] = uwamonoRecords[index].rotation[0];
    current[1] = uwamonoRecords[index].rotation[1];
    current[2] = uwamonoRecords[index].rotation[2];
    switch (uwamonoRecords[index].kind) {
    case MAP_KIND_ACTOR:
    case MAP_KIND_MODEL:
    case MAP_KIND_ACTOR_STATIC:
    case MAP_KIND_EFFECT:
        if (frames == 0) {
            if (uwamonoRecords[index].kind == MAP_KIND_ACTOR_STATIC || uwamonoRecords[index].kind == MAP_KIND_ACTOR) {
                struct XenoActor *actor = uwamonoRecords[index].actor;

                actor->rotation.x = actor->direction.x = uwamonoRecords[index].rotation[0] = angle[0];
                actor->rotation.y = actor->direction.y = uwamonoRecords[index].rotation[1] = angle[1];
                actor->rotation.z = actor->direction.z = uwamonoRecords[index].rotation[2] = angle[2];
            } else {
                uwamonoRecords[index].rotation[0] = angle[0];
                uwamonoRecords[index].rotation[1] = angle[1];
                uwamonoRecords[index].rotation[2] = angle[2];
            }
            uwamonoRecords[index].turnStep = -1;
            uwamonoRecords[index].turnCount = 0;
        } else {
            uwamonoRecords[index].turnFrom.coords[0] = current[0];
            uwamonoRecords[index].turnFrom.coords[1] = current[1];
            uwamonoRecords[index].turnFrom.coords[2] = current[2];
            uwamonoRecords[index].turnTo.coords[0] = angle[0];
            uwamonoRecords[index].turnTo.coords[1] = angle[1];
            uwamonoRecords[index].turnTo.coords[2] = angle[2];
            uwamonoRecords[index].turnCount = frames;
            uwamonoRecords[index].turnStep = 0;
        }
        break;
    }
    return 1;
}

/* Native "scale": key, frames, x, y, z (percent; an omitted component keeps the record's scale). */
int objxScale(int call)
{
    int key;
    int index;
    int frames;
    MapVector scale;

    if (func_A9F0A8(call) != 5) {
        func_A9CC70(call, 2);
        return 0;
    }
    key = func_A9F0C0(call, 1);
    index = uwamonoFindIndexByShortKey((short) key);
    frames = func_A9F1C0(call, 2) != 0 ? 0 : func_A9F0C0(call, 2);
    scale.vector.x = func_A9F1C0(call, 3) != 0 ? uwamonoRecords[index].scale.x : (float) func_A9F0C0(call, 3) / 100.0f;
    scale.vector.y = func_A9F1C0(call, 4) != 0 ? uwamonoRecords[index].scale.y : (float) func_A9F0C0(call, 4) / 100.0f;
    scale.vector.z = func_A9F1C0(call, 5) != 0 ? uwamonoRecords[index].scale.z : (float) func_A9F0C0(call, 5) / 100.0f;
    func_001F0750(index, &scale, frames);
    return 1;
}

/* Scales record index to scale over frames (at once when frames is 0, also on the actor of an actor
 * record). Flag, part, mover and free records are not scaled. */
void func_001F0750(int index, MapVector *scale, int frames)
{
    UwamonoRecord *record = &uwamonoRecords[index];
    float current[3];

    current[0] = record->scale.x;
    current[1] = record->scale.y;
    current[2] = record->scale.z;
    switch (record->kind) {
    case MAP_KIND_ACTOR:
    case MAP_KIND_MODEL:
    case MAP_KIND_ACTOR_STATIC:
    case MAP_KIND_EFFECT:
    case MAP_KIND_MODEL_REQUEST:
        if (frames == 0) {
            if (uwamonoRecords[index].kind == MAP_KIND_ACTOR_STATIC || uwamonoRecords[index].kind == MAP_KIND_ACTOR) {
                struct XenoActor *actor = record->actor;

                actor->scale.x = record->scale.x = scale->vector.x;
                actor->scale.y = record->scale.y = scale->vector.y;
                actor->scale.z = record->scale.z = scale->vector.z;
                actor->scale.w = record->scale.w = 1.0f;
            } else {
                record->scale.x = scale->vector.x;
                record->scale.y = scale->vector.y;
                record->scale.z = scale->vector.z;
                record->scale.w = 1.0f;
            }
            record->scaleCount = 0;
            record->scaleStep = -1;
        } else {
            record->scaleFrom.x = current[0];
            record->scaleFrom.y = current[1];
            record->scaleFrom.z = current[2];
            record->scaleFrom.w = 1.0f;
            record->scaleTo.x = scale->vector.x;
            record->scaleTo.y = scale->vector.y;
            record->scaleTo.z = scale->vector.z;
            record->scaleCount = frames;
            record->scaleTo.w = 1.0f;
            record->scaleStep = 0;
        }
        break;
    }
}

/* Native "transparent": key, frames, transparency (percent; an omitted transparency keeps the record's,
 * omitted frames means at once). */
int objxTransparent(int call)
{
    int key;
    int index;
    int frames;
    float value;

    if (func_A9F0A8(call) != 3) {
        func_A9CC70(call, 2);
        return 0;
    }
    key = func_A9F0C0(call, 1);
    index = uwamonoFindIndexByShortKey((short) key);
    if (func_A9F1C0(call, 2) == 0) {
        frames = func_A9F0C0(call, 2);
    } else {
        frames = 0;
    }
    if (func_A9F1C0(call, 3) != 0) {
        value = uwamonoRecords[index].transparency;
    } else {
        value = (float) func_A9F0C0(call, 3);
    }
    uwamonoStartTransparency(index, frames, value);
    return 1;
}

/* Changes the transparency of record index to value over frames, at once when frames is 0. */
void uwamonoStartTransparency(int index, int frames, float value)
{
    UwamonoRecord *record = &uwamonoRecords[index];

    if (frames == 0) {
        record->transparency = value;
        record->transparencyStep = -1;
        record->transparencyCount = -1;
    } else {
        record->transparencyCount = frames;
        record->transparencyTo = value;
        record->transparencyStep = 0;
        record->transparencyFrom = record->transparency;
    }
}

/* Native "offsetPivot": key, x, y, z (percent): the pivot of the record; an omitted component is kept. */
int objxOffsetPivot(int call)
{
    int key;
    int index;
    UwamonoRecord *record;

    if (func_A9F0A8(call) != 4) {
        func_A9CC70(call, 2);
        return 0;
    }
    key = func_A9F0C0(call, 1);
    index = uwamonoFindIndexByShortKey((short) key);
    record = &uwamonoRecords[index];
    record->pivot.x = func_A9F1C0(call, 2) != 0 ? record->pivot.x : (float) func_A9F0C0(call, 2) / 100.0f;
    record->pivot.y = func_A9F1C0(call, 3) != 0 ? record->pivot.y : (float) func_A9F0C0(call, 3) / 100.0f;
    record->pivot.z = func_A9F1C0(call, 4) != 0 ? record->pivot.z : (float) func_A9F0C0(call, 4) / 100.0f;
    record->pivot.w = 1.0f;
    return 1;
}

/* Native "enableFlags": key, bits, [target]: sets flag bits on the record; target 99 sets them on every used
 * record, another target on every record of that kind. */
int objxEnableFlags(int call)
{
    int key;
    int index;
    int bits;
    int target;
    int i;

    if (func_A9F0A8(call) < 2) {
        func_A9CC70(call, 2);
        return 0;
    }
    key = func_A9F0C0(call, 1);
    index = uwamonoFindIndexByShortKey((short) key);
    bits = func_A9F0C0(call, 2);
    if (func_A9F1C0(call, 3) != 0) {
        target = -1;
    } else {
        target = func_A9F0C0(call, 3);
    }
    if (target == -1) {
        uwamonoRecords[index].flags |= bits;
    } else if (target == 99) {
        for (i = 0; i < MAP_RECORD_COUNT; i++) {
            if (uwamonoRecords[i].kind != MAP_KIND_FREE) {
                uwamonoRecords[i].flags |= bits;
            }
        }
    } else {
        for (i = 0; i < MAP_RECORD_COUNT; i++) {
            if (uwamonoRecords[i].kind == target) {
                uwamonoRecords[i].flags |= bits;
            }
        }
    }
    return 1;
}

/* Native "disableFlags": key, bits, [target]: clears flag bits on the record (every record of kind target when
 * target is given); clearing bit 0x20000 also releases the record's rider effects. */
int objxDisableFlags(int call)
{
    int key;
    int index;
    int bits;
    int target;
    int first;
    int last;
    int i;
    UwamonoRecord *record;

    if (func_A9F0A8(call) < 2) {
        func_A9CC70(call, 2);
        return 0;
    }
    key = func_A9F0C0(call, 1);
    index = uwamonoFindIndexByShortKey((short) key);
    bits = func_A9F0C0(call, 2);
    target = func_A9F1C0(call, 3) != 0 ? -1 : func_A9F0C0(call, 3);
    if (target == -1) {
        first = index;
        last = index + 1;
    } else {
        first = 0;
        last = MAP_RECORD_COUNT;
    }
    for (i = first; i < last; i++) {
        if (uwamonoRecords[i].kind == target || target == -1) {
            uwamonoRecords[i].flags &= ~bits;
            if (bits & 0x20000) {
                record = &uwamonoRecords[i];
                record->riderSide = 0;
                if (record->riders[0] != 0) {
                    sefReleaseRecordHandle(record->riders[0]);
                }
                if (record->riders[1] != 0) {
                    sefReleaseRecordHandle(record->riders[1]);
                }
                record->riders[0] = record->riders[1] = 0;
            }
        }
    }
    return 1;
}

/* Native "setRotateTurn": key, order: the rotation order of the record. */
int objxSetRotateTurn(int call)
{
    int key;
    int index;

    if (func_A9F0A8(call) != 2) {
        func_A9CC70(call, 2);
        return 0;
    }
    key = func_A9F0C0(call, 1);
    index = uwamonoFindIndexByShortKey((short) key);
    uwamonoRecords[index].rotationOrder = func_A9F0C0(call, 2);
    return 1;
}

/* Native "setParent": key, [other], [attach]: with attach, the record's effect is owned by other's actor
 * (or by other's record when it has no actor); otherwise other becomes the record's target. */
int objxSetParent(int call)
{
    int key;
    int index;
    int other;

    if (func_A9F0A8(call) < 2) {
        func_A9CC70(call, 2);
        return 0;
    }
    key = func_A9F0C0(call, 1);
    index = uwamonoFindIndexByShortKey((short) key);
    if (index == -1) {
        return 1;
    }
    other = func_A9F1C0(call, 2) != 0 ? -1 : uwamonoFindIndexByShortKey((short) func_A9F0C0(call, 2));
    if (func_A9F1C0(call, 3) == 0) {
        if (uwamonoRecords[other].actor != 0) {
            sefSetRecordOwner(uwamonoRecords[index].effect, uwamonoRecords[other].actor);
            XenoActorPushMatrix(uwamonoRecords[other].actor);
            uwamonoRecords[other].effectOwner = index;
        } else {
            sefSetRecordOwnerMarked(uwamonoRecords[index].effect, &uwamonoRecords[other]);
            uwamonoRecords[other].effectOwner = index;
        }
    } else {
        uwamonoSetTarget(index, other, 1);
    }
    return 1;
}

void Get_MiddlePoint(const float *start, const float *following, short point_index, short point_count, float *result);
void uwamonoSetAction(int slot_index, int action);

/* Makes target the parent of record index. When either record is an actor record the record walks
 * to the target: the 128 points of the path are prepared, the state block starts action 5.
 * Otherwise the record keeps its offset from the target; with no target (-1) and flag 1 it takes its
 * position and rotation from its placement matrix instead. Returns 1. */
int uwamonoSetTarget(int index, int target, short flag)
{
    int i;
    float roll;

    if (uwamonoRecords[index].kind == MAP_KIND_ACTOR_STATIC || uwamonoRecords[index].kind == MAP_KIND_ACTOR
        || uwamonoRecords[target].kind == MAP_KIND_ACTOR_STATIC || uwamonoRecords[target].kind == MAP_KIND_ACTOR) {
        uwamonoRecords[index].parent = target;
        for (i = 0; i < 128; i++) {
            Get_MiddlePoint(uwamonoRecords[index].position.coords, uwamonoRecords[target].position.coords, (short) i, 128,
                            &uwamonoRecords[index].motions->path[i].x);
        }
        uwamonoRecords[index].motions->mode = 1;
        uwamonoRecords[index].motions->pathStep = 0;
        uwamonoSetAction(index, 5);
        return 1;
    }
    if (target == -1) {
        if (flag == 1) {
            uwamonoRecords[index].position.coords[0] = uwamonoRecords[index].matrix[3][0];
            uwamonoRecords[index].position.coords[1] = uwamonoRecords[index].matrix[3][1];
            uwamonoRecords[index].position.coords[2] = uwamonoRecords[index].matrix[3][2];
            uwamonoRecords[index].position.coords[3] = 1.0f;
            if (uwamonoRecords[index].matrix[1][1] >= 0.0f) {
                double sine = uwamonoRecords[index].matrix[0][1];

                roll = func_002A1DE0(sine / func_002A1DB0(1.0f - uwamonoRecords[index].matrix[2][1] * uwamonoRecords[index].matrix[2][1]));
            } else {
                double sine = uwamonoRecords[index].matrix[0][1];

                roll = 3.1415927f - func_002A1DE0(sine / func_002A1DB0(1.0f - uwamonoRecords[index].matrix[2][1] * uwamonoRecords[index].matrix[2][1]));
            }
            uwamonoRecords[index].rotation[0] = func_002A1DE0(uwamonoRecords[index].matrix[2][1]);
            uwamonoRecords[index].rotation[1] = atan2f(uwamonoRecords[index].matrix[2][0], uwamonoRecords[index].matrix[2][2]);
            uwamonoRecords[index].rotation[2] = roll;
            uwamonoRecords[index].rotation[3] = 1.0f;
        }
    } else {
        uwamonoRecords[index].savedPosition.coords[0] = uwamonoRecords[index].position.coords[0] - uwamonoRecords[target].position.coords[0];
        uwamonoRecords[index].savedPosition.coords[1] = uwamonoRecords[index].position.coords[1] - uwamonoRecords[target].position.coords[1];
        uwamonoRecords[index].savedPosition.coords[2] = uwamonoRecords[index].position.coords[2] - uwamonoRecords[target].position.coords[2];
        uwamonoRecords[index].savedRotation.coords[0] = uwamonoRecords[index].rotation[0] - uwamonoRecords[target].rotation[0];
        uwamonoRecords[index].savedRotation.coords[1] = uwamonoRecords[index].rotation[1] - uwamonoRecords[target].rotation[1];
        uwamonoRecords[index].savedRotation.coords[2] = uwamonoRecords[index].rotation[2] - uwamonoRecords[target].rotation[2];
    }
    uwamonoRecords[index].parent = target;
    uwamonoRecords[index].attached = flag;
    return 1;
}

/* Native "setPair": key, otherKey: links the two records to look at each other; when the other record is
 * an effect (MAP_KIND_EFFECT) the effect is owned by this record's actor instead. */
int objxSetPair(int call)
{
    int key;
    int index[2];
    UwamonoRecord *record;

    if (func_A9F0A8(call) != 2) {
        func_A9CC70(call, 2);
        return 0;
    }
    key = func_A9F0C0(call, 1);
    index[0] = uwamonoFindIndexByShortKey((short) key);
    key = func_A9F0C0(call, 2);
    index[1] = uwamonoFindIndexByShortKey((short) key);
    if (uwamonoRecords[index[1]].kind == MAP_KIND_EFFECT) {
        sefSetRecordOwner(uwamonoRecords[index[1]].effect, uwamonoRecords[index[0]].actor);
    } else {
        record = &uwamonoRecords[index[0]];
        record->lookLink = index[1];
        uwamonoRecords[index[1]].lookLink = index[0];
    }
    return 1;
}

/* Native: key, step (tenths), speed (percent); step -1 clears the move steps instead. */
int setStepMove(int call)
{
    int key;
    int index;
    float step;
    float speed;

    if (func_A9F0A8(call) != 3) {
        func_A9CC70(call, 2);
        return 0;
    }
    key = func_A9F0C0(call, 1);
    index = uwamonoFindIndexByShortKey((short) key);
    step = (float) func_A9F0C0(call, 2) / 10.0f;
    speed = (float) func_A9F0C0(call, 3) / 100.0f;
    if (func_A9F0C0(call, 2) == -1) {
        uwamonoClearMoveResetWord(index);
    } else {
        uwamonoAddMoveStep(index, step, speed);
    }
    return 1;
}

/* Native: key, step (tenths), speed (percent); step -1 clears the rotate steps instead. */
int setStepRotate(int call)
{
    int key;
    int index;
    float step;
    float speed;

    if (func_A9F0A8(call) != 3) {
        func_A9CC70(call, 2);
        return 0;
    }
    key = func_A9F0C0(call, 1);
    index = uwamonoFindIndexByShortKey((short) key);
    step = (float) func_A9F0C0(call, 2) / 10.0f;
    speed = (float) func_A9F0C0(call, 3) / 100.0f;
    if (func_A9F0C0(call, 2) == -1) {
        uwamonoClearRotateResetWord(index);
    } else {
        uwamonoAddRotateStep(index, step, speed);
    }
    return 1;
}

/* One 0x870-byte record of uwamonoSlotTable (uwamonoRecords + 0x330; tu085.c's UwamonoSlot):
 * the record's actor and state block; the rest of the record is not touched here. */
typedef struct UwamonoSlot {
    struct XenoActor *actor;    /* +0x00 */
    UwamonoState *state;        /* +0x04 */
    unsigned char unmodeled_08[0x870 - 8];
} UwamonoSlot;

int mapFindBlockEntry(int id, int index);
extern UwamonoSlot uwamonoSlotTable[];

/* Native "changeId": key, groundId: the map block 0x100 entry of the record's ground. */
int objxChangeId(int call)
{
    int key;
    int index;
    int groundId;
    int ground;
    UwamonoSlot *slot;

    if (func_A9F0A8(call) != 2) {
        func_A9CC70(call, 2);
        return 0;
    }
    key = func_A9F0C0(call, 1);
    index = uwamonoFindIndexByShortKey((short) key);
    groundId = func_A9F0C0(call, 2);
    ground = mapFindBlockEntry(0x100, groundId);
    slot = &uwamonoSlotTable[index];
    slot->actor->ground = ground;
    slot->state->groundId = groundId;
    return 1;
}

/* Native "footId": key, id: the map block 0x700 entry of the record's footstep sound. */
int objxFootId(int call)
{
    int key;
    int index;

    if (func_A9F0A8(call) != 2) {
        func_A9CC70(call, 2);
        return 0;
    }
    key = func_A9F0C0(call, 1);
    index = uwamonoFindIndexByShortKey((short) key);
    uwamonoRecords[index].motions->footId = func_A9F0C0(call, 2);
    return 1;
}

int func_A9CDB0(void *module, void *nativeTable);
/* Native entries of the objx module: (name, function) pairs of 8 bytes. */
extern unsigned char objxNatives[];
/* Module name "objx". */
extern unsigned char objxModuleName[];

/* Registers the objx script natives; called by the module registration in scriptRegisterModules. */
void objxRegisterNatives(void)
{
    func_A9CDB0(objxModuleName, objxNatives);
}

/* Native "setNumber": key, [kind], [entry], [fourth], [fifth]: creates the record of key (an omitted
 * kind releases the record instead). A string entry names a table entry, or an effect for MAP_KIND_EFFECT. */
int runtimeSetNumber(int call)
{
    int key;
    int kind;
    int entry;
    int fourth;

    if (func_A9F0A8(call) < 3) {
        func_A9CC70(call, 2);
        return 0;
    }
    key = func_A9F0C0(call, 1);
    if (func_A9F1C0(call, 2) != 0) {
        uwamonoReleaseRecord(uwamonoFindIndexByShortKey((short) key));
        return 1;
    }
    kind = func_A9F0C0(call, 2);
    if (func_A9F1C0(call, 3) != 0) {
        entry = -1;
    } else if (func_A9F140(call, 3) == 3) {
        if (kind != MAP_KIND_EFFECT) {
            entry = pxyFindEntryIndex(mapObjectTable, func_A9F238(call, func_A9F0C0(call, 3)));
            if (entry == -1) {
                return 0;
            }
        } else {
            entry = srsEffectKeyFromName(func_A9F238(call, func_A9F0C0(call, 3)));
        }
    } else {
        entry = func_A9F0C0(call, 3);
    }
    if (func_A9F1C0(call, 4) != 0) {
        fourth = -1;
    } else {
        fourth = func_A9F0C0(call, 4);
    }
    return uwamonoCreateRecord(key, kind, entry, fourth, func_A9F1C0(call, 5) != 0 ? -1 : func_A9F0C0(call, 5));
}

INCLUDE_ASM("asm/main/nonmatchings/tu067", uwamonoCreateRecord);

/* One 16-byte row of the pose table at uwamonoEntries (tu083.c). */
typedef struct UwamonoEntry {
    float x;
    float y;
    float z;
    short angle;
    unsigned short key;
} UwamonoEntry;

/* Holds the table's address (tu050.c stores it); declared int as in tu050.c and tu083.c. */
extern int uwamonoEntries;
int uwamonoCountLeadingInUse(void);
int uwamonoFindEntry(int key);

/* Native "setLocater": id, x1, y1, z1, x2, y2, z2 (percent), type, angle1, angle2 (degrees): creates
 * two mover records (MAP_KIND_MOVER) at the two points, linked to each other, with ids id and id + 1. When
 * arguments 3, 4, 6 and 7 are omitted the points are the pose table rows of keys x1 and x2. */
int runtimeSetLocater(int call)
{
    int index[2];
    int entry[2];
    UwamonoVector position[2];
    int i;

    if (func_A9F0A8(call) != 10) {
        func_A9CC70(call, 2);
        return 0;
    }
    if (func_A9F1C0(call, 3) != 0 && func_A9F1C0(call, 4) != 0 && func_A9F1C0(call, 6) != 0 && func_A9F1C0(call, 7) != 0) {
        entry[0] = uwamonoFindEntry(func_A9F0C0(call, 2));
        entry[1] = uwamonoFindEntry(func_A9F0C0(call, 5));
        for (i = 0; i < 2; i++) {
            position[i].coords[0] = ((UwamonoEntry *) uwamonoEntries)[entry[i]].x;
            position[i].coords[1] = ((UwamonoEntry *) uwamonoEntries)[entry[i]].y;
            position[i].coords[2] = ((UwamonoEntry *) uwamonoEntries)[entry[i]].z;
            position[i].coords[3] = 1.0f;
        }
    } else {
        for (i = 0; i < 2; i++) {
            position[i].coords[0] = (float) func_A9F0C0(call, i * 3 + 2) / 100.0f;
            position[i].coords[1] = (float) func_A9F0C0(call, i * 3 + 3) / 100.0f;
            position[i].coords[2] = (float) func_A9F0C0(call, i * 3 + 4) / 100.0f;
            position[i].coords[3] = 1.0f;
        }
    }
    for (i = 0; i < 2; i++) {
        index[i] = uwamonoCountLeadingInUse();
        uwamonoRecords[index[i]].kind = MAP_KIND_MOVER;
        uwamonoRecords[index[i]].position.coords[0] = position[i].coords[0];
        uwamonoRecords[index[i]].position.coords[1] = position[i].coords[1];
        uwamonoRecords[index[i]].position.coords[2] = position[i].coords[2];
        uwamonoRecords[index[i]].position.coords[3] = position[i].coords[3];
        uwamonoRecords[index[i]].turnFrom.coords[0] = (float) func_A9F0C0(call, 9) / 100.0f / 180.0f * 3.1415927f;
        uwamonoRecords[index[i]].turnTo.coords[0] = (float) func_A9F0C0(call, 10) / 100.0f / 180.0f * 3.1415927f;
        uwamonoRecords[index[i]].height = 1.0f;
    }
    uwamonoRecords[index[0]].lookLink = index[1];
    uwamonoRecords[index[1]].lookLink = index[0];
    uwamonoRecords[index[0]].id = func_A9F0C0(call, 1);
    uwamonoRecords[index[1]].id = func_A9F0C0(call, 1) + 1;
    uwamonoRecords[index[0]].locaterType = func_A9F0C0(call, 8);
    uwamonoRecords[index[1]].locaterType = 10;
    return 1;
}

void func_00246FF8(int value);

int runtimeSetLastDan(void)
{
    func_00246FF8((int) uwamonoSlotTable[uwamonoCurrentIndex].actor);
    return 1;
}

/* Native "setPlayer": scripted, frameScale, radius, five movement parameters, climbFrames, rate,
 * scale, [child]. An omitted value keeps the current one; scripted 1 sets CONTROL_SCRIPTED and wakes
 * the current record's actor, another value clears it and stops the actor; a child key attaches that
 * record's actor to joint 0x4D, a negative key releases it. */
int runtimeSetPlayer(int call)
{
    struct XenoActor *actor = uwamonoRecords[uwamonoCurrentIndex].actor;
    int childKey;

    if (func_A9F0A8(call) < 5) {
        func_A9CC70(call, 2);
        return 0;
    }
    if (func_A9F1C0(call, 1) == 0) {
        if (func_A9F0C0(call, 1) == 1) {
            gameWork.control |= CONTROL_SCRIPTED;
            XenoActorSetChannelValue(actor, 4.0f);
            XenoActorEnableChannel(actor);
            XenoActorSetMode(actor, 0);
        } else {
            gameWork.control &= ~CONTROL_SCRIPTED;
            uwamonoRecords[uwamonoCurrentIndex].actor->hitFlags |= 0x1000;
            if (actor->mode < 3 || actor->mode > 5) {
                XenoActorEnableChannel(actor);
                uwamonoSetMotion(uwamonoCurrentIndex, 0, 1.0f);
            }
        }
    }
    gameWork.frameScale = func_A9F1C0(call, 2) != 0 ? gameWork.frameScale : (float) func_A9F0C0(call, 2) / 100.0f / 100000.0f;
    uwamonoRecords[uwamonoCurrentIndex].extent = uwamonoRecords[uwamonoCurrentIndex].actor->radius = uwamonoRecords[uwamonoCurrentIndex].actor->probeRadius
        = func_A9F1C0(call, 3) != 0 ? uwamonoRecords[uwamonoCurrentIndex].extent : (float) func_A9F0C0(call, 3) / 100.0f;
    gameWork.playerParams[0][0] = func_A9F1C0(call, 4) != 0 ? gameWork.playerParams[0][0] : (float) func_A9F0C0(call, 4) / 100.0f;
    gameWork.playerParams[1][0] = func_A9F1C0(call, 5) != 0 ? gameWork.playerParams[1][0] : (float) func_A9F0C0(call, 5) / 100.0f;
    gameWork.playerParams[2][0] = func_A9F1C0(call, 6) != 0 ? gameWork.playerParams[2][0] : (float) func_A9F0C0(call, 6) / 100.0f;
    gameWork.playerParams[3][0] = func_A9F1C0(call, 7) != 0 ? gameWork.playerParams[3][0] : (float) func_A9F0C0(call, 7) / 100.0f;
    gameWork.playerParams[4][0] = func_A9F1C0(call, 8) != 0 ? gameWork.playerParams[4][0] : (float) func_A9F0C0(call, 8) / 100.0f;
    gameWork.climbFrames = func_A9F1C0(call, 9) != 0 ? gameWork.climbFrames : func_A9F0C0(call, 9);
    gameWork.playerRate = func_A9F1C0(call, 10) != 0 ? gameWork.playerRate : (float) func_A9F0C0(call, 10) / 10000.0f;
    gameWork.playerScale = func_A9F1C0(call, 11) != 0 ? gameWork.playerScale : (float) func_A9F0C0(call, 11) / 100.0f;
    if (func_A9F1C0(call, 12) == 0) {
        childKey = func_A9F0C0(call, 12);
        if (childKey >= 0) {
            XenoActorAttachChildToJoint(uwamonoRecords[uwamonoCurrentIndex].actor, uwamonoRecords[uwamonoFindIndexByShortKey((short) childKey)].actor, 0x4D);
        } else {
            XenoActorReleaseChild(uwamonoRecords[uwamonoCurrentIndex].actor, uwamonoRecords[uwamonoFindIndexByShortKey((short) -childKey)].actor);
        }
    }
    return 1;
}
