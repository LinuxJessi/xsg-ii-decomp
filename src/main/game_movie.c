#include "common.h"
#include "shared.h"
#include "main/pad.h"

char *caption_nextline(char *cursor);
char *caption_getparam(char *cursor, int *time, char *out);
void fadeStartOut(int);

/* Calls fadeStartOut(1); the callee is not named yet, so this wrapper is left as GameMovieCaptionFadeOut. */
void GameMovieCaptionFadeOut(void) {
    fadeStartOut(1);
}

char *caption_nextline(char *cursor)
{
    char character;

    do {
        if (*cursor != '\r' && *cursor != '\n') {
            if (*cursor != ';') {
                break;
            }
            cursor++;
        }
        for (;;) {
            character = *cursor++;
            if (character == '\r' || character == '\n') {
                if (*cursor != '\r' && *cursor != '\n' && *cursor != ';') {
                    break;
                }
            }
        }
    } while (0);
    return cursor;
}

extern int xglFontGetStringWidth(const char *text);

/* Reads one caption entry: a time stamp (minutes:seconds:frames, or minutes and seconds only; a leading
 * '+' makes it relative to the previous *time) followed by blanks and the text up to a CR, LF or
 * backslash. The text is written to out with a two byte header (a tag byte of 1 and the complement of
 * half the text width); a backslash starts a further line in the same entry, '@' starts a line with a
 * space and "[base/ruby]" expands to the ruby control strings. *time receives the time stamp in frames,
 * or -1 when the line has none. Returns the start of the next entry. */
char *caption_getparam(char *cursor, int *time, char *out)
{
    int frames;
    int colons;
    int relative;
    int value;
    int scaled;
    int width;
    char ch;
    char *text;

    if ((*cursor >= '0' && *cursor <= '9') || *cursor == '+') {
        colons = 0;
        relative = 0;
        frames = 0;
        if (*cursor == '+') {
            cursor++;
            relative = 1;
        }
        for (;;) {
            value = 0;
            while (*cursor >= '0' && *cursor <= '9') {
                value = value * 10 + *cursor++ - '0';
            }
            frames += value;
            if (*cursor == ':') {
                colons++;
                scaled = frames * 15;
                frames = scaled * 2;
                if (colons == 1) {
                    frames = scaled * 4;
                }
                cursor++;
            }
            if (*cursor == ' ' || *cursor == '\t' || *cursor == '\r' || *cursor == '\n') {
                break;
            }
        }
        while (*cursor == ' ' || *cursor == '\t') {
            cursor++;
        }
        if (relative) {
            frames += *time;
        }
        for (;;) {
            out += 2;
            text = out;
            if (*cursor == '@') {
                *out++ = ' ';
                cursor++;
            }
            ch = *cursor++;
            while (ch != '\\' && ch != '\r' && ch != '\n') {
                if (ch == '[') {
                    /* The TU builds with -fno-builtin: the original copies the three
                     * ruby tags inline (ldl/ldr, sdl/sdr), which only the explicit
                     * builtin produces. */
                    __builtin_strcpy(out, "$rubyb;");
                    while (*out != 0) {
                        out++;
                    }
                    ch = *cursor++;
                    while (ch != '/') {
                        *out++ = ch;
                        ch = *cursor++;
                    }
                    __builtin_strcpy(out, "$rubys;");
                    while (*out != 0) {
                        out++;
                    }
                    ch = *cursor++;
                    while (ch != ']') {
                        *out++ = ch;
                        ch = *cursor++;
                    }
                    __builtin_strcpy(out, "$rubye;");
                    while (*out != 0) {
                        out++;
                    }
                } else {
                    *out++ = ch;
                }
                ch = *cursor++;
            }
            *out = 0;
            width = xglFontGetStringWidth(text);
            text[-2] = 1;
            text[-1] = ~(width / 2);
            if (ch != '\\') {
                break;
            }
            *out++ = '\n';
            cursor++;
        }
    } else {
        frames = -1;
    }

    *time = frames;
    *out = 0;
    return caption_nextline(cursor);
}

