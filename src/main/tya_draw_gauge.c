#include "common.h"
#include "debug_break.h"
#include "shared.h"

#define NULL ((void *) 0)

/* Gauge geometry (in pixels) and the filled / unfilled colours. */
typedef struct TyaGaugeArgs {
    short x;
    short y;
    unsigned int vertex_attribute;
    short width;
    short height;
    unsigned short current;
    unsigned short maximum;
    unsigned char filled_color[4];
    unsigned char unfilled_color[4];
} TyaGaugeArgs;

/* One GS vertex: X and Y in 12.4 fixed point, then the attribute word. */
typedef struct TyaGaugeVertex {
    unsigned int x;
    unsigned int y;
    unsigned int attribute;
    unsigned int unmodeled_0C;
} TyaGaugeVertex;

/* DMA/VIF/GIF packet of the two gauge sprites, uploaded as 9 quadwords. */
typedef struct TyaGaugePacket {
    unsigned int dma_vif_tag[4];
    unsigned int vif_gif_setup[4];
    unsigned int gif_control[4];
    unsigned int filled_color[4];
    TyaGaugeVertex filled_vertices[2];
    unsigned int unfilled_color[4];
    TyaGaugeVertex unfilled_vertices[2];
} TyaGaugePacket;

#define TYA_GAUGE_COORDINATE_SCALE 0x10
#define TYA_GAUGE_X_ORIGIN 0x6FF8
#define TYA_GAUGE_Y_ORIGIN 0x71F7

extern TyaGaugePacket D_002BD570;

void pxyAddEntry(int key, void *data, int qwc);

/* Draws a horizontal gauge filled to current / maximum of its width. */
void tyaDrawGauge(TyaGaugeArgs *gauge)
{
    int left_x;
    int top_y;
    int right_x;
    int bottom_y;
    int fill_width;
    int fill_x;

    left_x = (gauge->x * TYA_GAUGE_COORDINATE_SCALE) + TYA_GAUGE_X_ORIGIN;
    top_y = (gauge->y * TYA_GAUGE_COORDINATE_SCALE) + TYA_GAUGE_Y_ORIGIN;
    right_x = left_x + (gauge->width * TYA_GAUGE_COORDINATE_SCALE);
    bottom_y = top_y + (gauge->height * TYA_GAUGE_COORDINATE_SCALE);
    fill_width = (gauge->current * gauge->width) / gauge->maximum;
    if (fill_width < 0) {
        fill_width = 0;
    }
    if (gauge->width < fill_width) {
        fill_width = gauge->width;
    }
    fill_x = left_x + (fill_width * TYA_GAUGE_COORDINATE_SCALE);

    D_002BD570.filled_color[0] = gauge->filled_color[0];
    D_002BD570.filled_color[1] = gauge->filled_color[1];
    D_002BD570.filled_color[2] = gauge->filled_color[2];
    D_002BD570.filled_color[3] = gauge->filled_color[3];
    D_002BD570.filled_vertices[0].x = left_x;
    D_002BD570.filled_vertices[0].y = top_y;
    D_002BD570.filled_vertices[0].attribute = gauge->vertex_attribute;
    D_002BD570.filled_vertices[1].x = fill_x;
    D_002BD570.filled_vertices[1].y = bottom_y;
    D_002BD570.filled_vertices[1].attribute = gauge->vertex_attribute;

    D_002BD570.unfilled_color[0] = gauge->unfilled_color[0];
    D_002BD570.unfilled_color[1] = gauge->unfilled_color[1];
    D_002BD570.unfilled_color[2] = gauge->unfilled_color[2];
    D_002BD570.unfilled_color[3] = gauge->unfilled_color[3];
    D_002BD570.unfilled_vertices[0].x = fill_x;
    D_002BD570.unfilled_vertices[0].y = top_y;
    D_002BD570.unfilled_vertices[0].attribute = gauge->vertex_attribute;
    D_002BD570.unfilled_vertices[1].x = right_x;
    D_002BD570.unfilled_vertices[1].y = bottom_y;
    D_002BD570.unfilled_vertices[1].attribute = gauge->vertex_attribute;

    pxyAddEntry(4, &D_002BD570, 9);
}

