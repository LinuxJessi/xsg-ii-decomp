/*
 * OV01 original TU 40: 0x00a9b0f8..0x00a9b120 (3 functions)
 */
#include "common.h"
#include "main/tu048.h"
#include "ov01/tu040.h"

/* Returns at once with no effect. Caller: ov01SceneInit. */
void func_00A9B0F8(void) {
}

/*
 * The two images of the scene in gameWork (main): resBuffer holds the SB script image (the key of its threads),
 * imageEnd the SBL library image dataThinkScriptLoad reads behind it. ov01StepSceneScript links the library under
 * the key (sbLibraryLink calls sbLinkImage and keeps the image when sbIsSblImage accepts it); ov01SceneRelease
 * releases the key's threads (sbThreadsRelease).
 */

/* The scene's SB script image (gameWork.resBuffer). */
struct SbImageHeader *ov01ScriptKeyGet(void) {
    return (struct SbImageHeader *)gameWork.resBuffer;
}

/* The scene's SBL library image (gameWork.imageEnd, an address main keeps as int). */
struct VmKeyTable *ov01ScriptImageGet(void) {
    return (struct VmKeyTable *)gameWork.imageEnd;
}
