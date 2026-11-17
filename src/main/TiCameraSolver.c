#include "common.h"
#include "shared.h"
#include "main/tu026.h"

void func_0028D4C8(void)
{
}

f32 atanf(f32);

/* Horizontal or vertical field of view in degrees from a focal length in millimetres and a frame size in inches
 * (0.03937 converts mm to inch). */
f32 tiCameraFovDegrees(f32 focal_length_mm, f32 frame_size)
{
    f32 half_frame;

    half_frame = frame_size * 0.5f;
    return 2.0f * atanf(half_frame / (focal_length_mm * 0.03937f)) * 57.29578f;
}

/* Channel written by the scalar and angle readers at camera +0x90. */
typedef struct TiCameraChannel {
    unsigned char unmodeled_00[4];
    float value;
    int unknown_08;
    int unknown_0c;
} TiCameraChannel;

/* Camera record (pCam). Only the fields the three readers touch are named. */
typedef struct TiCamera {
    unsigned char unmodeled_00[0x90];
    TiCameraChannel channel;
    Vector4 euler;
    unsigned char unmodeled_b0[0x20];
    float position[3];
} TiCamera;

extern const char tiCameraAssertPData[];
extern const char tiCameraSourceFile[];
extern const char tiCameraAssertPCam[];
void xacEvalJointPose(f32 *values, void **ppJointHeader, f32 curveParam);
void xacEvalJointValue(f32 curveParam, f32 *values, void **ppJointHeader);
void xglEulerFromRotation(Vector4 *euler, Vector4 *rotation);

void tiCameraReadPose(TiCamera *camera, void **ppData, f32 curveParam)
{
    /* Three position values, then three rotation angles, as xacEvalJointPose writes them. */
    f32 pose[6];
    Vector4 rotation;
    /* Unused local: removing it changes the stack frame and the .text bytes. */
    Vector4 unused_frame_slot;

    if (ppData == 0) {
        assertFailed(tiCameraAssertPData, tiCameraSourceFile, 28);
    }
    if (camera == 0) {
        assertFailed(tiCameraAssertPCam, tiCameraSourceFile, 29);
    }
    xacEvalJointPose(pose, ppData, curveParam);
    camera->position[0] = pose[0];
    camera->position[1] = pose[1];
    camera->position[2] = pose[2];
    rotation.x = pose[3];
    rotation.y = pose[4];
    rotation.z = pose[5];
    xglEulerFromRotation(&camera->euler, &rotation);
}

void tiCameraReadScalar(TiCamera *camera, void **ppData, f32 curveParam)
{
    f32 sample;
    TiCameraChannel *channel;

    if (ppData == 0) {
        assertFailed(tiCameraAssertPData, tiCameraSourceFile, 56);
    }
    if (camera == 0) {
        assertFailed(tiCameraAssertPCam, tiCameraSourceFile, 57);
    }
    xacEvalJointValue(curveParam, &sample, ppData);
    channel = &camera->channel;
    channel->unknown_08 = channel->unknown_0c = 0;
    channel->value = sample;
}

void tiCameraReadAngle(TiCamera *camera, void **ppData, f32 curveParam)
{
    f32 sample;
    TiCameraChannel *channel;

    if (ppData == 0) {
        assertFailed(tiCameraAssertPData, tiCameraSourceFile, 76);
    }
    if (camera == 0) {
        assertFailed(tiCameraAssertPCam, tiCameraSourceFile, 77);
    }
    xacEvalJointValue(curveParam, &sample, ppData);
    channel = &camera->channel;
    channel->unknown_0c = 0;
    channel->unknown_08 = 0;
    channel->value = sample * 3.1415927f / 180.0f;
}

/* Camera record: three header words, then `record[1]` picks the layout (< 1.1 uses the
 * scalar reader, otherwise the angle reader). The data cursor starts at record[3]. */
void tiCameraReadRecord(TiCamera *camera, f32 *record, f32 value)
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

/* Data of this TU: the assert strings, in address order. */
const char tiCameraAssertPData[8] = "pData";
const char tiCameraSourceFile[24] = "../TiCameraSolver.euc.c";
const char tiCameraAssertPCam[8] = "pCam";
