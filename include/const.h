#pragma once
#include <compiler.h>

#define LBA_SIZE 512
#define LBA_ALIGNMENT _ul(MB(1) / LBA_SIZE) // 1MB alignment, after MBR and GPT header

#define B2LBA(val) ((_ul(val)) / LBA_SIZE + ((_ul(val)) % LBA_SIZE ? 1 : 0))
#define LBA_NEXT_ALIGN(val) (val - (val % LBA_ALIGNMENT) + LBA_ALIGNMENT)

#define ESP_SIZE MB(33) //EFI Partition uses FAT32, so it needs at least 32MB for the FAT and filesystem structures
#define ESP_STARTING_LBA LBA_ALIGNMENT

#define DATA_SIZE MB(1) 
#define DATA_STARTING_LBA LBA_NEXT_ALIGN(ESP_STARTING_LBA + B2LBA(ESP_SIZE)) 

extern uint64_t DISK_SIZE;
