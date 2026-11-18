#ifndef INCLUDE_MAIN_SEF_H
#define INCLUDE_MAIN_SEF_H

#include "shared.h"

/* Functions of src/main/sef.c that other files call. */

#define SEF_TABLE_RECORD_SIZE 0xA70

/* A track key whose time is 0x400 ends the track: no key follows it. */
#define SEF_TRACK_END_KEY 0x400
#define SEF_LIST_END 1024
/* Effect record flags (record +0x44): running is set while the record runs its script; slots loaded is cleared when
 * sefLoadEffectSlots fails; pending is cleared by the frame-3 update; the skip bits exclude a record from registering and
 * from stepping. */
#define SEF_RECORD_RUNNING 0x2
#define SEF_RECORD_SKIP_REGISTER 0x400
#define SEF_RECORD_SKIP_STEP 0x400000
#define SEF_RECORD_SLOTS_LOADED 0x2000000
#define SEF_RECORD_PENDING 0x10000000
/* Record flag that sefCountOwnerValue does not count and sefClearOwnerValueFlags keeps. */
#define SEF_RECORD_EXCLUDE_FLAG 0x8000000
/* Pending bit of gameWork.control that holds the effect draw passes back (the X1 SEF_DRAW_FLAG_PENDING value). */
#define SEF_DRAW_FLAG_PENDING 0x04000000
/* Canonical spawn id of the first id range (see sefCanonicalEffectId). */
#define SEF_SPAWN_ID_BASE 0x100C8384

/* Modes of D_0069DC20 read by sefStartEffect and sefCreateDetachedEffect: 0x3E takes the group path, 0x3D sets record flag 0x800000. */
#define SEF_START_MODE_FLAG 0x3D
#define SEF_START_MODE_GROUP 0x3E

/* Scale volume (cells of the scale box) from which a spawn is large: smaller ones get a count of their own. */
#define SEF_SPAWN_LARGE_VOLUME 9
/* Effect slot table: SEF_SPAWN_SLOT_COUNT slots of SEF_SPAWN_SLOT_SIZE bytes each. */
#define SEF_SPAWN_SLOT_COUNT 32
#define SEF_SPAWN_SLOT_SIZE 0x40
/* Spawn ids: the canonical base, and the special id that spawns through the record path (func_00266D40 lookup). */
#define SEF_SPAWN_SPECIAL_ID 0x10000035
/* Entries of the handle table sefHandleSlots. */
#define SEF_HANDLE_SLOT_COUNT 48

/* Shape kinds of sefGetShapePos (the emission shape of a particle). */
enum SefShapeKind {
    SEF_SHAPE_SPHERE_RANDOM = 0,
    SEF_SHAPE_SPHERE = 1,
    SEF_SHAPE_CUBE_CENTER = 2,
    SEF_SHAPE_CUBE_TOP = 3,
    SEF_SHAPE_CUBE_BOTTOM = 4,
    SEF_SHAPE_CIRCLE_TURNED = 5,
    SEF_SHAPE_CIRCLE_RANDOM = 6
};

/* Modes of sefApplyRecordMode. */
enum SefRecordMode {
    SEF_RECORD_MODE_RELEASE = 1,
    SEF_RECORD_MODE_SET_FLAG_80,
    SEF_RECORD_MODE_CLEAR_FLAG_80,
    SEF_RECORD_MODE_GATE_ON,
    SEF_RECORD_MODE_GATE_OFF,
    SEF_RECORD_MODE_FLAGS_880
};

/* One of the record's five 8-byte script handles (+0xA04 sound, +0xA0C sound state, +0xA14 alter, +0xA44 special,
 * +0xA54 wind): the script address and its kind byte. */
typedef struct SefRecordHandle {
    int address;
    unsigned char unmodeled_04[3];
    signed char kind; /* +0x07 */
} SefRecordHandle;

typedef struct SefEffectEntry {
    signed char lineIndex[32];  /* +0x00: 32 line indices, negative means none (sv_get_image_item.c's view) */
    signed char sortKey[32];    /* +0x20: the second byte of each line */
    int state;
    int flags;
    short key;
    unsigned char unmodeled_04a[1];
    signed char signal;
} SefEffectEntry;

