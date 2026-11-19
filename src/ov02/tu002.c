#include "common.h"
#include "shared.h"
#include "main/control_entry.h"
#include "main/xgl_font.h"
#include "main/sdv_033.h"
#include "main/xgl_jpeg.h"

/* The install screens: the command TitleHddInstall takes (the title passes it for the state of the hard disk). */
#define HDD_SCREEN_INSTALL 0
#define HDD_SCREEN_UNINSTALL 1
#define HDD_SCREEN_MESSAGE_FIRST 2
#define HDD_SCREEN_MESSAGE_LAST 4
#define HDD_SCREEN_REACTIVATE 5
/* Frames the screens ignore the pad after they appear. */
#define HDD_INPUT_DELAY 4
/* Position of the screen message and of the installer's status text. */
#define HDD_MESSAGE_X 32
#define HDD_MESSAGE_Y 16
#define HDD_STATUS_X 84
#define HDD_STATUS_Y 208
/* Progress gauge box (tyaDrawGauge) and the texts drawn over it while an install copies its images. */
#define HDD_GAUGE_X 168
#define HDD_GAUGE_Y 416
#define HDD_GAUGE_WIDTH 320
#define HDD_GAUGE_HEIGHT 12
#define HDD_HINT_X 32
#define HDD_HINT_Y 80
#define HDD_COUNTER_X 152
#define HDD_COUNTER_Y 414
#define HDD_PERCENT_X 168
#define HDD_PERCENT_Y 400
/* The install archive: a 0x40-byte header, then one jpeg for every fifth of the progress. */
#define HDD_ARCHIVE_HEADER_SIZE 0x40
#define HDD_PROGRESS_IMAGES 5
#define HDD_PROGRESS_PERCENT_PER_IMAGE 20
/* Failures xglHddInstall reports to its callback as a negative command (the values of src/main/xgl_hdd.c). */
#define HDD_INSTALL_FAIL_RESOURCE (-1)
#define HDD_INSTALL_FAIL_PARTITION_OPEN (-2)
#define HDD_INSTALL_FAIL_WRITE_HEADER (-3)
#define HDD_INSTALL_FAIL_EXTEND (-4)
#define HDD_INSTALL_FAIL_PARTITION_CLOSE (-5)
#define HDD_INSTALL_FAIL_FORMAT (-6)
#define HDD_INSTALL_FAIL_MOUNT (-7)
#define HDD_INSTALL_FAIL_DATA_OPEN (-8)
#define HDD_INSTALL_FAIL_DATA_WRITE (-9)
#define HDD_INSTALL_FAIL_DATA_CLOSE (-10)
#define HDD_INSTALL_FAIL_CHECK_OPEN (-11)
#define HDD_INSTALL_FAIL_CHECK_WRITE (-12)
#define HDD_INSTALL_FAIL_CHECK_CLOSE (-13)
#define HDD_INSTALL_FAIL_UNMOUNT (-14)
#define HDD_INSTALL_FAIL_REMOUNT (-15)
/* Sound ids: CIRCLE plays the decide sound, CROSS the cancel sound. */
#define HDD_SE_DECIDE 0x1000002
#define HDD_SE_CANCEL 0x1000008

extern void sceVif1PkRef(XglPacket *packet, const void *environment,
                         int count, int mode, int offset, int flags);
extern void sceVif1PkCnt(XglPacket *packet, int count);
extern XglPacket *xglPacketGetCurrent(void);

void sceVif1PkAddDataN(XglPacket *packet, const void *data, int count);

/* The decoded jpeg is sent in two halves of HDD_TEXTURE_QWC quadwords. */
#define HDD_TEXTURE_QWC 0x7000
#define HDD_TEXTURE_HALF_SIZE (HDD_TEXTURE_QWC * 16)
/* sceVif1PkRef offset argument for each texture half (value of the original build). */
#define HDD_TEXTURE_VIF_OFFSET 0x51007000
/* Free-memory pointer is set this many bytes past the transfer buffer (the install's working area). */
#define HDD_WORK_AFTER_TRANSFER 0xE0000

struct HddInstallParams {
    u8 *image_data;
    u8 step;
    u8 unmodeled_05[3];
    u8 *transfer_buffer;
};

struct HddInstallRenderState {
    u8 unmodeled_00[0x20];
    u16 buffer_select;
};

