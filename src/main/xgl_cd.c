#include "common.h"
#include "shared.h"

struct request {
    u8 mode;
    u8 data;
};

typedef struct {
    unsigned char status;
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char day;
    unsigned char month;
    unsigned short year;
} XglClock;

typedef struct XglTaskPrefix XglTaskPrefix;
typedef struct XglTaskScheduler XglTaskScheduler;

/*
 * Only the link/callback prefix is evidenced by this allocation.  The
 * complete task records are allocated with a 0x80-byte stride, and callers
 * write task-specific payload beyond this 0x10-byte prefix.  This declaration
 * must not be read as the complete task layout or allocation size.
 */
struct XglTaskPrefix {
    XglTaskScheduler *scheduler;
    XglTaskPrefix *next;
    XglTaskPrefix *previous;
    int (*callback)(XglTaskPrefix *task);
};

struct XglTaskScheduler {
    XglTaskPrefix *free_tasks;
    XglTaskPrefix *active_head;
    XglTaskPrefix *active_tail;
    XglTaskPrefix *next_to_visit;
};

typedef struct StreamXssBuffer StreamXssBuffer;
typedef struct StreamRing StreamRing;

extern u8 *buffer;
extern u8 *WorkEnd;
extern int xglCdReadFile(const char *name, void *buffer, int mode, int flags);

void xglClockRead(XglClock *clock);

extern int xglHddActivate(int state);

extern XglTaskPrefix *xglTaskEntryNext(XglTaskScheduler *scheduler,
                                int (*callback)(XglTaskPrefix *task),
                                XglTaskPrefix *entry);

int xglArxExtract(u32 *dest, u32 *src);

/* Where a file is read from (CdFilePosition.source, CdStreamParam.mode). */
#define CD_SOURCE_DISC 0

#define CD_SOURCE_HOST 1
#define CD_SOURCE_HDD 2

typedef void CdCompletionCallback(int event, int value);
typedef struct CdStreamParam CdStreamParam;

struct StreamRing {
    unsigned char *buffer;
    int capacity;
    int write_position;
    int read_position;
};

struct StreamXssBuffer {
    unsigned char *source;
    unsigned char *position;
    unsigned char value;
    unsigned char mode;
    short sectors;
};

struct CdStreamParam {
    int iop_buffer;
    short sector_count;
    short block_count;
    unsigned char state;
    unsigned char mode;
    /* xss is naturally aligned at 0x0c; no field is asserted at 0x0a. */
    StreamXssBuffer *xss;
    int descriptor;
    int file_byte_count;
    int remaining; /* Bytes not yet transferred into the stream rings. */
    int ring_size;
    StreamRing ring;
    StreamRing alternate_ring;
};

typedef struct CdReadMode {
    unsigned char try_count;
    unsigned char spindle_control;
    unsigned char data_pattern;
} CdReadMode;

/* Only the fields xglCdArcInitSub1 writes are evidenced here. */
typedef struct CdArchiveEntry {
    unsigned char state;
    unsigned char unmodeled_01;
    unsigned char packed;
    unsigned char unmodeled_03;
    unsigned char *destination;
    int lbn;
    int file_lbn_offset;
    int stored_bytes;
    int size_bytes;
} CdArchiveEntry;

/* xglCdGetFilePos fills a 0x30-byte file-position record; only its leading
 * lbn word is evidenced by its callers in this TU. */
typedef struct CdFilePosition {
    unsigned int lbn;
    int stored_bytes;
    unsigned char unmodeled_08[0x1c];
    unsigned char source;
    unsigned char unmodeled_25;
    unsigned char packed;
    unsigned char unmodeled_27;
    int descriptor;
    int size_bytes;
} CdFilePosition;

unsigned int xglClockDayTime2UInt(XglClock *clock_time);

extern int xglCdReadFilePart(const char *name, void *buffer, int mode,
                             CdCompletionCallback *callback, int offset, int length);

/* Events passed to a completion callback (int event, int value). */
#define CD_EVENT_LOOKUP 0 /* value: the path being looked up */

#define CD_EVENT_SIZE 1
#define CD_EVENT_PROGRESS 2
#define CD_EVENT_DONE 4
#define CD_EVENT_READ_ERROR (-2)

void xglCdPowerOffCB(void);
void xglCdArcCheck(void);
void xglClockUInt2DayTime(XglClock *clock_time, unsigned int elapsed);
void xglCdStreamReadRingCore(CdStreamParam *stream);

int xglCdStreamReadRing(CdStreamParam *stream, int bytes);