extern const char movieCaptionControl[];

/* Lays the caption script out in place: the first line is kept as the header text when its time is 0
 * (otherwise the header is empty), then one (time, text) record per line until a negative time.
 * Returns the size of the converted script. */
int caption_convert(char *text)
{
    char *dst;
    char *src;
    char *next;
    int time;

    dst = text;
    /* The width is not used; the original makes the call all the same. */
    xglFontGetStringWidth(movieCaptionControl);
    src = caption_nextline(text);
    time = 0;
    next = caption_getparam(src, &time, text);
    if (time == 0) {
        while (*dst++ != 0) {
        }
        src = next;
    } else {
        *text = 0;
        dst = text + 1;
    }
    time = 0;
    do {
        src = caption_getparam(src, &time, dst + 2);
        dst[0] = time;
        dst[1] = time >> 8;
        dst += 4;
        while (*dst++ != 0) {
        }
    } while (time >= 0);
    return dst - text;
}

typedef struct XglPacket XglPacket;

/* The two frame-buffer base pages read by the copy packets below and the movie flag of the caption player. */
typedef struct MovieRenderState {
    unsigned char unmodeled_00[0x10];
    unsigned short buffer_base[2]; /* +0x10, +0x12 */
    unsigned char unmodeled_14[0x84 - 0x14];
    unsigned char movie_active;    /* +0x84: set while GameMovieCaptionPlay runs the player */
} MovieRenderState;

typedef struct MovieGsRegister {
    u64 data;
    u64 address;
} MovieGsRegister;

/* VIF DIRECT header, A+D GIF tag and the five registers of a local-to-local GS copy. */
typedef struct MovieCopyPacket {
    u64 header_padding;
    u64 vif_direct;
    u64 gif_tag;
    u64 gif_registers;
    MovieGsRegister bitbltbuf;
    MovieGsRegister trxpos;
    MovieGsRegister trxreg;
    MovieGsRegister trxdir;
    MovieGsRegister texflush;
} MovieCopyPacket;

/* EE scratchpad RAM (lui 0x7000). */
#define MOVIE_COPY_PACKET ((MovieCopyPacket *)0x70000000)
#define MOVIE_COPY_QWORDS 7

extern MovieRenderState sRender;
extern XglPacket *xglPacketGetCurrent(void);
extern void sceVif1PkCnt(XglPacket *packet, int count);
extern void sceVif1PkOpenDirectHLCode(XglPacket *packet, int count);
extern void sceVif1PkAddDirectDataN(XglPacket *packet, const void *data, int qwc);
extern void sceVif1PkCloseDirectHLCode(XglPacket *packet);

/* func_001E7B08 and func_001E7C08 queue the same 512x448 GS copy between the two frame-buffer pages at
 * sRender +0x10 / +0x12; they differ only in which page is the source. Direction not decided yet, so unnamed. */
void func_001E7B08(void)
{
    XglPacket *packet;
    MovieCopyPacket *copy;

    packet = xglPacketGetCurrent();
    copy = MOVIE_COPY_PACKET;
    copy->header_padding = 0;
    copy->vif_direct = 0x51000006;
    copy->gif_tag = 0x1000000000008005ULL;
    copy->gif_registers = 0xE;
    copy->bitbltbuf.data = ((u64)(sRender.buffer_base[1] << 5) | 0x80000) | ((u64)sRender.buffer_base[0] << 37) |
                           ((u64)0x8000 << 36);
    copy->bitbltbuf.address = 0x50;
    copy->trxpos.data = 0;
    copy->trxpos.address = 0x51;
    copy->trxreg.data = ((u64)448 << 32) | 512;
    copy->trxreg.address = 0x52;
    copy->trxdir.data = 2;
    copy->trxdir.address = 0x53;
    copy->texflush.data = 0;
    copy->texflush.address = 0x3F;
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    sceVif1PkAddDirectDataN(packet, MOVIE_COPY_PACKET, MOVIE_COPY_QWORDS);
    sceVif1PkCloseDirectHLCode(packet);
}

