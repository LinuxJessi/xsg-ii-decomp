#include "common.h"
#include "shared.h"
#include "main/tu051.h"
#include "main/tu077.h"

void func_A9CDB0(void *, void *);
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

void cameraFloatToCenti(f32 value) {
    F2I(value * 100.0f);
}

extern f32 cameraRadPerCentiDeg;

f32 cameraCentiDegToRad(int value) {
    return I2F(value) * cameraRadPerCentiDeg;
}

extern f32 cameraRadPerCentiDegDiv;

void cameraRadToCentiDeg(f32 value) {
    F2I(value / cameraRadPerCentiDegDiv);
}

/* Camera work block shared by this unit's camera and reference-action
 * functions. Each Vector4 slot is named after the function family that
 * writes it; the offsets are the original byte offsets.
 * Known but unmodeled here: +0xD0 halfword of flags (cameraPosReset sets
 * 0x20; cameraCfBattleCam edits 0x200, 0x80); +0xD2/+0xD3 bytes written by
 * cameraMode; +0x188..+0x18B flag bytes written by cameraCfLock; three
 * 0x10-byte shake records at +0xF0, +0x110, +0x120 written by cameraShake. */
struct CameraWork {
    /* 0x000 */ unsigned char unmodeled_000[0x144];
    /* 0x144 */ int ref_stop_word0;
    /* 0x148 */ int ref_stop_word1;
    /* 0x14C */ unsigned char unmodeled_14c[0x8];
    /* 0x154 */ int cam_stop_word0;
    /* 0x158 */ int cam_stop_word1;
    /* 0x15C */ unsigned char unmodeled_15c[0x16];
    /* 0x172 */ unsigned char bank_byte_a;
    /* 0x173 */ signed char bank_byte_b;
    /* 0x174 */ int bank_word_a;
    /* 0x178 */ float bank_float_b;
    /* 0x17C */ int bank_word_b;
    /* 0x180 */ int ref_word;
    /* 0x184 */ short ref_short;
    /* 0x186 */ unsigned char unmodeled_186[0x9];
    /* 0x18F */ u8 out_of_range;
    /* 0x190 */ f32 converted;
    /* 0x194 */ unsigned char unmodeled_194[0xC];
    /* 0x1A0 */ Vector4 ref_vec_a;
    /* 0x1B0 */ unsigned char unmodeled_1B0[0x10];
    /* 0x1C0 */ Vector4 cam_vec_a;
    /* 0x1D0 */ Vector4 ref_vec_b;
    /* 0x1E0 */ Vector4 cam_vec_b;
    /* 0x1F0 */ unsigned char unmodeled_1F0[0x30];
    /* 0x220 */ Vector4 cam_vec_c;
    /* 0x230 */ Vector4 ref_vec_c;
    /* 0x240 */ Vector4 cam_vec_d;
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
f32 cameraCentiDegToRad(int value);

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

INCLUDE_ASM("asm/main/nonmatchings/tu077", cameraMode);

int AngleMode(void) {
    return 1;
}

INCLUDE_ASM("asm/main/nonmatchings/tu077", cameraPosReset);

INCLUDE_ASM("asm/main/nonmatchings/tu077", cameraCfSet);

INCLUDE_ASM("asm/main/nonmatchings/tu077", cfChange);

INCLUDE_ASM("asm/main/nonmatchings/tu077", cameraCfRotRange);

int cameraCfCamId(int handle) {
    cameraArgCount = 1;
    if (cameraCheckArgCount(handle) == 0) {
        return 0;
    }
    cameraSetMode(func_A9F0C0(handle, 1), 0, 0);
    return 1;
}

INCLUDE_ASM("asm/main/nonmatchings/tu077", cameraCfLock);

int cameraCfRangeMode(int handle) {
    cameraArgCount = 1;
    if (cameraCheckArgCount(handle) == 0) {
        return 0;
    }
    cameraRangeMode[0] = (u8)func_A9F0C0(handle, 1);
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

INCLUDE_ASM("asm/main/nonmatchings/tu077", cameraCfHokan);

INCLUDE_ASM("asm/main/nonmatchings/tu077", cameraCfBattleCam);

int cfNorth(int handle) {
    int looked_up;
    int out_of_range;

    cameraArgCount = 1;
    if (cameraCheckArgCount(handle) == 0) {
        return 0;
    }
    looked_up = func_A9F0C0(handle, 1);
    out_of_range = (u32)looked_up + 0x8CA0U > 0x11940U;
    gameWork.out_of_range = out_of_range;
    gameWork.converted = cameraCentiDegToRad(looked_up);
    return 1;
}

INCLUDE_ASM("asm/main/nonmatchings/tu077", cameraSetRefPos);

INCLUDE_ASM("asm/main/nonmatchings/tu077", cameraSetRefAct);

/* Record returned by cameraAnchorGet; two floats are added to a position. */
struct CameraOffsetRecord {
    unsigned char unmodeled_00[0x50];
    float offset_x;
    float offset_y;
};

void *cameraApplyAnchorOffset(Vector4 *out, int slot_1, int slot_2, Vector4 *in);
f32 cameraCentiToFloat(int value);
int cameraCheckArgCount(int handle);
void cameraOrbitPosition(float *dst, float *a, float *b);
void *cameraAnchorGet(float *scratch, int slot_1, int slot_2);
extern u8 cameraRefMoveFlag;
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

            gameWork.ref_short = (short) frame_b;
            gameWork.ref_vec_a.w = input_w;
            gameWork.ref_vec_b.x = transformed_x;
            gameWork.ref_vec_b.y = transformed_y;
            gameWork.ref_vec_b.z = transformed_z;
            gameWork.ref_vec_a.x = input_x;
            gameWork.ref_vec_a.y = input_y;
            gameWork.ref_vec_a.z = input_z;
            gameWork.ref_word = frame_a;
        }
        else
        {
            gameWork.ref_vec_c.x = transformed[0];
            gameWork.ref_vec_c.y = transformed[1];
            gameWork.ref_vec_c.z = transformed[2];
        }
        cameraRefMoveFlag = 0;
        ok = 1;
    }
    return ok;
}