/* Empty body; no caller is known. */
void func_001EECD8(void)
{
}

/* The 0x40-byte pose block a record's transform pointer refers to; copied whole
 * (eight doubleword moves), row[3] is the translation (init_uwamono_sys.c). */
typedef struct UwamonoPose {
    AlignedVector4 row[4];
} UwamonoPose;

/* One 0x870-byte record of uwamonoRecords (field names follow tu050.c's
 * MapObjectRecord; the wordNNN/shortNNN members are only known by their reset
 * value here). */
typedef struct UwamonoRecord {
    short id;                           /* +0x000 */
    unsigned char kind;                 /* +0x002: 0 free, 1/4 actor, 2 model, 5 kept on reset */
    unsigned char unmodeled_003;
    int key;                            /* +0x004 */
    int model;                          /* +0x008: -1 for none */
    unsigned char unmodeled_00c[4];
    AlignedVector4 position;            /* +0x010 */
    unsigned char unmodeled_020[0x10];
    AlignedVector4 unmodeled_030;           /* +0x030: reset to (0, 0, 0, 1) */
    AlignedVector4 rotation;            /* +0x040 */
    AlignedVector4 scale;               /* +0x050 */
    AlignedVector4 pivot;               /* +0x060 */
    float fade;                         /* +0x070: 0..100 */
    unsigned char unmodeled_074[0x200 - 0x74];
    AlignedVector4 tilt;                /* +0x200 */
    short moveEndMode;                     /* +0x210: -1 on reset */
    short moveStep;                     /* +0x212 */
    short moveCount;                    /* +0x214 */
    short turnStep;                     /* +0x216 */
    short turnCount;                    /* +0x218 */
    short scaleStep;                    /* +0x21A */
    short scaleCount;                   /* +0x21C */
    short fadeStep;                     /* +0x21E: -1 on reset */
    unsigned char unmodeled_220[0x248 - 0x220];
    int eventSlot;                        /* +0x248: -1 on reset */
    short eventTimer;                   /* +0x24C */
    unsigned char unmodeled_24e[2];
    int event;                          /* +0x250 */
    int eventState;                     /* +0x254 */
    int eventStateArg;                  /* +0x258 */
    short tiltStep;                     /* +0x25C */
    short tiltCount;                    /* +0x25E */
    signed char rotationOrder;          /* +0x260: 5 on reset */
    unsigned char unmodeled_261[3];
    UwamonoPose *transform;             /* +0x264 */
    unsigned char unmodeled_268[8];
    Matrix4 savedMatrix;                /* +0x270: xglMatrixStackSave target on reset */
    UwamonoPose pose;                   /* +0x2B0: copied to *transform by func_001EECE0 */
    Matrix4 matrix;                     /* +0x2F0 */
    int actor;                          /* +0x330: XenoActor handle, released on a mode-1 reset */
    void *motions;                      /* +0x334 */
    float size;                         /* +0x338: 1.0 on reset */
    short motion;                       /* +0x33C: 4 on reset */
    unsigned char unmodeled_33e[0x546 - 0x33E];
    short parent;                       /* +0x546: -1 when unset */
    unsigned char unmodeled_548[0x570 - 0x548];
    int flags;                          /* +0x570: 0xA on reset, 0x2 shown */
    int unmodeled_574;                        /* +0x574: 0 on reset */
    unsigned char unmodeled_578[8];
    AlignedVector4 extent;              /* +0x580: reset to (0, 0, 0, 1) */
    signed char searchMode;             /* +0x590 */
    unsigned char unmodeled_591;
    short elevatorMotion;               /* +0x592: -1 on reset */
    int unmodeled_594;                        /* +0x594: 0 on reset */
    unsigned char unmodeled_598[8];
    int unmodeled_5a0;                        /* +0x5A0: -1 on reset */
    int unmodeled_5a4;                        /* +0x5A4: -1 on reset */
    int unmodeled_5a8;                        /* +0x5A8: -1 on reset */
    unsigned char unmodeled_5ac[4];
    AlignedVector4 riderOffset;         /* +0x5B0: reset to (0, 0, 0, 1) */
    signed char talkMode;               /* +0x5C0 */
    unsigned char unmodeled_5c1[0x618 - 0x5C1];
    int lookLink;                       /* +0x618: -1 on reset */
    short unmodeled_61c;                     /* +0x61C: -1 on reset */
    unsigned char unmodeled_61e[0x62C - 0x61E];
    signed char riderSide;              /* +0x62C */
    unsigned char unmodeled_62d[3];
    int unmodeled_630;                        /* +0x630: -1 on reset */
    int unmodeled_634;                        /* +0x634: 0xFF on reset */
    void *effect;                       /* +0x638 */
    void *riders[2];                    /* +0x63C */
    int effectOwner;                        /* +0x644: -1 on reset */
    unsigned char unmodeled_648[0x870 - 0x648];
} UwamonoRecord;

