#include "common.h"
#include "main/sdv_033.h"
#include "main/pad.h"
#include "main/tu021.h"
#include "main/tu051.h"
#include "main/tu086.h"
#include "main/xenoActor.h"
#include "main/xgl_font.h"
#include "main/xgl_2.h"

/* Studio camera state of the game camera (xglStudioGetCamera2(0)->state). */
#define CAMERA_STATE_GAME 4
/* Frames the camera update waits at start (scratchpad phase). */
#define CAMERA_INITIAL_WAIT 40
/* Bits of the camera flags word at gameWork+0xD0 (the same bits as Xenosaga I's GAME_CAMERA_FLAG_*). */
#define CAMERA_FLAG_DISABLED 0x8            /* the camera is not updated */
#define CAMERA_FLAG_TRACK_PLAYER 0x20       /* refresh the old player position and the track index */
#define CAMERA_FLAG_MODE_CHANGED 0x40       /* a new camera block was selected */
#define CAMERA_FLAG_SNAP 0x80               /* the slots are cleared and the camera jumps */
#define CAMERA_FLAG_VIEW_ONLY 0x200         /* only cameraApplyView runs */
/* Angles, in radians. */
#define CAMERA_PI 3.1415927f
#define CAMERA_TWO_PI 6.2831855f
#define CAMERA_HALF_PI 1.5707964f
/* Defaults of a camera record (cameraRangeSetDefaults). */
#define CAMERA_DEFAULT_PITCH (-0.39269909f)  /* -22.5 degrees */
#define CAMERA_DEFAULT_PERSPECTIVE 0.69813174f /* 40 degrees */
#define CAMERA_DEFAULT_DISTANCE 5.0f
#define CAMERA_CENTER_HEIGHT 1.2f           /* the look-at offset above the anchor */
/* Camera types of a camera record, selected in cameraUpdateFrame. */
#define CAMERA_TYPE_FOLLOW 0                /* cameraUpdateFollow */
#define CAMERA_TYPE_RECORD 1                /* cameraUpdateFromRecord */
/* Probe modes of a camera record (CameraRecord.range.probeMode): zero pulls the camera in along the line of
   sight, anything else searches the orbit pitch. */
#define CAMERA_PROBE_MODE_SLIDE 33          /* the first probe slides (cameraProbeSlide) */
#define CAMERA_PROBE_MODE_RESLIDE 17        /* the found height is probed again with a slide */
/* The orbit pitch search of cameraSearchOrbitPitch. */
#define CAMERA_SEARCH_STEP_FINE 0.052359883f /* 3 degrees */
#define CAMERA_SEARCH_STEP_COARSE 0.785398185f /* 45 degrees */
#define CAMERA_SEARCH_PITCH_DOWN (-1.57079637f)
#define CAMERA_SEARCH_TOLERANCE 0.01f
#define CAMERA_SEARCH_MAX_STEPS 60
/* The map object resource table loaded by mapObjectResourcesLoad. */
#define MAP_RESOURCE_COUNT 54
/* uwamono records: 256 of them; kind 4 is the player, kind 5 a part attached to it. */
#define UWAMONO_RECORD_COUNT 256
#define UWAMONO_KIND_TALK 1
#define UWAMONO_KIND_PLAYER 4
#define UWAMONO_KIND_PART 5
/* Draw flags of a XenoActor (drawFlags). */
#define UWAMONO_DRAW_HIDDEN 0x4
/* uwamonoChangePlayerModel: the request that takes the key mapped from the current record, and the key flags. */
#define UWAMONO_KEY_FROM_CURRENT (-1000)
#define UWAMONO_KEY_MASK 0x7FFFFFFF
#define UWAMONO_ES_KEY_FIRST 0x10000
#define UWAMONO_ES_KEY_COUNT 0x4000
/* The form value of an ES (the mecha the party rides) in record+0x338. */
#define UWAMONO_FORM_ES 1.0f
/* Joints the player parts attach to: the ES pair, then the walking pair. */
#define UWAMONO_PART_JOINT_ES_0 80
#define UWAMONO_PART_JOINT_ES_1 81
#define UWAMONO_PART_JOINT_WALK_0 10
#define UWAMONO_PART_JOINT_WALK_1 7
/* UwamonoAttachIdTable (uwamonoAttachIdTable): eight entries of {lo, hi, id0, id1}. */
#define UWAMONO_ATTACH_ENTRY_WORDS 4
#define UWAMONO_ATTACH_TABLE_WORDS 32
#define UWAMONO_ATTACH_PARTS 2

/* The camera range block at gameWork+0x180: an anchor index, the range mode and four staged vectors. */
struct CameraRange {
    int anchorIndex;                    /* +0x00: -1 when no anchor */
    short anchorSubIndex;               /* +0x04: -1 when no anchor */
    unsigned char unmodeled_006[0x2];
    unsigned char lockX;  /* +0x008 */
    unsigned char lockY;  /* +0x009 */
    unsigned char lockZ;  /* +0x00A */
    unsigned char probeMode;  /* +0x00B */
    signed char rangeMode;              /* +0x0C: scaffold cameraRangeMode */
    unsigned char unmodeled_0D[2];
    signed char rangeFlag;              /* +0x0F */
    unsigned char unmodeled_10[0x10];
    Vector4 offset;                   /* +0x20 */
    Vector4 angleLimits;                   /* +0x30 */
    Vector4 lockPosition;                   /* +0x40: w is the scaffold cameraPersAngle at gameWork+0x1CC */
    Vector4 lookAt;  /* +0x050 */
    Vector4 angles;                   /* +0x60 */
};

/* One of the four camera slots (0 look-at, 1 position and angles, 2 perspective, 3 roll): a timed transition set by
   cameraSlotSetup. */
struct CameraSlot {
    unsigned char unmodeled_00[2];
    unsigned char blendMode;            /* +0x02: stored by cameraSlotSetup when >= 0; slot 1 blends the position when 0, the angles otherwise */
    unsigned char easeMode;             /* +0x03: stored by cameraSlotSetup when >= 0; the mode of cameraEase */
    float progress;  /* +0x004 */
    float duration;                      /* +0x08: the frames of the transition as a float; a slot is active when nonzero */
    float fraction;                     /* +0x0C: the eased fraction of the transition, 1.0 when the frames were 0 */
};

/* One camera record of cameraCfSlots (208 bytes): its flag is cleared and its index set to -1 by cameraReset. */
struct CameraRecord {
    unsigned char unmodeled_00;
    unsigned char flag01;               /* +0x01: cleared by cameraReset */
    signed char index02;                /* +0x02: -1 after cameraReset */
    unsigned char unmodeled_03[0xD];
    struct CameraSlot slot;             /* +0x10: the slot settings of this record */
    struct CameraRange range;           /* +0x20: the range cleared by cameraRangeSetDefaults */
    unsigned char unmodeled_90[0x40];
};

/* File names of the map object resources (matumoto\\*.pxy models, *.txy textures, spline.dat, enemyorder.dat,
   treasurebox.dat, kowaremono.dat): the first argument of each GameResLoadName call in mapObjectResourcesLoad. */
int GameResLoadName(const char *name, void **data, unsigned int *size, void (*callback)(int index, void *buffer, unsigned int size, int value));
int xglCdServiceIfPending(void);
extern const char *mapObjectResourceNames[MAP_RESOURCE_COUNT];
/* Resource slots: each GameResLoadName call stores the loaded block through &mapObjectResources[i]. */
extern void *mapObjectResources[];
extern int D_0069D1DC;
extern int uwamonoEventTable;

/* Loads every map object resource named in mapObjectResourceNames (models, textures and data files), waiting for each CD read, and keeps
   entries 1 and 2 in uwamonoEventTable and D_0069D1DC. */
void mapObjectResourcesLoad(void)
{
    int i;

    for (i = 0; i < MAP_RESOURCE_COUNT; i++) {
        GameResLoadName(mapObjectResourceNames[i], &mapObjectResources[i], 0, 0);
        while (xglCdServiceIfPending() != 0) {
        }
    }
    uwamonoEventTable = (int)mapObjectResources[1];
    D_0069D1DC = (int)mapObjectResources[2];
}

#include "shared.h"

/* 32 camera records (0x1A00 bytes), copied whole. */
typedef struct CameraSlotBlock {
    union {
        DoublewordAlignedVector4 quadwords[0x1A00 / 16];
        struct CameraRecord record[0x1A00 / 0xD0];
    };
} CameraSlotBlock;

/* 0x280 bytes of the camera slot table, copied with a runtime alignment test. */
typedef struct CameraSlotTable {
    int unmodeled_000[0x280 / 4];
} CameraSlotTable;

extern CameraSlotBlock cameraCfSlots;
/* Three words each, cleared by cameraReset; their meaning is not established yet. */
typedef union CameraWords3 {
    int word[3];
    Vector4 v;
    int w[4];
} CameraWords3;

/* The probe block mapMoveProbe fills for the ground under the player. */
typedef struct UwamonoProbe {
    int clearedA;                           /* +0x00 */
    int clearedB;                           /* +0x04 */
    unsigned short mask08;                           /* +0x08 */
    unsigned char unmodeled_0A[0x6];
    float range;                            /* +0x10 */
    unsigned char unmodeled_14[0x4];
    int ground;                             /* +0x18 */
    unsigned int scratchWork;               /* +0x1C */
} UwamonoProbe;

static int cameraPlayerProbe[6];
static int D_0069E568[10];
static DoublewordAlignedVector4 cameraPlayerPositionOld;
static UwamonoProbe D_0069E5A0[2];
static CameraWords3 D_0069E5E0;
static UwamonoProbe cameraPositionProbe[2];
static CameraWords3 cameraProbePosition;
static CameraSlotBlock D_0069E640;
extern CameraSlotTable cameraSlotTable;
static CameraSlotTable D_006A0040;
extern unsigned char D_002B5E6C[];
static unsigned char D_0069E180;

/* Saves the camera slot records, the slot table and the track index to their save blocks. */
void cameraSaveState(void)
{
    D_0069E640 = cameraCfSlots;
    D_006A0040 = cameraSlotTable;
    D_0069E180 = D_002B5E6C[0];
}

/* Triangle wave record: amplitude is the current peak, step the amount it decays per cycle; period and phase
   are in frames, hold delays the decay, remaining counts the cycles left. */
struct CameraWave {
    float amplitude;                    /* +0x00 */
    float step;                         /* +0x04 */
    unsigned short period;              /* +0x08 */
    unsigned short phase;               /* +0x0A */
    unsigned short hold;                /* +0x0C */
    unsigned short remaining;           /* +0x0E */
};

/* A plain view of the flags word: cameraSetMode accesses it without the union alias set. */
typedef struct {
    unsigned char unmodeled_000[0xD0];
    unsigned short flags;
} CameraFlagsView;

/* A 128-bit GPR value: stored with sq. */
typedef unsigned int XglQuadword __attribute__((mode(TI)));

/* The camera state block at gameWork. */
struct CameraState {
    unsigned char unmodeled_000[8];
    /* A one-member union: the original reads this field with alias set 0, after the stores before it. */
    union {
        int control;                    /* +0x08: player control flags (0x1 paused, 0x8000 scripted, 0x10000 replayed input) */
    };
    unsigned char unmodeled_00C[0xC4];
    union { unsigned short flags; };               /* +0xD0: 0x10 set by cameraApplySlots, 0x20 by cameraSetIndex and cameraApplyAllSlots, 0x40 by cameraSetMode */
    unsigned char cameraType;           /* +0xD2: the camera type of the active record */
    unsigned char unmodeled_0D3;
    signed char recordIndex;            /* +0xD4: the camera record in use */
    signed char mode;                   /* +0xD5: set by cameraSetMode */
    unsigned char allSlotsFlagA;  /* +0x0D6 */
    unsigned char allSlotsFlagB;  /* +0x0D7: also the anchor pending flag of the range update */
    unsigned char previousRecordIndex;  /* +0xD8 */
    unsigned char anchorLocked;  /* +0x0D9 */
    unsigned char previousCameraType;   /* +0xDA */
    unsigned char unmodeled_0DB;
    signed char trackIndex;             /* +0xDC: set by cameraSetIndex */
    unsigned char unmodeled_0DD[0x3];
    XglQuadword wordE0;                 /* +0xE0: cleared when the record kind changes */
    unsigned char shakeFlags;  /* +0x0F0 */
    unsigned char unmodeled_0F1[0xF];
    struct CameraWave waves[3];  /* +0x100 */
    float sway[4];  /* +0x130 */
    struct CameraSlot slot[4];
    int anchorIndex;  /* +0x180 */
    short anchorSubIndex;  /* +0x184 */
    unsigned char unmodeled_186[0x6];
    unsigned char anchorReady;  /* +0x18C */
    unsigned char unmodeled_18D[0x2];
    unsigned char rangeFlag;  /* +0x18F */
    float stickAngle;                   /* +0x190: the camera yaw the stick directions are relative to */
    unsigned char unmodeled_194[0xC];
    /* The current camera: offset (the anchor offset), position, look-at point, perspective, roll and the pitch, yaw
       and distance between position and look-at. The from* fields are the values a slot transition starts from
       (cameraApplySlots copies the current ones there) and the to* fields the values it ends at (cameraCommitSlots). */
    float offsetX;
    float offsetY;
    float offsetZ;
    unsigned char unmodeled_1AC[0x14];
    float positionX;
    float positionY;
    float positionZ;
    float perspective;
    float lookAtX;
    float lookAtY;
    float lookAtZ;
    float roll;
    float pitch;
    float yaw;
    unsigned char unmodeled_1E8[0x4];
    float distance;
    float fromPositionX;
    float fromPositionY;
    float fromPositionZ;
    float fromPerspective;
    float fromLookAtX;
    float fromLookAtY;
    float fromLookAtZ;
    float fromRoll;
    float fromPitch;
    float fromYaw;
    unsigned char unmodeled_218[0x4];
    float fromDistance;
    float toPositionX;  /* +0x220 */
    float toPositionY;  /* +0x224 */
    float toPositionZ;  /* +0x228 */
    float toPerspective;  /* +0x22C */
    float toLookAtX;  /* +0x230 */
    float toLookAtY;  /* +0x234 */
    float toLookAtZ;  /* +0x238 */
    float toRoll;  /* +0x23C */
    float toPitch;  /* +0x240 */
    float toYaw;  /* +0x244 */
    unsigned char unmodeled_248[0x4];
    float toDistance;  /* +0x24C */
    float fromOffsetX;
    float fromOffsetY;
    float fromOffsetZ;
    unsigned char unmodeled_25C[0x4];
    Vector4 anchorOffset;               /* +0x260: the anchor offset vector (D_002B5FF0 in the other units) */
    float anchorBase[3];  /* +0x270 */
    unsigned char unmodeled_27C[0x54];
    float frameScale;                   /* +0x2D0: elapsed frames, scales the per-frame movement */
    unsigned char unmodeled_2D4[4];
    float field_2d8;
    unsigned char unmodeled_2DC[4];
    float field_2e0;
    unsigned char unmodeled_2E4[4];
    float field_2e8;
    unsigned char unmodeled_2EC[4];
    float field_2f0;
    unsigned char unmodeled_2F4[4];
    float field_2f8;
    unsigned char unmodeled_2FC[4];
    int field_300;
    unsigned char unmodeled_304[0xC];
    int replayStatus;                   /* +0x310: ground status recorded with the input */
    int replayStickX;                   /* +0x314 */
    int replayStickY;                   /* +0x318 */
    unsigned char unmodeled_31C[0x14];
    short fadeTimer;                    /* +0x330: frames left of the rider fade */
    unsigned char unmodeled_332[0x16];
    DoublewordAlignedVector4 extent;    /* +0x348 (MainWork in tu050) */
};

extern struct CameraState gameWork;
void cameraSetIndex(int index);
void cameraUpdateFrame(void);

/* Restores the camera slot records, the slot table and the track index from their save blocks. */
void cameraRestoreState(void)
{
    int keptBit;

    cameraCfSlots = D_0069E640;
    cameraSlotTable = D_006A0040;
    cameraSetIndex(D_0069E180);
    keptBit = gameWork.control & 0x400;
    gameWork.control &= ~0x400;
    cameraUpdateFrame();
    gameWork.control |= keptBit;
}

/* Gives a camera range its defaults: no anchor, the look-at height offset, the full angle limits, the default pitch and
   distance, and the default perspective. */
void cameraRangeSetDefaults(struct CameraRange *range)
{
    range->anchorIndex = -1;
    range->anchorSubIndex = -1;
    range->rangeMode = 1;
    range->rangeFlag = 1;
    range->offset.x = 0.0f;
    range->offset.y = CAMERA_CENTER_HEIGHT;
    range->offset.z = 0.0f;
    range->angleLimits.x = -CAMERA_HALF_PI;
    range->angleLimits.y = CAMERA_HALF_PI;
    range->angleLimits.z = -CAMERA_TWO_PI;
    range->angleLimits.w = CAMERA_TWO_PI;
    range->angles.x = CAMERA_DEFAULT_PITCH;
    range->angles.y = 0.0f;
    range->angles.w = CAMERA_DEFAULT_DISTANCE;
    range->lockPosition.w = CAMERA_DEFAULT_PERSPECTIVE;
}

XglCamera *xglStudioGetCamera2(int index);

/* Puts the studio camera in the game state, parked far away at the default pitch and facing pi. */
void cameraStudioReset(void)
{
    XglCamera *camera;
    float parkX;
    float parkY;
    float parkZ;

    camera = xglStudioGetCamera2(0);
    camera->state = CAMERA_STATE_GAME;
    parkX = 1000.0f;
    camera->travel.position.x = parkX;
    parkY = 1000.0f;
    camera->travel.position.y = parkY;
    parkZ = 1000.0f;
    camera->travel.position.z = parkZ;
    camera->travel.rotation.x = CAMERA_DEFAULT_PITCH;
    camera->travel.rotation.y = CAMERA_PI;
    camera->travel.rotation.z = 0.0f;
}

void *memset(void *dest, int value, unsigned int count);

void cameraInit(void)
{
    float perspective;
    float startY;
    float startZ;

    cameraStudioReset();
    memset(&gameWork.flags, 0, 432);
    gameWork.flags = 0x80;
    cameraRangeSetDefaults((struct CameraRange *) &gameWork.anchorIndex);
    perspective = CAMERA_DEFAULT_PERSPECTIVE;
    startY = 3.0f;
    startZ = 10.0f;
    gameWork.toPerspective = perspective;
    do {
    } while (0);
    gameWork.positionY = startY;
    gameWork.positionZ = startZ;
    gameWork.lookAtZ = 0.0f;
    do {
    } while (0);
    gameWork.fromPerspective = perspective;
    do {
    } while (0);
    gameWork.positionX = 0.0f;
    do {
    } while (0);
    gameWork.lookAtX = 0.0f;
    gameWork.lookAtY = 0.0f;
    do {
    } while (0);
}

/* One 20-byte entry of cameraSlotTable: two index bytes set to -1 and a word cleared by cameraReset. */
struct CameraSlotEntry {
    signed char indexA;                 /* +0x00 */
    signed char indexB;                 /* +0x01 */
    unsigned char unmodeled_02[0xA];
    int word0C;                         /* +0x0C */
    unsigned char unmodeled_10[0x4];
};


void cameraInit(void);
void cameraSetMode(int mode, int arg1, int arg2);

void cameraReset(void)
{
    struct CameraRecord *record;
    int i;

    cameraInit();
    record = cameraCfSlots.record;
    for (i = 31; i >= 0; i--) {
        memset(record, 0, 208);
        record->flag01 = 0;
        record->index02 = -1;
        cameraRangeSetDefaults(&record->range);
        record++;
    }
    {
        struct CameraSlotEntry *entry = (struct CameraSlotEntry *)&cameraSlotTable;
        signed char none = -1;
        int j;

        for (j = 31; j >= 0; j--) {
            entry->indexA = none;
            entry->indexB = none;
            entry->word0C = 0;
            entry++;
        }
    }
    D_0069E5E0.word[0] = 0;
    D_0069E5E0.word[1] = 0;
    D_0069E5E0.word[2] = 0;
    cameraProbePosition.word[0] = 0;
    cameraProbePosition.word[1] = 0;
    cameraProbePosition.word[2] = 0;
    gameWork.mode = 0;
    gameWork.trackIndex = -1;
    cameraSetMode(-1, 0, 0);
}


int mapFindBlockEntry(int, int);
void mapProbeInit(UwamonoProbe *probe);