INCLUDE_ASM("asm/main/nonmatchings/tu077", cameraSetRefAng);

INCLUDE_ASM("asm/main/nonmatchings/tu077", RefLock);

INCLUDE_ASM("asm/main/nonmatchings/tu077", SetRefCamActR);

INCLUDE_ASM("asm/main/nonmatchings/tu077", SetRefCamAct3R);

INCLUDE_ASM("asm/main/nonmatchings/tu077", cameraSetCamPos);

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
            gameWork.cam_vec_b.w = input.w;
            gameWork.cam_vec_b.x = input.x;
            gameWork.cam_vec_b.y = input.y;
            cameraOrbitPosition(&gameWork.cam_vec_a.x, &gameWork.ref_vec_b.x, &input.x);
        }
        else
        {
            gameWork.cam_vec_d.w = input.w;
            gameWork.cam_vec_d.x = input.x;
            gameWork.cam_vec_d.y = input.y;
            cameraOrbitPosition(&gameWork.cam_vec_c.x, &gameWork.ref_vec_c.x, &input.x);
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
            gameWork.cam_vec_a.x = transformed[0];
            gameWork.cam_vec_a.y = transformed[1];
            gameWork.cam_vec_a.z = transformed[2];
        }
        else
        {
            gameWork.cam_vec_c.x = transformed[0];
            gameWork.cam_vec_c.y = transformed[1];
            gameWork.cam_vec_c.z = transformed[2];
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
            gameWork.cam_vec_a.x = transformed[0];
            gameWork.cam_vec_a.y = transformed[1];
            gameWork.cam_vec_a.z = transformed[2];
        }
        else
        {
            gameWork.cam_vec_c.x = transformed[0];
            gameWork.cam_vec_c.y = transformed[1];
            gameWork.cam_vec_c.z = transformed[2];
        }
        cameraCamMoveFlag = 0;
        ok = 1;
    }
    return ok;
}