extern int xglHddCheck(void);
extern void xglHddErrorScreen(void);

/* Keep the direct state expression used by both queue loops. */
#define CD_READ_CONTROL (&LW)

/* A CD sector is fixed at 2048 bytes on this drive/read path (see also
 * xglCdStreamReadRing's own `& -2048` sector rounding later in this TU). */
#define CD_SECTOR_BYTES 2048

#define ARC_RECORD_EXTENDED        0x80
#define ARC_RECORD_TYPE_EXTENDED   0x40
#define ARC_RECORD_LENGTH_MASK     0x3f
#define ARC_RECORD_HEADER_BYTES    7
#define ARC_RECORD_TYPE_EXTRA_BYTES 3
#define ARC_TABLE_ALIGN_BYTES      64
#define CD_PACKED_MAGIC 0x585241
#define ARC_ENTRY_UNUSED 0xff

/* CdReadControl.dispatch_state: what the control thread is doing. */
#define CD_STATE_IDLE 0

#define CD_STATE_SYNC_READ 1
#define CD_STATE_ASYNC_READ 2
#define CD_STATE_STREAM 3

/* The request queue is a 32-entry ring. */
#define CD_QUEUE_SIZE 32

#define CD_QUEUE_MASK (CD_QUEUE_SIZE - 1)

/* Host and HDD files are read in pieces of at most 64 KiB. */
#define CD_FILE_CHUNK_BYTES 0x10000

#define CD_SECTOR_MASK (CD_SECTOR_BYTES - 1)
#define CD_STREAM_FILE_SECTORS 32
#define CD_STREAM_XSS_SECTORS 0x4000
#define CD_CLOCK_READ_INTERVAL 10
#define CD_PATH_MAX 256
#define CD_ARC_LOOKAHEAD_BYTES 0x2D000
#define CD_ARC_HEADER_RESERVE 0x2000

/* Calendar arithmetic of xglClockDayTime2UInt()/xglClockUInt2DayTime(). */
#define CLOCK_BASE_YEAR 2000

#define CLOCK_SECONDS_PER_MINUTE 60
#define CLOCK_SECONDS_PER_HOUR 3600
#define CLOCK_SECONDS_PER_DAY 86400
#define CLOCK_SECONDS_PER_COMMON_YEAR 31536000

/*
 * The queue entries occupy 0x80 bytes; only their used prefix is named.
 * xglCdReadFilePart (0x0021DEB8..0x0021E30C, still INCLUDE_ASM) enqueues by
 * storing its own second argument (buffer, a single `sw` at 0x0021DFB4) at
 * entry+0x00 and strcpy'ing its own first argument (name, the lbu/sb loop at
 * 0x0021DF90..0x0021DFA8) into entry+0x10; queue_next below hands those two
 * fields back to it in that same order, matching xglCdReadFile's
 * (name, buffer, ...) shape in shared.h.
 */
typedef struct CdRequestPartial {
    void *buffer;
    int offset;
    int length;
    CdCompletionCallback *completion_callback;
    char name[0x70];
} CdRequestPartial;

/*
 * Partial view of LW, limited to fields evidenced by this TU.
 * unmodeled_XX spans are opaque bytes between evidenced fields; naming them
 * `unmodeled_<offset>[<size>]` follows the convention used across the
 * project (e.g. src/main/game.c, src/main/ssd_init.h) instead of inventing
 * members to complete the struct's size.
 */
typedef struct CdReadControl {
    int active_file_descriptor;                 /* +0x00 */
    u8 *write_cursor;                            /* +0x04 */
    int remaining_bytes;                         /* +0x08 */
    int read_poll_mode;                          /* +0x0c */
    CdCompletionCallback *completion_callback;   /* +0x10 */
    u8 unmodeled_014[8];                         /* +0x14..0x1b */
    u8 disk_state;                               /* +0x1c */
    u8 disk_flags;                               /* +0x1d */
    u8 unmodeled_01e[2];                         /* +0x1e..0x1f */
    void *extract_source;                         /* +0x20 */
    void *extract_destination;                    /* +0x24 */
    int extract_byte_count;                      /* +0x28 */
    u8 unmodeled_02c[0x4];                       /* +0x2c..0x2f */
    signed char dispatch_state;                  /* +0x30 */
    u8 spindle_control;                          /* +0x31 */
    u8 search_layer;                             /* +0x32 */
    u8 dual_layer_disc;                          /* +0x33 */
    u8 power_off_pending;                        /* +0x34 */
    u8 unmodeled_035[0x9];                       /* +0x35..0x3d */
    u8 queue_cursor;                             /* +0x3e */
    u8 queue_tail;                               /* +0x3f */
    CdRequestPartial requests[32];               /* +0x40..0x103f */
} CdReadControl;

