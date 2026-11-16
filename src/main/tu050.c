#include "common.h"
#include "main/nml_model_set.h"
#include "main/tu050.h"
#include "main/get.h"
#include "shared.h"
#include "main/sdv_033.h"
#include "main/tu272.h"

/* The resource record handed to mainResLoadCallback; gameWork.active points at it. */
typedef struct MainActive {
    struct MainObject *object;  /* +0x00 */
    int jpegData;               /* +0x04 */
    u8 unmodeled_08[0x04];
    int blockId;                /* +0x0C */
    u8 *imageHead;              /* +0x10 */
    int imageOffset;            /* +0x14 */
    u8 unmodeled_18[0x08];
    int *jpegStream;            /* +0x20 */
    u8 *pending;                /* +0x24: decoded texture */
    short offsetY;              /* +0x28 */
    short offsetX;              /* +0x2A */
    u16 height;                 /* +0x2C */
    u16 width;                  /* +0x2E */
    int viewY;                  /* +0x30 */
    int viewX;                  /* +0x34 */
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
    u8 *imageDst;               /* +0x2C */
    u8 unmodeled_30[0x88];
    int imageEnd;               /* +0xB8 */
    u8 unmodeled_BC[0x264];
    short ladderState;          /* +0x320: -1 = fresh load */
    u8 unmodeled_322[0x26];
    Vector4 extent;
    u8 unmodeled_358[0x08];
    u8 unmodeled_360[0x10];
    u8 unmodeled_370[0x04];
    float unitScaleB;           /* +0x374 */
    u8 unmodeled_378[0x4];
    float unitScaleC;           /* +0x37C */
    u8 unmodeled_380[0x4];
    float tableA[16][12];
    float tableB[16][12];
    float tableP[16][4];
    float tableQ[16][4];
    float tableS[16];
} MainWork;

/* Header of a map object block. */
typedef struct MainObject {
    u8 unmodeled_00[0x34];
    int entryCount;             /* +0x34 */
    u8 unmodeled_38[0x98];
    int secondHeaderOffset;     /* +0xD0: offset of the second header; nonzero = has one */
} MainObject;

/* Second header inside the block. */
typedef struct MainObjectSecond {
    u8 unmodeled_00[0x34];
    int entryCount;             /* +0x34 */
} MainObjectSecond;

/* Argument block of xglJpegDecode. */
typedef struct JpegInfo {
    u8 *source;                 /* +0x00 */
    u8 *output;                 /* +0x04 */
    int dataOffset;             /* +0x08 */
    u8 unmodeled_0C[0x08];
    short height;               /* +0x14: rows, stored to MainActive.height */
    short width;                /* +0x16 */
    u8 unmodeled_18[0x08];
} JpegInfo;

/* The original accesses these vectors with alias set 0, as named components or as a float array. */
typedef union MapVector {
    Vector4 vector;
    float component[4];
} MapVector;