INCLUDE_ASM("asm/main/nonmatchings/tu077", SetCamAct2);

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
        input.x += offset_record->offset_x;
        input.y += offset_record->offset_y;
        cameraOrbitPosition(transformed, scratch, &input.x);
        if (cameraCamMoveFlag == 0)
        {
            float transformed_x = transformed[0];
            float transformed_y = transformed[1];
            float transformed_z = transformed[2];
            float input_x = input.x;
            float input_y = input.y;
            float input_w = input.w;

            gameWork.cam_vec_b.w = input_w;
            gameWork.cam_vec_a.y = transformed_y;
            gameWork.cam_vec_a.z = transformed_z;
            gameWork.cam_vec_b.x = input_x;
            gameWork.cam_vec_b.y = input_y;
            gameWork.cam_vec_a.x = transformed_x;
        }
        else
        {
            float transformed_x = transformed[0];
            float transformed_y = transformed[1];
            float transformed_z = transformed[2];
            float input_x = input.x;
            float input_y = input.y;
            float input_w = input.w;

            gameWork.cam_vec_d.w = input_w;
            gameWork.cam_vec_c.y = transformed_y;
            gameWork.cam_vec_c.z = transformed_z;
            gameWork.cam_vec_d.x = input_x;
            gameWork.cam_vec_d.y = input_y;
            gameWork.cam_vec_c.x = transformed_x;
        }
        cameraCamMoveFlag = 0;
        ok = 1;
    }
    return ok;
}

INCLUDE_ASM("asm/main/nonmatchings/tu077", SetCamAct3);

INCLUDE_ASM("asm/main/nonmatchings/tu077", OffsCamPos);

INCLUDE_ASM("asm/main/nonmatchings/tu077", OffsCamAng);

extern int D_002B5EE8[];

int CamLock(int value) {
    cameraArgCount = 0;
    if (cameraCheckArgCount(value) != 0) {
        D_002B5EE8[0] = 0;
        return 1;
    }
    return 0;
}

INCLUDE_ASM("asm/main/nonmatchings/tu077", cameraMoveRefMode);

INCLUDE_ASM("asm/main/nonmatchings/tu077", MoveCamMode);

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

INCLUDE_ASM("asm/main/nonmatchings/tu077", WaitMoveRef);

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
    gameWork.ref_stop_word0 = 0;
    gameWork.ref_stop_word1 = 0;
    return 1;
}

int MoveCamStop(int handle) {
    cameraArgCount = 0;
    if (cameraCheckArgCount(handle) == 0) {
        return 0;
    }
    gameWork.cam_stop_word0 = 0;
    gameWork.cam_stop_word1 = 0;
    return 1;
}

INCLUDE_ASM("asm/main/nonmatchings/tu077", MovePersStop);

INCLUDE_ASM("asm/main/nonmatchings/tu077", MoveBankStop);

INCLUDE_ASM("asm/main/nonmatchings/tu077", cameraShake);

INCLUDE_ASM("asm/main/nonmatchings/tu077", HandShake);

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
        gameWork.ref_vec_c.w = cameraCentiDegToRad(angle_raw);
        gameWork.bank_byte_b = (signed char) bank_byte;
        gameWork.bank_byte_a = 0;
        gameWork.bank_word_b = 0;
        gameWork.bank_word_a = 0;
        gameWork.bank_float_b = I2F(bank_raw);
        ok = 1;
    }
    return ok;
}

INCLUDE_ASM("asm/main/nonmatchings/tu077", MovePers);

INCLUDE_ASM("asm/main/nonmatchings/tu077", cameraGetRefPos);

INCLUDE_ASM("asm/main/nonmatchings/tu077", GetRefAng);

INCLUDE_ASM("asm/main/nonmatchings/tu077", GetCamPos);

INCLUDE_ASM("asm/main/nonmatchings/tu077", GetCamAng);

extern u8 cameraNatives[];
extern u8 cameraModuleName[];

void cameraRegisterNatives(void) {
    func_A9CDB0(cameraModuleName, cameraNatives);
    cameraRefMoveFlag = 0;
    cameraCamMoveFlag = 0;
    cameraMoveMode = 0;
}
