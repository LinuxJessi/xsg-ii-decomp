#include "common.h"
#include "shared.h"
#include "debug_break.h"
#include "main/pad.h"
#include "main/sdv_033.h"


/* XenoActor.rsrc slots (tu051.h's XActRsrc). */
enum FieldRsrc {
    XAct_Rsrc_MDL,
    XAct_Rsrc_TEX,
    XAct_Rsrc_XHR,
    XAct_Rsrc_3,
    XAct_Rsrc_XAP,
    XAct_Rsrc_5,
    XAct_Rsrc_XAP_Ext2,
    XAct_Rsrc_Count
};

/* One 0x40-byte row of the table at mapObjectTable (tu067.c): a placement matrix, its rotation rows then the
 * position. */
typedef struct ObjTableEntry {
    float rotation[3][4];                   /* +0x00 */
    Vector4 position;                       /* +0x30 */
} ObjTableEntry;

/* The motion/state block of a record (tya_draw_gauge.c's UwamonoState), only the fields used here. */
typedef struct FieldMotions {
    unsigned char unmodeled_0000[0x80];
    short groundId;                         /* +0x0080: map block 0x100 entry of the ground */
    unsigned char unmodeled_0082[0x4074 - 0x82];
    int resourceId;                         /* +0x4074: -1 for none */
} FieldMotions;

/* The mapMoveProbe block of a XenoActor (+0x740, tu051.h's hitBuffer onwards). */
typedef struct FieldProbe {
    unsigned char hitBuffer[8];             /* +0x00: the mapMoveProbe result */
    unsigned short hitFlags;                /* +0x08 */
    unsigned char unmodeled_0a[0x18 - 0x0A];
    int ground;                             /* +0x18: map block 0x100 entry of the ground */
    unsigned char unmodeled_1c[0x28 - 0x1C];
    float probeRadius;                      /* +0x28: set with radius to the form's size */
    unsigned char unmodeled_2c[0x40 - 0x2C];
} FieldProbe;

/* A XenoActor light block (xenoActor.c's XenoLight, 0x180 bytes), copied whole. */
typedef union FieldLight {
    DoublewordAlignedVector4 rows[24];
    struct {
        float midpointA[3][4];              /* +0x000: tu052.c's midpoint tracks */
        unsigned char unmodeled_030[0x70 - 0x30];
        float midpointB[3][4];              /* +0x070 */
        unsigned char unmodeled_0a0[0xD0 - 0xA0];
        float midpointC[4];                 /* +0x0D0 */
        unsigned char unmodeled_0e0[0xF0 - 0xE0];
        float midpointD[4];                 /* +0x0F0 */
        unsigned char unmodeled_100[0x180 - 0x100];
    } tracks;
} FieldLight;

/* The first word pair of a XenoActor motion state (xenoActor.c's XenoMotionState). */
typedef struct FieldMotion {
    u32 flags;                              /* +0x00 */
    float speed;                            /* +0x04 */
    unsigned char unmodeled_08[0x10 - 0x08];
    int range[2];                           /* +0x10: the play range */
    unsigned char unmodeled_18[0x50 - 0x18];
} FieldMotion;

/* The XenoActor of a map object (tu051.h), only the fields used here. The original reaches its vectors
 * through unions (alias set 0): the actor pointer is loaded again after each store to them. */
typedef struct FieldActor {
    unsigned char unmodeled_000[0x10];
    union {
        AlignedVector4 position;            /* +0x010 */
    };
    unsigned char unmodeled_020[0x30];
    union {
        AlignedVector4 rotation;            /* +0x050: y is the facing angle */
    };
    union {
        AlignedVector4 direction;           /* +0x060: y is the moving angle */
    };
    unsigned char unmodeled_070[0x40];
    float radius;                           /* +0x0B0 */
    float height;                           /* +0x0B4 */
    unsigned char unmodeled_0b8[4];
    int drawFlags;                          /* +0x0BC: 0x4 hides the actor */
    float alpha;                            /* +0x0C0 */
    int drawMode;                           /* +0x0C4 */
    unsigned char unmodeled_0c8[4];
    float modeValue;                        /* +0x0CC */
    unsigned char unmodeled_0d0[0x170 - 0xD0];
    float fog[4];                           /* +0x170 */
    float modeValue4;                       /* +0x180 */
    unsigned char unmodeled_184[0x190 - 0x184];
    FieldLight light;                       /* +0x190 */
    FieldLight sourceLight;                 /* +0x310 */
    union {
        DoublewordAlignedVector4 color;     /* +0x490 */
    };
    union {
        DoublewordAlignedVector4 colorBlock; /* +0x4A0 */
    };
    unsigned char unmodeled_4b0[0x5CC - 0x4B0];
    u32 flags;                              /* +0x5CC */
    float transformScale;                   /* +0x5D0 */
    int mode;                               /* +0x5D4 */
    int unmodeled_5d8;
    int requestedMode;                      /* +0x5DC */
    unsigned char unmodeled_5e0[0x5EC - 0x5E0];
    int rsrc[XAct_Rsrc_Count];              /* +0x5EC: the actor's resources */
    unsigned char unmodeled_608[0x610 - 0x608];
    void *sharedResource;                   /* +0x610: resource 0x30041 */
    unsigned char unmodeled_614[0x618 - 0x614];
    int lookLink;                           /* +0x618 */
    unsigned char unmodeled_61c[0x650 - 0x61C];
    FieldMotion motion;                     /* +0x650: state A of the channel block */
    unsigned char unmodeled_6a0[0x740 - 0x6A0];
    FieldProbe probe;                       /* +0x740 */
    unsigned char unmodeled_780[0x794 - 0x780];
    int recordIndex;                        /* +0x794: the index of its uwamonoRecords entry (tu088.c's moved) */
    int kind;                               /* +0x798: the record key */
} FieldActor;

/* The actor state a record keeps while its actor pool is rebuilt (func_001D5440 saves it, func_001D5820
 * puts it back). */
typedef struct FieldActorSave {
    FieldProbe probe;                       /* +0x648 */
    unsigned char unmodeled_688[8];
    FieldLight light;                       /* +0x690 */
    union {
        DoublewordAlignedVector4 color;     /* +0x810 */
    };
    union {
        DoublewordAlignedVector4 colorBlock; /* +0x820 */
    };
    float fog[4];                           /* +0x830 */
    int drawMode;                           /* +0x840 */
    int drawFlags;                          /* +0x844 */
    float modeValue;                        /* +0x848 */
    float modeValue4;                       /* +0x84C */
    u32 flags;                              /* +0x850 */
    float transformScale;                   /* +0x854 */
    int mode;                               /* +0x858 */
    int unmodeled_5d8;                      /* +0x85C */
    int requestedMode;                      /* +0x860 */
    float motionSpeed;                      /* +0x864 */
    int motionRange[2];                     /* +0x868 */
} FieldActorSave;

