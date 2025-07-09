#pragma once
#include <stdint.h>
#include <compiler.h>

typedef struct
{
    uint8_t BootIndicator;
    uint8_t StartingCHS[3];
    uint8_t OSIndicator;
    uint8_t EndingCHS[3];
    uint32_t StartingLBA;
    uint32_t SizeInLBA;
} __attribute__((packed)) MBR_PARTITION_RECORD;

typedef struct
{
    uint8_t BootStrapCode[440];
    uint32_t UniqueMbrSignature;
    uint16_t Unknown;
    MBR_PARTITION_RECORD Partition[4];
    uint16_t Signature;
} __attribute__((packed)) MASTER_BOOT_RECORD;
