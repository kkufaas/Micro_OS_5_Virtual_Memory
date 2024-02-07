#ifndef SYSLIB_H
#define SYSLIB_H

#include <stdnoreturn.h>

void          yield(void);
noreturn void exit(void);

int  getpid(void);
int  getpriority(void);
void setpriority(int);

int cpuspeed(void);

#endif /* !SYSLIB_H */
