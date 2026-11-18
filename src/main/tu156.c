#include "common.h"
#include "shared.h"
#include "main/xgl_2.h"

extern AlignedXglLightSet D_006AF340;
static AlignedXglLightSet D_006AF4C0;
static AlignedXglLightSet D_006AF640;

/* Defined in xgl_2.c; not in main/xgl_2.h because nml_model_set.c passes its own state record. */
void xglLightSetDefault(XglLightSet *lightSet);

/*
 * Three 0x180-byte blocks (D_006AF4C0, D_006AF640 and D_006AF340) are moved
 * by func_00252F98 and func_00253048. The getters below return two of them;
 * sefSetGateBlocks (main/tu175) calls both, sefRefreshEntryRows (main/tu170) the first.
 */
void *func_00252F78(void)
{
    return &D_006AF4C0;
}

void *func_00252F88(void)
{
    return &D_006AF640;
}

/* Resets D_006AF4C0 to the default light set and copies it to D_006AF640 and D_006AF340. */
void func_00252F98(void)
{
    xglLightSetDefault(&D_006AF4C0);
    D_006AF340 = D_006AF640 = D_006AF4C0;
}

/* Copies the light set to D_006AF340 and D_006AF4C0 and resets D_006AF640 to the default one. */
void func_00253048(XglLightSet *lights)
{
    D_006AF340 = *lights;
    D_006AF4C0 = *lights;
    xglLightSetDefault(&D_006AF640);
}
