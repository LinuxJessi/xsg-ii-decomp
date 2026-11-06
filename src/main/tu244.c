#include "common.h"
#include "main/tu244.h"

INCLUDE_ASM("asm/main/nonmatchings/tu244", func_00286F50);

/* One of the three control points that grControlPointBlend blends (set by func_00286F50). */
struct ControlPoint {
    float x;
    float y;
};

extern struct ControlPoint grControlPoints[];

/* A sized entry whose on-screen origin is centred by sizedRecordCenterX and sizedRecordCenterY. */
struct SizedRecord {
    unsigned char unmodeled_00[0xC];
    int stored_value;
    int size_x;
    int size_y;
};

/*
 * Position at `fraction` (clamped to 1) along the three control points: weights are
 * (1 - f)^3, 1 - w0 - w2 and f^3. The result is the truncated screen position.
 */
void grControlPointBlend(int *out_x, int *out_y, float fraction) {
    float weights[3];
    float complement;
    float sum_x;
    float sum_y;
    int i;

    if (fraction > 1.0f) {
        fraction = 1.0f;
    }
    complement = 1.0f - fraction;
    weights[2] = fraction * fraction * fraction;
    weights[0] = complement * complement * complement;
    weights[1] = (1.0f - weights[0]) - weights[2];

    sum_x = 0.0f;
    sum_y = 0.0f;
    for (i = 0; i < 3; i++) {
        sum_x += grControlPoints[i].x * weights[i];
        sum_y += grControlPoints[i].y * weights[i];
    }
    *out_x = (int) sum_x;
    *out_y = (int) sum_y;
}

/* X origin that centres the record on a 512-pixel-wide screen. */
int sizedRecordCenterX(struct SizedRecord *record) {
    return 0x100 - (record->size_x >> 1);
}

/* Y origin that centres the record on a 448-line screen. */
int sizedRecordCenterY(struct SizedRecord *record) {
    return 0xE0 - (record->size_y >> 1);
}

/* True when text starts with a value conversion, %d or %s. */
int textIsValueEscape(signed char *text) {
    int conversion;

    if (text[0] != '%') {
        return 0;
    }
    conversion = text[1];
    if (conversion == 'd') {
        return 1;
    }
    return conversion == 's';
}

/* True when the character at text is a space. */
int textIsSpace(signed char *text) {
    return *text == ' ';
}

/* Stores value in the record's stored_value field. */
void sizedRecordSetValue(struct SizedRecord *record, int value) {
    record->stored_value = value;
}

INCLUDE_ASM("asm/main/nonmatchings/tu244", textIsLineBreak);

INCLUDE_ASM("asm/main/nonmatchings/tu244", textSkipLineBreak);
