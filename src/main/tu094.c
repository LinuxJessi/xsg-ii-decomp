#include "common.h"
#include "main/tu094.h"

INCLUDE_ASM("asm/main/nonmatchings/tu094", xglPrimAddRenderScissor);

INCLUDE_ASM("asm/main/nonmatchings/tu094", bgLayerInit);

INCLUDE_ASM("asm/main/nonmatchings/tu094", bgLayerLoad);

int bgLayerLoad(int index);

/* Loads the four background layers: back (0), menu background (3), middle (1), front (2). */
void bgLayerLoadAll(void) {
    bgLayerLoad(0);
    bgLayerLoad(3);
    bgLayerLoad(1);
    bgLayerLoad(2);
}

INCLUDE_ASM("asm/main/nonmatchings/tu094", bgLayerAdvance);

INCLUDE_ASM("asm/main/nonmatchings/tu094", bgLayerResetState);

INCLUDE_ASM("asm/main/nonmatchings/tu094", bgLayerCameraSetup);

INCLUDE_ASM("asm/main/nonmatchings/tu094", bgLayerRelease);

/*
 * One background layer (0 back, 1 middle, 2 front; loaded by bgLayerLoad). The
 * object is allocated per layer and its flags word (+0xBC) is the only
 * field modeled here; the other bytes are unmodeled gaps.
 */
typedef struct BgLayer {
    unsigned char unmodeled_00[0xBC];
    unsigned int flags;             /* 0xBC */
} BgLayer;

extern int bgLayerCamera;
extern BgLayer *bgLayerObjects[];

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
