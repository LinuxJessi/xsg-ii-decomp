#include "common.h"
#include "shared.h"
#include "main/srs.h"
#include "main/sv_get_image_item.h"
#include "main/xgl_cd.h"

extern int sprintf(char *destination, const char *format, ...);
extern char *strcpy(char *destination, const char *source);

void srsSetLoadMode(int load_mode);
static int _nRead;

#define SRS_EFFECT2_INDEX_ROWS 117   /* _srsEffect2IndexTbl */
#define SRS_PATH_LENGTH 256          /* resolved file path and view path */
#define SRS_SECTOR_SIZE 2048         /* CD file lengths are rounded up to this */
/* File kinds returned by srsGetEffectFileKind: none, a .esp file or a .esd file. */
enum SrsEffectFileKind {
    SRS_EFFECT_FILE_NONE = 0,
    SRS_EFFECT_FILE_ESP = 1,
    SRS_EFFECT_FILE_ESD = 2
};
/* sceOpen flags and sceLseek origins used by the host loader. */
#define SRS_OPEN_RDONLY 1
#define SRS_OPEN_MODE_READ_ALL 0444  /* file mode bits passed to sceOpen (292) */
#define SRS_SEEK_SET 0
#define SRS_SEEK_END 2
/* xglCdReadFile completion callback: the read finished. */
#define SRS_CD_READ_COMPLETE 4
/* xglCdReadFile result that asks for the read to be retried in mode 1. */
#define SRS_CD_READ_RETRY -3
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
/* effectType written by srsAnalyzeEftNo for .esd effects (also its initial value). */
#define SRS_EFFECT_TYPE_ESD 9

extern void sresDataMapping(void);
extern int sceOpen(const char *path, int flags, ...);
extern int sceLseek(int descriptor, int offset, int whence);
extern int sceRead(int descriptor, void *buffer, int bytes);
extern int sceClose(int descriptor);
/* Read-completion callback for xglCdReadFile, defined below. */
void srsCdReadCallback(int readStatus);
char srsViewPath[SRS_PATH_LENGTH] = "";
extern signed char _srsEffect2IndexTbl[SRS_EFFECT2_INDEX_ROWS];
int srsMakeFileName(char *buffer, const char *fileName);


/* Classifies an effect key from its index (bits 12-21) and kind (bits 28-29):
 * 2 for an .esd effect, 1 for an .esp effect, 0 for none. */
int srsGetEffectFileKind(int index, int kind)
{
    int esdCategory = SRS_EFFECT_FILE_ESD;

    if (kind == 2) {
        return SRS_EFFECT_FILE_ESD;
    }
    if (kind == 1) {
        if (index == 200) {
            return SRS_EFFECT_FILE_NONE;
        }
        return ((u32)(index - 300) < 150U) ? kind : esdCategory;
    }
    if ((u32)(index - 1) < 10U) {
        return SRS_EFFECT_FILE_ESP;
    }
    if ((u32)(index - 101) < 3U) {
        return SRS_EFFECT_FILE_ESP;
    }
    if ((u32)(index - 500) < 300U) {
        return SRS_EFFECT_FILE_ESP;
    }
    return ((u32)(index - 300) < 100U) ? SRS_EFFECT_FILE_NONE : esdCategory;
}

int srsLoadMode = 0;
int srsEffectStamp = 0;
char srsPrefixEe[8] = "ee";
char srsPrefixCe[8] = "ce";
char srsPrefixPe[8] = "pe";
char *srsEffectKindPrefix[4] = { srsPrefixPe, srsPrefixCe, srsPrefixEe, 0 };

/* Formats the effect stem into buffer: the kind prefix from
 * srsEffectKindPrefix, the index as three digits, and, when withSubNo is
 * nonzero, an underscore and the low ten bits of the key as three digits. A
 * negative key prints "bad effect no:%d" instead. */