void cameraSetMode(int mode, int arg1, int arg2)
{
    int probeBlock;

    if (gameWork.mode == mode && D_0069E5A0[0].ground != 0 && cameraPositionProbe[0].ground != 0) {
        return;
    }
    probeBlock = 0;
    if (mode >= 0) {
        probeBlock = mapFindBlockEntry(512, mode);
        if (probeBlock == 0) {
            probeBlock = mapFindBlockEntry(512, -1);
            mode = -1;
        }
    } else {
        mode = -1;
    }
    gameWork.mode = mode;
    mapProbeInit(&D_0069E5A0[0]);
    D_0069E5A0[0].scratchWork = 0x70000170;
    D_0069E5A0[0].ground = probeBlock;
    D_0069E5A0[0].clearedB = 0;
    D_0069E5A0[0].clearedA = 0;
    D_0069E5A0[0].range = 1000.0f;
    D_0069E5A0[0].mask08 = 0x802;
    mapProbeInit(&cameraPositionProbe[0]);
    cameraPositionProbe[0].scratchWork = 0x70000170;
    ((CameraFlagsView *)&gameWork)->flags |= CAMERA_FLAG_MODE_CHANGED;
    cameraPositionProbe[0].mask08 = 0x802;
    cameraPositionProbe[0].ground = probeBlock;
    cameraPositionProbe[0].range = 1000.0f;
    cameraPositionProbe[0].clearedA = 0;
    cameraPositionProbe[0].clearedB = 0;
}


void cameraSetIndex(int index)
{
    int block;

    if (index < 0) {
        index = gameWork.trackIndex;
    }
    for (;;) {
        gameWork.trackIndex = index;
        block = mapFindBlockEntry(1536, index);
        cameraPlayerProbe[6] = block;
        if (index < 0) {
            break;
        }
        if (block != 0) {
            break;
        }
        index = -1;
    }
    gameWork.flags |= CAMERA_FLAG_TRACK_PLAYER;
}

float I2F(int value);

/* Stores the blend and ease modes when they are >= 0, zeroes the slot's progress, then the frames as a float and the fraction (0, or 1.0 when the frames are 0). */
int cameraSlotSetup(struct CameraSlot *slot, int blend_mode, int ease_mode, int frames)
{
    if (blend_mode >= 0) {
        slot->blendMode = blend_mode;
    }
    if (ease_mode >= 0) {
        slot->easeMode = ease_mode;
    }
    slot->progress = 0.0f;
    slot->duration = I2F(frames);
    if (frames == 0) {
        slot->fraction = 1.0f;
    } else {
        slot->fraction = 0.0f;
    }
}

/* slot[0..3] of gameWork, reached by their own symbols (the scaffold names them): the original addresses
   the camera state as slot symbol minus its offset, and the relocations only match through these names. */
extern unsigned char D_002B5ED0[];
extern struct CameraSlot D_002B5EE0[];
extern struct CameraSlot D_002B5EF0[];
extern struct CameraSlot D_002B5F00[];
/* Stores the blend and ease modes when they are >= 0, zeroes the slot's progress, then the frames as a float and the fraction (0, or 1.0 when the frames are 0). */ int cameraSlotSetup(struct CameraSlot *slot, int blend_mode, int ease_mode, int frames);

#define CAMERA_SLOT_MASK(slot) (1 << (slot))
#define CAMERA_FLAG_SLOTS_APPLIED 0x10

/* Applies the slots in slot_mask (zero: every slot whose duration flag is set) and copies each slot's staged
   vectors to its applied fields; sets flag 0x10 when frames is nonzero. */
void cameraApplySlots(int slot_mask, int blend_mode, int ease_mode, int frames)
{
    if (slot_mask == 0) {
        if (gameWork.slot[0].duration != 0.0f) {
            slot_mask = CAMERA_SLOT_MASK(0);
        }
        if (gameWork.slot[1].duration != 0.0f) {
            slot_mask |= CAMERA_SLOT_MASK(1);
        }
        if (gameWork.slot[2].duration != 0.0f) {
            slot_mask |= CAMERA_SLOT_MASK(2);
        }
        if (gameWork.slot[3].duration != 0.0f) {
            slot_mask |= CAMERA_SLOT_MASK(3);
        }
        if (slot_mask == 0) {
            return;
        }
    }
    if (slot_mask & CAMERA_SLOT_MASK(0)) {
        struct CameraState *state = (struct CameraState *)((unsigned char *)D_002B5ED0 - 0x140);
        cameraSlotSetup((struct CameraSlot *)D_002B5ED0, blend_mode, ease_mode, frames);
        state->fromLookAtX = state->lookAtX;
        state->fromLookAtY = state->lookAtY;
        state->fromLookAtZ = state->lookAtZ;
    }
    if (slot_mask & CAMERA_SLOT_MASK(1)) {
        struct CameraState *state = (struct CameraState *)((unsigned char *)D_002B5EE0 - 0x150);
        cameraSlotSetup(D_002B5EE0, blend_mode, ease_mode, frames);
        state->fromPositionX = state->positionX;
        state->fromPositionY = state->positionY;
        state->fromPositionZ = state->positionZ;
        state->fromPitch = state->pitch;
        state->fromYaw = state->yaw;
        state->fromDistance = state->distance;
        state->fromOffsetX = state->offsetX;
        state->fromOffsetY = state->offsetY;
        state->fromOffsetZ = state->offsetZ;
    }
    if (slot_mask & CAMERA_SLOT_MASK(2)) {
        struct CameraState *state = (struct CameraState *)((unsigned char *)D_002B5EF0 - 0x160);
        cameraSlotSetup(D_002B5EF0, blend_mode, ease_mode, frames);
        state->fromPerspective = state->perspective;
    }
    if (slot_mask & CAMERA_SLOT_MASK(3)) {
        struct CameraState *state = (struct CameraState *)((unsigned char *)D_002B5F00 - 0x170);
        cameraSlotSetup(D_002B5F00, blend_mode, ease_mode, frames);
        state->fromRoll = state->roll;
    }
    if (frames != 0) {
        gameWork.flags |= CAMERA_FLAG_SLOTS_APPLIED;
    }
}

/* Anchor record returned by cameraAnchorGet: the yaw and pitch used to turn a camera offset. */
struct CameraAnchor {
    unsigned char unmodeled_00[0x10];
    DoublewordAlignedVector4 position; /* +0x10: the anchor point cameraAnchorGet copies out */
    unsigned char unmodeled_20[0x50 - 0x20];
    float rotation_x;
    float rotation_y;
};

struct CameraAnchorSlot {
    union {
        struct CameraAnchor *actor; /* +0x00: the actor body of the slot */
        void *body;                 /* the same word as the other slot views see it */
    } head;
    unsigned char unmodeled_04[0x870 - 0x04];
};

extern struct CameraAnchorSlot uwamonoSlotTable[];
int XenoActorMulVector(struct CameraAnchor *actor, int channel, Vector4 *vector, int flag);
void xglMatrixStackTrans();
/* index below 4096 selects uwamonoSlotTable[index].head.actor; a larger value is the anchor pointer itself. */ struct CameraAnchor *cameraAnchorGet(Vector4 *out, int index, int subIndex);

/* index below 4096 selects uwamonoSlotTable[index].head.actor; a larger value is the anchor pointer itself. */
struct CameraAnchor *cameraAnchorGet(Vector4 *out, int index, int subIndex)
{
    struct CameraAnchor *anchor;

    if (index < 4096) {
        if (index < 0) {
            out->x = 0.0f;
            out->y = 0.0f;
            out->z = 0.0f;
            return 0;
        }
        anchor = uwamonoSlotTable[index].head.actor;
    } else {
        anchor = (struct CameraAnchor *)index;
    }
    out->x = anchor->position.x;
    out->y = anchor->position.y;
    out->z = anchor->position.z;
    if (subIndex >= 0) {
        XenoActorMulVector(anchor, subIndex, out, 1);
    }
    return anchor;
}

void xglMatrixStackRotX(float);
void xglMatrixStackRotY(float);
void xglMatrixStackSave(float *);

/* Moves position by offset. With a valid anchor and a unit offset.w, the offset is turned by the anchor's
   yaw and pitch first; otherwise it is added as it is. Returns the anchor. */
struct CameraAnchor *cameraApplyAnchorOffset(Vector4 *position, int anchorIndex, int anchorSubIndex, Vector4 *offset)
{
    float matrix[16];
    struct CameraAnchor *node;

    node = cameraAnchorGet(position, anchorIndex, anchorSubIndex);
    if (node != 0 && offset != 0 && offset->w == 1.0f) {
        xglMatrixStackUnit();
        xglMatrixStackRotY(node->rotation_y);
        xglMatrixStackRotX(node->rotation_x);
        xglMatrixStackTrans(offset);
        xglMatrixStackSave(matrix);
        position->x += matrix[12];
        position->y += matrix[13];
        position->z += matrix[14];
    } else if (offset != 0) {
        position->x += offset->x;
        position->y += offset->y;
        position->z += offset->z;
    }
    return node;
}

/* Applies all four slots with the (0xF, 0, 3) index pair, then flags the state. */
void cameraApplyAllSlots(void)
{
    cameraApplySlots(0xF, 0, 3, 0xF);
    gameWork.allSlotsFlagB = 1;
    gameWork.flags |= CAMERA_FLAG_TRACK_PLAYER;
    gameWork.allSlotsFlagA = 1;
}

/* One 20-byte entry of cameraSlotTable: the signed key and sub-key cameraSlotFind matches. */
struct CameraKeyEntry {
    signed char key;                /* +0x00 */
    signed char sub;                /* +0x01 */
    unsigned char anchorReady;      /* +0x02 */
    unsigned char unmodeled_03;
    struct CameraSlot slot;         /* +0x04 */
};

/* A key or sub-key of -1 matches nothing: both become -1 before the scan of 32 entries. */
/* A key or sub-key of -1 matches nothing: both become -1 before the scan of 32 entries. */
struct CameraKeyEntry *cameraSlotFind(int key, int subKey)
{
    struct CameraKeyEntry *entry;
    int index;

    entry = (struct CameraKeyEntry *)&cameraSlotTable;
    if (key == -1 ? subKey != -1 : subKey == -1) {
        key = -1;
        subKey = -1;
    }
    for (index = 0; index < 32; index++) {
        if (entry->key == key) {
            if (entry->sub == subKey) {
                return entry;
            }
        }
        entry++;
    }
    return 0;
}

extern struct CameraKeyEntry D_002BF6F0[];

/* The scratchpad vectors cameraOrbitPosition works in: the matrix stack saves its result to +0x50 and the point is read at +0x80. */
typedef struct CameraOrbitScratch {
    unsigned char unmodeled_00[0x80];
    Vector4 result;                     /* +0x80 */
    Vector4 shift;                      /* +0x90: the orbit distance on z */
    unsigned char unmodeled_a0[0x30];
    Vector4 origin;                     /* +0xD0 */
} CameraOrbitScratch;

void cameraOrbitPosition(Vector4 *position, Vector4 *offset, Vector4 *angles)
{
    CameraOrbitScratch *scratch;
    float one;
    float angleZ;

    one = 1.0f;
    scratch = (CameraOrbitScratch *)0x70000000;
    *(int *)&scratch->shift.x = 0;
    *(int *)&scratch->shift.y = 0;
    do {
        angleZ = angles->w;
    } while (0);
    scratch->shift.w = one;
    scratch->shift.z = angleZ;
    scratch->origin.x = offset->x;
    scratch->origin.y = offset->y;
    scratch->origin.z = offset->z;
    scratch->origin.w = one;
    xglMatrixStackUnit();
    xglMatrixStackTrans((float *)&scratch->origin);
    xglMatrixStackRotY(angles->y);
    xglMatrixStackRotX(angles->x);
    xglMatrixStackTrans((float *)&scratch->shift);
    xglMatrixStackSave((float *) 0x70000050);
    position->x = scratch->result.x;
    position->y = scratch->result.y;
    position->z = scratch->result.z;
}

float xglAtan2(float y, float x);

/* Writes the angles from `to` to `from`: x the pitch, y the yaw (plus pi), w the length of the difference. */
void cameraAnglesBetween(Vector4 *angles, const Vector4 *from, const Vector4 *to)
{
    float dx = from->x - to->x;
    float dy = from->y - to->y;
    float dz = from->z - to->z;

    angles->y = xglAtan2(dx, dz) + CAMERA_PI;
    dx = dx * dx;
    dz = dz * dz;
    angles->x = xglAtan2(dy, __builtin_sqrtf(dx + dz));
    angles->w = __builtin_sqrtf(dx + dy * dy + dz);
}

#include "quadword.h"
#include "vu0_macro.h"

void mapBindAndSnapObject(Vector4 *position, UwamonoProbe *probe);
void cameraRangeSetDefaults(struct CameraRange *range);
extern float D_0069AE48;
extern float D_0069AE4C;

/* Scratchpad block the move probe reads and writes: the probe's start point and its moving sample. */
typedef struct MoveProbeScratch {
    unsigned char unmodeled_000[0x150];
    Vector4 from;                       /* +0x150: the probe start (position plus delta) */
    Vector4 sample;                     /* +0x160: the moving sample passed to mapMoveProbe */
} MoveProbeScratch;

float D_0069AE40 = 0.0099999997764825821f;
float mapMoveProbe(const Vector4 *from, const Vector4 *to, void *result);

/* Takes the probe height unless mapMoveProbe reported no hit. */
static inline void setProbeHeight(Vector4 *position, float hit)
{
    if (hit != -1000.0f) {
        position->y = hit;
    }
}

float cameraProbeSlide(Vector4 *position, const Vector4 *delta, void *result);

/* Moves position by delta with mapMoveProbe; when the probe stopped short horizontally it keeps probing with halved steps until
   the step is below 0.01 in x and z. Returns the last probe height (-1000 for no floor). */
float cameraProbeSlide(Vector4 *position, const Vector4 *delta, void *result)
{
    MoveProbeScratch *scratch;
    float hit;
    float dx;

    ((MoveProbeScratch *)0x70000000)->from.x = position->x + delta->x;
    ((MoveProbeScratch *)0x70000000)->from.y = position->y + delta->y;
    ((MoveProbeScratch *)0x70000000)->from.z = position->z + delta->z;
    hit = mapMoveProbe(position, delta, result);
    scratch = (MoveProbeScratch *)0x70000000;
    if (hit != -1000.0f) {
        position->y = hit;
    }
    dx = scratch->from.x - position->x;
    if (D_0069AE40 < __builtin_fabsf(dx) || D_0069AE40 < __builtin_fabsf(scratch->from.z - position->z)) {
        scratch->sample.x = dx * 0.5f;
        scratch->sample.y = (scratch->from.y - position->y) * 0.5f;
        scratch->sample.z = (scratch->from.z - position->z) * 0.5f;
        for (;;) {
            scratch->from.x = position->x;
            scratch->from.z = position->z;
            hit = mapMoveProbe(position, &scratch->sample, result);
            setProbeHeight(position, hit);
            if (!(__builtin_fabsf(position->x - scratch->from.x) < __builtin_fabsf(scratch->sample.x))
                && !(__builtin_fabsf(position->z - scratch->from.z) < __builtin_fabsf(scratch->sample.z))) {
                continue;
            }
            scratch->sample.x *= 0.5f;
            scratch->sample.y *= 0.5f;
            scratch->sample.z *= 0.5f;
            if (0.01f > __builtin_fabsf(scratch->sample.x) && 0.01f > __builtin_fabsf(scratch->sample.z)) {
                break;
            }
        }
    }
    return hit;
}

/* Scratchpad block of cameraPullInAlongSight: lookAt is the look-at point, goal the wanted camera position and
   sight the vector from the look-at point to it. */
typedef struct SlopeProbeScratch {
    unsigned char unmodeled_000[0x30];
    Vector4 lookAt;
    unsigned char unmodeled_040[0x50];
    Vector4 probePosition;
    Vector4 probeDelta;
    Vector4 sight;
    Vector4 goal;
} SlopeProbeScratch;

static inline float slopeAlong(float base, float slope, float t)
{
    return base + slope * t;
}

float D_0069AE48 = 0.05000000074505806f;
float D_0069AE4C = -0.05000000074505806f;

/* Pulls the camera in along the line of sight: probes from the last camera position toward the wanted one, and while the floor
   height at the end is outside the allowed clearance moves the goal along the look-at line down to that height (up to 16 times). */
void cameraPullInAlongSight(void)
{
    SlopeProbeScratch *scratch = (SlopeProbeScratch *)0x70000000;
    int probeCleared;
    float lowerLimit;
    float upperLimit;
    float noHit;
    float clearance;
    float along;
    int i;

    Vu0SubVectorXYZ(&scratch->sight, &scratch->goal, &scratch->lookAt);
    probeCleared = cameraPositionProbe[0].clearedA;
    if (probeCleared == 0) {
        QuadwordCopy(&scratch->probePosition, &cameraProbePosition);
        mapBindAndSnapObject(&scratch->probePosition, &cameraPositionProbe[0]);
        mapMoveProbe(&scratch->probePosition, 0, &cameraPositionProbe[0]);
        probeCleared = cameraPositionProbe[0].clearedA;
    }
    noHit = -1000.0f;
    upperLimit = D_0069AE48;
    lowerLimit = D_0069AE4C;
    for (i = 0; i < 16; i++) {
        float floorHeight;
        float goalX;
        float goalY;
        float goalZ;
        float riseToFloor;
        QuadwordCopy(&scratch->probePosition, &cameraProbePosition);
        Vu0SubVectorXYZ(&scratch->probeDelta, &scratch->goal, &cameraProbePosition);
        cameraPositionProbe[0].clearedA = probeCleared;
        floorHeight = cameraProbeSlide(&scratch->probePosition, &scratch->probeDelta, &cameraPositionProbe[0]);
        if (floorHeight == noHit) {
            if (probeCleared == 0) {
                QuadwordCopy(&cameraProbePosition, &scratch->goal);
            }
            break;
        }
        clearance = floorHeight - scratch->goal.y;
        if (clearance < upperLimit && lowerLimit < clearance) {
            QuadwordCopy(&cameraProbePosition, &scratch->probePosition);
            break;
        }
        riseToFloor = floorHeight - scratch->lookAt.y;
        along = riseToFloor / scratch->sight.y;
        goalX = slopeAlong(scratch->lookAt.x, scratch->sight.x, along);
        goalY = slopeAlong(scratch->lookAt.y, scratch->sight.y, along);
        goalZ = slopeAlong(scratch->lookAt.z, scratch->sight.z, along);
        scratch->goal.x = goalX;
        scratch->goal.y = goalY;
        scratch->goal.z = goalZ;
    }
    QuadwordCopy(&scratch->goal, &cameraProbePosition);
}

/* The query block passed to mapMoveProbe (get.c UnduParam): word 0 and the attribute mask at +8. */
struct UnduParam {
    int query_flags;
    unsigned char unmodeled_04[0x4];
    unsigned short attr_mask;           /* +0x08: 0x1000 is set before each probe */
};

void cameraOrbitPosition(Vector4 *position, Vector4 *offset, Vector4 *angles);
float xglPointLength(Vector4 *from, Vector4 *to);

static inline void copyVector(AlignedVector4 *destination, AlignedVector4 *source)
{
    QuadwordCopy(destination, source);
}

/* The scratchpad block cameraSearchOrbitPitch works in (0x70000000). */
typedef struct CameraSearchScratch {
    struct CameraRecord *record;        /* +0x00: the active camera record */
    unsigned char unmodeled_04[0xC];
    struct XenoActor *actor;            /* +0x10: the actor the camera follows */
    unsigned char unmodeled_14[0x1C];
    AlignedVector4 lookAt;              /* +0x30 */
    unsigned char unmodeled_40[0x50];
    AlignedVector4 delta;               /* +0x90 */
    AlignedVector4 probe;               /* +0xA0 */
    AlignedVector4 orbit;               /* +0xB0: pitch, yaw, -, distance */
    AlignedVector4 position;            /* +0xC0 */
} CameraSearchScratch;

/* Searches the orbit pitch (up to CAMERA_SEARCH_MAX_STEPS steps, halving and reversing the step when the floor height crosses the
   camera height) for a camera position that stands on the floor; the result is left in cameraProbePosition. */
