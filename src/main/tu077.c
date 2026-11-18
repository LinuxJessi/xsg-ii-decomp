#include "common.h"
#include "shared.h"
#include "main/tu051.h"
#include "main/tu077.h"
#include "main/tu048.h"

/* gameWork.camera.flags bits that the camera commands set. */
enum CameraFlagBits {
    CAMERA_FLAG_POS_RESET = 0x20,
    CAMERA_FLAG_BATTLE = 0x200
};

/* cameraMoveMode bits: a move command selects the reference block or the camera block. */
enum CameraMoveBits {
    CAMERA_MOVE_REF = 1,
    CAMERA_MOVE_CAM = 2
};

/* gameWork.camera.shakeFlags bits: one per shake channel (shakeA, shakeB, shakeC). */
enum CameraShakeChannel {
    CAMERA_SHAKE_CHANNEL_A = 1,
    CAMERA_SHAKE_CHANNEL_B = 2,
    CAMERA_SHAKE_CHANNEL_C = 4
};

/* refAnchorIndex value when the reference is not tied to an anchor. */
enum {
    CAMERA_ANCHOR_NONE = -1
};

/* Record returned by cameraAnchorGet; two floats are added to a position. */
struct CameraOffsetRecord {
    unsigned char unmodeled_00[0x50];
    float offsetX;
    float offsetY;
};

void func_A9CDB0(void *, void *);
int *func_A9F298(int handle, int value);
struct CameraOffsetRecord *cameraApplyAnchorOffset(float *out, int slot_1, int slot_2, float *in);
struct CameraOffsetRecord *cameraAnchorGet(float *scratch, int slot_1, int slot_2);
extern void *ladderNatives[];
extern u8 ladderModuleName[];

void ladderRegisterNatives(void) {
    func_A9CDB0(ladderModuleName, ladderNatives);
}

float I2F(int value);
extern const f32 cameraCentiScale;

f32 cameraCentiToFloat(int value) {
    return I2F(value) * cameraCentiScale;
}

int F2I(f32);

int cameraFloatToCenti(f32 value) {
    return F2I(value * 100.0f);
}

extern const f32 cameraRadPerCentiDeg;

f32 cameraCentiDegToRad(int value) {
    return I2F(value) * cameraRadPerCentiDeg;
}

extern const f32 cameraRadPerCentiDegDiv;

int cameraRadToCentiDeg(f32 value) {
    return F2I(value / cameraRadPerCentiDegDiv);
}

/* A shake record (cameraShake): a float pair then four halfwords. */
struct CameraShakeRecord {
    float amplitude;
    float step;
    short paramA;
    short paramB;
    short paramC;
    short frames;
};

/* gameWork.camera.range.anchorIndex and anchorSubIndex as union members (alias set 0). Lever: SetRefCamActR stores
   them after the vectors; plain members let the scheduler move the stores. */
typedef struct {
    unsigned char unmodeled_000[0x180];
    union { int anchorIndex; };
    union { short anchorSubIndex; };
} CameraAnchorAlias0;

static u8 cameraRefMoveFlag;
static u8 cameraCamMoveFlag;
static u8 cameraMoveMode;
/* Script argument count of the camera command now running; cameraCheckArgCount compares it with argc. */
static u8 cameraArgCount;
/* The camera state block: every command below reads or writes its fields. */
int func_A9F0A8(void);
void func_A9CC70(int handle, int level);
int func_A9F0C0(int handle, int mode);
void cameraSetMode(int value, int mode, int sub_mode);
void cameraSetIndex(int value);

int cameraCheckArgCount(int handle) {
    if (func_A9F0A8() != cameraArgCount) {
        func_A9CC70(handle, 2);
        return 0;
    }
    return 1;
}

void cameraReset(void);

int talkReset(void) {
    cameraReset();
    return 1;
}

int cameraMode(int scriptHandle) {
    int modeA;
    int modeB;

    cameraArgCount = 2;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    modeA = func_A9F0C0(scriptHandle, 1);
    modeB = func_A9F0C0(scriptHandle, 2);
    gameWork.camera.cameraType = modeA;
    gameWork.camera.cameraSubType = modeB;
    return 1;
}

int AngleMode(void) {
    return 1;
}

int cameraPosReset(int scriptHandle) {
    cameraArgCount = 0;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    gameWork.camera.flags |= CAMERA_FLAG_POS_RESET;
    return 1;
}

/* One camera record of cameraCfSlots (208 bytes): cameraCfSet and cfChange copy the reference block into it. */
struct CameraRefBlock {
    int words[28];
};

struct CameraStopBlock {
    int words[4];
};

struct CameraSlotRecord {
    unsigned char modeA;               /* +0x00: copy of gameWork.camera.cameraType */
    unsigned char unmodeled_01;
    unsigned char slotIndex;           /* +0x02: copy of gameWork.camera.mode */
    unsigned char unmodeled_03[0xD];
    struct CameraStopBlock stopBlock;  /* +0x10: copy of gameWork.camera.slot[0] */
    struct CameraRefBlock refBlock;        /* +0x20: copy of gameWork.camera.range from anchorIndex */
    unsigned char unmodeled_90[0x40];
};

extern struct CameraSlotRecord cameraCfSlots[];

int cameraCfSet(int scriptHandle) {
    struct CameraSlotRecord *slot;
    int recordIndex;

    cameraArgCount = 1;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    recordIndex = func_A9F0C0(scriptHandle, 1);
    slot = &cameraCfSlots[recordIndex];
    slot->modeA = gameWork.camera.cameraType;
    slot->slotIndex = gameWork.camera.mode;
    *(struct CameraStopBlock *)&slot->stopBlock = *(struct CameraStopBlock *)&gameWork.camera.slot[0];
    *(struct CameraRefBlock *)&slot->refBlock = *(struct CameraRefBlock *)&gameWork.camera.range.anchorIndex;
    return 1;
}

int cfChange(int scriptHandle) {
    struct CameraSlotRecord *slot;
    int recordIndex;

    cameraArgCount = 1;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    recordIndex = func_A9F0C0(scriptHandle, 1);
    slot = &cameraCfSlots[recordIndex];
    slot->modeA = gameWork.camera.cameraType;
    slot->slotIndex = gameWork.camera.mode;
    *(struct CameraRefBlock *)&slot->refBlock = *(struct CameraRefBlock *)&gameWork.camera.range.anchorIndex;
    return 1;
}

