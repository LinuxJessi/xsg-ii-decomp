#include "common.h"
#include "shared.h"

extern char *strcat(char *destination, const char *source);
extern int sprintf(char *destination, const char *format, ...);
extern u8 *buffer;
extern char *strcpy(char *destination, const char *source);

int srsLoadEffectData(void *buffer, int effectNo);

extern int xglCdReadFile(const char *name, void *buffer, int mode, int flags);
extern int xglCdGetFileSize(const char *name);

/* Battle slots of the resource state. */
#define SRS_CHARACTER_SLOTS 3        /* party members that own weapon data */

#define SRS_WEAPONS_PER_CHARACTER 3  /* weapon effects loaded per party member */
#define SRS_COMBO_SLOTS 3            /* enemy combo data slots */
#define SRS_EFFECT_SLOTS 24          /* loaded cf effect slots */

/* Image-mapper resource types: every slot of the resource state is indexed by
 * the type it registers with svAddImageMapper. srsAnalyzeEftNo reports the
 * same numbers as the effect category. */
typedef enum {
    SRS_RES_IMAGE = 0,
    SRS_RES_EFFECT_IMAGE = 1,
    SRS_RES_WEAPON = 2,         /* 3 characters x 3 weapon effects */
    SRS_RES_COMBO = 11,         /* 3 enemy combo slots */
    SRS_RES_BATTLE_IMAGE = 14,
    SRS_RES_CF_IMAGE = 15,
    SRS_RES_EFFECT = 16,        /* 24 loaded effect slots */
    SRS_RES_COUNT = 40
} SrsResourceType;

/* The complete 0x1A0-byte resource state shared with sef.c. The pending
 * mapper arguments and resource numbers are tables indexed by the resource
 * type each data pointer registers with svAddImageMapper. */
typedef struct SrsMemRes {
    void *image;
    void *effectImage;
    void *weaponData[SRS_CHARACTER_SLOTS][SRS_WEAPONS_PER_CHARACTER];
    void *comboData[SRS_COMBO_SLOTS];
    void *battleImage;
    void *cfImage;
    void *effectData[SRS_EFFECT_SLOTS];
    short pending[SRS_RES_COUNT];
    short no[SRS_RES_COUNT];
    void *effectExtra[SRS_EFFECT_SLOTS];
} SrsMemRes;

extern SrsMemRes _srsMemRes;
extern int srsGetEffect2Idx(int effectNo);

void srsSetLoadMode(int load_mode);

char *srsGetEsdData(int index);
char *srsGetEffectName(int effectNo);

int fileLoad(void *buffer, const char *name, int mode);

extern int srsLoadMode;
extern int _nRead;

#define SRS_EFFECT2_INDEX_ROWS 117   /* _srsEffect2IndexTbl */
#define SRS_EFFECT2_INDEX_FIRST 601  /* effect number of its first row */
#define SRS_PATH_LENGTH 256          /* resolved file path and view path */
#define SRS_NAME_LENGTH 128          /* "esd/NAME.esd" style file name buffers */

/* The effect-name (.esd) table: one 16-byte name per effect number. */
#define SRS_EFFECT_NO_COUNT 0xC00    /* effect numbers are below this */

#define SRS_ESD_ENTRY_SIZE 0x10
#define SRS_ESD_TABLE_SIZE 0xC800

/* srsLoadMode values. */
#define SRS_LOAD_MODE_CD 0

#define SRS_LOAD_MODE_HOST 1
#define SRS_LOAD_MODE_USB 2
#define SRS_USB_PREFIX_LENGTH 5      /* "usb0:" */

/* sceOpen flags and sceLseek origins used by the host loader. */
#define SRS_OPEN_RDONLY 1

#define SRS_SEEK_SET 0
#define SRS_SEEK_END 2

/* xglCdReadFile completion callback: the read finished. */
#define SRS_CD_READ_COMPLETE 4

/* svAddImageMapper "height" (the fourth argument) registered for each resource. */
#define SRS_MAPPER_HEIGHT_IMAGE 3000

