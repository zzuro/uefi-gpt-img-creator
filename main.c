#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <compiler.h>
#include <mbr.h>
#include <gpt.h>
#include <utils.h>
#include <const.h>

#define NAME_LEN 512

static bool write_mbr(FILE *file);
static bool write_gpt(FILE *file);

int main(int argc, char *argv[])
{

    char img_name[NAME_LEN] = "test.img";
    if (argc > 1)
    {
        if (strlen(argv[1]) >= NAME_LEN)
        {
            fprintf(stderr, "Image name is too long.\n");
            return EXIT_FAILURE;
        }
        strncpy(img_name, argv[1], NAME_LEN - 1);
        img_name[NAME_LEN - 1] = '\0';
    }

    FILE *file = fopen(img_name, "wb+");
    if (file == NULL)
    {
        fprintf(stderr, "Error opening file %s\n", img_name);
        return EXIT_FAILURE;
    }

    if (!write_mbr(file))
    {
        fprintf(stderr, "Error writing MBR to file %s\n", img_name);
        fclose(file);
        return EXIT_FAILURE;
    }

    if (!write_gpt(file))
    {
        fprintf(stderr, "Error writing GPT to file %s\n", img_name);
        fclose(file);
        return EXIT_FAILURE;
    }

    fclose(file);
    return EXIT_SUCCESS;
}

bool write_mbr(FILE *file)
{
    MASTER_BOOT_RECORD mbr_record = {
        .BootStrapCode = {0x00},
        .UniqueMbrSignature = 0x0,
        .Unknown = 0x0,
         // FLAG to indicate to use GPT
        .Partition[0] = { 
                         .BootIndicator = 0x00,
                         .StartingCHS = {0x00, 0x02, 0x00},
                         .OSIndicator = 0xEE,   //protected MBR
                         .EndingCHS = {0xFF, 0xFF, 0xFF},
                         .StartingLBA = 0x00000001,
                         .SizeInLBA = B2LBA(DISK_SIZE) - 1},
        .Signature = 0xAA55};

    if (fwrite(&mbr_record, sizeof(mbr_record), 1, file) * sizeof(mbr_record) != sizeof(mbr_record))
    {
        return false;
    }

    return true;
}

bool write_gpt(FILE *file)
{
    GPT_HEADER gpt_header = {
        .Signature = {"EFI PART"},
        .Revision = 0x00010000, 
        .HeaderSize = 92,
        .HeaderCRC32 = 0x0, 
        .Reserved = 0x0,
        .MyLBA = 1, // GPT header starts at LBA 1
        .AlternateLBA = B2LBA(DISK_SIZE) - 1, 
        .FirstUsableLBA = 34, //MBR + GPT header + GPT TABLE
        .LastUsableLBA = B2LBA(DISK_SIZE) - 34, 
        .DiskGUID = new_guid(), 
        .PartitionEntryLBA = 2, // after MBR and GPT header
        .NumberOfPartitionEntries = GPT_TABLE_ENTRIES, 
        .SizeOfPartitionEntry = GPT_ENTRY_SIZE,
        .PartitionEntryArrayCRC32 = 0x0, 
        .Reserved2 = {0x00} 
    };

    GPT_PARTITION_ENTRY gpt_partition_entry[GPT_TABLE_ENTRIES] = {

        //EFI System Partition
        {
            .PartitionTypeGUID = {0xC12A7328, 0xF81F, 0x11D2, {0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xB8, 0xA1}}, // GUID for EFI System Partition
            .UniquePartitionGUID = new_guid(),
            .StartingLBA = ESP_STARTING_LBA, //1MB alignment
            .EndingLBA = ESP_STARTING_LBA + B2LBA(ESP_SIZE), 
            .Attributes = 0x0,
            .PartitionName = u"EFI System Partition"
        },
        // Microsoft Reserved Partition
        {
            .PartitionTypeGUID = {0xE3C9E316, 0x0B5C, 0x4DB8, {0x81, 0xD6, 0x08, 0x00, 0x20, 0xA5, 0xA5, 0xA5}}, // GUID for Microsoft Reserved Partition
            .UniquePartitionGUID = new_guid(),
            .StartingLBA = DATA_STARTING_LBA,
            .EndingLBA = DATA_STARTING_LBA + B2LBA(DATA_SIZE), 
            .Attributes = 0x0,
            .PartitionName = u"Data Partition"
        }
        
    };
    return true;
}