typedef struct SefEffectRecord {
    Vector4 position;           /* +0x00: the point sefTransformClipPoint takes (sv_get_image_item.c's SvEffectRecord) */
    unsigned char firstBlock[0x10];  /* +0x10: copied from sefSpawnEffectRecord's third argument */
    unsigned char secondBlock[0x10]; /* +0x20: copied from sefSpawnEffectRecord's fourth argument */
    unsigned char block[0x10];       /* +0x30: copied by sefSetRecordBlock */
    int cellSlot;               /* +0x40 */
    int flags;                  /* +0x44: sefSetRecordOwnerMarked sets 0x400 */
    int inUse;                  /* +0x48 */
    int loadTag;                /* +0x4c: copied to sefCurrentLoadTag by sefDestroyRecordEffects */
    int owner;                  /* +0x50 */
    unsigned char unmodeled_054[0x58 - 0x54];
    struct SefGroupObject *object; /* +0x58: the object the record belongs to */
    int values[3];              /* +0x5c: set by sefSetRecordValues */
    int requestTail;            /* +0x68: copied to the request by sefSpawnRecordChildren */
    short entryId;              /* +0x6c: the key-table entry index + 9 of the record */
    short gate;                 /* +0x6e */
    float coord_a;              /* +0x70 */
    int ownerKey;               /* +0x74 */
    float coord_b;              /* +0x78 */
    unsigned char spawnEnabled; /* +0x7c: sefSpawnRecordChildren spawns nothing when it is zero */
    unsigned char unmodeled_07d;
    unsigned short spawnBits;   /* +0x7e: bits 0..7 pick kinds 0x136008..0x136001, bits 12..15 pick kind 0x136009 + n */
    unsigned char unmodeled_080[0x82 - 0x80];
    unsigned short loadFlags;   /* +0x82: the third argument of sefLoadEffectSlots */
    SefEffectEntry entries[32]; /* +0x84 */
    SefRecordHandle sound;      /* +0xa04 */
    SefRecordHandle soundState; /* +0xa0c */
    SefRecordHandle alter;      /* +0xa14 */
    int cameraChoice;           /* +0xa1c: first word of the camera block (0x28 bytes) that sdvSelectCamera fills */
    unsigned char unmodeled_a20[0xa44 - 0xa20];
    SefRecordHandle special;    /* +0xa44 */
    unsigned char unmodeled_a4c[0xa54 - 0xa4c];
    SefRecordHandle wind;       /* +0xa54 */
    float fieldA5C;             /* +0xa5c: zeroed by sefCreateEffectRecord, copied to the request +0x314 */
    float fieldA60;             /* +0xa60: set to 8.0 by sefCreateEffectRecord */
    unsigned short value;       /* +0xa64 */
    short remaining;            /* +0xa66 */
    short slotIndex;            /* +0xa68: the second argument of sefLoadEffectSlots; negative skips the load */
    signed char linkValue;      /* +0xa6a: the link byte of the entry sefLoadEffectSlots linked to (-1 when none) */
    unsigned char effectType;   /* +0xa6b: the effect type srsAnalyzeEftNo returns for the record's effect number */
    unsigned char requestMode;  /* +0xa6c: copied from the request's mode byte */
    unsigned char unmodeled_a6d[SEF_TABLE_RECORD_SIZE - 0xa6d];
} SefEffectRecord;

SefEffectRecord *sefSpawnEffectRecordById(int effectId, int second, unsigned char *firstBlock);
SefEffectRecord *sefSpawnEffectRecordByCode(signed char *text, int first, unsigned char *firstBlock, unsigned char *secondBlock);
void sefSetSlotDirectionDefault(Matrix4 *destination, const Vector4 *direction);
void sefSetSlotDirectionBetweenDefault(void *state, Vector4 *minuend, Vector4 *subtrahend);
void sefGetCubePosBtm(Vector4 *pos, const Vector4 *extent);
void sefGetCubePosTop(Vector4 *pos, const Vector4 *extent);

SefEffectRecord *sefGetEffectRecords(void);
void sefReleaseEffectRecord(int index);

void sefClearLineDataRecords(void);
void sefClearHandleTable(void);
float srsAtan2(float x, float y);

void sefStartClipTestDisabled(void);
void sefExecEffectFrames(int id, float frames);
void sefStartWithDefaultMemory(void);

