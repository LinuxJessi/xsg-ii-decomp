/*
 * OV01 original TU 40: 0x00a9b0f8..0x00a9b120 (3 functions)
 */
#include "common.h"

/* Returns at once with no effect. Caller: ov01SceneInit. */
void func_00A9B0F8(void) {
}

/*
 * The fields of gameWork (main) this file reads: the SB script image of the
 * scene (the key of its threads) and an SBL library image. ov01StepSceneScript links the library under the key
 * (func_00A9CEE8 calls sbLinkImage and keeps the image when sbIsSblImage
 * accepts it); ov01SceneRelease releases the key's threads
 * (sbThreadsRelease).
 */
typedef struct SceneGameWork {
    unsigned char unmodeled_00[0x2C];
    struct SbImageHeader *script;     /* +0x2C */
    unsigned char unmodeled_30[0xB8 - 0x30];
    struct SbImageHeader *library;    /* +0xB8 */
} SceneGameWork;

extern SceneGameWork gameWork;

/* The scene's SB script image (gameWork +0x2C). */
struct SbImageHeader *ov01ScriptKeyGet(void) {
    return gameWork.script;
}

/* The scene's SBL library image (gameWork +0xB8). */
struct SbImageHeader *ov01ScriptImageGet(void) {
    return gameWork.library;
}
