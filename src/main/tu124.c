#include "common.h"
#include "main/tu124.h"

INCLUDE_ASM("asm/main/nonmatchings/tu124", choiceMenuResetAll);

INCLUDE_ASM("asm/main/nonmatchings/tu124", choiceMenuOpen);

INCLUDE_ASM("asm/main/nonmatchings/tu124", choiceMenuOpenFromRequest);

#include "shared.h"

/* 92-byte choice menu slot (two slots). flags bit 0 is the active bit set by choiceMenuSetActive; kind is
 * 0 free, 1 open, 2 opening, 3 closing (choiceMenuClose stores 3); phase counts the four opening steps. */
typedef struct ChoiceMenuSlot {
    u32 flags;
    u8 phase;
    u8 kind;
    u8 unmodeled_06[86];
} ChoiceMenuSlot;

extern ChoiceMenuSlot D_006ABDAC[];

/* Sets or clears the active bit (flags bit 0) of choice menu slot index. */
void choiceMenuSetActive(int index, int enable)
{
    if (enable != 0) {
        D_006ABDAC[index].flags |= 1;
        return;
    }
    D_006ABDAC[index].flags &= ~1;
}

INCLUDE_ASM("asm/main/nonmatchings/tu124", func_00235D78);

INCLUDE_ASM("asm/main/nonmatchings/tu124", choiceMenuIsOpen);

extern ChoiceMenuSlot D_006ABDE0[];

/* Closes choice menu slot index: kind 3 (closing), phase 4, active bit cleared. */
void choiceMenuClose(int index)
{
    D_006ABDE0[index].kind = 3;
    D_006ABDE0[index].phase = 4;
    choiceMenuSetActive(index, 0);
}

INCLUDE_ASM("asm/main/nonmatchings/tu124", choiceMenuShow);

INCLUDE_ASM("asm/main/nonmatchings/tu124", choiceMenuUpdate);

int textEncodeNewlines(unsigned char *text)
{
    unsigned char *cursor;
    unsigned char ch;
    int ch_shifted;

    cursor = text;
    ch = *cursor;
    ch_shifted = ch << 24;
    if (ch != 0) {
        do {
            if (ch_shifted < 0) {
                /* lead byte of a two-byte character: skip its trail byte */
                cursor += 1;
            } else if (ch == 0x5C && cursor[1] == 0x6E) {
                cursor[0] = 0x0A;
                cursor[1] = 0x07;
            }
            cursor += 1;
            ch = *cursor;
            ch_shifted = ch << 24;
        } while (ch != 0);
    }
    return ch_shifted;
}

/* 36-byte text entry of the four-entry pool at textEntryPool. state: 0 free, 1 page advance, 2 ticking in,
 * 3 removed (objRemove stores it) or closing. words[4] is the page pointer list, words[5] its current index;
 * style_index selects a word of textStyleTable, stored at +0x1C. */
typedef struct TextEntry {
    u32 words[6];
    u8 tick;
    u8 state;
    u8 style_index;
    u8 unmodeled_1b[5];
    u8 count;
    u8 status;
    u8 unmodeled_22[2];
} TextEntry;

extern TextEntry textEntryPool[];

/* Clears all four entries of the text entry pool. */
void textEntryPoolReset(void)
{
  int index;

  for (index = 0; index < 4; index++)
  {
    textEntryPool[index].words[3] = 0;
    textEntryPool[index].words[2] = 0;
    textEntryPool[index].words[1] = 0;
    textEntryPool[index].words[0] = 0;
    textEntryPool[index].style_index = 0;
    textEntryPool[index].words[4] = 0;
    textEntryPool[index].words[5] = 0;
    textEntryPool[index].tick = 0;
    textEntryPool[index].state = 0;
  }
}

INCLUDE_ASM("asm/main/nonmatchings/tu124", textEntryAlloc);

INCLUDE_ASM("asm/main/nonmatchings/tu124", objRemove);

void seVoiceStartDefault(int voice);
extern u16 textOptionFlags;

/* Returns the status byte of a text entry; in page-advance state 1, with the option bit 0x20 of textOptionFlags set, it
 * starts the default voice cue 0x01000002 once and counts it in count. */
u8 textEntryPlayVoice(TextEntry *entry)
{
    if (entry == 0) {
        return 0;
    }
    if (entry->count == 0) {
        entry->count = 1;
    }
    if (entry->state == 1 && (textOptionFlags & 0x20)) {
        seVoiceStartDefault(0x01000002);
        entry->count += 1;
    }
    return entry->status;
}