/* Four 0x18-byte archive slots; the initialization helper models each prefix. */
typedef union CdArchiveStorage {
    CdArchiveEntry entry;
    unsigned char bytes[0x18];
} CdArchiveStorage;

extern const char xenosagaPfs0Path[];
extern const char xenosagaArc00Name[];
extern int sceCdStStat(void);
extern int sceOpen(const char *path, int flags, ...);
extern int sceSifLoadModule(const char *path, int args_length, const char *args);

/* Defined later in this file (INCLUDE_ASM); declared here, like BCD2INT
 * below, so its LOCAL definition does not follow a non-static declaration. */
extern CdFilePosition *xglCdGetFilePos(CdFilePosition *file_position, const char *path,
                           void (*callback)(int event, int value));

extern int sceCdReadClock(unsigned char *clock);

int sceLseek(int descriptor, int offset, int whence);

extern int sceCdSync(int mode);
extern int sceCdDiskReady(int mode);
extern int sceCdRead(int lbn, int sectors, void *destination, CdReadMode *read_mode);

void xglCdArcInit(void);

static int BCD2INT(unsigned char x);

void extract(u32 *destination, u32 *source, int byte_count);

extern int sceCdStRead(int sectors, void *buffer, int mode, int *result);
extern int sceRead(int descriptor, void *buffer, int bytes);

int StreamReadRingCoreSub(CdStreamParam *stream, void *buffer, int sectors);

void StreamReadRingCoreNormal(CdStreamParam *stream);
void StreamReadRingCoreXss(CdStreamParam *stream);

extern u32 sceCdGetReadPos(void);
extern int sceCdGetError(void);
extern int sceCdLayerSearchFile(CdFilePosition *file, const char *name, int layer);
extern const char xenosagaArc10Name[16];
extern const char xenosagaArc20Name[16];
extern const char xenosagaArc30Name[16];
extern const char xenosagaPfs0Path[24];
extern const char xenosagaArc00Name[16];

/* Twelve month lengths followed by twelve cumulative month offsets. */
extern unsigned short monthday[24];

extern char head_8[13];
extern char root_0[6];
extern char hdd_head_6[7];
extern char arcname_1[12];
extern char arcname_2[12];
extern char arcname_3[12];
extern char arcname_4[12];
extern char vcd_head_5[24];
extern char tail_9[7];
extern CdCompletionCallback *callback;
extern CdReadControl LW;
extern CdArchiveStorage ArcHeader[4];
extern signed char ReadClockInterval;
extern unsigned char PresentTime[8];

void xglCdDefaultCallback(int event, int value);
void xglCdDummyCallback(int event, int value);

int xglCdGetFilePosSub(CdArchiveEntry *entry, const char *path);

CdFilePosition *xglCdGetFilePos(CdFilePosition *position, const char *name,
                                       CdCompletionCallback *callback);

int xglCdReadFilePart(const char *name, void *buffer, int mode,
                      CdCompletionCallback *notify, int offset, int length);

void hdd_error(void)
{
    int *descriptor = &LW.active_file_descriptor;

    sceClose(*descriptor);
    *descriptor = -1;
    xglHddErrorScreen();
}

void xglCdPowerOffCB(void)
{
    LW.power_off_pending = 1;
}

void extract(u32 *destination, u32 *source, int byte_count)
{
    if (source[0] == CD_PACKED_MAGIC && source[3] == 0) {
        do {
            xglArxExtract(destination, source);
        } while (0);
    } else if (destination != source) {
        while (byte_count > 0) {
            *destination++ = *source++;
            byte_count -= 4;
        }
    }
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019A958);

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019AA00);

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019AC30);

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019AC60);

static int BCD2INT(unsigned char x)
{
    return (x >> 4) * 10 + (x & 15);
}

void xglClockRead(XglClock *clock)
{
    if (ReadClockInterval == 0) {
        ReadClockInterval = CD_CLOCK_READ_INTERVAL;
        sceCdReadClock(PresentTime);
    }

    clock->year = BCD2INT(PresentTime[7]) + CLOCK_BASE_YEAR;
    clock->month = BCD2INT(PresentTime[6]);
    clock->day = BCD2INT(PresentTime[5]);
    clock->hour = BCD2INT(PresentTime[3]);
    clock->minute = BCD2INT(PresentTime[2]);
    clock->second = BCD2INT(PresentTime[1]);
    clock->status = 0;
}

