#include "common.h"
#include "main/TiWindBase.h"
#include "main/tu182.h"

/* One wind slot: the effect key and sub-key that own it. Four slots, the same
 * count as the directional and point wind tables of TiWindBase. */
typedef struct TiWindSlot {
    int key;
    unsigned short subkey;
    short aux;
} TiWindSlot;

extern TiWindSlot tiWindSlots[];

/* Stores key, subkey and aux in the first empty wind slot (key 0) and returns
 * its index, or -1 when all four slots are in use. */
int tiWindAcquireSlot(int key, unsigned short subkey, int aux)
{
    TiWindSlot *slot;
    int i;

    slot = tiWindSlots;
    for (i = 0; i < 4; i++) {
        if (slot->key == 0) {
            slot->key = key;
            slot->subkey = subkey;
            slot->aux = aux;
            return i;
        }
        slot++;
    }
    return -1;
}

void tiWindDirSetEnabled(int, int);

/* Releases every wind slot owned by key and subkey (key 0 matches all slots):
 * each one is disabled and cleared. */
void tiWindReleaseSlots(int key, int subkey)
{
    TiWindSlot *slot;
    int i;

    slot = tiWindSlots;
    for (i = 0; i < 4; i++) {
        if (key == 0 || (slot->key == key && slot->subkey == (subkey & 0xFFFF))) {
            tiWindDirSetEnabled(i, 0);
            tiWindPointSetEnabled(i, 0);
            slot->key = 0;
            slot->subkey = 0;
            slot->aux = 0;
        }
        slot++;
    }
}

/* Index of the wind slot whose key and subkey match, or -1. */
int tiWindFindSlot(int key, unsigned short subkey)
{
    TiWindSlot *slot;
    int i;

    slot = tiWindSlots;
    for (i = 0; i < 4; i++) {
        if (slot->key == key && slot->subkey == subkey) {
            return i;
        }
        slot++;
    }
    return -1;
}

void *memset(void *s, int c, unsigned int n);

/* Clears the first wind slot only (sizeof of one entry). */
void tiWindClearFirstSlot(void)
{
    memset(tiWindSlots, 0, sizeof(tiWindSlots[0]));
}

/* Releases all wind slots and clears the first one. */
void tiWindResetSlots(void)
{
    tiWindReleaseSlots(0, 0);
    tiWindClearFirstSlot();
}

INCLUDE_ASM("asm/main/nonmatchings/tu182", tiWindUpdateKeyframes);
