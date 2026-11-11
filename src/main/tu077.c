#include "common.h"
#include "shared.h"
#include "main/tu051.h"
#include "main/tu077.h"

/* gameWork.flags bits that the camera commands set. */
enum CameraFlagBits {
    CAMERA_FLAG_POS_RESET = 0x20,
    CAMERA_FLAG_BATTLE = 0x200
};

/* cameraMoveMode bits: a move command selects the reference block or the camera block. */
enum CameraMoveBits {
    CAMERA_MOVE_REF = 1,
    CAMERA_MOVE_CAM = 2
};

/* gameWork.shakeMode bits: one per shake channel (shakeA, shakeB, shakeC). */
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
extern u8 ladderNatives[];
extern u8 ladderModuleName[];

void ladderRegisterNatives(void) {
    func_A9CDB0(ladderModuleName, ladderNatives);
}

float I2F(int value);
extern f32 cameraCentiScale;

f32 cameraCentiToFloat(int value) {
    return I2F(value) * cameraCentiScale;
}

int F2I(f32);

int cameraFloatToCenti(f32 value) {
    return F2I(value * 100.0f);
}

extern f32 cameraRadPerCentiDeg;

f32 cameraCentiDegToRad(int value) {
    return I2F(value) * cameraRadPerCentiDeg;
}

extern f32 cameraRadPerCentiDegDiv;

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

/* The camera block of gameWork as the camera functions of this file read it.
 * The pers (+0x160) and bank (+0x170) blocks share one layout; their stop
 * words are the int views of wordA/floatB. */
struct CameraWork {
    /* 0x000 */ unsigned char unmodeled_000[0xD0];
    /* 0x0D0 */ unsigned short flags;
    /* 0x0D2 */ unsigned char modeByteA;
    /* 0x0D3 */ unsigned char modeByteB;
    /* 0x0D4 */ unsigned char unmodeled_0d4[0x1];
    /* 0x0D5 */ unsigned char cfSlotIndex;
    /* 0x0D6 */ unsigned char unmodeled_0d6[0x1A];
    /* 0x0F0 */ union {
        unsigned char shakeMode;
        unsigned char handShakeFlags;
    };
    /* 0x0F1 */ unsigned char unmodeled_0F1[0xF];
    /* 0x100 */ struct CameraShakeRecord shakeA;
    /* 0x110 */ struct CameraShakeRecord shakeB;
    /* 0x120 */ struct CameraShakeRecord shakeC;
    /* 0x130 */ int handShakeWord0;
    /* 0x134 */ int handShakeWord1;
    /* 0x138 */ int handShakeWord2;
    /* 0x13C */ int handShakeWord3;
    /* 0x140 */ unsigned char unmodeled_140[0x4];
    /* 0x144 */ int refStopWord0;
    /* 0x148 */ int refStopWord1;
    /* 0x14C */ unsigned char unmodeled_14c[0x8];
    /* 0x154 */ int camStopWord0;
    /* 0x158 */ int camStopWord1;
    /* 0x15C */ unsigned char unmodeled_15c[0x6];
    /* 0x162 */ unsigned char persByteA;
    /* 0x163 */ signed char persByteB;
    /* 0x164 */ int persWordA;
    /* 0x168 */ float persFloatB;
    /* 0x16C */ int persWordB;
    /* 0x170 */ unsigned char unmodeled_170[0x2];
    /* 0x172 */ unsigned char bankByteA;
    /* 0x173 */ signed char bankByteB;
    /* 0x174 */ int bankWordA;
    /* 0x178 */ float bankFloatB;
    /* 0x17C */ int bankWordB;
    /* 0x180 */ union { int refAnchorIndex; float refAnchorIndexFloat; };
    /* 0x184 */ union { short refAnchorSubIndex; float refAnchorSubIndexFloat; };
    /* 0x188 */ unsigned char lockFlag02;
    /* 0x189 */ unsigned char lockFlag04;
    /* 0x18A */ unsigned char lockFlag08;
    /* 0x18B */ unsigned char lockFlags21;
    /* 0x18C */ unsigned char cfHokanByte;
    /* 0x18D */ unsigned char unmodeled_18d[0x2];
    /* 0x18F */ u8 northOutOfRange;
    /* 0x190 */ f32 northRad;
    /* 0x194 */ unsigned char unmodeled_194[0xC];
    /* 0x1A0 */ Vector4 refOffset;    /* offset from the reference anchor (cameraSetRefAct, SetRefCamActR) */
    /* 0x1B0 */ Vector4 rotRange;     /* camera.cfRotRange limits in radians */
    /* 0x1C0 */ Vector4 camPos;       /* camera position (orbit result) */
    /* 0x1D0 */ Vector4 refPos;       /* reference position, written by cameraSetRefPos */
    /* 0x1E0 */ Vector4 camAngles;    /* camera angles x, y in radians; w is a distance (GetCamAng reads it in centi units) */
    /* 0x1F0 */ unsigned char unmodeled_1F0[0x30];
    /* 0x220 */ Vector4 camMovePos;   /* target position while a camera move is pending */
    /* 0x230 */ Vector4 refMovePos;   /* target while a reference move is pending; w is the MoveBank angle */
    /* 0x240 */ Vector4 camMoveAngles; /* target angles while a camera move is pending */
};