extern struct HddInstallRenderState sRender;
/* The installer keeps one transfer-buffer pointer in its own BSS. */
extern struct HddInstallParams *HddInstallParam;
/* DMA/VIF tags followed by the GS register data sent by HddInstallSync. */
extern u64 TestEnv_0_00A830A0[12];
extern u64 TransEnv_1_00A83100[4];
extern u64 FlushEnv_2_00A83120[6];
/* Font control bytes passed to the existing font-script renderer. */
extern const char installScreenFontText[8];

void HddInstallSync(void)
{
    XglPacket *packet;
    u8 *transfer_buffer;

    packet = xglPacketGetCurrent();
    transfer_buffer = HddInstallParam->transfer_buffer;
    TestEnv_0_00A830A0[4] = ((u64)sRender.buffer_select << 37) | ((u64)0x8000 << 36);
    sceVif1PkCnt(packet, 0);
    sceVif1PkAddDataN(packet, TestEnv_0_00A830A0, 0x18);
    sceVif1PkRef(packet, TransEnv_1_00A83100, 2, 0, 0, 0);
    sceVif1PkRef(packet, transfer_buffer, HDD_TEXTURE_QWC, 0, HDD_TEXTURE_VIF_OFFSET, 0);
    sceVif1PkRef(packet, TransEnv_1_00A83100, 2, 0, 0, 0);
    sceVif1PkRef(packet, transfer_buffer + HDD_TEXTURE_HALF_SIZE, HDD_TEXTURE_QWC, 0, HDD_TEXTURE_VIF_OFFSET, 0);
    sceVif1PkRef(packet, FlushEnv_2_00A83120, 3, 0, 0, 0);
    xglSleep();
    xglFontPrintDirectOT(-1, installScreenFontText);
}

#include "main/pad.h"

/* Pad ports (main/pad.h PadRecord) of the main data; port 0 is the one the installer polls. */
extern PadRecord PadData[2];
/* Prompt strings of the install and uninstall dialogs (splat D_ names). */
extern const char hddInstallStartPromptText[];
extern const char hddInstallDoneText[];
extern const char hddInstallFailedPromptText[];
extern const char hddUninstallStartPromptText[];
extern const char hddUninstallDoneText[];
extern const char hddUninstallFailedPromptText[];
void xglFontPrint(int x, int y, unsigned int color, const char *text);
void seVoiceStartDefault(int id);
int xglHddCheck(void);
int xglHddCheck2(void);
int xglHddInstall(int (*callback)(int, int, int *), int *state);
int xglHddUninstall(void);
int xglHddActivate(int mode);

/* Progress gauge drawn by tyaDrawGauge: value and max are the two halves of the progress word. */
struct HddGauge {
    s16 x;
    s16 y;
    int vertex_attribute;
    s16 width;
    s16 height;
    s16 current;
    s16 maximum;
    u8 colors[8];
};


extern u8 D_00A83E20;
extern const signed char D_00A83150[4];
extern const char D_00A83158[];
extern const char *const D_00A83160[];
extern const char D_00A83A30[];
extern const char hddNotAccessibleText[];
extern const char hddInstallAbortedText[];
extern const char hddUninstallAbortedText[];
/* WorkEnd is main/tu011's global (src/main/xgl_cd.c): the free-memory pointer
 * the installer reads its CD file into and restores when it returns. */
extern u8 *WorkEnd;
extern const char hddInstallingText[];
extern const char hddAbortHintText[];
extern const char hddAbortConfirmText[];
extern const char D_00A83728[];
extern const char D_00A83738[];
extern const char hddStepInstallStartText[];
extern const char hddStepCreatePartitionText[];
extern const char hddStepWriteResourceText[];
extern const char hddStepCreateSubPartitionText[];
extern const char hddStepFormatPartitionText[];
extern const char hddStepMountPartitionText[];
extern const char hddStepStartInstallText[];
extern const char D_00A836B0[];
extern const char D_00A836C8[];
extern const char D_00A836D8[];
extern const char D_00A836E8[];
extern const char D_00A83700[];
extern const char D_00A83710[];
void tyaDrawGauge(struct HddGauge *gauge);
void xglFontPrintf(int x, int y, unsigned int color, void *args);
void xglFontDebugPrintf(int x, int y, const char *fmt, ...);
int printf(const char *fmt, ...);
long long func_002A5C88(long long percent, int count);

/* Installer status callback: prints the step text for `event`, draws the
 * progress gauge for event 6, then waits on the pad once the install is done. */
