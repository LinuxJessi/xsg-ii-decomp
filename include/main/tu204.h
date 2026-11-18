#ifndef INCLUDE_MAIN_TU204_H
#define INCLUDE_MAIN_TU204_H

/* Functions of src/main/tu204.c that other files call. */

/* Per-channel linear blend of two RGBA colors by fraction t. The operands are 64-bit (callers widen the colors: dsll32/dsrl32 before the call; int parameters compile different bytes in tu234), and the result is a 64-bit value the caller sign-extends from 32 bits (dsll32/dsra32). */
long long xglColorLerp(long long from, long long to, float t);

#endif /* INCLUDE_MAIN_TU204_H */