typedef union AlignedMapVector {
    AlignedVector4 vector;
    float component[4];
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
#define MAP_OBJ_EFFECT_FOLLOW 0x2000 /* mirrored into SefEffectRecord.flags bit SEF_FOLLOW */
#define SEF_FOLLOW 0x10
/* First word of a JPEG stream (FF D8 FF E0) read as a little-endian int, and the size of its decode buffer. */
#define JPEG_MAGIC_WORD ((int)0xE0FFD8FF)
#define JPEG_DECODE_BUFFER_SIZE 0x400000

/* One 0x870-byte record of uwamonoRecords. */
typedef struct MapObjectRecord {
    short id;
    unsigned char kind;         /* +0x002: 0 free, 1/4 actor, 2 model, 3 flag, 6 elevator, 7 mover */
    unsigned char unmodeled_003;
    int key;                    /* +0x004: bit number in the map flag sets */
    int type;                  /* +0x008: index into mapModelResources, -1 for none */
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
    float fadeFrom;             /* +0x0E0 */
    float fadeTo;               /* +0x0E4 */
    u8 unmodeled_0e8[0x118];
    MapVector tilt;             /* +0x200 */
    short moveEndMode;          /* +0x210: mode for XenoActorCheckMode when the move ends, -1 for none */
    short moveStep;             /* +0x212 */
    short moveCount;            /* +0x214 */
    short turnStep;             /* +0x216 */
    short turnCount;            /* +0x218 */
    short scaleStep;            /* +0x21A */
    short scaleCount;           /* +0x21C */
    short fadeStep;             /* +0x21E */
    short fadeCount;            /* +0x220 */
    u8 unmodeled_222[0x26];
    int eventSlot;              /* +0x248: reset to -1 when the resource loads */
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
    float moveCurve[2][0x20];   /* +0x340 */
    short moveCurveType;        /* +0x440 */
    u8 unmodeled_442[2];
    float turnCurve[2][0x20];   /* +0x444 */
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
    MapVector focusOffset;      /* +0x5B0 */
    signed char talkMode;       /* +0x5C0 */
    signed char searchState;    /* +0x5C1 */
    signed char busy;           /* +0x5C2 */
    u8 unmodeled_5c3[0xD];
    MapVector closedPosition;        /* +0x5D0 */
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

/* One pair of mapObjectResources, the map resource table mapObjectResourcesLoad loads. */
typedef struct MapModelResource {
    MapModelData *data;
    int texture;
} MapModelResource;

extern MainWork gameWork;
extern MainObject *mainObject;
extern int D_0069B964;
extern void *mapObjectTable;
extern int uwamonoEntries;
extern unsigned short *uwamonoEntryCountPtr;
extern float D_0069AE00;
extern float D_0069AE04;
extern float D_0069B974[];
extern int D_002B7C88[];
extern int D_002B7CA8[];
extern MapObjectRecord uwamonoRecords[];
void *GameResBufferAlloc(unsigned int size);
void ladderInitTable(void);
void mapLoadBlock(int blockId);
void xglCullingMapSet(MainObject *object);
int xglJpegDecode(void *info);
void sefReleaseOwnedRecords(int owner);

/* A 0x12000-byte block copy of the map image. */
typedef struct MapImageBlock {
    unsigned char bytes[0x12000];
} MapImageBlock;

/* Map resource scale constants read by mainResLoadCallback. */
float D_0069AE00 = 0.02f;
float D_0069AE04 = 0.15f;

void mainResLoadCallback(int index, void *buffer, unsigned int size, int value)
{
    JpegInfo jpegInfo;
    MainActive *record = buffer;
    MainObject *object;
    MainObjectSecond *secondHeader;
    u8 *pair;
    u8 *pixels;
    int i;
    int entryIndex;
    int i2;
    int secondOffset;

    int jpegData;

    object = record->object;
    jpegData = record->jpegData;
    mainObject = object;
    D_0069B964 = jpegData;

    if (object != 0) {
        xglCullingMapSet(object);
        if (mainObject != 0) {
            MainObject *m = mainObject;
            for (entryIndex = 0; entryIndex < m->entryCount; entryIndex++) {
                MAP_BIT_CLEAR(D_002B7C88, entryIndex);
            }
            secondOffset = m->secondHeaderOffset;
            secondHeader = (MainObjectSecond *)((u8 *)m + secondOffset);
            mapObjectTable = secondHeader;
            if (secondOffset != 0) {
                i2 = 0;
                if (secondHeader->entryCount > 0) {
                    MainObjectSecond *headerBase = secondHeader;
                    secondHeader = 0;
                    do {
                        MAP_BIT_CLEAR(D_002B7CA8, i2);
                        i2++;
                    } while (i2 < headerBase->entryCount);
                }
            }
        }
    }

    if (D_0069B964 != 0 && *(int *)D_0069B964 == JPEG_MAGIC_WORD) {
        record->jpegStream = (int *)D_0069B964;
        record->pending = GameResBufferAlloc(JPEG_DECODE_BUFFER_SIZE);
        record->offsetX = 0;
        record->offsetY = 0;
        memset(&jpegInfo, 0, 0x20);
        jpegInfo.source = (u8 *)record->jpegStream;
        jpegInfo.output = record->pending;
        xglJpegDecode(&jpegInfo);
        pair = jpegInfo.output;
        record->height = jpegInfo.height;
        record->width = jpegInfo.width;
        pixels = jpegInfo.source + ((jpegInfo.dataOffset + 0x3F) & ~0x3F);
        for (i = 0; i < jpegInfo.height * jpegInfo.width; i++) {
            pair[3] = (u8)(*pixels >> 1);
            pixels++;
            pair += 4;
        }
        D_0069B964 = 0;
    }

    mapLoadBlock(record->blockId);
    uwamonoEntryCountPtr = 0;
    uwamonoEntryCountPtr = (unsigned short *)mapFindBlockEntry(0x300, -1);
    if (uwamonoEntryCountPtr != 0) {
        unsigned short entryCount = uwamonoEntryCountPtr[0];
        uwamonoEntries = (int)uwamonoEntryCountPtr + 16;
        i2 = 0;
        if (entryCount != 0) {
            do {
            } while (++i2 < (int)uwamonoEntryCountPtr[0]);
        }
    }

    gameWork.flags |= GAME_HIDE_RIDERS_DELAY | GAME_HIDE_RIDERS;
    if (gameWork.ladderState == -1) {
        float zeroF = 0.0f;
        gameWork.extent.w = zeroF;
        gameWork.extent.z = zeroF;
        gameWork.extent.y = zeroF;
        gameWork.extent.x = zeroF;
    }
    gameWork.unitScaleB = D_0069AE00;
    gameWork.unitScaleC = 1.5f;
    if (gameWork.ladderState == -1) {
        ladderInitTable();
    }
    sefReleaseOwnedRecords(-1);

    {
    u8 *imageHead = record->imageHead;
    if (imageHead != 0) {
        u8 *copyDst = gameWork.imageDst;
        u8 *copySrc = imageHead + (((unsigned int)imageHead & 0xFF) >> 8);
        *(MapImageBlock *)copyDst = *(MapImageBlock *)copySrc;
        gameWork.imageEnd = (int)gameWork.imageDst + record->imageOffset;
    }
    }

    {
        float zero = D_0069B974[0];
        float one = 1.0f;
        float half = 0.5f;
        float negOne = -1.0f;
        for (i2 = 0; i2 < 16; i2++) {
            gameWork.tableA[i2][0] = zero;
            gameWork.tableA[i2][1] = negOne;
            gameWork.tableA[i2][2] = zero;
            gameWork.tableA[i2][3] = one;
            gameWork.tableA[i2][4] = one;
            gameWork.tableA[i2][5] = one;
            gameWork.tableA[i2][6] = one;
            gameWork.tableA[i2][7] = one;
            gameWork.tableA[i2][8] = one;
            gameWork.tableA[i2][9] = one;
            gameWork.tableA[i2][10] = one;
            gameWork.tableA[i2][11] = one;
            gameWork.tableB[i2][0] = half;
            gameWork.tableB[i2][1] = half;
            gameWork.tableB[i2][2] = half;
            gameWork.tableB[i2][3] = one;
            gameWork.tableB[i2][4] = half;
            gameWork.tableB[i2][5] = half;
            gameWork.tableB[i2][6] = half;
            gameWork.tableB[i2][7] = one;
            gameWork.tableB[i2][8] = half;
            gameWork.tableB[i2][9] = half;
            gameWork.tableB[i2][10] = half;
            gameWork.tableB[i2][11] = one;
            gameWork.tableP[i2][0] = half;
            gameWork.tableP[i2][1] = half;
            gameWork.tableP[i2][2] = half;
            gameWork.tableP[i2][3] = one;
            gameWork.tableQ[i2][0] = 0;
            gameWork.tableQ[i2][1] = one;
            gameWork.tableQ[i2][2] = 0;
            gameWork.tableQ[i2][3] = 0;
            gameWork.tableS[i2] = D_0069AE04;
        }
    }

    for (i2 = 0; i2 < MAP_RECORD_COUNT; i2++) {
        uwamonoRecords[i2].eventSlot = -1;
    }
}

int GameResLoadID(int id, void **buffer, unsigned int *size, void (*callback)(int index, void *buffer, unsigned int size, int value));
int GameResBlockFreeByOwner(int owner);
void uwamonoResetRecords(int, int);
void sefResetHandleSlots(void);
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
    while (GameResLoadID(id, D_002B5DB8, 0, mainResLoadCallback) == -1) {
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

void XenoActorCheckMode(MapActor *actor, int expected_mode);

/* Advances the move, turn, scale and fade steps of one map object by a frame.
 * Returns 1 when any of the four steps ran. */
int mapObjectAdvanceTweens(MapObjectRecord *record)
{
    int advanced = 0;

    if (gameWork.flags & GAME_PAUSE_MAP_OBJECTS) {
        return 0;
    }
    if (record->moveStep != -1) {
        if (record->moveCurveType == 0) {
            Get_MiddlePoint(&record->moveFrom.vector, &record->moveTo.vector, record->moveStep,
                            record->moveCount, &record->position.vector);
        } else {
            Get_MiddlePoint_Curve(&record->moveFrom.vector, &record->moveTo.vector, record->moveStep,
                          record->moveCount, record->moveCurve[0], record->moveCurve[1],
                          record->moveCurveType, &record->position.vector);
        }
        record->moveStep = record->moveStep + 1;
        if (record->moveCount < record->moveStep) {
            short endMode = record->moveEndMode;

            record->moveStep = -1;
            record->moveCount = 0;
            if (endMode != -1) {
                MapActor *actor = record->actor;

                if (actor != 0) {
                    XenoActorCheckMode(actor, endMode);
                }
            }
        }
        advanced = 1;
    }
    if (record->turnStep != -1) {
        if (record->turnCurveType == 0) {
            Get_MiddlePoint(&record->turnFrom.vector, &record->turnTo.vector, record->turnStep,
                            record->turnCount, &record->rotation.vector);
        } else {
            Get_MiddlePoint_Curve(&record->turnFrom.vector, &record->turnTo.vector, record->turnStep,
                          record->turnCount, record->turnCurve[0], record->turnCurve[1],
                          record->turnCurveType, &record->rotation.vector);
        }
        record->turnStep = record->turnStep + 1;
        if (record->turnCount < record->turnStep) {
            MapActor *actor = record->actor;

            record->turnStep = -1;
            record->turnCount = 0;
            if (actor != 0) {
                actor->direction.y = record->rotation.vector.y;
            }
        }
        advanced = 1;
    }
    if (record->scaleStep != -1) {
        record->scale.vector.x = record->scaleFrom.vector.x
            + ((record->scaleTo.vector.x - record->scaleFrom.vector.x) * (float) record->scaleStep) / (float) record->scaleCount;
        record->scale.vector.y = record->scaleFrom.vector.y
            + ((record->scaleTo.vector.y - record->scaleFrom.vector.y) * (float) record->scaleStep) / (float) record->scaleCount;
        record->scale.vector.z = record->scaleFrom.vector.z
            + ((record->scaleTo.vector.z - record->scaleFrom.vector.z) * (float) record->scaleStep) / (float) record->scaleCount;
        record->scaleStep = record->scaleStep + 1;
        if (record->scaleCount < record->scaleStep) {
            record->scaleStep = -1;
            record->scaleCount = 0;
        }
        advanced = 1;
    }
    if (record->fadeStep != -1) {
        record->fade = record->fadeFrom
            + ((record->fadeTo - record->fadeFrom) * (float) record->fadeStep) / (float) record->fadeCount;
        record->fadeStep = record->fadeStep + 1;
        if (record->fadeCount < record->fadeStep) {
            record->fadeStep = -1;
            record->fadeCount = 0;
        }
        advanced = 1;
    }
    return advanced;
}

/* The effect record a map object owns through MapObjectRecord.effect; only the fields the transform code touches. */
typedef struct SefEffectRecord {
    float transform[3][4];      /* +0x00: position, rotation and scale rows, column 3 is 1.0f */
    u8 unmodeled_30[0x14];
    int flags;                  /* +0x44: bit 0x10 follows bit 0x2000 of MapObjectRecord.flags */
    u8 unmodeled_48[0x26];
    short shown;                /* +0x6E: bit 1 of MapObjectRecord.flags, shifted down */
} SefEffectRecord;

extern int gameLoopFlags[];

void mapObjectUpdateEffectTransform(MapObjectRecord *record)
{
    SefEffectRecord *effect;

    effect = record->effect;
    if (effect == 0) { return; }
    if (record->key != -1) {
        effect->shown = (record->flags >> 1) & 1;
        if (record->flags & MAP_OBJ_EFFECT_FOLLOW) { effect->flags |= SEF_FOLLOW; } else { effect->flags &= ~SEF_FOLLOW; }
    }
    if ((gameLoopFlags[0] & 1) == 0) {
        if (record->parent == -1 || record->attached != 1) {
            mapObjectAdvanceTweens(record);
        } else {
            record->position.vector.x = uwamonoRecords[record->parent].position.component[0] + record->savedPosition.vector.x;
            record->position.vector.y = uwamonoRecords[record->parent].position.component[1] + record->savedPosition.vector.y;
            record->position.vector.z = uwamonoRecords[record->parent].position.component[2] + record->savedPosition.vector.z;
            record->rotation.vector.x = uwamonoRecords[record->parent].rotation.component[0] + record->savedRotation.vector.x;
            record->rotation.vector.y = uwamonoRecords[record->parent].rotation.component[1] + record->savedRotation.vector.y;
            record->rotation.vector.z = uwamonoRecords[record->parent].rotation.component[2] + record->savedRotation.vector.z;
        }
    }
    effect->transform[0][0] = record->position.vector.x;
    effect->transform[0][1] = record->position.vector.y;
    effect->transform[0][2] = record->position.vector.z;
    effect->transform[0][3] = 1.0f;
    effect->transform[1][0] = record->rotation.vector.x;
    effect->transform[1][1] = record->rotation.vector.y;
    effect->transform[1][2] = record->rotation.vector.z;
    effect->transform[1][3] = 1.0f;
    effect->transform[2][0] = record->scale.vector.x;
    effect->transform[2][1] = record->scale.vector.y;
    effect->transform[2][2] = record->scale.vector.z;
    effect->transform[2][3] = 1.0f;
}

extern MapObjectRecord uwamonoRecords[];
extern int uwamonoCurrentIndex;
/* The GIF packet of the map image strips; only the words rewritten per strip. */
typedef struct StripPacket {
    u8 unmodeled_00[0x30];
    long long tag;              /* +0x30 */
    u8 unmodeled_38[0x28];
    int topY;                   /* +0x60 */
    u8 unmodeled_64[0x10];
    int topV;                   /* +0x74 */
    int topU;                   /* +0x78 */
    u8 unmodeled_7c[4];
    int bottomY;                /* +0x80 */
    u8 unmodeled_84[0x10];
    int bottomV;                /* +0x94 */
    int bottomU;                /* +0x98 */
} StripPacket;

extern StripPacket D_002B7A90[];
extern u8 D_002B7B30[];
extern u8 D_002B7BA0[];
extern float D_0069AE08;
extern XglPacket *xglPacketGetCurrent(void);
extern void sceVif1PkRef(XglPacket *packet, const void *environment, int count, int mode,
                         int offset, int flags);
extern void sceVif1PkCnt(XglPacket *packet, int count);
extern void sceVif1PkAddDataN(XglPacket *packet, const void *data, int count);
XglCamera *xglStudioGetCamera2(int);
void _CurMatrixSet(void *);
void _CurSetViewScaleTrans(void *, void *);
int _CurRotTransPersClipScreen(void *, void *);
void func_001BA924(void);

typedef struct ScanDelta {
    float x;
    float y;
    float z;
    float w;
} ScanDelta;

void func_001D7EB8(MainActive *active)
{
    u8 *cursor;
    ScanDelta delta;
    MapActor *target;
    float bestDistance;
    float nearestDz;
    float dx;
    float dz;
    float distanceSq;
    int i;
    int recordIndex;
    int rowFixedBottom;
    int newViewX;
    int col;
    int row;
    int maxCol;
    int maxRow;
    int rowFixedTop;
    u8 *imageAddress;
    int texAddress;
    int state;
    XglPacket *packet;
    XglCamera *camera;
    StripPacket *strip;

    target = uwamonoRecords[uwamonoCurrentIndex].actor;
    cursor = (u8 *)&uwamonoRecords[0].position.vector.z;
    nearestDz = 1.0f;
    bestDistance = 1000000.0f;
    for (recordIndex = 0; recordIndex < MAP_RECORD_COUNT; recordIndex++, cursor += 0x870) {
        if (*(cursor - 0x16) == MAP_KIND_MOVER) {
            state = *((signed char *)cursor + 0x579);
            if (state != 0 && state != 10) {
                dx = *(float *)(cursor - 8) - target->position.x;
                delta.x = dx;
                dz = *(float *)cursor - target->position.z;
                delta.z = dz;
                distanceSq = dx * dx + dz * dz;
                delta.w = distanceSq;
                if (distanceSq < bestDistance) {
                    bestDistance = distanceSq;
                    nearestDz = dz;
                }
            }
        }
    }

    camera = xglStudioGetCamera2(0);
    _CurMatrixSet(&camera->volume[0].viewMatrix);
    _CurSetViewScaleTrans(&camera->screen.screenScale, &camera->screen.screenOffset);
    _CurRotTransPersClipScreen(&active->viewY, D_002B7BA0);

    active->viewY = 0x800 - (active->viewY >> 4);
    newViewX = 0x800 - (active->viewX >> 4);
    active->viewX = newViewX;
    row = (short)active->height / 2 + active->offsetY + active->viewY - 0x100;
    if (row < 0) {
        row = 0;
    }
    maxRow = (short)active->height - 0x201;
    if (maxRow < row) {
        row = maxRow;
    }
    col = (short)active->width / 2 + active->offsetX + newViewX - 0xE0;
    if (col < 0) {
        col = 0;
    }
    maxCol = (short)active->width - 0x1C0;
    if (maxCol < col) {
        col = maxCol;
    }
    imageAddress = active->pending + (col << 12);
    rowFixedTop = row << 4;
    i = 0;
    func_001BA924();
    packet = xglPacketGetCurrent();

    strip = D_002B7A90;
    texAddress = 0x71F7;
    rowFixedBottom = rowFixedTop + 0x2000;
    for (; i < 4; i++) {
        sceVif1PkRef(packet, D_002B7B30, 7, 0, 0, 0);
        sceVif1PkRef(packet, imageAddress, 0x7000, 0, 0x51007000, 0);
        strip->tag = 0x53001;
        strip->topV = texAddress;
        texAddress += 0x700;
        strip->topU = 0;
        strip->topY = rowFixedTop;
        imageAddress += 0x70000;
        strip->bottomV = texAddress;
        strip->bottomU = 0;
        strip->bottomY = rowFixedBottom;
        sceVif1PkCnt(packet, 0);
        sceVif1PkAddDataN(packet, strip, 40);
        if (0.0f < nearestDz) {
            strip->tag = 0x5000D;
            strip->topU = 0x20000;
            strip->bottomU = 0x20000;
            sceVif1PkAddDataN(packet, strip, 40);
        }
    }
}

void pxySetTxyBlock(int);
void pxySubmitModel(void *);
extern int uwamonoCurrentIndex;
extern MapModelResource mapObjectResources[];
extern int D_002B7CC8[];
void xglMatrixUnit(float matrix[4][4]);
void xglMatrixStackLoad(float matrix[4][4]);
void xglMatrixStackSave(float matrix[4][4]);
void xglMatrixStackRotX(float angle);
void xglMatrixStackRotY(float angle);
void xglMatrixStackRotZ(float angle);
void xglMatrixStackTrans(Vector4 *translation);
void xglMatrixStackScale(Vector4 *scale);
/* Advances the move, turn, scale and fade steps of one map object by a frame. * Returns 1 when any of the four steps ran. */ int mapObjectAdvanceTweens(MapObjectRecord *record);
void mapObjectUpdateEffectTransform(MapObjectRecord *record);
void func_001D7EB8(MainActive *active);
void uwamonoChangePlayerModel(int value);
void fadeStartIn(int frames);
void uwamonoPlayFootEffects(int index);
void uwamonoStartAreaBlend(int index);
float unduGetSlotHeight(int index, Vector4 *position, int motion, int frame, int flags);
void uwamonoUpdateRecord(int index);
float cursolGetByReduceSpeedAngleLoop(float current, float target, float speed);
void mapObjectClearFlags(MapObjectRecord *record);
void uwamonoUpdateMidpoints(int index);
void DispPillar(Vector4 *position, float extent, float scale, int mode);
void uwamonoUpdateTransform(MapObjectRecord *record);
void elvUpdateMotion(MapObjectRecord *record);
void *xglPacketReserve(int size);
void func_001BAE58(MapModelHeader *header);
void func_001BA8F0(void);
float Get_Decimal_Surplus_for_Radius(float angle);
float Get_Distance(const Vector4 *a, const Vector4 *b);
float Get_Distance3D(const Vector4 *a, const Vector4 *b);
void *sefSpawnEffectRecordById(int id, Vector4 *position, int flags);
void sefReleaseRecordHandle(void *record);
void seVoiceStartDefault(int id);
void battleSlotsRefresh(void);
void uwamonoDrawPointAxes(float *point);
void XenoActorPushMatrix(MapActor *actor);
void uwamonoUpdateInReach(MapActor *actor, int index);
void uwamonoSetPairMode(int index, int value);
void uwamonoUpdatePoints(void);
void uwamonoUpdateCountdowns(void);
void DispCollUwamono(void);

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
                    uwamonoChangePlayerModel(-1000);
                    fadeStartIn(30);
                    break;
                }
            }
        }
        switch (record->kind) {
        case MAP_KIND_ACTOR:
        case MAP_KIND_ACTOR_STATIC:
            actor = record->actor;
            uwamonoPlayFootEffects(i);
            if (record->flags & MAP_OBJ_SHOWN) {
                actor->drawFlags &= ~4;
                actor->motion = record->motion;
                uwamonoStartAreaBlend(i);
            } else {
                actor->drawFlags |= 4;
            }
            if (record->kind != MAP_KIND_ACTOR_STATIC) {
                actor->previous.x = actor->position.x;
                actor->previous.y = actor->position.y;
                actor->previous.z = actor->position.z;
                actor->previous.w = actor->position.w;
            }
            if (mapObjectAdvanceTweens(record) == 1) {
                if (record->flags & 0x08000000) {
                    record->position.vector.y = unduGetSlotHeight(i, &record->actor->position, record->motions->id,
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
            DispPillar(&record->position.vector, record->extent, 0.1f, 7);
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
                    mapObjectUpdateEffectTransform(record);
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
                    mapObjectAdvanceTweens(record);
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
            if (record->type != -1) {
                header = xglPacketReserve(8);
                data = mapObjectResources[record->type + 2].data;
                source = (MapModelHeader *) ((u8 *) data + data->headerOffset);
                if (record->tiltStep != -1) {
                    Get_MiddlePoint(&record->turnFrom.vector, &record->turnTo.vector, record->tiltStep, record->tiltCount,
                                    &record->tilt.vector);
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
                    pxySetTxyBlock(mapObjectResources[record->type + 2].texture);
                    pxySubmitModel(mapObjectResources[record->type + 2].data);
                }
                if (record->type >= 7 && !(gameWork.flags & GAME_PAUSE_MAP_OBJECTS)) {
                    record->rotation.vector.y = Get_Decimal_Surplus_for_Radius(record->rotation.vector.y + 0.1f);
                }
                if (record->type >= 7 && record->type <= 9) {
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
            uwamonoDrawPointAxes(&point.x);
            uwamonoDrawPointAxes(record->transform[3]);
            break;
        case MAP_KIND_FLAG:
            if (record->flags & MAP_OBJ_SHOWN) {
                MAP_BIT_CLEAR(D_002B7C88, record->key);
            } else {
                MAP_BIT_SET(D_002B7C88, record->key);
            }
            break;
        case MAP_KIND_ELEVATOR:
            mapObjectUpdateEffectTransform(record);
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
                    Get_MiddlePoint(&record->moveFrom.vector, &record->moveTo.vector, record->moveStep, record->moveCount,
                                    &record->position.vector);
                } else {
                    Get_MiddlePoint_Curve(&record->moveFrom.vector, &record->moveTo.vector, record->moveStep, record->moveCount,
                                  record->moveCurve[0], record->moveCurve[1], record->moveCurveType,
                                  &record->position.vector);
                }
                record->moveStep++;
                if (record->moveStep > record->moveCount) {
                    record->moveStep = -1;
                }
            }
            if (record->turnStep != -1) {
                if (record->turnCurveType == 0) {
                    Get_MiddlePoint(&record->turnFrom.vector, &record->turnTo.vector, record->turnStep, record->turnCount,
                                    &record->rotation.vector);
                } else {
                    Get_MiddlePoint_Curve(&record->turnFrom.vector, &record->turnTo.vector, record->turnStep, record->turnCount,
                                  record->turnCurve[0], record->turnCurve[1], record->turnCurveType,
                                  &record->rotation.vector);
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
            uwamonoUpdateInReach(uwamonoRecords[uwamonoCurrentIndex].actor, j);
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
                        riders[0]->position.vector.x = record->position.vector.x + record->focusOffset.vector.x;
                        riders[0]->position.vector.y = record->position.vector.y + record->focusOffset.vector.y + record->height * 0.5f;
                        riders[0]->position.vector.z = record->position.vector.z + record->focusOffset.vector.z;
                        riders[0]->scale.vector.x = riders[0]->scale.vector.y = riders[0]->scale.vector.z
                            = uwamonoRecords[uwamonoCurrentIndex].size;
                    } else {
                        riders[0]->visible = 0;
                        riders[1]->visible = 1;
                        riders[1]->position.vector.x = record->position.vector.x + record->focusOffset.vector.x;
                        riders[1]->position.vector.y = record->position.vector.y + record->focusOffset.vector.y + record->height * 0.5f;
                        riders[1]->position.vector.z = record->position.vector.z + record->focusOffset.vector.z;
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
                    point = record->closedPosition.vector;
                } else {
                    Get_MiddlePoint(&record->closedPosition.vector, &uwamonoRecords[record->lookLink].closedPosition.vector, 1, 2, &point);
                }
                if (uwamonoRecords[uwamonoCurrentIndex].size * 5.0f
                    <= Get_Distance(&uwamonoRecords[uwamonoCurrentIndex].actor->position, &point)) {
                    uwamonoSetPairMode(j, 1);
                }
            }
        }
    }
    uwamonoUpdatePoints();
    uwamonoUpdateCountdowns();
    DispCollUwamono();
    return 1;
}

void pxySetFogColor(void *);
void pxySetFogDist(f32, f32, f32, f32);
void func_001BB3FC(void *);
extern unsigned char D_002B5E50[];

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
    if (mainObject->secondHeaderOffset != 0 && mapObjectTable != 0) {
        func_001BB678(mainObject);
        func_001BB44C(D_002B7CA8);
        func_001BB47C(D_002B7CC8);
        pxySetTxyBlock(D_0069B964);
        func_001BB3FC(D_002B5E50);
        pxySubmitModel(mapObjectTable);
    }
}

