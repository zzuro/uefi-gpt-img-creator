#include <utils.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <stdio.h>


static uint32_t crc_table[256];

GUID new_guid(void)
{
    uint8_t random_bytes[16];

    for (int i = 0; i < 16; i++)
    {
        random_bytes[i] = (uint8_t)(rand() & 0xFF);
    }

    GUID guid = {
        .TimeLow = *(uint32_t *)&random_bytes[0],
        .TimeMid = *(uint16_t *)&random_bytes[4],
        .TimeHiAndVersion = ((*(uint16_t *)&random_bytes[6]) & 0x0FFF) | 0x4000, // Set version to 4
        .ClockSeqHiAndReserved = (random_bytes[8] | 0x80) & 0xBF,              // Set variant to RFC 4122
        .ClockSeqLow = random_bytes[9],
        .Node = {
            random_bytes[10], random_bytes[11], random_bytes[12],
            random_bytes[13], random_bytes[14], random_bytes[15]}};
    

    return guid;
}

void make_crc_table(void)
{
    uint32_t c;
    for (int n = 0; n < 256; n++)
    {
        c = (uint32_t)n;
        for (int k = 0; k < 8; k++)
        {
            if (c & 1)
            {
                c = 0xedb88320L ^ (c >> 1);
            }
            else
            {
                c = c >> 1;
            }
        }
        crc_table[n] = c;
    }
}

uint32_t crc32(uint8_t *buf, size_t len)
{
    static bool crc_table_computed = false;
    if (!crc_table_computed)
    {
        make_crc_table();
        crc_table_computed = true;
    }

    uint32_t c = 0xFFFFFFFFL;
    for (size_t n = 0; n < len; n++)
    {
        c = crc_table[(c ^ buf[n]) & 0xFF] ^ (c >> 8);
    }
    return c ^ 0xFFFFFFFFL;
}
