#include "common.h"
#include "main/tu229.h"
#include "main/tu242.h"
#include "main/tu243.h"

typedef struct MsgEntryPayload MsgEntryPayload;

/*
 * The 0x90-byte payload of one message window entry (main/tu216, MsgEntry.payload):
 * msgWindowLayoutText passes window[current].payload to msgEntryFeedText. Other TUs view the
 * same bytes under their own names (TalkWork in tu228, InterpObject in tu227,
 * TimecodeRecord in tu241). Only the fields the functions below touch are modelled;
 * the payload extent beyond the last named member is not evidenced here.
 */
struct MsgEntryPayload {
    unsigned char unmodeled_00[0x08];
    int buffer;                        /* 0x08 */
    unsigned char unmodeled_0c[0x08];  /* 0x0C..0x13 */
    int size;                          /* 0x14 */
    unsigned char unmodeled_18[0x04];  /* 0x18..0x1B */
    int state;                         /* 0x1C */
    unsigned char unmodeled_20[0x1c];  /* 0x20..0x3B */
    int saved_state;                   /* 0x3C */
    unsigned char unmodeled_40[0x14];  /* 0x40..0x53 */
    int setup_result;                  /* 0x54 */
    int attach_result;                 /* 0x58 */
    int line_count;                    /* 0x5C: textLineCount(entry) of the last input */
    int lookup_result;                 /* 0x60 */
};


void func_0027DCB0(void *object) {
    timecodeClearFlag80(object);
}

void timecodeSplit(void);

void func_0027DCC8(void) {
    timecodeSplit();
}


void timecodeSetStoredValue(void *object, int value) {
    func_00286228(object, value);
}

void func_00286240(void *record, int value);

void timecodeSetWideValue(void *record, int value) {
    func_00286240(record, value);
}

void func_0027DD28(int first, int second, int flag);

void msgChoiceShowSingle(void *first, int second) {
    func_0027DD28(first, second, 0);
}

INCLUDE_ASM("asm/main/nonmatchings/tu225", func_0027DD28);

INCLUDE_ASM("asm/main/nonmatchings/tu225", func_0027DE28);

INCLUDE_ASM("asm/main/nonmatchings/tu225", func_0027DEA0);

/* Nonzero when any bit of mask is set in *flags; msgChoicePoll passes the choice object's first word. */
int msgChoiceTestFlags(int *flags, int mask) {
    return (*flags & mask) != 0;
}

INCLUDE_ASM("asm/main/nonmatchings/tu225", func_0027DF58);

INCLUDE_ASM("asm/main/nonmatchings/tu225", func_0027DFD8);

int func_0027E0C0(int start, int end);

/* In state 1: records the buffer setup of the record (the usable part of the buffer between two 0x20-byte ends). */
void func_0027E048(MsgEntryPayload *record, int first_value, int second_value, int value, int fourth_value) {
    int state;
    int buffer;

    state = record->state;
    if (state == 1) {
        msgSlotSetGeometry(record, first_value, second_value, value, fourth_value);
        buffer = record->buffer;
        record->saved_state = state;
        record->setup_result = func_0027E0C0(buffer + 0x20, buffer + record->size - 0x20);
        record->attach_result = func_0027E7F8(value);
    }
}


int func_0027E0C0(int start, int end) {
    return func_00286ED0(start, end, 30);
}

int textLineCount(void *record);
int textSkipBracketTag(int input);
int textTagBody(int input);
void textFormatWrapped(void *record, int mapped);

/*
 * In state 1: feeds one text input into the entry. A nonzero tag body is kept in
 * lookup_result, the input with its bracket tag skipped is formatted into the
 * entry, and the resulting line count is stored in line_count.
 */
void msgEntryFeedText(MsgEntryPayload *record, int input) {
    int lookup;
    int mapped;

    if (record->state == 1) {
        lookup = textTagBody(input);
        if (lookup != 0) {
            record->lookup_result = lookup;
        }
        mapped = textSkipBracketTag(input);
        textFormatWrapped(record, mapped);
        record->line_count = textLineCount(record);
    }
}
