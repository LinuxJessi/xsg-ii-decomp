#ifndef INCLUDE_MAIN_TU124_H
#define INCLUDE_MAIN_TU124_H

/* Functions of src/main/tu124.c that other files call. */

/* One text entry: an opening box with a list of pages (the same layout as tu125 reads). */
typedef struct TextEntry {
    int x;
    int y;
    int width;
    int height;
    unsigned char **pages;
    int page;
    unsigned char tick;        /* +0x18 opening step, 0..4 */
    unsigned char state;       /* +0x19 TextState: 0 free, 1 open, 2 opening, 3 closing (removed) */
    unsigned char style_index; /* +0x1A */
    unsigned char unmodeled_1b;
    union {
        unsigned int style_value;  /* +0x1C textStyleTable word of style_index */
        char *title;               /* +0x1C box title (tu125), NULL for none */
    } word_1c;
    unsigned char count;       /* +0x20 voice cues started (tu124) or the page-cut count (tu125) */
    unsigned char atLastPage;  /* +0x21 set when the entry reaches its last page */
    unsigned char unmodeled_22[2];
} TextEntry;

int choiceMenuOpen(unsigned int index, int x, int y, int width, int height, int style_index, unsigned char unit_size,
                   unsigned int flags, unsigned char **items, short *values, unsigned char *itemFlags);
void choiceMenuSetActive(int index, int enable);
TextEntry *textEntryAlloc(int x, int y, int width, int height, signed char style_index, unsigned char **pages);
void objRemove(TextEntry *entry);
void choiceMenuClose(int index);
void choiceMenuResetAll(void);
void textEntryPoolReset(void);

#endif /* INCLUDE_MAIN_TU124_H */
