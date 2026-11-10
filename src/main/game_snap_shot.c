#include "common.h"
#include "shared.h"
#include "main/game_snap_shot.h"
#include "main/xgl_dma.h"

typedef unsigned int XglQuadword __attribute__((mode(TI)));

typedef union SnapshotDmaPacket {
    u64 words[24];
    XglQuadword quadwords[12];
} SnapshotDmaPacket;

enum {
    SNAPSHOT_OPEN_FLAGS = 0x602,
    SNAPSHOT_CACHE_LINE_SIZE = 64,
    SNAPSHOT_WIDTH = 512,
    SNAPSHOT_HEIGHT = 448,
    SNAPSHOT_METADATA_SIZE = 160,
    SNAPSHOT_HEADER_SIZE = 6,       /* SOI marker, APP14 marker and its length */
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
    SNAPSHOT_FILE_MODE = 0666,
    SNAPSHOT_FRAME1_FBP_MASK = 0x1ff,
    SNAPSHOT_FRAME1_FBW_MASK = 0x3f,
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

typedef struct {
    unsigned char unmodeled_00[0x10];
    unsigned long long frame1;
} GameSnapShotDrawEnv;

extern GameSnapShotDrawEnv DrawEnv;
extern u8 *WorkEnd;
extern const char snapShotLogFormat[];
extern const char snapShotQualityFormat[];
extern int printf(const char *format, ...);
extern int sceOpen(const char *path, int flags, ...);
extern int sceWrite(int fd, const void *buffer, int size);
extern int sceClose(int fd);

/* Encodes the 512x448 display frame (DrawEnv display origin) into the WorkEnd buffer and writes the JPEG to path. */
void GameSnapShotWriteJpeg(const char *path, int quality) {
    GameSnapShotJpegParams params;
    unsigned int output_buffer;
    int fd;
    int frame_base;

    memset(&params, 0, sizeof(params));
    output_buffer = ((unsigned int)WorkEnd + (SNAPSHOT_CACHE_LINE_SIZE - 1)) & ~(SNAPSHOT_CACHE_LINE_SIZE - 1);
    params.output_buffer = output_buffer;
    printf(snapShotLogFormat, path, output_buffer);
    frame_base = DrawEnv.frame1 & SNAPSHOT_FRAME1_FBP_MASK;
    params.source_x = frame_base << 5;
    params.source_y = (DrawEnv.frame1 >> 16) & SNAPSHOT_FRAME1_FBW_MASK;
    params.width = SNAPSHOT_WIDTH;
    params.height = SNAPSHOT_HEIGHT;
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
    printf(snapShotQualityFormat, params.quality.luma_quality, params.quality.cb_quality, params.quality.cr_quality);
    xglJpegEncode(&params);
    fd = sceOpen(path, SNAPSHOT_OPEN_FLAGS, SNAPSHOT_FILE_MODE);
    sceWrite(fd, (const void *)params.output_buffer, params.output_size);
    sceClose(fd);
}

/* Points at the three-character extension of the snapshot file name. */
extern char *snapnamext;
extern char snapname[];
extern void xglClockRead(void *time);
extern void xglClockUInt2DayTime(void *time, unsigned int seconds);

typedef struct {
    unsigned char unmodeled_00[0x20];
    int save_word;
} GameSnapShotWork;

extern GameSnapShotWork gameWork;

typedef struct {
    unsigned char save_time[8];
    unsigned char play_time[8];
    int save_word;
    unsigned char unmodeled_14[0x8C];
} GameSnapShotMetadata;

/* Calls GameSnapShotWriteJpeg; with flags & 1 writes the
 * 160-byte metadata to the .bin name first, then advances the four-digit counter in snapnameno. */
void GameSnapShotExecute(int quality, int flags) {
    GameSnapShotMetadata metadata;
    int file_handle;

    GameSnapShotWriteJpeg(snapname, quality);
    if (flags & 1) {
        snapnamext[0] = 'b';
        snapnamext[1] = 'i';
        snapnamext[2] = 'n';
        memset(&metadata, 0, sizeof(metadata));
        xglClockRead(metadata.save_time);
        xglClockUInt2DayTime(metadata.play_time, 0);
        metadata.save_word = gameWork.save_word;
        file_handle = sceOpen(snapname, SNAPSHOT_OPEN_FLAGS, SNAPSHOT_FILE_MODE);
        sceWrite(file_handle, &metadata, sizeof(metadata));
        sceClose(file_handle);
        snapnamext[0] = 'j';
        snapnamext[1] = 'p';
        snapnamext[2] = 'g';
    }
    if (snapnameno[3] == '9') {
        if (snapnameno[2] == '9') {
            snapnameno[1]++;
            snapnameno[2] = '0';
        } else {
            snapnameno[2]++;
        }
        snapnameno[3] = '0';
    } else {
        snapnameno[3]++;
    }
}

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

/* Scaffold data labels of the words the original reads: gameWork.save_word (0x2B5DB0)
 * and DrawEnv.frame1 (0x645650). The original relocates against these labels. */
extern int D_002B5DB0[];
extern unsigned long long D_00645650[];

int GameSnapShotSave(int quality, unsigned char *output) {
    GameSnapShotJpegParams params;
    GameSnapShotMetadata meta;
    unsigned char *output_head;
    unsigned char *write_cursor;
    unsigned int thumbnail_size;
    int app14_length;
    int frame_base;

    output_head = output;
    write_cursor = output;
    write_cursor[1] = SNAPSHOT_MARKER_SOI;
    write_cursor[0] = SNAPSHOT_MARKER_PREFIX;
    write_cursor[2] = SNAPSHOT_MARKER_PREFIX;
    write_cursor[3] = SNAPSHOT_MARKER_APP14;
    write_cursor += SNAPSHOT_HEADER_SIZE;

    memset(&meta, 0, SNAPSHOT_METADATA_SIZE);
    xglClockRead(meta.save_time);
    xglClockUInt2DayTime(meta.play_time, 0);
    meta.save_word = D_002B5DB0[0];
    *(GameSnapShotMetadata *)write_cursor = meta;
    write_cursor += SNAPSHOT_METADATA_SIZE;

    thumbnail_size = GameSnapShotSaveThumbnail(SNAPSHOT_THUMBNAIL_QUALITY, (unsigned int)write_cursor);
    app14_length = thumbnail_size + SNAPSHOT_METADATA_SIZE + SNAPSHOT_SEGMENT_OVERHEAD;
    output_head[5] = app14_length;
    output_head[4] = app14_length >> 8;
    write_cursor += thumbnail_size;
    thumbnail_size += SNAPSHOT_METADATA_SIZE + SNAPSHOT_HEADER_SIZE;

    memset(&params, 0, sizeof(params));
    params.output_buffer = (unsigned int)write_cursor;
    frame_base = D_00645650[0] & SNAPSHOT_FRAME1_FBP_MASK;
    params.source_x = frame_base << 5;
    params.source_y = (D_00645650[0] >> 16) & SNAPSHOT_FRAME1_FBW_MASK;
    params.width = SNAPSHOT_WIDTH;
    params.height = SNAPSHOT_HEIGHT;
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
    printf(snapShotLogFormat, snapname, params.output_buffer);
    printf(snapShotQualityFormat, params.quality.luma_quality, params.quality.cb_quality, params.quality.cr_quality);
    xglJpegEncode(&params);
    return thumbnail_size + params.output_size;
}

int GameSnapShotSaveFile(int slot_number, const void *data, int size)
{
    int file_handle;

    if (slot_number >= 0) {
        snapnameno[3] = slot_number % 10 + '0';
        slot_number /= 10;
        snapnameno[2] = slot_number % 10 + '0';
        slot_number /= 10;
        snapnameno[1] = slot_number % 10 + '0';
        slot_number /= 10;
        snapnameno[0] = slot_number % 10 + '0';
    }
    file_handle = sceOpen(snapname, SNAPSHOT_OPEN_FLAGS, SNAPSHOT_FILE_MODE);
    sceWrite(file_handle, data, size);
    sceClose(file_handle);
    slot_number = (snapnameno[3] - '0') * 1000 + (snapnameno[2] - '0') * 100 +
            (snapnameno[1] - '0') * 10 + (snapnameno[0] - '0');
    if (snapnameno[3] == '9') {
        if (snapnameno[2] == '9') {
            snapnameno[1]++;
            snapnameno[2] = '0';
        } else {
            snapnameno[2]++;
        }
        snapnameno[3] = '0';
        return slot_number;
    }
    snapnameno[3]++;
    return slot_number;
}