StripPacket D_002B7A90[1] = {
    {
        .unmodeled_00 = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 0, 0, 80, 1, 128, 0, 0, 0, 64, 139, 128, 238, 22, 83, 83, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 63, 0, 0, 0, 0, 0, 0, 0 },
        .tag = 329737LL,
        .unmodeled_38 = { 71, 0, 0, 0, 0, 0, 0, 0, 0, 56, 4, 40, 6, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 128, 0, 0, 0, 128, 0, 0, 0, 128, 0, 0, 0, 128, 0, 0, 0 },
        .topY = 0,
        .unmodeled_64 = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 248, 111, 0, 0 },
        .topV = 29175,
        .topU = 0,
        .unmodeled_7c = { 0, 0, 0, 0 },
        .bottomY = 8192,
        .unmodeled_84 = { 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 248, 143, 0, 0 },
        .bottomV = 36343,
        .bottomU = 0,
    },
};

u8 D_002B7B30[112] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 81, 4, 0, 0, 0, 0, 0, 0, 16, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 56, 16, 0, 80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 81, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 112, 0, 0, 0, 82, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 83, 0, 0, 0, 0, 0, 0, 0, 0, 112, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0 };

u8 D_002B7BA0[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 128, 63 };

float D_0069B974[1] = { -0.0f };

int D_0069B978 = -1;

/* The map resource table: 27 (data, texture) pairs, zero until mapObjectResourcesLoad fills them. Other files
 * read single words of it through their own labels (D_002B7BBC, D_002B7BC0). */
MapModelResource mapObjectResources[27] = { { 0 } };

/* The data that follows the table up to 0x2BB578 (.data, address order). The names and types are those of the
 * files that read them; nothing in this file does. */
int D_002B7C88[8] = { 0 };

int D_002B7CA8[8] = { 0 };

int D_002B7CC8[10] = { 0 };

unsigned char cameraCfSlots[6656] = { 0 };

unsigned char cameraSlotTable[640] = { 0 };

unsigned short uwamonoAttachIdTable[64] = {
    1, 1, 6, 1, 8, 1, 65535, 65535,
    9, 1, 13, 1, 14, 1, 65535, 65535,
    15, 1, 20, 1, 65535, 65535, 65535, 65535,
    21, 1, 26, 1, 50, 1, 51, 1,
    27, 1, 31, 1, 32, 1, 65535, 65535,
    34, 1, 36, 1, 65535, 65535, 65535, 65535,
    37, 1, 41, 1, 42, 1, 43, 1,
    16389, 1, 16389, 1, 16390, 1, 16391, 1
};

unsigned int uwamonoKeyRangeTable[170] = {
    0x10001u, 0x10006u, 15, 16, 17, 0, 0, 0,
    0x14005u, 29, 9, 14, 49, 10, 15, 50,
    306, 0x10009u, 0x1000Du, 3, 4, 5, 0, 0,
    0, 0x14005u, 29, 9, 14, 49, 10, 15,
    50, 304, 0x1000Fu, 0x10014u, 15, 16, 17, 0,
    0, 0, 0x14005u, 29, 9, 14, 49, 10,
    15, 50, 303, 0x10015u, 0x1001Au, 15, 16, 17,
    0, 0, 0, 0x14005u, 29, 9, 14, 49,
    10, 15, 50, 305, 0x1001Bu, 0x1001Fu, 15, 16,
    17, 0, 0, 0, 0x14005u, 29, 9, 14,
    49, 10, 15, 50, 307, 0x10022u, 0x10024u, 3,
    4, 5, 0, 0, 0, 0x14005u, 29, 9,
    14, 49, 10, 15, 50, 301, 0x10025u, 0x10029u,
    15, 16, 17, 0, 0, 0, 0x14005u, 29,
    9, 14, 49, 10, 15, 50, 302, 0x14002u,
    0x14002u, 25, 26, 26, 0, 0, 0, 0x10023u,
    21, 14, 19, 19, 15, 20, 20, 401,
    0x14003u, 0x14003u, 25, 26, 26, 0, 0, 0,
    0x10023u, 19, 14, 19, 19, 15, 20, 20,
    403, 0x14005u, 0x14005u, 25, 26, 26, 0, 0,
    0, 0x10023u, 34, 14, 19, 19, 15, 20,
    20, 402
};

float D_002B9C98[4] = {
    0.5f, 1.0f, 2.0f, 3.0f
};

unsigned int D_002B9CA8[4] = {
    0x3020400u, 0x10000u, 1286, 7
};

unsigned char uwamonoProbeTemplate[64] = { 0 };

