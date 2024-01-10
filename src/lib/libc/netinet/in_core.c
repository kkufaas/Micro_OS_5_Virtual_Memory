#include "in.h"

uint32_t ntohl(uint32_t data)
{
    return (((data & 0x000000ff) << 24) | ((data & 0x0000ff00) << 8)
            | ((data & 0x00ff0000) >> 8) | ((data & 0xff000000) >> 24));
}

uint32_t htonl(uint32_t data) { return ntohl(data); }

uint16_t ntohs(uint16_t data)
{
    return (((data & 0x00ff) << 8) | (data & 0xff00) >> 8);
}

uint16_t htons(uint16_t data) { return ntohs(data); }

