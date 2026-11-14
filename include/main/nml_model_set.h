#ifndef INCLUDE_MAIN_NML_MODEL_SET_H
#define INCLUDE_MAIN_NML_MODEL_SET_H

#include "shared.h"

/* Functions of src/main/nml_model_set.c that other files call. */

/* The 0x180-byte light block pxySetDefaultLight copies into pxyState (XglLightSet has the same layout). */
struct PxyShadowLight;

void func_001BA658(void);
void func_001BA6F4(void);
void func_001BB44C(void *value);
void func_001BB47C(void *value);
void func_001BB678(void *value);
void nmlModelSetStencilAlpha(long long value);
int func_001BBC74(void);
void nmlModelSetDrawMatrix(void *matrix);
void nmlModelSetStencil(int enabled);
void pxyAddEntryFlagged(int key, void *data, int qwc, int flag);
void pxyAddEntryFull(int key, void *data, int qwc, int flag, int state);
void pxySetDefaultLight(struct PxyShadowLight *light);
void xglDirectionFromAngles(Vector4 *direction, const Vector4 *rotation);

#endif /* INCLUDE_MAIN_NML_MODEL_SET_H */
