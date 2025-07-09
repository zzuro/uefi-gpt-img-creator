#include <utils.h>
#include <stdlib.h>
#include <time.h>

static uint64_t crc_table[256];
static int crc_table_computed = 0;

GUID new_guid(void)
{
    uint8_t random_bytes[16];
    static int seeded = 0;
    if (!seeded)
    {
        srand((unsigned int)time(NULL));
        seeded = 1;
    }

    for (int i = 0; i < 16; i++)
    {
        random_bytes[i] = (uint8_t)(rand() & 256);
    }

    GUID guid = {
        .TimeLow = *(uint32_t *)&random_bytes[0],
        .TimeMid = *(uint16_t *)&random_bytes[4],
        .TimeHiAndVersion = (*(uint16_t *)&random_bytes[6]) & 0x0FFF | 0x4000, // Set version to 4
        .ClockSeqHiAndReserved = (random_bytes[8] & 0x3F) | 0x80,              // Set variant to RFC 4122
        .ClockSeqLow = random_bytes[9],
        .Node = {
            random_bytes[10], random_bytes[11], random_bytes[12],
            random_bytes[13], random_bytes[14], random_bytes[15]}};

    return guid;
}

void make_crc_table(void)
{
    uint64_t c;
    int n, k;

    for (n = 0; n < 256; n++)
    {
        c = (uint64_t)n;
        for (k = 0; k < 8; k++)
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
    crc_table_computed = 1;
}

uint64_t update_crc(uint64_t crc, uint8_t *buf, size_t len)
{
    uint64_t c = crc;
    int n;

    if (!crc_table_computed)
    {
        make_crc_table();
    }
        
    for (n = 0; n < len; n++)
    {
        c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
    }
    return c;
}

uint64_t crc32(uint8_t *buf, size_t len)
{
    return update_crc(0xffffffffL, buf, len) ^ 0xffffffffL;
}
