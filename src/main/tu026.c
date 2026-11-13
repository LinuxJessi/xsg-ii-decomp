#include "common.h"
#include "main/tu026.h"

int printf(const char *format, ...);
/* Exit path shared by the assertion and error reporters: prints "> Exit Program", then traps (break). */ void assertExit(void);
extern char assertBannerTop[];
extern char assertFormat[];
extern char assertBannerBottom[];

/* Assertion failure report: prints the banner, "ASSERTION <file, line> :: message", then exits. */
void assertFailed(const char *message, const char *file, int line) {
    printf(assertBannerTop);
    printf(assertFormat, file, line, message);
    printf(assertBannerBottom);
    assertExit();
}

#include "debug_break.h"

extern char exitProgramBanner[];

/* Exit path shared by the assertion and error reporters: prints "> Exit Program", then traps (break). */
void assertExit(void) {
    printf(exitProgramBanner);
    DEBUG_BREAK();
}

typedef char *va_list;

#define va_start(ap, last) ((ap) = (va_list)__builtin_next_arg(last) - (8 - __builtin_args_info(2)) * 8)

extern char errorBanner[];
int func_002AF028();

/* Fatal error with a printf-style message: prints "> Error!!", the formatted message, then exits. */
void errorFatal(const char *format, ...) {
    va_list args;

    printf(errorBanner);
    va_start(args, format);
    func_002AF028(format, args);
    printf(format);
    assertExit();
}
