#include "common.h"
#include "main/tu096.h"
#include "main/tu097.h"
#include "main/tu107.h"

INCLUDE_ASM("asm/main/nonmatchings/tu107", msgBufferAppend);

INCLUDE_ASM("asm/main/nonmatchings/tu107", choiceInit);

/* One pad record (PadData[0..1], stride 0x68). The direction halfwords match
 * main/tu010's MenuPadRecord; only the members this TU reads are named. */
typedef struct PadRecord {
    unsigned char unmodeled_00[0x2a];
    unsigned short buttons_2a;
    unsigned char unmodeled_2c[0x06];
    unsigned short direction_pressed;
    unsigned short direction_repeated;
    unsigned char unmodeled_36[0x32];
} PadRecord;

extern PadRecord PadData[2];

/* The two-choice (yes/no) prompt at main 0x006a57d8. +0x04 is the row the highlight is placed at (choiceUpdate
 * passes it to the placement call); +0x38 is the selected option (0 or 1, 2 once closed); the words at +0x3C
 * (msg buffer), +0x40 and +0x44 are the prompt text and the two option labels drawn by choiceDraw. */
typedef struct ChoiceState {
    unsigned char unmodeled_00[0x04];
    int id;
    unsigned char unmodeled_08[0x30];
    unsigned char state;
} ChoiceState;

extern ChoiceState choiceState;

/* The countdown object at main 0x006a5820. Its +0x04 word is the one
 * D_006A5824 names; main 0x00228438 reaches the record through that label.
 * countdownInit sets count, countdownTick counts it down (a pad button zeroes it), countdownDraw draws
 * value and text. */
typedef struct CountdownState {
    int count;
    int value;
    unsigned char unmodeled_08[0x18];
    unsigned char *text;
} CountdownState;

extern CountdownState countdownState;
extern int D_006A5824[];
extern int choiceFirstOptionText[];

/* Sets the text of the first option of the choice prompt (choiceState +0x40); choiceDraw draws it at x 0xB4. */
void choiceSetFirstOptionText(int text)
{
    choiceFirstOptionText[0] = text;
}

extern int choiceSecondOptionText[];

/* Sets the text of the second option of the choice prompt (choiceState +0x44); choiceDraw draws it at x 0x12C. */
void choiceSetSecondOptionText(int text)
{
    choiceSecondOptionText[0] = text;
}

INCLUDE_ASM("asm/main/nonmatchings/tu107", func_002280E0);

void seVoiceStartDefault(int);

int choiceUpdate(void)
{
    if (choiceState.state < 2) {
        if (PadData[0].direction_repeated & 0xA000) {
            seVoiceStartDefault(0x01000001);
            choiceState.state ^= 1;
            fontTrackSetTarget(&choiceState, (signed char) choiceState.state * 0x78 + 0xA0, choiceState.id);
        }
        if (PadData[0].direction_pressed & 0x40) {
            choiceState.state = 2;
            seVoiceStartDefault(0x01000008);
            return 2;
        }
        if (PadData[0].direction_pressed & 0x20) {
            choiceState.state |= 0x10;
            seVoiceStartDefault(0x01000002);
            return 1;
        }
    }
    return 0;
}

INCLUDE_ASM("asm/main/nonmatchings/tu107", choiceDraw);

INCLUDE_ASM("asm/main/nonmatchings/tu107", func_00228340);

INCLUDE_ASM("asm/main/nonmatchings/tu107", countdownInit);

int countdownTick(void)
{
    if (countdownState.count > 0) {
        countdownState.count = countdownState.count - 1;
    }
    if (PadData[0].buttons_2a & 0x60) {
        if (countdownState.count >= -1) {
            seVoiceStartDefault(0x01000002);
            countdownState.count = 0;
        }
    }
    return countdownState.count;
}

void func_0020CD50(void *);
void textDrawSegment(int, int, void *, int);
void func_0021DE60(int, int, int, int);
void func_0021E338(int, int, int, int);
void xglFontFlush(void);
extern unsigned char msgStyleInit[];
extern unsigned char D_0068AC80[];

void countdownDraw(void)
{
    CountdownState *countdown;

    /* The record starts four bytes before D_006A5824, the word that the
     * two callees below receive by address. */
    countdown = (CountdownState *) ((unsigned char *) D_006A5824 - 4);
    func_0020CD50(D_006A5824);
    func_0020CD50(D_006A5824);
    fontObjSetOpacity(countdown->value);
    func_0021DE60(0x50, 0xB4, 0x160, 0x50);
    func_0021E338(0x54, 0xB8, 0x158, 0x48);
    textDrawSegment(0, 0, msgStyleInit, 0);
    textDrawSegment(0, 0, D_0068AC80, 0);
    if (countdown->text != 0) {
        textDrawSegment(0x60, 0xC4, countdown->text, 4);
    }
    fontObjResetOpacity();
    xglFontFlush();
    fontObjResetAndBuild();
}

extern signed char textCharSpacing;

/* Sets the extra advance added to every glyph of text. */
void textSetCharSpacing(signed char spacing)
{
    textCharSpacing = spacing;
}

/* Restores the default glyph spacing of 4. */
void textResetCharSpacing(void)
{
    textCharSpacing = 4;
}
