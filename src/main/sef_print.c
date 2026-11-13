#include "common.h"
#include "shared.h"
#include "main/pad.h"

extern u8 *buffer;

void tracePrint(const char *format, ...);

#define va_start(ap, last) ((ap) = (va_list)__builtin_next_arg(last) - (8 - __builtin_args_info(2)) * 8)
#define va_end(ap) ((void)0)

typedef char *va_list;

extern const char tracePrintFormat[];
extern int printf(const char *format, ...);
extern int vsprintf(char *buffer, const char *format, va_list args);
extern const char tracePrintFormat[8];
extern PadRecord PadData;
/* The reset message resetWaitForPad shows. */
extern const char resetText[];
void xglFontDebugPrintf(int x, int y, const char *format, ...);
void xglRenderClearFrame(void);
void xglSleep(void);

/* Shows the reset message until SELECT and START are held together. */
void resetWaitForPad(void)
{
    while ((PadData.heldAndPressed & (PAD_SELECT | PAD_START)) != (PAD_SELECT | PAD_START)) {
        xglFontDebugPrintf(8, 8, resetText);
        xglRenderClearFrame();
        xglSleep();
    }
}

void tracePrint(const char *format, ...) {
    va_list args;
    char buffer[0x100];

    va_start(args, format);
    vsprintf(buffer, format, args);
    printf(tracePrintFormat, buffer);
    va_end(args);
}

/* Log call sites use this variadic no-op (lightSetLogDump passes a vector, a format and an index): it spills
 * its arguments and returns. */
void logNoOp(const void *data, const char *format, ...)
{
}