void cameraSearchOrbitPitch(void)
{
    CameraSearchScratch *scratch;
    float floorHeight;
    float pitchStep;
    float pitch;
    float error;
    int flip;
    int iteration;

    scratch = (CameraSearchScratch *)0x70000000;
    scratch->orbit.y = scratch->record->range.angles.y;
    scratch->orbit.w = scratch->record->range.angles.w;
    if (cameraPositionProbe[0].clearedA == 0) {
        *(int *)&scratch->orbit.x = 0;
        cameraOrbitPosition((AlignedVector4 *)&cameraProbePosition, &scratch->lookAt, &scratch->orbit);
        mapBindAndSnapObject((AlignedVector4 *)&cameraProbePosition, &cameraPositionProbe[0]);
        mapMoveProbe((AlignedVector4 *)&cameraProbePosition, 0, &cameraPositionProbe[0]);
        scratch->orbit.x = scratch->actor->position.x;
        cameraOrbitPosition(&scratch->position, &scratch->lookAt, &scratch->orbit);
        Vu0SubVectorXYZ(&scratch->probe, &scratch->position, (AlignedVector4 *)&cameraProbePosition);
        cameraProbeSlide((AlignedVector4 *)&cameraProbePosition, &scratch->probe, &cameraPositionProbe[0]);
    }

    pitch = scratch->record->range.angles.x;
    scratch->orbit.x = pitch;
    cameraOrbitPosition(&scratch->position, &scratch->lookAt, &scratch->orbit);
    pitchStep = CAMERA_SEARCH_STEP_FINE;
    QuadwordCopy(&scratch->probe, (AlignedVector4 *)&cameraProbePosition);
    Vu0SubVectorXYZ(&scratch->delta, &scratch->position, (AlignedVector4 *)&cameraProbePosition);
    cameraPositionProbe[0].mask08 |= 0x1000;
    if (scratch->record->range.probeMode == CAMERA_PROBE_MODE_SLIDE) {
        floorHeight = cameraProbeSlide(&scratch->probe, &scratch->delta, &cameraPositionProbe[0]);
    } else {
        floorHeight = mapMoveProbe(&scratch->probe, &scratch->delta, &cameraPositionProbe[0]);
    }

    if (floorHeight != -1000.0f) {
        if (scratch->delta.y < floorHeight) {
            pitchStep = -pitchStep;
        }
    } else {
        pitch = CAMERA_SEARCH_PITCH_DOWN;
        scratch->orbit.x = pitch;
        pitchStep = CAMERA_SEARCH_STEP_COARSE;
        QuadwordCopy(&scratch->probe, &scratch->lookAt);
        QuadwordCopy(&scratch->position, &scratch->probe);
        cameraPositionProbe[0].mask08 |= 0x1000;
        floorHeight = mapMoveProbe(&scratch->probe, 0, &cameraPositionProbe[0]);
        if (floorHeight == -1000.0f) {
            pitch = 0.0f;
            scratch->orbit.x = pitch;
            cameraOrbitPosition(&scratch->probe, &scratch->lookAt, &scratch->orbit);
            pitchStep = -CAMERA_SEARCH_STEP_COARSE;
            QuadwordCopy(&scratch->position, &scratch->probe);
            cameraPositionProbe[0].mask08 |= 0x1000;
            floorHeight = mapMoveProbe(&scratch->probe, 0, &cameraPositionProbe[0]);
        }
    }

    if (floorHeight == -1000.0f) {
        copyVector((AlignedVector4 *)&cameraProbePosition, &scratch->actor->velocity);
    } else {
        flip = 1;
        *(int *)&scratch->delta.x = 0;
        *(int *)&scratch->delta.y = 0;
        *(int *)&scratch->delta.z = 0;
        QuadwordCopy((AlignedVector4 *)&cameraProbePosition, &scratch->probe);
        iteration = 0;
        while (iteration < CAMERA_SEARCH_MAX_STEPS) {
            error = floorHeight - scratch->position.y;
            if (error < CAMERA_SEARCH_TOLERANCE && -CAMERA_SEARCH_TOLERANCE < error) {
                xglPointLength(&scratch->probe, &scratch->lookAt);
                if (scratch->record->range.probeMode == CAMERA_PROBE_MODE_RESLIDE) {
                    scratch->delta.x = (scratch->probe.x + scratch->delta.x) - ((AlignedVector4 *)(AlignedVector4 *)&cameraProbePosition)->x;
                    scratch->delta.y = (floorHeight + scratch->delta.y) - ((AlignedVector4 *)(AlignedVector4 *)&cameraProbePosition)->y;
                    scratch->delta.z = (scratch->probe.z + scratch->delta.z) - ((AlignedVector4 *)(AlignedVector4 *)&cameraProbePosition)->z;
                    cameraPositionProbe[0].mask08 |= 0x1000;
                    cameraProbeSlide((AlignedVector4 *)&cameraProbePosition, &scratch->delta, &cameraPositionProbe[0]);
                } else {
                    QuadwordCopy((AlignedVector4 *)&cameraProbePosition, &scratch->probe);
                    ((AlignedVector4 *)(AlignedVector4 *)&cameraProbePosition)->y = floorHeight;
                }
                break;
            }
            pitch = pitch + pitchStep;
            scratch->orbit.x = pitch;
            cameraOrbitPosition(&scratch->position, &scratch->lookAt, &scratch->orbit);
            Vu0SubVectorXYZ(&scratch->delta, &scratch->position, &scratch->probe);
            floorHeight = mapMoveProbe(&scratch->probe, &scratch->delta, &cameraPositionProbe[0]);
            if (floorHeight == -1000.0f) {
                if (flip == 1) {
                    pitchStep = -pitchStep * 0.5f;
                }
                flip = 0;
            } else {
                if (((floorHeight - scratch->position.y) < 0.0f && pitchStep < 0.0f) || (0.0f < (floorHeight - scratch->position.y) && 0.0f < pitchStep)) {
                    flip = 0;
                }
                if (flip == 0) {
                    pitchStep = -pitchStep * 0.5f;
                }
                flip = 1;
            }
            iteration = iteration + 1;
        }
        if (iteration >= CAMERA_SEARCH_MAX_STEPS) {
            copyVector((AlignedVector4 *)&cameraProbePosition, &scratch->position);
        }
    }
    QuadwordCopy(&scratch->position, (AlignedVector4 *)&cameraProbePosition);
}

struct CameraScratch {
    struct CameraRecord *record;        /* +0x00: the camera record pointer */
    unsigned char unmodeled_04[0x04];
    XglCamera *camera;                  /* +0x08 */
    int anchorIndex;                    /* +0x0C */
    unsigned char unmodeled_10[0x08];
    unsigned char moved;                /* +0x18 */
    unsigned char seen;                 /* +0x19 */
    unsigned char unmodeled_1A[0x02];
    float step;                         /* +0x1C */
    unsigned char unmodeled_20[0x10];
    Vector4 lookAt;                   /* +0x30 */
    unsigned char unmodeled_40[0x50];
    Vector4 angles;                   /* +0x90 */
    unsigned char unmodeled_A0[0x20];
    Vector4 position;                   /* +0xC0 */
};

/* The scratchpad block the camera update functions share. */
#define CAMERA_PAD ((struct CameraScratch *)0x70000000)

extern int D_0069E608[];
/* Writes the angles from `to` to `from`: x the pitch, y the yaw (plus pi), w the length of the difference. */ void cameraAnglesBetween(Vector4 *angles, const Vector4 *from, const Vector4 *to);
void cameraPullInAlongSight(void);
void cameraSearchOrbitPitch(void);

/* Camera type follow: the look-at point is the anchor plus the offset, the camera position the orbit around it (angles of
   the record), floor-probed when the position probe has a block, with the record's lock axes overriding the result; stores the
   position, look-at and the angles between them. */
void cameraUpdateFollow(void)
{
    struct CameraScratch *scratch;
    struct CameraRecord *record;
    float lookAtX;
    float lookAtY;
    float lookAtZ;

    scratch = (struct CameraScratch *)0x70000000;
    cameraApplyAnchorOffset(&scratch->lookAt, scratch->anchorIndex, -1, (Vector4 *)&gameWork.offsetX);
    scratch->lookAt.w = 1.0f;
    lookAtX = scratch->lookAt.x;
    gameWork.lookAtX = lookAtX;
    lookAtY = scratch->lookAt.y;
    gameWork.lookAtY = lookAtY;
    lookAtZ = scratch->lookAt.z;
    gameWork.lookAtZ = lookAtZ;
    cameraOrbitPosition(&scratch->position, &scratch->lookAt, &scratch->record->range.angles);
    if (D_0069E608[0] != 0) {
        if (scratch->record->range.probeMode == 0) {
            cameraPullInAlongSight();
        } else {
            cameraSearchOrbitPitch();
        }
    }
    record = scratch->record;
    if (record->range.lockX != 0) {
        CAMERA_PAD->position.x = record->range.lockPosition.x;
    }
    if (record->range.lockY != 0) {
        CAMERA_PAD->position.y = record->range.lockPosition.y;
    }
    if (record->range.lockZ != 0) {
        CAMERA_PAD->position.z = record->range.lockPosition.z;
    }
    cameraAnglesBetween(&scratch->angles, &scratch->lookAt, &scratch->position);
    gameWork.pitch = CAMERA_PAD->angles.x;
    gameWork.yaw = *(float *)0x70000094;
    gameWork.distance = *(float *)0x7000009C;
    /* The CameraVector union: the original orders these three stores with alias set 0. */
    ((CameraVector *)&cameraProbePosition)->vector.x = *(float *)0x700000C0;
    ((CameraVector *)&cameraProbePosition)->vector.y = *(float *)0x700000C4;
    gameWork.positionX = *(float *)0x700000C0;
    gameWork.positionY = *(float *)0x700000C4;
    gameWork.positionZ = ((CameraVector *)&cameraProbePosition)->vector.z = *(float *)0x700000C8;
    gameWork.perspective = scratch->record->range.lockPosition.w;
    gameWork.roll = scratch->record->range.lookAt.w;
}


/* Copies the record's anchor vectors into gameWork when D_0069E568 is set, applies the anchor offset when the
   record has an anchor, and writes the angles from the staged vector to gameWork. */
void cameraUpdateFromRecord(void)
{
    struct CameraRecord *record;
    Vector4 anchorPosition;
    float stageX;
    float stageY;
    float stageZ;

    if (D_0069E568[0] != 0) {
        record = CAMERA_PAD->record;
        gameWork.positionX = record->range.lockPosition.x;
        gameWork.positionY = record->range.lockPosition.y;
        gameWork.positionZ = record->range.lockPosition.z;
        gameWork.lookAtX = record->range.lookAt.x;
        gameWork.lookAtY = record->range.lookAt.y;
        gameWork.lookAtZ = record->range.lookAt.z;
        gameWork.perspective = record->range.lockPosition.w;
        gameWork.roll = record->range.lookAt.w;
    } else {
        record = CAMERA_PAD->record;
    }
    ((CameraVector *)&cameraProbePosition)->vector.x = gameWork.positionX;
    ((CameraVector *)&cameraProbePosition)->vector.y = gameWork.positionY;
    ((CameraVector *)&cameraProbePosition)->vector.z = gameWork.positionZ;
    if (record->range.anchorIndex >= 0) {
        cameraApplyAnchorOffset(&anchorPosition, record->range.anchorIndex, record->range.anchorSubIndex, (Vector4 *)&gameWork.offsetX);
        gameWork.lookAtX = anchorPosition.x;
        gameWork.lookAtY = anchorPosition.y;
        gameWork.lookAtZ = anchorPosition.z;
    }
    (CAMERA_PAD->lookAt.x) = gameWork.lookAtX;
    (*(float *)0x70000034) = gameWork.lookAtY;
    (*(float *)0x70000038) = gameWork.lookAtZ;
    cameraAnglesBetween(&CAMERA_PAD->angles, &CAMERA_PAD->lookAt, &cameraProbePosition);
    gameWork.pitch = *(float *)0x70000090;
    gameWork.yaw = *(float *)0x70000094;
    gameWork.distance = *(float *)0x7000009C;
}


/* Stages the current camera position and the anchor-adjusted look-at point for cameraApplyView (scratchpad +0x30). */
void cameraStageLookAt(void)
{
    Vector4 anchorPosition;
    int anchorIndex;

    cameraProbePosition.v.x = gameWork.positionX;
    cameraProbePosition.v.y = gameWork.positionY;
    cameraProbePosition.v.z = gameWork.positionZ;
    anchorIndex = gameWork.anchorIndex;
    if (anchorIndex >= 0) {
        cameraApplyAnchorOffset(&anchorPosition, anchorIndex, gameWork.anchorSubIndex, (Vector4 *)&gameWork.offsetX);
        gameWork.lookAtX = anchorPosition.x;
        gameWork.lookAtY = anchorPosition.y;
        gameWork.lookAtZ = anchorPosition.z;
    }
    CAMERA_PAD->lookAt.x = gameWork.lookAtX;
    *(float *)0x70000034 = gameWork.lookAtY;
    *(float *)0x70000038 = gameWork.lookAtZ;
}

/* Classifies angle against the arc from limits[0] to limits[1] (both wrapped into [0, 2pi)): -1 inside the arc,
   otherwise the index of the nearer limit (1 before the midpoint of the excluded arc, 0 after it). */
int cameraAngleLimitIndex(float *limits, float angle)
{
    float fromMin;
    float arcSpan;
    float excludedMidpoint;

    fromMin = angle - limits[0];
    arcSpan = limits[1] - limits[0];
    if (CAMERA_TWO_PI < fromMin) {
        do {
            fromMin -= CAMERA_TWO_PI;
        } while (CAMERA_TWO_PI < fromMin);
    }
    if (fromMin < 0.0f) {
        do {
            fromMin += CAMERA_TWO_PI;
        } while (fromMin < 0.0f);
    }
    if (CAMERA_TWO_PI < arcSpan) {
        do {
            arcSpan -= CAMERA_TWO_PI;
        } while (CAMERA_TWO_PI < arcSpan);
    }
    if (arcSpan < 0.0f) {
        do {
            arcSpan += CAMERA_TWO_PI;
        } while (arcSpan < 0.0f);
    }
    if (fromMin < arcSpan) {
        return -1;
    }
    excludedMidpoint = arcSpan * 0.5f + CAMERA_PI;
    if (fromMin < excludedMidpoint) {
        return 1;
    }
    return 0;
}

float xglSin(float angle);
float xglCos(float angle);

/* Clamps the view yaw and pitch to the record's angle limits (looking at the nearest limit when outside), then recomputes the
   look-at point from the position, angles and distance; flags in scratchpad +0x18 whether the look-at point moved by more than 0.1. */
void cameraClampAngles(int afterCommit)
{
    Vector4 back;
    Vector4 eye;
    float matrix[16];
    float yaw;
    float pitch;
    float yawDelta;
    float depth;
    float tanYaw;
    float shift;
    int limitIndex;
    int moved;

    if (afterCommit == 0 || gameWork.anchorReady != 0) {
        yaw = gameWork.yaw;
        pitch = gameWork.pitch;
        yawDelta = yaw;
        limitIndex = cameraAngleLimitIndex(&(CAMERA_PAD->record)->range.angleLimits.z, yaw);
        if (limitIndex >= 0) {
            yaw = (&(CAMERA_PAD->record)->range.angleLimits.z)[limitIndex];
            depth = gameWork.lookAtZ - gameWork.positionZ;
            tanYaw = xglSin(yaw) / xglCos(yaw);
            gameWork.lookAtX = gameWork.positionX + depth * tanYaw;
            cameraAnglesBetween(&CAMERA_PAD->angles, (Vector4 *)&gameWork.lookAtX, (Vector4 *)&gameWork.positionX);
            gameWork.pitch = CAMERA_PAD->angles.x;
            pitch = gameWork.pitch;
            gameWork.yaw = CAMERA_PAD->angles.y;
            gameWork.distance = CAMERA_PAD->angles.w;
        }
        yawDelta = yaw - yawDelta;
        while (yawDelta < -CAMERA_PI) {
            yawDelta = yawDelta + CAMERA_TWO_PI;
        }
        while (CAMERA_PI < yawDelta) {
            yawDelta = yawDelta - CAMERA_TWO_PI;
        }
        if (yawDelta < -CAMERA_HALF_PI || CAMERA_HALF_PI < yawDelta) {
            pitch = CAMERA_PI - pitch;
        }
        limitIndex = cameraAngleLimitIndex(&(CAMERA_PAD->record)->range.angleLimits.x, pitch);
        if (limitIndex >= 0) {
            pitch = (&(CAMERA_PAD->record)->range.angleLimits.x)[limitIndex];
        }
        gameWork.pitch = pitch;
        gameWork.yaw = yaw;
        eye.x = gameWork.positionX;
        eye.y = gameWork.positionY;
        back.z = -gameWork.distance;
        eye.z = gameWork.positionZ;
        back.x = 0.0f;
        back.y = 0.0f;
        back.w = 1.0f;
        eye.w = 1.0f;
        xglMatrixStackUnit();
        xglMatrixStackTrans((float *)&eye);
        xglMatrixStackRotY(yaw);
        xglMatrixStackRotX(pitch);
        xglMatrixStackTrans((float *)&back);
        xglMatrixStackSave(matrix);
        moved = 0;
        shift = gameWork.lookAtX - matrix[12];
        if (shift < -0.1f || 0.1f < shift) {
            moved = 1;
        }
        shift = gameWork.lookAtY - matrix[13];
        if (shift < -0.1f || 0.1f < shift) {
            moved = 1;
        }
        shift = gameWork.lookAtZ - matrix[14];
        if (shift < -0.1f || 0.1f < shift) {
            moved = 1;
        }
        CAMERA_PAD->moved = moved;
        gameWork.lookAtX = matrix[12];
        gameWork.lookAtY = matrix[13];
        gameWork.lookAtZ = matrix[14];
    }
}

/* Once the slots are applied (CAMERA_FLAG_SLOTS_APPLIED), copies the current position, look-at, perspective, roll and angles
   to the "to" targets of the slot transition; while allSlotsFlagB is set it also re-bases the anchor offset on the anchor
   joint and root. Returns 1 when it ran, 0 otherwise. */
int cameraCommitSlots(void)
{
    struct CameraState *state = &gameWork;
    Vector4 jointPosition;
    Vector4 rootPosition;

    if ((state->flags & CAMERA_FLAG_SLOTS_APPLIED) != 0) {
        float anchorX;
        float anchorY;
        float anchorZ;

        state->toLookAtX = state->lookAtX;
        state->toLookAtY = state->lookAtY;
        state->toLookAtZ = state->lookAtZ;
        state->toRoll = state->roll;
        state->toPositionX = state->positionX;
        state->toPositionY = state->positionY;
        state->toPositionZ = state->positionZ;
        state->toPerspective = state->perspective;
        state->toPitch = state->pitch;
        state->toYaw = state->yaw;
        state->toDistance = state->distance;
        if (state->allSlotsFlagB != 0) {
            cameraApplyAnchorOffset(&jointPosition, state->anchorIndex, state->anchorSubIndex, 0);
            cameraApplyAnchorOffset(&rootPosition, state->anchorIndex, -1, 0);
            anchorX = (state->anchorBase[0] + rootPosition.x) - jointPosition.x;
            anchorY = (state->anchorBase[1] + rootPosition.y) - jointPosition.y;
            anchorZ = (state->anchorBase[2] + rootPosition.z) - jointPosition.z;
            state->fromOffsetX = anchorX;
            state->offsetX = anchorX;
            state->fromOffsetY = anchorY;
            state->offsetY = anchorY;
            state->fromOffsetZ = anchorZ;
            state->offsetZ = anchorZ;
            if (state->anchorReady != 0) {
                if (CAMERA_PAD->seen != 0 && CAMERA_PAD->moved == 0) {
                    if (state->anchorLocked == 0) {
                        state->slot[0].duration = 0.0f;
                    }
                }
            }
            gameWork.allSlotsFlagB = 0;
        }
        return 1;
    }
    return 0;
}

float cameraEase(u8 mode, float time, float duration)
{
    float scale;
    float half;
    float doubled;
    float rate;

    if (duration == 0.0f) {
        return 1.0f;
    }
    scale = 1.0f / duration;
    switch (mode) {
    case 1:
        doubled = scale + scale;
        half = duration * 0.5f;
        if (time <= half) {
            rate = time * doubled;
            rate = rate * rate * rate * rate * 0.5f;
        } else {
            rate = 1.0f - (time - half) * doubled;
            rate = 1.0f - rate * rate * rate * rate * 0.5f;
        }
        break;
    case 2:
        rate = time * scale;
        rate = rate * rate;
        break;
    case 3:
        rate = 1.0f - time * scale;
        rate = 1.0f - rate * rate;
        break;
    case 0:
    default:
        rate = time * scale;
        break;
    }
    if (rate < 0.0f) {
        rate = 0.0f;
    }
    if (rate > 1.0f) {
        rate = 1.0f;
    }
    return rate;
}

