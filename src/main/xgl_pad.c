#include "common.h"

/*
 * Controller record, 0x80 bytes per port (0x68 in XS1). The fields up to
 * +0x68 keep the XS1 layout: xglPadInitial clears 0x80 bytes per port and
 * writes button_map at +0x58 and axis_dead_zone at +0x60; xglPadSetRepeat
 * writes +0x48 to +0x4d. The pressed halfword at +0x2a is also read by
 * xgl_hdd.c, the direction halfwords at +0x32/+0x34 by xgl_menu.c.
 */
typedef struct XglPadRecord {
    unsigned char packet[0x20];
    unsigned char mode;
    unsigned char actuator_count;
    short mode_id;
    unsigned char unmodeled_24[2];
    unsigned char connection_state;
    unsigned char setup_state;
    unsigned short buttons;
    unsigned short pressed;
    unsigned short repeated;
    unsigned short released;
    unsigned short directions;
    unsigned short direction_pressed;
    unsigned short direction_repeated;
    unsigned short direction_released;
    signed char position[4];
    short horizontal_pressure;
    short vertical_pressure;
    signed char pressure[8];
    signed char repeat_delay;           /* +0x48 */
    signed char repeat_interval;        /* +0x49 */
    unsigned short repeat_mask;         /* +0x4a */
    signed char repeat_wait;            /* +0x4c */
    signed char repeat_count;           /* +0x4d */
    unsigned char horizontal_dead_zone; /* +0x4e */
    unsigned char vertical_dead_zone;   /* +0x4f */
    unsigned char actuator[6];          /* +0x50 */
    unsigned char actuator_pending;     /* +0x56 */
    unsigned char unmodeled_57;         /* +0x57 */
    unsigned char button_map[8];        /* +0x58 */
    unsigned char axis_dead_zone[4];    /* +0x60 */
    signed char axis[4];                /* +0x64 */
    unsigned char unmodeled_68[0x18];
} XglPadRecord;

#define PAD_DMA_BUFFER_SIZE 0x100
/* xglPadInitial's defaults: auto-repeat on the four directions and L1/R1
 * (after 8 frames, then every frame), and the stick dead zones. */
#define PAD_REPEAT_MASK 0xf00c
#define PAD_REPEAT_DELAY 8
#define PAD_REPEAT_INTERVAL 1
#define PAD_DEFAULT_DEAD_ZONE 64
#define PAD_AXIS_THRESHOLD 32

extern XglPadRecord PadData[2];
extern unsigned char PadDmaBuffer[2 * PAD_DMA_BUFFER_SIZE];

void xglPadSetRepeat(int port, unsigned short mask, int delay, int interval);
int scePadInit(int mode);
int scePadPortOpen(int port, int slot, unsigned char *dma_buffer);

/* Returns at once with no effect. Only caller: InitializeSystem. */
void func_00199428(void) {
}

void xglPadInitial(void) {
    unsigned int byte_index;
    int map_remaining;
    int dead_zone_remaining;
    unsigned char axis_threshold;
    int button;
    unsigned char *first_record_bytes;
    unsigned char *second_record_bytes;

    scePadInit(0);
    scePadPortOpen(0, 0, PadDmaBuffer);
    scePadPortOpen(1, 0, &PadDmaBuffer[PAD_DMA_BUFFER_SIZE]);

    first_record_bytes = (unsigned char *)&PadData[0];
    second_record_bytes = (unsigned char *)&PadData[1];
    /* Keep the original paired byte walk across the two 0x80-byte records. */
    byte_index = 0;
    do {
        *first_record_bytes = 0;
        *second_record_bytes = 0;
        ++first_record_bytes;
        ++second_record_bytes;
        ++byte_index;
    } while (byte_index < sizeof(XglPadRecord));

    xglPadSetRepeat(0, PAD_REPEAT_MASK, PAD_REPEAT_DELAY, PAD_REPEAT_INTERVAL);
    xglPadSetRepeat(1, PAD_REPEAT_MASK, PAD_REPEAT_DELAY, PAD_REPEAT_INTERVAL);

    button = 1;
    map_remaining = sizeof(PadData[0].button_map) - 1;
    do {
        PadData[0].button_map[sizeof(PadData[0].button_map) - 1 - map_remaining] = button;
        PadData[1].button_map[sizeof(PadData[1].button_map) - 1 - map_remaining] = button;
        button <<= 1;
        --map_remaining;
    } while (map_remaining >= 0);

    PadData[0].horizontal_dead_zone = PAD_DEFAULT_DEAD_ZONE;
    PadData[0].vertical_dead_zone = PAD_DEFAULT_DEAD_ZONE;
    PadData[1].horizontal_dead_zone = PAD_DEFAULT_DEAD_ZONE;
    PadData[1].vertical_dead_zone = PAD_DEFAULT_DEAD_ZONE;
    axis_threshold = PAD_AXIS_THRESHOLD;
    dead_zone_remaining = sizeof(PadData[0].axis_dead_zone) - 1;
    do {
        PadData[0].axis_dead_zone[sizeof(PadData[0].axis_dead_zone) - 1 - dead_zone_remaining] = axis_threshold;
        PadData[1].axis_dead_zone[sizeof(PadData[1].axis_dead_zone) - 1 - dead_zone_remaining] = axis_threshold;
        --dead_zone_remaining;
    } while (dead_zone_remaining >= 0);
}

/*
 * Auto-repeat setup of one port (-1: both): delay, interval and button mask,
 * with the repeat counters cleared.
 */
void xglPadSetRepeat(int port, unsigned short mask, int delay, int interval)
{
    unsigned short button_mask = mask;
    XglPadRecord *pad;

    if (port == -1) {
        xglPadSetRepeat(0, button_mask, delay, interval);
        xglPadSetRepeat(1, button_mask, delay, interval);
        return;
    }

    pad = &PadData[port];
    pad->repeat_delay = delay;
    pad->repeat_interval = interval;
    pad->repeat_mask = button_mask;
    pad->repeat_wait = 0;
    pad->repeat_count = 0;
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_pad", xglPadRead);
