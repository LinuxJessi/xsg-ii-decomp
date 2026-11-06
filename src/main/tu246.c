#include "common.h"
#include "main/xgl_font.h"

extern int xglFontWidthBase;
extern int xglFontWidthReady;
extern int xglFontWidthSum;
extern int xglFontWidthIndex;
extern unsigned char **xglFontWidthValues;
extern unsigned char D_0068D590[];
extern unsigned char xglFontWidthFormat[];
extern unsigned char xglFontPropTag[];
extern unsigned char xglFontWidthPlaceholder[];
extern unsigned char xglFontWidthValueFormat[];
void func_002AA5D8();
void xglFontWidthMeasureTemplate(void);

/*
 * Starts a width-summing run over the value list `values`: the list is stored, the
 * running sum and index are reset, and the width of the bare template is measured once
 * (xglFontWidthMeasureTemplate). Returns 1 once the template width is known.
 */
int xglFontWidthBegin(unsigned char **values)
{
    int ready;

    ready = xglFontWidthReady;
    xglFontWidthValues = values;
    xglFontWidthSum = 0;
    xglFontWidthIndex = 0;
    if (ready == 0)
    {
        xglFontWidthMeasureTemplate();
        ready = 1;
        xglFontWidthReady = 1;
    }
    return ready;
}

/*
 * Measures the template "<ruby prefix>$prop1;%s" with the value placeholder left literal;
 * every later item is measured against this base width.
 */
void xglFontWidthMeasureTemplate(void)
{
    int template_text[20];

    func_002AA5D8(template_text, xglFontWidthFormat, D_0068D590, xglFontPropTag, xglFontWidthPlaceholder);
    xglFontWidthBase = xglFontGetStringWidth(template_text);
}

/*
 * Formats the current list entry as "%d", substitutes it into the template, and adds the
 * width beyond the template base to the running sum; then advances the index.
 */
void xglFontWidthAddValue(void)
{
    int value_text[4];
    int item_text[20];
    int width;

    if (xglFontWidthValues != 0)
    {
        func_002AA5D8(value_text, xglFontWidthValueFormat, xglFontWidthValues[xglFontWidthIndex]);
        func_002AA5D8(item_text, xglFontWidthFormat, D_0068D590, xglFontPropTag, value_text);
        width = xglFontGetStringWidth(item_text);
        xglFontWidthSum = xglFontWidthSum + (width - xglFontWidthBase);
        xglFontWidthIndex = xglFontWidthIndex + 1;
    }
}

void xglFontWidthReset(void)
{
    xglFontWidthSum = 0;
}

/* Sum of the widths added since the last reset. */
int xglFontWidthGetSum(void)
{
    return xglFontWidthSum;
}