/* Linear interpolation from `from` to `to` by `fraction`. */
float cameraLerpFloat(float from, float to, float fraction)
{
    return ((to - from) * fraction) + from;
}

/* Interpolates an angle from `from` to `to` by `fraction` along the shorter way round the circle. */
float cameraLerpAngle(float from, float to, float fraction)
{
    to = to - from;
    if (to > 3.14159265f) {
        do {
            to -= 6.2831853f;
        } while (to > 3.14159265f);
    }
    if (to < -3.14159265f) {
        do {
            to += 6.2831853f;
        } while (to < -3.14159265f);
    }
    return from + to * fraction;
}

/* VU0 linear interpolation of two vectors into destination, by fraction. */
void cameraLerpVector(Vector4 *destination, const Vector4 *from, const Vector4 *to, float fraction)
{
    __asm__ __volatile__(
        "lqc2 vf4, 0(%0)\n\t"
        "lqc2 vf2, 0(%1)\n\t"
        "lqc2 vf1, 0(%2)\n\t"
        "qmtc2.ni %3, vf3\n\t"
        "vsub.xyz vf4, vf2, vf1\n\t"
        "vmulx.xyz vf2, vf4, vf3x\n\t"
        "vadd.xyz vf4, vf2, vf1\n\t"
        "sqc2 vf4, 0(%0)"
        : : "r"(destination), "r"(to), "r"(from), "r"(fraction) : "memory");
}

/* Advances the wave one frame and returns its offset: rising from -amplitude to amplitude over the first half of the
   period, falling back over the second; zero once no cycles remain. */
float cameraWaveStep(struct CameraWave *wave)
{
    float result = 0.0f;
    float doubled;
    int half;
    unsigned int period;
    unsigned int phase;

    if (wave->remaining == 0) {
        return result;
    }
    phase = wave->phase + 1;
    wave->phase = phase;
    period = wave->period;
    if ((unsigned short)phase >= period) {
        wave->phase = 0;
    }
    if (wave->hold != 0) {
        wave->hold = wave->hold - 1;
    } else {
        wave->remaining = wave->remaining - 1;
        wave->amplitude = wave->amplitude - wave->step;
        if (wave->amplitude < 0.0f) {
            wave->amplitude = 0.0f;
        }
    }
    half = wave->period >> 1;
    doubled = wave->amplitude + wave->amplitude;
    if (wave->phase < half) {
        result = (doubled * I2F(wave->phase)) / I2F(half) - wave->amplitude;
    } else {
        result = wave->amplitude - (doubled * I2F(wave->phase - half)) / I2F(half);
    }
    return result;
}

float cameraEase(u8 mode, float time, float duration);
/* Interpolates an angle from `from` to `to` by `fraction` along the shorter way round the circle. */ float cameraLerpAngle(float from, float to, float fraction);
float xglFRand(void);
extern float D_0069AEC0;
extern float D_0069AEC4;

/* Eases the four camera slots by the frame delta, copies finished vectors into the applied set and drives the
   three shake waves through the scratchpad block; the camera move runs last. */
float D_0069AEC0 = 0.029999999329447746f;
float D_0069AEC4 = 9.9999997473787516e-05f;

extern const float D_0069AEC8;
extern const float D_0069AECC;
extern const float D_0069AED0;
extern const float D_0069AED4;
extern const float D_0069AED8;
extern const float D_0069AEDC;

void cameraAdvanceTracks(float *tracks)
{
    float e;
    float r1;
    float r2;
    float r3;
    float r4;
    float t1;
    float t2;
    float p2;
    float p3;
    float k1;
    float k2;
    float lim2;
    float d2;
    union { CameraTravel *p; } *scratch;
    float lim3;
    float d3;
    float cy;
    float cz;

    scratch = (union { CameraTravel *p; } *)0x70000010;
    if (gameWork.slot[0].progress < gameWork.slot[0].duration) {
        gameWork.slot[0].progress += CAMERA_PAD->step;
        gameWork.slot[0].fraction = cameraEase((unsigned char)gameWork.slot[0].easeMode, gameWork.slot[0].progress, gameWork.slot[0].duration);
        cameraLerpVector((Vector4 *)&gameWork.lookAtX, (Vector4 *)&gameWork.fromLookAtX, (Vector4 *)&gameWork.toLookAtX, gameWork.slot[0].fraction);
    }

    if (gameWork.slot[1].progress < gameWork.slot[1].duration) {
        gameWork.slot[1].progress += CAMERA_PAD->step;
        e = cameraEase((unsigned char)gameWork.slot[1].easeMode, gameWork.slot[1].progress, gameWork.slot[1].duration);
        gameWork.slot[1].fraction = e;
        if (gameWork.slot[1].blendMode == 0) {
            cameraLerpVector((Vector4 *)&gameWork.positionX, (Vector4 *)&gameWork.fromPositionX, (Vector4 *)&gameWork.toPositionX, e);
            cameraAnglesBetween(&CAMERA_PAD->angles, (Vector4 *)&gameWork.lookAtX, (Vector4 *)&gameWork.positionX);
            gameWork.pitch = CAMERA_PAD->angles.x;
            gameWork.yaw = CAMERA_PAD->angles.y;
            gameWork.distance = CAMERA_PAD->angles.w;
        } else {
            gameWork.pitch = cameraLerpAngle(gameWork.fromPitch, gameWork.toPitch, e);
            gameWork.yaw = cameraLerpAngle(gameWork.fromYaw, gameWork.toYaw, e);
            gameWork.distance = cameraLerpFloat(gameWork.fromDistance, gameWork.toDistance, e);
            cameraOrbitPosition((Vector4 *)&gameWork.positionX, (Vector4 *)&gameWork.lookAtX, (Vector4 *)&gameWork.pitch);
        }
        cameraLerpVector((Vector4 *)&gameWork.offsetX, (Vector4 *)&gameWork.fromOffsetX, (Vector4 *)tracks, gameWork.slot[1].fraction);
    }

    p2 = gameWork.slot[2].progress;
    lim2 = gameWork.slot[2].duration;
    if (p2 < lim2) {
        float next2 = p2 + CAMERA_PAD->step;
        gameWork.slot[2].progress = next2;
        d2 = cameraEase((unsigned char)gameWork.slot[2].easeMode, next2, lim2);
        gameWork.slot[2].fraction = d2;
        gameWork.perspective = cameraLerpFloat(gameWork.fromPerspective, gameWork.toPerspective, gameWork.slot[2].fraction);
        p2 = next2;
    }

    p3 = gameWork.slot[3].progress;
    lim3 = gameWork.slot[3].duration;
    if (p3 < lim3) {
        float next3 = p3 + CAMERA_PAD->step;
        gameWork.slot[3].progress = next3;
        d3 = cameraEase((unsigned char)gameWork.slot[3].easeMode, next3, lim3);
        gameWork.slot[3].fraction = d3;
        gameWork.roll = cameraLerpAngle(gameWork.fromRoll, gameWork.toRoll, gameWork.slot[3].fraction);
        p3 = next3;
    }

    if (gameWork.slot[0].progress >= gameWork.slot[0].duration) {
        gameWork.slot[0].fraction = 1.0f;
        gameWork.fromLookAtX = gameWork.lookAtX;
        gameWork.fromLookAtY = gameWork.lookAtY;
        gameWork.fromLookAtZ = gameWork.lookAtZ;
        gameWork.slot[0].progress = 0.0f;
        gameWork.slot[0].duration = 0.0f;
    }

    if (gameWork.slot[1].progress >= gameWork.slot[1].duration) {
        gameWork.slot[1].fraction = 1.0f;
        gameWork.fromPositionX = gameWork.positionX;
        gameWork.fromPositionY = gameWork.positionY;
        gameWork.fromPositionZ = gameWork.positionZ;
        gameWork.fromPitch = gameWork.pitch;
        gameWork.fromYaw = gameWork.yaw;
        gameWork.fromDistance = gameWork.distance;
        gameWork.flags = gameWork.flags & 0xffef;
        gameWork.slot[1].progress = 0.0f;
        gameWork.slot[1].duration = 0.0f;
        gameWork.offsetX = tracks[0];
        gameWork.offsetY = tracks[1];
        gameWork.offsetZ = tracks[2];
    }

    if (p2 >= lim2) {
        gameWork.slot[2].fraction = 1.0f;
        gameWork.fromPerspective = gameWork.perspective;
        gameWork.slot[2].progress = 0.0f;
        gameWork.slot[2].duration = 0.0f;
    }

    if (p3 >= lim3) {
        gameWork.slot[3].fraction = 1.0f;
        gameWork.fromRoll = gameWork.roll;
        gameWork.slot[3].progress = 0.0f;
        gameWork.slot[3].duration = 0.0f;
    }

    *(int *)0x700000C0 = 0;
    *(int *)0x700000C4 = 0;
    *(int *)0x700000C8 = 0;
    if (gameWork.shakeFlags != 0) {
        CAMERA_PAD->position.x = cameraWaveStep(&gameWork.waves[0]);
        *(float *)0x700000C4 = cameraWaveStep(&gameWork.waves[1]);
        *(float *)0x700000C8 = cameraWaveStep(&gameWork.waves[2]);
        if (gameWork.waves[0].remaining == 0) {
            gameWork.shakeFlags &= 0xFE;
        }
        if (gameWork.waves[1].remaining == 0) {
            gameWork.shakeFlags &= 0xFD;
        }
        if (gameWork.waves[2].remaining == 0) {
            gameWork.shakeFlags &= 0xFB;
        }
        if ((gameWork.shakeFlags & 0x8) != 0) {
            r1 = xglFRand();
            r2 = xglFRand();
            k1 = D_0069AEC0;
            k2 = D_0069AEC4;
            t1 = -(gameWork.sway[0] + gameWork.sway[2]) * ((r1 - 1.0f) * k1) + (r2 - 1.5f) * k2;
            r3 = xglFRand();
            r4 = xglFRand();
            t2 = -(gameWork.sway[1] + gameWork.sway[3]) * ((r3 - 1.0f) * k1) + (r4 - 1.5f) * k2;
            gameWork.sway[2] += t1;
            gameWork.sway[0] += gameWork.sway[2];
            CAMERA_PAD->position.x += gameWork.sway[0];
            gameWork.sway[3] += t2;
            gameWork.sway[1] += gameWork.sway[3];
            CAMERA_PAD->position.y += gameWork.sway[1];
        }
    }

    scratch->p->position.x = gameWork.positionX;
    cy = gameWork.positionY;
    scratch->p->position.y = cy;
    cz = gameWork.positionZ;
    scratch->p->position.z = cz;
    {
        CameraTravel *q = scratch->p;
        q->fov = gameWork.perspective;
        q->chase_position.x = gameWork.lookAtX;
    }
    scratch->p->chase_position.y = gameWork.lookAtY;
    scratch->p->chase_position.z = gameWork.lookAtZ;
    scratch->p->rotation.z = gameWork.roll + CAMERA_PAD->position.z;
    cameraAnglesBetween(&CAMERA_PAD->angles, &scratch->p->chase_position, &scratch->p->position);
    scratch->p->rotation.y = *(float *)0x70000094 + *(float *)0x700000C0;
    scratch->p->rotation.x = *(float *)0x70000090 + *(float *)0x700000C4;
    QuadwordCopy(&scratch->p->chase_reference, &scratch->p->chase_position);
    xglCameraMove(*(XglCamera **)0x70000008);
}

/* The ground probe block at cameraPlayerProbe: a UwamonoProbe followed by the record index the probe was last mapped to. */
typedef struct CameraGroundProbe {
    UwamonoProbe probe;
    unsigned char recordIndex;          /* +0x20 */
} CameraGroundProbe;

extern Vector4 D_002B5FF0;
void cameraApplyView(void);
void cameraUpdateFollow(void);
void cameraUpdateFromRecord(void);
int cameraCommitSlots(void);

/* The scratchpad block cameraUpdateFrame works in (0x70000000). */
typedef struct CameraFrameScratch {
    struct CameraRecord *record;        /* +0x00: the active camera record */
    struct CameraSlot *slot;            /* +0x04: the slot settings in use */
    XglCamera *camera;                  /* +0x08 */
    struct CameraAnchor *anchor;        /* +0x0C: the actor the camera follows */
    void *travel;                       /* +0x10: &camera->travel */
    short phase;                        /* +0x14 */
    unsigned char unmodeled_16[2];
    unsigned char anchorLocked;         /* +0x18 */
    unsigned char anchorMatched;        /* +0x19: the record anchors the current anchor index */
    unsigned char unmodeled_1a[2];
    float height;                       /* +0x1C */
    unsigned char unmodeled_20[0x3C - 0x20];
    float scale;                        /* +0x3C */
    unsigned char unmodeled_40[0xC0 - 0x40];
    Vector4 delta;                      /* +0xC0: the anchor move of this frame */
} CameraFrameScratch;

void cameraUpdateFrame(void)
{
    XglCamera *camera;
    CameraGroundProbe *probe;
    struct CameraAnchor *anchor;
    struct CameraRecord *record;
    struct CameraSlot *slot;
    struct CameraKeyEntry *entry;
    int control;
    int flags;
    int recordIndex;
    int previous;
    unsigned char anchorReady;
    float ret;
    CameraFrameScratch *scratch = (CameraFrameScratch *)0x70000000;

    if (!(gameWork.flags & CAMERA_FLAG_DISABLED)) {
        if (gameWork.flags & CAMERA_FLAG_VIEW_ONLY) {
            cameraApplyView();
            return;
        }
        scratch->phase = CAMERA_INITIAL_WAIT;
        camera = xglStudioGetCamera2(0);
        scratch->camera = camera;
        if (camera->state != CAMERA_STATE_GAME) {
            return;
        }
        control = gameWork.control;
        scratch->travel = &camera->travel;
        scratch->height = camera->unknown_5f4;
        if (control & CONTROL_PAUSED) {
            scratch->height = 0.0f;
        }
        scratch->anchorMatched = 0;
        scratch->scale = 1.0f;
        gameWork.shakeFlags = 0;
        scratch->anchor = uwamonoSlotTable[uwamonoCurrentIndex].head.actor;
        if (!(control & 0x400)) {
            if (control & 0x20) {
                cameraStudioReset();
                gameWork.flags |= CAMERA_FLAG_SNAP;
            } else {
                flags = gameWork.flags;
                if (flags & CAMERA_FLAG_SNAP) {
                    gameWork.flags = flags | (CAMERA_FLAG_SNAP | CAMERA_FLAG_MODE_CHANGED | CAMERA_FLAG_TRACK_PLAYER);
                    gameWork.recordIndex = 0;
                    probe = (CameraGroundProbe *)cameraPlayerProbe;
                    mapProbeInit(&probe->probe);
                    probe->probe.scratchWork = 0x70000170;
                    probe->probe.mask08 = 0x800;
                    cameraSetIndex(-1);
                    gameWork.mode = 0;
                    cameraSetMode(-1, 0, 0);
                    flags = gameWork.flags;
                }
                if (flags & CAMERA_FLAG_TRACK_PLAYER) {
                    cameraSetIndex(-1);
                    cameraPlayerPositionOld = (scratch->anchor)->position;
                    ((CameraFlagsView *)&gameWork)->flags &= 0xFFDF;
                    gameWork.recordIndex = -1;
                    D_0069E5A0[0].clearedA = 0;
                    cameraPositionProbe[0].clearedA = 0;
                    ((CameraGroundProbe *)cameraPlayerProbe)->probe.clearedA = 0;
                }
                recordIndex = 0;
                {
                    CameraGroundProbe *probe = (CameraGroundProbe *)cameraPlayerProbe;
                    if (probe->probe.ground != 0) {
                        anchor = scratch->anchor;
                        Vu0SubVector(&scratch->delta, &anchor->position, &cameraPlayerPositionOld);
                        ret = mapMoveProbe(&cameraPlayerPositionOld, &scratch->delta, probe);
                        recordIndex = probe->recordIndex + 1;
                        if (__builtin_fabsf(cameraPlayerPositionOld.y - ret) > 4.0f) {
                            probe->probe.mask08 |= 0x1000;
                        }
                    }
                }
                record = &cameraCfSlots.record[recordIndex];
                scratch->record = record;
                scratch->slot = &record->slot;
                previous = gameWork.recordIndex;
                if (previous != recordIndex) {
                    gameWork.previousRecordIndex = gameWork.recordIndex;
                    gameWork.recordIndex = recordIndex;
                    entry = cameraSlotFind(previous, (signed char)recordIndex);
                    if (entry != 0) {
                        scratch->slot = &entry->slot;
                        anchorReady = gameWork.anchorReady = entry->anchorReady;
                    } else {
                        record = scratch->record;
                        anchorReady = gameWork.anchorReady = (unsigned char)record->range.rangeMode;
                    }
                    cameraSetMode((signed char)scratch->record->index02, 0, 0);
                    if (gameWork.allSlotsFlagA == 0) {
                        slot = scratch->slot;
                        cameraApplySlots(0xF, slot->blendMode, slot->easeMode, (int)slot->duration);
                        if (gameWork.anchorIndex != -1) {
                            record = scratch->record;
                            if (gameWork.anchorIndex == record->range.anchorIndex
                                && !(*(long long *)&record->range.lockX & 0xFFFFFF)) {
                                scratch->anchorMatched = 1;
                            }
                        }
                    }
                    record = scratch->record;
                    if (gameWork.cameraType != record->unmodeled_00) {
                        gameWork.previousCameraType = gameWork.cameraType;
                        gameWork.cameraType = record->unmodeled_00;
                        gameWork.wordE0 = 0;
                        gameWork.unmodeled_0D3 = 0;
                    }
                    *(struct CameraRange *)&gameWork.anchorIndex = record->range;
                    gameWork.anchorReady = anchorReady;
                    gameWork.allSlotsFlagB = 1;
                }
                if (gameWork.flags & CAMERA_FLAG_SNAP) {
                    gameWork.slot[0].duration = 0.0f;
                    gameWork.flags &= 0xFFEF;
                    gameWork.slot[1].duration = 0.0f;
                    gameWork.slot[2].duration = 0.0f;
                    gameWork.slot[3].duration = 0.0f;
                }
                switch (gameWork.cameraType) {
                case CAMERA_TYPE_FOLLOW:
                    cameraUpdateFollow();
                    cameraClampAngles(0);
                    if (cameraCommitSlots() != 0) {
                        cameraUpdateFollow();
                        cameraClampAngles(1);
                    }
                    cameraAdvanceTracks((float *)&scratch->record->range.offset);
                    break;
                case CAMERA_TYPE_RECORD:
                    cameraUpdateFromRecord();
                    cameraClampAngles(0);
                    if (cameraCommitSlots() != 0) {
                        cameraUpdateFromRecord();
                        cameraClampAngles(1);
                    }
                    cameraAdvanceTracks((float *)&scratch->record->range.offset);
                    break;
                }
                cameraApplyAnchorOffset(&gameWork.anchorOffset, gameWork.anchorIndex, -1, 0);
                gameWork.anchorBase[0] = gameWork.lookAtX - gameWork.anchorOffset.x;
                gameWork.anchorBase[1] = gameWork.lookAtY - gameWork.anchorOffset.y;
                gameWork.anchorBase[2] = gameWork.lookAtZ - gameWork.anchorOffset.z;
                gameWork.anchorLocked = scratch->anchorLocked;
                if (gameWork.rangeFlag != 0) {
                    gameWork.stickAngle = scratch->camera->travel.rotation.y;
                }
                gameWork.allSlotsFlagA = 0;
                gameWork.flags &= 0xFF3F;
            }
        }
    }
}

/* Pending child indices of the robo change (two ints, -1 when none); no symbol recovered yet. */
extern int uwamonoPartIndices[];

/* Key ranges, 68 bytes each: lo at +0x00, hi at +0x04, value at +0x24; no symbol recovered yet. */
typedef struct UwamonoKeyRange {
    int lo;
    int hi;
    unsigned char unmodeled_08[0x20 - 0x08];
    int mapped;                         /* +0x20: the id the block-A search maps a key to */
    int value;
    unsigned char unmodeled_28[0x44 - 0x28];
} UwamonoKeyRange;

/* The key range table, read as words by uwamonoChangePlayerModel (fields in enum UwamonoKeyField). */
extern int uwamonoKeyRangeTable[10 * 17];

void cameraStageLookAt(void);
/* Eases the four camera slots by the frame delta, copies finished vectors into the applied set and drives the three shake waves through the scratchpad block; the camera move runs last. */ void cameraAdvanceTracks(float *tracks);
void xglCameraMove(XglCamera *camera);

