#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tu233", talkStashPrefix);

/*
 * The talk text box as tu233 reads it (the same object as tu232's
 * TalkTextBox): the text buffer pointer at +0x2c and the speaker name pointer
 * at +0x60, where tu232 has its second_pass word.
 */
typedef struct TalkBoxContext {
    unsigned char unmodeled_00[0x2c];
    unsigned char *text;                   /* two leading bytes are stashed around the print */
    unsigned char unmodeled_30[0x30];
    char *speaker_name;                    /* text up to NUL or ']' is the name plate */
} TalkBoxContext;

/* Two leading bytes of the text, stashed into talkPrefixStash by talkStashPrefix. */
extern signed char talkPrefixStash[2];

/* Puts the stashed leading bytes back into the text; the second byte only when it is non-zero. */
void talkRestorePrefix(TalkBoxContext *box)
{
    unsigned char *text;

    text = box->text;
    text[0] = talkPrefixStash[0];
    if (talkPrefixStash[1] != 0) {
        text[1] = talkPrefixStash[1];
    }
}

/* Copies the speaker name into buffer up to NUL or ']' and terminates it. */
void talkCopySpeakerName(unsigned char *buffer, TalkBoxContext *box)
{
    char *src;
    int ch;
    int next;
    unsigned char out;

    src = box->speaker_name;
    if (src != 0) {
        ch = *src;
        out = *src;
        if (ch != 0 && ch != 0x5D) {
            do {
                *buffer = out;
                buffer += 1;
                src += 1;
                next = *src;
                out = *src;
            } while (next != 0 && next != 0x5D);
        }
        *buffer = 0;
    }
}