/* One 0x870-byte record of uwamonoRecords (tu051.h's UwamonoObject), only the fields used here; its
 * position and rotation are reached through unions as well. */
typedef struct FieldRecord {
    short id;                               /* +0x00 */
    unsigned char kind;                     /* +0x02: 1 talks, 2 is searched */
    unsigned char unmodeled_03;
    int key;                                /* +0x04 */
    int type;                               /* +0x08: 7-9 are save points */
    unsigned char unmodeled_0c[4];
    union {
        float coords[4];
        AlignedVector4 vector;
    } position;                             /* +0x10 */
    unsigned char unmodeled_20[0x20];
    union {
        float coords[4];
        Vector4 vector;
    } rotation;                             /* +0x40: coords[1] is the heading */
    union {
        float coords[4];
        AlignedVector4 vector;
    } scale;                                /* +0x50: x, y, z set by the player's form */
    unsigned char unmodeled_60[0x120 - 0x60];
    float followA[3][4];                    /* +0x120: tu052.c's midpoint track targets */
    unsigned char unmodeled_150[0x180 - 0x150];
    float followB[3][4];                    /* +0x180 */
    unsigned char unmodeled_1b0[0x1C0 - 0x1B0];
    float followD[4];                       /* +0x1C0 */
    unsigned char unmodeled_1d0[4];
    float blendEnd;                         /* +0x1D4 */
    unsigned char unmodeled_1d8[0x1F0 - 0x1D8];
    float followC[4];                       /* +0x1F0 */
    unsigned char unmodeled_200[0x264 - 0x200];
    ObjTableEntry *transform;               /* +0x264: the placement the model is drawn with */
    unsigned char unmodeled_268[8];
    ObjTableEntry savedMatrix;              /* +0x270 */
    ObjTableEntry pose;                     /* +0x2B0 */
    unsigned char unmodeled_2f0[0x330 - 0x2F0];
    FieldActor *actor;                      /* +0x330 */
    FieldMotions *motions;                  /* +0x334 */
    unsigned char unmodeled_338[0x580 - 0x338];
    union {
        float coords[4];
        AlignedVector4 vector;
    } extent;                               /* +0x580: x is the form's size */
    unsigned char unmodeled_590[0x638 - 0x590];
    void *effect;                           /* +0x638 */
    unsigned char unmodeled_63c[0x644 - 0x63C];
    int effectOwner;                        /* +0x644: the record whose effect follows this one, -1 for none */
    FieldActorSave saved;                   /* +0x648: the actor's state while the pool is rebuilt */
} FieldRecord;

extern FieldRecord uwamonoRecords[];
extern int uwamonoCurrentIndex;

/* The light sets of the game work block (tu069.c's lightDirection onwards), one per light set. */
typedef struct GameLightSets {
    float direction[16][3][4];          /* +0x000: light types 1-3 */
    float color[16][3][4];              /* +0x300 */
    float ambient[16][4];               /* +0x600: light type 0 */
    float type4Direction[16][4];        /* +0x700 */
    float type4Value[16];               /* +0x800 */
} GameLightSets;

/* One 256-bit map flag set (tu050.c's MAP_BIT_* arrays). */
typedef struct MapBits {
    unsigned int bits[8];
} MapBits;

/* The game work block as this file sees it: the scene state the field loop (func_001D6070) reads and writes,
 * the player parameters (tu067.c's GameWork view) and the saved copies uwamonoLoadCurrent writes beside them. */
typedef struct GameWorkPlayer {
    u8 *workEnd;                        /* +0x00: WorkEnd, saved when the loop starts */
    unsigned char unmodeled_04[2];
    u16 loadStage;                      /* +0x06: 1 once the save is read, 2 once the party models are loaded */
    int control;                        /* +0x08: CONTROL_* bits (tu051.h) */
    unsigned char pauseDelay;           /* +0x0C: frames before the pause can be toggled again */
    unsigned char unmodeled_0d[0x14 - 0x0D];
    int nextScene;                      /* +0x14: -1 when no scene change is requested */
    int mapId;                          /* +0x18 */
    int entryKey;                       /* +0x1C: -1 none, -2 the party position, else an entry key */
    int loadSlot;                       /* +0x20 */
    unsigned char unmodeled_24;
    unsigned char elevatorMask;         /* +0x25: elvActiveMask, saved with the scene */
    unsigned char unmodeled_26[0x2C - 0x26];
    void *resBuffer;                    /* +0x2C: GameResBufferAlloc result */
    unsigned char unmodeled_30[0xB0 - 0x30];
    int mapLoaded;                      /* +0xB0 */
    int keepOffMask;                    /* +0xB4: bit n set when elvKeepOffFlags[n] is */
    unsigned char unmodeled_b8[0xC0 - 0xB8];
    union {
        Vector4 scale;                  /* +0xC0: x, y and z are 1.0 */
    };
    u16 cameraFlags;                    /* +0xD0: CAMERA_FLAG_* bits (camera code, tu051.c) */
    unsigned char unmodeled_d2[0x290 - 0xD2];
    unsigned char sceneFlag;            /* +0x290: cleared at each scene start */
    unsigned char unmodeled_291[0x2D0 - 0x291];
    float frameScale;                   /* +0x2D0: elapsed frames, scales the per-frame movement */
    float frameScaleSaved;              /* +0x2D4: copy of frameScale made by uwamonoLoadCurrent */
    float playerParams[5][2];           /* +0x2D8: setPlayer arguments 4 to 8 in [i][0]; [i][1] is the saved copy */
    int climbFrames;                    /* +0x300: setPlayer argument 9 */
    int climbFramesSaved;               /* +0x304 */
    float formScale;                    /* +0x308: the form's scale, the current record's scale.x */
    float formSize;                     /* +0x30C: the form's size, the current record's extent.x */
    unsigned char unmodeled_310[0x320 - 0x310];
    short encounterReaction;            /* +0x320: -1 none */
    unsigned char unmodeled_322[2];
    int encounterFlags;                 /* +0x324 */
    unsigned char unmodeled_328[0x32C - 0x328];
    int encounterRecord;                /* +0x32C: record index of the current encounter */
    short blocked;                      /* +0x330 */
    unsigned char unmodeled_332[0x344 - 0x332];
    void *releaseHandle;                /* +0x344 */
    unsigned char unmodeled_348[0x370 - 0x348];
    int sceneMode;                      /* +0x370 */
    float playerRate;                   /* +0x374: setPlayer argument 10 */
    float playerRateSaved;              /* +0x378 */
    float playerScale;                  /* +0x37C: setPlayer argument 11 */
    float playerScaleSaved;             /* +0x380 */
    GameLightSets lights;               /* +0x384 */
    unsigned char unmodeled_bc4[0xC84 - 0xBC4];
    GameLightSets savedLights;          /* +0xC84 */
    unsigned char unmodeled_14c4[0x14E0 - 0x14C4];
    MapBits savedMapBits[2];            /* +0x14E0: D_002B7C88 and D_002B7CA8 */
    unsigned char unmodeled_1520[0x1B20 - 0x1520];
    signed char timerArmed;             /* +0x1B20: clear while the timer counts down */
    unsigned char unmodeled_1b21[3];
    int timerFrames;                    /* +0x1B24 */
} GameWorkPlayer;