void srsFormatEffectStem(int effectNo, char *buffer, int withSubNo)
{
    int effectKind = (effectNo >> 28) & 3;
    int effectIndex = (effectNo >> 12) & 0x3ff;
    int effectSub = effectNo & 0x3ff;

    if (effectNo < 0) {
        sprintf(buffer, "bad effect no:%d", effectNo);
        return;
    }
    if (withSubNo != 0) {
        sprintf(buffer, "%s%03d_%03d", srsEffectKindPrefix[effectKind], effectIndex, effectSub);
        return;
    }
    sprintf(buffer, "%s%03d", srsEffectKindPrefix[effectKind], effectIndex);
}

extern void *memset(void *destination, int value, unsigned int size);
static char srsEffectStemBuffer[16];

/* Returns the effect stem with its sub number, in srsEffectStemBuffer. */
char *srsGetEffectStem(int effectNo)
{
    srsFormatEffectStem(effectNo, srsEffectStemBuffer, 1);
    return srsEffectStemBuffer;
}

extern int xglDebFileSize(const char *name);
extern int xglDebFileRead(const char *name, void *buffer);
extern void srsMapPendingResources(void);
static char srsEftNameBuffer[256];
extern char *strcat(char *dest, const char *src);

/* Writes *charNo (the kind, for an .esp effect) and *effectType, and returns
 * the file name of the effect's .esp or .esd file, or 0 when it has none. */
char *srsAnalyzeEftNo(int effectNo, int *charNo, int *effectType)
{
    char *name = 0;
    int effectKind = (effectNo >> 28) & 3;
    int effectIndex = (effectNo >> 12) & 0x3ff;
    int fileKind;

    *charNo = 0;
    *effectType = SRS_EFFECT_TYPE_ESD;
    if (effectNo == 0) {
        return 0;
    }
    fileKind = srsGetEffectFileKind(effectIndex, effectKind);
    switch (fileKind) {
        case SRS_EFFECT_FILE_NONE:
            *charNo = 0;
            *effectType = 0;
            break;
        case SRS_EFFECT_FILE_ESP:
            *charNo = effectKind;
            *effectType = fileKind;
            srsFormatEffectStem(effectNo, srsEftNameBuffer, 0);
            strcat(srsEftNameBuffer, ".esp");
            name = srsEftNameBuffer;
            break;
        case SRS_EFFECT_FILE_ESD:
            *charNo = 0;
            *effectType = SRS_EFFECT_TYPE_ESD;
            srsFormatEffectStem(effectNo, srsEftNameBuffer, 1);
            strcat(srsEftNameBuffer, ".esd");
            name = srsEftNameBuffer;
            break;
    }
    return name;
}

/* Sets the load mode: 0 CD, 1 host, 2 USB. */
void srsSetLoadMode(int load_mode)
{
    srsLoadMode = load_mode;
}

/* Returns the load mode set by srsSetLoadMode. */
int srsGetLoadMode(void)
{
    return srsLoadMode;
}

void srsSetViewPath(char *path)
{
    strcpy(srsViewPath, path);
}

/* Returns the view path set by srsSetViewPath. */
char *srsGetViewPath(void)
{
    return srsViewPath;
}

/* Root text of each load mode, copied as one struct: "simajiri/" (CD, 9 chars
 * and the terminator), "host0:/home/xeno/work/" (host) and "usb0:/" (USB). */
typedef struct { char text[10]; } SrsCdRoot;

typedef struct { char text[23]; } SrsHostRoot;

typedef struct { char text[7]; } SrsUsbRoot;

/* Builds the resource path for the current load mode from the mode's root
 * ("simajiri/", "host0:/home/xeno/work/" or "usb0:/") and srsViewPath. */