void func_001E7C08(void)
{
    XglPacket *packet;
    MovieCopyPacket *copy;

    packet = xglPacketGetCurrent();
    copy = MOVIE_COPY_PACKET;
    copy->header_padding = 0;
    copy->vif_direct = 0x51000006;
    copy->gif_tag = 0x1000000000008005ULL;
    copy->gif_registers = 0xE;
    copy->bitbltbuf.data = ((u64)(sRender.buffer_base[0] << 5) | 0x80000) | ((u64)sRender.buffer_base[1] << 37) |
                           ((u64)0x8000 << 36);
    copy->bitbltbuf.address = 0x50;
    copy->trxpos.data = 0;
    copy->trxpos.address = 0x51;
    copy->trxreg.data = ((u64)448 << 32) | 512;
    copy->trxreg.address = 0x52;
    copy->trxdir.data = 2;
    copy->trxdir.address = 0x53;
    copy->texflush.data = 0;
    copy->texflush.address = 0x3F;
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    sceVif1PkAddDirectDataN(packet, MOVIE_COPY_PACKET, MOVIE_COPY_QWORDS);
    sceVif1PkCloseDirectHLCode(packet);
}

/* The movie player record of src/main/tu017.c (AllocRecord there) with the frame counter read back here. */
typedef struct MoviePlayer {
    int handle;
    int data_start;
    int frame;                 /* +0x08 */
    unsigned char clear_state;
    unsigned char extra_enabled;
    signed char max_value;
    unsigned char unmodeled_0f;
} MoviePlayer;

/* The fields of the global game block this file uses: the flags word, the pad input delay and the scene number. */
typedef struct MovieGameWork {
    unsigned char unmodeled_00[8];
    int flags;                  /* +0x08 */
    unsigned char input_delay;  /* +0x0C */
    unsigned char unmodeled_0d[0x18 - 0x0D];
    int scene;                  /* +0x18 */
} MovieGameWork;

/* The two save settings the movie player reads. */
typedef struct MovieSaveSettings {
    unsigned char unmodeled_00[0x29];
    unsigned char extra_enabled;   /* +0x29: passed to movieRenderBufferSize/Alloc */
    unsigned char unmodeled_2a[0x3A - 0x2A];
    unsigned char caption_all;     /* +0x3A: nonzero shows every caption line, not only the ones starting with a space */
} MovieSaveSettings;

#define MOVIE_FLAG_SKIP_ALL 0x10000000

#define PAD_BUTTON_SKIP 0x10
#define PAD_BUTTON_REPLAY 0x20
#define PAD_BUTTON_START 0x800
#define PAD_COMBO_ABORT 0x10F
/* held PAD_COMBO_ABORT and pressed PAD_BUTTON_START in PadData.heldAndPressed */
#define PAD_COMBO_ABORT_PRESSED 0x800010FULL

#define MOVIE_SE_PAUSE 0x2000011

extern MovieGameWork gameWork;
extern MovieSaveSettings SaveData;
extern PadRecord PadData;
extern unsigned char D_0069E094;
extern int StudioEntrySkip;
extern char *D_0069D000; /* ".sfd" */
extern char *D_0069D010; /* "movie/" */
extern const char D_00688420[]; /* "$ay%d;$xx0,22;" */
extern const char D_00688430[]; /* the "seek error" message with its replay/skip hints */