int cameraCfRotRange(int scriptHandle) {
    f32 radX;
    f32 radY;
    f32 radZ;
    f32 radW;
    int centiDeg;

    cameraArgCount = 4;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    centiDeg = func_A9F0C0(scriptHandle, 1);
    if (centiDeg == -9000) {
        centiDeg = -8999;
    }
    radX = cameraCentiDegToRad(centiDeg);
    radY = cameraCentiDegToRad(func_A9F0C0(scriptHandle, 2));
    radZ = cameraCentiDegToRad(func_A9F0C0(scriptHandle, 3));
    radW = cameraCentiDegToRad(func_A9F0C0(scriptHandle, 4));
    gameWork.camera.range.angleLimits.x = radX;
    gameWork.camera.range.angleLimits.y = radY;
    gameWork.camera.range.angleLimits.z = radZ;
    gameWork.camera.range.angleLimits.w = radW;
    return 1;
}

int cameraCfCamId(int handle) {
    cameraArgCount = 1;
    if (cameraCheckArgCount(handle) == 0) {
        return 0;
    }
    cameraSetMode(func_A9F0C0(handle, 1), 0, 0);
    return 1;
}

int cameraCfLock(int scriptHandle) {
    int lockFlags;

    cameraArgCount = 1;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    lockFlags = func_A9F0C0(scriptHandle, 1);
    gameWork.camera.range.probeMode = lockFlags & 0x21;
    gameWork.camera.range.lockX = lockFlags & 0x2;
    gameWork.camera.range.lockY = lockFlags & 0x4;
    gameWork.camera.range.lockZ = lockFlags & 0x8;
    return 1;
}

int cameraCfRangeMode(int handle) {
    cameraArgCount = 1;
    if (cameraCheckArgCount(handle) == 0) {
        return 0;
    }
    gameWork.camera.range.rangeMode = func_A9F0C0(handle, 1);
    return 1;
}

int cameraCfSelectChg(int handle) {
    cameraArgCount = 1;
    if (cameraCheckArgCount(handle) == 0) {
        return 0;
    }
    cameraSetIndex(func_A9F0C0(handle, 1));
    return 1;
}

struct CameraKeyEntry {
    signed char key;
    signed char subKey;
    unsigned char unmodeled_02[0x14 - 0x02];
};

struct CameraKeyEntry *cameraSlotFind(int key, int subKey);

struct CameraBlock16 {
    unsigned char bytes[16];
};

int cameraCfHokan(int scriptHandle)
{
    int key;
    int subKey;
    struct CameraKeyEntry *entry;

    cameraArgCount = 2;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    key = func_A9F0C0(scriptHandle, 1);
    subKey = func_A9F0C0(scriptHandle, 2);
    entry = cameraSlotFind(-1, -1);
    if (entry == 0) {
        func_A9CC70(scriptHandle, 2);
        return 0;
    }
    entry->key = key;
    entry->subKey = subKey;
    entry->unmodeled_02[0] = gameWork.camera.range.rangeMode;
    *(struct CameraBlock16 *)&entry->unmodeled_02[2] = *(struct CameraBlock16 *)&gameWork.camera.slot[0];
    return 1;
}

int cameraCfBattleCam(int scriptHandle) {
    cameraArgCount = 1;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    gameWork.camera.flags &= 0xFDFF;
    if (func_A9F0C0(scriptHandle, 1) != 0) {
        gameWork.camera.flags |= CAMERA_FLAG_BATTLE;
    }
    gameWork.camera.slot[0].duration = 0;
    gameWork.camera.slot[1].duration = 0;
    *(int *) &gameWork.camera.slot[2].duration = 0;
    gameWork.camera.slot[3].duration = 0;
    gameWork.camera.flags = (gameWork.camera.flags & 0xFFEF) | 0x80;
    return 1;
}

int cfNorth(int handle) {
    int looked_up;
    int out_of_range;

    cameraArgCount = 1;
    if (cameraCheckArgCount(handle) == 0) {
        return 0;
    }
    looked_up = func_A9F0C0(handle, 1);
    out_of_range = (u32)looked_up + 0x8CA0U > 0x11940U;
    gameWork.camera.range.rangeFlag = out_of_range;
    gameWork.camera.range.northAngle = cameraCentiDegToRad(looked_up);
    return 1;
}

int cameraSetRefPos(int scriptHandle) {
    f32 posX;
    f32 posY;
    f32 posZ;

    cameraArgCount = 3;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    posX = cameraCentiToFloat(func_A9F0C0(scriptHandle, 1));
    posY = cameraCentiToFloat(func_A9F0C0(scriptHandle, 2));
    posZ = cameraCentiToFloat(func_A9F0C0(scriptHandle, 3));
    if (cameraRefMoveFlag == 0) {
        gameWork.camera.range.lookAt.x = posX;
        gameWork.camera.range.lookAt.y = posY;
        gameWork.camera.range.lookAt.z = posZ;
        gameWork.camera.range.anchorIndex = CAMERA_ANCHOR_NONE;
        gameWork.camera.range.anchorSubIndex = 0;
        gameWork.camera.range.offset.x = 0;
        gameWork.camera.range.offset.y = 0;
        gameWork.camera.range.offset.z = 0;
    } else {
        gameWork.camera.to.lookAt.x = posX;
        gameWork.camera.to.lookAt.y = posY;
        gameWork.camera.to.lookAt.z = posZ;
    }
    cameraRefMoveFlag = 0;
    return 1;
}

int cameraSetRefAct(int scriptHandle) {
    Vector4 position;
    Vector4 offset;
    int anchorIndex;
    int anchorSubIndex;
    int centiX;
    int centiY;
    int centiZ;

    cameraArgCount = 5;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    anchorIndex = func_A9F0C0(scriptHandle, 1);
    anchorSubIndex = func_A9F0C0(scriptHandle, 2);
    centiX = func_A9F0C0(scriptHandle, 3);
    offset.x = cameraCentiToFloat(centiX);
    centiY = func_A9F0C0(scriptHandle, 4);
    offset.y = cameraCentiToFloat(centiY);
    centiZ = func_A9F0C0(scriptHandle, 5);
    offset.z = cameraCentiToFloat(centiZ);
    offset.w = 0.0f;
    cameraApplyAnchorOffset(&position.x, anchorIndex, anchorSubIndex, &offset.x);
    if (cameraRefMoveFlag == 0) {
        gameWork.camera.range.lookAt.x = position.x;
        gameWork.camera.range.lookAt.y = position.y;
        gameWork.camera.range.lookAt.z = position.z;
        gameWork.camera.range.offset.x = offset.x;
        gameWork.camera.range.offset.y = offset.y;
        gameWork.camera.range.offset.z = offset.z;
        gameWork.camera.range.anchorIndex = anchorIndex;
        gameWork.camera.range.anchorSubIndex = anchorSubIndex;
        gameWork.camera.range.offset.w = offset.w;
    } else {
        gameWork.camera.to.lookAt.x = position.x;
        gameWork.camera.to.lookAt.y = position.y;
        gameWork.camera.to.lookAt.z = position.z;
    }
    cameraRefMoveFlag = 0;
    return 1;
}

