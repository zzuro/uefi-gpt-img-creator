#pragma once
#include <compiler.h>

#define LBA_SIZE 512
#define B2LBA(val) ((_ul(val)) / LBA_SIZE + ((_ul(val)) % LBA_SIZE ? 1 : 0))

#define ESP_SIZE MB(33)
#define ESP_STARTING_LBA (MB(1) / LBA_SIZE) // 1MB alignment, after MBR and GPT header

#define DATA_SIZE MB(1) 
#define DATA_STARTING_LBA 1

#define PADDIGG_SIZE MB(1) 

#define DISK_SIZE (ESP_SIZE + DATA_SIZE + PADDIGG_SIZE)