int HddInstallCB(int command, int offset, int *status)
{
    struct HddGauge gauge;
    u64 print_args[2];
    int progress_max;
    int step;

    if (*status) {
        HddInstallSync();
        return *status;
    }

    xglFontPrint(HDD_MESSAGE_X, HDD_MESSAGE_Y, -1, hddInstallingText);
    switch (command) {
    case 0:
        xglFontDebugPrintf(HDD_STATUS_X, HDD_STATUS_Y, hddStepInstallStartText);
        break;
    case 1:
        xglFontDebugPrintf(HDD_STATUS_X, HDD_STATUS_Y, hddStepCreatePartitionText);
        break;
    case 2:
        xglFontDebugPrintf(HDD_STATUS_X, HDD_STATUS_Y, hddStepWriteResourceText);
        break;
    case 3:
        xglFontDebugPrintf(HDD_STATUS_X, HDD_STATUS_Y, hddStepCreateSubPartitionText, offset);
        break;
    case 4:
        xglFontDebugPrintf(HDD_STATUS_X, HDD_STATUS_Y, hddStepFormatPartitionText);
        break;
    case 5:
        xglFontDebugPrintf(HDD_STATUS_X, HDD_STATUS_Y, hddStepMountPartitionText);
        break;
    case 6:
        if (offset == 0) {
            xglFontDebugPrintf(HDD_STATUS_X, HDD_STATUS_Y, hddStepStartInstallText);
            break;
        }
        gauge.x = HDD_GAUGE_X;
        gauge.y = HDD_GAUGE_Y;
        gauge.vertex_attribute = -1;
        gauge.width = HDD_GAUGE_WIDTH;
        gauge.current = offset;
        gauge.colors[0] = 0x80;
        gauge.colors[2] = 0x80;
        gauge.colors[3] = 0x80;
        gauge.colors[4] = 0x40;
        gauge.colors[5] = 0x40;
        gauge.colors[6] = 0x40;
        gauge.colors[7] = 0x80;
        gauge.height = HDD_GAUGE_HEIGHT;
        gauge.colors[1] = 0xc0;
        progress_max = offset >> 16;
        gauge.maximum = progress_max;
        tyaDrawGauge(&gauge);
        xglFontPrint(HDD_HINT_X, HDD_HINT_Y, -1, hddAbortHintText);
        print_args[0] = (u32)D_00A83158;
        print_args[1] = (u32)D_00A83150[(D_00A83E20 >> 3) & 3];
        xglFontPrintf(HDD_COUNTER_X, HDD_COUNTER_Y, -1, print_args);
        D_00A83E20 += 1;
        print_args[0] = (u32)D_00A83160;
        print_args[1] = (u16)offset * 100 / progress_max;
        xglFontPrintf(HDD_PERCENT_X, HDD_PERCENT_Y, -1, print_args);
        step = func_002A5C88(print_args[1], HDD_PROGRESS_PERCENT_PER_IMAGE);
        if (step != HddInstallParam->step && step < HDD_PROGRESS_IMAGES) {
            JpegDecodeControl jpeg;
            u8 *image_data;

            image_data = HddInstallParam->image_data;
            memset(&jpeg, 0, sizeof(jpeg));
            HddInstallParam->step = step;
            jpeg.source = image_data + ((u32 *)image_data)[step] + HDD_ARCHIVE_HEADER_SIZE;
            jpeg.destination = HddInstallParam->transfer_buffer;
            xglJpegDecode(&jpeg);
        }
        if ((PadData[0].pressed & PAD_CROSS) == 0) {
            break;
        }
        seVoiceStartDefault(HDD_SE_CANCEL);
        HddInstallSync();
        while (1) {
            u16 pressed;

            xglFontPrint(HDD_MESSAGE_X, HDD_MESSAGE_Y, -1, hddAbortConfirmText);
            pressed = PadData[0].pressed;
            if ((pressed & PAD_CIRCLE) != 0) {
                seVoiceStartDefault(HDD_SE_DECIDE);
                *status = 1;
                return 1;
            }
            if ((pressed & PAD_CROSS) != 0) {
                seVoiceStartDefault(HDD_SE_CANCEL);
                if (xglHddCheck() >= 0) {
                    HddInstallSync();
                    return 0;
                }
                *status = 2;
                return 2;
            }
            HddInstallSync();
        }
    case 7:
        switch (offset) {
        case 0:
            xglFontDebugPrintf(HDD_STATUS_X, HDD_STATUS_Y, D_00A836B0);
            break;
        case 1:
            xglFontDebugPrintf(HDD_STATUS_X, HDD_STATUS_Y, D_00A836C8);
            break;
        case 2:
            xglFontDebugPrintf(HDD_STATUS_X, HDD_STATUS_Y, D_00A836D8);
            break;
        case 3:
            xglFontDebugPrintf(HDD_STATUS_X, HDD_STATUS_Y, D_00A836E8);
            break;
        case 4:
            xglFontDebugPrintf(HDD_STATUS_X, HDD_STATUS_Y, D_00A83700);
            break;
        }
        break;
    case 8:
        xglFontDebugPrintf(HDD_STATUS_X, HDD_STATUS_Y, D_00A83710);
        break;
    case HDD_INSTALL_FAIL_RESOURCE:
    case HDD_INSTALL_FAIL_PARTITION_OPEN:
    case HDD_INSTALL_FAIL_WRITE_HEADER:
    case HDD_INSTALL_FAIL_EXTEND:
    case HDD_INSTALL_FAIL_PARTITION_CLOSE:
    case HDD_INSTALL_FAIL_FORMAT:
    case HDD_INSTALL_FAIL_MOUNT:
    case HDD_INSTALL_FAIL_DATA_OPEN:
    case HDD_INSTALL_FAIL_DATA_WRITE:
    case HDD_INSTALL_FAIL_DATA_CLOSE:
    case HDD_INSTALL_FAIL_CHECK_OPEN:
    case HDD_INSTALL_FAIL_CHECK_WRITE:
    case HDD_INSTALL_FAIL_CHECK_CLOSE:
    case HDD_INSTALL_FAIL_UNMOUNT:
    case HDD_INSTALL_FAIL_REMOUNT:
        printf(D_00A83728, command, offset, offset);
        xglFontDebugPrintf(HDD_STATUS_X, HDD_STATUS_Y, D_00A83738, command, offset);
        break;
    }
    HddInstallSync();
    return 0;
}