/* Script argument count of the camera command now running; cameraCheckArgCount compares it with argc. */
extern u8 cameraArgCount;
extern u8 cameraRangeMode[];
extern f32 D_002B5EEC[];
extern f32 cameraPersAngle[];
extern f32 cameraBankAngle[];
/* The camera state block: every command below reads or writes its fields. */
extern struct CameraWork gameWork;
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
    gameWork.modeByteA = modeA;
    gameWork.modeByteB = modeB;
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
    gameWork.flags |= CAMERA_FLAG_POS_RESET;
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
    unsigned char modeA;               /* +0x00: copy of gameWork.modeByteA */
    unsigned char unmodeled_01;
    unsigned char slotIndex;           /* +0x02: copy of gameWork byte 0xD5 */
    unsigned char unmodeled_03[0xD];
    struct CameraStopBlock stopBlock;  /* +0x10: copy of gameWork 0x140..0x14F */
    struct CameraRefBlock refBlock;        /* +0x20: copy of gameWork 0x180..0x1F0 */
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
    slot->modeA = gameWork.modeByteA;
    slot->slotIndex = gameWork.cfSlotIndex;
    *(struct CameraStopBlock *)&slot->stopBlock = *(struct CameraStopBlock *)&gameWork.unmodeled_0d4[0x6C];
    *(struct CameraRefBlock *)&slot->refBlock = *(struct CameraRefBlock *)&gameWork.refAnchorIndex;
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
    slot->modeA = gameWork.modeByteA;
    slot->slotIndex = gameWork.cfSlotIndex;
    *(struct CameraRefBlock *)&slot->refBlock = *(struct CameraRefBlock *)&gameWork.refAnchorIndex;
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
    gameWork.rotRange.x = radX;
    gameWork.rotRange.y = radY;
    gameWork.rotRange.z = radZ;
    gameWork.rotRange.w = radW;
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
    gameWork.lockFlags21 = lockFlags & 0x21;
    gameWork.lockFlag02 = lockFlags & 0x2;
    gameWork.lockFlag04 = lockFlags & 0x4;
    gameWork.lockFlag08 = lockFlags & 0x8;
    return 1;
}

int cameraCfRangeMode(int handle) {
    cameraArgCount = 1;
    if (cameraCheckArgCount(handle) == 0) {
        return 0;
    }
    cameraRangeMode[0] = func_A9F0C0(handle, 1);
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
    entry->unmodeled_02[0] = gameWork.cfHokanByte;
    *(struct CameraBlock16 *)&entry->unmodeled_02[2] = *(struct CameraBlock16 *)&gameWork.unmodeled_0d4[0x6C];
    return 1;
}

int cameraCfBattleCam(int scriptHandle) {
    cameraArgCount = 1;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    gameWork.flags &= 0xFDFF;
    if (func_A9F0C0(scriptHandle, 1) != 0) {
        gameWork.flags |= CAMERA_FLAG_BATTLE;
    }
    gameWork.refStopWord1 = 0;
    gameWork.camStopWord1 = 0;
    *(int *) &gameWork.persFloatB = 0;
    gameWork.bankFloatB = 0;
    gameWork.flags = (gameWork.flags & 0xFFEF) | 0x80;
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
    gameWork.northOutOfRange = out_of_range;
    gameWork.northRad = cameraCentiDegToRad(looked_up);
    return 1;
}

