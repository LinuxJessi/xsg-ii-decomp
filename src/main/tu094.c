#include "common.h"
#include "shared.h"
#include "main/tu094.h"
#include "gs.h"

/* GIF tag fields of the scissor packet: NREG 1, end of packet. */
#define GIF_NREG_1 0x10000000
#define GIF_TAG_EOP 0x8000

typedef struct XglPacket XglPacket;

/* The cursor the scissor record is written through: the packet at +0x00 and two quadwords of direct data at +0x30. */
typedef struct ScissorCursor {
    XglPacket *packet;              /* +0x00 */
    unsigned char unmodeled_08[0x28];
    unsigned long long words[4];                   /* +0x30 */
} ScissorCursor;

/*
 * Local view of sRender (shared view: RenderFrameState in main/tu093.h). This file declares the record with its
 * larger extent, and the declared size of an extern is recorded in the object.
 */
typedef struct RenderFrameSize {
    unsigned char unmodeled_00[4];
    short frame_width;              /* +0x04 */
    short frame_height;             /* +0x06 */
    unsigned char unmodeled_08[0x54];
} RenderFrameSize;

extern RenderFrameSize sRender;
extern void sceVif1PkAddDirectDataN(XglPacket *packet, const void *data, int count);

/* Adds a GIF tag, the A+D register list and the GS scissor (0..width-1, 0..height-1) to the cursor's packet. */
void xglFontScissorReset(ScissorCursor *cursor) {
    unsigned long long *words;

    words = cursor->words;
    words[0] = ((unsigned long long) GIF_NREG_1 << 32) | (GIF_TAG_EOP | 1);
    words[1] = GIF_AD;
    words[2] = ((unsigned long long) (sRender.frame_width - 1) << 16) | ((unsigned long long) (sRender.frame_height - 1) << 48);
    words[3] = GS_SCISSOR_1;
    sceVif1PkAddDirectDataN(cursor->packet, words, 2);
}

extern int bgLayerPxyData[4];
extern int bgLayerTxyData[4];
extern unsigned char bgLayerState0[4];

/*
 * One background layer object: an XenoActor record (xenoActor.c), with only
 * the fields used here. The position and rotation are written through their
 * union view; the original reloads the layer slot after each of those stores.
 */
typedef struct BgLayer {
    unsigned char unmodeled_000[0x10];
    DoublewordVector4 position;     /* 0x010 */
    unsigned char unmodeled_020[0x30];
    DoublewordVector4 rotation;     /* 0x050 */
    unsigned char unmodeled_060[0xBC - 0x60];
    unsigned int flags;             /* 0x0BC */
    unsigned char unmodeled_0c0[0x5EC - 0xC0];
    int pxy_data;                   /* 0x5EC: the layer's pxy data */
    int txy_data;                   /* 0x5F0: the layer's txy data */
    struct BgLayerHierarchy *hierarchy; /* 0x5F4 */
    unsigned char unmodeled_5f8[4];
    unsigned char *slotTable;       /* 0x5FC */
} BgLayer;

/* Layer state: BG_LAYER_DONE (include/main/tu094.h) marks a finished layer; BG_LAYER_READY_STATE means its start and both files are in. */
#define BG_LAYER_READY_STATE 3
/* The XenoActor flag bgLayerCameraSetup sets on every pool record and bgLayerRelease clears; its meaning is not shown. */
#define BG_LAYER_FLAG 0x400
#define BG_LAYER_FADE_IN_FRAMES 8
#define BG_LAYER_WINDOW_WIDTH 512
#define BG_LAYER_WINDOW_HEIGHT 448

extern BgLayer *bgLayerObjects[];
/* The camera record bgLayerCameraSetup prepares: fov at +0x94, the rotation words at +0xA0, the position at +0xD0. */
typedef struct StudioCamera {
    unsigned char unmodeled_00[0x94];
    float fov;                      /* +0x94 */
    unsigned char unmodeled_98[0x08];
    unsigned int rotation[3];       /* +0xA0 */
    unsigned char unmodeled_ac[0x24];
    unsigned int position[2];       /* +0xD0 */
    float position_z;               /* +0xD8 */
} StudioCamera;

