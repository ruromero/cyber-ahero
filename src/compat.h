#ifndef COMPAT_H
#define COMPAT_H

/* Lets portable modules use the MSVCRT names on non-Windows hosts, so the
 * same source builds for the app (mingw) and for the native unit tests. */
#ifndef _WIN32
#include <strings.h>
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#endif

#endif