extern GameWorkPlayer gameWork;
extern MapBits D_002B7C88;
extern MapBits D_002B7CA8;
extern unsigned char elvActiveMask;
extern int elvKeepOffFlags[32];
extern unsigned char D_003C6DC0[];

void cameraSaveState(void);
void XenoActorPoolInitDefault(void);
void uwamonoLoadCurrent(void);
void func_A9D710(unsigned char *script);
void sefCollectEffectRecords(void);
void sefResetWorkTablesAll(void);
void fontObjClearSlots(void);

/* Saves the scene before the actor pool is rebuilt: the camera, the map flags, the light sets, the elevator
 * masks and each record's actor state; then rebuilds the pool and reloads the current record. */
void func_001D5440(void)
{
    int i;
    int set;
    int column;
    int row;
    int bit;
    int mask;
    FieldRecord *record;
    FieldActor *actor;
    MapBits *savedBits;

    cameraSaveState();
    savedBits = gameWork.savedMapBits;
    savedBits[0] = D_002B7C88;
    savedBits[1] = D_002B7CA8;
    for (set = 0; set < 16; set++) {
        for (column = 0; column < 4; column++) {
            for (row = 0; row < 3; row++) {
                gameWork.savedLights.direction[set][row][column] = gameWork.lights.direction[set][row][column];
                gameWork.savedLights.color[set][row][column] = gameWork.lights.color[set][row][column];
            }
            gameWork.savedLights.ambient[set][column] = gameWork.lights.ambient[set][column];
            gameWork.savedLights.type4Direction[set][column] = gameWork.lights.type4Direction[set][column];
        }
        gameWork.savedLights.type4Value[set] = gameWork.lights.type4Value[set];
    }
    gameWork.elevatorMask = elvActiveMask;
    mask = 0;
    for (bit = 0; bit < 32; bit++) {
        if (elvKeepOffFlags[bit]) {
            mask |= 1 << bit;
        }
    }
    gameWork.keepOffMask = mask;
    for (i = 0; i < 256; i++) {
        record = &uwamonoRecords[i];
        actor = record->actor;
        if (actor) {
            record->saved.probe = actor->probe;
            record->saved.light = actor->light;
            record->saved.color = actor->color;
            record->saved.colorBlock = actor->colorBlock;
            record->saved.fog[0] = actor->fog[0];
            record->saved.fog[1] = actor->fog[1];
            record->saved.fog[2] = actor->fog[2];
            record->saved.fog[3] = actor->fog[3];
            record->saved.drawMode = actor->drawMode;
            record->saved.drawFlags = actor->drawFlags;
            record->saved.modeValue = actor->modeValue;
            record->saved.modeValue4 = actor->modeValue4;
            record->saved.flags = actor->flags;
            record->saved.transformScale = actor->transformScale;
            record->saved.mode = actor->mode;
            record->saved.unmodeled_5d8 = actor->unmodeled_5d8;
            record->saved.requestedMode = actor->requestedMode;
            record->saved.motionSpeed = actor->motion.speed;
            record->saved.motionRange[0] = actor->motion.range[0];
            record->saved.motionRange[1] = actor->motion.range[1];
        }
    }
    XenoActorPoolInitDefault();
    uwamonoLoadCurrent();
    func_A9D710(D_003C6DC0);
    sefCollectEffectRecords();
    sefResetWorkTablesAll();
    fontObjClearSlots();
}

void uwamonoLoadCurrent(void)
{
    gameWork.playerRateSaved = gameWork.playerRate;
    gameWork.playerScaleSaved = gameWork.playerScale;
    gameWork.frameScaleSaved = gameWork.frameScale;
    gameWork.playerParams[0][1] = gameWork.playerParams[0][0];
    gameWork.playerParams[1][1] = gameWork.playerParams[1][0];
    gameWork.playerParams[2][1] = gameWork.playerParams[2][0];
    gameWork.playerParams[3][1] = gameWork.playerParams[3][0];
    gameWork.playerParams[4][1] = gameWork.playerParams[4][0];
    gameWork.climbFramesSaved = gameWork.climbFrames;
    gameWork.formScale = uwamonoRecords[uwamonoCurrentIndex].scale.coords[0];
    gameWork.formSize = uwamonoRecords[uwamonoCurrentIndex].extent.coords[0];
}

/* The model pack mapObjectTable points to (tu067.c's PxyHeader); only the field read here. */
typedef struct PxyHeader {
    unsigned char unmodeled_00[0x68];
    int entry_offset;                       /* +0x68: offset of the 0x40-byte entry matrices */
} PxyHeader;

/* The resource block an actor's resource id loads (cfePlugIn.c's CfeResourceBlock). */
typedef struct FieldResourceBlock {
    int pack;
    int effect;
    int hierarchy;
    int lookLink;
    int slotTable;
} FieldResourceBlock;

extern u8 *WorkEnd;
extern PxyHeader *mapObjectTable;

void mapObjectResourcesLoad(void);
void mainSwitchId(int id, signed char mode);
void sefStartClipTestEnabled(void);
void sefRestoreEffectRecords(void);
int GameResLoadID(int id, void **buffer, unsigned int *size,
                  void (*callback)(int index, void *buffer, unsigned int size, int value));
/* Typed over FieldActor, the file's own view of the record; main/xenoActor.h declares XenoActorAlloc over
   struct XenoActor *, so this file does not include it. */