/* Publishes the studio camera to the scratchpad block, then runs the view update only while the camera is in
   CAMERA_STATE_GAME; the scratchpad camera pointer is read back after the calls because they may have rewritten it. */
void cameraApplyView(void)
{
    XglCamera *camera;
    CameraFrameScratch *scratch = (CameraFrameScratch *)0x70000000;

    scratch->record = 0;
    scratch->phase = CAMERA_INITIAL_WAIT;
    camera = xglStudioGetCamera2(0);
    scratch->camera = camera;
    if (camera->state != CAMERA_STATE_GAME) {
        return;
    }
    scratch->travel = &camera->travel;
    scratch->height = camera->unknown_5f4;
    scratch->scale = 1.0f;
    if ((gameWork.control & CONTROL_PAUSED) != 0) {
        *(int *)&scratch->height = 0;
    }
    cameraStageLookAt();
    cameraAdvanceTracks(&gameWork.offsetX);
    xglCameraMove(scratch->camera);
}

/* The attachment table uwamonoAttachIdTable: eight entries of four words {lo, hi, id0, id1}; the entry applies to the model ids lo..hi and names the
   two part models that go with it, ids of -1 are unused. */
extern unsigned char D_0069B9B7;
extern unsigned int uwamonoAttachIdTable[UWAMONO_ATTACH_TABLE_WORDS];
void resUpdatePathSlot(int slot);
void resPathSlotLoadID(int slot, int value, int mode);

/* Loads model modelId into path slot `slot`, then the part models the attachment table lists for that model id. */
void resPathSlotLoadWithParts(int slot, unsigned int modelId)
{
    unsigned int headerFlag = 0;
    int entry;
    int part;

    if (D_0069B9B7 == 0) {
        headerFlag = 0x80000000;
    }
    D_0069B9B7 = 0;
    resUpdatePathSlot(slot);
    resPathSlotLoadID(slot, headerFlag | modelId, 0);
    for (entry = 0; entry < UWAMONO_ATTACH_TABLE_WORDS; entry += UWAMONO_ATTACH_ENTRY_WORDS) {
        if (modelId >= uwamonoAttachIdTable[entry] && modelId <= uwamonoAttachIdTable[entry + 1]) {
            for (part = 0; part < UWAMONO_ATTACH_PARTS; part++) {
                if (uwamonoAttachIdTable[entry + 2 + part] != -1) {
                    resPathSlotLoadID(slot, headerFlag | uwamonoAttachIdTable[entry + 2 + part], 0);
                }
            }
            return;
        }
    }
}

int resPathSlotFindByOwner(unsigned int id);

/* Loads three model ids into the three path slots: an id that resPathSlotFindByOwner finds already resident keeps its slot; then
   every free slot takes the first pending id, which resPathSlotLoadWithParts loads with its parts. */
void resPathSlotsLoadModels(int modelId0, int modelId1, int modelId2)
{
    unsigned int pending[3];
    unsigned int routed[3];
    int residentSlot;
    int source;
    int slot;
    int candidate;

    pending[0] = modelId0;
    pending[1] = modelId1;
    pending[2] = modelId2;
    routed[0] = 0;
    routed[1] = 0;
    routed[2] = 0;
    for (source = 0; source < 3; source++) {
        residentSlot = resPathSlotFindByOwner(pending[source]);
        if (residentSlot >= 0) {
            routed[residentSlot] = pending[source];
            pending[source] = 0;
        }
    }
    for (slot = 0; slot < 3; slot++) {
        if (routed[slot] == 0) {
            for (candidate = 0; candidate < 3; candidate++) {
                if (pending[candidate] != 0) {
                    resPathSlotLoadWithParts(slot, pending[candidate]);
                    routed[slot] = pending[candidate];
                    pending[candidate] = 0;
                    break;
                }
            }
        }
    }
}

/* Attaches the player's part records (uwamonoPartIndices) to the player actor: in the ES form to joints 80 and 81 (the first shown, the
   second hidden), otherwise released and re-attached to joints 10 and 7; then applies the key range value to the actor. */
void uwamonoAttachPlayerParts(void)
{
    UwamonoObject *record = &uwamonoRecords[uwamonoCurrentIndex];
    float form = record->form;
    UwamonoKeyRange *keyRange;
    int wordsLeft;

    if (form == UWAMONO_FORM_ES) {
        if (uwamonoPartIndices[0] != -1) {
            uwamonoRecords[uwamonoPartIndices[0]].actor->drawFlags &= ~UWAMONO_DRAW_HIDDEN;
            uwamonoRecords[uwamonoPartIndices[0]].actor->drawMode = 1;
            XenoActorAttachChildToJoint(record->actor, uwamonoRecords[uwamonoPartIndices[0]].actor, UWAMONO_PART_JOINT_ES_0);
        }
        if (uwamonoPartIndices[1] != -1) {
            uwamonoRecords[uwamonoPartIndices[1]].actor->drawFlags |= UWAMONO_DRAW_HIDDEN;
            uwamonoRecords[uwamonoPartIndices[1]].actor->drawMode = 1;
            XenoActorAttachChildToJoint(record->actor, uwamonoRecords[uwamonoPartIndices[1]].actor, UWAMONO_PART_JOINT_ES_1);
        }
    } else {
        if (uwamonoPartIndices[0] != -1) {
            uwamonoRecords[uwamonoPartIndices[0]].actor->drawFlags &= ~UWAMONO_DRAW_HIDDEN;
            XenoActorReleaseChild(record->actor, uwamonoRecords[uwamonoPartIndices[0]].actor);
            XenoActorAttachChildToJoint(record->actor, uwamonoRecords[uwamonoPartIndices[0]].actor, UWAMONO_PART_JOINT_WALK_0);
        }
        if (uwamonoPartIndices[1] != -1) {
            uwamonoRecords[uwamonoPartIndices[1]].actor->drawFlags &= ~UWAMONO_DRAW_HIDDEN;
            XenoActorReleaseChild(record->actor, uwamonoRecords[uwamonoPartIndices[1]].actor);
            XenoActorAttachChildToJoint(record->actor, uwamonoRecords[uwamonoPartIndices[1]].actor, UWAMONO_PART_JOINT_WALK_1);
        }
    }
    for (wordsLeft = 153, keyRange = (UwamonoKeyRange *)uwamonoKeyRangeTable; wordsLeft >= 0; wordsLeft -= 17, keyRange++) {
        if (keyRange->lo <= record->key && record->key <= keyRange->hi) {
            func_0028B2F0(record->actor, keyRange->value);
        }
    }
}

/* Key ranges: 10 records of 17 words each; no symbol recovered yet. */
enum UwamonoKeyField {
    KEY_LO = 0,
    KEY_HI = 1,
    KEY_COLOR_A = 2,    /* 3 words: copied to D_0069B998 */
    KEY_COLOR_B = 5,    /* 3 words: copied to D_0069B9A0 */
    KEY_MAPPED = 8,     /* the id the block-A search maps a key to */
    KEY_VALUE = 9,
    KEY_LEVEL_B = 10,   /* 3 words: copied to D_0069B9B0 */
    KEY_LEVEL_A = 13,   /* 3 words: copied to D_0069B9A8 */
    KEY_EFFECT = 16,
    KEY_RANGE_WORDS = 17
};
extern unsigned int uwamonoAttachIdTable[32];
extern int uwamonoPartIndices[];
extern short D_0069B992;
extern short D_0069B9A8[3];
extern short D_0069B9B0[3];
extern short uwamonoLadderState;
extern unsigned char D_0069B998[3];
extern unsigned char D_0069B9A0[3];
void resUpdatePathSlot(int slot);
void resPathSlotLoadID(int slot, int id, int value);
int uwamonoCreateRecord(int slot, int kind, int key, int a, int b);
void uwamonoReleaseRecord(int index);
void seFootBanksLoad(void);
void func_0028B2F0(struct XenoActor *actor, int value);
void XenoActorAllocMatrices(struct XenoActor *actor);
int sceCdSync(int mode);
void uwamonoAttachPlayerParts(void);

/* Changes the player model (key = request without the high bit; UWAMONO_KEY_FROM_CURRENT takes the key mapped from the current record): loads
   the model, creates or updates the player record (kind 4), re-creates its part records, copies the colors and levels of the key range,
   starts its effect and sets the ES or walking scale values. */
void uwamonoChangePlayerModel(int request)
{
    int key = request & UWAMONO_KEY_MASK;
    int partCount = 0;
    int slot;
    int currentIndex;
    int playerIndex;
    int i;
    int n;
    int j;
    int m;
    int q;
    int recordIndex;
    int *modelParams;
    unsigned short effectId;
    UwamonoObject *record;

    while (xglCdServiceIfPending() != 0) {
    }
    if (key == UWAMONO_KEY_FROM_CURRENT) {
        int recKey = uwamonoRecords[uwamonoCurrentIndex].key;

        for (i = 0; i < 10 * KEY_RANGE_WORDS; i += KEY_RANGE_WORDS) {
            if (recKey < uwamonoKeyRangeTable[i + KEY_LO] || uwamonoKeyRangeTable[i + KEY_HI] < recKey) {
                continue;
            }
            key = uwamonoKeyRangeTable[i + KEY_MAPPED];
            break;
        }
    }
    uwamonoCurrentIndex = -1;
    uwamonoPartIndices[0] = -1;
    uwamonoPartIndices[1] = -1;
    for (recordIndex = 0; recordIndex < UWAMONO_RECORD_COUNT; recordIndex++) {
        if (uwamonoRecords[recordIndex].kind == UWAMONO_KIND_PLAYER) {
            uwamonoCurrentIndex = recordIndex;
        }
        if (uwamonoRecords[recordIndex].kind == UWAMONO_KIND_PART && partCount < UWAMONO_ATTACH_PARTS) {
            uwamonoPartIndices[partCount] = recordIndex;
            partCount++;
        }
    }
    slot = resPathSlotFindByOwner(key);
    if (slot < 0) {
        resUpdatePathSlot(0);
        slot = 0;
    }
    currentIndex = uwamonoCurrentIndex;
    if (currentIndex == -1) {
        resPathSlotLoadID(slot, key, 0);
        playerIndex = uwamonoCreateRecord(0, UWAMONO_KIND_PLAYER, key, -1, -1);
        uwamonoCurrentIndex = playerIndex;
        record = &uwamonoRecords[playerIndex];
    } else {
        record = &uwamonoRecords[currentIndex];
        record->flags |= 0x2;
        {
            struct XenoActor *actor = record->actor;

            if (request == actor->kind) {
                return;
            }
            resPathSlotLoadID(slot, key, (int)&modelParams);
            {
                int *values = modelParams;

                actor->rsrc[XAct_Rsrc_MDL] = values[0];
                actor->rsrc[XAct_Rsrc_TEX] = values[1];
                actor->rsrc[XAct_Rsrc_XHR] = values[2];
                actor->rsrc[XAct_Rsrc_XAP] = values[4];
                actor->lookLink = values[3];
            }
            XenoActorAllocMatrices(actor);
            XenoActorSetChannelValue(actor, 4.0f);
            XenoActorSetMode(actor, 0);
            record->actor = actor;
            actor->kind = key;
            record->key = key;
        }
    }

    uwamonoLadderState = 0;
    record->flags |= 0x2;
    record->actor->probeRadius = D_0069AEC8;
    record->formSet = 1;
    record->actor->radius = D_0069AEC8;
    record->extent.x = D_0069AEC8;
    record->actor->probeEnabled = 1;
    if ((unsigned int)(key - UWAMONO_ES_KEY_FIRST) < UWAMONO_ES_KEY_COUNT) {
        record->form = UWAMONO_FORM_ES;
    } else {
        record->form = 5.0f;
    }

    gameWork.control &= ~CONTROL_SCRIPTED;
    gameWork.field_2d8 = 6.0f;
    gameWork.field_2e0 = D_0069AECC;
    gameWork.field_2f8 = 1.0f;
    gameWork.field_2f0 = 1.0f;
    gameWork.field_2e8 = 1.0f;
    gameWork.field_300 = 15;

    for (i = 0; i < UWAMONO_ATTACH_PARTS; i++) {
        if (uwamonoPartIndices[i] != -1) {
            uwamonoReleaseRecord(uwamonoPartIndices[i]);
            uwamonoPartIndices[i] = -1;
        }
    }

    for (i = 0; i < UWAMONO_ATTACH_TABLE_WORDS; i += UWAMONO_ATTACH_ENTRY_WORDS) {
        if (key < (int)uwamonoAttachIdTable[i] || (int)uwamonoAttachIdTable[i + 1] < key) {
            continue;
        }
        for (j = 0; j < UWAMONO_ATTACH_PARTS; j++) {
            int id = (int)uwamonoAttachIdTable[i + 2 + j];

            if (id != -1) {
                resPathSlotLoadID(slot, id, 0);
                uwamonoPartIndices[j] = uwamonoCreateRecord(j + 1, UWAMONO_KIND_PART, id, -1, -1);
            }
        }
        break;
    }

    uwamonoAttachPlayerParts();
    D_0069B998[0] = D_0069B998[1] = D_0069B998[2] = 0xFF;

    for (i = 0; i < 10 * KEY_RANGE_WORDS; i += KEY_RANGE_WORDS) {
        int effectFlags;

        if (uwamonoKeyRangeTable[i + KEY_LO] > key || uwamonoKeyRangeTable[i + KEY_HI] < key) {
            continue;
        }
        D_0069B998[0] = uwamonoKeyRangeTable[i + KEY_COLOR_A + 0];
        D_0069B998[1] = uwamonoKeyRangeTable[i + KEY_COLOR_A + 1];
        D_0069B998[2] = uwamonoKeyRangeTable[i + KEY_COLOR_A + 2];
        D_0069B9A0[0] = uwamonoKeyRangeTable[i + KEY_COLOR_B + 0];
        D_0069B9A0[1] = uwamonoKeyRangeTable[i + KEY_COLOR_B + 1];
        D_0069B9A0[2] = uwamonoKeyRangeTable[i + KEY_COLOR_B + 2];
        D_0069B9B0[0] = uwamonoKeyRangeTable[i + KEY_LEVEL_B + 0];
        D_0069B9B0[1] = uwamonoKeyRangeTable[i + KEY_LEVEL_B + 1];
        D_0069B9B0[2] = uwamonoKeyRangeTable[i + KEY_LEVEL_B + 2];
        D_0069B9A8[0] = uwamonoKeyRangeTable[i + KEY_LEVEL_A + 0];
        D_0069B9A8[1] = uwamonoKeyRangeTable[i + KEY_LEVEL_A + 1];
        D_0069B9A8[2] = uwamonoKeyRangeTable[i + KEY_LEVEL_A + 2];
        func_0028B2F0(record->actor, uwamonoKeyRangeTable[i + KEY_VALUE]);
        effectId = uwamonoKeyRangeTable[i + KEY_EFFECT];
        effectFlags = 0x10000001;
        D_0069B992 = (short)effectId;
        sefStartEffectKeyDefault(0, (((int)((unsigned int)effectId << 16)) >> 4) | effectFlags);
        sceCdSync(0);
    }

    if (record->form == UWAMONO_FORM_ES) {
        gameWork.frameScale = D_0069AED4;
        record->actor->probeRadius = D_0069AED0;
        record->actor->radius = D_0069AED0;
        record->extent.x = D_0069AED0;
        record->scale.z = 1.0f;
        record->scale.y = 1.0f;
        record->scale.x = 1.0f;
    } else {
        gameWork.frameScale = D_0069AEDC;
        record->actor->probeRadius = 4.0f;
        record->actor->radius = 4.0f;
        record->extent.x = 4.0f;
        record->scale.z = D_0069AED8;
        record->scale.y = D_0069AED8;
        record->scale.x = D_0069AED8;
        gameWork.field_2d8 = 20.0f;
        gameWork.field_2e0 = 1.0f;
    }
    uwamonoReleaseHandles();
    seFootBanksLoad();
}

#define NULL ((void *) 0)

/* One entry of elvSlopeTable. */
typedef struct UwamonoSlope {
    float x;
    float z;
    float push;
    float speed;
} UwamonoSlope;

extern int D_0069B978;
extern int D_0069B97C;
extern int D_0069B980;
extern short D_0069B992;
extern signed char D_0069B9B6;
static float D_0069E184;
static int D_0069E188;
extern PadRecord PadData;
extern UwamonoSlope elvSlopeTable[];
extern float D_002B9C98[];
extern unsigned char D_002B9CA8[];
extern UwamonoProbe uwamonoProbeTemplate;

/* Per jump height step (UwamonoSearch.level): the frames before the landing
 * check (D_0069B978) and before the target action (D_0069B97C). */
short D_0069B9A8[3] = { -1, -1, -1 };

short D_0069B9B0[3] = { -1, -1, -1 };

int uwamonoUpdateLadderState(void);
void XenoActorHidePart(struct XenoActor *actor, const char *part);
void XenoActorShowPart(struct XenoActor *actor, const char *part);
int uwamonoFindSelected(void);
void uwamonoSendEvent(int kind, int id, int value);
void uwamonoTriggerRecord(int index);
void uwamonoReleaseHandles(void);
float atan2f(float y, float x);
float func_002A1B30(float value);
float Get_Decimal_Surplus_for_Radius(float angle);
float xglSin(float angle);
float xglCos(float angle);
void XenoActorCheckMode(struct XenoActor *actor, int mode);
void XenoActorSetMode(struct XenoActor *actor, int mode);
int uwamonoFindTalkTarget(struct XenoActor *actor, float range);
unsigned int msgWindowInUse(void);
void talkSetResult(int id);
void mapObjectSetActive(UwamonoObject *object, UwamonoObject *target);
void uwamonoSetPairMode(int index, int value);
void uwamonoStartPairMove(int index, int kind, int frames);
void uwamonoNoticePlayer(int index, AlignedVector4 *position);
void uwamonoMoveSelection(int step);
float Get_Angle(const Vector4 *first, const Vector4 *second);
float cursolGetByReduceSpeedAngleLoop(float current, float target, float speed);
int Get_Rnd(int min, int max);
int sefSpawnEffectRecordById(int id, int arg1, int arg2);
void sefSetRecordOwner(int record, struct XenoActor *owner);
void sefSetRecordValues(int record, struct XenoActor *first, struct XenoActor *second, int value);
void func_00208B50(int value);
void elvGetPosition(int index, int arg, Vector4 *position);
void uwamonoHitCheckWithNyuru(struct XenoActor *actor);
/* The first record other than owner (a talk or player record, not blocked) whose radius sphere, scaled by reachScale,
   reaches point (the owner position when null), or -1. */ int uwamonoFindOverlapRecord(int owner, AlignedVector4 *point, float reachScale);
int uwamonoPushOutOfActor(int index, struct XenoActor *actor);
void uwamonoPickTarget(short index);
void uwamonoAnyEngaged(void);
void uwamonoSelectTarget(int index);
void func_0029E690(void);

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define UWAMONO_IS_ES(actor) ((unsigned int) ((actor)->kind - 0x10000) < 0x4000)
/* Stick directions snap to 16 sectors: pi/8 per sector, offset by half a sector. */
#define EIGHTH_TURN 0.39269909f
#define EIGHTH_TURN_HALF 0.19634955f
#define QUARTER_TURN_FLOAT 0.78539819f /* pi/4 as the original float constant */
/* Draw flags of a XenoActor (drawFlags): UWAMONO_DRAW_HIDDEN is defined at the top of the file. */
#define UWAMONO_DRAW_TRANSLUCENT 0x08000000
/* Record keys: the ES range of the party starts at 0x10000; 0x10026 is Jin's ES. */
#define UWAMONO_KEY_JIN_ES 0x10026
#define UWAMONO_KEY_IN(key, first, count) ((unsigned int) ((key) - (first)) < (count))
/* Ground status classes of XenoActor.status. */
#define GROUND_SLOPE 0x10000
#define GROUND_LADDER 0x100
#define GROUND_ELEVATOR 0x200

/* Candidate target points and their reach (10 entries). */
extern Vector4 D_0033EE00[];
extern float D_0033EE90[];
extern int gameLoopFlags[];
float Get_Distance(const Vector4 *from, const Vector4 *to);
int uwamonoTryClearTarget(int index, signed char threshold);
int uwamonoFindNearbyTarget(int index);
void uwamonoUpdateTargetVector(int index);

/*
 * Picks the target of a map object: the nearest point in reach first, then
 * the script target, then a nearby object that can be targeted.
 */