/* A route node of SlotIdRecord (uwamonoAddRouteNode links them). The reset
 * writes it through the union: that keeps the compiler from moving the node's
 * stores past the link-count stores that follow them. */
typedef union RouteNode {
    float component[4];
    AlignedVector4 position;
} RouteNode;

/* One 0x40C0-byte record of D_002BD600 (tu083.c models the id only); the
 * unmodeled_ members are only known by their reset value. */
typedef struct SlotIdRecord {
    int id;                             /* +0x0000: -1 when free */
    short index;                        /* +0x0004: the record's own index */
    unsigned char unmodeled_0006[2];
    int unmodeled_0008;                       /* -1 on reset */
    unsigned char unmodeled_000c;
    unsigned char unmodeled_000d;
    short unmodeled_000e;
    AlignedVector4 unmodeled_0010;          /* x, y, z zeroed on reset */
    AlignedVector4 unmodeled_0020;          /* x, y, z zeroed on reset */
    AlignedVector4 unmodeled_0030;
    AlignedVector4 unmodeled_0040;
    short unmodeled_0050[7];
    unsigned char unmodeled_005e;
    unsigned char unmodeled_005f;
    short unmodeled_0060;
    char names[2][8];                   /* +0x0062 */
    short unmodeled_0072[2];
    unsigned char unmodeled_0076;
    unsigned char unmodeled_0077;
    short unmodeled_0078;
    short unmodeled_007a;
    int unmodeled_007c;
    short unmodeled_0080;                    /* -1 on reset */
    short unmodeled_0082;                    /* -1 on reset */
    unsigned char unmodeled_0084[0xC];
    float points[256][4];               /* +0x0090: x, y, z zeroed on reset */
    short unmodeled_1090;
    unsigned char unmodeled_1092[0xE];
    float normals[256][4];              /* +0x10A0: reset to (0, 0, 0, 1) */
    short unmodeled_20a0[256][4];            /* +0x20A0 */
    short unmodeled_28a0[256][4];            /* +0x28A0 */
    short unmodeled_30a0;
    short unmodeled_30a2;
    short unmodeled_30a4;
    short unmodeled_30a6;
    unsigned char unmodeled_30a8[8];
    RouteNode routeNodes[64];           /* +0x30B0: reset to (0, 0, 0, 1) */
    short routeLinks[64][4];            /* +0x34B0: per node, the linked node ids */
    short routeLinkCounts[64];          /* +0x36B0 */
    short routeParents[64];             /* +0x3730: -1 for none (uwamonoAddRouteNode) */
    short routeNodeCount;               /* +0x37B0 */
    unsigned char unmodeled_37b2[0xE];
    AlignedVector4 unmodeled_37c0[128];    /* +0x37C0: only x zeroed on reset */
    short unmodeled_3fc0;                    /* -1 on reset */
    short unmodeled_3fc2;
    int unmodeled_3fc4[7];                   /* +0x3FC4 */
    AlignedVector4 unmodeled_3fe0;          /* x, y, z zeroed on reset */
    int unmodeled_3ff0;
    short unmodeled_3ff4;                    /* -1 on reset */
    unsigned char unmodeled_3ff6[0xA];
    float unmodeled_4000;
    unsigned char unmodeled_4004[0xC];
    int unmodeled_4010;                       /* -1 on reset */
    int unmodeled_4014;                       /* -1 on reset */
    int unmodeled_4018;                       /* -1 on reset */
    unsigned char unmodeled_401c;             /* 1 on reset */
    unsigned char unmodeled_401d;
    short unmodeled_401e;
    int unmodeled_4020;                       /* 99 on reset */
    int unmodeled_4024;                       /* -1 on reset */
    short unmodeled_4028;
    short unmodeled_402a;                    /* -1 on reset */
    unsigned char unmodeled_402c[4];
    AlignedVector4 unmodeled_4030;          /* x, y, z zeroed on reset */
    short unmodeled_4040;
    short unmodeled_4042[8];                /* -1 on reset */
    unsigned char unmodeled_4052[2];
    int unmodeled_4054[4];                   /* -1 on reset */
    short unmodeled_4064;
    unsigned char unmodeled_4066[2];
    int unmodeled_4068;
    int unmodeled_406c;
    short unmodeled_4070;
    unsigned char unmodeled_4072[2];
    int unmodeled_4074;                       /* -1 on reset */
    int unmodeled_4078[16];                  /* +0x4078: identity order on reset */
    unsigned char unmodeled_40b8[8];
} SlotIdRecord;

