#include "common.h"
#include "main/tu167.h"

INCLUDE_ASM("asm/main/nonmatchings/tu167", func_00260D38);

/* One effect record (the table of 0xA70-byte records that tu168.c walks). Partial layout: only the fields
 * the setters below write are named; the gaps stay unmodeled. */
typedef struct SefEffectRecord {
    unsigned char unmodeled_00[0x44];
    int flags;                  /* 0x44: flag word; sefSetRecordOwnerMarked sets 0x400 */
    unsigned char unmodeled_48[0x50 - 0x48];
    int owner;                  /* 0x50: owner id, compared with the caller's id by tu168.c */
    unsigned char unmodeled_54[0x5C - 0x54];
    int values[3];              /* 0x5C: set by sefSetRecordValues */
} SefEffectRecord;

int func_00260D38();

/* Maps an effect id to its canonical key: the alias table of func_00260D38 first, then a band of
 * ids folded onto 16-aligned values plus 1. srsLoadEffectSlot and the spawn functions use the key. */
int sefCanonicalEffectId(void) {
    int effectId;

    effectId = func_00260D38();
    if ((unsigned int) (effectId + 0xEFF37C7B) < 0x27U) {
        effectId = 0x100C8384;
    } else if ((unsigned int) (effectId + 0xFFED3FFF) <= 0x283E6U) {
        effectId = (effectId & ~0xF) + 1;
    }
    return effectId;
}

INCLUDE_ASM("asm/main/nonmatchings/tu167", func_00260E20);

int func_00260E20(int, int, int);

int sefStartEffectKeyDefault(int first, int second) {
    return func_00260E20(first, second, 0);
}

int srsLoadEspFile(int, int);
int srsEffectKeyFromName(signed char *);

/* Starts the effect named by the text code: decodes it to a key, loads the esp file when the code begins
 * with 'e', then starts the key through func_00260E20 in mode 1. */
void sefStartEffectByCode(int first, signed char *text) {
    int effectKey;

    effectKey = srsEffectKeyFromName(text);
    if ((first != 0) && (effectKey != 0) && (text != 0)) {
        if (*text == 0x65) {
            srsLoadEspFile(0, 1);
        }
    }
    func_00260E20(first, effectKey, 1);
}

INCLUDE_ASM("asm/main/nonmatchings/tu167", sefSpawnEffectRecord);

void sefSetRecordOwner(SefEffectRecord *record, int owner) {
    if (record != 0) {
        record->owner = owner;
    }
}

void sefSetRecordOwnerMarked(SefEffectRecord *record, int owner) {
    if (record != 0) {
        record->owner = owner;
        record->flags |= 0x400;
    }
}

void sefSetRecordValues(SefEffectRecord *record, int first, int second, int third) {
    if (record != 0) {
        record->values[0] = first;
        record->values[1] = second;
        record->values[2] = third;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu167", sefSetRecordBlock);

int sefSpawnEffectRecord(int, int, int, int, int);

/* Creates the record for an effect id; the first argument is the id itself. */
int sefSpawnEffectRecordById(int effectId, int second, int third) {
    return sefSpawnEffectRecord(effectId, second, third, 0, -1);
}

/* Decodes the effect code in text, then creates the record for it. */
int sefSpawnEffectRecordByCode(signed char *text, int first, int second, int third) {
    int effectId;

    effectId = srsEffectKeyFromName(text);
    return sefSpawnEffectRecord(effectId, first, second, third, -1);
}

INCLUDE_ASM("asm/main/nonmatchings/tu167", sefSpawnOwnedEffect);
