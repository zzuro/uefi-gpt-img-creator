#pragma once
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <fat32.h>

bool add_path(char* path, FILE* img, FILE* in_file);
bool add_file(char* name, FILE* img, FILE* in_file, FILE_TYPE type, uint32_t* parent_cluster);

extern uint32_t data_region;
