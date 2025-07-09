#pragma once
#include <stdint.h>
#include <uchar.h>
#include <utils.h>

#define GPT_TABLE_ENTRIES 128
#define GPT_ENTRY_SIZE 128

typedef struct
{
    uint8_t Signature[8];
    uint32_t Revision;
    uint32_t HeaderSize;
    uint32_t HeaderCRC32;
    uint32_t Reserved;
    uint64_t MyLBA;
    uint64_t AlternateLBA;
    uint64_t FirstUsableLBA;
    uint64_t LastUsableLBA;
    GUID DiskGUID;
    uint64_t PartitionEntryLBA;
    uint32_t NumberOfPartitionEntries;
    uint32_t SizeOfPartitionEntry;
    uint32_t PartitionEntryArrayCRC32;
    uint8_t Reserved2[420];
} __attribute__((packed)) GPT_HEADER;


typedef struct
{
    GUID PartitionTypeGUID;
    GUID UniquePartitionGUID;
    uint64_t StartingLBA;
    uint64_t EndingLBA;
    uint64_t Attributes;
    char16_t PartitionName[36]; 
} __attribute__((packed)) GPT_PARTITION_ENTRY;