int srsMakeFileName(char *buffer, const char *fileName)
{
    if (fileName == 0) {
        buffer[0] = 0;
        return 0;
    }
    if (srsLoadMode == SRS_LOAD_MODE_CD) {
        *(SrsCdRoot *)buffer = *(SrsCdRoot *)"simajiri/";
        strcat(buffer, fileName);
    } else if (srsLoadMode == SRS_LOAD_MODE_HOST) {
        *(SrsHostRoot *)buffer = *(SrsHostRoot *)"host0:/home/xeno/work/";
        strcat(buffer, srsViewPath);
        strcat(buffer, fileName);
    } else if (srsLoadMode == SRS_LOAD_MODE_DEB) {
        strcpy(buffer, srsViewPath);
        strcat(buffer, fileName);
    } else if (srsLoadMode == SRS_LOAD_MODE_USB) {
        *(SrsUsbRoot *)buffer = *(SrsUsbRoot *)"usb0:/";
        strcat(buffer, srsViewPath);
        strcat(buffer, fileName);
    }
    return 1;
}

/* Returns the file length for the current load mode, rounded up to a 2 KB
 * sector, or 0 when the file cannot be opened. */
/* Two counters after the srsMakeFileName literals in .sdata (types not declared in this TU: int guessed). */
int D_0069DC20 = 0;
int D_0069DC24 = 0;
int sefGroupKey = 0;
int D_0069DC2C = 0;

int srsGetFileLen(const char *fileName)
{
    char path[SRS_PATH_LENGTH];
    int fileLen = 0;
    int descriptor;

    if (fileName == 0) {
        return 0;
    }
    if (srsMakeFileName(path, fileName) != 0) {
        if (srsLoadMode == SRS_LOAD_MODE_CD) {
            srsChangeSeparator(path);
            fileLen = xglCdGetFileSize(path);
            if (fileLen <= 0) {
                fileLen = 0;
            }
        } else if (srsLoadMode == SRS_LOAD_MODE_HOST) {
            descriptor = sceOpen(path, SRS_OPEN_RDONLY, SRS_OPEN_MODE_READ_ALL);
            if (descriptor >= 0) {
                fileLen = sceLseek(descriptor, 0, SRS_SEEK_END);
                sceLseek(descriptor, 0, SRS_SEEK_SET);
                sceClose(descriptor);
            }
        } else if (srsLoadMode == SRS_LOAD_MODE_DEB) {
            fileLen = xglDebFileSize(path);
        }
    }
    if (fileLen <= 0) {
        return 0;
    }
    return (fileLen + (SRS_SECTOR_SIZE - 1)) & -SRS_SECTOR_SIZE;
}

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

int fileLoad(void *buffer, const char *name, int readMode)
{
    char path[SRS_PATH_LENGTH];
    int size = 0;

    if (srsMakeFileName(path, name) == 0) {
        return -1;
    }
    if (srsLoadMode == SRS_LOAD_MODE_CD) {
        int readCallback;
        int readResult;

        srsChangeSeparator(path);
        size = xglCdGetFileSize(path);
        if (size <= 0) {
            return -1;
        }
        readCallback = (int)srsCdReadCallback;
        _nRead++;
        readResult = xglCdReadFile(path, buffer, readMode, readCallback);
        if (readResult == SRS_CD_READ_RETRY) {
            readResult = xglCdReadFile(path, buffer, 1, readCallback);
            readMode = 1;
        }
        if (readResult > 0 && readMode == 0) {
            size = readResult;
            _nRead--;
            if (_nRead < 0) {
                _nRead = 0;
            }
        } else if (readResult < 0 && readMode == 0) {
            _nRead--;
        }
    } else if (srsLoadMode == SRS_LOAD_MODE_HOST) {
        int descriptor = sceOpen(path, SRS_OPEN_RDONLY);

        if (descriptor < 0) {
            return -1;
        }
        size = sceLseek(descriptor, 0, SRS_SEEK_END);
        sceLseek(descriptor, 0, SRS_SEEK_SET);
        size = sceRead(descriptor, buffer, size);
        sceClose(descriptor);
        srsMapPendingResources();
    } else if (srsLoadMode == SRS_LOAD_MODE_DEB) {
        size = xglDebFileSize(path);
        if (size <= 0) {
            return -1;
        }
        xglDebFileRead(path, buffer);
        srsMapPendingResources();
    } else if (srsLoadMode == SRS_LOAD_MODE_USB) {
        size = xglDebFileRead(path, buffer);
        if (size > 0) {
            srsMapPendingResources();
        }
    }
    return size;
}