void cameraOrbitPosition(float *dst, float *a, float *b);

int SetRefActR(int act)
{
    float transformed[3];
    Vector4 input;
    int frame_a;
    int frame_b;
    int raw_x;
    int raw_y;
    int raw_z;
    int ok;

    cameraArgCount = 5;
    ok = cameraCheckArgCount(act);
    if (ok != 0)
    {
        frame_a = func_A9F0C0(act, 1);
        frame_b = func_A9F0C0(act, 2);
        raw_x = func_A9F0C0(act, 3);
        input.x = cameraCentiToFloat(raw_x);
        raw_y = func_A9F0C0(act, 4);
        input.y = cameraCentiToFloat(raw_y);
        raw_z = func_A9F0C0(act, 5);
        input.z = cameraCentiToFloat(raw_z);
        input.w = 1.0f;
        cameraApplyAnchorOffset(transformed, frame_a, frame_b, &input.x);
        if (cameraRefMoveFlag == 0)
        {
            float transformed_x = transformed[0];
            float transformed_y = transformed[1];
            float transformed_z = transformed[2];
            float input_x = input.x;
            float input_y = input.y;
            float input_z = input.z;
            float input_w = input.w;

            gameWork.camera.range.anchorSubIndex = frame_b;
            gameWork.camera.range.offset.w = input_w;
            gameWork.camera.range.lookAt.x = transformed_x;
            gameWork.camera.range.lookAt.y = transformed_y;
            gameWork.camera.range.lookAt.z = transformed_z;
            gameWork.camera.range.offset.x = input_x;
            gameWork.camera.range.offset.y = input_y;
            gameWork.camera.range.offset.z = input_z;
            gameWork.camera.range.anchorIndex = frame_a;
        }
        else
        {
            gameWork.camera.to.lookAt.x = transformed[0];
            gameWork.camera.to.lookAt.y = transformed[1];
            gameWork.camera.to.lookAt.z = transformed[2];
        }
        cameraRefMoveFlag = 0;
        ok = 1;
    }
    return ok;
}

extern Vector4 D_003E7040;
void xglMatrixStackUnit(void);
void xglMatrixStackTrans(void *);
void xglMatrixStackRotX(float);
void xglMatrixStackRotY(float);
void xglMatrixStackSave(void *);

int cameraSetRefAng(int scriptHandle) {
    Matrix4 matrix;
    Vector4 offset;
    f32 rotX;
    f32 rotY;

    cameraArgCount = 2;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    rotX = cameraCentiDegToRad(func_A9F0C0(scriptHandle, 1));
    rotY = cameraCentiDegToRad(func_A9F0C0(scriptHandle, 2));
    offset.x = gameWork.camera.range.lockPosition.x;
    offset.y = gameWork.camera.range.lockPosition.y;
    offset.z = gameWork.camera.range.lockPosition.z;
    offset.w = 1.0f;
    xglMatrixStackUnit();
    xglMatrixStackTrans(&offset);
    xglMatrixStackRotY(rotY);
    xglMatrixStackRotX(rotX);
    xglMatrixStackTrans(&D_003E7040);
    xglMatrixStackSave(matrix);
    if (cameraRefMoveFlag == 0) {
        gameWork.camera.range.lookAt.x = matrix[3][0];
        gameWork.camera.range.lookAt.y = matrix[3][1];
        gameWork.camera.range.lookAt.z = matrix[3][2];
        gameWork.camera.range.anchorIndex = CAMERA_ANCHOR_NONE;
        gameWork.camera.range.anchorSubIndex = 0;
        gameWork.camera.range.offset.x = 0;
        gameWork.camera.range.offset.y = 0;
        gameWork.camera.range.offset.z = 0;
    } else {
        gameWork.camera.to.lookAt.x = matrix[3][0];
        gameWork.camera.to.lookAt.y = matrix[3][1];
        gameWork.camera.to.lookAt.z = matrix[3][2];
    }
    cameraRefMoveFlag = 0;
    return 1;
}

int RefLock(int scriptHandle) {
    cameraArgCount = 0;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    gameWork.camera.slot[0].duration = 0;
    gameWork.camera.range.anchorSubIndex = 0;
    gameWork.camera.range.anchorIndex = CAMERA_ANCHOR_NONE;
    return 1;
}

int SetRefCamActR(int scriptHandle) {
    Vector4 position;
    Vector4 orbitPos;
    Vector4 offset;
    Vector4 orbitAngles;
    struct CameraOffsetRecord *anchorRecord;
    int anchorIndex;
    int anchorSubIndex;

    cameraArgCount = 8;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    anchorIndex = func_A9F0C0(scriptHandle, 1);
    anchorSubIndex = func_A9F0C0(scriptHandle, 2);
    offset.x = cameraCentiToFloat(func_A9F0C0(scriptHandle, 3));
    offset.y = cameraCentiToFloat(func_A9F0C0(scriptHandle, 4));
    offset.z = cameraCentiToFloat(func_A9F0C0(scriptHandle, 5));
    offset.w = 1.0f;
    orbitAngles.x = cameraCentiDegToRad(func_A9F0C0(scriptHandle, 6));
    orbitAngles.y = cameraCentiDegToRad(func_A9F0C0(scriptHandle, 7));
    orbitAngles.z = 0.0f;
    orbitAngles.w = cameraCentiToFloat(func_A9F0C0(scriptHandle, 8));
    anchorRecord = cameraApplyAnchorOffset(&position.x, anchorIndex, anchorSubIndex, &offset.x);
    orbitAngles.x = orbitAngles.x + anchorRecord->offsetX;
    orbitAngles.y = orbitAngles.y + anchorRecord->offsetY;
    cameraOrbitPosition(&orbitPos.x, &position.x, &orbitAngles.x);
    if (cameraRefMoveFlag == 0) {
        gameWork.camera.range.offset.w = 1.0f;
        gameWork.camera.range.lookAt.x = position.x;
        gameWork.camera.range.lookAt.y = position.y;
        gameWork.camera.range.lookAt.z = position.z;
        ((CameraAnchorAlias0 *)&gameWork)->anchorIndex = CAMERA_ANCHOR_NONE;
        gameWork.camera.range.offset.x = offset.x;
        gameWork.camera.range.offset.y = offset.y;
        gameWork.camera.range.offset.z = offset.z;
        ((CameraAnchorAlias0 *)&gameWork)->anchorSubIndex = 0;
    } else {
        gameWork.camera.to.lookAt.x = position.x;
        gameWork.camera.to.lookAt.y = position.y;
        gameWork.camera.to.lookAt.z = position.z;
    }
    cameraRefMoveFlag = 0;
    if (cameraCamMoveFlag == 0) {
        gameWork.camera.range.lockPosition.x = orbitPos.x;
        gameWork.camera.range.lockPosition.y = orbitPos.y;
        gameWork.camera.range.lockPosition.z = orbitPos.z;
        gameWork.camera.range.angles.x = orbitAngles.x;
        gameWork.camera.range.angles.y = orbitAngles.y;
        gameWork.camera.range.angles.w = orbitAngles.w;
    } else {
        gameWork.camera.to.position.x = orbitPos.x;
        gameWork.camera.to.position.y = orbitPos.y;
        gameWork.camera.to.position.z = orbitPos.z;
        gameWork.camera.to.angles.x = orbitAngles.x;
        gameWork.camera.to.angles.y = orbitAngles.y;
        gameWork.camera.to.angles.w = orbitAngles.w;
    }
    cameraCamMoveFlag = 0;
    return 1;
}

