#include "common.h"
#include "main/tu234.h"

/*
 * The text box the talk window draws. Only the members these functions read
 * are named; the layout is shared with tu233 (speaker name, text buffer)
 * and tu234 (func_00283BC8, func_00283C30, textLastRowY read box members).
 */
typedef struct TalkBoxOptions {
    int visible;                           /* panel height; 0 draws nothing */
    unsigned char unmodeled_04[0x34];
    /*
     * +0x60 in the box (options + 0x38). tu233's talkCopySpeakerName reads the same
     * word as the speaker name pointer (char *): non-zero means a speaker name
     * is present and the name plate is drawn. The integer type here is the
     * tu232 view; the integrator reconciles it with tu233's field.
     */
    int second_pass;
    unsigned char payload[0x14];
    int bias;
} TalkBoxOptions;

typedef struct TalkTextBox {
    unsigned char unmodeled_00[0x4];
    int x;
    int y;
    int line;                              /* index passed to xglFontPrintExtFunc */
    unsigned char unmodeled_10[0x18];
    TalkBoxOptions options;
} TalkTextBox;

int func_00283BC8(TalkTextBox *box);

/* Returns func_00283BC8(box) plus the box's bias. */
int talkGetBoxLayoutX(TalkTextBox *box)
{
    TalkBoxOptions *options;

    options = &box->options;
    return func_00283BC8(box) + options->bias;
}

void func_00283C30(void);

/* Forwards to func_00283C30; the caller's box pointer passes through in $4. */
void talkGetBoxLayoutY(void)
{
    func_00283C30();
}

void talkStashPrefix(void *obj);
void talkRestorePrefix(void *obj);
void talkCopySpeakerName(unsigned char *buffer, void *obj);
void fontPrintIntRow(int x, int y, int width, int height, void *data);
int textLastRowY(void *obj);
void xglFontFlush(void);
void xglFontPrintExtFunc(int x, void *callback, void *arg);
extern unsigned char func_00283C78[];

/*
 * Draws one talk text box: when visible, the speaker name plate first (the
 * second pass, when a speaker name is present), then the text, the panel
 * behind it, and a flush of the font batch.
 */
void talkDrawTextBox(TalkTextBox *box)
{
    TalkBoxOptions *options;
    unsigned int buffer[0x10];
    int width;

    options = &box->options;
    if (options->visible != 0)
    {
        if (options->second_pass != 0)
        {
            talkCopySpeakerName(&buffer[0], box);
            xglFontPrintExtFunc(box->line, func_00283C78, (void *) 1);
            drawShadowedText(box->x + 0x20, box->y - 0x12, box->line + 7, &buffer[0]);
        }
        xglFontPrintExtFunc(box->line, func_00283C78, box);
        width = textLastRowY(box);
        talkStashPrefix(box);
        fontPrintIntRow(box->x + 0x20, width, box->line + 7, options->visible, options->payload);
        talkRestorePrefix(box);
        xglFontFlush();
        xglFontPrintExtFunc(0, func_00283C78, (void *) 1);
    }
}
