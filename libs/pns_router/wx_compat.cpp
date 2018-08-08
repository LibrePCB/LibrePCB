#include "wx_compat.h"
#include <stdio.h>
#include <stdarg.h>


void wxLogTrace (const char *  mask, const char *  	formatString, ...) {
	va_list args;

	va_start(args, formatString);
    vprintf(formatString, args);
    puts("\n");

    va_end(args);
}