void uwamonoPickTarget(short index)
{
    UwamonoMotionTable *motions = uwamonoRecords[index].motions;
    float nearest = 1000.0f;
    short found = -1;
    int scriptTarget;
    int nearby;
    int i;

    for (i = 0; i < 10; i++) {
        float distance = Get_Distance(&D_0033EE00[i], &uwamonoRecords[index].position);

        if (D_0033EE90[i] != 0.0f && distance <= D_0033EE90[i] && distance < nearest) {
            nearest = distance;
            found = i;
        }
    }
    if (found != -1) {
        uwamonoOfferTarget(index, 2, -1, &D_0033EE00[found]);
        uwamonoUpdateTargetVector(index);
        return;
    }
    uwamonoTryClearTarget(index, 2);
    if (motions->targetPriority == 3) {
        uwamonoUpdateTargetVector(index);
        return;
    }
    if (gameLoopFlags[0] & CONTROL_SCRIPTED) {
        uwamonoTryClearTarget(index, 0);
        return;
    }
    scriptTarget = uwamonoFindSelected();
    if (scriptTarget != -1) {
        uwamonoOfferTarget(index, 0, scriptTarget, NULL);
        uwamonoUpdateTargetVector(index);
        return;
    }
    uwamonoTryClearTarget(index, 0);
    nearby = uwamonoFindNearbyTarget(index);
    if (nearby == -1) {
        uwamonoTryClearTarget(index, 1);
        return;
    }
    if (uwamonoRecords[nearby].flags & 2) {
        uwamonoOfferTarget(index, 1, nearby, NULL);
        uwamonoUpdateTargetVector(index);
        return;
    }
    /* The original tests the result here: the call is a jal + jr, not a tail call. */
    if (uwamonoTryClearTarget(index, 1)) {
        return;
    }
}

/*
 * Per-frame control of the player's map object: rider fade, input, walk
 * motions, talk/search targets, jumps, ground status and the move probe, then
 * copies the result to the record and to the riders.
 */
void uwamonoPlayerUpdate(void)
{
    /* record and the indexed uwamonoRecords[uwamonoCurrentIndex] reads below are the same object; the mix
       is kept for the original code generation. */
    UwamonoObject *record = &uwamonoRecords[uwamonoCurrentIndex];
    struct XenoActor *actor = uwamonoRecords[uwamonoCurrentIndex].actor;
    struct XenoActor *riders[2];
    Vector4 unused; /* Kept: the original builds this (0, 0, 0, 1) vector and never reads it. */
    Vector4 elevator;
    AlignedVector4 old;
    float speed;
    float frameScale;
    int i;
    int riding;
    int moving;
    int walk;
    int target;
    int stick_x;
    int stick_y;
    unsigned int status;

    if (uwamonoPartIndices[0] != -1) {
        riders[0] = uwamonoRecords[uwamonoPartIndices[0]].actor;
    } else {
        riders[0] = NULL;
    }
    if (uwamonoPartIndices[1] != -1) {
        riders[1] = uwamonoRecords[uwamonoPartIndices[1]].actor;
    } else {
        riders[1] = NULL;
    }
    actor->previous.vector.x = actor->position.x;
    actor->previous.vector.y = actor->position.y;
    actor->previous.vector.z = actor->position.z;
    actor->previous.vector.w = actor->position.w;
    actor->moved = 0;
    for (i = 0; i < 2; i++) {
        if (riders[i] != NULL) {
            riders[i]->moved = 0;
            riders[i]->radius = actor->radius;
            riders[i]->previous.vector.x = actor->position.x;
            riders[i]->previous.vector.y = actor->position.y;
            riders[i]->previous.vector.z = actor->position.z;
            riders[i]->previous.vector.w = actor->position.w;
        }
    }
    if (gameWork.control & CONTROL_PAUSED) {
        return;
    }
    if (xglStudioGetCamera2(0)->state != 4) {
        return;
    }
    if (gameWork.fadeTimer != 0) {
        if (--gameWork.fadeTimer <= 0) {
            record->fade = 0.0f;
            gameWork.fadeTimer = 0;
            actor->alpha = 1.0f;
            actor->drawFlags &= ~UWAMONO_DRAW_TRANSLUCENT;
            if (riders[0] != NULL) {
                riders[0]->alpha = 1.0f;
                riders[0]->drawFlags &= ~UWAMONO_DRAW_TRANSLUCENT;
            }
            if (riders[1] != NULL) {
                riders[1]->alpha = 1.0f;
                riders[1]->drawFlags &= ~UWAMONO_DRAW_TRANSLUCENT;
            }
        } else if (gameWork.fadeTimer % 2 == 0 || gameWork.fadeTimer >= 23) {
            actor->drawFlags |= UWAMONO_DRAW_TRANSLUCENT;
            actor->alpha = (100.0f - record->fade) / 100.0f;
            if (riders[0] != NULL) {
                riders[0]->drawFlags |= UWAMONO_DRAW_TRANSLUCENT;
                riders[0]->alpha = (100.0f - record->fade) / 100.0f;
            }
            if (riders[1] != NULL) {
                riders[1]->drawFlags |= UWAMONO_DRAW_TRANSLUCENT;
                riders[1]->alpha = (100.0f - record->fade) / 100.0f;
            }
        } else {
            actor->alpha = 1.0f;
            actor->drawFlags &= ~UWAMONO_DRAW_TRANSLUCENT;
            if (riders[0] != NULL) {
                riders[0]->alpha = 1.0f;
                riders[0]->drawFlags &= ~UWAMONO_DRAW_TRANSLUCENT;
            }
            if (riders[1] != NULL) {
                riders[1]->alpha = 1.0f;
                riders[1]->drawFlags &= ~UWAMONO_DRAW_TRANSLUCENT;
            }
        }
    }
    if (uwamonoUpdateLadderState() == 0) {
        speed = 0.0f;
        frameScale = gameWork.frameScale;
        if (D_0069B978 >= 0 && record->key == UWAMONO_KEY_JIN_ES) {
            if (actor->mode == 5 && actor->motion[0].frame >= 18.0f) {
                /* The original calls it twice in a row (and ShowPart below the same). */
                XenoActorHidePart(riders[0], "jin_saya_tuka");
                XenoActorHidePart(riders[0], "jin_saya_tuka");
            }
            if (record->key == UWAMONO_KEY_JIN_ES && actor->mode == 5 && actor->motion[0].frame >= 50.0f) {
                XenoActorShowPart(riders[0], "jin_saya_tuka");
                XenoActorShowPart(riders[0], "jin_saya_tuka");
            }
        }
        if (D_0069B97C != -1 && --D_0069B97C == -1 && D_0069B980 < 6 && D_0069B980 >= 3) {
            target = uwamonoFindSelected();
            if (target != -1) {
                uwamonoSendEvent(1, uwamonoRecords[target].id, 0);
                if (uwamonoRecords[target].flags & 0x40000) {
                    uwamonoTriggerRecord(target);
                    uwamonoReleaseHandles();
                }
            }
        }
        if (D_0069B978 >= 0) {
            D_0069B978--;
        }
        /* "Wait %3d" */
        xglFontDebugPrintf(8, 0x40, "\xa5\xa6\xa5\xa7\xa5\xa4\xa5\xc8 %3d", D_0069B978);
        if (D_0069B978 == -1) {
            D_0069B978 = 0;
            if (D_0069B980 < 6) {
                if (D_0069B980 >= 3) {
                    XenoActorSetChannelValueB(actor, 4.0f);
                    XenoActorSetSelector(actor, 0);
                    gameWork.control &= ~CONTROL_SEARCHING;
                }
            }
            D_0069B980 = -1;
            if (UWAMONO_IS_ES(actor)) {
                /* do/while(0) with break: kept for the original branch layout (same in the blocks below). */
                do {
                    if (UWAMONO_KEY_IN(record->key, 0x10025, 7)) {
                        if (riders[0] != NULL) {
                            riders[0]->drawFlags &= ~UWAMONO_DRAW_HIDDEN;
                        }
                        if (riders[1] != NULL) {
                            riders[1]->drawFlags |= UWAMONO_DRAW_HIDDEN;
                        }
                        break;
                    }
                    if (UWAMONO_KEY_IN(record->key, 0x10015, 6)) {
                        if (riders[0] != NULL) {
                            riders[0]->drawFlags &= ~UWAMONO_DRAW_HIDDEN;
                        }
                        if (riders[1] != NULL) {
                            riders[1]->drawFlags &= ~UWAMONO_DRAW_HIDDEN;
                        }
                        break;
                    }
                    if (riders[0] != NULL) {
                        riders[0]->drawFlags |= UWAMONO_DRAW_HIDDEN;
                    }
                    if (riders[1] != NULL) {
                        riders[1]->drawFlags |= UWAMONO_DRAW_HIDDEN;
                    }
                } while (0);
            }
            if (!(gameWork.control & CONTROL_SCRIPTED) || (gameWork.control & CONTROL_REPLAYED)) {
                float x;
                float y;
                float angle;
                float step;
                int pressure;
                int digital;

                if (gameWork.control & CONTROL_REPLAYED) {
                    stick_x = gameWork.replayStickX;
                    stick_y = gameWork.replayStickY;
                    actor->status = gameWork.replayStatus;
                } else {
                    /* D-pad pressure and the pad's digital direction (scaled to the same range): the one with
                       the larger magnitude wins, opposite signs cancel. */
                    pressure = PadData.horizontalPressure;
                    digital = PadData.directionX * 2;
                    if (pressure != 0 && digital != 0 && (pressure ^ digital) < 0) {
                        stick_x = 0;
                    } else if (pressure < 0 || digital < 0) {
                        stick_x = MIN(pressure, digital);
                    } else {
                        stick_x = MAX(pressure, digital);
                    }
                    pressure = PadData.verticalPressure;
                    digital = PadData.directionY * 2;
                    if (pressure != 0 && digital != 0 && (pressure ^ digital) < 0) {
                        stick_y = 0;
                    } else if (pressure < 0 || digital < 0) {
                        stick_y = MIN(pressure, digital);
                    } else {
                        stick_y = MAX(pressure, digital);
                    }
                    gameWork.replayStickX = stick_x;
                    gameWork.replayStickY = stick_y;
                    gameWork.replayStatus = actor->status;
                }
                if (stick_x == 0 && stick_y == 0) {
                    D_0069E184 = gameWork.stickAngle;
                }
                x = I2F(stick_x);
                y = I2F(stick_y);
                speed = __builtin_sqrtf(x * x + y * y);
                if (speed > 256.0f) {
                    speed = 256.0f;
                }
                angle = Get_Decimal_Surplus_for_Radius(
                    func_002A1B30((atan2f(x, y) + EIGHTH_TURN_HALF) / EIGHTH_TURN) * EIGHTH_TURN + D_0069E184);
                if (speed > 0.0f) {
                    actor->direction.y = angle;
                }
                if ((PadData.held & PAD_R2) && speed > 191.0f) {
                    speed = 191.0f;
                }
                step = speed * frameScale;
                actor->velocity.x += xglSin(actor->direction.y) * step;
                actor->velocity.z += xglCos(actor->direction.y) * step;
            }
            if (!(gameWork.control & CONTROL_SCRIPTED)) {
                do {
                    if (speed <= 0.0f) {
                        actor->motion[0].speed = 1.0f;
                        walk = 0;
                        break;
                    }
                    if (speed < 192.0f) {
                        actor->motion[0].speed = speed * 2.6f / 192.0f;
                        walk = 1;
                        break;
                    }
                    walk = 2;
                    actor->motion[0].speed = speed * (1.0f / 256.0f);
                } while (0);
                if (actor->mode != walk && (walk != 0 || actor->mode != 3) && (walk != 0 || actor->mode != 4)
                    && (walk != 0 || actor->mode != 5)) {
                    actor->flags &= ~2;
                    if (D_0069B9B6 != 0) {
                        XenoActorEnableChannel(actor);
                    }
                    XenoActorCheckMode(actor, uwamonoRecords[uwamonoCurrentIndex].motions->walk[walk]);
                    XenoActorSetChannelValue(actor, 4.0f);
                    /* moving and riding are kept as locals: the original goes through them for its code generation. */
                    moving = 1;
                    riding = UWAMONO_IS_ES(actor);
                    D_0069B9B6 = moving;
                    if (riding) {
                        for (i = 0; i < 2; i++) {
                            if (riders[i] != NULL) {
                                XenoActorSetChannelValue(riders[i], 4.0f);
                                XenoActorEnableChannel(riders[i]);
                                XenoActorSetMode(riders[i], walk);
                            }
                        }
                    }
                }
                target = uwamonoFindTalkTarget(actor, 50.0f);
                /* "Talk target -> %8d" */
                xglFontDebugPrintf(8, 0x20, "\xa5\xc8\xa1\xbc\xa5\xaf\xc2\xd0\xbe\xdd \xa2\xaa %8d", target);
                if (target != -1) {
                    if (PadData.pressed & PAD_CIRCLE) {
                        UwamonoObject *object = &uwamonoRecords[target];

                        switch (object->kind) {
                        case 2:
                            if (UWAMONO_IS_ES(actor)) {
                                if (uwamonoRecords[target].searchState == 0) {
                                    uwamonoSetPairMode(target, 0);
                                }
                            } else if (uwamonoRecords[target].searchState == 2) {
                                uwamonoSetPairMode(target, 0);
                            }
                            uwamonoStartPairMove(target, 5, 30);
                            break;
                        case 1: {
                            struct XenoActor *other = uwamonoRecords[target].actor;

                            if (UWAMONO_KEY_IN(other->kind, 0x10000, 0x8000) && msgWindowInUse() == 0) {
                                talkSetResult(object->id);
                                mapObjectSetActive(record, object);
                                XenoActorEnableChannel(record->actor);
                                XenoActorSetMode(record->actor, 0);
                            }
                            break;
                        }
                        }
                    } else if (uwamonoRecords[target].flags & 0x800) {
                        if (msgWindowInUse() == 0) {
                            talkSetResult(uwamonoRecords[target].id);
                        }
                    } else {
                        mapObjectSetActive(record, NULL);
                    }
                } else {
                    mapObjectSetActive(record, NULL);
                }
                if (walk == 2) {
                    uwamonoNoticePlayer(uwamonoCurrentIndex, &uwamonoRecords[uwamonoCurrentIndex].actor->position);
                }
                if (walk != 0) {
                    uwamonoMoveSelection(0);
                }
                if (!(gameWork.control & CONTROL_SCRIPTED) && !(gameWork.control & CONTROL_SEARCHING) && D_0069B97C == -1) {
                    if (PadData.pressed & PAD_L1) {
                        uwamonoMoveSelection(1);
                    }
                    if (PadData.pressed & PAD_R1) {
                        uwamonoMoveSelection(-1);
                    }
                    if (PadData.pressed & PAD_SQUARE) {
                        int searched;
                        int jumpMode;

                        searched = uwamonoFindSelected();
                        if (searched != -1 && (uwamonoRecords[searched].flags & 0x20000)) {
                            gameWork.control |= CONTROL_SEARCHING;
                            uwamonoSendEvent(0, uwamonoRecords[searched].id, 0);
                            actor->direction.y = Get_Angle(&actor->position, &uwamonoRecords[searched].position);
                            actor->rotation.y = cursolGetByReduceSpeedAngleLoop(actor->rotation.y, actor->direction.y, 4.0f);
                            /* The smoothed value above is overwritten at once; the call stays for the original code. */
                            actor->rotation.y = Get_Angle(&actor->position, &uwamonoRecords[searched].position);
                            XenoActorSetChannelValue(actor, 0.0f);
                            D_0069B9B6 = 0;
                            XenoActorSetMode(actor, uwamonoRecords[searched].search.level + 3);
                            actor->requested_mode = 0;
                            D_0069B978 = D_0069B9A8[uwamonoRecords[searched].search.level];
                            D_0069B97C = D_0069B9B0[uwamonoRecords[searched].search.level];
                            if (UWAMONO_IS_ES(actor)) {
                                for (i = 0; i < 2; i++) {
                                    if (riders[i] != NULL) {
                                        XenoActorSetChannelValue(riders[i], 0.0f);
                                        XenoActorEnableChannel(riders[i]);
                                        XenoActorSetMode(riders[i], uwamonoRecords[searched].search.level + 3);
                                        riders[i]->flags |= 2;
                                    }
                                }
                                do {
                                    if (UWAMONO_KEY_IN(record->key, 0x10025, 7)) {
                                        if (riders[0] != NULL) {
                                            riders[0]->drawFlags &= ~UWAMONO_DRAW_HIDDEN;
                                        }
                                        if (riders[1] != NULL) {
                                            riders[1]->drawFlags &= ~UWAMONO_DRAW_HIDDEN;
                                        }
                                        break;
                                    }
                                    if (UWAMONO_KEY_IN(record->key, 0x10015, 6)) {
                                        if (riders[0] != NULL) {
                                            riders[0]->drawFlags &= ~UWAMONO_DRAW_HIDDEN;
                                        }
                                        if (riders[1] != NULL) {
                                            riders[1]->drawFlags &= ~UWAMONO_DRAW_HIDDEN;
                                        }
                                        break;
                                    }
                                    if (riders[0] != NULL) {
                                        riders[0]->drawFlags &= ~UWAMONO_DRAW_HIDDEN;
                                    }
                                    if (riders[1] != NULL) {
                                        riders[1]->drawFlags |= UWAMONO_DRAW_HIDDEN;
                                    }
                                } while (0);
                                seVoiceStartDefault(uwamonoRecords[searched].search.level * 3 + Get_Rnd(0, 2) + 0x07000001);
                                seVoiceStartDefault(uwamonoRecords[searched].search.level + 0x0700000B);
                                D_0069E188 = sefSpawnEffectRecordById((D_0069B992 << 12) | (uwamonoRecords[searched].search.level + 1) | 0x10000000, 0, 0);
                            } else if (uwamonoRecords[searched].search.level == 0) {
                                seVoiceStartDefault(0x0700000B);
                                D_0069E188 = sefSpawnEffectRecordById((D_0069B992 << 12) | 0x10000001, 0, 0);
                            } else {
                                seVoiceStartDefault(0x0700000C);
                                D_0069E188 = sefSpawnEffectRecordById((D_0069B992 << 12) | 0x10000002, 0, 0);
                            }
                            sefSetRecordOwner(D_0069E188, actor);
                            sefSetRecordValues(D_0069E188, riders[0], riders[1], 0);
                            /* jumpMode is kept as a local for the original code generation. */
                            jumpMode = uwamonoRecords[searched].search.level + 3;
                            actor->motion[0].speed = 1.0f;
                            D_0069B980 = jumpMode;
                            XenoActorSetChannelValueB(actor, 4.0f);
                            XenoActorSetSelector(actor, 5);
                            func_00208B50(4);
                        }
                    }
                }
            }
        }
        status = actor->status;
        if ((status & 0xFFFF0000) == GROUND_SLOPE) {
            float angle;
            float distance;
            unsigned int slope = status & 0xF0F;

            if (!(slope & 0xF00)) {
                float factor;

                if (slope & 8) {
                    UwamonoSlope *entry = &elvSlopeTable[slope & 7];
                    float push = frameScale * entry->push * 256.0f;

                    actor->velocity.x += entry->x * push;
                    actor->velocity.z += entry->z * push;
                    factor = entry->speed;
                } else {
                    factor = D_002B9C98[slope & 3];
                }
                angle = actor->direction.y;
                distance = frameScale * ((factor - 1.0f) * speed);
            } else {
                angle = I2F(D_002B9CA8[slope >> 8]) * QUARTER_TURN_FLOAT;
                distance = frameScale * D_002B9C98[slope & 3] * 256.0f;
            }
            actor->velocity.x += xglSin(angle) * distance;
            actor->velocity.z += xglCos(angle) * distance;
        }
        if ((actor->status & ~0xFF) == GROUND_ELEVATOR) {
            elvGetPosition(actor->status & 0xF, 0, &elevator);
            actor->velocity.x += elevator.x;
            actor->velocity.z += elevator.z;
        }
        if (!(gameWork.control & CONTROL_SCRIPTED)) {
            actor->rotation.y = cursolGetByReduceSpeedAngleLoop(actor->rotation.y, actor->direction.y, 4.0f);
        }
        actor->hitFlags = (actor->hitFlags & ~7) | 0x302;
        if (!(uwamonoRecords[uwamonoCurrentIndex].flags & 0x04000000)) {
            float height;

            old = actor->position;
            height = mapMoveProbe(&actor->position, &actor->velocity, actor->hitBuffer);
            actor->velocity.x = actor->position.x - old.x;
            actor->velocity.z = actor->position.z - old.z;
            if (height != -1000.0f) {
                if ((actor->groundFlags & 0x10) && actor->velocity.y != 0.0f) {
                    actor->velocity.y = -0.011f;
                }
                if (actor->position.y < height
                    || (actor->velocity.y == 0.0f && actor->position.y - actor->stepHeight < height)) {
                    actor->position.y = height;
                    actor->velocity.y = 0.0f;
                } else {
                    actor->position.y += actor->velocity.y;
                    actor->velocity.y += -0.011f;
                }
            } else {
                actor->velocity.x = 0.0f;
                actor->velocity.y = 0.0f;
                actor->velocity.z = 0.0f;
            }
        }
        uwamonoHitCheckWithNyuru(actor);
        target = uwamonoFindOverlapRecord(uwamonoCurrentIndex, 0, 1.0f);
        if (target != -1) {
            uwamonoPushOutOfActor(uwamonoCurrentIndex, uwamonoRecords[target].actor);
        }
        actor->velocity.x = actor->velocity.z = 0.0f;
        if (!(gameWork.control & 0x20) && (actor->status & ~0x1F) == GROUND_LADDER) {
            if (!(gameWork.control & 0x420)) {
                gameWork.control |= 0x400;
                uwamonoSendEvent(3, (actor->status & 0x1F) + 1, 0);
            }
            gameWork.control |= 0x400;
        } else {
            gameWork.control &= ~0x400;
        }
    } else {
        if (UWAMONO_IS_ES(actor)) {
            do {
                if (UWAMONO_KEY_IN(record->key, 0x10025, 7)) {
                    if (riders[0] != NULL) {
                        riders[0]->drawFlags &= ~UWAMONO_DRAW_HIDDEN;
                    }
                    if (riders[1] != NULL) {
                        riders[1]->drawFlags |= UWAMONO_DRAW_HIDDEN;
                    }
                    break;
                }
                if (UWAMONO_KEY_IN(record->key, 0x10015, 6)) {
                    if (riders[0] != NULL) {
                        riders[0]->drawFlags &= ~UWAMONO_DRAW_HIDDEN;
                    }
                    if (riders[1] != NULL) {
                        riders[1]->drawFlags &= ~UWAMONO_DRAW_HIDDEN;
                    }
                    break;
                }
                if (riders[0] != NULL) {
                    riders[0]->drawFlags |= UWAMONO_DRAW_HIDDEN;
                }
                if (riders[1] != NULL) {
                    riders[1]->drawFlags |= UWAMONO_DRAW_HIDDEN;
                }
            } while (0);
        }
    }
    mapProbeInit(&uwamonoProbeTemplate);
    uwamonoProbeTemplate.ground = actor->ground;
    mapMoveProbe(&actor->position, NULL, &uwamonoProbeTemplate);
    record->position = actor->position;
    record->rotation = actor->rotation;
    if (UWAMONO_IS_ES(actor)) {
        for (i = 0; i < 2; i++) {
            if (riders[i] != NULL) {
                riders[i]->position = actor->position;
                riders[i]->rotation = actor->rotation;
                riders[i]->direction = actor->direction;
                riders[i]->vector90 = actor->vector90;
                riders[i]->vector70 = actor->vector70;
                riders[i]->vector80 = actor->vector80;
                riders[i]->velocity = actor->velocity;
                riders[i]->radius = actor->radius;
            }
        }
    }
    unused.x = unused.y = unused.z = 0.0f;
    unused.w = 1.0f;
    uwamonoPickTarget(uwamonoCurrentIndex);
    uwamonoAnyEngaged();
    uwamonoSelectTarget(uwamonoCurrentIndex);
    func_0029E690();
}