void fadeClear(void);
void soundRestart(void);
int movieRenderBufferSize(int include_extra);
int movieRenderBufferAlloc(MoviePlayer *player, int cursor, int extra_enabled);
void *GameResArenaReserve(unsigned int bytesWanted);
void GameResArenaRelease(void);
int xglCdServiceIfPending(void);
void xglFontLoad(int, int);
void xglRenderClearDepth(void);
/* Also returns the two-letter code (daddu $2,$3 before its jr); declaring it void moves the '/' constant from $3 to $2. */
int xglGetLanguageCode(char *code);
int xglCdReadFile(const char *name, void *buffer, int mode, int flags);
int func_001A0378(MoviePlayer *player, const char *name);
int func_001A04A0(MoviePlayer *player);
int func_001A0688(MoviePlayer *player, int pause);
int func_001A06B8(MoviePlayer *player);
void xglFontPrint(int x, int y, unsigned int color, const char *text);
void xglFontPrintDirect(const char *text);
char *func_00195E90(int mode, char *cursor, int flag);
int func_002AA5D8(char *destination, const char *format, ...);
void pauseRefBackdrop(void);
void pauseDrawTitleAndHints(int skip_hidden);
void xglSleep(void);
void xglClockSampleDayTime(void);
void xglClockAccumulate(void);
void seVoiceStartDefault(int voice);

/* Plays the movie name (file name.sfd) with its caption script movie/<language>/name.txt: the script is read
 * with xglCdReadFile into the render buffer, laid out by caption_convert and printed with xglFontPrintDirect
 * while the player runs; START pauses, the abort combination skips every movie, a seek error offers replay or
 * skip. Called by the mpeg2 playback. */