FieldActor *XenoActorAlloc(void);
void XenoActorAllocMatrices(FieldActor *actor);
void XenoActorSetChannelValue(FieldActor *actor, float value);
void XenoActorSetMode(FieldActor *actor, int mode);
void XenoActorSetModeInSlot(FieldActor *actor, int slot, int mode);
int mapFindBlockEntry(int groupType, int keepId);
void sefSetRecordOwner(void *record, FieldActor *owner);
void sefSetRecordOwnerMarked(void *record, FieldRecord *owner);
void XenoActorSetHoldFlag(void);
void XenoActorClearHoldFlag(void);
void uwamonoAttachPlayerParts(void);
void uwamonoStoreCurrent(void);
void cameraRestoreState(void);
void func_A9D7B8(unsigned char *script);
void func_A9C9C0(void);
void scriptRegisterModules(void);

/* Puts back what func_001D5440 saved after a battle: the map flags, the light sets, the elevator masks and the
 * player rates; then gives every record that had an actor a new one, reloaded from its resources and restored
 * from the record's saved state, and places the save-point models again. */
void func_001D5820(void)
{
    void *data;
    FieldResourceBlock *resource;
    FieldMotions *motions;
    FieldRecord *record;
    FieldActor *actor;
    ObjTableEntry *entry;
    int i;
    int set;
    int j;
    int k;
    int bit;
    int flag;

    data = 0;
    WorkEnd = gameWork.workEnd;
    mapObjectResourcesLoad();
    mainSwitchId(gameWork.loadSlot, 0);
    D_002B7C88 = gameWork.savedMapBits[0];
    D_002B7CA8 = gameWork.savedMapBits[1];
    for (set = 0; set < 16; set++) {
        for (j = 0; j < 4; j++) {
            for (k = 0; k < 3; k++) {
                gameWork.lights.direction[set][k][j] = gameWork.savedLights.direction[set][k][j];
                gameWork.lights.color[set][k][j] = gameWork.savedLights.color[set][k][j];
            }
            gameWork.lights.ambient[set][j] = gameWork.savedLights.ambient[set][j];
            gameWork.lights.type4Direction[set][j] = gameWork.savedLights.type4Direction[set][j];
        }
        gameWork.lights.type4Value[set] = gameWork.savedLights.type4Value[set];
    }
    elvActiveMask = gameWork.elevatorMask;
    for (bit = 0; bit < 32; bit++) {
        flag = 1 << bit;
        if (!(gameWork.keepOffMask & flag)) {
            elvKeepOffFlags[bit] = 0;
        }
    }
    gameWork.playerRate = gameWork.playerRateSaved;
    gameWork.playerScale = gameWork.playerScaleSaved;
    sefStartClipTestEnabled();
    sefRestoreEffectRecords();
    XenoActorPoolInitDefault();
    for (i = 0; i < 256; i++) {
        record = &uwamonoRecords[i];
        if (record->actor) {
            do {
            } while (GameResLoadID(record->key, &data, 0, 0) == -1);
            resource = data;
            actor = XenoActorAlloc();
            motions = record->motions;
            record->actor = actor;
            do {
            } while (GameResLoadID(0x30041, &data, 0, 0) == -1);
            actor->rsrc[XAct_Rsrc_MDL] = resource->pack;
            actor->rsrc[XAct_Rsrc_TEX] = resource->effect;
            actor->rsrc[XAct_Rsrc_XHR] = resource->hierarchy;
            actor->rsrc[XAct_Rsrc_XAP] = resource->slotTable;
            actor->lookLink = resource->lookLink;
            actor->sharedResource = data;
            if (motions && motions->resourceId != -1) {
                GameResLoadID(motions->resourceId, &data, 0, 0);
                actor->rsrc[XAct_Rsrc_5] = (int) data;
            }
            XenoActorAllocMatrices(actor);
            XenoActorSetChannelValue(actor, 0.0f);
            XenoActorSetMode(actor, 0);
            actor->position = record->position.vector;
            if (uwamonoCurrentIndex == i) {
                actor->direction.y = actor->rotation.y = record->rotation.coords[1];
            } else {
                actor->rotation.y = actor->direction.y = record->rotation.coords[1];
            }
            actor->kind = record->key;
            actor->recordIndex = i;
            actor->probe.probeRadius = actor->radius = record->extent.coords[0];
            actor->height = record->extent.coords[1];
            actor->probe = record->saved.probe;
            if (motions) {
                actor->probe.ground = mapFindBlockEntry(256, motions->groundId);
            }
            for (j = 0; j < 3; j++) {
                actor->light.tracks.midpointA[j][0] = record->followA[j][0];
                actor->light.tracks.midpointA[j][1] = record->followA[j][1];
                actor->light.tracks.midpointA[j][2] = record->followA[j][2];
                actor->light.tracks.midpointA[j][3] = record->followA[j][3];
                actor->light.tracks.midpointB[j][0] = record->followB[j][0];
                actor->light.tracks.midpointB[j][1] = record->followB[j][1];
                actor->light.tracks.midpointB[j][2] = record->followB[j][2];
                actor->light.tracks.midpointB[j][3] = record->followB[j][3];
            }
            actor->light.tracks.midpointC[0] = record->followC[0];
            actor->light.tracks.midpointC[1] = record->followC[1];
            actor->light.tracks.midpointC[2] = record->followC[2];
            actor->light.tracks.midpointC[3] = record->followC[3];
            actor->light.tracks.midpointD[0] = record->followD[0];
            actor->light.tracks.midpointD[1] = record->followD[1];
            actor->light.tracks.midpointD[2] = record->followD[2];
            actor->light.tracks.midpointD[3] = record->followD[3];
            actor->modeValue = record->blendEnd;
            actor->light = record->saved.light;
            actor->color = record->saved.color;
            actor->colorBlock = record->saved.colorBlock;
            actor->fog[0] = record->saved.fog[0];
            actor->fog[1] = record->saved.fog[1];
            actor->fog[2] = record->saved.fog[2];
            actor->fog[3] = record->saved.fog[3];
            actor->drawMode = record->saved.drawMode;
            actor->drawFlags = record->saved.drawFlags;
            actor->modeValue = record->saved.modeValue;
            actor->modeValue4 = record->saved.modeValue4;
            if (record->effectOwner >= 0) {
                sefSetRecordOwner(uwamonoRecords[record->effectOwner].effect, record->actor);
            }
            XenoActorSetModeInSlot(actor, record->saved.unmodeled_5d8, record->saved.mode);
            actor->flags = record->saved.flags;
            actor->transformScale = record->saved.transformScale;
            actor->unmodeled_5d8 = record->saved.unmodeled_5d8;
            actor->requestedMode = record->saved.requestedMode;
            actor->motion.speed = record->saved.motionSpeed;
            actor->motion.range[0] = record->saved.motionRange[0];
            actor->motion.range[1] = record->saved.motionRange[1];
            actor->flags |= 0x200;
        }
        if (record->kind == 2) {
            if (record->type == -1) {
                if (record->key != -1) {
                    entry = (ObjTableEntry *) ((unsigned char *) mapObjectTable + mapObjectTable->entry_offset) + record->key;
                    record->savedMatrix = *entry;
                    record->transform = entry;
                    record->pose = *entry;
                } else {
                    record->transform = &record->savedMatrix;
                }
            } else {
                record->transform = &record->savedMatrix;
            }
            if (record->effectOwner >= 0) {
                sefSetRecordOwnerMarked(uwamonoRecords[record->effectOwner].effect, record);
            }
        }
    }
    XenoActorSetHoldFlag();
    XenoActorClearHoldFlag();
    uwamonoAttachPlayerParts();
    uwamonoRecords[uwamonoCurrentIndex].actor->probe.hitFlags |= 0x1000;
    uwamonoStoreCurrent();
    cameraRestoreState();
    gameWork.cameraFlags |= 0x20;
    func_A9D7B8(D_003C6DC0);
    func_A9C9C0();
    scriptRegisterModules();
}

