#include "common.h"
#include "shared.h"
#include "main/srs.h"
#include "main/sv_get_image_item.h"
#include "main/xgl_cd.h"

extern char *strcat(char *destination, const char *source);
extern u8 *buffer;


extern int _nRead;

#define SRS_PATH_LENGTH 256          /* resolved file path and view path */

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

extern void sresDataMapping(void);
extern int sceOpen(const char *path, int flags, ...);
extern int sceLseek(int descriptor, int offset, int whence);
extern int sceRead(int descriptor, void *buffer, int bytes);
extern int sceClose(int descriptor);
extern int func_00A103C8(const char *path, void *buffer);
extern void srsCdReadCallback(int code);
extern char srsViewPath[SRS_PATH_LENGTH];

int srsMakeFileName(char *buffer, const char *fileName);

void srsCdReadCallback(int code);
void sresDataMapping(void);

/* Returns 1 when the effect key is of kind 1 (effect image) with an index in
 * 300..449, else 0. */
INCLUDE_ASM("asm/main/nonmatchings/tu179", srsIsEffectImageKey);

int srsFileLoad(void *buffer, const char *name, int mode)
{
    return fileLoad(buffer, name, mode);
}

/* Parses a nine-character effect name into an effect key: a two-letter prefix
 * gives the kind (ce is 1, ee is 2, anything else 0), three decimal digits the
 * index and three the sub-number. Returns 0 when the index field comes out
 * negative. */
INCLUDE_ASM("asm/main/nonmatchings/tu179", srsEffectKeyFromName);
