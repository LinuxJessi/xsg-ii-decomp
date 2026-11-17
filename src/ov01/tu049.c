#include "common.h"
#include "shared.h"
#include "ov01/tu048.h"
#include "ov01/tu049.h"

/* GIF packet words are written as doubleword descriptors by the openers and
 * as 32-bit lanes by the vertex writers. Both are the same packet storage. */
typedef union GrGifWord {
    unsigned long long bits;
    unsigned int lanes[2];
} GrGifWord;

typedef struct GrGifOpenTag {
    GrGifWord control;
    GrGifWord register_id;
} GrGifOpenTag;

static inline void grGifOpenRegister(GrPacket *packet, unsigned long long register_id)
{
    GrGifOpenTag *tags = packet->data;
    tags[packet->count].register_id.bits = register_id;
}

void GS_REG_SET(GrPacket *packet, long long register_id, long long control)
{
    GrGifOpenTag *tags;
    int *count;
    int tag_index;

    grGifOpenRegister(packet, register_id);
    count = &packet->count;
    tags = packet->data;
    tag_index = *count;
    tags[tag_index].control.bits = control;
    *count = tag_index + 1;
}

/* Menu panel rows: MENU_PANEL_ROW_COUNT slots, a slot holds the row's y
 * (MENU_PANEL_ROW_Y0 + MENU_PANEL_ROW_STEP * index) while the row is taken, 0
 * when free. menuPanelRowAlloc takes one, menuPanelRowFree frees it. */
#define MENU_PANEL_ROW_COUNT 10
#define MENU_PANEL_ROW_Y0 100
#define MENU_PANEL_ROW_STEP 20

extern int menuPanelRows[MENU_PANEL_ROW_COUNT];
extern int screenFadeTask;
extern int screenFillTask;
void *memset(void *, int, unsigned int);

void menuPanelRowsReset(void)
{
    memset(menuPanelRows, 0, sizeof(menuPanelRows));
    screenFadeTask = 0;
    screenFillTask = 0;
}

/* Takes the first free row and returns its y, or 0 when all rows are taken. */
int menuPanelRowAlloc(void)
{
    int i;
    int y;

    for (i = 0, y = MENU_PANEL_ROW_Y0; i < MENU_PANEL_ROW_COUNT; i++) {
        if (menuPanelRows[i] == 0) {
            menuPanelRows[i] = y;
            return y;
        }
        y += MENU_PANEL_ROW_STEP;
    }
    return 0;
}

/* Frees the row taken at y; returns 1, or 0 when no row has that y. */
int menuPanelRowFree(int y)
{
    int i;

    for (i = 0; i < MENU_PANEL_ROW_COUNT; i++) {
        if (menuPanelRows[i] == y) {
            menuPanelRows[i] = 0;
            return 1;
        }
    }
    return 0;
}

/* A menu object from objSlotAlloc: +0x04 a state word, +0x08 the draw callback
 * (the draw slot of the other objects, see arrowCreate). */
struct MenuTexFlushTask {
    char unmodeled_000[4];
    int state;                  /* +0x04 */
    void (*draw)(void);         /* +0x08 */
};

void *objSlotAlloc(void);
void menuTexFlush(void);
void menuTexFlushDraw(void);
extern struct MenuTexFlushTask *menuTexFlushTask;

/* Starts the texture-flush task: the object's draw is menuTexFlushDraw, and the
 * object is kept in menuTexFlushTask (nothing else in the overlay reads it). */
void menuTexFlushTaskStart(void)
{
    struct MenuTexFlushTask *task;

    task = objSlotAlloc();
    if (task != 0) {
        menuTexFlushTask = task;
        task->draw = menuTexFlushDraw;
        task->state = 0;
    }
}

/* Draw callback of the texture-flush task: flushes the menu texture. */
void menuTexFlushDraw(void)
{
    menuTexFlush();
}

/* Creates one stat record per unit of the unit table (record pointers in menuStatRecords):
 * origin x = 16 + 167 * created index, y = 12.0, +0x04 the step, +0x08 the draw. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu049", menuStatRecordsCreate);

/* Step of a stat record: eases three displayed floats (+0x98, +0x9C, +0xA0) toward
 * the unit's stat fields (at +0x94 -> +0x144), snapping within 0.5 and clamped. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu049", menuStatGaugeStep);

void menuStatRecordDraw(int, GrPacket *);

/* Draw callback of a stat record (record + 0x08). `first` is the record pointer
 * passed through an int parameter: menuStatRecordDraw reads its origin at +0x10 and
 * its unit at +0x94. Retyping it needs its owner. */
void menuStatRecordSubmit(int first)
{
    GrPacket packet;

    grGpInit(&packet);
    menuStatRecordDraw(first, &packet);
    grPacketSend(&packet);
}

/* Draws one stat record at its origin: sprite groups through grPutSprOpen/grPutSprClose
 * and the menu stat sprites and numbers. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu049", menuStatRecordDraw);