float uwamonoRecordsOverlap(int owner, int other);

/* The first record other than owner (a talk or player record, not blocked) whose radius sphere, scaled by reachScale,
   reaches point (the owner position when null), or -1. */
int uwamonoFindOverlapRecord(int owner, AlignedVector4 *point, float reachScale)
{
    UwamonoObject *ownerRecord = &uwamonoRecords[owner];
    struct XenoActor *ownerActor = ownerRecord->actor;
    AlignedVector4 *from = point ? point : &ownerActor->position;
    UwamonoMotionTable *motions = ownerRecord->motions;
    short other;

    for (other = 0; other < UWAMONO_RECORD_COUNT; other++) {
        float dx;
        float dz;
        float dist;
        float limit;

        int motionBlocked;

        if (uwamonoRecords[other].kind != UWAMONO_KIND_TALK && uwamonoRecords[other].kind != UWAMONO_KIND_PLAYER) {
            continue;
        }
        motionBlocked = motions->flags & 1;
        if (motionBlocked && uwamonoRecords[other].kind == UWAMONO_KIND_PLAYER) {
            continue;
        }
        if (owner == other || uwamonoRecords[other].actor->radius == 0.0f || (uwamonoRecords[other].flags & 0x02000000) != 0) {
            continue;
        }
        if (uwamonoRecordsOverlap(owner, other) == 0.0f) {
            continue;
        }
        dx = from->x - uwamonoRecords[other].actor->position.x;
        dz = from->z - uwamonoRecords[other].actor->position.z;
        dist = __builtin_sqrtf(dx * dx + dz * dz);
        limit = (ownerActor->radius + uwamonoRecords[other].actor->radius) * reachScale;
        if (dist <= limit) {
            return other;
        }
    }
    return -1;
}

int uwamonoHitCheckRecord(struct XenoActor *actor);

/* Pushes the mover out to touch the obstacle's circle (by the sum of the radii, at the angle between them). Returns 1 when the new
   spot is free of other records and has floor, otherwise undoes the move and returns 0. */
int uwamonoPushOutOfActor(int moverIndex, struct XenoActor *obstacle)
{
    struct XenoActor *mover = uwamonoRecords[moverIndex].actor;
    AlignedVector4 *moverPosition = &mover->position;
    float angle = Get_Angle(moverPosition, &obstacle->position);
    float reach;
    AlignedVector4 goal;
    float groundHeight;

    moverPosition->x -= mover->velocity.x;
    mover->position.z -= mover->velocity.z;
    reach = mover->radius + obstacle->radius + 0.01f;
    goal.x = obstacle->position.x - reach * xglSin(angle);
    reach = reach * xglCos(angle);
    goal.z = obstacle->position.z - reach;
    mover->velocity.x = goal.x - mover->position.x;
    mover->velocity.z = goal.z - mover->position.z;
    mover->position.x += mover->velocity.x;
    mover->position.z += mover->velocity.z;
    if (uwamonoFindOverlapRecord(moverIndex, 0, 1.0f) != -1) {
        moverPosition->x -= mover->velocity.x;
        mover->position.z -= mover->velocity.z;
        return 0;
    }
    if (uwamonoHitCheckRecord(mover) != -1) {
        moverPosition->x -= mover->velocity.x;
        mover->position.z -= mover->velocity.z;
        return 0;
    }
    groundHeight = mapMoveProbe(moverPosition, 0, mover->hitBuffer);
    if (groundHeight == -1000.0f) {
        moverPosition->x -= mover->velocity.x;
        mover->position.z -= mover->velocity.z;
        return 0;
    }
    mover->position.y = groundHeight;
    return 1;
}

/* The record at uwamonoCurrentIndex, or null while no record is selected (index below zero). */
void *uwamonoCurrentRecord(void)
{
    if (!(uwamonoCurrentIndex >= 0)) {
        return 0;
    }
    return &uwamonoRecords[uwamonoCurrentIndex];
}