/* data labels of other TUs referenced by the tables below */
extern const char D_006857A0[];
extern const char D_006857B0[];
extern const char D_006857C0[];
extern const char D_006857D0[];
extern const char D_006857E0[];
extern const char D_006857F0[];
extern const char D_00685800[];
extern const char D_00685810[];
extern const char D_00685820[];
extern const char D_00685830[];
extern const char D_00685840[];
extern const char D_00685850[];
extern const char D_00685860[];
extern const char D_00685870[];
extern const char D_00685880[];
extern const char D_00685890[];
extern const char D_006858A0[];
extern const char D_006858B0[];
extern const char D_006858C0[];
extern const char D_006858D0[];
extern const char D_006858E0[];
extern const char D_006858F0[];
extern const char D_00685900[];
extern const char D_00685910[];
extern const char D_00685920[];
extern const char D_00685930[];
extern const char D_00685940[];
extern const char D_00685950[];
extern const char D_00685960[];
extern const char D_00685970[];
extern const char D_00685980[];
extern const char D_00685990[];
extern const char D_006859A0[];
extern const char D_006859B0[];
extern const char D_006859C0[];
extern const char D_006859D0[];
extern const char D_006859E0[];
extern const char D_006859F0[];
extern const char D_00685A00[];
extern const char D_00685A10[];
extern const char D_00685A20[];
extern const char D_00685A30[];
extern const char D_00685A40[];
extern const char D_00685A50[];
extern const char D_00685A60[];
extern const char D_00685A70[];
extern const char D_00685A80[];
extern const char D_00685A90[];
extern const char D_00685AA0[];
extern const char D_00685AB0[];
extern const char D_00685AC0[];
extern const char D_00685AD0[];
extern const char D_00685AE0[];
extern const char D_00685AF0[];
extern const char D_00685B00[];
extern const char D_00685B10[];
extern const char D_00685B20[];
extern const char D_00685B30[];
extern const char D_00685B40[];
extern const char D_00685B50[];
extern const char D_00685B60[];
extern const char D_00685B70[];
extern const char D_00685B80[];
extern const char D_00685B90[];
extern const char D_00685BA0[];
extern const char D_00685BB0[];
extern const char D_00685BC0[];
extern const char D_00685BD0[];
extern const char D_00685BE0[];
extern const char D_00685BF0[];
extern const char D_00685C00[];
extern const char D_00685C10[];
extern const char D_00685C20[];
extern const char D_00685C30[];
extern const char D_00685C40[];
extern const char D_00685C50[];
extern const char D_00685C60[];
extern const char D_00685C70[];
extern const char D_00685C80[];
extern const char D_00685C90[];
extern const char D_00685CA0[];
extern const char D_00685CB0[];
extern const char D_00685CC0[];
extern const char D_00685CD0[];
extern const char D_00685CE0[];
extern const char D_00685CF0[];
extern const char D_00685D00[];
extern const char D_00685D10[];
extern const char D_00685D20[];
extern const char D_00685D30[];
extern const char D_00685D40[];
extern const char D_00685D50[];
extern const char D_00685D60[];
extern const char D_00685D70[];
extern const char D_00685D80[];
extern const char D_00685D90[];
extern const char D_00685DA0[];
extern const char D_00685DB0[];
extern const char D_00685DC0[];
extern const char D_00685DD0[];
extern const char D_00685DE0[];
extern const char D_00685DF8[];
extern const char D_00685E08[];
extern const char D_00685E18[];
extern const char D_00685E28[];
extern const char D_00685E38[];
extern const char D_00685E48[];
extern const char D_00685E58[];
extern const char D_00685E68[];
extern const char D_00685E78[];
extern const char D_00685E88[];
extern const char D_00685E98[];
extern const char D_00685EA8[];
extern const char D_00685EB8[];
extern const char D_00685EC8[];
extern const char D_00685EE0[];
extern const char D_00685EF0[];
extern const char D_00685F00[];
extern const char D_00685F10[];
extern const char D_00685F20[];
extern const char D_00685F30[];
extern const char D_00685F40[];
extern const char D_00685F50[];
extern const char D_00685F68[];
extern const char D_00685F78[];
extern const char D_00685F88[];
extern const char D_00685F98[];
extern const char D_00685FA8[];
extern const char D_00685FB8[];
extern const char D_00685FC8[];
extern const char D_00685FD8[];
extern const char D_00685FE8[];
extern const char D_00685FF8[];
extern const char D_00686010[];
extern const char D_00686020[];
extern const char D_00686030[];
extern const char D_00686040[];
extern const char D_00686050[];
extern const char D_00686060[];
extern const char D_00686070[];
extern const char D_00686080[];
extern const char D_00686090[];
extern const char D_006860A0[];
extern const char D_006860B0[];
extern const char D_006860C0[];
extern const char D_006860D0[];
extern const char D_006860E0[];
extern const char D_006860F0[];
extern const char D_00686100[];
extern const char D_00686110[];
extern const char D_00686120[];
extern const char D_00686130[];
extern const char D_00686140[];
extern const char D_00686150[];
extern const char D_00686160[];
extern const char D_00686170[];
extern const char D_00686180[];
extern const char D_00686190[];
extern const char D_006861A0[];
extern const char D_006861B0[];
extern const char D_006861C0[];
extern const char D_006861D0[];
extern const char D_006861E0[];
extern const char D_006861F0[];
extern const char D_00686200[];
extern const char D_00686210[];
extern const char D_00686220[];
extern const char D_00686230[];
extern const char D_00686240[];
extern const char D_00686250[];
extern const char D_00686260[];
extern const char D_00686270[];
extern const char D_00686280[];
extern const char D_00686290[];
extern const char D_006862A0[];
extern const char D_006862B0[];
extern const char D_006862C0[];
extern const char D_006862D0[];
extern const char D_006862E0[];
extern const char D_006862F0[];
extern const char D_00686300[];
extern const char D_00686310[];
extern const char D_00686320[];
extern const char D_00686330[];
extern const char D_00686340[];
extern const char D_00686350[];
extern const char D_00686360[];
extern const char D_00686370[];
extern const char D_00686380[];
extern const char D_00686390[];
extern const char D_006863A0[];
extern const char D_006863B0[];
extern const char D_006863C0[];
extern const char D_006863D0[];
extern const char D_006863E0[];
extern const char D_006863F0[];
extern const char D_00686400[];
extern const char D_00686410[];
extern const char D_00686420[];
extern const char D_00686430[];
extern const char D_00686440[];
extern const char D_00686450[];
extern const char D_00686460[];
extern const char D_00686470[];
extern const char D_00686480[];
extern const char D_00686490[];
extern const char D_006864A0[];
extern const char D_006864B0[];
extern const char D_006864C0[];
extern const char D_006864D0[];
extern const char D_006864E0[];
extern const char D_006864F0[];
extern const char D_00686500[];
extern const char D_00686510[];
extern const char D_00686520[];
extern const char D_00686538[];
extern const char D_00686548[];
extern const char D_00686558[];
extern const char D_00686568[];
extern const char D_00686578[];
extern const char D_00686588[];
extern const char D_00686598[];
extern const char D_006865A8[];
extern const char D_006865B8[];
extern const char D_006865C8[];
extern const char D_006865D8[];
extern const char D_006865E8[];
extern const char D_006865F8[];
extern const char D_00686608[];
extern const char D_00686618[];
extern const char D_00686628[];
extern const char D_00686638[];
extern const char D_00686648[];
extern const char D_00686658[];
extern const char D_00686668[];
extern const char D_00686678[];
extern const char D_00686688[];
extern const char D_00686698[];
extern const char D_006866A8[];
extern const char D_006866B8[];
extern const char D_006866C8[];
extern const char D_006866D8[];
extern const char D_006866E8[];
extern const char D_006866F8[];
extern const char D_00686708[];
extern const char D_00686718[];
extern const char D_00686728[];
extern const char D_00686738[];
extern const char D_00686748[];
extern const char D_00686758[];
extern const char D_00686768[];
extern const char D_00686778[];
extern const char D_00686788[];
extern const char D_00686798[];
extern const char D_006867A8[];
extern const char D_006867B8[];
extern const char D_006867C8[];
extern const char D_006867D8[];
extern const char D_006867E8[];
extern const char D_006867F8[];
extern const char D_00686808[];
extern const char D_00686818[];
extern const char D_00686828[];
extern const char D_00686838[];
extern const char D_00686848[];
extern const char D_00686858[];
extern const char D_00686868[];
extern const char D_00686878[];
extern const char D_00686888[];
extern const char D_00686898[];
extern const char D_006868A8[];
extern const char D_006868B8[];
extern const char D_006868C8[];
extern const char D_006868D8[];
extern const char D_006868E8[];
extern const char D_006868F8[];
extern const char D_00686908[];
extern const char D_00686918[];
extern const char D_00686928[];
extern const char D_00686938[];
extern const char D_00686948[];
extern const char D_00686958[];
extern const char D_00686968[];
extern const char D_00686978[];
extern const char D_00686988[];
extern const char D_00686998[];
extern const char D_006869A8[];
extern const char D_006869B8[];
extern const char D_006869C8[];
extern const char D_006869D8[];
extern const char D_006869E8[];
extern const char D_006869F8[];
extern const char D_00686A08[];
extern const char D_00686A18[];
extern const char D_00686A28[];
extern const char D_00686A38[];
extern const char D_00686A48[];
extern const char D_00686A58[];
extern const char D_00686A68[];
extern const char D_00686A78[];
extern const char D_00686A88[];
extern const char D_00686A98[];
extern const char D_00686AA8[];
extern const char D_00686AB8[];
extern const char D_00686AC8[];
extern const char D_00686AD8[];
extern const char D_00686AE8[];
extern const char D_00686AF8[];
extern const char D_00686B08[];
extern const char D_00686B18[];
extern const char D_00686B28[];
extern const char D_00686B38[];
extern const char D_00686B48[];
extern const char D_00686B58[];
extern const char D_00686B68[];
extern const char D_00686B78[];
extern const char D_00686B88[];
extern const char D_00686B98[];
extern const char D_00686BA8[];
extern const char D_00686BB8[];
extern const char D_00686BC8[];
extern const char D_00686BD8[];
extern const char D_00686BE8[];
extern const char D_00686BF8[];
extern const char D_00686C08[];
extern const char D_00686C18[];
extern const char D_00686C28[];
extern const char D_00686C38[];
extern const char D_00686C48[];
extern const char D_00686C58[];
extern const char D_00686C68[];
extern const char D_00686C78[];
extern const char D_00686C88[];
extern const char D_00686C98[];
extern const char D_00686CA8[];
extern const char D_00686CB8[];
extern const char D_00686CC8[];
extern const char D_00686CD8[];
extern const char D_00686CE8[];
extern const char D_00686CF8[];
extern const char D_00686D08[];
extern const char D_00686D18[];
extern const char D_00686D28[];
extern const char D_00686D38[];
extern const char D_00686D48[];
extern const char D_00686D58[];
extern const char D_00686D68[];
extern const char D_00686D78[];
extern const char D_00686D88[];
extern const char D_00686D98[];
extern const char D_00686DA8[];
extern const char D_00686DB8[];
extern const char D_00686DC8[];
extern const char D_00686DD8[];
extern const char D_00686DE8[];
extern const char D_00686DF8[];
extern const char D_00686E08[];
extern const char D_00686E18[];
extern const char D_00686E28[];
extern const char D_00686E38[];
extern const char D_00686E48[];
extern const char D_00686E58[];
extern const char D_00686E68[];
extern const char D_00686E78[];
extern const char D_00686E88[];
extern const char D_00686E98[];
extern const char D_00686EA8[];
extern const char D_00686EB8[];
extern const char D_00686EC8[];
extern const char D_00686ED8[];
extern const char D_00686EE8[];
extern const char D_00686EF8[];
extern const char D_00686F08[];
extern const char D_00686F18[];
extern const char D_00686F28[];
extern const char D_00686F38[];
extern const char D_00686F48[];
extern const char D_00686F58[];
extern const char D_00686F68[];
extern const char D_00686F78[];
extern const char D_00686F88[];
extern const char D_00686F98[];
extern const char D_00686FA8[];
extern const char D_00686FB8[];
extern const char D_00686FC8[];
extern const char D_00686FD8[];
extern const char D_00686FE8[];
extern const char D_00686FF8[];
extern const char D_00687008[];
extern const char D_00687018[];
extern const char D_00687028[];
extern const char D_00687038[];
extern const char D_00687048[];
extern const char D_00687058[];
extern const char D_00687068[];
extern const char D_00687078[];
extern const char D_00687088[];
extern const char D_00687098[];
extern const char D_006870A8[];
extern const char D_006870B8[];
extern const char D_006870C8[];
extern const char D_006870D8[];
extern const char D_006870E8[];
extern const char D_006870F8[];
extern const char D_00687108[];
extern const char D_00687118[];
extern const char D_00687128[];
extern const char D_00687138[];
extern const char D_00687148[];
extern const char D_00687158[];
extern const char D_00687168[];
extern const char D_00687178[];
extern const char D_00687188[];
extern const char D_00687198[];
extern const char D_006871A8[];
extern const char D_006871B8[];
extern const char D_006871C8[];
extern const char D_006871D8[];
extern const char D_006871E8[];
extern const char D_006871F8[];
extern const char D_00687208[];
extern const char D_00687218[];
extern const char D_00687228[];
extern const char D_00687238[];
extern const char D_00687248[];
extern const char D_00687258[];
extern const char D_00687268[];
extern const char D_00687278[];
extern const char D_00687288[];
extern const char D_00687298[];
extern const char D_006872A8[];
extern const char D_006872B8[];
extern const char D_006872C8[];
extern const char D_006872D8[];
extern const char D_006872E8[];
extern const char D_006872F8[];
extern const char D_00687308[];
extern const char D_00687318[];
extern const char D_00687328[];
extern const char D_00687338[];
extern const char D_00687348[];
extern const char D_00687358[];
extern const char D_00687368[];
extern const char D_00687378[];
extern const char D_00687388[];
extern const char D_00687398[];
extern const char D_006873A8[];
extern const char D_006873B8[];
extern const char D_006873C8[];
extern const char D_006873D8[];
extern const char D_006873E8[];
extern const char D_006873F8[];
extern const char D_00687408[];
extern const char D_00687418[];
extern const char D_00687428[];
extern const char D_00687438[];
extern const char D_00687448[];
extern const char D_00687458[];
extern const char D_00687468[];
extern const char D_00687478[];
extern const char D_00687488[];
extern const char D_00687498[];
extern const char D_006874A8[];
extern const char D_006874B8[];
extern const char D_006874C8[];
extern const char D_006874D8[];
extern const char D_006874E8[];
extern const char D_006874F8[];
extern const char D_00687508[];
extern const char D_00687518[];
extern const char D_00687528[];
extern const char D_00687538[];
extern const char D_00687548[];
extern const char D_00687558[];
extern const char D_00687568[];
extern const char D_00687578[];
extern const char D_00687588[];
extern const char D_00687598[];
extern const char D_006875A8[];
extern const char D_006875B8[];
extern const char D_006875C8[];
extern const char D_006875D8[];
extern const char D_006875E8[];
extern const char D_006875F8[];
extern const char D_00687608[];
extern const char D_00687618[];
extern const char D_00687628[];
extern const char D_00687638[];
extern const char D_00687648[];
extern const char D_00687658[];
extern const char D_00687668[];
extern const char D_00687678[];
extern const char D_00687688[];
extern const char D_00687698[];
extern const char D_006876A8[];
extern const char D_006876B8[];
extern const char D_006876C8[];
extern const char D_006876D8[];
extern const char D_006876E8[];
extern const char D_006876F8[];
extern const char D_00687708[];
extern const char D_00687718[];
extern const char D_00687728[];
extern const char D_00687738[];
extern const char D_00687748[];
extern const char D_00687758[];
extern const char D_00687768[];
extern const char D_00687778[];
extern const char D_00687788[];
extern const char D_00687798[];
extern const char D_006877A8[];
extern const char D_006877B8[];
extern const char D_006877C8[];
extern const char D_006877D8[];
extern const char D_006877E8[];
extern const char D_006877F8[];
extern const char D_00687808[];
extern const char D_00687818[];
extern const char D_00687828[];
extern const char D_00687838[];
extern const char D_00687848[];
extern const char D_00687858[];
extern const char D_00687868[];
extern const char D_00687878[];
extern const char D_00687888[];
extern const char D_00687898[];
extern const char D_006878A8[];
extern const char D_006878B8[];
extern const char D_006878C8[];
extern const char D_006878D8[];
extern const char D_006878E8[];
extern const char D_006878F8[];
extern const char D_00687908[];
extern const char D_00687918[];
extern const char D_00687928[];
extern const char D_00687938[];
extern const char D_00687948[];
extern const char D_00687958[];
extern const char D_00687968[];
extern const char D_00687978[];
extern const char D_00687988[];
extern const char D_00687998[];
extern const char D_006879A8[];
extern const char D_006879B8[];
extern const char D_006879C8[];
extern const char D_006879D8[];
extern const char D_006879E8[];
extern const char D_006879F8[];
extern const char D_00687A08[];
extern const char D_00687A18[];
extern const char D_00687A28[];
extern const char D_00687A38[];
extern const char D_00687A48[];
extern const char D_00687A58[];
extern const char D_00687A68[];
extern const char D_00687A78[];
extern const char D_00687A88[];
extern const char D_00687A98[];
extern const char D_00687AA8[];
extern const char D_00687AB8[];
extern const char D_00687AC8[];
extern const char D_00687AD8[];
extern const char D_00687AE8[];
extern const char D_00687AF8[];
extern const char D_00687B08[];
extern const char D_00687B18[];
extern const char D_00687B28[];
extern const char D_00687B38[];
extern const char D_00687B48[];
extern const char D_00687B58[];
extern const char D_00687B68[];
extern const char D_00687B78[];
extern const char D_00687B88[];
extern const char D_00687B98[];
extern const char D_00687BA8[];
extern const char D_00687BB8[];
extern const char D_00687BC8[];
extern const char D_00687BD8[];
extern const char D_00687BE8[];
extern const char D_00687BF8[];
extern const char D_00687C08[];
extern const char D_00687C18[];
extern const char D_00687C28[];
extern const char D_00687C38[];
extern const char D_00687C48[];
extern const char D_00687C58[];
extern const char D_00687C68[];
extern const char D_00687C78[];
extern const char D_00687C88[];
extern const char D_00687C98[];
extern const char D_00687CA8[];
extern const char D_00687CB8[];
extern const char D_00687CC8[];
extern const char D_00687CD8[];
extern const char D_00687CE8[];
extern const char D_00687CF8[];
extern const char D_00687D08[];
extern const char D_00687D18[];
extern const char D_00687D28[];
extern const char D_00687D38[];
extern const char D_00687D48[];
extern const char D_00687D58[];
extern const char D_00687D68[];
extern const char D_00687D78[];
extern const char D_00687D88[];
extern const char D_00687D98[];
extern const char D_00687DA8[];
extern const char D_00687DB8[];
extern const char D_00687DC8[];
extern const char D_00687DD8[];
extern const char D_00687DE8[];
extern const char D_00687DF8[];
extern const char D_00687E08[];
extern const char D_00687E18[];
extern const char D_00687E28[];
extern const char D_00687E38[];
extern const char D_00687E48[];
extern const char D_00687E58[];
extern const char D_00687E68[];
extern const char D_00687E78[];
extern const char D_00687E88[];
extern const char D_00687E98[];
extern const char D_00687EA8[];
extern const char D_00687EB8[];
extern const char D_00687EC8[];
extern const char D_00687ED8[];
extern const char D_00687EE8[];
extern const char D_00687EF8[];
extern const char D_00687F08[];
extern const char D_00687F18[];
extern const char D_00687F28[];
extern const char D_00687F38[];
extern const char D_00687F48[];
extern const char D_00687F58[];
extern const char D_00687F68[];
extern const char D_00687F78[];
extern const char D_00687F88[];
extern const char D_00687F98[];
extern const char D_00687FA8[];
extern const char D_00687FB8[];
extern const char D_00687FC8[];
extern const char D_00687FD8[];
extern const char D_00687FE8[];
extern const char D_00687FF8[];
extern const char D_00688008[];
extern const char D_00688018[];
extern const char D_00688028[];
extern const char D_00688038[];
extern const char D_00688048[];
extern const char D_00688058[];
extern const char D_00688068[];
extern const char D_00688078[];
extern const char D_00688088[];
extern const char D_00688098[];
extern const char D_006880A8[];
extern const char D_006880B8[];
extern const char D_006880C8[];
extern const char D_006880D8[];
extern const char D_006880E8[];
extern const char D_006880F8[];
extern const char D_00688108[];
extern const char D_00688118[];
extern const char D_00688128[];
extern const char D_00688138[];
extern const char D_00688148[];
extern const char D_00688158[];
extern const char D_00688168[];
extern const char D_00688178[];
extern const char D_00688188[];
extern const char D_00688198[];
extern const char D_006881A8[];
extern const char D_006881B8[];
extern const char D_006881C8[];
extern const char D_006881D8[];
extern const char D_006881E8[];
extern const char D_006881F8[];
extern const char D_00688208[];
extern const char D_00688218[];
extern const char D_00688228[];
extern const char D_0069B9B8[];
extern const char D_0069B9C0[];
extern const char D_0069B9C8[];
extern const char D_0069B9D0[];
extern const char D_0069B9D8[];
extern const char D_0069B9E0[];
extern const char D_0069B9E8[];
extern const char D_0069B9F0[];
extern const char D_0069B9F8[];
extern const char D_0069BA00[];
extern const char D_0069BA08[];
extern const char D_0069BA10[];
extern const char D_0069BA18[];
extern const char D_0069BA20[];
extern const char D_0069BA28[];
extern const char D_0069BA30[];
extern const char D_0069BA38[];
extern const char D_0069BA40[];
extern const char D_0069BA48[];
extern const char D_0069BA50[];
extern const char D_0069BA58[];
extern const char D_0069BA60[];
extern const char D_0069BA68[];
extern const char D_0069BA70[];
extern const char D_0069BA78[];
extern const char D_0069BA80[];
extern const char D_0069BA88[];
extern const char D_0069BA90[];
extern const char D_0069BA98[];
extern const char D_0069BAA0[];
extern const char D_0069BAA8[];
extern const char D_0069BAB0[];
extern const char D_0069BAB8[];
extern const char D_0069BAC0[];
extern const char D_0069BAC8[];
extern const char D_0069BAD0[];
extern const char D_0069BAD8[];
extern const char D_0069BAE0[];
extern const char D_0069BAE8[];
extern const char D_0069BAF0[];
extern const char D_0069BAF8[];
extern const char D_0069BB00[];
extern const char D_0069BB08[];
extern const char D_0069BB10[];
extern const char D_0069BB18[];
extern const char D_0069BB20[];
extern const char D_0069BB28[];
extern const char D_0069BB30[];
extern const char D_0069BB38[];
extern const char D_0069BB40[];
extern const char D_0069BB48[];
extern const char D_0069BB50[];
extern const char D_0069BB58[];
extern const char D_0069BB60[];
extern const char D_0069BB68[];
extern const char D_0069BB70[];
extern const char D_0069BB78[];
extern const char D_0069BB80[];
extern const char D_0069BB88[];
extern const char D_0069BB90[];
extern const char D_0069BB98[];
extern const char D_0069BBA0[];
extern const char D_0069BBA8[];
extern const char D_0069BBB0[];
extern const char D_0069BBB8[];
extern const char D_0069BBC0[];
extern const char D_0069BBC8[];
extern const char D_0069BBD0[];
extern const char D_0069BBD8[];
extern const char D_0069BBE0[];
extern const char D_0069BBE8[];
extern const char D_0069BBF0[];
extern const char D_0069BBF8[];
extern const char D_0069BC00[];
extern const char D_0069BC08[];
extern const char D_0069BC10[];
extern const char D_0069BC18[];
extern const char D_0069BC20[];
extern const char D_0069BC28[];
extern const char D_0069BC30[];
extern const char D_0069BC38[];
extern const char D_0069BC40[];
extern const char D_0069BC48[];
extern const char D_0069BC50[];
extern const char D_0069BC58[];
extern const char D_0069BC60[];
extern const char D_0069BC68[];
extern const char D_0069BC70[];
extern const char D_0069BC78[];
extern const char D_0069BC80[];
extern const char D_0069BC88[];
extern const char D_0069BC90[];
extern const char D_0069BC98[];
extern const char D_0069BCA0[];
extern const char D_0069BCA8[];
extern const char D_0069BCB0[];
extern const char D_0069BCB8[];
extern const char D_0069BCC0[];
extern const char D_0069BCC8[];
extern const char D_0069BCD0[];
extern const char D_0069BCD8[];
extern const char D_0069BCE0[];
extern const char D_0069BCE8[];
extern const char D_0069BCF0[];
extern const char D_0069BCF8[];
extern const char D_0069BD00[];
extern const char D_0069BD08[];
extern const char D_0069BD10[];
extern const char D_0069BD18[];
extern const char D_0069BD20[];
extern const char D_0069BD28[];
extern const char D_0069BD30[];
extern const char D_0069BD38[];
extern const char D_0069BD40[];
extern const char D_0069BD48[];
extern const char D_0069BD50[];
extern const char D_0069BD58[];
extern const char D_0069BD60[];
extern const char D_0069BD68[];
extern const char D_0069BD70[];
extern const char D_0069BD78[];
extern const char D_0069BD80[];
extern const char D_0069BD88[];
extern const char D_0069BD90[];
extern const char D_0069BD98[];
extern const char D_0069BDA0[];
extern const char D_0069BDA8[];
extern const char D_0069BDB0[];
extern const char D_0069BDB8[];
extern const char D_0069BDC0[];
extern const char D_0069BDC8[];
extern const char D_0069BDD0[];
extern const char D_0069BDD8[];
extern const char D_0069BDE0[];
extern const char D_0069BDE8[];
extern const char D_0069BDF0[];
extern const char D_0069BDF8[];
extern const char D_0069BE00[];
extern const char D_0069BE08[];
extern const char D_0069BE10[];
extern const char D_0069BE18[];
extern const char D_0069BE20[];
extern const char D_0069BE28[];
extern const char D_0069BE30[];
extern const char D_0069BE38[];
extern const char D_0069BE40[];
extern const char D_0069BE48[];
extern const char D_0069BE50[];
extern const char D_0069BE58[];
extern const char D_0069BE60[];
extern const char D_0069BE68[];
extern const char D_0069BE70[];
extern const char D_0069BE78[];
extern const char D_0069BE80[];
extern const char D_0069BE88[];
extern const char D_0069BE90[];
extern const char D_0069BE98[];
extern const char D_0069BEA0[];
extern const char D_0069BEA8[];
extern const char D_0069BEB0[];
extern const char D_0069BEB8[];
extern const char D_0069BEC0[];
extern const char D_0069BEC8[];
extern const char D_0069BED0[];
extern const char D_0069BED8[];
extern const char D_0069BEE0[];
extern const char D_0069BEE8[];
extern const char D_0069BEF0[];
extern const char D_0069BEF8[];
extern const char D_0069BF00[];
extern const char D_0069BF08[];
extern const char D_0069BF10[];
extern const char D_0069BF18[];
extern const char D_0069BF20[];
extern const char D_0069BF28[];
extern const char D_0069BF30[];
extern const char D_0069BF38[];
extern const char D_0069BF40[];
extern const char D_0069BF48[];
extern const char D_0069BF50[];
extern const char D_0069BF58[];
extern const char D_0069BF60[];
extern const char D_0069BF68[];
extern const char D_0069BF70[];
extern const char D_0069BF78[];
extern const char D_0069BF80[];
extern const char D_0069BF88[];
extern const char D_0069BF90[];
extern const char D_0069BF98[];
extern const char D_0069BFA0[];
extern const char D_0069BFA8[];
extern const char D_0069BFB0[];
extern const char D_0069BFB8[];
extern const char D_0069BFC0[];
extern const char D_0069BFC8[];
extern const char D_0069BFD0[];
extern const char D_0069BFD8[];
extern const char D_0069BFE0[];
extern const char D_0069BFE8[];
extern const char D_0069BFF0[];
extern const char D_0069BFF8[];
extern const char D_0069C000[];
extern const char D_0069C008[];
extern const char D_0069C010[];
extern const char D_0069C018[];
extern const char D_0069C020[];
extern const char D_0069C028[];
extern const char D_0069C030[];
extern const char D_0069C038[];
extern const char D_0069C040[];
extern const char D_0069C048[];
extern const char D_0069C050[];
extern const char D_0069C058[];
extern const char D_0069C060[];
extern const char D_0069C068[];
extern const char D_0069C070[];
extern const char D_0069C078[];
extern const char D_0069C080[];
extern const char D_0069C088[];
extern const char D_0069C090[];
extern const char D_0069C098[];
extern const char D_0069C0A0[];
extern const char D_0069C0A8[];
extern const char D_0069C0B0[];
extern const char D_0069C0B8[];
extern const char D_0069C0C0[];
extern const char D_0069C0C8[];
extern const char D_0069C0D0[];
extern const char D_0069C0D8[];
extern const char D_0069C0E0[];
extern const char D_0069C0E8[];
extern const char D_0069C0F0[];
extern const char D_0069C0F8[];
extern const char D_0069C100[];
extern const char D_0069C108[];
extern const char D_0069C110[];
extern const char D_0069C118[];
extern const char D_0069C120[];
extern const char D_0069C128[];
extern const char D_0069C130[];
extern const char D_0069C138[];
extern const char D_0069C140[];
extern const char D_0069C148[];
extern const char D_0069C150[];
extern const char D_0069C158[];
extern const char D_0069C160[];
extern const char D_0069C168[];
extern const char D_0069C170[];
extern const char D_0069C178[];
extern const char D_0069C180[];
extern const char D_0069C188[];
extern const char D_0069C190[];
extern const char D_0069C198[];
extern const char D_0069C1A0[];
extern const char D_0069C1A8[];
extern const char D_0069C1B0[];
extern const char D_0069C1B8[];
extern const char D_0069C1C0[];
extern const char D_0069C1C8[];
extern const char D_0069C1D0[];
extern const char D_0069C1D8[];
extern const char D_0069C1E0[];
extern const char D_0069C1E8[];
extern const char D_0069C1F0[];
extern const char D_0069C1F8[];
extern const char D_0069C200[];
extern const char D_0069C208[];
extern const char D_0069C210[];
extern const char D_0069C218[];
extern const char D_0069C220[];
extern const char D_0069C228[];
extern const char D_0069C230[];
extern const char D_0069C238[];
extern const char D_0069C240[];
extern const char D_0069C248[];
extern const char D_0069C250[];
extern const char D_0069C258[];
extern const char D_0069C260[];
extern const char D_0069C268[];
extern const char D_0069C270[];
extern const char D_0069C278[];
extern const char D_0069C280[];
extern const char D_0069C288[];
extern const char D_0069C290[];
extern const char D_0069C298[];
extern const char D_0069C2A0[];
extern const char D_0069C2A8[];
extern const char D_0069C2B0[];
extern const char D_0069C2B8[];
extern const char D_0069C2C0[];
extern const char D_0069C2C8[];
extern const char D_0069C2D0[];
extern const char D_0069C2D8[];
extern const char D_0069C2E0[];
extern const char D_0069C2E8[];
extern const char D_0069C2F0[];
extern const char D_0069C2F8[];
extern const char D_0069C300[];
extern const char D_0069C308[];
extern const char D_0069C310[];
extern const char D_0069C318[];
extern const char D_0069C320[];
extern const char D_0069C328[];
extern const char D_0069C330[];
extern const char D_0069C338[];
extern const char D_0069C340[];
extern const char D_0069C348[];
extern const char D_0069C350[];
extern const char D_0069C358[];
extern const char D_0069C360[];
extern const char D_0069C368[];
extern const char D_0069C370[];
extern const char D_0069C378[];
extern const char D_0069C380[];
extern const char D_0069C388[];
extern const char D_0069C390[];
extern const char D_0069C398[];
extern const char D_0069C3A0[];
extern const char D_0069C3A8[];
extern const char D_0069C3B0[];
extern const char D_0069C3B8[];
extern const char D_0069C3C0[];
extern const char D_0069C3C8[];
extern const char D_0069C3D0[];
extern const char D_0069C3D8[];
extern const char D_0069C3E0[];
extern const char D_0069C3E8[];
extern const char D_0069C3F0[];
extern const char D_0069C3F8[];
extern const char D_0069C400[];
extern const char D_0069C408[];
extern const char D_0069C410[];
extern const char D_0069C418[];
extern const char D_0069C420[];
extern const char D_0069C428[];
extern const char D_0069C430[];
extern const char D_0069C438[];
extern const char D_0069C440[];
extern const char D_0069C448[];
extern const char D_0069C450[];
extern const char D_0069C458[];
extern const char D_0069C460[];
extern const char D_0069C468[];
extern const char D_0069C470[];
extern const char D_0069C478[];
extern const char D_0069C480[];
extern const char D_0069C488[];
extern const char D_0069C490[];
extern const char D_0069C498[];
extern const char D_0069C4A0[];
extern const char D_0069C4A8[];
extern const char D_0069C4B0[];
extern const char D_0069C4B8[];
extern const char D_0069C4C0[];
extern const char D_0069C4C8[];
extern const char D_0069C4D0[];
extern const char D_0069C4D8[];
extern const char D_0069C4E0[];
extern const char D_0069C4E8[];
extern const char D_0069C4F0[];
extern const char D_0069C4F8[];
extern const char D_0069C500[];
extern const char D_0069C508[];
extern const char D_0069C510[];
extern const char D_0069C518[];
extern const char D_0069C520[];
extern const char D_0069C528[];
extern const char D_0069C530[];
extern const char D_0069C538[];
extern const char D_0069C540[];
extern const char D_0069C548[];
extern const char D_0069C550[];
extern const char D_0069C558[];
extern const char D_0069C560[];
extern const char D_0069C568[];
extern const char D_0069C570[];
extern const char D_0069C578[];
extern const char D_0069C580[];
extern const char D_0069C588[];
extern const char D_0069C590[];
extern const char D_0069C598[];
extern const char D_0069C5A0[];
extern const char D_0069C5A8[];
extern const char D_0069C5B0[];
extern const char D_0069C5B8[];
extern const char D_0069C5C0[];
extern const char D_0069C5C8[];
extern const char D_0069C5D0[];
extern const char D_0069C5D8[];
extern const char D_0069C5E0[];
extern const char D_0069C5E8[];
extern const char D_0069C5F0[];
extern const char D_0069C5F8[];
extern const char D_0069C600[];
extern const char D_0069C608[];
extern const char D_0069C610[];
extern const char D_0069C618[];
extern const char D_0069C620[];
extern const char D_0069C628[];
extern const char D_0069C630[];
extern const char D_0069C638[];
extern const char D_0069C640[];
extern const char D_0069C648[];
extern const char D_0069C650[];
extern const char D_0069C658[];
extern const char D_0069C660[];
extern const char D_0069C668[];
extern const char D_0069C670[];
extern const char D_0069C678[];
extern const char D_0069C680[];
extern const char D_0069C688[];
extern const char D_0069C690[];
extern const char D_0069C698[];
extern const char D_0069C6A0[];
extern const char D_0069C6A8[];
extern const char D_0069C6B0[];
extern const char D_0069C6B8[];
extern const char D_0069C6C0[];
extern const char D_0069C6C8[];
extern const char D_0069C6D0[];
extern const char D_0069C6D8[];
extern const char D_0069C6E0[];
extern const char D_0069C6E8[];
extern const char D_0069C6F0[];
extern const char D_0069C6F8[];
extern const char D_0069C700[];
extern const char D_0069C708[];
extern const char D_0069C710[];
extern const char D_0069C718[];
extern const char D_0069C720[];
extern const char D_0069C728[];
extern const char D_0069C730[];
extern const char D_0069C738[];
extern const char D_0069C740[];
extern const char D_0069C748[];
extern const char D_0069C750[];
extern const char D_0069C758[];
extern const char D_0069C760[];
extern const char D_0069C768[];
extern const char D_0069C770[];
extern const char D_0069C778[];
extern const char D_0069C780[];
extern const char D_0069C788[];
extern const char D_0069C790[];
extern const char D_0069C798[];
extern const char D_0069C7A0[];
extern const char D_0069C7A8[];
extern const char D_0069C7B0[];
extern const char D_0069C7B8[];
extern const char D_0069C7C0[];
extern const char D_0069C7C8[];
extern const char D_0069C7D0[];
extern const char D_0069C7D8[];
extern const char D_0069C7E0[];
extern const char D_0069C7E8[];
extern const char D_0069C7F0[];
extern const char D_0069C7F8[];
extern const char D_0069C800[];
extern const char D_0069C808[];
extern const char D_0069C810[];
extern const char D_0069C818[];
extern const char D_0069C820[];
extern const char D_0069C828[];
extern const char D_0069C830[];
extern const char D_0069C838[];
extern const char D_0069C840[];
extern const char D_0069C848[];
extern const char D_0069C850[];
extern const char D_0069C858[];
extern const char D_0069C860[];
extern const char D_0069C868[];
extern const char D_0069C870[];
extern const char D_0069C878[];
extern const char D_0069C880[];
extern const char D_0069C888[];
extern const char D_0069C890[];
extern const char D_0069C898[];
extern const char D_0069C8A0[];
extern const char D_0069C8A8[];
extern const char D_0069C8B0[];
extern const char D_0069C8B8[];
extern const char D_0069C8C0[];
extern const char D_0069C8C8[];
extern const char D_0069C8D0[];
extern const char D_0069C8D8[];
extern const char D_0069C8E0[];
extern const char D_0069C8E8[];
extern const char D_0069C8F0[];
extern const char D_0069C8F8[];
extern const char D_0069C900[];
extern const char D_0069C908[];
extern const char D_0069C910[];
extern const char D_0069C918[];
extern const char D_0069C920[];
extern const char D_0069C928[];
extern const char D_0069C930[];
extern const char D_0069C938[];
extern const char D_0069C940[];
extern const char D_0069C948[];
extern const char D_0069C950[];
extern const char D_0069C958[];
extern const char D_0069C960[];
extern const char D_0069C968[];
extern const char D_0069C970[];
extern const char D_0069C978[];
extern const char D_0069C980[];
extern const char D_0069C988[];
extern const char D_0069C990[];
extern const char D_0069C998[];
extern const char D_0069C9A0[];
extern const char D_0069C9A8[];
extern const char D_0069C9B0[];
extern const char D_0069C9B8[];
extern const char D_0069C9C0[];
extern const char D_0069C9C8[];
extern const char D_0069C9D0[];
extern const char D_0069C9D8[];
extern const char D_0069C9E0[];
extern const char D_0069C9E8[];
extern const char D_0069C9F0[];
extern const char D_0069C9F8[];
extern const char D_0069CA00[];
extern const char D_0069CA08[];
extern const char D_0069CA10[];
extern const char D_0069CA18[];
extern const char D_0069CA20[];
extern const char D_0069CA28[];
extern const char D_0069CA30[];
extern const char D_0069CA38[];
extern const char D_0069CA40[];
extern const char D_0069CA48[];
extern const char D_0069CA50[];
extern const char D_0069CA58[];
extern const char D_0069CA60[];
extern const char D_0069CA68[];
extern const char D_0069CA70[];
extern const char D_0069CA78[];
extern const char D_0069CA80[];
extern const char D_0069CA88[];
extern const char D_0069CA90[];
extern const char D_0069CA98[];
extern const char D_0069CAA0[];
extern const char D_0069CAA8[];
extern const char D_0069CAB0[];
extern const char D_0069CAB8[];
extern const char D_0069CAC0[];
extern const char D_0069CAC8[];
extern const char D_0069CAD0[];
extern const char D_0069CAD8[];
extern const char D_0069CAE0[];
extern const char D_0069CAE8[];
extern const char D_0069CAF0[];
extern const char D_0069CAF8[];
extern const char D_0069CB00[];
extern const char D_0069CB08[];
extern const char D_0069CB10[];
extern const char D_0069CB18[];
extern const char D_0069CB20[];
extern const char D_0069CB28[];
extern const char D_0069CB30[];
extern const char D_0069CB38[];
extern const char D_0069CB40[];
extern const char D_0069CB48[];
extern const char D_0069CB50[];
extern const char D_0069CB58[];
extern const char D_0069CB60[];
extern const char D_0069CB68[];
extern const char D_0069CB70[];
extern const char D_0069CB78[];
extern const char D_0069CB80[];
extern const char D_0069CB88[];
extern const char D_0069CB90[];
extern const char D_0069CB98[];
extern const char D_0069CBA0[];
extern const char D_0069CBA8[];
extern const char D_0069CBB0[];
extern const char D_0069CBB8[];
extern const char D_0069CBC0[];
extern const char D_0069CBC8[];
extern const char D_0069CBD0[];
extern const char D_0069CBD8[];
extern const char D_0069CBE0[];
extern const char D_0069CBE8[];
extern const char D_0069CBF0[];
extern const char D_0069CBF8[];
extern const char D_0069CC00[];
extern const char D_0069CC08[];
extern const char D_0069CC10[];
extern const char D_0069CC18[];
extern const char D_0069CC20[];
extern const char D_0069CC28[];
extern const char D_0069CC30[];
extern const char D_0069CC38[];
extern const char D_0069CC40[];
extern const char D_0069CC48[];
extern const char D_0069CC50[];
extern const char D_0069CC58[];
extern const char D_0069CC60[];
extern const char D_0069CC68[];
extern const char D_0069CC70[];
extern const char D_0069CC78[];
extern const char D_0069CC80[];
extern const char D_0069CC88[];
extern const char D_0069CC90[];
extern const char D_0069CC98[];
extern const char D_0069CCA0[];
extern const char D_0069CCA8[];
extern const char D_0069CCB0[];
extern const char D_0069CCB8[];
extern const char D_0069CCC0[];
extern const char D_0069CCC8[];
extern const char D_0069CCD0[];
extern const char D_0069CCD8[];
extern const char D_0069CCE0[];
extern const char D_0069CCE8[];
extern const char D_0069CCF0[];
extern const char D_0069CCF8[];
extern const char D_0069CD00[];
extern const char D_0069CD08[];
extern const char D_0069CD10[];
extern const char D_0069CD18[];
extern const char D_0069CD20[];
extern const char D_0069CD28[];
extern const char D_0069CD30[];
extern const char D_0069CD38[];
extern const char D_0069CD40[];
extern const char D_0069CD48[];
extern const char D_0069CD50[];
extern const char D_0069CD58[];
extern const char D_0069CD60[];
extern const char D_0069CD68[];
extern const char D_0069CD70[];
extern const char D_0069CD78[];
extern const char D_0069CD80[];
extern const char D_0069CD88[];
extern const char D_0069CD90[];
extern const char D_0069CD98[];
extern const char D_0069CDA0[];
extern const char D_0069CDA8[];
extern const char D_0069CDB0[];
extern const char D_0069CDB8[];
extern const char D_0069CDC0[];
extern const char D_0069CDC8[];
extern const char D_0069CDD0[];
extern const char D_0069CDD8[];
extern const char D_0069CDE0[];
extern const char D_0069CDE8[];
extern const char D_0069CDF0[];
extern const char D_0069CDF8[];
extern const char D_0069CE00[];
extern const char D_0069CE08[];
extern const char D_0069CE10[];
extern const char D_0069CE18[];
extern const char D_0069CE20[];
extern const char D_0069CE28[];
extern const char D_0069CE30[];
extern const char D_0069CE38[];
extern const char D_0069CE40[];
extern const char D_0069CE48[];
extern const char D_0069CE50[];
extern const char D_0069CE58[];
extern const char D_0069CE60[];
extern const char D_0069CE68[];
extern const char D_0069CE70[];
extern const char D_0069CE78[];
extern const char D_0069CE80[];
extern const char D_0069CE88[];
extern const char D_0069CE90[];
extern const char D_0069CE98[];
extern const char D_0069CEA0[];
extern const char D_0069CEA8[];
extern const char D_0069CEB0[];
extern const char D_0069CEB8[];
extern const char D_0069CEC0[];
extern const char D_0069CEC8[];
extern const char D_0069CED0[];
extern const char D_0069CED8[];

