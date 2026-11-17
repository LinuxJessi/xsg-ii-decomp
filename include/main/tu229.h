#ifndef INCLUDE_MAIN_TU229_H
#define INCLUDE_MAIN_TU229_H

#include "gs.h"
#include "shared.h"
#include "main/tu227.h"

/* Functions of src/main/tu229.c that other files call. */

/* One A+D pair of a packet built in the scratchpad (also used by tu231.c). */
typedef union GsPair {
    struct {
        u64 data;
        u64 reg;
    } ad;
    long qword[2];
} GsPair;

/* Phase bits of MsgSlot.flags that the frame drawers test (same bits as msgWindowUpdate, tu238:
 * 0x2 closing, 0x4 opening, 0x8 open). The two higher bits are named by what their draw step does. */
#define TALK_FLAG_CLOSING 0x2
#define TALK_FLAG_OPENING 0x4
#define TALK_FLAG_OPEN 0x8
#define TALK_FLAG_BOX 0x40     /* talkDrawFramesWithBox also draws the selection box (talkDrawSelectionBox) */
#define TALK_FLAG_FADE 0x400   /* talkDrawFramesWithFade adds the fade frames driven by the item counter */
/* talkWindowDrawFrame draws the page cursor of an active text slot while it is open and no longer
 * revealing text (bit 0x10, INTERP_FLAG_SCAN of tu227.h, clear). */
#define TALK_CURSOR_FLAGS_MASK (TALK_FLAG_OPEN | INTERP_FLAG_SCAN)
#define TALK_CURSOR_FLAGS TALK_FLAG_OPEN

/* Frame modes of talkWindowDrawFrame; 3 is the sized frame of talkWindowFrameRect. */
#define TALK_FRAME_FULL 0
#define TALK_FRAME_OPENING 1
#define TALK_FRAME_CLOSING 2
#define TALK_FRAME_SIZED 3

/* Screen area of the talk frames (a 512x448 display): the last pixel column and row. */
#define TALK_SCREEN_RIGHT 0x1FF
#define TALK_SCREEN_BOTTOM 0x1BF

/* GIF tag of the A+D packets the talk code builds in the scratchpad: GIF_TAG_ONE_AD_EOP (gs.h) with
 * NLOOP 0; the pair count is added to it when the packet is sent. */
#define TALK_GIF_TAG_AD_EOP (GIF_TAG_ONE_AD_EOP - 1)

#include "main/tu242.h"

extern int talkActive;
extern void *talkScriptHandle;

void talkWindowDrawFrame(MsgSlot *window, int mode, float scale);

int textWrapWidth(int value);
int interpCursorFrame(int value);
void talkEndScript(void);
void talkDeactivate(void);
void talkSetActive(void);
void talkStartScriptLoad(void);
void talkLoadOmiyaScript(void);
void talkCloseScript(void);
void talkSetPending(int value);
int talkTakePending(void);
void talkDrawFramesBasic(MsgSlot *slot);
void talkDrawFramesWithCursor(MsgSlot *slot);
void talkDrawFramesWithBox(MsgSlot *slot);
void talkDrawFrameRects(MsgSlot *slot);
void talkDrawFramesWithFade(MsgSlot *slot);
void talkDrawCursorWindow(MsgSlot *slot);

#endif /* INCLUDE_MAIN_TU229_H */
