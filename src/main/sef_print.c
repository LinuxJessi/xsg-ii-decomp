#include "common.h"
#include "shared.h"

extern u8 *buffer;

void tracePrint(const char *format, ...);

#define va_start(ap, last) ((ap) = (va_list)__builtin_next_arg(last) - (8 - __builtin_args_info(2)) * 8)
#define va_end(ap) ((void)0)

typedef char *va_list;

extern const char tracePrintFormat[];
extern int printf(const char *format, ...);
extern int vsprintf(char *buffer, const char *format, va_list args);
extern const char tracePrintFormat[8];

INCLUDE_ASM("asm/main/nonmatchings/sef_print", func_00251E30);

void tracePrint(const char *format, ...) {
    va_list args;
    char buffer[0x100];

    va_start(args, format);
    vsprintf(buffer, format, args);
    printf(tracePrintFormat, buffer);
    va_end(args);
}

INCLUDE_ASM("asm/main/nonmatchings/sef_print", func_00251F30);