void *D_002B9CF8[58] = {
    (void *) D_00685900, (void *) D_006858F0, (void *) D_0069BAB8, (void *) D_0069BAB0,
    (void *) D_0069BAA8, (void *) D_0069BAA0, (void *) D_0069BA98, (void *) D_006858E0,
    (void *) D_006858D0, (void *) D_006858C0, (void *) D_006858B0, (void *) D_006858A0,
    (void *) D_00685890, (void *) D_00685880, (void *) D_00685870, (void *) D_00685860,
    (void *) D_0069BA90, (void *) D_0069BA88, (void *) D_0069BA80, (void *) D_0069BA78,
    (void *) D_00685850, (void *) D_0069BA70, (void *) D_0069BA68, (void *) D_0069BA60,
    (void *) D_0069BA58, (void *) D_0069BA50, (void *) D_0069BA48, (void *) D_0069BA40,
    (void *) D_0069BA38, (void *) D_0069BA30, (void *) D_0069BA28, (void *) D_0069BA20,
    (void *) D_0069BA18, (void *) D_0069BAB0, (void *) D_0069BA10, (void *) D_0069BA08,
    (void *) D_0069BA00, (void *) D_0069B9F8, (void *) D_0069B9F0, (void *) D_0069B9E8,
    (void *) D_0069B9E0, (void *) D_0069B9D8, (void *) D_00685840, (void *) D_00685830,
    (void *) D_0069B9D0, (void *) D_0069B9C8, (void *) D_0069B9C0, (void *) D_0069B9B8,
    (void *) D_0069BAB0, (void *) D_00685820, (void *) D_00685810, (void *) D_00685800,
    (void *) D_006857F0, (void *) D_006857E0, (void *) D_006857D0, (void *) D_006857C0,
    (void *) D_006857B0, (void *) D_006857A0
};