extern u8 cameraRefMoveFlag;

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
        gameWork.refPos.x = posX;
        gameWork.refPos.y = posY;
        gameWork.refPos.z = posZ;
        gameWork.refAnchorIndex = CAMERA_ANCHOR_NONE;
        gameWork.refAnchorSubIndex = 0;
        gameWork.refOffset.x = 0;
        gameWork.refOffset.y = 0;
        gameWork.refOffset.z = 0;
    } else {
        gameWork.refMovePos.x = posX;
        gameWork.refMovePos.y = posY;
        gameWork.refMovePos.z = posZ;
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
        gameWork.refPos.x = position.x;
        gameWork.refPos.y = position.y;
        gameWork.refPos.z = position.z;
        gameWork.refOffset.x = offset.x;
        gameWork.refOffset.y = offset.y;
        gameWork.refOffset.z = offset.z;
        gameWork.refAnchorIndex = anchorIndex;
        gameWork.refAnchorSubIndex = anchorSubIndex;
        gameWork.refOffset.w = offset.w;
    } else {
        gameWork.refMovePos.x = position.x;
        gameWork.refMovePos.y = position.y;
        gameWork.refMovePos.z = position.z;
    }
    cameraRefMoveFlag = 0;
    return 1;
}

void cameraOrbitPosition(float *dst, float *a, float *b);
extern u8 cameraCamMoveFlag;
extern u8 cameraMoveMode;

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

            gameWork.refAnchorSubIndex = frame_b;
            gameWork.refOffset.w = input_w;
            gameWork.refPos.x = transformed_x;
            gameWork.refPos.y = transformed_y;
            gameWork.refPos.z = transformed_z;
            gameWork.refOffset.x = input_x;
            gameWork.refOffset.y = input_y;
            gameWork.refOffset.z = input_z;
            gameWork.refAnchorIndex = frame_a;
        }
        else
        {
            gameWork.refMovePos.x = transformed[0];
            gameWork.refMovePos.y = transformed[1];
            gameWork.refMovePos.z = transformed[2];
        }
        cameraRefMoveFlag = 0;
        ok = 1;
    }
    return ok;
}

extern u8 D_003E7040[];
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
    offset.x = gameWork.camPos.x;
    offset.y = gameWork.camPos.y;
    offset.z = gameWork.camPos.z;
    offset.w = 1.0f;
    xglMatrixStackUnit();
    xglMatrixStackTrans(&offset);
    xglMatrixStackRotY(rotY);
    xglMatrixStackRotX(rotX);
    xglMatrixStackTrans(D_003E7040);
    xglMatrixStackSave(matrix);
    if (cameraRefMoveFlag == 0) {
        gameWork.refPos.x = matrix[3][0];
        gameWork.refPos.y = matrix[3][1];
        gameWork.refPos.z = matrix[3][2];
        gameWork.refAnchorIndex = CAMERA_ANCHOR_NONE;
        gameWork.refAnchorSubIndex = 0;
        gameWork.refOffset.x = 0;
        gameWork.refOffset.y = 0;
        gameWork.refOffset.z = 0;
    } else {
        gameWork.refMovePos.x = matrix[3][0];
        gameWork.refMovePos.y = matrix[3][1];
        gameWork.refMovePos.z = matrix[3][2];
    }
    cameraRefMoveFlag = 0;
    return 1;
}