/* The game work block at gameWork as this file uses it: the state and map id
 * words (tu050.c MainWork.state, tu110.c SaveGameWork.mapId) and the three
 * 512-byte runs func_001EEF60 and func_001EEF98 clear. */
typedef struct SceneWork {
    unsigned char unmodeled_0000[0x14];
    int state;                          /* +0x0014: -1 after a scene load */
    int mapId;                          /* +0x0018 */
    unsigned char unmodeled_001c[0x1520 - 0x1C];
    unsigned char clearedRuns[3][0x200]; /* +0x1520 */
} SceneWork;

extern UwamonoRecord uwamonoRecords[];
extern int uwamonoCurrentIndex;
extern SlotIdRecord D_002BD600[];
extern SceneWork gameWork;
extern unsigned char D_003C6DC0[];      /* the scene file buffer */
extern char D_006895C0[];               /* "sc0000.sb": the digits are rewritten */
extern const char D_0069D1D0[];         /* "cf\\%s" */
extern const char D_006895D0[];         /* "cf\\xx\\%sl" */
extern int D_003E7AF8[];
extern float D_0033EE90[];
extern int D_0069D1D8;
extern unsigned short *DataSpline;
/*
 * Declared as an incomplete array, not a scalar: cc1 -G8 would otherwise
 * classify a plain scalar extern as small data and emit a gp-relative load,
 * but the original reads this word through absolute hi/lo instructions.
 */
extern unsigned short *D_002B7BBC[];

void func_A9C938(void);
void scriptRegisterModules(void);
void sefSetCurrentOwner(int owner);
void resApplyCfFile(int id);
int func_002AA5D8(char *destination, const char *format, ...);
int xglCdReadFile(const char *name, void *buffer, int mode, int flags);
int xglGetLanguageCode(char *code);
int func_A9CEE8(unsigned char *scene, unsigned char *end);
void func_A9CAD0(unsigned char *scene);
void uwamonoResetRecords(int index, signed char mode);
void XenoActorRelease(int handle);
void uwamonoClearMoveResetWord(int index);
void uwamonoClearRotateResetWord(int index);
void xglMatrixStackUnit(void);
void xglMatrixStackSave(Matrix4 matrix);
void uwamonoResetPoints(void);