/* Loads a file with mode 0 (the read is not left counted in _nRead). */
int srsLoadFileSync(void *buffer, const char *name)
{
    return fileLoad(buffer, name, 0);
}

const char srsEsdPathFormat[16] = "esd/%s.esd";
static char srsEsdNameBuffer[256];

/* Returns esd/<stem>.esd for a positive effect number, in srsEsdNameBuffer. */
char *srsGetEffectName(int effectNo)
{
    char stem[256];
    char *name;

    name = 0;
    if (effectNo > 0) {
        srsFormatEffectStem(effectNo, stem, 1);
        sprintf(srsEsdNameBuffer, srsEsdPathFormat, stem);
        name = srsEsdNameBuffer;
    }
    return name;
}

const char srsEspPathFormat[16] = "esp/%s.esp";
static char srsEspNameBuffer[256];

/* Returns esp/<stem>.esp for a positive effect number, in srsEspNameBuffer. */
char *srsGetEspName(int effectNo)
{
    char stem[256];

    if (effectNo <= 0) {
        return 0;
    }
    srsFormatEffectStem(effectNo, stem, 0);
    sprintf(srsEspNameBuffer, srsEspPathFormat, stem);
    return srsEspNameBuffer;
}

/* Maps a kind-0 key whose index is 300..340 to slot (low nibble - 1); any
 * other key gives 100. */
int srsGetEffectSlot(int effectKey)
{
    int effectIndex;
    int slot;

    effectIndex = effectKey >> 12;
    slot = 100;
    if (((effectKey >> 28) & 3) == 0 && (unsigned int) ((effectIndex & 0x3FF) - 300) < 41) {
        slot = (effectKey & 0xF) - 1;
    }
    return slot;
}

void srsInitCdRead(void)
{
    _nRead = 0;
}

/* Returns _nRead, the number of CD reads still counted. */
int srsLeaveCdRead(void)
{
    return _nRead;
}

/* Read-completion callback for xglCdReadFile. Code 4 finishes a read: _nRead
 * drops (never below 0) and sresDataMapping runs when none is left. Codes -1
 * and -2 drop _nRead the same way. */
void srsCdReadCallback(int readStatus)
{
    int remaining;

    switch (readStatus) {
    case SRS_CD_READ_COMPLETE:
        remaining = _nRead - 1;
        if (remaining < 0) {
            remaining = 0;
        }
        _nRead = remaining;
        if (remaining == 0) {
            srsMapPendingResources();
        }
        break;
    case -2:
    case -1:
        remaining = _nRead - 1;
        if (remaining >= 0) {
            goto store;
        }
        _nRead = 0;
        break;
    case SRS_CD_READ_RETRY:
        remaining = _nRead - 1;
    store:
        _nRead = remaining;
        break;
    case 0:
    case 1:
    case 2:
    case 3:
        break;
    }
}

int srsLoadEffectData(void *buffer, int effectNo)
{
    char *name;

    name = srsGetEffectName(effectNo);
    if (name == 0) {
        return -1;
    }
    return fileLoad(buffer, name, 1);
}

/* local view: unsized byte array; the sized SrsTable cannot be passed to memset here and would change the symbol size. */
extern unsigned char srsRecordTable[];
#define SRS_RECORD_TABLE_SIZE 0x658  /* 58 key records of 0x1C bytes */

/* Clears the 58-record key table srsRecordTable (0x1C-byte records) and srsEffectStamp. */
void srsResetKeyTable(void)
{
    srsEffectStamp = 0;
    memset(srsRecordTable, 0, SRS_RECORD_TABLE_SIZE);
}

unsigned char srsRecordTable[12] = { 0 };
int D_005E66DC = 0;