void *D_002B9DE0[90] = {
    (void *) D_00685C50, (void *) D_00685C40, (void *) D_0069BB38, (void *) D_00685C30,
    (void *) D_00685C20, (void *) D_00685C10, (void *) D_00685C00, (void *) D_00685BF0,
    (void *) D_00685BE0, (void *) D_00685BD0, (void *) D_00685BC0, (void *) D_0069BB30,
    (void *) D_00685BB0, (void *) D_0069BB28, (void *) D_0069BB20, (void *) D_00685BA0,
    (void *) D_00685B90, (void *) D_00685B80, (void *) D_00685B70, (void *) D_0069BB18,
    (void *) D_0069BB10, (void *) D_0069BB08, (void *) D_00685B60, (void *) D_0069BAB0,
    (void *) D_00685B50, (void *) D_00685B40, (void *) D_0069BAB0, (void *) D_00685B30,
    (void *) D_00685B20, (void *) D_00685B10, (void *) D_00685B00, (void *) D_0069BAB0,
    (void *) D_0069BAB0, (void *) D_0069BAB0, (void *) D_0069BAB0, (void *) D_00685AF0,
    (void *) D_0069BAB0, (void *) D_0069BAB0, (void *) D_0069BAB0, (void *) D_0069BAB0,
    (void *) D_0069BAB0, (void *) D_0069BAB0, (void *) D_0069BAB0, (void *) D_0069BAB0,
    (void *) D_0069BAB0, (void *) D_0069BB00, (void *) D_0069BAF8, (void *) D_0069BAB0,
    (void *) D_00685AE0, (void *) D_00685AD0, (void *) D_0069BAB0, (void *) D_0069BAF0,
    (void *) D_00685AC0, (void *) D_00685AB0, (void *) D_0069BAB0, (void *) D_0069BAB0,
    (void *) D_00685AA0, (void *) D_0069BAE8, (void *) D_0069BAB0, (void *) D_0069BAB0,
    (void *) D_00685A90, (void *) D_00685A80, (void *) D_00685A70, (void *) D_00685A60,
    (void *) D_00685A50, (void *) D_00685A40, (void *) D_00685A30, (void *) D_00685A20,
    (void *) D_00685A10, (void *) D_0069BAE0, (void *) D_0069BAD8, (void *) D_0069BAD0,
    (void *) D_00685A00, (void *) D_006859F0, (void *) D_006859E0, (void *) D_006859D0,
    (void *) D_0069BAC8, (void *) D_0069BAC0, (void *) D_006859C0, (void *) D_006859B0,
    (void *) D_006859A0, (void *) D_00685990, (void *) D_00685980, (void *) D_00685970,
    (void *) D_00685960, (void *) D_00685950, (void *) D_00685940, (void *) D_00685930,
    (void *) D_00685920, (void *) D_00685910
};

void *D_002B9F48[88] = {
    (void *) D_00685DE0, (void *) D_0069BD28, (void *) D_0069BD20, (void *) D_0069BD18,
    (void *) D_0069BD10, (void *) D_0069BD08, (void *) D_0069BD00, (void *) D_0069BAB0,
    (void *) D_0069BCF8, (void *) D_0069BCF0, (void *) D_00685DD0, (void *) D_0069BCE8,
    (void *) D_0069BCE0, (void *) D_0069BCD8, (void *) D_00685DC0, (void *) D_00685DB0,
    (void *) D_00685DA0, (void *) D_00685D90, (void *) D_00685D80, (void *) D_0069BCD0,
    (void *) D_0069BCC8, (void *) D_0069BCC0, (void *) D_0069BCB8, (void *) D_0069BCB0,
    (void *) D_0069BCA8, (void *) D_0069BCA0, (void *) D_0069BC98, (void *) D_0069BC90,
    (void *) D_0069BC88, (void *) D_0069BC80, (void *) D_0069BC78, (void *) D_0069BC70,
    (void *) D_0069BC68, (void *) D_0069BC60, (void *) D_0069BC58, (void *) D_0069BC50,
    (void *) D_0069BC48, (void *) D_00685D70, (void *) D_00685D60, (void *) D_0069BC40,
    (void *) D_00685D50, (void *) D_00685D40, (void *) D_0069BC38, (void *) D_00685D30,
    (void *) D_0069BC30, (void *) D_0069BC28, (void *) D_0069BC20, (void *) D_00685D20,
    (void *) D_0069BC18, (void *) D_00685D10, (void *) D_00685D00, (void *) D_00685CF0,
    (void *) D_00685CE0, (void *) D_00685CD0, (void *) D_0069BC10, (void *) D_0069BC08,
    (void *) D_0069BC00, (void *) D_00685CC0, (void *) D_00685CB0, (void *) D_00685CA0,
    (void *) D_00685C90, (void *) D_00685C80, (void *) D_00685C70, (void *) D_0069BBF8,
    (void *) D_0069BBF0, (void *) D_0069BBE8, (void *) D_0069BBE0, (void *) D_0069BBD8,
    (void *) D_0069BBD0, (void *) D_0069BBC8, (void *) D_00685C60, (void *) D_0069BBC0,
    (void *) D_0069BBB8, (void *) D_0069BBB0, (void *) D_0069BBA8, (void *) D_0069BBA0,
    (void *) D_0069BB98, (void *) D_0069BB90, (void *) D_0069BB88, (void *) D_0069BB80,
    (void *) D_0069BB78, (void *) D_0069BB70, (void *) D_0069BB68, (void *) D_0069BB60,
    (void *) D_0069BB58, (void *) D_0069BB50, (void *) D_0069BB48, (void *) D_0069BB40
};

void *D_002BA0A8[72] = {
    (void *) D_00685EC8, (void *) D_0069BEF8, (void *) D_0069BEF0, (void *) D_0069BEE8,
    (void *) D_0069BEE0, (void *) D_0069BED8, (void *) D_0069BED0, (void *) D_0069BEC8,
    (void *) D_0069BEC0, (void *) D_0069BEB8, (void *) D_0069BEB0, (void *) D_0069BEA8,
    (void *) D_0069BEA0, (void *) D_0069BE98, (void *) D_0069BE90, (void *) D_0069BE88,
    (void *) D_0069BE80, (void *) D_0069BE78, (void *) D_0069BE70, (void *) D_0069BE68,
    (void *) D_0069BE60, (void *) D_0069BE58, (void *) D_0069BE50, (void *) D_0069BE48,
    (void *) D_0069BE40, (void *) D_0069BE38, (void *) D_0069BE30, (void *) D_0069BE28,
    (void *) D_0069BE20, (void *) D_0069BE18, (void *) D_0069BE10, (void *) D_0069BE08,
    (void *) D_0069BE00, (void *) D_0069BDF8, (void *) D_0069BDF0, (void *) D_0069BDE8,
    (void *) D_0069BDE0, (void *) D_0069BDD8, (void *) D_0069BDD0, (void *) D_0069BDC8,
    (void *) D_0069BDC0, (void *) D_00685EB8, (void *) D_00685EA8, (void *) D_00685E98,
    (void *) D_00685E88, (void *) D_00685E78, (void *) D_00685E68, (void *) D_00685E58,
    (void *) D_00685E48, (void *) D_0069BDB8, (void *) D_0069BDB0, (void *) D_0069BDA8,
    (void *) D_0069BDA0, (void *) D_0069BD98, (void *) D_0069BD90, (void *) D_0069BD88,
    (void *) D_0069BD80, (void *) D_0069BD78, (void *) D_0069BD70, (void *) D_00685E38,
    (void *) D_0069BD68, (void *) D_00685E28, (void *) D_0069BD60, (void *) D_0069BD58,
    (void *) D_00685E18, (void *) D_00685E08, (void *) D_0069BD50, (void *) D_0069BD48,
    (void *) D_0069BD40, (void *) D_00685DF8, (void *) D_0069BD38, (void *) D_0069BD30
};

void *D_002BA1C8[20] = {
    (void *) D_00685F50, (void *) D_0069BF40, (void *) D_0069BF38, (void *) D_0069BF30,
    (void *) D_0069BAB0, (void *) D_0069BF28, (void *) D_00685F40, (void *) D_00685F30,
    (void *) D_0069BF20, (void *) D_0069BF18, (void *) D_0069BF10, (void *) D_0069BF08,
    (void *) D_00685F20, (void *) D_00685F10, (void *) D_00685F00, (void *) D_00685F40,
    (void *) D_00685EF0, (void *) D_00685EE0, (void *) D_0069BF00, 0
};

void *D_002BA218[34] = {
    (void *) D_00685FF8, (void *) D_0069BFF8, (void *) D_0069BFF0, (void *) D_0069BFE8,
    (void *) D_0069BFE0, (void *) D_0069BFD8, (void *) D_0069BFD0, (void *) D_0069BFC8,
    (void *) D_0069BFC0, (void *) D_0069BFB8, (void *) D_0069BFB0, (void *) D_0069BFA8,
    (void *) D_0069BFA0, (void *) D_0069BF98, (void *) D_0069BF90, (void *) D_0069BF88,
    (void *) D_0069BF80, (void *) D_0069BF78, (void *) D_0069BF70, (void *) D_0069BF68,
    (void *) D_0069BF60, (void *) D_0069BF58, (void *) D_0069BF50, (void *) D_0069BF48,
    (void *) D_00685FE8, (void *) D_00685FD8, (void *) D_00685FC8, (void *) D_00685FB8,
    (void *) D_00685FA8, (void *) D_00685F98, (void *) D_00685F88, (void *) D_00685F78,
    (void *) D_00685F68, 0
};

void *D_002BA2A0[82] = {
    (void *) D_00686520, (void *) D_00686510, (void *) D_00686500, (void *) D_006864F0,
    (void *) D_006864E0, (void *) D_006864D0, (void *) D_006864C0, (void *) D_006864B0,
    (void *) D_006864A0, (void *) D_00686490, (void *) D_00686480, (void *) D_00686470,
    (void *) D_00686460, (void *) D_00686450, (void *) D_00686440, (void *) D_00686430,
    (void *) D_00686420, (void *) D_00686410, (void *) D_00686400, (void *) D_006863F0,
    (void *) D_006863E0, (void *) D_006863D0, (void *) D_006863C0, (void *) D_006863B0,
    (void *) D_006863A0, (void *) D_00686390, (void *) D_00686380, (void *) D_00686370,
    (void *) D_00686360, (void *) D_00686350, (void *) D_00686340, (void *) D_00686330,
    (void *) D_00686320, (void *) D_00686310, (void *) D_00686300, (void *) D_006862F0,
    (void *) D_006862E0, (void *) D_006862D0, (void *) D_006862C0, (void *) D_006862B0,
    (void *) D_006862A0, (void *) D_00686290, (void *) D_00686280, (void *) D_00686270,
    (void *) D_00686260, (void *) D_00686250, (void *) D_00686240, (void *) D_00686230,
    (void *) D_00686220, (void *) D_00686210, (void *) D_00686200, (void *) D_006861F0,
    (void *) D_006861E0, (void *) D_006861D0, (void *) D_006861C0, (void *) D_006861B0,
    (void *) D_006861A0, (void *) D_00686190, (void *) D_00686180, (void *) D_00686170,
    (void *) D_00686160, (void *) D_00686150, (void *) D_00686140, (void *) D_00686130,
    (void *) D_00686120, (void *) D_00686110, (void *) D_00686100, (void *) D_006860F0,
    (void *) D_006860E0, (void *) D_006860D0, (void *) D_006860C0, (void *) D_006860B0,
    (void *) D_006860A0, (void *) D_00686090, (void *) D_00686080, (void *) D_00686070,
    (void *) D_00686060, (void *) D_00686050, (void *) D_00686040, (void *) D_00686030,
    (void *) D_00686020, (void *) D_00686010
};