const char D_0069B9B8[8] = "jinH04";
const char D_0069B9C0[8] = "jinH03";
const char D_0069B9C8[8] = "jrH04";
const char D_0069B9D0[8] = "jrH02";
const char D_0069B9D8[8] = "jinH01";
const char D_0069B9E0[8] = "jin02";
const char D_0069B9E8[8] = "jin01";
const char D_0069B9F0[8] = "jin00";
const char D_0069B9F8[8] = "jinH00";
const char D_0069BA00[8] = "ziggy01";
const char D_0069BA08[8] = "ziggy00";
const char D_0069BA10[8] = "ziggy02";
const char D_0069BA18[8] = "mm_ym01";
const char D_0069BA20[8] = "momoH03";
const char D_0069BA28[8] = "momo02";
const char D_0069BA30[8] = "momo01";
const char D_0069BA38[8] = "momo00";
const char D_0069BA40[8] = "momoH00";
const char D_0069BA48[8] = "jrH01";
const char D_0069BA50[8] = "jr03";
const char D_0069BA58[8] = "jr02";
const char D_0069BA60[8] = "jr01";
const char D_0069BA68[8] = "jr00";
const char D_0069BA70[8] = "jrH00";
const char D_0069BA78[8] = "chaos03";
const char D_0069BA80[8] = "chaos02";
const char D_0069BA88[8] = "chaos01";
const char D_0069BA90[8] = "chaos00";
const char D_0069BA98[8] = "shion04";
const char D_0069BAA0[8] = "shion03";
const char D_0069BAA8[8] = "shion05";
const char D_0069BAB0[8] = "_";
const char D_0069BAB8[8] = "shion00";
const char D_0069BAC0[8] = "yuriH01";
const char D_0069BAC8[8] = "yuriH00";
const char D_0069BAD0[8] = "popeH00";
const char D_0069BAD8[8] = "bos001";
const char D_0069BAE0[8] = "bos013";
const char D_0069BAE8[8] = "canan01";
const char D_0069BAF0[8] = "tonny00";
const char D_0069BAF8[8] = "kirsh01";
const char D_0069BB00[8] = "kirsh00";
const char D_0069BB08[8] = "yuri01";
const char D_0069BB10[8] = "yuri00";
const char D_0069BB18[8] = "mary00";
const char D_0069BB20[8] = "kebin01";
const char D_0069BB28[8] = "kebin00";
const char D_0069BB30[8] = "pope00";
const char D_0069BB38[8] = "allen01";
const char D_0069BB40[8] = "umnm03";
const char D_0069BB48[8] = "du103";
const char D_0069BB50[8] = "gfgf03";
const char D_0069BB58[8] = "gfgm03";
const char D_0069BB60[8] = "vecf400";
const char D_0069BB68[8] = "vecm400";
const char D_0069BB70[8] = "umnf02";
const char D_0069BB78[8] = "umnm02";
const char D_0069BB80[8] = "umnfh00";
const char D_0069BB88[8] = "umnmh00";
const char D_0069BB90[8] = "umnf01";
const char D_0069BB98[8] = "umnm01";
const char D_0069BBA0[8] = "uso006";
const char D_0069BBA8[8] = "uso005";
const char D_0069BBB0[8] = "bos030";
const char D_0069BBB8[8] = "uso012";
const char D_0069BBC0[8] = "uso011";
const char D_0069BBC8[8] = "org00";
const char D_0069BBD0[8] = "thief04";
const char D_0069BBD8[8] = "thief03";
const char D_0069BBE0[8] = "thief02";
const char D_0069BBE8[8] = "thief01";
const char D_0069BBF0[8] = "thief00";
const char D_0069BBF8[8] = "bar00";
const char D_0069BC00[8] = "yurim00";
const char D_0069BC08[8] = "umnf00";
const char D_0069BC10[8] = "umnm00";
const char D_0069BC18[8] = "clean00";
const char D_0069BC20[8] = "bread00";
const char D_0069BC28[8] = "king00";
const char D_0069BC30[8] = "mouth00";
const char D_0069BC38[8] = "scott00";
const char D_0069BC40[8] = "engm00";
const char D_0069BC48[8] = "realf00";
const char D_0069BC50[8] = "realm00";
const char D_0069BC58[8] = "du202";
const char D_0069BC60[8] = "du201";
const char D_0069BC68[8] = "du102";
const char D_0069BC70[8] = "du101";
const char D_0069BC78[8] = "gfgf02";
const char D_0069BC80[8] = "gfgf01";
const char D_0069BC88[8] = "gfgm02";
const char D_0069BC90[8] = "gfgm01";
const char D_0069BC98[8] = "hyaku00";
const char D_0069BCA0[8] = "du200";
const char D_0069BCA8[8] = "du100";
const char D_0069BCB0[8] = "gomi00";
const char D_0069BCB8[8] = "water00";
const char D_0069BCC0[8] = "denki00";
const char D_0069BCC8[8] = "engm02";
const char D_0069BCD0[8] = "engm01";
const char D_0069BCD8[8] = "uticm00";
const char D_0069BCE0[8] = "urtvf00";
const char D_0069BCE8[8] = "urtvm00";
const char D_0069BCF0[8] = "gfgf00";
const char D_0069BCF8[8] = "gfgm00";
const char D_0069BD00[8] = "vecf300";
const char D_0069BD08[8] = "vecm300";
const char D_0069BD10[8] = "vecf200";
const char D_0069BD18[8] = "vecf100";
const char D_0069BD20[8] = "vecm200";
const char D_0069BD28[8] = "vecm100";
const char D_0069BD30[8] = "robo02";
const char D_0069BD38[8] = "robo01";
const char D_0069BD40[8] = "rat00";
const char D_0069BD48[8] = "dog00";
const char D_0069BD50[8] = "cat00";
const char D_0069BD58[8] = "cook200";
const char D_0069BD60[8] = "cook100";
const char D_0069BD68[8] = "spy00";
const char D_0069BD70[8] = "robo00";
const char D_0069BD78[8] = "ukun00";
const char D_0069BD80[8] = "oldf201";
const char D_0069BD88[8] = "oldf200";
const char D_0069BD90[8] = "oldf101";
const char D_0069BD98[8] = "oldf100";
const char D_0069BDA0[8] = "oldm201";
const char D_0069BDA8[8] = "oldm200";
const char D_0069BDB0[8] = "oldm101";
const char D_0069BDB8[8] = "oldm100";
const char D_0069BDC0[8] = "man401";
const char D_0069BDC8[8] = "man400";
const char D_0069BDD0[8] = "man301";
const char D_0069BDD8[8] = "man300";
const char D_0069BDE0[8] = "man201";
const char D_0069BDE8[8] = "man200";
const char D_0069BDF0[8] = "man101";
const char D_0069BDF8[8] = "man100";
const char D_0069BE00[8] = "kidf300";
const char D_0069BE08[8] = "kidm300";
const char D_0069BE10[8] = "girl501";
const char D_0069BE18[8] = "girl500";
const char D_0069BE20[8] = "girl401";
const char D_0069BE28[8] = "girl400";
const char D_0069BE30[8] = "girl301";
const char D_0069BE38[8] = "girl300";
const char D_0069BE40[8] = "girl201";
const char D_0069BE48[8] = "girl200";
const char D_0069BE50[8] = "girl101";
const char D_0069BE58[8] = "girl100";
const char D_0069BE60[8] = "boy601";
const char D_0069BE68[8] = "boy600";
const char D_0069BE70[8] = "boy501";
const char D_0069BE78[8] = "boy500";
const char D_0069BE80[8] = "boy401";
const char D_0069BE88[8] = "boy400";
const char D_0069BE90[8] = "boy301";
const char D_0069BE98[8] = "boy300";
const char D_0069BEA0[8] = "boy201";
const char D_0069BEA8[8] = "boy200";
const char D_0069BEB0[8] = "boy101";
const char D_0069BEB8[8] = "boy100";
const char D_0069BEC0[8] = "kidf201";
const char D_0069BEC8[8] = "kidf200";
const char D_0069BED0[8] = "kidf101";
const char D_0069BED8[8] = "kidf100";
const char D_0069BEE0[8] = "kidm201";
const char D_0069BEE8[8] = "kidm200";
const char D_0069BEF0[8] = "kidm101";
const char D_0069BEF8[8] = "kidm100";
const char D_0069BF00[8] = "pcr01b";
const char D_0069BF08[8] = "pcr05b";
const char D_0069BF10[8] = "pcr02d";
const char D_0069BF18[8] = "pcr02c";
const char D_0069BF20[8] = "pcr02b";
const char D_0069BF28[8] = "pcr05";
const char D_0069BF30[8] = "pcr03";
const char D_0069BF38[8] = "pcr02a";
const char D_0069BF40[8] = "pcr01a";
const char D_0069BF48[8] = "npcr12b";
const char D_0069BF50[8] = "npcr12a";
const char D_0069BF58[8] = "npcr09b";
const char D_0069BF60[8] = "npcr19";
const char D_0069BF68[8] = "npcr18";
const char D_0069BF70[8] = "npcr17";
const char D_0069BF78[8] = "npcr16";
const char D_0069BF80[8] = "npcr15";
const char D_0069BF88[8] = "npcr14";
const char D_0069BF90[8] = "npcr13";
const char D_0069BF98[8] = "npcr12";
const char D_0069BFA0[8] = "npcr11";
const char D_0069BFA8[8] = "npcr10";
const char D_0069BFB0[8] = "npcr09a";
const char D_0069BFB8[8] = "npcr08";
const char D_0069BFC0[8] = "npcr07";
const char D_0069BFC8[8] = "npcr06";
const char D_0069BFD0[8] = "npcr05";
const char D_0069BFD8[8] = "npcr04";
const char D_0069BFE0[8] = "npcr03";
const char D_0069BFE8[8] = "npcr02b";
const char D_0069BFF0[8] = "npcr02a";
const char D_0069BFF8[8] = "npcr01";
const char D_0069C000[8] = "uma013";
const char D_0069C008[8] = "gno019";
const char D_0069C010[8] = "gno018";
const char D_0069C018[8] = "gno017";
const char D_0069C020[8] = "gno016";
const char D_0069C028[8] = "gno015";
const char D_0069C030[8] = "gno014";
const char D_0069C038[8] = "gno013";
const char D_0069C040[8] = "gno012";
const char D_0069C048[8] = "gno011";
const char D_0069C050[8] = "gno010";
const char D_0069C058[8] = "gno009";
const char D_0069C060[8] = "gno008";
const char D_0069C068[8] = "gno007";
const char D_0069C070[8] = "gno006";
const char D_0069C078[8] = "gno005";
const char D_0069C080[8] = "gno004";
const char D_0069C088[8] = "gno003";
const char D_0069C090[8] = "gno002";
const char D_0069C098[8] = "gno001";
const char D_0069C0A0[8] = "uma011";
const char D_0069C0A8[8] = "cre017";
const char D_0069C0B0[8] = "cre016";
const char D_0069C0B8[8] = "cre015";
const char D_0069C0C0[8] = "uro015";
const char D_0069C0C8[8] = "uro014";
const char D_0069C0D0[8] = "uro013";
const char D_0069C0D8[8] = "uro012";
const char D_0069C0E0[8] = "uro011";
const char D_0069C0E8[8] = "uma012";
const char D_0069C0F0[8] = "cre014";
const char D_0069C0F8[8] = "cre013";
const char D_0069C100[8] = "uso010";
const char D_0069C108[8] = "uso009";
const char D_0069C110[8] = "uro010";
const char D_0069C118[8] = "uma010";
const char D_0069C120[8] = "uma009";
const char D_0069C128[8] = "uro009";
const char D_0069C130[8] = "uro008";
const char D_0069C138[8] = "cre018";
const char D_0069C140[8] = "cre012";
const char D_0069C148[8] = "cre011";
const char D_0069C150[8] = "uso008";
const char D_0069C158[8] = "uso007";
const char D_0069C160[8] = "uma008";
const char D_0069C168[8] = "uro007";
const char D_0069C170[8] = "uro006";
const char D_0069C178[8] = "uro005";
const char D_0069C180[8] = "uma007";
const char D_0069C188[8] = "uro004";
const char D_0069C190[8] = "uro003";
const char D_0069C198[8] = "vma003";
const char D_0069C1A0[8] = "vma002";
const char D_0069C1A8[8] = "vma001";
const char D_0069C1B0[8] = "cre010";
const char D_0069C1B8[8] = "cre009";
const char D_0069C1C0[8] = "cre008";
const char D_0069C1C8[8] = "cre007";
const char D_0069C1D0[8] = "cre006";
const char D_0069C1D8[8] = "cre005";
const char D_0069C1E0[8] = "cre004";
const char D_0069C1E8[8] = "cre003";
const char D_0069C1F0[8] = "cre002";
const char D_0069C1F8[8] = "cre001";
const char D_0069C200[8] = "uma006";
const char D_0069C208[8] = "uma005";
const char D_0069C210[8] = "uma004";
const char D_0069C218[8] = "uro002";
const char D_0069C220[8] = "uma003";
const char D_0069C228[8] = "uso004";
const char D_0069C230[8] = "uso003";
const char D_0069C238[8] = "uma002";
const char D_0069C240[8] = "uma001";
const char D_0069C248[8] = "uro001";
const char D_0069C250[8] = "uso002";
const char D_0069C258[8] = "uso001";
const char D_0069C260[8] = "bos031";
const char D_0069C268[8] = "bos029";
const char D_0069C270[8] = "bos028";
const char D_0069C278[8] = "bos027";
const char D_0069C280[8] = "bos026";
const char D_0069C288[8] = "bos025";
const char D_0069C290[8] = "bos024";
const char D_0069C298[8] = "bos023";
const char D_0069C2A0[8] = "bos022";
const char D_0069C2A8[8] = "bos021";
const char D_0069C2B0[8] = "bos020";
const char D_0069C2B8[8] = "bos019";
const char D_0069C2C0[8] = "bos018";
const char D_0069C2C8[8] = "bos017";
const char D_0069C2D0[8] = "bos016";
const char D_0069C2D8[8] = "bos015";
const char D_0069C2E0[8] = "bos014";
const char D_0069C2E8[8] = "bos012";
const char D_0069C2F0[8] = "bos011";
const char D_0069C2F8[8] = "bos010";
const char D_0069C300[8] = "bos009";
const char D_0069C308[8] = "bos008";
const char D_0069C310[8] = "bos007";
const char D_0069C318[8] = "bos006";
const char D_0069C320[8] = "bos005";
const char D_0069C328[8] = "bos004";
const char D_0069C330[8] = "bos003";
const char D_0069C338[8] = "bos002";
const char D_0069C340[8] = "CFEVE22";
const char D_0069C348[8] = "CFEVE21";
const char D_0069C350[8] = "CFEVE20";
const char D_0069C358[8] = "CFEVE19";
const char D_0069C360[8] = "CFEVE18";
const char D_0069C368[8] = "CFEVE17";
const char D_0069C370[8] = "CFEVE16";
const char D_0069C378[8] = "CFEVE15";
const char D_0069C380[8] = "CFEVE14";
const char D_0069C388[8] = "CFEVE13";
const char D_0069C390[8] = "CFEVE12";
const char D_0069C398[8] = "CFEVE11";
const char D_0069C3A0[8] = "CFEVE10";
const char D_0069C3A8[8] = "CFEVE09";
const char D_0069C3B0[8] = "CFEVE08";
const char D_0069C3B8[8] = "CFEVE07";
const char D_0069C3C0[8] = "CFEVE06";
const char D_0069C3C8[8] = "CFEVE05";
const char D_0069C3D0[8] = "CFEVE04";
const char D_0069C3D8[8] = "CFEVE03";
const char D_0069C3E0[8] = "CFEVE02";
const char D_0069C3E8[8] = "CFEVE01";
const char D_0069C3F0[8] = "idtest";
const char D_0069C3F8[8] = "pcr04";
const char D_0069C400[8] = "pcr02";
const char D_0069C408[8] = "pcr01";
const char D_0069C410[8] = "pcr00";
const char D_0069C418[8] = "kanan00";
const char D_0069C420[8] = "motion\\";
const char D_0069C428[8] = "07";
const char D_0069C430[8] = "06";
const char D_0069C438[8] = "05";
const char D_0069C440[8] = "04";
const char D_0069C448[8] = "03";
const char D_0069C450[8] = "02";
const char D_0069C458[8] = "01";
const char D_0069C460[8] = "00";
const char D_0069C468[8] = "effect\\";
const char D_0069C470[8] = "mobj299";
const char D_0069C478[8] = "mobj298";
const char D_0069C480[8] = "mobj297";
const char D_0069C488[8] = "mobj296";
const char D_0069C490[8] = "mobj295";
const char D_0069C498[8] = "mobj294";
const char D_0069C4A0[8] = "mobj293";
const char D_0069C4A8[8] = "mobj292";
const char D_0069C4B0[8] = "mobj291";
const char D_0069C4B8[8] = "mobj290";
const char D_0069C4C0[8] = "mobj289";
const char D_0069C4C8[8] = "mobj288";
const char D_0069C4D0[8] = "mobj287";
const char D_0069C4D8[8] = "mobj286";
const char D_0069C4E0[8] = "mobj285";
const char D_0069C4E8[8] = "mobj284";
const char D_0069C4F0[8] = "mobj283";
const char D_0069C4F8[8] = "mobj282";
const char D_0069C500[8] = "mobj281";
const char D_0069C508[8] = "mobj280";
const char D_0069C510[8] = "mobj279";
const char D_0069C518[8] = "mobj278";
const char D_0069C520[8] = "mobj277";
const char D_0069C528[8] = "mobj276";
const char D_0069C530[8] = "mobj275";
const char D_0069C538[8] = "mobj274";
const char D_0069C540[8] = "mobj273";
const char D_0069C548[8] = "mobj272";
const char D_0069C550[8] = "mobj271";
const char D_0069C558[8] = "mobj270";
const char D_0069C560[8] = "mobj269";
const char D_0069C568[8] = "mobj268";
const char D_0069C570[8] = "mobj267";
const char D_0069C578[8] = "mobj266";
const char D_0069C580[8] = "mobj265";
const char D_0069C588[8] = "mobj264";
const char D_0069C590[8] = "mobj263";
const char D_0069C598[8] = "mobj262";
const char D_0069C5A0[8] = "mobj261";
const char D_0069C5A8[8] = "mobj260";
const char D_0069C5B0[8] = "mobj259";
const char D_0069C5B8[8] = "mobj258";
const char D_0069C5C0[8] = "mobj257";
const char D_0069C5C8[8] = "mobj256";
const char D_0069C5D0[8] = "mobj255";
const char D_0069C5D8[8] = "mobj254";
const char D_0069C5E0[8] = "mobj253";
const char D_0069C5E8[8] = "mobj252";
const char D_0069C5F0[8] = "mobj251";
const char D_0069C5F8[8] = "mobj250";
const char D_0069C600[8] = "mobj249";
const char D_0069C608[8] = "mobj248";
const char D_0069C610[8] = "mobj247";
const char D_0069C618[8] = "mobj246";
const char D_0069C620[8] = "mobj245";
const char D_0069C628[8] = "mobj244";
const char D_0069C630[8] = "mobj243";
const char D_0069C638[8] = "mobj242";
const char D_0069C640[8] = "mobj241";
const char D_0069C648[8] = "mobj240";
const char D_0069C650[8] = "mobj239";
const char D_0069C658[8] = "mobj238";
const char D_0069C660[8] = "mobj237";
const char D_0069C668[8] = "mobj236";
const char D_0069C670[8] = "mobj235";
const char D_0069C678[8] = "mobj234";
const char D_0069C680[8] = "mobj233";
const char D_0069C688[8] = "mobj232";
const char D_0069C690[8] = "mobj231";
const char D_0069C698[8] = "mobj230";
const char D_0069C6A0[8] = "mobj229";
const char D_0069C6A8[8] = "mobj228";
const char D_0069C6B0[8] = "mobj227";
const char D_0069C6B8[8] = "mobj226";
const char D_0069C6C0[8] = "mobj225";
const char D_0069C6C8[8] = "mobj224";
const char D_0069C6D0[8] = "mobj223";
const char D_0069C6D8[8] = "mobj222";
const char D_0069C6E0[8] = "mobj221";
const char D_0069C6E8[8] = "mobj220";
const char D_0069C6F0[8] = "mobj219";
const char D_0069C6F8[8] = "mobj218";
const char D_0069C700[8] = "mobj217";
const char D_0069C708[8] = "mobj216";
const char D_0069C710[8] = "mobj215";
const char D_0069C718[8] = "mobj214";
const char D_0069C720[8] = "mobj213";
const char D_0069C728[8] = "mobj212";
const char D_0069C730[8] = "mobj211";
const char D_0069C738[8] = "mobj210";
const char D_0069C740[8] = "mobj209";
const char D_0069C748[8] = "mobj208";
const char D_0069C750[8] = "mobj207";
const char D_0069C758[8] = "mobj206";
const char D_0069C760[8] = "mobj205";
const char D_0069C768[8] = "mobj204";
const char D_0069C770[8] = "mobj203";
const char D_0069C778[8] = "mobj202";
const char D_0069C780[8] = "mobj201";
const char D_0069C788[8] = "mobj200";
const char D_0069C790[8] = "mobj199";
const char D_0069C798[8] = "mobj198";
const char D_0069C7A0[8] = "mobj197";
const char D_0069C7A8[8] = "mobj196";
const char D_0069C7B0[8] = "mobj195";
const char D_0069C7B8[8] = "mobj194";
const char D_0069C7C0[8] = "mobj193";
const char D_0069C7C8[8] = "mobj192";
const char D_0069C7D0[8] = "mobj191";
const char D_0069C7D8[8] = "mobj190";
const char D_0069C7E0[8] = "mobj189";
const char D_0069C7E8[8] = "mobj188";
const char D_0069C7F0[8] = "mobj187";
const char D_0069C7F8[8] = "mobj186";
const char D_0069C800[8] = "mobj185";
const char D_0069C808[8] = "mobj184";
const char D_0069C810[8] = "mobj183";
const char D_0069C818[8] = "mobj182";
const char D_0069C820[8] = "mobj181";
const char D_0069C828[8] = "mobj180";
const char D_0069C830[8] = "mobj179";
const char D_0069C838[8] = "mobj178";
const char D_0069C840[8] = "mobj177";
const char D_0069C848[8] = "mobj176";
const char D_0069C850[8] = "mobj175";
const char D_0069C858[8] = "mobj174";
const char D_0069C860[8] = "mobj173";
const char D_0069C868[8] = "mobj172";
const char D_0069C870[8] = "mobj171";
const char D_0069C878[8] = "mobj170";
const char D_0069C880[8] = "mobj169";
const char D_0069C888[8] = "mobj168";
const char D_0069C890[8] = "mobj167";
const char D_0069C898[8] = "mobj166";
const char D_0069C8A0[8] = "mobj165";
const char D_0069C8A8[8] = "mobj164";
const char D_0069C8B0[8] = "mobj163";
const char D_0069C8B8[8] = "mobj162";
const char D_0069C8C0[8] = "mobj161";
const char D_0069C8C8[8] = "mobj160";
const char D_0069C8D0[8] = "mobj159";
const char D_0069C8D8[8] = "mobj158";
const char D_0069C8E0[8] = "mobj157";
const char D_0069C8E8[8] = "mobj156";
const char D_0069C8F0[8] = "mobj155";
const char D_0069C8F8[8] = "mobj154";
const char D_0069C900[8] = "mobj152";
const char D_0069C908[8] = "mobj151";
const char D_0069C910[8] = "mobj150";
const char D_0069C918[8] = "mobj149";
const char D_0069C920[8] = "mobj148";
const char D_0069C928[8] = "mobj147";
const char D_0069C930[8] = "mobj146";
const char D_0069C938[8] = "mobj145";
const char D_0069C940[8] = "mobj144";
const char D_0069C948[8] = "mobj143";
const char D_0069C950[8] = "mobj142";
const char D_0069C958[8] = "mobj141";
const char D_0069C960[8] = "mobj140";
const char D_0069C968[8] = "mobj139";
const char D_0069C970[8] = "mobj138";
const char D_0069C978[8] = "mobj137";
const char D_0069C980[8] = "mobj136";
const char D_0069C988[8] = "mobj135";
const char D_0069C990[8] = "mobj134";
const char D_0069C998[8] = "mobj133";
const char D_0069C9A0[8] = "mobj132";
const char D_0069C9A8[8] = "mobj131";
const char D_0069C9B0[8] = "mobj130";
const char D_0069C9B8[8] = "mobj129";
const char D_0069C9C0[8] = "mobj128";
const char D_0069C9C8[8] = "mobj127";
const char D_0069C9D0[8] = "mobj126";
const char D_0069C9D8[8] = "mobj125";
const char D_0069C9E0[8] = "mobj124";
const char D_0069C9E8[8] = "mobj123";
const char D_0069C9F0[8] = "mobj122";
const char D_0069C9F8[8] = "mobj121";
const char D_0069CA00[8] = "mobj120";
const char D_0069CA08[8] = "mobj119";
const char D_0069CA10[8] = "mobj118";
const char D_0069CA18[8] = "mobj117";
const char D_0069CA20[8] = "mobj116";
const char D_0069CA28[8] = "mobj115";
const char D_0069CA30[8] = "mobj114";
const char D_0069CA38[8] = "mobj113";
const char D_0069CA40[8] = "mobj112";
const char D_0069CA48[8] = "mobj111";
const char D_0069CA50[8] = "mobj110";
const char D_0069CA58[8] = "mobj109";
const char D_0069CA60[8] = "mobj108";
const char D_0069CA68[8] = "mobj107";
const char D_0069CA70[8] = "mobj106";
const char D_0069CA78[8] = "mobj105";
const char D_0069CA80[8] = "mobj104";
const char D_0069CA88[8] = "mobj103";
const char D_0069CA90[8] = "mobj102";
const char D_0069CA98[8] = "mobj101";
const char D_0069CAA0[8] = "mobj100";
const char D_0069CAA8[8] = "mobj099";
const char D_0069CAB0[8] = "mobj098";
const char D_0069CAB8[8] = "mobj097";
const char D_0069CAC0[8] = "mobj096";
const char D_0069CAC8[8] = "mobj095";
const char D_0069CAD0[8] = "mobj094";
const char D_0069CAD8[8] = "mobj093";
const char D_0069CAE0[8] = "mobj092";
const char D_0069CAE8[8] = "mobj091";
const char D_0069CAF0[8] = "mobj090";
const char D_0069CAF8[8] = "mobj089";
const char D_0069CB00[8] = "mobj088";
const char D_0069CB08[8] = "mobj087";
const char D_0069CB10[8] = "mobj086";
const char D_0069CB18[8] = "mobj085";
const char D_0069CB20[8] = "mobj084";
const char D_0069CB28[8] = "mobj083";
const char D_0069CB30[8] = "mobj082";
const char D_0069CB38[8] = "mobj081";
const char D_0069CB40[8] = "mobj080";
const char D_0069CB48[8] = "mobj079";
const char D_0069CB50[8] = "mobj078";
const char D_0069CB58[8] = "mobj077";
const char D_0069CB60[8] = "mobj076";
const char D_0069CB68[8] = "mobj075";
const char D_0069CB70[8] = "mobj074";
const char D_0069CB78[8] = "mobj073";
const char D_0069CB80[8] = "mobj072";
const char D_0069CB88[8] = "mobj071";
const char D_0069CB90[8] = "mobj070";
const char D_0069CB98[8] = "mobj069";
const char D_0069CBA0[8] = "mobj068";
const char D_0069CBA8[8] = "mobj067";
const char D_0069CBB0[8] = "mobj066";
const char D_0069CBB8[8] = "mobj065";
const char D_0069CBC0[8] = "mobj064";
const char D_0069CBC8[8] = "mobj063";
const char D_0069CBD0[8] = "mobj062";
const char D_0069CBD8[8] = "mobj061";
const char D_0069CBE0[8] = "mobj060";
const char D_0069CBE8[8] = "mobj059";
const char D_0069CBF0[8] = "mobj058";
const char D_0069CBF8[8] = "mobj057";
const char D_0069CC00[8] = "mobj056";
const char D_0069CC08[8] = "mobj055";
const char D_0069CC10[8] = "mobj054";
const char D_0069CC18[8] = "mobj053";
const char D_0069CC20[8] = "mobj052";
const char D_0069CC28[8] = "mobj051";
const char D_0069CC30[8] = "mobj050";
const char D_0069CC38[8] = "mobj049";
const char D_0069CC40[8] = "mobj048";
const char D_0069CC48[8] = "mobj047";
const char D_0069CC50[8] = "mobj046";
const char D_0069CC58[8] = "mobj045";
const char D_0069CC60[8] = "mobj044";
const char D_0069CC68[8] = "mobj043";
const char D_0069CC70[8] = "mobj042";
const char D_0069CC78[8] = "mobj041";
const char D_0069CC80[8] = "mobj040";
const char D_0069CC88[8] = "mobj038";
const char D_0069CC90[8] = "mobj037";
const char D_0069CC98[8] = "mobj036";
const char D_0069CCA0[8] = "mobj035";
const char D_0069CCA8[8] = "mobj034";
const char D_0069CCB0[8] = "mobj033";
const char D_0069CCB8[8] = "mobj027";
const char D_0069CCC0[8] = "mobj026";
const char D_0069CCC8[8] = "mobj025";
const char D_0069CCD0[8] = "mobj024";
const char D_0069CCD8[8] = "mobj023";
const char D_0069CCE0[8] = "mobj022";
const char D_0069CCE8[8] = "mobj021";
const char D_0069CCF0[8] = "mobj020";
const char D_0069CCF8[8] = "mobj019";
const char D_0069CD00[8] = "mobj018";
const char D_0069CD08[8] = "mobj017";
const char D_0069CD10[8] = "mobj016";
const char D_0069CD18[8] = "mobj014";
const char D_0069CD20[8] = "mobj013";
const char D_0069CD28[8] = "mobj012";
const char D_0069CD30[8] = "mobj011";
const char D_0069CD38[8] = "mobj010";
const char D_0069CD40[8] = "mobj009";
const char D_0069CD48[8] = "mobj008";
const char D_0069CD50[8] = "mobj007";
const char D_0069CD58[8] = "mobj006";
const char D_0069CD60[8] = "mobj005";
const char D_0069CD68[8] = "mobj004";
const char D_0069CD70[8] = "mobj003";
const char D_0069CD78[8] = "mobj002";
const char D_0069CD80[8] = "mobj001";
const char D_0069CD88[8] = "mobj000";
const char D_0069CD90[8] = "npc_020";
const char D_0069CD98[8] = "npc_019";
const char D_0069CDA0[8] = "npc_018";
const char D_0069CDA8[8] = "npc_017";
const char D_0069CDB0[8] = "npc_016";
const char D_0069CDB8[8] = "npc_015";
const char D_0069CDC0[8] = "npc_014";
const char D_0069CDC8[8] = "npc_013";
const char D_0069CDD0[8] = "npc_012";
const char D_0069CDD8[8] = "npc_011";
const char D_0069CDE0[8] = "npc_010";
const char D_0069CDE8[8] = "npc_009";
const char D_0069CDF0[8] = "npc_008";
const char D_0069CDF8[8] = "npc_007";
const char D_0069CE00[8] = "npc_006";
const char D_0069CE08[8] = "npc_005";
const char D_0069CE10[8] = "npc_004";
const char D_0069CE18[8] = "npc_003";
const char D_0069CE20[8] = "npc_002";
const char D_0069CE28[8] = "npc_001";
const char D_0069CE30[8] = "cf\\";
const char D_0069CE38[8] = "oldf";
const char D_0069CE40[8] = "cat";
const char D_0069CE48[8] = "cook2";
const char D_0069CE50[8] = "cook1";
const char D_0069CE58[8] = "erobo";
const char D_0069CE60[8] = "kenkyuf";
const char D_0069CE68[8] = "kenkyum";
const char D_0069CE70[8] = "faithm";
const char D_0069CE78[8] = "proudm";
const char D_0069CE80[8] = "urtvf";
const char D_0069CE88[8] = "kebin";
const char D_0069CE90[8] = "albelt";
const char D_0069CE98[8] = "kidf";
const char D_0069CEA0[8] = "kidm";
const char D_0069CEA8[8] = "soldf";
const char D_0069CEB0[8] = "soldm";
const char D_0069CEB8[8] = "oldm";
const char D_0069CEC0[8] = "girl";
const char D_0069CEC8[8] = "boy";
const char D_0069CED0[8] = "woman";
const char D_0069CED8[8] = "man";

const float D_0069AEC8 = 0.639999986f;
const float D_0069AECC = 0.200000003f;
const float D_0069AED0 = 0.639999986f;
const float D_0069AED4 = 0.000510000042f;
const float D_0069AED8 = 0.899999976f;
const float D_0069AEDC = 0.00144500006f;

int D_0069B97C = -1;
int D_0069B980 = -1;
int uwamonoCurrentIndex = -1;
int uwamonoPartIndices[2] = { -1, -1 };
unsigned char D_0069B990 = 0xff;   /* no reader in this file */
short D_0069B992 = -1;
unsigned char D_0069B998[3] = { 0xff, 0xff, 0xff };
unsigned char D_0069B9A0[3] = { 0xff, 0xff, 0xff };
signed char D_0069B9B6 = 0;
unsigned char D_0069B9B7 = 0;

unsigned char D_002B5ED0[12] = { 0 };
