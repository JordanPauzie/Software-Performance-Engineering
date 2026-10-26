/* Sources:
    https://stackoverflow.com/questions/40159892/using-asprintf-on-
    https://stackoverflow.com/questions/4785381/replacement-for-ms-vscprintf-on-macos-linux
*/
#ifndef ASPRINTF_H
#define ASPRINTF_H

#include <stdarg.h> /* needed for va_list */

int _vscprintf_so(const char *format, va_list pargs);

int vasprintf(char **strp, const char *fmt, va_list ap);

int asprintf(char *strp[], const char *fmt, ...);

#endif // ASPRINTF_H