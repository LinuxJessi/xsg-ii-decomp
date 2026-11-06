#include "common.h"
#include "shared.h"
#include "main/tu187.h"

typedef char *va_list;

#define va_start(ap, last) ((ap) = (va_list)__builtin_next_arg(last) - (8 - __builtin_args_info(2)) * 8)

/* Formats into the shared debug buffer MOutputDebugBuffer, then prints it with the variadic arguments. */
void func_002AA5D8(unsigned char *buffer, unsigned char *format, int value);
void func_002AF028(unsigned char *buffer, va_list args);
extern unsigned char MOutputDebugInfoFormat[];
extern unsigned char MOutputDebugBuffer[];

/* Debug message, info colour ("MDMSG:" in cyan). The first argument is the message text (a char *). */
void MOutputDebugString(char *value, ...)
{
    va_list ap;
    va_start(ap, value);

    func_002AA5D8(MOutputDebugBuffer, MOutputDebugInfoFormat, value);
    func_002AF028(MOutputDebugBuffer, ap);
}

extern unsigned char MOutputDebugWarnFormat[];

/* Debug message, warning colour ("MDMSG:" in magenta). The first argument is the message text (a char *). */
void MOutputDebugStringWarn(char *value, ...)
{
    va_list ap;
    va_start(ap, value);

    func_002AA5D8(MOutputDebugBuffer, MOutputDebugWarnFormat, value);
    func_002AF028(MOutputDebugBuffer, ap);
}

/* Mersenne Twister (MT19937): seeding, then the tempered 32-bit output of genrand_int32. */
INCLUDE_ASM("asm/main/nonmatchings/tu187", MMathRandomSeed);

INCLUDE_ASM("asm/main/nonmatchings/tu187", MMathRandomNext);

u32 MMathRandomNext(void);

/* A random integer in [0, 2^31). */
u32 MMathMakeRandomInt(void)
{
    return MMathRandomNext() >> 1;
}

/* A random float in [0, 1): the 32-bit output scaled by 2^-32. */
f32 MMathMakeRandom(void)
{
    u32 bits;

    bits = MMathRandomNext();
    return (f32) bits * 2.3283064e-10f;
}

/* A random float in [-1, 1): the 32-bit output scaled by 2^-31, minus 1. */
f32 MMathMakeRandomSigned(void)
{
    u32 bits;

    bits = MMathRandomNext();
    return ((f32) bits * 4.656613e-10f) - 1.0f;
}

/* A random angle in [0, 2*pi): the 32-bit output scaled by 2*pi / 2^32. */
INCLUDE_ASM("asm/main/nonmatchings/tu187", MMathMakeRandom2PI);

/* A random angle in [-pi, pi). */
INCLUDE_ASM("asm/main/nonmatchings/tu187", MMathMakeRandomPI);

/* Shortest signed difference second - first, wrapped into [-pi, pi]. */
INCLUDE_ASM("asm/main/nonmatchings/tu187", MMathCalcRotNear);

/* Difference second - first wrapped into the other half-turn (|difference| >= pi). */
INCLUDE_ASM("asm/main/nonmatchings/tu187", MMathCalcRotFar);