void cameraAnglesBetween(float *dst, float *a, float *b);

int SetRefCamAct3R(int scriptHandle)
{
    Vector4 position;
    Vector4 orbitPos;
    Vector4 anchorPos;
    Vector4 offset;
    Vector4 orbitAngles;
    float matrix[16];
    float betweenAngles[2];
    int startIndex;
    int startSubIndex;
    int endIndex;
    int endSubIndex;
    float blend;

    cameraArgCount = 11;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    startIndex = func_A9F0C0(scriptHandle, 1);
    startSubIndex = func_A9F0C0(scriptHandle, 2);
    endIndex = func_A9F0C0(scriptHandle, 3);
    endSubIndex = func_A9F0C0(scriptHandle, 4);
    blend = cameraCentiToFloat(func_A9F0C0(scriptHandle, 5));
    offset.x = cameraCentiToFloat(func_A9F0C0(scriptHandle, 6));
    offset.y = cameraCentiToFloat(func_A9F0C0(scriptHandle, 7));
    offset.z = cameraCentiToFloat(func_A9F0C0(scriptHandle, 8));
    offset.w = 1.0f;
    orbitAngles.x = cameraCentiDegToRad(func_A9F0C0(scriptHandle, 9));
    orbitAngles.y = cameraCentiDegToRad(func_A9F0C0(scriptHandle, 10));
    orbitAngles.z = 0.0f;
    orbitAngles.w = cameraCentiToFloat(func_A9F0C0(scriptHandle, 11));
    cameraAnchorGet(&orbitPos.x, startIndex, startSubIndex);
    cameraAnchorGet(&anchorPos.x, endIndex, endSubIndex);
    position.x = (anchorPos.x - orbitPos.x) * blend + orbitPos.x;
    position.y = (anchorPos.y - orbitPos.y) * blend + orbitPos.y;
    position.z = (anchorPos.z - orbitPos.z) * blend + orbitPos.z;
    cameraAnglesBetween(betweenAngles, &orbitPos.x, &anchorPos.x);
    xglMatrixStackUnit();
    xglMatrixStackRotY(betweenAngles[1]);
    xglMatrixStackRotX(betweenAngles[0]);
    xglMatrixStackTrans(&offset);
    xglMatrixStackSave(matrix);
    position.x = position.x + matrix[12];
    position.y = position.y + matrix[13];
    position.z = position.z + matrix[14];
    orbitAngles.x = orbitAngles.x + betweenAngles[0];
    orbitAngles.y = orbitAngles.y + betweenAngles[1];
    cameraOrbitPosition(&orbitPos.x, &position.x, &orbitAngles.x);
    if (cameraRefMoveFlag == 0) {
        gameWork.camera.range.offset.w = 1.0f;
        gameWork.camera.range.lookAt.x = position.x;
        gameWork.camera.range.lookAt.y = position.y;
        gameWork.camera.range.lookAt.z = position.z;
        gameWork.camera.range.offset.x = 0.0f;
        gameWork.camera.range.offset.y = 0.0f;
        gameWork.camera.range.offset.z = 0.0f;
        gameWork.camera.range.anchorIndex = CAMERA_ANCHOR_NONE;
        gameWork.camera.range.anchorSubIndex = 0;
    } else {
        gameWork.camera.to.lookAt.x = position.x;
        gameWork.camera.to.lookAt.y = position.y;
        gameWork.camera.to.lookAt.z = position.z;
    }
    cameraRefMoveFlag = 0;
    if (cameraCamMoveFlag == 0) {
        gameWork.camera.range.lockPosition.x = orbitPos.x;
        gameWork.camera.range.lockPosition.y = orbitPos.y;
        gameWork.camera.range.lockPosition.z = orbitPos.z;
        gameWork.camera.range.angles.x = orbitAngles.x;
        gameWork.camera.range.angles.y = orbitAngles.y;
        gameWork.camera.range.angles.w = orbitAngles.w;
    } else {
        gameWork.camera.to.position.x = orbitPos.x;
        gameWork.camera.to.position.y = orbitPos.y;
        gameWork.camera.to.position.z = orbitPos.z;
        gameWork.camera.to.angles.x = orbitAngles.x;
        gameWork.camera.to.angles.y = orbitAngles.y;
        gameWork.camera.to.angles.w = orbitAngles.w;
    }
    cameraCamMoveFlag = 0;
    return 1;
}

int cameraSetCamPos(int scriptHandle)
{
    float posX;
    float posY;
    float posZ;

    cameraArgCount = 3;
    if (cameraCheckArgCount(scriptHandle) == 0)
    {
        return 0;
    }
    posX = cameraCentiToFloat(func_A9F0C0(scriptHandle, 1));
    posY = cameraCentiToFloat(func_A9F0C0(scriptHandle, 2));
    posZ = cameraCentiToFloat(func_A9F0C0(scriptHandle, 3));
    if (cameraCamMoveFlag == 0)
    {
        gameWork.camera.range.lockPosition.x = posX;
        gameWork.camera.range.lockPosition.y = posY;
        gameWork.camera.range.lockPosition.z = posZ;
    }
    else
    {
        gameWork.camera.to.position.x = posX;
        gameWork.camera.to.position.y = posY;
        gameWork.camera.to.position.z = posZ;
    }
    cameraCamMoveFlag = 0;
    return 1;
}