/* TitleHddInstall install arm: start prompt, then the install and its completion or failure prompt. */
void TitleHddInstallSubInstall(void)
{
    int installStatus;
    int result;
    int frames;
    u16 pad;

    for (;;) {
        installStatus = 0;
        frames = HDD_INPUT_DELAY;
        for (;;) {
            xglFontPrint(HDD_MESSAGE_X, HDD_MESSAGE_Y, -1, hddInstallStartPromptText);
            HddInstallSync();
            if (frames > 0) {
                frames--;
                continue;
            }
            pad = PadData[0].pressed;
            if (pad & PAD_CIRCLE) {
                break;
            }
            if (pad & PAD_CROSS) {
                seVoiceStartDefault(HDD_SE_CANCEL);
                return;
            }
        }
        seVoiceStartDefault(HDD_SE_DECIDE);
        result = xglHddCheck();
        if (result == 0) {
            result = xglHddInstall(HddInstallCB, &installStatus);
        }
        if (result == 1) {
            HddInstallSync();
            return;
        }
        if (result == 0) {
            xglHddActivate(1);
            result = xglHddActivate(-1);
        } else {
            result = 0;
        }
        HddInstallSync();
        frames = HDD_INPUT_DELAY;
        if (result == 1) {
            for (;;) {
                xglFontPrint(HDD_MESSAGE_X, HDD_MESSAGE_Y, -1, hddInstallDoneText);
                HddInstallSync();
                if (frames > 0) {
                    frames--;
                    continue;
                }
                pad = PadData[0].pressed;
                if (pad & PAD_CIRCLE) {
                    seVoiceStartDefault(HDD_SE_DECIDE);
                    return;
                }
            }
        }
        for (;;) {
            xglFontPrint(HDD_MESSAGE_X, HDD_MESSAGE_Y, -1, hddInstallFailedPromptText);
            HddInstallSync();
            if (frames > 0) {
                frames--;
                continue;
            }
            pad = PadData[0].pressed;
            if (pad & PAD_CIRCLE) {
                seVoiceStartDefault(HDD_SE_DECIDE);
                return;
            }
            if (pad & PAD_CROSS) {
                break;
            }
        }
        seVoiceStartDefault(HDD_SE_CANCEL);
        HddInstallSync();
    }
}