void GameMovieCaptionPlay(char *name)
{
    char path[0x40];
    MoviePlayer player;
    char control[0x20];
    int replay;
    char *buffer;
    char *text;
    char *src;
    char *cursor;
    char *dst;
    unsigned char *entry;
    char *line;
    int status;
    int size;
    int time;
    int lines;
    int y;
    int pass;
    int skip_hidden;

    fadeClear();
    skip_hidden = 0;
    func_001E7C08();
    soundRestart();
    buffer = GameResArenaReserve(movieRenderBufferSize(SaveData.extra_enabled));
    if (gameWork.flags & MOVIE_FLAG_SKIP_ALL) {
        return;
    }
    while (xglCdServiceIfPending() != 0) {
    }
    xglFontLoad(0, 0);
    src = name;
    cursor = path;
    if (*src == 'E') {
        skip_hidden = src[1] == 'P';
    }
    while ((*cursor = *src) != 0) {
        src++;
        cursor++;
    }
    src = D_0069D000;
    while ((*cursor = *src) != 0) {
        src++;
        cursor++;
    }
    xglRenderClearDepth();
    do {
        dst = buffer;
        replay = 0;
        cursor = D_0069D010;
        while (*cursor != 0) {
            *dst++ = *cursor++;
        }
        xglGetLanguageCode(dst);
        dst[2] = '/';
        dst += 3;
        for (cursor = path; *cursor != 0; *dst++ = *cursor++) {
            if (*cursor == '.') {
                dst[0] = '.';
                dst[1] = 't';
                dst[2] = 'x';
                dst[3] = 't';
                dst[4] = 0;
                break;
            }
        }
        size = xglCdReadFile(buffer, buffer, 0, 0);
        if (size < 0) {
            size = 0;
        } else {
            size = caption_convert(buffer);
        }
        text = 0;
        if (size != 0) {
            text = buffer;
        }
        movieRenderBufferAlloc(&player, (int)(buffer + ((size + 63) & ~63)), SaveData.extra_enabled);
        if (func_001A0378(&player, path) >= 0) {
            sRender.movie_active = 1;
            D_0069E094 = StudioEntrySkip = 1;
            gameWork.input_delay = 32;
            for (;;) {
                status = func_001A04A0(&player);
                if (text != 0) {
                    entry = (unsigned char *)text;
                    while (*entry++ != 0) {
                    }
                    time = entry[0] + (entry[1] << 8);
                    if (time >= 0 && player.frame >= time) {
                        text = (char *)(entry + 2);
                    }
                    xglFontPrint(0, 317, 0xFFFFFF, movieCaptionControl);
                    y = 370;
                    lines = 1;
                    line = text;
                    while (*line != 0) {
                        if (*line == '\n') {
                            lines++;
                        }
                        line = func_00195E90(0, line, 0);
                    }
                    if (lines >= 3) {
                        y -= (lines - 2) * 22;
                    }
                    func_002AA5D8(control, D_00688420, y);
                    xglFontPrintDirect(control);
                    if (SaveData.caption_all != 0 || text[2] == ' ') {
                        xglFontPrintDirect(text);
                    }
                }
                if (status < 0) {
                    func_001E7B08();
                    for (pass = 0; pass < 2; pass++) {
                        xglFontPrint(96, 200, 0xFFFFFF, D_00688430);
                        pauseRefBackdrop();
                        xglSleep();
                    }
                    xglClockSampleDayTime();
                    for (;;) {
                        if (PadData.pressed & PAD_BUTTON_SKIP) {
                            GameMovieCaptionFadeOut();
                            break;
                        }
                        if (PadData.pressed & PAD_BUTTON_REPLAY) {
                            replay = 1;
                            break;
                        }
                        xglSleep();
                    }
                    seVoiceStartDefault(MOVIE_SE_PAUSE);
                    xglClockAccumulate();
                    break;
                }
                if (status > 0) {
                    break;
                }
                if (player.frame >= 0) {
                    if (gameWork.input_delay != 0) {
                        gameWork.input_delay--;
                    } else if (PadData.pressed & PAD_BUTTON_START) {
                        func_001A0688(&player, 1);
                        xglClockSampleDayTime();
                        gameWork.input_delay = 32;
                        if ((PadData.held & PAD_COMBO_ABORT) == PAD_COMBO_ABORT) {
                            gameWork.flags |= MOVIE_FLAG_SKIP_ALL;
                            break;
                        }
                        seVoiceStartDefault(MOVIE_SE_PAUSE);
                        xglSleep();
                        func_001E7B08();
                        pauseDrawTitleAndHints(skip_hidden);
                        xglSleep();
                        pauseDrawTitleAndHints(skip_hidden);
                        xglSleep();
                        for (;;) {
                            if (gameWork.input_delay != 0) {
                                gameWork.input_delay--;
                            } else if (skip_hidden == 0 && (PadData.pressed & PAD_BUTTON_SKIP)) {
                                GameMovieCaptionFadeOut();
                                break;
                            } else if (PadData.pressed & PAD_BUTTON_START) {
                                break;
                            }
                            xglSleep();
                        }
                        if ((PadData.heldAndPressed & PAD_COMBO_ABORT_PRESSED) == PAD_COMBO_ABORT_PRESSED) {
                            gameWork.flags |= MOVIE_FLAG_SKIP_ALL;
                            break;
                        }
                        seVoiceStartDefault(MOVIE_SE_PAUSE);
                        gameWork.input_delay = 32;
                        if (skip_hidden == 0 && (PadData.pressed & PAD_BUTTON_SKIP)) {
                            gameWork.input_delay = 64;
                            break;
                        }
                        xglClockAccumulate();
                        func_001A0688(&player, 0);
                    }
                }
                xglSleep();
            }
            StudioEntrySkip = 0;
            D_0069E094 = 2;
            sRender.movie_active = 0;
        }
        func_001A06B8(&player);
    } while (replay);
    /* The scenes whose movie keeps the screen as it is; every other one fades out. */
    if (!(gameWork.scene == 0x1B6C || gameWork.scene == 0x1B6D || gameWork.scene == 0x1B6E ||
          gameWork.scene == 0x1B6F || gameWork.scene == 0x1B71 || gameWork.scene == 0x1B8D ||
          gameWork.scene == 0x1B8E || gameWork.scene == 0x1B8F || gameWork.scene == 0x1B60 ||
          gameWork.scene == 0x1B70 || gameWork.scene == 0x1B72 || gameWork.scene == 0x1B81 ||
          gameWork.scene == 0x1B89 || gameWork.scene == 0x1B90 || gameWork.scene == 0x1B91)) {
        fadeStartOut(1);
    }
    func_001E7B08();
    xglSleep();
    xglSleep();
    xglFontLoad(1, 0);
    GameResArenaRelease();
    fadeClear();
}
