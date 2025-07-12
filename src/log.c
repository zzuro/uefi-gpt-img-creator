#include <log.h>


void print_guid(const GUID *guid)
{
    if (guid == NULL)
    {
        fprintf(stderr, "GUID is NULL\n");
        return;
    }

    printf("GUID: %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
           guid->TimeLow,
           guid->TimeMid,
           guid->TimeHiAndVersion,
           guid->ClockSeqHiAndReserved,
           guid->ClockSeqLow,
           guid->Node[0],
           guid->Node[1],
           guid->Node[2],
           guid->Node[3],
           guid->Node[4],
           guid->Node[5]);
}
