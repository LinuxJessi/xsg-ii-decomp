#include "common.h"
#include "shared.h"

extern void *memset(void *destination, int value, unsigned int count);

void GameSnapShotCheck(void);

typedef unsigned int XglQuadword __attribute__((mode(TI)));

unsigned int GameSnapShotSaveThumbnail(int quality, unsigned int output_buffer);

int GameSnapShotNumber(int number);

typedef union SnapshotDmaPacket {
    u64 words[24];
    XglQuadword quadwords[12];
} SnapshotDmaPacket;

void xglDmaDirectNormal(u32 channel, u32 address, u32 count);

enum {
    SNAPSHOT_OPEN_FLAGS = 0x602,
    SNAPSHOT_CACHE_LINE_SIZE = 64,
    SNAPSHOT_WIDTH = 512,
    SNAPSHOT_HEIGHT = 448,
    SNAPSHOT_METADATA_SIZE = 160,
    SNAPSHOT_CHANNEL_LUMA = 0,
    SNAPSHOT_CHANNEL_CB = 1,
    SNAPSHOT_CHANNEL_CR = 2,
    SNAPSHOT_THUMBNAIL_WIDTH = 128,
    SNAPSHOT_THUMBNAIL_HEIGHT = 112,
    SNAPSHOT_THUMBNAIL_SOURCE_X = 0x3800,
    SNAPSHOT_THUMBNAIL_SOURCE_Y = 8,
    SNAPSHOT_THUMBNAIL_QUALITY = 50,
    SNAPSHOT_SEGMENT_OVERHEAD = 4,
    SNAPSHOT_MARKER_PREFIX = 0xFF,
    SNAPSHOT_MARKER_SOI = 0xD8,
    SNAPSHOT_MARKER_APP14 = 0xEE,
};

typedef struct {
    unsigned char quality;
    unsigned char unmodeled_01[3];
} GameSnapShotQualityChannel;

typedef struct {
    GameSnapShotQualityChannel channel[3];
    unsigned char unmodeled_0c[4];
} GameSnapShotQuality;

typedef struct {
    unsigned char luma_quality;
    unsigned char cb_quality;
    unsigned char cr_quality;
    unsigned char unmodeled_03[5];
} GameSnapShotQualitySettings;

typedef struct {
    unsigned int unmodeled_00;
    unsigned int output_buffer;
    unsigned int output_size;
    short source_x;
    short source_y;
    short destination_x;
    short destination_y;
    short width;
    short height;
    GameSnapShotQualitySettings quality;
} GameSnapShotJpegParams;

typedef struct {
    unsigned char unmodeled_00[0x22];
    unsigned short draw_buffer_base;
} GameSnapShotRenderState;

extern GameSnapShotRenderState sRender;
extern int xglJpegEncode(GameSnapShotJpegParams *params);
extern void *memset(void *destination, int value, unsigned int size);
extern void SyncDCache(void *start, void *end);
extern int sceGsSyncPath(int mode, int timeout);

/* Points at the slot digit inside the snapshot file name (snapname + 0x1F). */
extern char *snapnameno;

extern GameSnapShotQuality quality_table[5];
extern SnapshotDmaPacket TestPrim_0_002BC220;

int GameSnapShotNumber(int number) {
    if (number >= 0) {
        *snapnameno = number + '0';
    }
    return *snapnameno - '0';
}

void GameSnapShotCheck(void) {
}

INCLUDE_ASM("asm/main/nonmatchings/game_snap_shot", func_001E4D20);

INCLUDE_ASM("asm/main/nonmatchings/game_snap_shot", func_001E4E60);

unsigned int GameSnapShotSaveThumbnail(int quality, unsigned int output_buffer) {
    GameSnapShotJpegParams params;
    unsigned long long bitbltbuf;

    bitbltbuf = 0x24020000 | (sRender.draw_buffer_base << 5);
    TestPrim_0_002BC220.words[10] = bitbltbuf | (0xC800ULL << 19);
    TestPrim_0_002BC220.words[8] = 0x801C0;
    SyncDCache(TestPrim_0_002BC220.words, TestPrim_0_002BC220.words + 24);
    sceGsSyncPath(0, 0);
    xglDmaDirectNormal(2, (u32)TestPrim_0_002BC220.words, 12);
    sceGsSyncPath(0, 0);

    memset(&params, 0, sizeof(params));
    params.output_buffer = output_buffer;
    params.source_x = SNAPSHOT_THUMBNAIL_SOURCE_X;
    params.source_y = SNAPSHOT_THUMBNAIL_SOURCE_Y;
    params.width = SNAPSHOT_THUMBNAIL_WIDTH;
    params.height = SNAPSHOT_THUMBNAIL_HEIGHT;
    params.destination_x = 0;
    params.destination_y = 0;
    if (quality < 0) {
        params.quality.luma_quality = quality_table[~quality].channel[SNAPSHOT_CHANNEL_LUMA].quality;
        params.quality.cb_quality = quality_table[~quality].channel[SNAPSHOT_CHANNEL_CB].quality;
        params.quality.cr_quality = quality_table[~quality].channel[SNAPSHOT_CHANNEL_CR].quality;
    } else {
        params.quality.luma_quality = quality;
        params.quality.cb_quality = quality;
        params.quality.cr_quality = 0;
    }
    xglJpegEncode(&params);
    return params.output_size;
}

INCLUDE_ASM("asm/main/nonmatchings/game_snap_shot", GameSnapShotSave);

INCLUDE_ASM("asm/main/nonmatchings/game_snap_shot", func_001E5330);