extern StudioCamera *bgLayerCamera;
void bgLayerLoad(int index);

/*
 * Clears the three layer slots (coordinate data, object) and the four state
 * bytes, starts loading the menu background and clears the camera gate.
 * State byte BG_BACK is written twice, as in the original.
 */
void bgLayerInit(void) {
    bgLayerState0[BG_BACK] = 0;
    bgLayerPxyData[BG_BACK] = 0;
    bgLayerTxyData[BG_BACK] = 0;
    bgLayerObjects[BG_BACK] = 0;
    bgLayerPxyData[BG_MIDDLE] = 0;
    bgLayerTxyData[BG_MIDDLE] = 0;
    bgLayerObjects[BG_MIDDLE] = 0;
    bgLayerState0[BG_MENUBG] = 0;
    bgLayerState0[BG_FRONT] = 0;
    bgLayerState0[BG_MIDDLE] = 0;
    bgLayerPxyData[BG_FRONT] = 0;
    bgLayerTxyData[BG_FRONT] = 0;
    bgLayerObjects[BG_FRONT] = 0;
    bgLayerState0[BG_BACK] = 0;
    bgLayerLoad(BG_MENUBG);
    bgLayerCamera = 0;
}

extern int bgLayerPaths[];
extern char D_003E7D50[];
extern char D_003E7D70[];
extern char D_003E7D90[];
extern char D_003E7DB0[];
extern char D_003E7DD0[];
extern char D_003E7DF0[];
extern char D_00689F70[];
extern char D_00689F90[];
extern char D_00689FB0[];
extern int xglCdReadFile(const char *name, void *buffer, int mode, int flags);
extern int xglCdReadFileAsync(const char *name, int buffer, int mode, int flags);
extern int xglCdSectorAlloc(int size);
extern int xglCdServiceIfPending(void);

/* One 512-byte hierarchy block, copied out of the read buffer. */
typedef struct BgLayerHierarchy {
    unsigned long long words[64];
} BgLayerHierarchy;

extern unsigned char bgLayerReadBuffer[];
extern BgLayerHierarchy bgLayerMiddleHierarchy;
extern BgLayerHierarchy bgLayerFrontHierarchy;