int cameraSetCamAng(int act)
{
    Vector4 input;
    int raw_x;
    int raw_y;
    int raw_w;
    int ok;

    cameraArgCount = 3;
    ok = cameraCheckArgCount(act);
    if (ok != 0)
    {
        raw_x = func_A9F0C0(act, 1);
        input.x = cameraCentiDegToRad(raw_x);
        raw_y = func_A9F0C0(act, 2);
        input.y = cameraCentiDegToRad(raw_y);
        input.z = 0.0f;
        raw_w = func_A9F0C0(act, 3);
        input.w = cameraCentiToFloat(raw_w);
        if (cameraCamMoveFlag == 0)
        {
            gameWork.camera.range.angles.w = input.w;
            gameWork.camera.range.angles.x = input.x;
            gameWork.camera.range.angles.y = input.y;
            cameraOrbitPosition(&gameWork.camera.range.lockPosition.x, &gameWork.camera.range.lookAt.x, &input.x);
        }
        else
        {
            gameWork.camera.to.angles.w = input.w;
            gameWork.camera.to.angles.x = input.x;
            gameWork.camera.to.angles.y = input.y;
            cameraOrbitPosition(&gameWork.camera.to.position.x, &gameWork.camera.to.lookAt.x, &input.x);
        }
        cameraCamMoveFlag = 0;
        ok = 1;
    }
    return ok;
}

int SetCamAct(int act)
{
    float transformed[3];
    Vector4 input;
    int frame_a;
    int frame_b;
    int raw_x;
    int raw_y;
    int raw_z;
    int ok;

    cameraArgCount = 5;
    ok = cameraCheckArgCount(act);
    if (ok != 0)
    {
        frame_a = func_A9F0C0(act, 1);
        frame_b = func_A9F0C0(act, 2);
        raw_x = func_A9F0C0(act, 3);
        input.x = cameraCentiToFloat(raw_x);
        raw_y = func_A9F0C0(act, 4);
        input.y = cameraCentiToFloat(raw_y);
        raw_z = func_A9F0C0(act, 5);
        input.z = cameraCentiToFloat(raw_z);
        input.w = 0.0f;
        cameraApplyAnchorOffset(transformed, frame_a, frame_b, &input.x);
        if (cameraCamMoveFlag == 0)
        {
            gameWork.camera.range.lockPosition.x = transformed[0];
            gameWork.camera.range.lockPosition.y = transformed[1];
            gameWork.camera.range.lockPosition.z = transformed[2];
        }
        else
        {
            gameWork.camera.to.position.x = transformed[0];
            gameWork.camera.to.position.y = transformed[1];
            gameWork.camera.to.position.z = transformed[2];
        }
        cameraCamMoveFlag = 0;
        ok = 1;
    }
    return ok;
}

int SetCamActR(int act)
{
    float transformed[3];
    Vector4 input;
    int frame_a;
    int frame_b;
    int raw_x;
    int raw_y;
    int raw_z;
    int ok;

    cameraArgCount = 5;
    ok = cameraCheckArgCount(act);
    if (ok != 0)
    {
        frame_a = func_A9F0C0(act, 1);
        frame_b = func_A9F0C0(act, 2);
        raw_x = func_A9F0C0(act, 3);
        input.x = cameraCentiToFloat(raw_x);
        raw_y = func_A9F0C0(act, 4);
        input.y = cameraCentiToFloat(raw_y);
        raw_z = func_A9F0C0(act, 5);
        input.z = cameraCentiToFloat(raw_z);
        input.w = 1.0f;
        cameraApplyAnchorOffset(transformed, frame_a, frame_b, &input.x);
        if (cameraCamMoveFlag == 0)
        {
            gameWork.camera.range.lockPosition.x = transformed[0];
            gameWork.camera.range.lockPosition.y = transformed[1];
            gameWork.camera.range.lockPosition.z = transformed[2];
        }
        else
        {
            gameWork.camera.to.position.x = transformed[0];
            gameWork.camera.to.position.y = transformed[1];
            gameWork.camera.to.position.z = transformed[2];
        }
        cameraCamMoveFlag = 0;
        ok = 1;
    }
    return ok;
}

int SetCamAct2(int scriptHandle)
{
    float anchorPos[3];
    Vector4 angles;
    float orbitPos[3];
    int anchorIndex;
    int anchorSubIndex;

    cameraArgCount = 5;
    if (cameraCheckArgCount(scriptHandle) == 0)
    {
        return 0;
    }
    anchorIndex = func_A9F0C0(scriptHandle, 1);
    anchorSubIndex = func_A9F0C0(scriptHandle, 2);
    angles.x = cameraCentiDegToRad(func_A9F0C0(scriptHandle, 3));
    angles.y = cameraCentiDegToRad(func_A9F0C0(scriptHandle, 4));
    angles.z = 0.0f;
    angles.w = cameraCentiToFloat(func_A9F0C0(scriptHandle, 5));
    cameraAnchorGet(anchorPos, anchorIndex, anchorSubIndex);
    cameraOrbitPosition(orbitPos, anchorPos, &angles.x);
    if (cameraCamMoveFlag == 0)
    {
        gameWork.camera.range.lockPosition.x = orbitPos[0];
        gameWork.camera.range.lockPosition.y = orbitPos[1];
        gameWork.camera.range.lockPosition.z = orbitPos[2];
        gameWork.camera.range.angles.x = angles.x;
        gameWork.camera.range.angles.y = angles.y;
        gameWork.camera.range.angles.w = angles.w;
    }
    else
    {
        gameWork.camera.to.position.x = orbitPos[0];
        gameWork.camera.to.position.y = orbitPos[1];
        gameWork.camera.to.position.z = orbitPos[2];
        gameWork.camera.to.angles.x = angles.x;
        gameWork.camera.to.angles.y = angles.y;
        gameWork.camera.to.angles.w = angles.w;
    }
    cameraCamMoveFlag = 0;
    return 1;
}