void *D_002BA3E8[84] = {
    (void *) D_00686558, (void *) D_0069C258, (void *) D_0069C250, (void *) D_0069C248,
    (void *) D_0069C240, (void *) D_0069C238, (void *) D_0069C230, (void *) D_0069C228,
    (void *) D_0069C220, (void *) D_0069C218, (void *) D_0069C210, (void *) D_0069C208,
    (void *) D_0069C200, (void *) D_0069C1F8, (void *) D_0069C1F0, (void *) D_0069C1E8,
    (void *) D_0069C1E0, (void *) D_0069C1D8, (void *) D_0069C1D0, (void *) D_0069C1C8,
    (void *) D_0069C1C0, (void *) D_0069C1B8, (void *) D_0069C1B0, (void *) D_0069C1A8,
    (void *) D_0069C1A0, (void *) D_0069C198, (void *) D_0069C190, (void *) D_0069C188,
    (void *) D_0069C180, (void *) D_0069C178, (void *) D_0069C170, (void *) D_0069C168,
    (void *) D_0069BBA8, (void *) D_0069BBA0, (void *) D_0069C160, (void *) D_0069C158,
    (void *) D_0069C150, (void *) D_0069C148, (void *) D_0069C140, (void *) D_0069C138,
    (void *) D_0069C130, (void *) D_0069C128, (void *) D_0069C120, (void *) D_0069C118,
    (void *) D_0069C110, (void *) D_0069C108, (void *) D_0069C100, (void *) D_0069C0F8,
    (void *) D_0069C0F0, (void *) D_0069C0E8, (void *) D_0069C0E0, (void *) D_0069C0D8,
    (void *) D_0069C0D0, (void *) D_0069C0C8, (void *) D_0069C0C0, (void *) D_0069BBC0,
    (void *) D_0069BBB8, (void *) D_0069C0B8, (void *) D_0069C0B0, (void *) D_0069C0A8,
    (void *) D_0069C0A0, (void *) D_0069C098, (void *) D_0069C090, (void *) D_0069C088,
    (void *) D_0069C080, (void *) D_0069C078, (void *) D_0069C070, (void *) D_0069C068,
    (void *) D_0069C060, (void *) D_0069C058, (void *) D_0069C050, (void *) D_0069C048,
    (void *) D_0069C040, (void *) D_0069C038, (void *) D_0069C030, (void *) D_0069C028,
    (void *) D_0069C020, (void *) D_0069C018, (void *) D_0069C010, (void *) D_0069C008,
    (void *) D_00686548, (void *) D_00686538, (void *) D_0069C000, 0
};

void *D_002BA538[32] = {
    (void *) D_00686558, (void *) D_0069BAD8, (void *) D_0069C338, (void *) D_0069C330,
    (void *) D_0069C328, (void *) D_0069C320, (void *) D_0069C318, (void *) D_0069C310,
    (void *) D_0069C308, (void *) D_0069C300, (void *) D_0069C2F8, (void *) D_0069C2F0,
    (void *) D_0069C2E8, (void *) D_0069BAE0, (void *) D_0069C2E0, (void *) D_0069C2D8,
    (void *) D_0069C2D0, (void *) D_0069C2C8, (void *) D_0069C2C0, (void *) D_0069C2B8,
    (void *) D_0069C2B0, (void *) D_0069C2A8, (void *) D_0069C2A0, (void *) D_0069C298,
    (void *) D_0069C290, (void *) D_0069C288, (void *) D_0069C280, (void *) D_0069C278,
    (void *) D_0069C270, (void *) D_0069C268, (void *) D_0069BBB0, (void *) D_0069C260
};

void *D_002BA5B8[362] = {
    (void *) D_00687A88, (void *) D_00687A78, (void *) D_00687A68, (void *) D_0069C3F0,
    (void *) D_00687A58, (void *) D_00687A48, (void *) D_00687A38, (void *) D_00687A28,
    (void *) D_00687A18, (void *) D_00687A08, (void *) D_006879F8, (void *) D_006879E8,
    (void *) D_006879D8, (void *) D_006879C8, (void *) D_006879B8, (void *) D_006879A8,
    (void *) D_00687998, (void *) D_00687988, (void *) D_00687978, (void *) D_00687968,
    (void *) D_00687958, (void *) D_00687948, (void *) D_00687938, (void *) D_00687928,
    (void *) D_00687918, (void *) D_00687908, (void *) D_006878F8, (void *) D_006878E8,
    (void *) D_006878D8, (void *) D_006878C8, (void *) D_006878B8, (void *) D_006878A8,
    (void *) D_00687898, (void *) D_00687888, (void *) D_00687878, (void *) D_00687868,
    (void *) D_00687858, (void *) D_00687848, (void *) D_00687838, (void *) D_00687828,
    (void *) D_00687818, (void *) D_00687808, (void *) D_006877F8, (void *) D_006877E8,
    (void *) D_006877D8, (void *) D_006877C8, (void *) D_006877B8, (void *) D_006877A8,
    (void *) D_00687798, (void *) D_00687788, (void *) D_00687778, (void *) D_00687768,
    (void *) D_00687758, (void *) D_00687748, (void *) D_00687738, (void *) D_00687728,
    (void *) D_00687718, (void *) D_00687708, (void *) D_006876F8, (void *) D_006876E8,
    (void *) D_006876D8, (void *) D_006876C8, (void *) D_006876B8, (void *) D_006876A8,
    (void *) D_00687698, (void *) D_00687688, (void *) D_00687678, (void *) D_00687668,
    (void *) D_00687658, (void *) D_00687648, (void *) D_00687638, (void *) D_00687628,
    (void *) D_00687618, (void *) D_00687608, (void *) D_006875F8, (void *) D_006875E8,
    (void *) D_006875D8, (void *) D_006875C8, (void *) D_006875B8, (void *) D_006875A8,
    (void *) D_00687598, (void *) D_00687588, (void *) D_00687578, (void *) D_00687568,
    (void *) D_00687558, (void *) D_00687548, (void *) D_00687538, (void *) D_00687528,
    (void *) D_00687518, (void *) D_00687508, (void *) D_006874F8, (void *) D_006874E8,
    (void *) D_006874D8, (void *) D_006874C8, (void *) D_006874B8, (void *) D_006874A8,
    (void *) D_00687498, (void *) D_00687488, (void *) D_00687478, (void *) D_00687468,
    (void *) D_00687458, (void *) D_00687448, (void *) D_00687438, (void *) D_00687428,
    (void *) D_00687418, (void *) D_00687408, (void *) D_006873F8, (void *) D_006873E8,
    (void *) D_006873D8, (void *) D_006873C8, (void *) D_006873B8, (void *) D_006873A8,
    (void *) D_00687398, (void *) D_00687388, (void *) D_00687378, (void *) D_00687368,
    (void *) D_00687358, (void *) D_00687348, (void *) D_00687338, (void *) D_00687328,
    (void *) D_00687318, (void *) D_00687308, (void *) D_006872F8, (void *) D_006872E8,
    (void *) D_006872D8, (void *) D_006872C8, (void *) D_006872B8, (void *) D_006872A8,
    (void *) D_00687298, (void *) D_00687288, (void *) D_00687278, (void *) D_00687268,
    (void *) D_00687258, (void *) D_00687248, (void *) D_00687238, (void *) D_00687228,
    (void *) D_00687218, (void *) D_00687208, (void *) D_006871F8, (void *) D_006871E8,
    (void *) D_006871D8, (void *) D_006871C8, (void *) D_006871B8, (void *) D_006871A8,
    (void *) D_00687198, (void *) D_00687188, (void *) D_00687178, (void *) D_00687168,
    (void *) D_00687158, (void *) D_00687148, (void *) D_00687138, (void *) D_00687128,
    (void *) D_00687118, (void *) D_00687108, (void *) D_006870F8, (void *) D_006870E8,
    (void *) D_006870D8, (void *) D_006870C8, (void *) D_006870B8, (void *) D_006870A8,
    (void *) D_00687098, (void *) D_00687088, (void *) D_00687078, (void *) D_00687068,
    (void *) D_00687058, (void *) D_00687048, (void *) D_00687038, (void *) D_00687028,
    (void *) D_00687018, (void *) D_00687008, (void *) D_00686FF8, (void *) D_00686FE8,
    (void *) D_00686FD8, (void *) D_00686FC8, (void *) D_00686FB8, (void *) D_00686FA8,
    (void *) D_00686F98, (void *) D_00686F88, (void *) D_00686F78, (void *) D_00686F68,
    (void *) D_00686F58, (void *) D_00686F48, (void *) D_00686F38, (void *) D_00686F28,
    (void *) D_00686F18, (void *) D_00686F08, (void *) D_00686EF8, (void *) D_00686EE8,
    (void *) D_00686ED8, (void *) D_00686EC8, (void *) D_00686EB8, (void *) D_00686EA8,
    (void *) D_00686E98, (void *) D_00686E88, (void *) D_00686E78, (void *) D_00686E68,
    (void *) D_00686E58, (void *) D_00686E48, (void *) D_00686E38, (void *) D_00686E28,
    (void *) D_00686E18, (void *) D_00686E08, (void *) D_00686DF8, (void *) D_00686DE8,
    (void *) D_00686DD8, (void *) D_00686DC8, (void *) D_00686DB8, (void *) D_00686DA8,
    (void *) D_00686D98, (void *) D_00686D88, (void *) D_00686D78, (void *) D_00686D68,
    (void *) D_00686D58, (void *) D_00686D48, (void *) D_00686D38, (void *) D_00686D28,
    (void *) D_00686D18, (void *) D_00686D08, (void *) D_00686CF8, (void *) D_00686CE8,
    (void *) D_00686CD8, (void *) D_00686CC8, (void *) D_00686CB8, (void *) D_00686CA8,
    (void *) D_00686C98, (void *) D_00686C88, (void *) D_00686C78, (void *) D_00686C68,
    (void *) D_00686C58, (void *) D_00686C48, (void *) D_00686C38, (void *) D_00686C28,
    (void *) D_00686C18, (void *) D_00686C08, (void *) D_00686BF8, (void *) D_0069C3E8,
    (void *) D_0069C3E0, (void *) D_0069C3D8, (void *) D_0069C3D0, (void *) D_0069C3C8,
    (void *) D_0069C3C0, (void *) D_0069C3B8, (void *) D_0069C3B0, (void *) D_0069C3A8,
    (void *) D_0069C3A0, (void *) D_0069C398, (void *) D_0069C390, (void *) D_0069C388,
    (void *) D_0069C380, (void *) D_0069C378, (void *) D_00686BE8, (void *) D_00686BD8,
    (void *) D_00686BC8, (void *) D_00686BB8, (void *) D_00686BA8, (void *) D_00686B98,
    (void *) D_00686B88, (void *) D_00686B78, (void *) D_00686B68, (void *) D_00686B58,
    (void *) D_00686B48, (void *) D_00686B38, (void *) D_00686B28, (void *) D_00686B18,
    (void *) D_00686B08, (void *) D_00686AF8, (void *) D_00686AE8, (void *) D_00686AD8,
    (void *) D_00686AC8, (void *) D_00686AB8, (void *) D_00686AA8, (void *) D_00686A98,
    (void *) D_00686A88, (void *) D_00686A78, (void *) D_00686A68, (void *) D_00686A58,
    (void *) D_00686A48, (void *) D_00686A38, (void *) D_00686A28, (void *) D_00686A18,
    (void *) D_00686A08, (void *) D_006869F8, (void *) D_006869E8, (void *) D_006869D8,
    (void *) D_006869C8, (void *) D_006869B8, (void *) D_006869A8, (void *) D_00686998,
    (void *) D_00686988, (void *) D_00686978, (void *) D_00686968, (void *) D_00686958,
    (void *) D_00686948, (void *) D_00686938, (void *) D_00686928, (void *) D_00686918,
    (void *) D_00686908, (void *) D_006868F8, (void *) D_006868E8, (void *) D_006868D8,
    (void *) D_006868C8, (void *) D_006868B8, (void *) D_006868A8, (void *) D_00686898,
    (void *) D_00686888, (void *) D_00686878, (void *) D_00686868, (void *) D_00686858,
    (void *) D_00686848, (void *) D_00686838, (void *) D_00686828, (void *) D_00686818,
    (void *) D_00686808, (void *) D_006867F8, (void *) D_006867E8, (void *) D_006867D8,
    (void *) D_006867C8, (void *) D_006867B8, (void *) D_006867A8, (void *) D_00686798,
    (void *) D_00686788, (void *) D_00686778, (void *) D_00686768, (void *) D_00686758,
    (void *) D_00686748, (void *) D_00686738, (void *) D_00686728, (void *) D_00686718,
    (void *) D_00686708, (void *) D_006866F8, (void *) D_006866E8, (void *) D_006866D8,
    (void *) D_006866C8, (void *) D_006866B8, (void *) D_006866A8, (void *) D_00686698,
    (void *) D_00686688, (void *) D_00686678, (void *) D_00686668, (void *) D_00686658,
    (void *) D_00686648, (void *) D_00686638, (void *) D_00686628, (void *) D_00686618,
    (void *) D_00686608, (void *) D_006865F8, (void *) D_006865E8, (void *) D_006865D8,
    (void *) D_006865C8, (void *) D_006865B8, (void *) D_006865A8, (void *) D_00686598,
    (void *) D_00686588, (void *) D_00686578, (void *) D_0069C370, (void *) D_0069C368,
    (void *) D_0069C360, (void *) D_0069C358, (void *) D_00686568, (void *) D_0069C350,
    (void *) D_0069C348, (void *) D_0069C340
};

void *D_002BAB60[66] = {
    (void *) D_0069C420, (void *) D_0069BA90, (void *) D_0069B9F0, (void *) D_0069BAB8,
    (void *) D_0069BA38, (void *) D_0069BA08, (void *) D_0069BA68, (void *) D_006858B0,
    (void *) D_0069C418, (void *) D_0069C410, (void *) D_0069C408, (void *) D_0069C400,
    (void *) D_0069BF30, (void *) D_0069C3F8, (void *) D_0069BF28, (void *) D_00687DB8,
    (void *) D_00687DA8, (void *) D_00687D98, (void *) D_00687D88, (void *) D_00687D78,
    (void *) D_00687D68, (void *) D_00687D58, (void *) D_00687D48, (void *) D_00687D38,
    (void *) D_00687D28, (void *) D_00687D18, (void *) D_00687D08, (void *) D_00687CF8,
    (void *) D_00687CE8, (void *) D_00687CD8, (void *) D_00687CC8, (void *) D_00687CB8,
    (void *) D_00687CA8, (void *) D_00687C98, (void *) D_00687C88, (void *) D_00687C78,
    (void *) D_00687C68, (void *) D_00687C58, (void *) D_00687C48, (void *) D_00687C38,
    (void *) D_00687C28, (void *) D_00687C18, (void *) D_00687C08, (void *) D_00687BF8,
    (void *) D_00687BE8, (void *) D_00687BD8, (void *) D_00687BC8, (void *) D_00687BB8,
    (void *) D_00687BA8, (void *) D_00687B98, (void *) D_00687B88, (void *) D_00687B78,
    (void *) D_00687B68, (void *) D_00687B58, (void *) D_00687B48, (void *) D_00687B38,
    (void *) D_00687B28, (void *) D_00687B18, (void *) D_00687B08, (void *) D_00687AF8,
    (void *) D_00687AE8, (void *) D_00687AD8, (void *) D_00687AC8, (void *) D_00687AB8,
    (void *) D_00687AA8, (void *) D_00687A98
};