int RefLock(int scriptHandle) {
    cameraArgCount = 0;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    gameWork.refStopWord1 = 0;
    gameWork.refAnchorSubIndex = 0;
    gameWork.refAnchorIndex = CAMERA_ANCHOR_NONE;
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
        gameWork.refOffset.w = 1.0f;
        gameWork.refPos.x = position.x;
        gameWork.refPos.y = position.y;
        gameWork.refPos.z = position.z;
        gameWork.refAnchorIndex = CAMERA_ANCHOR_NONE;
        gameWork.refOffset.x = offset.x;
        gameWork.refOffset.y = offset.y;
        gameWork.refOffset.z = offset.z;
        gameWork.refAnchorSubIndex = 0;
    } else {
        gameWork.refMovePos.x = position.x;
        gameWork.refMovePos.y = position.y;
        gameWork.refMovePos.z = position.z;
    }
    cameraRefMoveFlag = 0;
    if (cameraCamMoveFlag == 0) {
        gameWork.camPos.x = orbitPos.x;
        gameWork.camPos.y = orbitPos.y;
        gameWork.camPos.z = orbitPos.z;
        gameWork.camAngles.x = orbitAngles.x;
        gameWork.camAngles.y = orbitAngles.y;
        gameWork.camAngles.w = orbitAngles.w;
    } else {
        gameWork.camMovePos.x = orbitPos.x;
        gameWork.camMovePos.y = orbitPos.y;
        gameWork.camMovePos.z = orbitPos.z;
        gameWork.camMoveAngles.x = orbitAngles.x;
        gameWork.camMoveAngles.y = orbitAngles.y;
        gameWork.camMoveAngles.w = orbitAngles.w;
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
        gameWork.refOffset.w = 1.0f;
        gameWork.refPos.x = position.x;
        gameWork.refPos.y = position.y;
        gameWork.refPos.z = position.z;
        gameWork.refOffset.x = 0.0f;
        gameWork.refOffset.y = 0.0f;
        gameWork.refOffset.z = 0.0f;
        gameWork.refAnchorIndex = CAMERA_ANCHOR_NONE;
        gameWork.refAnchorSubIndex = 0;
    } else {
        gameWork.refMovePos.x = position.x;
        gameWork.refMovePos.y = position.y;
        gameWork.refMovePos.z = position.z;
    }
    cameraRefMoveFlag = 0;
    if (cameraCamMoveFlag == 0) {
        gameWork.camPos.x = orbitPos.x;
        gameWork.camPos.y = orbitPos.y;
        gameWork.camPos.z = orbitPos.z;
        gameWork.camAngles.x = orbitAngles.x;
        gameWork.camAngles.y = orbitAngles.y;
        gameWork.camAngles.w = orbitAngles.w;
    } else {
        gameWork.camMovePos.x = orbitPos.x;
        gameWork.camMovePos.y = orbitPos.y;
        gameWork.camMovePos.z = orbitPos.z;
        gameWork.camMoveAngles.x = orbitAngles.x;
        gameWork.camMoveAngles.y = orbitAngles.y;
        gameWork.camMoveAngles.w = orbitAngles.w;
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
        gameWork.camPos.x = posX;
        gameWork.camPos.y = posY;
        gameWork.camPos.z = posZ;
    }
    else
    {
        gameWork.camMovePos.x = posX;
        gameWork.camMovePos.y = posY;
        gameWork.camMovePos.z = posZ;
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
            gameWork.camAngles.w = input.w;
            gameWork.camAngles.x = input.x;
            gameWork.camAngles.y = input.y;
            cameraOrbitPosition(&gameWork.camPos.x, &gameWork.refPos.x, &input.x);
        }
        else
        {
            gameWork.camMoveAngles.w = input.w;
            gameWork.camMoveAngles.x = input.x;
            gameWork.camMoveAngles.y = input.y;
            cameraOrbitPosition(&gameWork.camMovePos.x, &gameWork.refMovePos.x, &input.x);
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
            gameWork.camPos.x = transformed[0];
            gameWork.camPos.y = transformed[1];
            gameWork.camPos.z = transformed[2];
        }
        else
        {
            gameWork.camMovePos.x = transformed[0];
            gameWork.camMovePos.y = transformed[1];
            gameWork.camMovePos.z = transformed[2];
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
            gameWork.camPos.x = transformed[0];
            gameWork.camPos.y = transformed[1];
            gameWork.camPos.z = transformed[2];
        }
        else
        {
            gameWork.camMovePos.x = transformed[0];
            gameWork.camMovePos.y = transformed[1];
            gameWork.camMovePos.z = transformed[2];
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
        gameWork.camPos.x = orbitPos[0];
        gameWork.camPos.y = orbitPos[1];
        gameWork.camPos.z = orbitPos[2];
        gameWork.camAngles.x = angles.x;
        gameWork.camAngles.y = angles.y;
        gameWork.camAngles.w = angles.w;
    }
    else
    {
        gameWork.camMovePos.x = orbitPos[0];
        gameWork.camMovePos.y = orbitPos[1];
        gameWork.camMovePos.z = orbitPos[2];
        gameWork.camMoveAngles.x = angles.x;
        gameWork.camMoveAngles.y = angles.y;
        gameWork.camMoveAngles.w = angles.w;
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

            gameWork.camAngles.w = input_w;
            gameWork.camPos.y = transformed_y;
            gameWork.camPos.z = transformed_z;
            gameWork.camAngles.x = input_x;
            gameWork.camAngles.y = input_y;
            gameWork.camPos.x = transformed_x;
        }
        else
        {
            float transformed_x = transformed[0];
            float transformed_y = transformed[1];
            float transformed_z = transformed[2];
            float input_x = input.x;
            float input_y = input.y;
            float input_w = input.w;

            gameWork.camMoveAngles.w = input_w;
            gameWork.camMovePos.y = transformed_y;
            gameWork.camMovePos.z = transformed_z;
            gameWork.camMoveAngles.x = input_x;
            gameWork.camMoveAngles.y = input_y;
            gameWork.camMovePos.x = transformed_x;
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
        gameWork.camPos.x = posX;
        gameWork.camPos.y = posY;
        gameWork.camPos.z = posZ;
    }
    else
    {
        gameWork.camMovePos.x = posX;
        gameWork.camMovePos.y = posY;
        gameWork.camMovePos.z = posZ;
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
    posX += gameWork.camPos.x;
    posY += gameWork.camPos.y;
    posZ += gameWork.camPos.z;
    if (cameraCamMoveFlag == 0)
    {
        gameWork.camPos.x = posX;
        gameWork.camPos.y = posY;
        gameWork.camPos.z = posZ;
    }
    else
    {
        gameWork.camMovePos.x = posX;
        gameWork.camMovePos.y = posY;
        gameWork.camMovePos.z = posZ;
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
    angles.x += gameWork.camAngles.x;
    angles.y += gameWork.camAngles.y;
    angles.w += gameWork.camAngles.w;
    if (cameraCamMoveFlag == 0)
    {
        gameWork.camAngles.x = angles.x;
        gameWork.camAngles.y = angles.y;
        gameWork.camAngles.w = angles.w;
        cameraOrbitPosition(&gameWork.camPos.x, &gameWork.refPos.x, &angles.x);
    }
    else
    {
        gameWork.camMoveAngles.x = angles.x;
        gameWork.camMoveAngles.y = angles.y;
        gameWork.camMoveAngles.w = angles.w;
        cameraOrbitPosition(&gameWork.camMovePos.x, &gameWork.refMovePos.x, &angles.x);
    }
    cameraCamMoveFlag = 0;
    return 1;
}

extern int D_002B5EE8[];

int CamLock(int value) {
    cameraArgCount = 0;
    if (cameraCheckArgCount(value) != 0) {
        D_002B5EE8[0] = 0;
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

extern f32 cameraRefMoveProgress[];

int WaitMoveRef(int scriptHandle, int *waitFlag) {
    cameraArgCount = 0;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    if (cameraRefMoveProgress[0] < 1.0f) {
        *waitFlag = 1;
    }
    return 1;
}

int WaitMoveCam(int handle, int *flag) {
    cameraArgCount = 0;
    if (cameraCheckArgCount(handle) == 0) {
        return 0;
    }
    if (D_002B5EEC[0] < 1.0f) {
        *flag = 1;
    }
    return 1;
}

int MoveRefStop(int handle) {
    cameraArgCount = 0;
    if (cameraCheckArgCount(handle) == 0) {
        return 0;
    }
    gameWork.refStopWord0 = 0;
    gameWork.refStopWord1 = 0;
    return 1;
}

int MoveCamStop(int handle) {
    cameraArgCount = 0;
    if (cameraCheckArgCount(handle) == 0) {
        return 0;
    }
    gameWork.camStopWord0 = 0;
    gameWork.camStopWord1 = 0;
    return 1;
}

int MovePersStop(int scriptHandle) {
    cameraArgCount = 0;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    gameWork.persWordA = 0;
    *(int *) &gameWork.persFloatB = 0;
    return 1;
}

int MoveBankStop(int scriptHandle) {
    cameraArgCount = 0;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    gameWork.bankWordA = 0;
    gameWork.bankFloatB = 0.0f;
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
    gameWork.shakeMode = shakeMode;
    if (shakeMode & CAMERA_SHAKE_CHANNEL_A)
    {
        gameWork.shakeA.amplitude = amplitude;
        gameWork.shakeA.step = step;
        gameWork.shakeA.paramA = paramA;
        gameWork.shakeA.paramB = 0;
        gameWork.shakeA.paramC = paramC;
        gameWork.shakeA.frames = frames;
    }
    if (shakeMode & CAMERA_SHAKE_CHANNEL_B)
    {
        gameWork.shakeB.amplitude = amplitude;
        gameWork.shakeB.step = step;
        gameWork.shakeB.paramA = paramA;
        gameWork.shakeB.paramC = paramC;
        gameWork.shakeB.frames = frames;
    }
    if (shakeMode & CAMERA_SHAKE_CHANNEL_C)
    {
        gameWork.shakeC.amplitude = amplitude;
        gameWork.shakeC.step = step;
        gameWork.shakeC.paramA = paramA;
        gameWork.shakeC.paramC = paramC;
        gameWork.shakeC.frames = frames;
    }
    return 1;
}

int HandShake(int scriptHandle) {
    cameraArgCount = 0;
    if (cameraCheckArgCount(scriptHandle) == 0) {
        return 0;
    }
    gameWork.handShakeWord2 = 0;
    gameWork.handShakeFlags ^= 8;
    gameWork.handShakeWord3 = 0;
    gameWork.handShakeWord0 = 0;
    gameWork.handShakeWord1 = 0;
    return 1;
}

int cameraSetBank(int handle) {
    cameraArgCount = 1;
    if (cameraCheckArgCount(handle) == 0) {
        return 0;
    }
    cameraBankAngle[0] = cameraCentiDegToRad(func_A9F0C0(handle, 1));
    return 1;
}

int cameraSetPers(int handle) {
    cameraArgCount = 1;
    if (cameraCheckArgCount(handle) == 0) {
        return 0;
    }
    cameraPersAngle[0] = cameraCentiDegToRad(func_A9F0C0(handle, 1));
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
        gameWork.refMovePos.w = cameraCentiDegToRad(angle_raw);
        gameWork.bankByteB = bank_byte;
        gameWork.bankByteA = 0;
        gameWork.bankWordB = 0;
        gameWork.bankWordA = 0;
        gameWork.bankFloatB = I2F(bank_raw);
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
        gameWork.camMovePos.w = cameraCentiDegToRad(angleCenti);
        gameWork.persByteB = persByte;
        gameWork.persByteA = 0;
        gameWork.persWordB = 0;
        gameWork.persWordA = 0;
        gameWork.persFloatB = I2F(persRaw);
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
    refPosY = gameWork.refPos.y;
    refPosZ = gameWork.refPos.z;
    outWords[0] = cameraFloatToCenti(gameWork.refPos.x);
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
    angleY = gameWork.camAngles.y;
    outWords[0] = cameraRadToCentiDeg(gameWork.camAngles.x);
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
    camPosY = gameWork.camPos.y;
    camPosZ = gameWork.camPos.z;
    outWords[0] = cameraFloatToCenti(gameWork.camPos.x);
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
    camAngleY = gameWork.camAngles.y;
    camAngleW = gameWork.camAngles.w;
    outWords[0] = cameraRadToCentiDeg(gameWork.camAngles.x);
    outWords[1] = cameraRadToCentiDeg(camAngleY);
    outWords[2] = cameraFloatToCenti(camAngleW);
    return 1;
}

extern u8 cameraNatives[];
extern u8 cameraModuleName[];

void cameraRegisterNatives(void) {
    func_A9CDB0(cameraModuleName, cameraNatives);
    cameraRefMoveFlag = 0;
    cameraCamMoveFlag = 0;
    cameraMoveMode = 0;
}
