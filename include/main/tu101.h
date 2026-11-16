#ifndef INCLUDE_MAIN_TU101_H
#define INCLUDE_MAIN_TU101_H

/* Functions of src/main/tu101.c that other files call. */

/* SaveData as the option pages and the subtitles read it (the save settings). */
typedef struct ConfigSaveSettings {
    unsigned char unmodeled_00[0x28];
    unsigned char soundMode;            /* +0x28: 0..2 (tu100.c's option page cycles it) */
    unsigned char dolby_digital;        /* +0x29 */
    unsigned char unmodeled_2a[0x32 - 0x2A];
    short displayOffsetX;               /* +0x32: the horizontal argument of tyaSetDisplayOffset */
    short displayOffsetY;               /* +0x34: the vertical argument */
    unsigned char unmodeled_36[0x3A - 0x36];
    unsigned char caption_all;          /* +0x3A */
    unsigned char unmodeled_3b[0x3E - 0x3B];
    unsigned char actuatorMode;         /* +0x3E: copied to both pads' PadRecord.actuatorMode */
    unsigned char unmodeled_3f[0x46 - 0x3F];
    unsigned char cursor_memory;        /* +0x46 */
    unsigned char cursor_memory_alt;    /* +0x47 */
} ConfigSaveSettings;


void packetSubmitRequest(int command, int first, int second, int flags);
void packetSubmitRequestEx(int command, int first, int second, int fourth, int fifth, int flags);
void menuDrawKindIcon(int x, int y, int index);
void menuDrawOutlineFrame(int x, int y, int width, int height);
void countdownDraw(void);
int countdownTick(void);
int choiceUpdate(void);
void menuDrawSelectArrows(int x, int width, int y);

#endif /* INCLUDE_MAIN_TU101_H */
