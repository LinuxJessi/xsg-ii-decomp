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

/* File names of the map object resources (matumoto\\*.pxy models, *.txy textures, spline.dat, enemyorder.dat,
   treasurebox.dat, kowaremono.dat): the first argument of each GameResLoadName call in mapObjectResourcesLoad. */
int GameResLoadName(const char *name, void **data, unsigned int *size, void (*callback)(int index, void *buffer, unsigned int size, int value));
int xglCdServiceIfPending(void);
extern const char *mapObjectResourceNames[MAP_RESOURCE_COUNT];
/* Resource slots: each GameResLoadName call stores the loaded block through &mapObjectResources[i]. */
extern void *mapObjectResources[];
extern int D_0069D1DC;
extern int D_0069D1E0;

/* Loads every map object resource named in mapObjectResourceNames (models, textures and data files), waiting for each CD read, and keeps
   entries 1 and 2 in D_0069D1E0 and D_0069D1DC. */
void mapObjectResourcesLoad(void)
{
    int i;

    for (i = 0; i < MAP_RESOURCE_COUNT; i++) {
        GameResLoadName(mapObjectResourceNames[i], &mapObjectResources[i], 0, 0);
        while (xglCdServiceIfPending() != 0) {
        }
    }
    D_0069D1E0 = (int)mapObjectResources[1];
    D_0069D1DC = (int)mapObjectResources[2];
}

#include "shared.h"

/* 0x1A00 bytes of camera slot records, 8-aligned: copied whole with ld/sd. */
typedef struct CameraSlotBlock {
    DoublewordAlignedVector4 unmodeled_000[0x1A00 / 16];
} CameraSlotBlock;

/* 0x280 bytes of the camera slot table, copied with a runtime alignment test. */
typedef struct CameraSlotTable {
    int unmodeled_000[0x280 / 4];
} CameraSlotTable;

extern CameraSlotBlock cameraCfSlots;
extern CameraSlotBlock D_0069E640;
extern CameraSlotTable cameraSlotTable;
extern CameraSlotTable D_006A0040;
extern unsigned char D_002B5E6C[];
extern unsigned char D_0069E180;

/* Saves the camera slot records, the slot table and the track index to their save blocks. */
void cameraSaveState(void)
{
    D_0069E640 = cameraCfSlots;
    D_006A0040 = cameraSlotTable;
    D_0069E180 = D_002B5E6C[0];
}

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

/* Triangle wave record: value is the current amplitude, step the amount it decays per cycle; period and phase
   are in frames, hold delays the decay, remaining counts the cycles left. */