/*
 * Converts a calendar record to the number of seconds elapsed since
 * 2000-01-01 00:00:00.  monthday[12..23] holds the cumulative day offset of
 * each month; the leap day of the current year counts from March onwards,
 * and ((year - 1997) >> 2) counts the leap days of the whole years before it.
 */
unsigned int xglClockDayTime2UInt(XglClock *clock_time)
{
    unsigned int seconds;
    unsigned int year;
    unsigned char month;

    month = clock_time->month;
    seconds = (monthday[month + 11] + clock_time->day - 1) * CLOCK_SECONDS_PER_DAY
        + clock_time->hour * CLOCK_SECONDS_PER_HOUR + clock_time->minute * CLOCK_SECONDS_PER_MINUTE
        + clock_time->second;
    year = clock_time->year;
    if (((year - CLOCK_BASE_YEAR) & 3) == 0 && month >= 3) {
        seconds += CLOCK_SECONDS_PER_DAY;
    }
    return seconds
        + ((year - CLOCK_BASE_YEAR) * CLOCK_SECONDS_PER_COMMON_YEAR
           + ((year - 1997) >> 2) * CLOCK_SECONDS_PER_DAY);
}

void xglClockUInt2DayTime(XglClock *clock_time, unsigned int elapsed)
{
    XglClock *clock;
    unsigned int year_length = 366;
    unsigned int year = 0;
    unsigned int common_year_length = 365;
    unsigned int leap_year_length = 366;
    unsigned int days;
    unsigned int remainder;
    unsigned int month;
    unsigned int month_length;

    clock = clock_time;
    days = elapsed / CLOCK_SECONDS_PER_DAY;
    remainder = elapsed % CLOCK_SECONDS_PER_DAY;
    clock->hour = remainder / CLOCK_SECONDS_PER_HOUR;
    remainder -= clock->hour * CLOCK_SECONDS_PER_HOUR;
    clock->minute = remainder / CLOCK_SECONDS_PER_MINUTE;
    remainder -= clock->minute * CLOCK_SECONDS_PER_MINUTE;
    clock->second = remainder;

    while (days >= year_length) {
        year += 1;
        days -= year_length;
        year_length = (year & 3) == 0 ? leap_year_length : common_year_length;
    }
    clock->year = year + CLOCK_BASE_YEAR;

    for (month = 0; ; month += 1) {
        month_length = monthday[month];
        if (month == 1 && (clock->year & 3) == 0)
            month_length += 1;
        if (days < month_length)
            break;
        days -= month_length;
    }
    clock->month = month + 1;
    clock->day = days + 1;
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019B0A8);

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", xglCdDefaultCallback);

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", xglCdDummyCallback);

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019B2B8);

CdCompletionCallback *xglCdSetCallback(int callback_address)
{
    CdCompletionCallback *old_callback = callback;

    switch (callback_address) {
    case 0:
        callback = xglCdDefaultCallback;
        break;
    case 1:
        callback = xglCdDummyCallback;
        break;
    case -1:
        break;
    default:
        callback = (CdCompletionCallback *)callback_address;
        break;
    }
    return old_callback;
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019B390);

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019B398);

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019B458);

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", xglCdGetFilePos);

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019BBF8);

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019BC68);

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", xglCdReadFilePart);

int xglCdReadFile(const char *name, void *buffer, int mode, int flags)
{
    return xglCdReadFilePart(name, buffer, mode, (CdCompletionCallback *)flags, 0, -1);
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019C200);

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019C308);

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019C338);

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019C348);

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019C388);

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019C510);

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019C5A0);

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019C5F0);

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019C7A8);

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", xglCdStreamReadRingCore);

int xglCdStreamReadRing(CdStreamParam *stream, int bytes)
{
    int aligned;

    aligned = (bytes + CD_SECTOR_BYTES - 1) & -CD_SECTOR_BYTES;
    stream->ring.read_position =
        (stream->ring.read_position + aligned) % stream->ring.capacity;
    do {
        xglCdStreamReadRingCore(stream);
    } while (stream->remaining != 0 &&
             stream->ring.write_position != stream->ring.read_position);
    return 0;
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019C9E0);

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019CA80);

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019CAF8);

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019CB10);

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019CBD8);

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019CBF8);

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019CCF0);

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019CF48);

