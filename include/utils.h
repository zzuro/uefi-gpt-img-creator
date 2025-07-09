#pragma once
#include <stdint.h>
#include <stddef.h>

typedef struct
{
    uint32_t TimeLow;
    uint16_t TimeMid;
    uint16_t TimeHiAndVersion;
    uint8_t ClockSeqHiAndReserved;
    uint8_t ClockSeqLow;
    uint8_t Node[6];
} __attribute__((packed)) GUID;


GUID new_guid(void);

void make_crc_table(void);
uint64_t update_crc(uint64_t crc, uint8_t *buf, size_t len);
uint64_t crc32(uint8_t* buf, size_t len);