int SetCamAct2R(int act)
{
    float scratch[4];
    Vector4 input;
    float transformed[3];
    struct CameraOffsetRecord *offset_record;
    int frame_a;
    int frame_b;
    int raw_x;
    int raw_y;
    int raw_w;
    int ok;

    cameraArgCount = 5;
    ok = cameraCheckArgCount(act);
    if (ok != 0)
    {
        frame_a = func_A9F0C0(act, 1);
        frame_b = func_A9F0C0(act, 2);
        raw_x = func_A9F0C0(act, 3);
        input.x = cameraCentiDegToRad(raw_x);
        raw_y = func_A9F0C0(act, 4);
        input.y = cameraCentiDegToRad(raw_y);
        input.z = 0.0f;
        raw_w = func_A9F0C0(act, 5);
        input.w = cameraCentiToFloat(raw_w);
        offset_record = cameraAnchorGet(scratch, frame_a, frame_b);
        input.x += offset_record->offsetX;
        input.y += offset_record->offsetY;
        cameraOrbitPosition(transformed, scratch, &input.x);
        if (cameraCamMoveFlag == 0)
        {
            float transformed_x = transformed[0];
            float transformed_y = transformed[1];
            float transformed_z = transformed[2];
            float input_x = input.x;
            float input_y = input.y;
            float input_w = input.w;

            gameWork.camera.range.angles.w = input_w;
            gameWork.camera.range.lockPosition.y = transformed_y;
            gameWork.camera.range.lockPosition.z = transformed_z;
            gameWork.camera.range.angles.x = input_x;
            gameWork.camera.range.angles.y = input_y;
            gameWork.camera.range.lockPosition.x = transformed_x;
        }
        else
        {
            float transformed_x = transformed[0];
            float transformed_y = transformed[1];
            float transformed_z = transformed[2];
            float input_x = input.x;
            float input_y = input.y;
            float input_w = input.w;

            gameWork.camera.to.angles.w = input_w;
            gameWork.camera.to.position.y = transformed_y;
            gameWork.camera.to.position.z = transformed_z;
            gameWork.camera.to.angles.x = input_x;
            gameWork.camera.to.angles.y = input_y;
            gameWork.camera.to.position.x = transformed_x;
        }
        cameraCamMoveFlag = 0;
        ok = 1;
    }
    return ok;
}

int SetCamAct3(int scriptHandle)
{
    float startAnchor[3];
    float endAnchor[3];
    float blend;
    float offsetX;
    float offsetY;
    float offsetZ;
    float posX;
    float posY;
    float posZ;
    int startIndex;
    int startSubIndex;
    int endIndex;
    int endSubIndex;
    int centiOffsetX;
    int centiOffsetY;
    int centiOffsetZ;
    int blendRaw;

    cameraArgCount = 8;
    if (cameraCheckArgCount(scriptHandle) == 0)
    {
        return 0;
    }
    startIndex = func_A9F0C0(scriptHandle, 1);
    startSubIndex = func_A9F0C0(scriptHandle, 2);
    endIndex = func_A9F0C0(scriptHandle, 3);
    endSubIndex = func_A9F0C0(scriptHandle, 4);
    centiOffsetX = func_A9F0C0(scriptHandle, 5);
    centiOffsetY = func_A9F0C0(scriptHandle, 6);
    centiOffsetZ = func_A9F0C0(scriptHandle, 7);
    blendRaw = func_A9F0C0(scriptHandle, 8);
    cameraAnchorGet(startAnchor, startIndex, startSubIndex);
    blend = blendRaw;
    cameraAnchorGet(endAnchor, endIndex, endSubIndex);
    offsetX = cameraCentiToFloat(centiOffsetX);
    posX = (endAnchor[0] - startAnchor[0]) * blend + startAnchor[0] + offsetX;
    offsetY = cameraCentiToFloat(centiOffsetY);
    posY = (endAnchor[1] - startAnchor[1]) * blend + startAnchor[1] + offsetY;
    offsetZ = cameraCentiToFloat(centiOffsetZ);
    posZ = (endAnchor[2] - startAnchor[2]) * blend + startAnchor[2] + offsetZ;
    if (cameraCamMoveFlag == 0)
    {
        gameWork.camera.range.lockPosition.x = posX;
        gameWork.camera.range.lockPosition.y = posY;
        gameWork.camera.range.lockPosition.z = posZ;
    }
    else
    {
        gameWork.camera.to.position.x = posX;
        gameWork.camera.to.position.y = posY;
        gameWork.camera.to.position.z = posZ;
    }
    cameraCamMoveFlag = 0;
    return 1;
}

int OffsCamPos(int scriptHandle)
{
    float posX;
    float posY;
    float posZ;

    cameraArgCount = 3;
    if (cameraCheckArgCount(scriptHandle) == 0)
    {
        return 0;
    }
    posX = cameraCentiToFloat(func_A9F0C0(scriptHandle, 1));
    posY = cameraCentiToFloat(func_A9F0C0(scriptHandle, 2));
    posZ = cameraCentiToFloat(func_A9F0C0(scriptHandle, 3));
    posX += gameWork.camera.range.lockPosition.x;
    posY += gameWork.camera.range.lockPosition.y;
    posZ += gameWork.camera.range.lockPosition.z;
    if (cameraCamMoveFlag == 0)
    {
        gameWork.camera.range.lockPosition.x = posX;
        gameWork.camera.range.lockPosition.y = posY;
        gameWork.camera.range.lockPosition.z = posZ;
    }
    else
    {
        gameWork.camera.to.position.x = posX;
        gameWork.camera.to.position.y = posY;
        gameWork.camera.to.position.z = posZ;
    }
    cameraCamMoveFlag = 0;
    return 1;
}

int OffsCamAng(int scriptHandle)
{
    Vector4 angles;

    cameraArgCount = 3;
    if (cameraCheckArgCount(scriptHandle) == 0)
    {
        return 0;
    }
    angles.x = cameraCentiDegToRad(func_A9F0C0(scriptHandle, 1));
    angles.y = cameraCentiDegToRad(func_A9F0C0(scriptHandle, 2));
    angles.z = 0.0f;
    angles.w = cameraCentiToFloat(func_A9F0C0(scriptHandle, 3));
    angles.x += gameWork.camera.range.angles.x;
    angles.y += gameWork.camera.range.angles.y;
    angles.w += gameWork.camera.range.angles.w;
    if (cameraCamMoveFlag == 0)
    {
        gameWork.camera.range.angles.x = angles.x;
        gameWork.camera.range.angles.y = angles.y;
        gameWork.camera.range.angles.w = angles.w;
        cameraOrbitPosition(&gameWork.camera.range.lockPosition.x, &gameWork.camera.range.lookAt.x, &angles.x);
    }
    else
    {
        gameWork.camera.to.angles.x = angles.x;
        gameWork.camera.to.angles.y = angles.y;
        gameWork.camera.to.angles.w = angles.w;
        cameraOrbitPosition(&gameWork.camera.to.position.x, &gameWork.camera.to.lookAt.x, &angles.x);
    }
    cameraCamMoveFlag = 0;
    return 1;
}

int CamLock(int value) {
    cameraArgCount = 0;
    if (cameraCheckArgCount(value) != 0) {
        gameWork.camera.slot[1].duration = 0;
        return 1;
    }
    return 0;
}

int cameraMoveRefMode(int scriptHandle) {
    cameraArgCount = 0;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    cameraRefMoveFlag = 1;
    cameraMoveMode |= CAMERA_MOVE_REF;
    return 1;
}

int MoveCamMode(int scriptHandle) {
    cameraArgCount = 0;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    cameraCamMoveFlag = 1;
    cameraMoveMode |= CAMERA_MOVE_CAM;
    return 1;
}

