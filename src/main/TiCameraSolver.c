#include "common.h"

/* Named in the proposals: tiCameraFovDegrees. Horizontal or vertical field of view in degrees
 * from a focal length in millimetres and a frame size in inches (0.03937 converts mm to inch). */
void func_0028D4C8(void)
{
}

typedef float f32;

f32 func_002A1790(f32);

f32 tiCameraFovDegrees(f32 focal_length_mm, f32 frame_size)
{
    f32 half_frame;

    half_frame = frame_size * 0.5f;
    return 2.0f * func_002A1790(half_frame / (focal_length_mm * 0.03937f)) * 57.29578f;
}

INCLUDE_ASM("asm/main/nonmatchings/TiCameraSolver", tiCameraReadPose);

INCLUDE_ASM("asm/main/nonmatchings/TiCameraSolver", tiCameraReadScalar);

INCLUDE_ASM("asm/main/nonmatchings/TiCameraSolver", tiCameraReadAngle);

/* The `camera` argument is the pCam pointer of the asserts, typed int here by the stage. */
void tiCameraReadPose(int camera, void **stream, f32 value);
void tiCameraReadScalar(int camera, void **stream, f32 value);
void tiCameraReadAngle(int camera, void **stream, f32 value);

/* Camera record: three header words, then `record[1]` picks the layout (< 1.1 uses the
 * scalar reader, otherwise the angle reader). The data cursor starts at record[3]. */
void tiCameraReadRecord(int camera, f32 *record, f32 value)
{
    void *stream;

    stream = &record[3];
    if (record[1] < 1.1f)
    {
        tiCameraReadPose(camera, &stream, value);
        tiCameraReadScalar(camera, &stream, value);
        return;
    }
    tiCameraReadPose(camera, &stream, value);
    tiCameraReadAngle(camera, &stream, value);
}