/* Starts loading background layer `index` (BG_BACK, BG_MIDDLE, BG_FRONT: its pxy and txy files; BG_MENUBG: the menu background hierarchies). */
void bgLayerLoad(int index) {
    switch (index) {
    case BG_BACK:
        if (bgLayerState0[BG_BACK] == 0) {
            bgLayerPxyData[BG_BACK] = xglCdSectorAlloc(bgLayerPaths[0]);
            bgLayerTxyData[BG_BACK] = xglCdSectorAlloc(bgLayerPaths[1]);
            if (bgLayerPxyData[BG_BACK] != 0) {
                xglCdReadFileAsync(D_003E7D50, bgLayerPxyData[BG_BACK], 2, 0);
            }
            if (bgLayerTxyData[BG_BACK] != 0) {
                xglCdReadFileAsync(D_003E7D70, bgLayerTxyData[BG_BACK], 2, 1);
            }
            bgLayerState0[BG_BACK]++;
        }
        break;
    case BG_MIDDLE:
        if (bgLayerState0[BG_MIDDLE] == 0) {
            bgLayerPxyData[BG_MIDDLE] = xglCdSectorAlloc(bgLayerPaths[2]);
            bgLayerTxyData[BG_MIDDLE] = xglCdSectorAlloc(bgLayerPaths[3]);
            if (bgLayerPxyData[BG_MIDDLE] != 0) {
                xglCdReadFileAsync(D_003E7D90, bgLayerPxyData[BG_MIDDLE], 2, 2);
            }
            if (bgLayerTxyData[BG_MIDDLE] != 0) {
                xglCdReadFileAsync(D_003E7DB0, bgLayerTxyData[BG_MIDDLE], 2, 3);
            }
            bgLayerState0[BG_MIDDLE]++;
        }
        break;
    case BG_FRONT:
        if (bgLayerState0[BG_FRONT] == 0) {
            bgLayerPxyData[BG_FRONT] = xglCdSectorAlloc(bgLayerPaths[4]);
            bgLayerTxyData[BG_FRONT] = xglCdSectorAlloc(bgLayerPaths[5]);
            if (bgLayerPxyData[BG_FRONT] != 0) {
                xglCdReadFileAsync(D_003E7DD0, bgLayerPxyData[BG_FRONT], 2, 4);
            }
            if (bgLayerTxyData[BG_FRONT] != 0) {
                xglCdReadFileAsync(D_003E7DF0, bgLayerTxyData[BG_FRONT], 2, 5);
            }
            bgLayerState0[BG_FRONT]++;
        }
        break;
    case BG_MENUBG:
        if (bgLayerState0[BG_MENUBG] == 0) {
            xglCdReadFile(D_00689F70, bgLayerReadBuffer, 1, 0);
            while (xglCdServiceIfPending() != 0) {
            }
            bgLayerMiddleHierarchy = *(BgLayerHierarchy *) bgLayerReadBuffer;
            xglCdReadFile(D_00689F90, bgLayerReadBuffer, 1, 0);
            while (xglCdServiceIfPending() != 0) {
            }
            bgLayerFrontHierarchy = *(BgLayerHierarchy *) bgLayerReadBuffer;
            xglCdReadFile(D_00689FB0, bgLayerReadBuffer, 1, 0);
            while (xglCdServiceIfPending() != 0) {
            }
        }
        bgLayerState0[BG_MENUBG] = BG_LAYER_DONE;
        break;
    }
}


/* Loads the four background layers: back, menu background, middle, front. */
void bgLayerLoadAll(void) {
    bgLayerLoad(BG_BACK);
    bgLayerLoad(BG_MENUBG);
    bgLayerLoad(BG_MIDDLE);
    bgLayerLoad(BG_FRONT);
}

extern BgLayer *XenoActorAlloc(void);
extern void XenoActorAllocMatrices(BgLayer *actor);
extern void XenoActorSetChannelValue(BgLayer *actor, float value);
extern void XenoActorEnableChannel(BgLayer *actor);
extern void XenoActorSetMode(BgLayer *actor, int mode);

/*
 * Counts a finished file of a layer (index 0..5: two files per layer) and,
 * once both files of a layer are in (state BG_LAYER_READY_STATE and up), creates its object at
 * the origin. The middle and front layers wait for the menu background (its state is BG_LAYER_DONE)
 * and get the hierarchies bgLayerLoad copied out; BG_LAYER_DONE marks a layer done.
 */