int cameraMove(int act)
{
    int ok;
    int slot_1;
    int slot_2;
    int slot_3;

    cameraArgCount = 3;
    ok = cameraCheckArgCount(act);
    if (ok != 0)
    {
        slot_1 = func_A9F0C0(act, 1);
        slot_2 = func_A9F0C0(act, 2);
        slot_3 = func_A9F0C0(act, 3);
        if (cameraMoveMode == 0)
        {
            cameraMoveMode = 3;
        }
        cameraApplySlots(cameraMoveMode, slot_1, slot_2, slot_3);
        cameraMoveMode = 0;
        ok = 1;
    }
    return ok;
}

int WaitMoveRef(int scriptHandle, int *waitFlag) {
    cameraArgCount = 0;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    if (gameWork.camera.slot[0].fraction < 1.0f) {
        *waitFlag = 1;
    }
    return 1;
}

int WaitMoveCam(int handle, int *flag) {
    cameraArgCount = 0;
    if (cameraCheckArgCount(handle) == 0) {
        return 0;
    }
    if (gameWork.camera.slot[1].fraction < 1.0f) {
        *flag = 1;
    }
    return 1;
}

int MoveRefStop(int handle) {
    cameraArgCount = 0;
    if (cameraCheckArgCount(handle) == 0) {
        return 0;
    }
    gameWork.camera.slot[0].progress = 0;
    gameWork.camera.slot[0].duration = 0;
    return 1;
}

int MoveCamStop(int handle) {
    cameraArgCount = 0;
    if (cameraCheckArgCount(handle) == 0) {
        return 0;
    }
    gameWork.camera.slot[1].progress = 0;
    gameWork.camera.slot[1].duration = 0;
    return 1;
}

int MovePersStop(int scriptHandle) {
    cameraArgCount = 0;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    gameWork.camera.slot[2].progress = 0;
    *(int *) &gameWork.camera.slot[2].duration = 0;
    return 1;
}

int MoveBankStop(int scriptHandle) {
    cameraArgCount = 0;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    gameWork.camera.slot[3].progress = 0;
    gameWork.camera.slot[3].duration = 0.0f;
    return 1;
}

int cameraShake(int scriptHandle)
{
    float amplitude;
    float step;
    int shakeMode;
    int paramA;
    int paramC;
    int frames;

    cameraArgCount = 5;
    if (cameraCheckArgCount(scriptHandle) == 0)
    {
        return 0;
    }
    shakeMode = func_A9F0C0(scriptHandle, 1);
    amplitude = cameraCentiDegToRad(func_A9F0C0(scriptHandle, 2));
    paramA = func_A9F0C0(scriptHandle, 3);
    paramC = func_A9F0C0(scriptHandle, 4);
    frames = func_A9F0C0(scriptHandle, 5);
    step = amplitude / I2F(frames);
    gameWork.camera.shakeFlags = shakeMode;
    if (shakeMode & CAMERA_SHAKE_CHANNEL_A)
    {
        gameWork.camera.waves[0].value = amplitude;
        gameWork.camera.waves[0].step = step;
        gameWork.camera.waves[0].period = paramA;
        gameWork.camera.waves[0].phase = 0;
        gameWork.camera.waves[0].hold = paramC;
        gameWork.camera.waves[0].remaining = frames;
    }
    if (shakeMode & CAMERA_SHAKE_CHANNEL_B)
    {
        gameWork.camera.waves[1].value = amplitude;
        gameWork.camera.waves[1].step = step;
        gameWork.camera.waves[1].period = paramA;
        gameWork.camera.waves[1].hold = paramC;
        gameWork.camera.waves[1].remaining = frames;
    }
    if (shakeMode & CAMERA_SHAKE_CHANNEL_C)
    {
        gameWork.camera.waves[2].value = amplitude;
        gameWork.camera.waves[2].step = step;
        gameWork.camera.waves[2].period = paramA;
        gameWork.camera.waves[2].hold = paramC;
        gameWork.camera.waves[2].remaining = frames;
    }
    return 1;
}

int HandShake(int scriptHandle) {
    cameraArgCount = 0;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    gameWork.camera.sway[2] = 0;
    gameWork.camera.shakeFlags ^= 8;
    gameWork.camera.sway[3] = 0;
    gameWork.camera.sway[0] = 0;
    gameWork.camera.sway[1] = 0;
    return 1;
}

int cameraSetBank(int handle) {
    cameraArgCount = 1;
    if (cameraCheckArgCount(handle) == 0) {
        return 0;
    }
    gameWork.camera.range.lookAt.w = cameraCentiDegToRad(func_A9F0C0(handle, 1));
    return 1;
}

int cameraSetPers(int handle) {
    cameraArgCount = 1;
    if (cameraCheckArgCount(handle) == 0) {
        return 0;
    }
    gameWork.camera.range.lockPosition.w = cameraCentiDegToRad(func_A9F0C0(handle, 1));
    return 1;
}

int MoveBank(int act)
{
    int angle_raw;
    int bank_byte;
    int bank_raw;
    int ok;

    cameraArgCount = 3;
    ok = cameraCheckArgCount(act);
    if (ok != 0)
    {
        angle_raw = func_A9F0C0(act, 1);
        bank_byte = func_A9F0C0(act, 2);
        bank_raw = func_A9F0C0(act, 3);
        gameWork.camera.to.lookAt.w = cameraCentiDegToRad(angle_raw);
        gameWork.camera.slot[3].easeMode = bank_byte;
        gameWork.camera.slot[3].blendMode = 0;
        gameWork.camera.slot[3].fraction = 0;
        gameWork.camera.slot[3].progress = 0;
        gameWork.camera.slot[3].duration = I2F(bank_raw);
        ok = 1;
    }
    return ok;
}

int MovePers(int scriptHandle)
{
    int angleCenti;
    int persByte;
    int persRaw;
    int ok;

    cameraArgCount = 3;
    ok = cameraCheckArgCount(scriptHandle);
    if (ok != 0)
    {
        angleCenti = func_A9F0C0(scriptHandle, 1);
        persByte = func_A9F0C0(scriptHandle, 2);
        persRaw = func_A9F0C0(scriptHandle, 3);
        gameWork.camera.to.position.w = cameraCentiDegToRad(angleCenti);
        gameWork.camera.slot[2].easeMode = persByte;
        gameWork.camera.slot[2].blendMode = 0;
        gameWork.camera.slot[2].fraction = 0;
        gameWork.camera.slot[2].progress = 0;
        gameWork.camera.slot[2].duration = I2F(persRaw);
        ok = 1;
    }
    return ok;
}

