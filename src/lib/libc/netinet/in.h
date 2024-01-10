#ifndef CUSTOM_LIBC_NETINET_IN_H
#define CUSTOM_LIBC_NETINET_IN_H

#include <stdint.h>

/* === Core: no syscalls required === */

uint32_t ntohl(uint32_t data);
uint32_t htonl(uint32_t data);
uint16_t ntohs(uint16_t data);
uint16_t htons(uint16_t data);

/* === Non-core: syscalls required === */
#ifndef OSLIBC_CORE_ONLY
#endif /* !OSLIBC_CORE_ONLY */

#endif /* CUSTOM_LIBC_NETINET_IN_H */