void bgLayerAdvance(int index) {
    switch ((unsigned char) index) {
    case 0:
    case 1:
        bgLayerState0[BG_BACK]++;
        break;
    case 2:
    case 3:
        bgLayerState0[BG_MIDDLE]++;
        break;
    case 4:
    case 5:
        bgLayerState0[BG_FRONT]++;
        break;
    }

    if (bgLayerState0[BG_BACK] != BG_LAYER_DONE && bgLayerState0[BG_BACK] >= BG_LAYER_READY_STATE) {
        if (bgLayerObjects[BG_BACK] == 0) {
            BgLayer *layer = XenoActorAlloc();
            int pxy_data;
            int txy_data;

            bgLayerObjects[BG_BACK] = layer;
            pxy_data = bgLayerPxyData[BG_BACK];
            txy_data = bgLayerTxyData[BG_BACK];
            layer->position.vector.x = 0.0f;
            layer->txy_data = txy_data;
            layer->pxy_data = pxy_data;
            bgLayerObjects[BG_BACK]->position.vector.y = 0.0f;
            bgLayerObjects[BG_BACK]->position.vector.z = 0.0f;
            bgLayerObjects[BG_BACK]->rotation.vector.x = 0.0f;
            bgLayerObjects[BG_BACK]->rotation.vector.y = 0.0f;
            bgLayerObjects[BG_BACK]->rotation.vector.z = 0.0f;
        }
        bgLayerState0[BG_BACK] = BG_LAYER_DONE;
    }

    if (bgLayerState0[BG_MENUBG] == BG_LAYER_DONE) {
        if (bgLayerState0[BG_MIDDLE] != BG_LAYER_DONE && bgLayerState0[BG_MIDDLE] >= BG_LAYER_READY_STATE) {
            if (bgLayerObjects[BG_MIDDLE] == 0) {
                BgLayer *layer = XenoActorAlloc();
                int pxy_data;
                int txy_data;

                pxy_data = bgLayerPxyData[BG_MIDDLE];
                txy_data = bgLayerTxyData[BG_MIDDLE];
                layer->hierarchy = &bgLayerMiddleHierarchy;
                layer->pxy_data = pxy_data;
                layer->txy_data = txy_data;
                layer->slotTable = bgLayerReadBuffer;
                bgLayerObjects[BG_MIDDLE] = layer;
                XenoActorAllocMatrices(layer);
                XenoActorSetChannelValue(bgLayerObjects[BG_MIDDLE], 0.0f);
                /* A do-while(0) block, likely an original macro: its loop notes order the reloads. */
                do {
                    XenoActorEnableChannel(bgLayerObjects[BG_MIDDLE]);
                    XenoActorSetMode(bgLayerObjects[BG_MIDDLE], 1);
                    bgLayerObjects[BG_MIDDLE]->position.vector.x = 0.0f;
                    bgLayerObjects[BG_MIDDLE]->position.vector.y = 0.0f;
                    bgLayerObjects[BG_MIDDLE]->position.vector.z = 0.0f;
                    bgLayerObjects[BG_MIDDLE]->rotation.vector.x = 0.0f;
                    bgLayerObjects[BG_MIDDLE]->rotation.vector.y = 0.0f;
                    bgLayerObjects[BG_MIDDLE]->rotation.vector.z = 0.0f;
                } while (0);
            }
            bgLayerState0[BG_MIDDLE] = BG_LAYER_DONE;
        }
        if (bgLayerState0[BG_MENUBG] == BG_LAYER_DONE) {
            if (bgLayerState0[BG_FRONT] != BG_LAYER_DONE && bgLayerState0[BG_FRONT] >= BG_LAYER_READY_STATE) {
                if (bgLayerObjects[BG_FRONT] == 0) {
                    BgLayer *layer = XenoActorAlloc();
                    int pxy_data;
                    int txy_data;

                    pxy_data = bgLayerPxyData[BG_FRONT];
                    txy_data = bgLayerTxyData[BG_FRONT];
                    layer->hierarchy = &bgLayerFrontHierarchy;
                    layer->pxy_data = pxy_data;
                    layer->txy_data = txy_data;
                    layer->slotTable = bgLayerReadBuffer;
                    bgLayerObjects[BG_FRONT] = layer;
                    XenoActorAllocMatrices(layer);
                    XenoActorSetChannelValue(bgLayerObjects[BG_FRONT], 0.0f);
                    do {
                        XenoActorEnableChannel(bgLayerObjects[BG_FRONT]);
                        XenoActorSetMode(bgLayerObjects[BG_FRONT], 0);
                        bgLayerObjects[BG_FRONT]->position.vector.x = 0.0f;
                        bgLayerObjects[BG_FRONT]->position.vector.y = 0.0f;
                        bgLayerObjects[BG_FRONT]->position.vector.z = 0.0f;
                        bgLayerObjects[BG_FRONT]->rotation.vector.x = 0.0f;
                        bgLayerObjects[BG_FRONT]->rotation.vector.y = 0.0f;
                        bgLayerObjects[BG_FRONT]->rotation.vector.z = 0.0f;
                    } while (0);
                }
                bgLayerState0[BG_FRONT] = BG_LAYER_DONE;
            }
        }
    }
}