#define UWAMONO_RECORD_COUNT 256
#define SLOT_ID_RECORD_COUNT 32

/* Loads a uwamono scene: copies the pose of every model record to its
 * transform, registers the script modules, with mode 1 makes id the sef owner,
 * reads cf\scNNNN.sb and the language file cf\xx\scNNNNl (NNNN are the digits
 * of id), makes id the current map, shows the current record and resets the
 * records through uwamonoResetRecords(-1, 1). The scene parse failing is a
 * debugger trap (DEBUG_BREAK, class ee-assert-break). */
void func_001EECE0(int id, signed char mode)
{
    char path[0x20];
    char languagePath[0x20];
    char *sceneName = D_006895C0;
    int i;
    int slot;
    UwamonoRecord *record;
    int size;

    for (i = 0; i < UWAMONO_RECORD_COUNT; i++) {
        record = &uwamonoRecords[i];
        if (record->kind == 2 && record->transform != NULL) {
            *record->transform = record->pose;
        }
    }
    func_A9C938();
    scriptRegisterModules();
    if (mode == 1) {
        sefSetCurrentOwner(id);
        resApplyCfFile(id);
    }
    sceneName[2] = id / 1000 % 10 + '0';
    sceneName[3] = id / 100 % 10 + '0';
    sceneName[4] = id / 10 % 10 + '0';
    sceneName[5] = id % 10 + '0';
    func_002AA5D8(path, D_0069D1D0, sceneName);
    size = xglCdReadFile(path, D_003C6DC0, 0, 0);
    func_002AA5D8(languagePath, D_006895D0, sceneName);
    xglGetLanguageCode(languagePath + 3);
    xglCdReadFile(languagePath, D_003C6DC0 + size, 0, 0);
    if (!func_A9CEE8(D_003C6DC0, D_003C6DC0 + size)) {
        DEBUG_BREAK();
    }
    func_A9CAD0(D_003C6DC0);
    gameWork.mapId = id;
    uwamonoRecords[uwamonoCurrentIndex].flags |= 2;
    gameWork.state = -1;
    for (slot = 0; slot < 8; slot++) {
        D_003E7AF8[slot] = -1;
    }
    uwamonoResetRecords(-1, 1);
}

#define CLEAR_RUN_LENGTH 512

/* Clears the second and third 512-byte runs of the work block. */
void func_001EEF60(void)
{
    int i;

    for (i = 0; i < CLEAR_RUN_LENGTH; i++) {
        gameWork.clearedRuns[1][i] = 0;
        gameWork.clearedRuns[2][i] = 0;
    }
}

/* Clears the first 512-byte run of the work block. */
void func_001EEF98(void)
{
    int i;

    for (i = 0; i < CLEAR_RUN_LENGTH; i++) {
        gameWork.clearedRuns[0][i] = 0;
    }
}

/* Resets uwamono records: one record (index >= 0) or all 256 (index -1). Mode 1
 * keeps the kind-4 and kind-5 records and releases each reset record's actor
 * through XenoActorRelease. With index -1 the 32 D_002BD600 records are reset
 * too, except the one whose id is the kept kind-4 index, then uwamonoResetPoints
 * runs and the D_0033EE90 radii and D_0069D1D8 are cleared. */