struct CameraWave {
    float value;                        /* +0x00 */
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
    unsigned char unmodeled_0D2[3];
    signed char mode;                   /* +0xD5: set by cameraSetMode */
    unsigned char allSlotsFlagA;  /* +0x0D6 */
    unsigned char allSlotsFlagB;  /* +0x0D7: also the anchor pending flag of the range update */
    unsigned char unmodeled_0D8[0x1];
    unsigned char anchorLocked;  /* +0x0D9 */
    unsigned char unmodeled_0DA[0x2];
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
    unsigned char unmodeled_18D[0x3];
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

/* One camera record of cameraCfSlots (208 bytes): its flag is cleared and its index set to -1 by cameraReset. */
struct CameraRecord {
    unsigned char unmodeled_00;
    unsigned char flag01;               /* +0x01: cleared by cameraReset */
    signed char index02;                /* +0x02: -1 after cameraReset */
    unsigned char unmodeled_03[0x1D];
    struct CameraRange range;           /* +0x20: the range cleared by cameraRangeSetDefaults */
    unsigned char unmodeled_90[0x40];
};

/* One 20-byte entry of cameraSlotTable: two index bytes set to -1 and a word cleared by cameraReset. */
struct CameraSlotEntry {
    signed char indexA;                 /* +0x00 */
    signed char indexB;                 /* +0x01 */
    unsigned char unmodeled_02[0xA];
    int word0C;                         /* +0x0C */
    unsigned char unmodeled_10[0x4];
};

/* Three words each, cleared by cameraReset; their meaning is not established yet. */
typedef union CameraWords3 {
    int word[3];
    Vector4 v;
    int w[4];
} CameraWords3;

extern CameraWords3 D_0069E5E0;
extern CameraWords3 cameraProbePosition;
void cameraInit(void);
void cameraSetMode(int mode, int arg1, int arg2);

void cameraReset(void)
{
    struct CameraRecord *record;
    int i;

    cameraInit();
    record = (struct CameraRecord *)&cameraCfSlots;
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

extern UwamonoProbe D_0069E5A0;
extern UwamonoProbe cameraPositionProbe;
int mapFindBlockEntry(int, int);
void mapProbeInit(UwamonoProbe *probe);

void cameraSetMode(int mode, int arg1, int arg2)
{
    int probeBlock;

    if (gameWork.mode == mode && D_0069E5A0.ground != 0 && cameraPositionProbe.ground != 0) {
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
    mapProbeInit(&D_0069E5A0);
    D_0069E5A0.scratchWork = 0x70000170;
    D_0069E5A0.ground = probeBlock;
    D_0069E5A0.clearedB = 0;
    D_0069E5A0.clearedA = 0;
    D_0069E5A0.range = 1000.0f;
    D_0069E5A0.mask08 = 0x802;
    mapProbeInit(&cameraPositionProbe);
    cameraPositionProbe.scratchWork = 0x70000170;
    ((CameraFlagsView *)&gameWork)->flags |= CAMERA_FLAG_MODE_CHANGED;
    cameraPositionProbe.mask08 = 0x802;
    cameraPositionProbe.ground = probeBlock;
    cameraPositionProbe.range = 1000.0f;
    cameraPositionProbe.clearedA = 0;
    cameraPositionProbe.clearedB = 0;
}

extern int cameraPlayerProbe[];

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
extern struct CameraSlot D_002B5ED0[];
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
        cameraSlotSetup(D_002B5ED0, blend_mode, ease_mode, frames);
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
    unsigned char unmodeled_02[0x14 - 0x02];
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

extern float D_0069AE40;
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
    probeCleared = cameraPositionProbe.clearedA;
    if (probeCleared == 0) {
        QuadwordCopy(&scratch->probePosition, &cameraProbePosition);
        mapBindAndSnapObject(&scratch->probePosition, &cameraPositionProbe);
        mapMoveProbe(&scratch->probePosition, 0, &cameraPositionProbe);
        probeCleared = cameraPositionProbe.clearedA;
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
        cameraPositionProbe.clearedA = probeCleared;
        floorHeight = cameraProbeSlide(&scratch->probePosition, &scratch->probeDelta, &cameraPositionProbe);
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
    if (cameraPositionProbe.clearedA == 0) {
        *(int *)&scratch->orbit.x = 0;
        cameraOrbitPosition((AlignedVector4 *)&cameraProbePosition, &scratch->lookAt, &scratch->orbit);
        mapBindAndSnapObject((AlignedVector4 *)&cameraProbePosition, &cameraPositionProbe);
        mapMoveProbe((AlignedVector4 *)&cameraProbePosition, 0, &cameraPositionProbe);
        scratch->orbit.x = scratch->actor->position.x;
        cameraOrbitPosition(&scratch->position, &scratch->lookAt, &scratch->orbit);
        Vu0SubVectorXYZ(&scratch->probe, &scratch->position, (AlignedVector4 *)&cameraProbePosition);
        cameraProbeSlide((AlignedVector4 *)&cameraProbePosition, &scratch->probe, &cameraPositionProbe);
    }

    pitch = scratch->record->range.angles.x;
    scratch->orbit.x = pitch;
    cameraOrbitPosition(&scratch->position, &scratch->lookAt, &scratch->orbit);
    pitchStep = CAMERA_SEARCH_STEP_FINE;
    QuadwordCopy(&scratch->probe, (AlignedVector4 *)&cameraProbePosition);
    Vu0SubVectorXYZ(&scratch->delta, &scratch->position, (AlignedVector4 *)&cameraProbePosition);
    cameraPositionProbe.mask08 |= 0x1000;
    if (scratch->record->range.probeMode == CAMERA_PROBE_MODE_SLIDE) {
        floorHeight = cameraProbeSlide(&scratch->probe, &scratch->delta, &cameraPositionProbe);
    } else {
        floorHeight = mapMoveProbe(&scratch->probe, &scratch->delta, &cameraPositionProbe);
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
        cameraPositionProbe.mask08 |= 0x1000;
        floorHeight = mapMoveProbe(&scratch->probe, 0, &cameraPositionProbe);
        if (floorHeight == -1000.0f) {
            pitch = 0.0f;
            scratch->orbit.x = pitch;
            cameraOrbitPosition(&scratch->probe, &scratch->lookAt, &scratch->orbit);
            pitchStep = -CAMERA_SEARCH_STEP_COARSE;
            QuadwordCopy(&scratch->position, &scratch->probe);
            cameraPositionProbe.mask08 |= 0x1000;
            floorHeight = mapMoveProbe(&scratch->probe, 0, &cameraPositionProbe);
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
                    cameraPositionProbe.mask08 |= 0x1000;
                    cameraProbeSlide((AlignedVector4 *)&cameraProbePosition, &scratch->delta, &cameraPositionProbe);
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
            floorHeight = mapMoveProbe(&scratch->probe, &scratch->delta, &cameraPositionProbe);
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
    unsigned char unmodeled_04[0x08];
    int anchorIndex;                    /* +0x0C */
    unsigned char unmodeled_10[0x20];
    Vector4 lookAt;                   /* +0x30 */
    unsigned char unmodeled_40[0x50];
    Vector4 angles;                   /* +0x90 */
    unsigned char unmodeled_A0[0x20];
    Vector4 position;                   /* +0xC0 */
};

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
        *(float *)0x700000C0 = record->range.lockPosition.x;
    }
    if (record->range.lockY != 0) {
        *(float *)0x700000C4 = record->range.lockPosition.y;
    }
    if (record->range.lockZ != 0) {
        *(float *)0x700000C8 = record->range.lockPosition.z;
    }
    cameraAnglesBetween(&scratch->angles, &scratch->lookAt, &scratch->position);
    gameWork.pitch = *(float *)0x70000090;
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

extern int D_0069E568[];

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
        record = *(struct CameraRecord **)0x70000000;
        gameWork.positionX = record->range.lockPosition.x;
        gameWork.positionY = record->range.lockPosition.y;
        gameWork.positionZ = record->range.lockPosition.z;
        gameWork.lookAtX = record->range.lookAt.x;
        gameWork.lookAtY = record->range.lookAt.y;
        gameWork.lookAtZ = record->range.lookAt.z;
        gameWork.perspective = record->range.lockPosition.w;
        gameWork.roll = record->range.lookAt.w;
    } else {
        record = *(struct CameraRecord **)0x70000000;
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
    (*(float *)0x70000030) = gameWork.lookAtX;
    (*(float *)0x70000034) = gameWork.lookAtY;
    (*(float *)0x70000038) = gameWork.lookAtZ;
    cameraAnglesBetween((Vector4 *)0x70000090, (Vector4 *)0x70000030, &cameraProbePosition);
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
    *(float *)0x70000030 = gameWork.lookAtX;
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
        limitIndex = cameraAngleLimitIndex(&(*(struct CameraRecord **)0x70000000)->range.angleLimits.z, yaw);
        if (limitIndex >= 0) {
            yaw = (&(*(struct CameraRecord **)0x70000000)->range.angleLimits.z)[limitIndex];
            depth = gameWork.lookAtZ - gameWork.positionZ;
            tanYaw = xglSin(yaw) / xglCos(yaw);
            gameWork.lookAtX = gameWork.positionX + depth * tanYaw;
            cameraAnglesBetween((Vector4 *)0x70000090, (Vector4 *)&gameWork.lookAtX, (Vector4 *)&gameWork.positionX);
            gameWork.pitch = *(float *)0x70000090;
            pitch = gameWork.pitch;
            gameWork.yaw = *(float *)0x70000094;
            gameWork.distance = *(float *)0x7000009C;
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
        limitIndex = cameraAngleLimitIndex(&(*(struct CameraRecord **)0x70000000)->range.angleLimits.x, pitch);
        if (limitIndex >= 0) {
            pitch = (&(*(struct CameraRecord **)0x70000000)->range.angleLimits.x)[limitIndex];
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
        *(unsigned char *)0x70000018 = moved;
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
                if (*(unsigned char *)0x70000019 != 0 && *(unsigned char *)0x70000018 == 0) {
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

/* Advances the wave one frame and returns its offset: rising from -value to value over the first half of the
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
        wave->value = wave->value - wave->step;
        if (wave->value < 0.0f) {
            wave->value = 0.0f;
        }
    }
    half = wave->period >> 1;
    doubled = wave->value + wave->value;
    if (wave->phase < half) {
        result = (doubled * I2F(wave->phase)) / I2F(half) - wave->value;
    } else {
        result = wave->value - (doubled * I2F(wave->phase - half)) / I2F(half);
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
        gameWork.slot[0].progress += *(float *)0x7000001C;
        gameWork.slot[0].fraction = cameraEase((unsigned char)gameWork.slot[0].easeMode, gameWork.slot[0].progress, gameWork.slot[0].duration);
        cameraLerpVector((Vector4 *)&gameWork.lookAtX, (Vector4 *)&gameWork.fromLookAtX, (Vector4 *)&gameWork.toLookAtX, gameWork.slot[0].fraction);
    }

    if (gameWork.slot[1].progress < gameWork.slot[1].duration) {
        gameWork.slot[1].progress += *(float *)0x7000001C;
        e = cameraEase((unsigned char)gameWork.slot[1].easeMode, gameWork.slot[1].progress, gameWork.slot[1].duration);
        gameWork.slot[1].fraction = e;
        if (gameWork.slot[1].blendMode == 0) {
            cameraLerpVector((Vector4 *)&gameWork.positionX, (Vector4 *)&gameWork.fromPositionX, (Vector4 *)&gameWork.toPositionX, e);
            cameraAnglesBetween((Vector4 *)0x70000090, (Vector4 *)&gameWork.lookAtX, (Vector4 *)&gameWork.positionX);
            gameWork.pitch = *(float *)0x70000090;
            gameWork.yaw = *(float *)0x70000094;
            gameWork.distance = *(float *)0x7000009C;
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
        float next2 = p2 + *(float *)0x7000001C;
        gameWork.slot[2].progress = next2;
        d2 = cameraEase((unsigned char)gameWork.slot[2].easeMode, next2, lim2);
        gameWork.slot[2].fraction = d2;
        gameWork.perspective = cameraLerpFloat(gameWork.fromPerspective, gameWork.toPerspective, gameWork.slot[2].fraction);
        p2 = next2;
    }

    p3 = gameWork.slot[3].progress;
    lim3 = gameWork.slot[3].duration;
    if (p3 < lim3) {
        float next3 = p3 + *(float *)0x7000001C;
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
        *(float *)0x700000C0 = cameraWaveStep(&gameWork.waves[0]);
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
            *(float *)0x700000C0 += gameWork.sway[0];
            gameWork.sway[3] += t2;
            gameWork.sway[1] += gameWork.sway[3];
            *(float *)0x700000C4 += gameWork.sway[1];
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
    scratch->p->rotation.z = gameWork.roll + *(float *)0x700000C8;
    cameraAnglesBetween((Vector4 *)0x70000090, &scratch->p->chase_position, &scratch->p->position);
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

extern DoublewordAlignedVector4 cameraPlayerPositionOld;
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
                    gameWork.unmodeled_0D2[2] = 0;
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
                    gameWork.unmodeled_0D2[2] = -1;
                    D_0069E5A0.clearedA = 0;
                    cameraPositionProbe.clearedA = 0;
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
                record = (struct CameraRecord *)((unsigned char *)&cameraCfSlots + recordIndex * 0xD0);
                scratch->record = record;
                scratch->slot = (struct CameraSlot *)((unsigned char *)record + 16);
                previous = (signed char)gameWork.unmodeled_0D2[2];
                if (previous != recordIndex) {
                    gameWork.unmodeled_0D8[0] = (unsigned char)gameWork.unmodeled_0D2[2];
                    gameWork.unmodeled_0D2[2] = (unsigned char)recordIndex;
                    entry = cameraSlotFind(previous, (signed char)recordIndex);
                    if (entry != 0) {
                        scratch->slot = (struct CameraSlot *)((unsigned char *)entry + 4);
                        anchorReady = gameWork.anchorReady = entry->unmodeled_02[0];
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
                                && !(*(long long *)((unsigned char *)record + 0x28) & 0xFFFFFF)) {
                                scratch->anchorMatched = 1;
                            }
                        }
                    }
                    record = scratch->record;
                    if (gameWork.unmodeled_0D2[0] != record->unmodeled_00) {
                        gameWork.unmodeled_0DA[0] = gameWork.unmodeled_0D2[0];
                        gameWork.unmodeled_0D2[0] = record->unmodeled_00;
                        gameWork.wordE0 = 0;
                        gameWork.unmodeled_0D2[1] = 0;
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
                switch (gameWork.unmodeled_0D2[0]) {
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
                if (gameWork.unmodeled_18D[2] != 0) {
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
extern const float D_0069AEC8;
extern const float D_0069AECC;
extern const float D_0069AED0;
extern const float D_0069AED4;
extern const float D_0069AED8;
extern const float D_0069AEDC;
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

                actor->pack = values[0];
                actor->effect = values[1];
                actor->hierarchy = values[2];
                actor->slotTable = values[4];
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
extern float D_0069E184;
extern int D_0069E188;
extern PadRecord PadData;
extern UwamonoSlope elvSlopeTable[];
extern float D_002B9C98[];
extern unsigned char D_002B9CA8[];
extern UwamonoProbe D_002B9CB8;

/* Per jump height step (UwamonoSearch.level): the frames before the landing
 * check (D_0069B978) and before the target action (D_0069B97C). */
short D_0069B9A8[3] = { -1, -1, -1 };

short D_0069B9B0[3] = { -1, -1, -1 };

int uwamonoUpdateLadderState(void);
void XenoActorHidePart(struct XenoActor *actor, const char *part);
void XenoActorShowPart(struct XenoActor *actor, const char *part);
int func_001FD7F8(void);
void func_00207650(int kind, int id, int value);
void func_001FF3C0(int index);
void uwamonoReleaseHandles(void);
void XenoActorSetSelector(struct XenoActor *actor, int selector);
float atan2f(float y, float x);
float func_002A1B30(float value);
float Get_Decimal_Surplus_for_Radius(float angle);
float xglSin(float angle);
float xglCos(float angle);
void XenoActorCheckMode(struct XenoActor *actor, int mode);
void XenoActorSetMode(struct XenoActor *actor, int mode);
int func_001FEAC8(struct XenoActor *actor, float range);
unsigned int msgWindowInUse(void);
void talkSetResult(int id);
void mapObjectSetActive(UwamonoObject *object, UwamonoObject *target);
void func_001FE5B0(int index, int value);
void func_001FE830(int index, int kind, int frames);
void func_00205D20(int index, AlignedVector4 *position);
void func_001FDE18(int step);
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
void func_00205C78(void);
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
extern int D_002B5D98[];
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
    if (D_002B5D98[0] & CONTROL_SCRIPTED) {
        uwamonoTryClearTarget(index, 0);
        return;
    }
    scriptTarget = func_001FD7F8();
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
            if (actor->mode == 5 && actor->motionFrame >= 18.0f) {
                /* The original calls it twice in a row (and ShowPart below the same). */
                XenoActorHidePart(riders[0], "jin_saya_tuka");
                XenoActorHidePart(riders[0], "jin_saya_tuka");
            }
            if (record->key == UWAMONO_KEY_JIN_ES && actor->mode == 5 && actor->motionFrame >= 50.0f) {
                XenoActorShowPart(riders[0], "jin_saya_tuka");
                XenoActorShowPart(riders[0], "jin_saya_tuka");
            }
        }
        if (D_0069B97C != -1 && --D_0069B97C == -1 && D_0069B980 < 6 && D_0069B980 >= 3) {
            target = func_001FD7F8();
            if (target != -1) {
                func_00207650(1, uwamonoRecords[target].id, 0);
                if (uwamonoRecords[target].flags & 0x40000) {
                    func_001FF3C0(target);
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
                int analog;
                int digital;

                if (gameWork.control & CONTROL_REPLAYED) {
                    stick_x = gameWork.replayStickX;
                    stick_y = gameWork.replayStickY;
                    actor->status = gameWork.replayStatus;
                } else {
                    /* Analog stick and the pad's digital direction (scaled to stick range): the one with
                       the larger magnitude wins, opposite signs cancel. */
                    analog = PadData.stickX;
                    digital = PadData.directionX * 2;
                    if (analog != 0 && digital != 0 && (analog ^ digital) < 0) {
                        stick_x = 0;
                    } else if (analog < 0 || digital < 0) {
                        stick_x = MIN(analog, digital);
                    } else {
                        stick_x = MAX(analog, digital);
                    }
                    analog = PadData.stickY;
                    digital = PadData.directionY * 2;
                    if (analog != 0 && digital != 0 && (analog ^ digital) < 0) {
                        stick_y = 0;
                    } else if (analog < 0 || digital < 0) {
                        stick_y = MIN(analog, digital);
                    } else {
                        stick_y = MAX(analog, digital);
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
                        actor->motionSpeed = 1.0f;
                        walk = 0;
                        break;
                    }
                    if (speed < 192.0f) {
                        actor->motionSpeed = speed * 2.6f / 192.0f;
                        walk = 1;
                        break;
                    }
                    walk = 2;
                    actor->motionSpeed = speed * (1.0f / 256.0f);
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
                target = func_001FEAC8(actor, 50.0f);
                /* "Talk target -> %8d" */
                xglFontDebugPrintf(8, 0x20, "\xa5\xc8\xa1\xbc\xa5\xaf\xc2\xd0\xbe\xdd \xa2\xaa %8d", target);
                if (target != -1) {
                    if (PadData.pressed & PAD_CIRCLE) {
                        UwamonoObject *object = &uwamonoRecords[target];

                        switch (object->kind) {
                        case 2:
                            if (UWAMONO_IS_ES(actor)) {
                                if (uwamonoRecords[target].searchState == 0) {
                                    func_001FE5B0(target, 0);
                                }
                            } else if (uwamonoRecords[target].searchState == 2) {
                                func_001FE5B0(target, 0);
                            }
                            func_001FE830(target, 5, 30);
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
                    func_00205D20(uwamonoCurrentIndex, &uwamonoRecords[uwamonoCurrentIndex].actor->position);
                }
                if (walk != 0) {
                    func_001FDE18(0);
                }
                if (!(gameWork.control & CONTROL_SCRIPTED) && !(gameWork.control & CONTROL_SEARCHING) && D_0069B97C == -1) {
                    if (PadData.pressed & PAD_L1) {
                        func_001FDE18(1);
                    }
                    if (PadData.pressed & PAD_R1) {
                        func_001FDE18(-1);
                    }
                    if (PadData.pressed & PAD_SQUARE) {
                        int searched;
                        int jumpMode;

                        searched = func_001FD7F8();
                        if (searched != -1 && (uwamonoRecords[searched].flags & 0x20000)) {
                            gameWork.control |= CONTROL_SEARCHING;
                            func_00207650(0, uwamonoRecords[searched].id, 0);
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
                            actor->motionSpeed = 1.0f;
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
                func_00207650(3, (actor->status & 0x1F) + 1, 0);
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
    mapProbeInit(&D_002B9CB8);
    D_002B9CB8.ground = actor->ground;
    mapMoveProbe(&actor->position, NULL, &D_002B9CB8);
    record->position = actor->position;
    record->anchor = actor->rotation;
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
    func_00205C78();
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