/* TitleHddInstall uninstall arm: same prompt flow as the install arm, without the install callback. */
void TitleHddInstallSubUninstall(void)
{
    int result;
    int frames;
    u16 pad;

    for (;;) {
        frames = HDD_INPUT_DELAY;
        for (;;) {
            xglFontPrint(HDD_MESSAGE_X, HDD_MESSAGE_Y, -1, hddUninstallStartPromptText);
            HddInstallSync();
            if (frames > 0) {
                frames--;
                continue;
            }
            pad = PadData[0].pressed;
            if (pad & PAD_CIRCLE) {
                break;
            }
            if (pad & PAD_CROSS) {
                seVoiceStartDefault(HDD_SE_CANCEL);
                return;
            }
        }
        seVoiceStartDefault(HDD_SE_DECIDE);
        result = xglHddCheck2();
        if (result == 0) {
            result = xglHddUninstall();
        }
        frames = HDD_INPUT_DELAY;
        HddInstallSync();
        if (result == 0) {
            for (;;) {
                xglFontPrint(HDD_MESSAGE_X, HDD_MESSAGE_Y, -1, hddUninstallDoneText);
                HddInstallSync();
                if (frames > 0) {
                    frames--;
                    continue;
                }
                pad = PadData[0].pressed;
                if (pad & PAD_CIRCLE) {
                    seVoiceStartDefault(HDD_SE_DECIDE);
                    return;
                }
            }
        }
        for (;;) {
            xglFontPrint(HDD_MESSAGE_X, HDD_MESSAGE_Y, -1, hddUninstallFailedPromptText);
            HddInstallSync();
            if (frames > 0) {
                frames--;
                continue;
            }
            pad = PadData[0].pressed;
            if (pad & PAD_CIRCLE) {
                seVoiceStartDefault(HDD_SE_DECIDE);
                xglHddCheck();
                return;
            }
            if (pad & PAD_CROSS) {
                break;
            }
        }
        seVoiceStartDefault(HDD_SE_CANCEL);
        HddInstallSync();
    }
}

/* HDD install dispatcher called from Title: shows the install or uninstall arm, or the message screens. */
void TitleHddInstall(int command)
{
    struct HddInstallParams params;
    JpegDecodeControl request;
    u8 *saved_work_end;
    u8 *buffer;
    u8 *transfer_buffer;
    int size;

    saved_work_end = WorkEnd;
    if (command == HDD_SCREEN_INSTALL) {
        if (xglHddCheck()) {
            command = HDD_SCREEN_MESSAGE_FIRST;
        }
    } else if (command == HDD_SCREEN_UNINSTALL) {
        if (xglHddCheck2()) {
            xglHddActivate(0);
            command = HDD_SCREEN_REACTIVATE;
        }
    }

    buffer = WorkEnd;
    size = xglCdReadFile(D_00A83A30, buffer, 0, 1);
    transfer_buffer = (u8 *)(((u32)buffer + size + 0x3f) & ~0x3f);
    WorkEnd = &transfer_buffer[HDD_WORK_AFTER_TRANSFER];

    params.image_data = buffer;
    params.step = 0;
    params.transfer_buffer = transfer_buffer;
    memset(&request, 0, sizeof(request));
    request.source = &buffer[HDD_ARCHIVE_HEADER_SIZE];
    request.destination = transfer_buffer;
    xglJpegDecode(&request);
    HddInstallParam = &params;
    HddInstallSync();
    xglFontPrintDirectOT(-1, installScreenFontText);

    switch (command) {
    case HDD_SCREEN_INSTALL:
        TitleHddInstallSubInstall();
        break;
    case HDD_SCREEN_UNINSTALL:
        TitleHddInstallSubUninstall();
        break;
    case HDD_SCREEN_MESSAGE_LAST:
    case HDD_SCREEN_MESSAGE_FIRST + 1:
    case HDD_SCREEN_MESSAGE_FIRST:
        {
            PadRecord *pad = &PadData[0];
            int frames = HDD_INPUT_DELAY;

            while (1) {
                xglFontPrint(HDD_MESSAGE_X, HDD_MESSAGE_Y, -1, D_00A83160[command]);
                xglFontPrintDirectOT(-1, hddInstallAbortedText);
                HddInstallSync();
                if (frames > 0) {
                    frames--;
                    continue;
                }
                if ((pad->pressed & PAD_CIRCLE) != 0) {
                    break;
                }
            }
            seVoiceStartDefault(HDD_SE_DECIDE);
        }
        break;
    case HDD_SCREEN_REACTIVATE:
        {
            PadRecord *pad = &PadData[0];
            int frames = HDD_INPUT_DELAY;

            while (1) {
                xglFontPrint(HDD_MESSAGE_X, HDD_MESSAGE_Y, -1, hddNotAccessibleText);
                xglFontPrintDirectOT(-1, hddUninstallAbortedText);
                HddInstallSync();
                if (frames > 0) {
                    frames--;
                    continue;
                }
                if ((pad->pressed & PAD_CIRCLE) != 0) {
                    break;
                }
            }
            seVoiceStartDefault(HDD_SE_DECIDE);
            xglHddActivate(0x103);
            xglHddCheck();
        }
        break;
    }

    HddInstallSync();
    WorkEnd = saved_work_end;
}