int cameraGetRefPos(int scriptHandle) {
    int *outWords;
    float refPosY;
    float refPosZ;

    cameraArgCount = 1;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    outWords = func_A9F298(scriptHandle, func_A9F0C0(scriptHandle, 1));
    refPosY = gameWork.camera.range.lookAt.y;
    refPosZ = gameWork.camera.range.lookAt.z;
    outWords[0] = cameraFloatToCenti(gameWork.camera.range.lookAt.x);
    outWords[1] = cameraFloatToCenti(refPosY);
    outWords[2] = cameraFloatToCenti(refPosZ);
    return 1;
}

int GetRefAng(int scriptHandle) {
    int *outWords;
    float angleY;

    cameraArgCount = 1;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    outWords = func_A9F298(scriptHandle, func_A9F0C0(scriptHandle, 1));
    angleY = gameWork.camera.range.angles.y;
    outWords[0] = cameraRadToCentiDeg(gameWork.camera.range.angles.x);
    outWords[1] = cameraRadToCentiDeg(angleY);
    return 1;
}

int GetCamPos(int scriptHandle) {
    int *outWords;
    float camPosY;
    float camPosZ;

    cameraArgCount = 1;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    outWords = func_A9F298(scriptHandle, func_A9F0C0(scriptHandle, 1));
    camPosY = gameWork.camera.range.lockPosition.y;
    camPosZ = gameWork.camera.range.lockPosition.z;
    outWords[0] = cameraFloatToCenti(gameWork.camera.range.lockPosition.x);
    outWords[1] = cameraFloatToCenti(camPosY);
    outWords[2] = cameraFloatToCenti(camPosZ);
    return 1;
}

int GetCamAng(int scriptHandle) {
    int *outWords;
    float camAngleY;
    float camAngleW;

    cameraArgCount = 1;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    outWords = func_A9F298(scriptHandle, func_A9F0C0(scriptHandle, 1));
    camAngleY = gameWork.camera.range.angles.y;
    camAngleW = gameWork.camera.range.angles.w;
    outWords[0] = cameraRadToCentiDeg(gameWork.camera.range.angles.x);
    outWords[1] = cameraRadToCentiDeg(camAngleY);
    outWords[2] = cameraFloatToCenti(camAngleW);
    return 1;
}

extern void *cameraNatives[];
extern u8 cameraModuleName[];

void cameraRegisterNatives(void) {
    func_A9CDB0(cameraModuleName, cameraNatives);
    cameraRefMoveFlag = 0;
    cameraCamMoveFlag = 0;
    cameraMoveMode = 0;
}

unsigned char ladderModuleName[8] = "ladder";
unsigned char uwamonoLadderState[8] = "";
unsigned char D_0069D328[8] = "SetPers";
unsigned char D_0069D330[8] = "SetBank";
unsigned char D_0069D338[8] = "Shake";
unsigned char D_0069D340[8] = "Move";
unsigned char D_0069D348[8] = "CamLock";
unsigned char D_0069D350[8] = "RefLock";
unsigned char D_0069D358[8] = "cfNorth";
unsigned char D_0069D360[8] = "cfHokan";
unsigned char D_0069D368[8] = "cfLock";
unsigned char D_0069D370[8] = "cfCamID";
unsigned char D_0069D378[8] = "cfSet";
unsigned char D_0069D380[8] = "mode";
unsigned char D_0069D388[8] = "reset";
unsigned char cameraModuleName[8] = "camera";

const f32 cameraCentiScale = 0.00999999978f;
const f32 cameraRadPerCentiDeg = 0.000174532935f;
const f32 cameraRadPerCentiDegDiv = 0.000174532935f;

void ladderSetLadder();
void ladderSetEntrance();

/* .data of this file (0x3E7020-0x3E71E8): the ladder and camera script natives. */

/* Native table of the ladder module. */
void *ladderNatives[8] = {
    "setLadder",
    ladderSetLadder,
    "setEntrance",
    ladderSetEntrance,
    0,
    0,
    0,
    0
};

/* Camera reference position: x = 0, y = 0, z = -10, w = 1. */
Vector4 D_003E7040 = { 0.0f, 0.0f, -10.0f, 1.0f };

/* Native table of the camera module. */
void *cameraNatives[102] = {
    (void *) D_0069D388,
    talkReset,
    (void *) D_0069D380,
    cameraMode,
    "AngleMode",
    AngleMode,
    "PosReset",
    cameraPosReset,
    (void *) D_0069D378,
    cameraCfSet,
    "cfChange",
    cfChange,
    "cfRotRange",
    cameraCfRotRange,
    "cfRangeMode",
    cameraCfRangeMode,
    (void *) D_0069D370,
    cameraCfCamId,
    (void *) D_0069D368,
    cameraCfLock,
    "cfSelectCHG",
    cameraCfSelectChg,
    (void *) D_0069D360,
    cameraCfHokan,
    "cfBattleCam",
    cameraCfBattleCam,
    (void *) D_0069D358,
    cfNorth,
    "SetRefPos",
    cameraSetRefPos,
    "SetRefAct",
    cameraSetRefAct,
    "SetRefActR",
    SetRefActR,
    "SetRefAng",
    cameraSetRefAng,
    (void *) D_0069D350,
    RefLock,
    "SetRefCamActR",
    SetRefCamActR,
    "SetRefCamAct3R",
    SetRefCamAct3R,
    "SetCamPos",
    cameraSetCamPos,
    "SetCamAng",
    cameraSetCamAng,
    "SetCamAct",
    SetCamAct,
    "SetCamActR",
    SetCamActR,
    "SetCamAct2",
    SetCamAct2,
    "SetCamAct2R",
    SetCamAct2R,
    "SetCamAct3",
    SetCamAct3,
    "OffsCamPos",
    OffsCamPos,
    "OffsCamAng",
    OffsCamAng,
    (void *) D_0069D348,
    CamLock,
    "MoveRefMode",
    cameraMoveRefMode,
    "MoveCamMode",
    MoveCamMode,
    (void *) D_0069D340,
    cameraMove,
    "WaitMoveRef",
    WaitMoveRef,
    "WaitMoveCam",
    WaitMoveCam,
    "MoveRefStop",
    MoveRefStop,
    "MoveCamStop",
    MoveCamStop,
    "MovePersStop",
    MovePersStop,
    "MoveBankStop",
    MoveBankStop,
    (void *) D_0069D338,
    cameraShake,
    "HandShake",
    HandShake,
    (void *) D_0069D330,
    cameraSetBank,
    (void *) D_0069D328,
    cameraSetPers,
    "MoveBank",
    MoveBank,
    "MovePers",
    MovePers,
    "GetRefPos",
    cameraGetRefPos,
    "GetRefAng",
    GetRefAng,
    "GetCamPos",
    GetCamPos,
    "GetCamAng",
    GetCamAng,
    0,
    0
};