/* Copies the form's saved player parameters back (the reverse of uwamonoLoadCurrent) and gives the
 * current record and its actor the form's scale and size. */
void uwamonoStoreCurrent(void)
{
    uwamonoRecords[uwamonoCurrentIndex].scale.coords[0] = uwamonoRecords[uwamonoCurrentIndex].scale.coords[1] = uwamonoRecords[uwamonoCurrentIndex].scale.coords[2] = gameWork.formScale;
    gameWork.frameScale = gameWork.frameScaleSaved;
    gameWork.playerParams[0][0] = gameWork.playerParams[0][1];
    gameWork.playerParams[1][0] = gameWork.playerParams[1][1];
    gameWork.playerParams[2][0] = gameWork.playerParams[2][1];
    gameWork.playerParams[3][0] = gameWork.playerParams[3][1];
    gameWork.playerParams[4][0] = gameWork.playerParams[4][1];
    gameWork.climbFrames = gameWork.climbFramesSaved;
    uwamonoRecords[uwamonoCurrentIndex].actor->probe.probeRadius = uwamonoRecords[uwamonoCurrentIndex].actor->radius = uwamonoRecords[uwamonoCurrentIndex].extent.coords[0] = gameWork.formSize;
}

#define NULL ((void *) 0)

/* A 16-byte row of the table at uwamonoEntries (tu083.c has the same fields). */
typedef struct UwamonoEntry {
    float x;
    float y;
    float z;
    short angle;
    unsigned short key;
} UwamonoEntry;

extern UwamonoEntry *uwamonoEntries;

/* The position block ptState points to: only the position is read here. */
typedef struct PtPosition {
    unsigned char unmodeled_00[0x10];
    float x;
    float y;
    float z;
} PtPosition;

/* ptState as this file sees it: its first word points at the party position block. */
typedef struct UwamonoPartyView {
    PtPosition *position;
    unsigned char unmodeled_04[0x60 - 0x04];
} UwamonoPartyView;

extern UwamonoPartyView ptState;

/* The clear colour the frame clear reads (tu127.c has the same layout). */
typedef struct ClearEnvColor {
    u8 unmodeled_00[0x20];
    u32 color_r;
    u32 color_g;
    u32 color_b;
    u32 color_a;
} ClearEnvColor;

extern ClearEnvColor ClearEnv;
extern PadRecord PadData[2];
extern u8 *WorkEnd;
extern int fadeUpdateHook[];
extern short uwamonoLadderState;
extern int D_0069B978;
extern union {
    float coords[4];
    Vector4 vector;
} D_002B5D80;
extern char D_00685180[];
extern char D_00685190[];
extern char D_006851A0[];
extern char D_006851B0[];
extern char D_006851C0[];

/* Callees of func_001D6070 (declared where their owners do not declare them for this TU). */
void padApplySaveSettings(void);
void GameSynopsis(void);
void XenoActorPoolInitDefault(void);
void uwamonoResetRecords(int index, int flag);
void func_001EEF60(void);
void func_001EEF98(void);
void resInitPathHeader(unsigned int a, unsigned int b);
void sefStartWithDefaultMemory(void);
void sefStartClipTestEnabled(void);
void seBankLoad(int bank, char *name, unsigned int size);
void sefStartEffectKeyDefault(int a, int key);
int xglCdServiceIfPending(void);
void xglRenderClearSkipOff(void);
void xglRenderClearSkipOn(void);
void xglCullingMapInit(void);
void yg2PathResetAll(void);
void gifStateDisableEffect(void);
void func_A99E68(void);
void saveReadImage(void);
void saveReadGameWork(void);
void mapObjectResourcesLoad(void);
void tiGlobalCollClearHandle(void);
void msgResetAll(void);
void *GameResBufferAlloc(unsigned int size);
void resPathSlotsInit(void);
void resResetDescriptorTables(void);
int ptFindSlot(int slot);
void ptSwapSlots(int a, int b);
void ptEnterSplit(void);
void ptLoadPartyModels(void);
void ptApplyCurrentModel(void);
void resSelectSlot(int slot);
int func_001E95C8(void);
void fadeReset(void);
void fadeClear(void);
void fadeStartIn(int frames);
void xglRenderClearDepth(void);
void xglRenderDispOn(void);
void xglStudioInit(void);
void xglStudioChange(int studio);
void xglStudioMainCameraInit(void);
XglCamera *xglStudioGetCamera2(int index);
void xglCameraSetPadPort(XglCamera *camera, int port);
void xglCameraInit(XglCamera *camera);
void xglCameraMove(XglCamera *camera);
void func_001B2724(void);
void xglRenderCpUDispOn(void);
void xglFontPrint(int x, int y, int flags, const char *text);
void func_001EECE0(int a, int b);
void cameraReset(void);
void uwamonoReleaseHandles(void);
void vibrationResetFade(void);
void seSetMuted(int muted, int flag);
int printf(const char *format, ...);
void msgUpdateTimed(void);
void msgPanelClear(void);
void msgPanelOpen(void);
void msgPanelSetTimecode(int a, int b, int minutes, int seconds);
void msgClose(void);
void resJumpPointsUpdate(void);
void padDebugPrintDirection(int direction);
void pauseStart(void);
void sefSetRenderHooks(void);
void xglClockAccumulate(void);
void xglClockSampleDayTime(void);
void func_001FCBF8(void);
void pxySetMode3Flag(int flag);
void func_001BA75C(void);
void func_001BBBD8(void);
void func_001BA68C(void);
void func_001BA358(int mode);
void func_001BAA6C(void);
int uwamonoFindEntry(int key);
float uwamonoGetEntryPose(int key, float *out);
void uwamonoHaltPursuit(int key);
void uwamonoSendEvent(int kind, int id, int value);
void uwamonoReleaseChildren(int index);
void uwamonoPlayerUpdate(void);
void uwamonoResetPoints(void);
void func_A9C9D8(void);
void pxyBegin(void *block);
void pxyEnd(void *block);
void DispMapObject(void);
void cameraUpdateFrame(void);
void mainDrawObjects(void);
void sefExecEffect(void);
void vibrationStepFade(void);
void XenoActorDoit(void);
void XenoActorDisp(void);
void resResetScriptState(void);
void cameraSaveState(void);
void cameraRestoreState(void);
void soundSetMuted(int muted);
void menuSceneRunLoop(void);
void menuSceneRunKeyCheck(void);
int func_001FFB70(void);
void saveScreenRun(int mode);
void soundSlotSetLevelIfUnset(int slot, int level);
void seVoiceStartDefault(unsigned int id);
void soundSlotsRestart(void);
void soundRestart(void);
void func_001394B8(int a, int b);
void func_001D5440(void);
void func_00246EB0(void);
void xglFontFlush(void);
void sceGsSyncPath(int a, int b);
void xglRenderCopyDisp2Draw(void);
void func_A99FB0(void);
void func_A99FD8(void);
void func_A9A140(void);
int func_A97880(void);
void battleResultScene(int mode);
void fontObjClearSlots(void);
void func_00251B18(void);
void resResetActState(void);
void resResetAllKinds(void);
void func_001D5820(void);
void sefReleaseRecordHandle(void *handle);
void sefResetWorkTablesAll(void);
void msgStateReset(void);
void xglSleep(void);
void xglRenderGlobalFadeReset(void);
void soundReset(void);
void soundSlotSetLevel(int slot, int level);
void soundSetDefaults(void);
void xglRenderCpUDispOff(void);
void GameOver(void);
int xglRenderGlobalFadeGet(void);


