#ifndef INCLUDE_MAIN_XGL_STUDIO_H
#define INCLUDE_MAIN_XGL_STUDIO_H

/* Functions of src/main/xgl_studio.c that other files call. */

struct XglCamera;

void xglStudioInit(void);
void xglStudioMainCameraInit(void);
void xglStudioChange(int studio_index);
void xglStudioGetCamera(struct XglCamera **camera, int camera_index);
void xglStudioMove(void);
void xglStudioUpdate(void);

#endif /* INCLUDE_MAIN_XGL_STUDIO_H */
