#ifndef SYSLIB_H
#define SYSLIB_H

#include <stdnoreturn.h>

#include "common.h"

void          yield(void);
noreturn void exit(void);

int  getpid(void);
int  getpriority(void);
void setpriority(int);

int cpuspeed(void);

int mbox_open(int key);
int mbox_close(int q);
int mbox_stat(int q, int *count, int *space);
int mbox_recv(int q, msg_t *m);
int mbox_send(int q, msg_t *m);
int getchar(int *c);

int readdir(unsigned char *buf);
int loadproc(int location, int size);

#endif /* !SYSLIB_H */