void func_001D6070(void)
{
    int i;
    int studio;
    int choice;
    int result;
    int found;
    int pressed;
    float pose;
    float zero;
    XglCamera *camera;
    FieldRecord *rec;
    FieldActor *actor;
    UwamonoEntry *entry;

    padApplySaveSettings();
    GameSynopsis();
    XenoActorPoolInitDefault();
    uwamonoResetRecords(-1, 0);
    gameWork.control &= ~0x100000;
    gameWork.encounterReaction = gameWork.entryKey = -1;
    gameWork.timerArmed = 1;
    gameWork.blocked = 0;
    func_001EEF60();
    func_001EEF98();
    zero = 0.0f;
    D_002B5D80.coords[2] = zero;
    D_002B5D80.coords[1] = zero;
    D_002B5D80.coords[0] = zero;
    resInitPathHeader(0x1000000, 0x1000000);
    seBankLoad(2, "CFS_SE", 0x7D000);
    sefStartWithDefaultMemory();
    sefStartClipTestEnabled();
    sefStartEffectKeyDefault(0, 0x10000033);
    sefStartEffectKeyDefault(0, 0x10000035);
    sefStartEffectKeyDefault(0, 0x10000037);
    sefStartEffectKeyDefault(0, 0x10000021);
    sefStartEffectKeyDefault(0, 0x1000003F);
    sefStartEffectKeyDefault(0, 0x10000040);
    do {
    } while (xglCdServiceIfPending() != 0);
    xglRenderClearSkipOff();
    xglCullingMapInit();
    yg2PathResetAll();
    gifStateDisableEffect();
    func_A99E68();
    if (gameWork.loadStage != 0) {
        saveReadImage();
        saveReadGameWork();
        gameWork.loadStage = 1;
    }
    gameWork.loadSlot = -1;
    gameWork.workEnd = WorkEnd;
    uwamonoCurrentIndex = -1;
    gameWork.mapLoaded = 0;
    mapObjectResourcesLoad();
    tiGlobalCollClearHandle();
    msgResetAll();
    gameWork.resBuffer = GameResBufferAlloc(0x12000);
    resPathSlotsInit();
    resResetDescriptorTables();
    if (gameWork.loadStage == 0) {
        ptSwapSlots(7, ptFindSlot(13));
        ptEnterSplit();
    }
    ptLoadPartyModels();
    ptApplyCurrentModel();
    resSelectSlot(1);
    if (func_001E95C8() != 0) {
        return;
    }
    fadeReset();

    for (;;) {
        xglRenderClearDepth();
        ClearEnv.color_r = 0;
        ClearEnv.color_g = 0;
        ClearEnv.color_b = 0;
        ClearEnv.color_a = 128;
        xglRenderDispOn();
        xglStudioInit();
        for (studio = 0; studio < 4; studio++) {
            xglStudioChange(studio);
            xglStudioMainCameraInit();
            camera = xglStudioGetCamera2(0);
            camera->state = 2;
            xglCameraSetPadPort(camera, 0);
            for (i = 1; i < 8; i++) {
                camera = xglStudioGetCamera2(i);
                xglCameraInit(camera);
                xglCameraMove(camera);
                xglCameraSetPadPort(camera, 0);
                camera->state = 2;
            }
        }
        func_001B2724();
        xglRenderCpUDispOn();
        xglFontPrint(0, 0, 0, "$init;");
        gameWork.control &= ~0x401;
        gameWork.sceneFlag = 0;
        gameWork.blocked = 0;
        func_001EECE0(gameWork.nextScene, 1);
        XenoActorPoolInitDefault();
        uwamonoResetRecords(-1, 0);
        if (uwamonoCurrentIndex == -1) {
            printf(D_00685180);
            DEBUG_BREAK();
        } else {
            ptLoadPartyModels();
            ptApplyCurrentModel();
        }
        cameraReset();
        uwamonoReleaseHandles();
        vibrationResetFade();
        seSetMuted(1, 0);
        uwamonoRecords[uwamonoCurrentIndex].actor->position.x = D_002B5D80.coords[0];
        uwamonoRecords[uwamonoCurrentIndex].actor->position.y = D_002B5D80.coords[1];
        uwamonoRecords[uwamonoCurrentIndex].actor->position.z = D_002B5D80.coords[2];
        gameWork.encounterReaction = -1;
        uwamonoRecords[uwamonoCurrentIndex].actor->drawFlags |= 4;
        gameWork.sceneMode = 1;
        gameWork.scale.z = gameWork.scale.y = gameWork.scale.x = 1.0f;
        gameWork.control &= ~0x80000;
        gameWork.control &= ~0x800000;
        gameWork.control &= ~0x10000;
        gameWork.control &= ~0x20000000;
        gameWork.control &= ~0x200000;
        if (gameWork.loadStage != 0) {
            ptLoadPartyModels();
            ptApplyCurrentModel();
            gameWork.loadStage = 2;
        }
        func_001FCBF8();
        fadeStartIn(30);
        if (gameWork.control & 0x100000) {
            fadeClear();
        }
        while (1) {
            if ((PadData[0].heldAndPressed & 0x0800010F) == 0x0800010F) {
                gameWork.control |= 0x10000000;
                break;
            }
            pxySetMode3Flag(1);
            if (!(gameWork.control & 0x800000)) {
                func_001BA75C();
            }
            if (gameWork.control & 0x80000) {
                func_001BBBD8();
            }
            if (gameWork.control & 0x200000) {
                xglRenderClearSkipOn();
            } else {
                xglRenderClearSkipOff();
            }
            func_001BA68C();
            func_001BA358(gameWork.sceneMode);
            if (gameWork.control & 1) {
                func_001BAA6C();
            }
            if ((gameWork.control & 0x20) && gameWork.entryKey != -1) {
                if (gameWork.entryKey == -2) {
                    uwamonoRecords[uwamonoCurrentIndex].position.coords[0] = uwamonoRecords[uwamonoCurrentIndex].actor->position.x = ptState.position->x;
                    uwamonoRecords[uwamonoCurrentIndex].position.coords[1] = uwamonoRecords[uwamonoCurrentIndex].actor->position.y = ptState.position->y;
                    uwamonoRecords[uwamonoCurrentIndex].position.coords[2] = uwamonoRecords[uwamonoCurrentIndex].actor->position.z = ptState.position->z;
                    uwamonoRecords[uwamonoCurrentIndex].actor->position.w = 1.0f;
                    uwamonoRecords[uwamonoCurrentIndex].rotation.coords[1] = uwamonoRecords[uwamonoCurrentIndex].actor->rotation.y = uwamonoRecords[uwamonoCurrentIndex].actor->direction.y = 0.0f;
                } else {
                    found = uwamonoFindEntry(gameWork.entryKey);
                    if (found != -1) {
                        uwamonoRecords[uwamonoCurrentIndex].position.coords[0] = uwamonoRecords[uwamonoCurrentIndex].actor->position.x = uwamonoEntries[found].x;
                        uwamonoRecords[uwamonoCurrentIndex].position.coords[1] = uwamonoRecords[uwamonoCurrentIndex].actor->position.y = uwamonoEntries[found].y;
                        uwamonoRecords[uwamonoCurrentIndex].position.coords[2] = uwamonoRecords[uwamonoCurrentIndex].actor->position.z = uwamonoEntries[found].z;
                        uwamonoRecords[uwamonoCurrentIndex].actor->position.w = 1.0f;
                        pose = uwamonoGetEntryPose(gameWork.entryKey, NULL);
                        uwamonoRecords[uwamonoCurrentIndex].rotation.coords[1] = uwamonoRecords[uwamonoCurrentIndex].actor->rotation.y = uwamonoRecords[uwamonoCurrentIndex].actor->direction.y = pose;
                        uwamonoRecords[uwamonoCurrentIndex].actor->drawFlags &= ~4;
                    } else {
                        uwamonoRecords[uwamonoCurrentIndex].actor->position.x = uwamonoRecords[uwamonoCurrentIndex].actor->position.y = uwamonoRecords[uwamonoCurrentIndex].actor->position.z = 0.0f;
                        uwamonoRecords[uwamonoCurrentIndex].actor->position.w = 1.0f;
                        uwamonoRecords[uwamonoCurrentIndex].rotation.coords[1] = uwamonoRecords[uwamonoCurrentIndex].actor->rotation.y = uwamonoRecords[uwamonoCurrentIndex].actor->direction.y = 0.0f;
                    }
                }
                gameWork.control &= ~0x400;
                uwamonoRecords[uwamonoCurrentIndex].actor->probe.hitFlags |= 0x1000;
                gameWork.cameraFlags |= 0x80;
            }
            if (gameWork.loadStage != 0) {
                int slot;

                for (slot = 0; slot < 256; slot++) {
                    if (uwamonoRecords[slot].kind == 2 && (unsigned int)(uwamonoRecords[slot].type - 7) < 2) {
                        uwamonoRecords[uwamonoCurrentIndex].position.coords[0] = uwamonoRecords[uwamonoCurrentIndex].actor->position.x = uwamonoRecords[slot].position.coords[0];
                        uwamonoRecords[uwamonoCurrentIndex].position.coords[1] = uwamonoRecords[uwamonoCurrentIndex].actor->position.y = uwamonoRecords[slot].position.coords[1];
                        uwamonoRecords[uwamonoCurrentIndex].position.coords[2] = uwamonoRecords[uwamonoCurrentIndex].actor->position.z = uwamonoRecords[slot].position.coords[2];
                        uwamonoRecords[uwamonoCurrentIndex].actor->position.w = 1.0f;
                        uwamonoRecords[uwamonoCurrentIndex].rotation.coords[1] = uwamonoRecords[uwamonoCurrentIndex].actor->rotation.y = uwamonoRecords[uwamonoCurrentIndex].actor->direction.y = 0.0f;
                        gameWork.loadStage--;
                        gameWork.control &= ~0x400;
                        uwamonoRecords[uwamonoCurrentIndex].actor->probe.hitFlags |= 0x1000;
                        uwamonoRecords[uwamonoCurrentIndex].actor->drawFlags &= ~4;
                        gameWork.cameraFlags |= 0x80;
                        break;
                    }
                }
            }

            if (!(gameWork.control & 1)) {
                func_A9C9D8();
                if (gameWork.control & 0x10000000) {
                    break;
                }
                if (gameWork.control & 0x08000000) {
                    break;
                }
            }
            if (!(gameWork.control & 0x200)) {
                pxyBegin(D_00685190);
                DispMapObject();
                pxyEnd(D_00685190);
                uwamonoPlayerUpdate();
                cameraUpdateFrame();
                mainDrawObjects();
            }
            if (!(gameWork.control & 1)) {
                if (!(gameWork.control & 0x200)) {
                    pxyBegin(D_006851A0);
                    sefExecEffect();
                    pxyEnd(D_006851A0);
                }
                vibrationStepFade();
                if (!(gameWork.control & 0x200) && !(D_0069B978 > 0 || (gameWork.control & 0x20000000))) {
                    if (!(gameWork.control & 0x40008400) && uwamonoLadderState == 0 && gameWork.encounterReaction == -1) {
                        pressed = PadData[0].pressed;
                        if (pressed & 0x10) {
                            resResetScriptState();
                            cameraSaveState();
                            soundSetMuted(1);
                            menuSceneRunLoop();
                            soundSetMuted(0);
                            cameraRestoreState();
                        } else if (pressed & 0x20) {
                            choice = func_001FFB70();
                            if ((unsigned int)(choice - 7) < 2) {
                                resResetScriptState();
                                cameraSaveState();
                                soundSetMuted(1);
                                saveScreenRun(0);
                                soundSetMuted(0);
                                cameraRestoreState();
                            }
                            if (choice == 9) {
                                menuSceneRunKeyCheck();
                            }
                        }
                    }
                    msgUpdateTimed();
                }
                if (gameWork.control & 0x10000000) {
                    break;
                }
                if (gameWork.timerArmed == 0) {
                    gameWork.timerFrames--;
                    if (gameWork.timerFrames < 0) {
                        gameWork.timerFrames = 0;
                        gameWork.timerArmed = 1;
                        uwamonoSendEvent(7, 0, 0);
                    }
                } else {
                    msgPanelClear();
                }
                padDebugPrintDirection(12);
            } else {
                sefSetRenderHooks();
                pauseStart();
                xglClockAccumulate();
            }
            if (gameWork.timerArmed == 0) {
                msgPanelSetTimecode(0, 0, gameWork.timerFrames / 30, gameWork.timerFrames % 30);
            } else {
                msgPanelClear();
            }
            msgClose();
            resJumpPointsUpdate();
            if (fadeUpdateHook[0] != 0 || gameWork.encounterReaction != -1 || (gameWork.control & 0x230)
                || gameWork.nextScene != -1 || uwamonoLadderState == 2 || uwamonoLadderState == 3
                || uwamonoLadderState == 7 || gameWork.pauseDelay != 0 || xglRenderGlobalFadeGet() < 0) {
                if (gameWork.pauseDelay != 0) {
                    gameWork.pauseDelay--;
                }
            } else if (PadData[0].pressed & 0x800) {
                gameWork.pauseDelay = 16;
                if (gameWork.control & 1) {
                    gameWork.control &= ~1;
                    seSetMuted(1, 0);
                    soundSlotSetLevelIfUnset(0, 0);
                    seVoiceStartDefault(0x2000011);
                } else {
                    xglClockSampleDayTime();
                    gameWork.control |= 1;
                    seVoiceStartDefault(0x2000011);
                    seSetMuted(1, 1);
                    soundSlotSetLevelIfUnset(0, 1);
                }
            }
            if (gameWork.nextScene != -1) {
                break;
            }
            if (!(gameWork.control & 0x200)) {
                if (gameWork.encounterReaction == -1) {
                    if (!(gameWork.control & 1)) {
                        pxyBegin(D_006851B0);
                        XenoActorDoit();
                        pxyEnd(D_006851B0);
                    }
                    pxyBegin(D_006851C0);
                    XenoActorDisp();
                    pxyEnd(D_006851C0);
                }
            }
            if (gameWork.encounterReaction == -1) {
                xglSleep();
                if (gameWork.encounterReaction == -1) {
                    continue;
                }
            }

            xglRenderClearDepth();
            xglSleep();
            resResetScriptState();
            soundRestart();
            func_001394B8(1, 0);
            func_001D5440();
            vibrationResetFade();
            func_00246EB0();
            XenoActorPoolInitDefault();
            fadeClear();
            xglFontFlush();
            sceGsSyncPath(0, 0);
            xglRenderCopyDisp2Draw();
            func_A99FB0();
            func_A99FD8();
            func_A9A140();
            gifStateDisableEffect();
            if (gameWork.control & 0x10000000) {
                soundSlotsRestart();
                break;
            }
            result = func_A97880();
            if (result == 2) {
                soundSlotsRestart();
                gameWork.control |= 0x88000000;
                break;
            }
            if (result != 5) {
                battleResultScene(0);
                fontObjClearSlots();
            }
            if (gameWork.control & 0x10000000) {
                break;
            }
            func_00251B18();
            soundSlotsRestart();
            if (gameWork.mapId < 6300) {
                resResetActState();
            } else {
                resResetAllKinds();
            }
            uwamonoResetPoints();
            func_001D5820();
            uwamonoReleaseHandles();
            msgResetAll();
            gameWork.encounterReaction = -1;
            gameWork.blocked = 90;
            gameWork.encounterFlags = 0;
            switch (func_A97880()) {
            case 1:
                uwamonoSendEvent(6, 0, uwamonoRecords[gameWork.encounterRecord].id);
                uwamonoReleaseChildren(gameWork.encounterRecord);
                break;
            case 2:
                uwamonoSendEvent(6, 1, uwamonoRecords[gameWork.encounterRecord].id);
                break;
            case 3:
                uwamonoSendEvent(6, 2, uwamonoRecords[gameWork.encounterRecord].id);
                break;
            case 4:
                uwamonoSendEvent(6, 3, uwamonoRecords[gameWork.encounterRecord].id);
                uwamonoReleaseChildren(gameWork.encounterRecord);
                break;
            case 5:
                uwamonoSendEvent(7, -1, -1);
                break;
            }
            gameWork.entryKey = -1;
            gameWork.releaseHandle = NULL;
            uwamonoHaltPursuit(-1);
            if (gameWork.timerArmed == 0) {
                msgPanelOpen();
            }
            actor = uwamonoRecords[uwamonoCurrentIndex].actor;
            if (actor != NULL) {
                actor->drawFlags |= 4;
            }
            if (gameWork.releaseHandle != NULL) {
                sefReleaseRecordHandle(gameWork.releaseHandle);
                gameWork.releaseHandle = NULL;
            }
        }
        gameWork.loadStage = 0;
        soundSlotsRestart();
        resResetScriptState();
        if (gameWork.control & 0x18000000) {
            break;
        }
        sefResetWorkTablesAll();
        msgStateReset();
        xglSleep();
        xglSleep();
        fadeClear();
        if (gameWork.nextScene == -1) {
            break;
        }
    }
    fadeReset();
    xglRenderGlobalFadeReset();
    tiGlobalCollClearHandle();
    soundReset();
    soundSlotSetLevel(0, 0);
    soundSetDefaults();
    resSelectSlot(0);
    xglRenderCpUDispOff();
    XenoActorPoolInitDefault();
    sefStartClipTestEnabled();
    uwamonoResetRecords(-1, 0);
    xglCullingMapInit();
    xglRenderClearSkipOff();
    xglSleep();
    xglSleep();
    if (gameWork.control & 0x08000000) {
        GameOver();
    }
}
