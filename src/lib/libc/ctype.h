#ifndef CUSTOM_LIBC_CTYPE_H
#define CUSTOM_LIBC_CTYPE_H

#include <stddef.h>

/* === Core: no syscalls required === */

/* Not yet implemented: isalnum */
/* Not yet implemented: isalpha */
/* Not yet implemented: isblank */
/* Not yet implemented: iscntrl */

int isdigit(int ch);

/* Not yet implemented: isgraph */

int islower(int ch);
int isprint(int ch);

/* Not yet implemented: ispunct */

int isspace(int ch);
int isupper(int ch);

/* Not yet implemented: isxdigit */

/* === Non-core: syscalls required === */
#ifndef OSLIBC_CORE_ONLY
#endif /* !OSLIBC_CORE_ONLY */
#endif /* CUSTOM_LIBC_CTYPE_H */