void *D_002BAC68[10] = {
    (void *) D_0069C468, (void *) D_0069C460, (void *) D_0069C458, (void *) D_0069C450,
    (void *) D_0069C448, (void *) D_0069C440, (void *) D_0069C438, (void *) D_0069C430,
    (void *) D_0069C428, 0
};

void *D_002BAC90[326] = {
    (void *) D_00687FB8, (void *) D_0069CD88, (void *) D_0069CD80, (void *) D_0069CD78,
    (void *) D_0069CD70, (void *) D_0069CD68, (void *) D_0069CD60, (void *) D_0069CD58,
    (void *) D_0069CD50, (void *) D_0069CD48, (void *) D_0069CD40, (void *) D_0069CD38,
    (void *) D_0069CD30, (void *) D_0069CD28, (void *) D_0069CD20, (void *) D_0069CD18,
    (void *) D_00687FA8, (void *) D_00687F98, (void *) D_0069CD10, (void *) D_0069CD08,
    (void *) D_0069CD00, (void *) D_0069CCF8, (void *) D_0069CCF0, (void *) D_0069CCE8,
    (void *) D_0069CCE0, (void *) D_0069CCD8, (void *) D_0069CCD0, (void *) D_0069CCC8,
    (void *) D_0069CCC0, (void *) D_0069CCB8, (void *) D_00687F88, (void *) D_00687F78,
    (void *) D_00687F68, (void *) D_00687F58, (void *) D_00687F48, (void *) D_00687F38,
    (void *) D_00687F28, (void *) D_00687F18, (void *) D_00687F08, (void *) D_00687EF8,
    (void *) D_00687EE8, (void *) D_00687ED8, (void *) D_00687EC8, (void *) D_00687EB8,
    (void *) D_00687EA8, (void *) D_00687E98, (void *) D_00687E88, (void *) D_00687E78,
    (void *) D_00687E68, (void *) D_00687E58, (void *) D_00687E48, (void *) D_00687E38,
    (void *) D_00687E28, (void *) D_0069CCB0, (void *) D_0069CCA8, (void *) D_0069CCA0,
    (void *) D_0069CC98, (void *) D_0069CC90, (void *) D_0069CC88, (void *) D_00687E18,
    (void *) D_00687E08, (void *) D_0069CC80, (void *) D_0069CC78, (void *) D_0069CC70,
    (void *) D_0069CC68, (void *) D_0069CC60, (void *) D_0069CC58, (void *) D_0069CC50,
    (void *) D_0069CC48, (void *) D_0069CC40, (void *) D_0069CC38, (void *) D_0069CC30,
    (void *) D_0069CC28, (void *) D_0069CC20, (void *) D_0069CC18, (void *) D_0069CC10,
    (void *) D_0069CC08, (void *) D_0069CC00, (void *) D_0069CBF8, (void *) D_0069CBF0,
    (void *) D_0069CBE8, (void *) D_0069CBE0, (void *) D_0069CBD8, (void *) D_0069CBD0,
    (void *) D_0069CBC8, (void *) D_0069CBC0, (void *) D_0069CBB8, (void *) D_0069CBB0,
    (void *) D_0069CBA8, (void *) D_0069CBA0, (void *) D_0069CB98, (void *) D_0069CB90,
    (void *) D_0069CB88, (void *) D_0069CB80, (void *) D_00687DF8, (void *) D_0069CB78,
    (void *) D_0069CB70, (void *) D_0069CB68, (void *) D_0069CB60, (void *) D_0069CB58,
    (void *) D_0069CB50, (void *) D_0069CB48, (void *) D_0069CB40, (void *) D_0069CB38,
    (void *) D_0069CB30, (void *) D_0069CB28, (void *) D_0069CB20, (void *) D_0069CB18,
    (void *) D_0069CB10, (void *) D_0069CB08, (void *) D_0069CB00, (void *) D_0069CAF8,
    (void *) D_0069CAF0, (void *) D_0069CAE8, (void *) D_0069CAE0, (void *) D_0069CAD8,
    (void *) D_0069CAD0, (void *) D_0069CAC8, (void *) D_0069CAC0, (void *) D_0069CAB8,
    (void *) D_0069CAB0, (void *) D_0069CAA8, (void *) D_0069CAA0, (void *) D_0069CA98,
    (void *) D_0069CA90, (void *) D_0069CA88, (void *) D_0069CA80, (void *) D_0069CA78,
    (void *) D_0069CA70, (void *) D_0069CA68, (void *) D_0069CA60, (void *) D_0069CA58,
    (void *) D_0069CA50, (void *) D_0069CA48, (void *) D_0069CA40, (void *) D_0069CA38,
    (void *) D_0069CA30, (void *) D_0069CA28, (void *) D_0069CA20, (void *) D_0069CA18,
    (void *) D_0069CA10, (void *) D_0069CA08, (void *) D_0069CA00, (void *) D_0069C9F8,
    (void *) D_0069C9F0, (void *) D_0069C9E8, (void *) D_0069C9E0, (void *) D_0069C9D8,
    (void *) D_0069C9D0, (void *) D_0069C9C8, (void *) D_0069C9C0, (void *) D_0069C9B8,
    (void *) D_0069C9B0, (void *) D_0069C9A8, (void *) D_0069C9A0, (void *) D_0069C998,
    (void *) D_0069C990, (void *) D_0069C988, (void *) D_0069C980, (void *) D_0069C978,
    (void *) D_0069C970, (void *) D_0069C968, (void *) D_0069C960, (void *) D_0069C958,
    (void *) D_0069C950, (void *) D_0069C948, (void *) D_0069C940, (void *) D_0069C938,
    (void *) D_0069C930, (void *) D_0069C928, (void *) D_0069C920, (void *) D_0069C918,
    (void *) D_0069C910, (void *) D_0069C908, (void *) D_0069C900, (void *) D_0069BAB0,
    (void *) D_0069C8F8, (void *) D_0069C8F0, (void *) D_0069C8E8, (void *) D_0069C8E0,
    (void *) D_0069C8D8, (void *) D_0069C8D0, (void *) D_0069C8C8, (void *) D_0069C8C0,
    (void *) D_0069C8B8, (void *) D_0069C8B0, (void *) D_0069C8A8, (void *) D_0069C8A0,
    (void *) D_0069C898, (void *) D_0069C890, (void *) D_0069C888, (void *) D_0069C880,
    (void *) D_0069C878, (void *) D_0069C870, (void *) D_0069C868, (void *) D_0069C860,
    (void *) D_0069C858, (void *) D_0069C850, (void *) D_0069C848, (void *) D_0069C840,
    (void *) D_0069C838, (void *) D_0069C830, (void *) D_0069C828, (void *) D_0069C820,
    (void *) D_0069C818, (void *) D_0069C810, (void *) D_0069C808, (void *) D_0069C800,
    (void *) D_0069C7F8, (void *) D_0069C7F0, (void *) D_0069C7E8, (void *) D_0069C7E0,
    (void *) D_0069C7D8, (void *) D_0069C7D0, (void *) D_0069C7C8, (void *) D_0069C7C0,
    (void *) D_0069C7B8, (void *) D_0069C7B0, (void *) D_0069C7A8, (void *) D_0069C7A0,
    (void *) D_0069C798, (void *) D_0069C790, (void *) D_0069C788, (void *) D_0069C780,
    (void *) D_0069C778, (void *) D_0069C770, (void *) D_0069C768, (void *) D_0069C760,
    (void *) D_0069C758, (void *) D_0069C750, (void *) D_0069C748, (void *) D_0069C740,
    (void *) D_0069C738, (void *) D_0069C730, (void *) D_0069C728, (void *) D_0069C720,
    (void *) D_0069C718, (void *) D_0069C710, (void *) D_0069C708, (void *) D_0069C700,
    (void *) D_0069C6F8, (void *) D_0069C6F0, (void *) D_0069C6E8, (void *) D_0069C6E0,
    (void *) D_0069C6D8, (void *) D_0069C6D0, (void *) D_0069C6C8, (void *) D_0069C6C0,
    (void *) D_0069C6B8, (void *) D_0069C6B0, (void *) D_0069C6A8, (void *) D_0069C6A0,
    (void *) D_0069C698, (void *) D_0069C690, (void *) D_0069C688, (void *) D_0069C680,
    (void *) D_0069C678, (void *) D_0069C670, (void *) D_0069C668, (void *) D_0069C660,
    (void *) D_0069C658, (void *) D_0069C650, (void *) D_0069C648, (void *) D_0069C640,
    (void *) D_0069C638, (void *) D_0069C630, (void *) D_0069C628, (void *) D_0069C620,
    (void *) D_0069C618, (void *) D_0069C610, (void *) D_0069C608, (void *) D_0069C600,
    (void *) D_0069C5F8, (void *) D_0069C5F0, (void *) D_0069C5E8, (void *) D_0069C5E0,
    (void *) D_0069C5D8, (void *) D_0069C5D0, (void *) D_0069C5C8, (void *) D_0069C5C0,
    (void *) D_0069C5B8, (void *) D_0069C5B0, (void *) D_0069C5A8, (void *) D_0069C5A0,
    (void *) D_0069C598, (void *) D_0069C590, (void *) D_0069C588, (void *) D_0069C580,
    (void *) D_0069C578, (void *) D_0069C570, (void *) D_0069C568, (void *) D_0069C560,
    (void *) D_0069C558, (void *) D_0069C550, (void *) D_0069C548, (void *) D_0069C540,
    (void *) D_0069C538, (void *) D_0069C530, (void *) D_0069C528, (void *) D_0069C520,
    (void *) D_0069C518, (void *) D_0069C510, (void *) D_0069C508, (void *) D_0069C500,
    (void *) D_0069C4F8, (void *) D_0069C4F0, (void *) D_0069C4E8, (void *) D_0069C4E0,
    (void *) D_0069C4D8, (void *) D_0069C4D0, (void *) D_0069C4C8, (void *) D_0069C4C0,
    (void *) D_0069C4B8, (void *) D_0069C4B0, (void *) D_0069C4A8, (void *) D_0069C4A0,
    (void *) D_0069C498, (void *) D_0069C490, (void *) D_0069C488, (void *) D_0069C480,
    (void *) D_0069C478, (void *) D_0069C470, (void *) D_00687DE8, (void *) D_00687DD8,
    (void *) D_00687DC8, 0
};

void *D_002BB1A8[60] = {
    (void *) D_0069CE30, (void *) D_00688228, (void *) D_00688218, (void *) D_00688208,
    (void *) D_006881F8, (void *) D_006881E8, (void *) D_006881D8, (void *) D_006881C8,
    (void *) D_006881B8, (void *) D_006881A8, (void *) D_00688198, (void *) D_00688188,
    (void *) D_00688178, (void *) D_00688168, (void *) D_00688158, (void *) D_00688148,
    (void *) D_00688138, (void *) D_00688128, (void *) D_00688118, (void *) D_00688108,
    (void *) D_006880F8, (void *) D_006880E8, (void *) D_006880D8, (void *) D_006880C8,
    (void *) D_006880B8, (void *) D_006880A8, (void *) D_00688098, (void *) D_00688088,
    (void *) D_00688078, (void *) D_00688068, (void *) D_00688058, (void *) D_00688048,
    (void *) D_00688038, (void *) D_00688028, (void *) D_00688018, (void *) D_00688008,
    (void *) D_00687FF8, (void *) D_00687FE8, (void *) D_00687FD8, (void *) D_00687FC8,
    (void *) D_0069CE28, (void *) D_0069CE20, (void *) D_0069CE18, (void *) D_0069CE10,
    (void *) D_0069CE08, (void *) D_0069CE00, (void *) D_0069CDF8, (void *) D_0069CDF0,
    (void *) D_0069CDE8, (void *) D_0069CDE0, (void *) D_0069CDD8, (void *) D_0069CDD0,
    (void *) D_0069CDC8, (void *) D_0069CDC0, (void *) D_0069CDB8, (void *) D_0069CDB0,
    (void *) D_0069CDA8, (void *) D_0069CDA0, (void *) D_0069CD98, (void *) D_0069CD90
};

unsigned int xglCdFileGroups[46] = {
    0x10000u, 58, (unsigned int) D_002B9CF8, 0x11000u, 90, (unsigned int) D_002B9DE0, 0x12000u, 88,
    (unsigned int) D_002B9F48, 0x13000u, 72, (unsigned int) D_002BA0A8, 0x14000u, 19, (unsigned int) D_002BA1C8, 0x15000u,
    33, (unsigned int) D_002BA218, 0x16000u, 82, (unsigned int) D_002BA2A0, 0x18000u, 83, (unsigned int) D_002BA3E8,
    0x19000u, 32, (unsigned int) D_002BA538, 0x20000u, 362, (unsigned int) D_002BA5B8, 0x30000u, 66,
    (unsigned int) D_002BAB60, 0x40000u, 9, (unsigned int) D_002BAC68, 0x70000u, 325, (unsigned int) D_002BAC90, 0x80000u,
    60, (unsigned int) D_002BB1A8, 0xFFFFFFFFu, 0, 0, 0
};

int xglCdXapGroups[138] = {
    (int) D_0069CED8, 69656, 73774, 73779, -1, (int) D_0069CED0, 73776, -1,
    (int) D_0069CEC8, 69692, 73767, 73770, 73749, 73750, -1, (int) D_0069CEC0,
    69662, 73775, -1, (int) D_0069CEB8, 73751, -1, (int) D_0069CEB0, 69641,
    69642, 69643, 69667, 69668, 69669, 73736, 73755, 73756,
    73812, 73741, 73742, 73743, 73765, 73766, 73778, 73785,
    73786, 73787, 73788, 73789, 73790, 73784, 73799, 73800,
    -1, (int) D_0069CEA8, 69644, 73737, 73757, 73758, 73813, -1,
    (int) D_0069CEA0, 73777, 77855, -1, (int) D_0069CE98, 73771, 77856, -1,
    (int) D_0069CE90, 69657, -1, (int) D_0069BA38, 69654, 69677, 69680, 73754,
    -1, (int) D_0069CE88, 69659, 69660, 73797, 73798, -1, (int) D_0069CE80,
    69661, -1, (int) D_0069CE78, 73744, 73745, 73746, -1, (int) D_0069CE70,
    73781, -1, (int) D_0069CE68, 73729, 73730, 73733, 73810, 73752,
    73759, 73760, 73814, 73763, 73782, 73804, 73808, 73815,
    -1, (int) D_0069CE60, 73731, 73732, 73734, 73811, 73753, 73761,
    73762, 73764, 73783, 73805, 73809, 77885, -1, (int) D_0069CE58,
    77882, 77894, 77895, -1, (int) D_0069CE50, 77886, -1, (int) D_0069CE48,
    77887, -1, (int) D_0069CE40, 77888, 77889, -1, (int) D_0069CE38, 77893,
    -1, -1
};