/* Zeroes the first three words of both coordinate tables, the three layer state bytes and the camera gate. */
void bgLayerResetState(void) {
    bgLayerPxyData[BG_FRONT] = 0;
    bgLayerTxyData[BG_FRONT] = 0;
    bgLayerState0[BG_BACK] = 0;
    bgLayerPxyData[BG_BACK] = 0;
    bgLayerTxyData[BG_BACK] = 0;
    do {
        bgLayerPxyData[BG_MIDDLE] = 0;
        bgLayerTxyData[BG_MIDDLE] = 0;
    } while (0);
    do {
        bgLayerState0[BG_FRONT] = 0;
    } while (0);
    bgLayerState0[BG_MIDDLE] = 0;
    bgLayerCamera = 0;
}

extern float D_0069B1E0;
extern void XenoActorSetFlagAll(int flag);
extern void fadeReset(void);
extern void fadeStartIn(int frames);
extern void xglCameraSetWindow(StudioCamera *camera, int x0, int y0, int x1, int y1);
extern StudioCamera *xglStudioSelectGetActiveCamera(int index);

/* Sets up the background camera on first use, then resets the actor flags and the fade-in. */
void bgLayerCameraSetup(void) {
    StudioCamera *camera;

    if (bgLayerCamera == 0) {
        camera = xglStudioSelectGetActiveCamera(0);
        bgLayerCamera = camera;
        camera->position_z = 14.0f;
        camera->fov = D_0069B1E0;
        camera->position[0] = 0;
        camera->position[1] = 0;
        camera->rotation[0] = 0;
        camera->rotation[1] = 0;
        camera->rotation[2] = 0;
        xglCameraSetWindow(camera, 0, 0, BG_LAYER_WINDOW_WIDTH, BG_LAYER_WINDOW_HEIGHT);
    }
    XenoActorSetFlagAll(BG_LAYER_FLAG);
    fadeReset();
    fadeStartIn(BG_LAYER_FADE_IN_FRAMES);
}

extern void XenoActorRelease(BgLayer *actor);
extern int XenoActorGetPoolCount(void);
extern BgLayer *XenoActorPoolGet(int index);

/* Releases the layer objects and clears the BG_LAYER_FLAG flag of every pool record. */
void bgLayerRelease(void) {
    int i;
    int pool_count;
    BgLayer *actor;

    for (i = 0; i < BG_OBJECT_COUNT; i++) {
        if (bgLayerObjects[i] != 0) {
            XenoActorRelease(bgLayerObjects[i]);
        }
        bgLayerObjects[i] = 0;
    }
    pool_count = XenoActorGetPoolCount();
    for (i = 0; i < pool_count; i++) {
        actor = XenoActorPoolGet(i);
        if (actor != 0) {
            actor->flags &= ~BG_LAYER_FLAG;
        }
    }
}

/* Clears bits 0x400 and 0x2 of the flags of background layer `index`, when the camera gate is set. */
void bgLayerClearFlags(int index) {
    BgLayer *layer;

    if (bgLayerCamera != 0) {
        layer = bgLayerObjects[index];
        if (layer != 0) {
            layer->flags &= ~0x402;
        }
    }
}

/* Clears the flags of the three layers 0..2. */
void bgLayerClearAllFlags(void) {
    bgLayerClearFlags(0);
    bgLayerClearFlags(1);
    bgLayerClearFlags(2);
}