void sefStartEffectDefault(void *object);
void sefStartEffectWithValue(void *object, int value);

int sefStartEffectKeyDefault(int first, int second);

/* Spawns the effect record for an effect id. Inside an entry (D_0069DC2C set) it hands the remapped key to
 * func_0026C9C8; otherwise it creates a record, copies the two optional quadword blocks into it and marks it. */
SefEffectRecord *sefSpawnEffectRecord(int effectId, int second, unsigned char *third, unsigned char *fourth, int fifth);

int sefCanonicalEffectId(int effectId);

/* The request block sefGetRequestBuffer returns (sefRequestBuffer): the kind keys, the kind matrices and the kind tables. */
typedef struct SefLinkedContext {
    int kind1Key;                    /* +0x000, the second argument of sefGetKindMatrix for kind 1 */
    int kind5Key;                    /* +0x004 */
    int kind0Key;                    /* +0x008 */
    int kind2Key;                    /* +0x00C */
    int kind3Key;                    /* +0x010 */
    int kind4Key;                    /* +0x014 */
    int kind6Key;                    /* +0x018 */
    unsigned char unmodeled_01C[0x64];
    Matrix4 kind64Matrix;            /* +0x080, copied for kind 64 */
    Matrix4 kind256Matrix;           /* +0x0C0, copied for kind 256 */
    Matrix4 kind128Matrix;           /* +0x100, copied for kinds 128 and 162 */
    Matrix4 kind1024Matrix;          /* +0x140, sefApplyPartTransform for bit 0x400 */
    Matrix4 kind2048Matrix;          /* +0x180, sefApplyPartTransform for bit 0x800 */
    Matrix4 kind4096Matrix;          /* +0x1C0, sefApplyPartTransform for bit 0x1000 */
    Matrix4 kind512Matrix;           /* +0x200, sefApplyPartTransform for bit 0x200 */
    Matrix4 kind128AltMatrix;        /* +0x240, bit 0x80 when sefUseAltMatrix is set */
    unsigned char unmodeled_280[0x94];
    float kind1VectorY;              /* +0x314: overrides y of the kind-1 slot vector when nonzero */
    union {
        int objectOffset;            /* +0x318: read as an offset by sefGetLinkedFlag */
        struct SefKindTable *kind1Table; /* +0x318: the kind-1 table of sefGetKindSlotVector */
    };
    struct SefKindTable *kind5Table;
    struct SefKindTable *kind0Table;
    struct SefKindTable *kind2Table;
    struct SefKindTable *kind3Table;
    struct SefKindTable *kind4Table;
    struct SefKindTable *kind6Table;
} SefLinkedContext;

SefLinkedContext *sefGetRequestBuffer(void);

void sefMemClear(void *dest, unsigned int size);
void sefMemZero(void *dest, int size);
void sefMemFillWords(void *dest, int value, int size);

void sefNoiseInit(void);

void sefResetSlotTable(void);

int loadWaitPending(void);
void sefReleaseWaitSlot(int key);
void sefSetGateForId(int owner, int id);
void xglCdWaitUntilIdle(void);

int sefFlagTest(int mask);

/* Counts the entries of record that are active and have flag 0x8000. */
int sefCountMarkedEntries(SefEffectRecord *record);

int sefCountOwnerValue(int owner, unsigned short value);
void sefClearOwnerValueFlags(int owner, unsigned short value, int mask);
void sefReleaseOwnedRecords(int owner);
int sefOwnerPairInUse(int owner, int ownerKey);
int sefGetEntryFlags(int key);
int sefEntryHasFlag1(int key);
int sefEntryHasFlag2(int key);
void sefPruneHandleSlots(void);
void sefRegisterHandle(int handle, int value);

/* Four counters per effect entry, 0x14 bytes each (sefEntryCounts, cleared by the sef init chain). */
typedef struct SefEntryCounts {
    int upper;                                       /* +0x00 */
    int lower;                                       /* +0x04 */
    int plain;                                       /* +0x08 */
    int unmodeled_0c[2];
} SefEntryCounts;

extern SefEntryCounts sefEntryCounts[];
/* Address of the counter entry entry_index (tu163.c). */
void *func_00259638(int entry_index);
void sefExecEffect(void);

#endif /* INCLUDE_MAIN_SEF_H */