void uwamonoResetRecords(int index, signed char mode)
{
    int kept;
    int i;
    int j;
    int k;
    int first;
    int last;
    UwamonoRecord *record;
    SlotIdRecord *slot;
    AlignedVector4 *vector;

    kept = -100;
    first = index;
    last = index + 1;
    if (index == -1) {
        first = 0;
        last = UWAMONO_RECORD_COUNT;
    }
    DataSpline = D_002B7BBC[0];
    for (i = first; i < last; i++) {
        record = &uwamonoRecords[i];
        if (mode == 1) {
            if (record->kind == 4) {
                kept = i;
                continue;
            }
            if (record->kind == 5) {
                continue;
            }
        }
        record->kind = 0;
        record->model = -1;
        record->parent = -1;
        record->tiltStep = -1;
        record->eventTimer = -1;
        record->fadeStep = -1;
        record->scaleStep = -1;
        record->turnStep = -1;
        record->moveStep = -1;
        record->motion = 4;
        record->rotationOrder = 5;
        record->moveEndMode = -1;
        record->position.x = record->position.y = record->position.z = 0.0f;
        record->position.w = 1.0f;
        record->rotation.x = record->rotation.y = record->rotation.z = 0.0f;
        record->rotation.w = 1.0f;
        record->scale.x = record->scale.y = record->scale.z = 1.0f;
        record->scale.w = 1.0f;
        record->pivot.x = record->pivot.y = record->pivot.z = record->fade = 0.0f;
        record->pivot.w = 1.0f;
        record->extent.x = record->extent.y = record->extent.z = 0.0f;
        record->extent.w = 1.0f;
        record->flags = 0xA;
        record->unmodeled_634 = 0xFF;
        record->searchMode = 0;
        record->size = 1.0f;
        record->unmodeled_030.x = record->unmodeled_030.y = record->unmodeled_030.z = 0.0f;
        record->unmodeled_030.w = 1.0f;
        record->tilt.x = record->tilt.y = record->tilt.z = 0.0f;
        record->tilt.w = 1.0f;
        record->unmodeled_574 = 0;
        record->riderSide = 0;
        record->unmodeled_630 = -1;
        record->transform = NULL;
        record->talkMode = 0;
        record->lookLink = -1;
        record->unmodeled_61c = -1;
        if (mode == 1 && record->actor != 0) {
            XenoActorRelease(record->actor);
        }
        record->actor = 0;
        record->motions = NULL;
        record->elevatorMotion = -1;
        record->unmodeled_594 = 0;
        record->riders[0] = NULL;
        record->riders[1] = NULL;
        record->effect = NULL;
        record->effectOwner = -1;
        record->unmodeled_5a0 = -1;
        record->riderOffset.x = record->riderOffset.y = record->riderOffset.z = 0.0f;
        record->riderOffset.w = 1.0f;
        uwamonoClearMoveResetWord(i);
        uwamonoClearRotateResetWord(i);
        record->unmodeled_5a8 = -1;
        record->unmodeled_5a4 = -1;
        record->eventSlot = -1;
        xglMatrixStackUnit();
        xglMatrixStackSave(record->savedMatrix);
    }
    if (index != -1) {
        return;
    }
    for (i = 0; i < SLOT_ID_RECORD_COUNT; i++) {
        slot = &D_002BD600[i];
        if (slot->id == kept) {
            continue;
        }
        slot->id = -1;
        slot->index = i;
        slot->unmodeled_0008 = -1;
        slot->unmodeled_000c = 0;
        slot->unmodeled_000e = 0;
        slot->unmodeled_0010.x = slot->unmodeled_0010.y = slot->unmodeled_0010.z = 0.0f;
        slot->unmodeled_0020.x = slot->unmodeled_0020.y = slot->unmodeled_0020.z = slot->unmodeled_0030.x = 0.0f;
        slot->unmodeled_0040.w = slot->unmodeled_0040.z = slot->unmodeled_0040.y = slot->unmodeled_0040.x =
            slot->unmodeled_0030.w = slot->unmodeled_0030.z = slot->unmodeled_0030.y = 0.0f;
        slot->unmodeled_0050[0] = 0;
        slot->unmodeled_0050[1] = 0;
        slot->unmodeled_0050[2] = 0;
        slot->unmodeled_0050[3] = 0;
        slot->unmodeled_0050[4] = 0;
        slot->unmodeled_0050[5] = 0;
        slot->unmodeled_0050[6] = 0;
        slot->unmodeled_005e = 0;
        slot->unmodeled_005f = 0;
        slot->unmodeled_0060 = 0;
        for (j = 0; j < 2; j++) {
            for (k = 0; k < 8; k++) {
                slot->names[j][k] = 0;
            }
            slot->unmodeled_0072[j] = 0;
        }
        slot->unmodeled_0076 = 0;
        slot->unmodeled_0077 = 0;
        slot->unmodeled_0078 = 0;
        slot->unmodeled_007a = 0;
        slot->unmodeled_1090 = 0;
        for (j = 0; j < 256; j++) {
            slot->points[j][0] = slot->points[j][1] = slot->points[j][2] = 0.0f;
        }
        slot->unmodeled_401c = 1;
        for (j = 0; j < 256; j++) {
            slot->normals[j][0] = slot->normals[j][1] = slot->normals[j][2] = 0.0f;
            slot->normals[j][3] = 1.0f;
            for (k = 0; k < 4; k++) {
                slot->unmodeled_20a0[j][k] = 0;
                slot->unmodeled_28a0[j][k] = 0;
            }
        }
        slot->unmodeled_30a6 = 0;
        slot->routeNodeCount = 0;
        for (j = 0; j < 64; j++) {
            slot->routeNodes[j].component[0] = slot->routeNodes[j].component[1] =
                slot->routeNodes[j].component[2] = 0.0f;
            slot->routeNodes[j].component[3] = 1.0f;
            slot->routeLinkCounts[j] = 0;
            slot->routeParents[j] = 0;
            for (k = 0; k < 4; k++) {
                slot->routeLinks[j][k] = 0;
            }
        }
        for (j = 0; j < 8; j++) {
            slot->unmodeled_4042[j] = -1;
        }
        for (j = 0; j < 4; j++) {
            slot->unmodeled_4054[j] = -1;
        }
        slot->unmodeled_30a0 = 0;
        slot->unmodeled_30a2 = 0;
        slot->unmodeled_30a4 = 0;
        /* The zero is a loop-local value: the original keeps it in a register
         * loaded just before the loop and walks the entries forwards. */
        vector = slot->unmodeled_37c0;
        for (j = 0; j < 128; j++, vector++) {
            float zero = 0.0f;

            vector->x = zero;
        }
        slot->unmodeled_3fc4[0] = 0;
        slot->unmodeled_3fc4[1] = 0;
        slot->unmodeled_3fc4[2] = 0;
        slot->unmodeled_3fc4[3] = 0;
        slot->unmodeled_3fc4[4] = 0;
        slot->unmodeled_3fc4[5] = 0;
        slot->unmodeled_3fc4[6] = 0;
        slot->unmodeled_3fe0.x = slot->unmodeled_3fe0.y = slot->unmodeled_3fe0.z = 0.0f;
        slot->unmodeled_3ff4 = -1;
        slot->unmodeled_4000 = 0.0f;
        slot->unmodeled_401e = 0;
        slot->unmodeled_007c = 0;
        slot->unmodeled_0080 = -1;
        slot->unmodeled_0082 = -1;
        slot->unmodeled_4024 = -1;
        slot->unmodeled_3ff0 = 0;
        slot->unmodeled_3fc0 = -1;
        slot->unmodeled_3fc2 = 0;
        slot->unmodeled_4018 = -1;
        slot->unmodeled_4014 = -1;
        slot->unmodeled_4010 = -1;
        slot->unmodeled_4064 = 0;
        slot->unmodeled_4020 = 99;
        for (j = 0; j < 16; j++) {
            slot->unmodeled_4078[j] = j;
        }
        slot->unmodeled_4074 = -1;
        slot->unmodeled_4028 = 0;
        slot->unmodeled_402a = -1;
        slot->unmodeled_4030.x = slot->unmodeled_4030.y = slot->unmodeled_4030.z = 0.0f;
        slot->unmodeled_4068 = 0;
        slot->unmodeled_406c = 0;
        slot->unmodeled_4070 = 0;
    }
    uwamonoResetPoints();
    for (i = 0; i < 10; i++) {
        D_0033EE90[i] = 0.0f;
    }
    D_0069D1D8 = 0;
}