#define SRS_MAPPER_HEIGHT_EFFECT_IMAGE 3100
#define SRS_MAPPER_HEIGHT_WEAPON 2000
#define SRS_MAPPER_HEIGHT_COMBO 2100
#define SRS_MAPPER_HEIGHT_BATTLE_IMAGE 3400
#define SRS_MAPPER_HEIGHT_CF_IMAGE 3500
#define SRS_MAPPER_HEIGHT_EFFECT 3600

/*
 * Effect number ranges (first number and count). Each range resolves to one
 * of the SrsResourceType categories in srsAnalyzeEftNo/srsGetEffectType.
 */
#define SRS_EFFECT_COMMON_FIRST 2600       /* default.esp */

/* Weapon effects 2900..2998 share the data of effects 100 lower. */
#define SRS_EFFECT_WEAPON_ALT_FIRST 2900

#define SRS_EFFECT_WEAPON_ALT_COUNT 99
#define SRS_EFFECT_WEAPON_ALT_OFFSET 100

extern char _loadEsdData[SRS_ESD_TABLE_SIZE];
extern void sresDataMapping(void);
extern void svAddImageMapper(int type, int width, void *chunk, int height);
extern int sceOpen(const char *path, int flags, ...);
extern int sceLseek(int descriptor, int offset, int whence);
extern int sceRead(int descriptor, void *buffer, int bytes);
extern int sceClose(int descriptor);
extern int func_00A103C8(const char *path, void *buffer);
extern void srsCdReadCallback(int code);
extern char srsViewPath[SRS_PATH_LENGTH];
extern signed char _srsEffect2IndexTbl[SRS_EFFECT2_INDEX_ROWS];

int srsGetEffect2Idx(int effectNo);
int srsMakeFileName(char *buffer, const char *fileName);

char *srsGetEffectData(int index);

void srsCdReadCallback(int code);
void sresDataMapping(void);

INCLUDE_ASM("asm/main/nonmatchings/srs", func_00265820);

INCLUDE_ASM("asm/main/nonmatchings/srs", func_00265898);

INCLUDE_ASM("asm/main/nonmatchings/srs", func_00265938);

INCLUDE_ASM("asm/main/nonmatchings/srs", func_00265970);

void srsSetLoadMode(int load_mode)
{
    srsLoadMode = load_mode;
}

INCLUDE_ASM("asm/main/nonmatchings/srs", func_00265AC0);

void srsSetViewPath(char *path)
{
    strcpy(srsViewPath, path);
}

INCLUDE_ASM("asm/main/nonmatchings/srs", func_00265AE8);

INCLUDE_ASM("asm/main/nonmatchings/srs", func_00265AF8);

INCLUDE_ASM("asm/main/nonmatchings/srs", func_00265C68);

void srsChangeSeparator(char *path)
{
    int index;

    if (path != 0) {
        for (index = 0; path[index] != '\0'; index++) {
            if (path[index] == '/')
                path[index] = '\\';
        }
    }
}

INCLUDE_ASM("asm/main/nonmatchings/srs", fileLoad);

INCLUDE_ASM("asm/main/nonmatchings/srs", func_00265FB0);

INCLUDE_ASM("asm/main/nonmatchings/srs", srsGetEffectName);

INCLUDE_ASM("asm/main/nonmatchings/srs", func_00266018);

INCLUDE_ASM("asm/main/nonmatchings/srs", func_00266068);

void srsInitCdRead(void)
{
    _nRead = 0;
}

INCLUDE_ASM("asm/main/nonmatchings/srs", func_002660A8);

INCLUDE_ASM("asm/main/nonmatchings/srs", func_002660B0);

int srsLoadEffectData(void *buffer, int effectNo)
{
    char *name;

    name = srsGetEffectName(effectNo);
    if (name == 0) {
        return -1;
    }
    return fileLoad(buffer, name, 1);
}

INCLUDE_ASM("asm/main/nonmatchings/srs", func_00266188);
