#include "common.h"
#include "main/nml_model_set.h"
#include "main/tu050.h"
#include "main/get.h"
#include "shared.h"
#include "main/sdv_033.h"

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
unsigned short *mapFindBlockEntry(int id, int index);
void xglCullingMapSet(MainObject *object);
int xglJpegDecode(void *info);
void sefReleaseOwnedRecords(int owner);

/* A 0x12000-byte block copy of the map image. */
typedef struct MapImageBlock {
    unsigned char bytes[0x12000];
} MapImageBlock;

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
    uwamonoEntryCountPtr = mapFindBlockEntry(0x300, -1);
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

extern int D_002B5D98[];

void mapObjectUpdateEffectTransform(MapObjectRecord *record)
{
    SefEffectRecord *effect;

    effect = record->effect;
    if (effect == 0) { return; }
    if (record->key != -1) {
        effect->shown = (record->flags >> 1) & 1;
        if (record->flags & MAP_OBJ_EFFECT_FOLLOW) { effect->flags |= SEF_FOLLOW; } else { effect->flags &= ~SEF_FOLLOW; }
    }
    if ((D_002B5D98[0] & 1) == 0) {
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