void xglCdSifLoadModule(const char *path, const char *args)
{
    char name[CD_PATH_MAX];
    const char *src;
    char *dst;
    int args_length;

    src = head_8;
    dst = name;
    while ((*dst = *src) != 0) {
        src++;
        dst++;
    }
    for (src = path; *src != 0; src++, dst++) {
        if (*src < 'a')
            *dst = *src;
        else
            *dst = *src - ('a' - 'A');
    }
    src = tail_9;
    while ((*dst = *src) != 0) {
        src++;
        dst++;
    }
    args_length = 0;
    if (args != 0) {
        const char *scan = args;

        args_length = 1;
        if (scan[0] != 0 || scan[1] != 0) {
            for (;;) {
                scan++;
                args_length++;
                if (*scan == 0 && scan[1] == 0)
                    break;
            }
        }
    }
    sceSifLoadModule(name, args_length, args);
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019D5B8);

static void xglCdArcInitSub0(const int *archive_entry, void *destination, int sectors)
{
    int lbn;
    CdReadMode read_mode;

    sceCdSync(0);
    sceCdDiskReady(0);
    if (sectors == 0) {
        lbn = archive_entry[0];
        sectors = 1;
    } else {
        lbn = archive_entry[0] + 1;
    }
    read_mode.spindle_control = 1;
    read_mode.try_count = 0;
    read_mode.data_pattern = 0;
    sceCdRead(lbn, sectors, destination, &read_mode);
    while (sceCdSync(1) > 0)
        ;
}

int xglCdArcInitSub1(CdArchiveEntry *archive_entry,
                            const char *path, unsigned char *destination)
{
    CdFilePosition file_position;
    int header_count;

    if (xglCdGetFilePos(&file_position, path, xglCdDummyCallback) == 0)
        return 0;

    archive_entry->destination = destination;
    archive_entry->lbn = file_position.lbn;
    archive_entry->state = 0;

    /* xglCdArcInitSub0 only reads its `archive_entry` argument's leading
     * word as an LBN, so the file position's own lbn field (the one
     * evidenced member of CdFilePosition) is exactly what it needs. */
    xglCdArcInitSub0(&file_position.lbn, destination, 0);

    /* Sector 0 of the file is the archive header; its first byte is the
     * header's own sector count. */
    header_count = destination[0];
    if (header_count >= 2) {
        /* The header spans more than one sector: read the remaining
         * header_count - 1 sectors right after the first. */
        xglCdArcInitSub0(&file_position.lbn, destination + CD_SECTOR_BYTES,
                         header_count - 1);
    }
    return header_count + 1;
}

/*
 * Archive table-of-contents walk: archive_data is a header buffer filled by
 * xglCdArcInitSub1/xglCdArcInit, and this finds the byte right after the
 * table's last record so the caller can place fresh data there.
 *
 * Each record begins with a one-byte length/flags field:
 *   - ARC_RECORD_EXTENDED set: no fixed header past this byte.
 *   - ARC_RECORD_EXTENDED clear: a fixed ARC_RECORD_HEADER_BYTES-byte header
 *     follows, extended by ARC_RECORD_TYPE_EXTRA_BYTES more bytes when
 *     ARC_RECORD_TYPE_EXTENDED is also set.
 *   - ARC_RECORD_LENGTH_MASK (the low bits) is the record's payload length,
 *     walked past after the fixed header, if any.
 * A zero length/flags byte ends the table.
 */
unsigned char *xglCdArcInitSub2(unsigned char *archive_data)
{
    unsigned char *cursor = archive_data + 1;
    unsigned char value = *cursor;

    while (value != 0) {
        int high_bit = value & ARC_RECORD_EXTENDED;
        int type_flag = value & ARC_RECORD_TYPE_EXTENDED;
        if (high_bit == 0) {
            cursor += ARC_RECORD_HEADER_BYTES;
            if (type_flag != 0)
                cursor += ARC_RECORD_TYPE_EXTRA_BYTES;
        }
        value &= ARC_RECORD_LENGTH_MASK;
        cursor += value;
        value = *cursor;
    }

    /* Round up to the next ARC_TABLE_ALIGN_BYTES-byte boundary and write a
     * two-byte zero terminator marking the end of the table. */
    cursor = (unsigned char *)(((u32)cursor + ARC_TABLE_ALIGN_BYTES)
        & (u32)-ARC_TABLE_ALIGN_BYTES);
    cursor[1] = 0;
    cursor[0] = 0;
    return cursor;
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", xglCdArcInit);

void xglCdArcCheck(void)
{
    u8 *saved_work_end = WorkEnd;

    WorkEnd = ArcHeader[0].entry.destination;
    xglCdArcInit();
    WorkEnd = saved_work_end;
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_cd", func_0019DA80);
