// Source for LBA calculations: https://en.wikipedia.org/wiki/GUID_Partition_Table

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <compiler.h>
#include <mbr.h>
#include <gpt.h>
#include <utils.h>
#include <const.h>
#include <log.h>
#include <fat32.h>
#include <esp.h>

#define NAME_LEN 512

static bool write_mbr(FILE *file);
static bool write_gpt(FILE *file);
static bool write_esp(FILE *file);

uint32_t data_region = 0; 

int main(int argc, char *argv[])
{

    char img_name[NAME_LEN] = "test.img";
    srand(time(NULL));

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
    else
    {
        printf("MBR written successfully to %s\n", img_name);
    }

    if (!write_gpt(file))
    {
        fprintf(stderr, "Error writing GPT to file %s\n", img_name);
        fclose(file);
        return EXIT_FAILURE;
    }
    else
    {
        printf("GPT written successfully to %s\n", img_name);
    }

    if (!write_esp(file))
    {
        fprintf(stderr, "Error writing ESP to file %s\n", img_name);
        fclose(file);
        return EXIT_FAILURE;
    }
    else
    {
        printf("ESP written successfully to %s\n", img_name);
    }

    FILE* fp = fopen("BOOTX64.EFI", "rb");
    if (fp)
    {
        char path[25];
        strcpy(path, "/EFI/BOOT/BOOTX64.EFI");

        if(!add_path(path, file, fp))
        {
            fprintf(stderr, "Error adding path %s to file %s\n", path, img_name);
            fclose(fp);
            return EXIT_FAILURE;
        }
        else
        {
            printf("Path %s added successfully to %s\n", path, img_name);
            fclose(fp);
        }
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
            .OSIndicator = 0xEE, // protected MBR
            .EndingCHS = {0xFF, 0xFF, 0xFF},
            .StartingLBA = 0x00000001,
            .SizeInLBA = B2LBA(DISK_SIZE) - 1},
        .Signature = 0xAA55};

    if (fwrite(&mbr_record, sizeof(MASTER_BOOT_RECORD), 1, file) != 1)
    {
        return false;
    }

    return true;
}

bool write_gpt(FILE *file)
{
    GPT_HEADER primary_gpt_header = {
        .Signature = {"EFI PART"},
        .Revision = 0x00010000,
        .HeaderSize = 92,
        .HeaderCRC32 = 0x0,
        .Reserved = 0x0,
        .MyLBA = 1,                                                                                           // GPT header starts at LBA 1
        .AlternateLBA = B2LBA(DISK_SIZE) - 1,                                                                 // Backup GPT header at the end of the disk
        .FirstUsableLBA = 1 + 1 + B2LBA(GPT_TABLE_ENTRIES * sizeof(GPT_PARTITION_ENTRY)),                     // MBR + GPT header + GPT TABLE
        .LastUsableLBA = B2LBA(DISK_SIZE) - (1 + 1 + B2LBA(GPT_TABLE_ENTRIES * sizeof(GPT_PARTITION_ENTRY))), // Last usable LBA before the backup GPT header
        .DiskGUID = new_guid(),
        .PartitionEntryLBA = 2, // after MBR and GPT header
        .NumberOfPartitionEntries = GPT_TABLE_ENTRIES,
        .SizeOfPartitionEntry = GPT_ENTRY_SIZE,
        .PartitionEntryArrayCRC32 = 0x0,
        .Reserved2 = {0x00}};

    GPT_PARTITION_ENTRY primary_gpt_partition_entry[GPT_TABLE_ENTRIES] = {

        // EFI System Partition
        {
            .PartitionTypeGUID = {0xC12A7328, 0xF81F, 0x11D2, 0xBA, 0x4B, {0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B}}, // GUID for EFI System Partition
            .UniquePartitionGUID = new_guid(),
            .StartingLBA = ESP_STARTING_LBA,
            .EndingLBA = ESP_STARTING_LBA + B2LBA(ESP_SIZE) - 1,
            .Attributes = 0x0,
            .PartitionName = u"EFI System Partition"},

        // Microsoft basic data
        {
            .PartitionTypeGUID = {0xEBD0A0A2, 0xB9E5, 0x4433, 0x87, 0xC0, {0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7}},
            .UniquePartitionGUID = new_guid(),
            .StartingLBA = DATA_STARTING_LBA,
            .EndingLBA = DATA_STARTING_LBA + B2LBA(DATA_SIZE) - 1,
            .Attributes = 0x0,
            .PartitionName = u"Data Partition"}};

    GPT_HEADER backup_gpt_header = primary_gpt_header;
    backup_gpt_header.MyLBA = primary_gpt_header.AlternateLBA;
    backup_gpt_header.AlternateLBA = primary_gpt_header.MyLBA;
    backup_gpt_header.PartitionEntryLBA = B2LBA(DISK_SIZE) - 33;

    primary_gpt_header.PartitionEntryArrayCRC32 = crc32((uint8_t *)primary_gpt_partition_entry, sizeof(primary_gpt_partition_entry));
    primary_gpt_header.HeaderCRC32 = crc32((uint8_t *)&primary_gpt_header, primary_gpt_header.HeaderSize);

    if ((fwrite(&primary_gpt_header, sizeof(GPT_HEADER), 1, file) != 1) ||
        (fwrite(primary_gpt_partition_entry, sizeof(GPT_PARTITION_ENTRY), GPT_TABLE_ENTRIES, file) != GPT_TABLE_ENTRIES))
    {
        return false;
    }

    backup_gpt_header.PartitionEntryArrayCRC32 = primary_gpt_header.PartitionEntryArrayCRC32;
    backup_gpt_header.HeaderCRC32 = crc32((uint8_t *)&backup_gpt_header, backup_gpt_header.HeaderSize);

    fseek(file, backup_gpt_header.PartitionEntryLBA * LBA_SIZE, SEEK_SET);
    if ((fwrite(primary_gpt_partition_entry, sizeof(GPT_PARTITION_ENTRY), GPT_TABLE_ENTRIES, file) != GPT_TABLE_ENTRIES) ||
        (fwrite(&backup_gpt_header, sizeof(GPT_HEADER), 1, file) != 1))
    {
        return false;
    }

    return true;
}

bool write_esp(FILE *file)
{
    BPB bpb = {
        .BS_jmpBoot = {0xEB, 0x00, 0x90},
        .BS_OEMName = "MSWIN4.1",
        .BPB_BytesPerSec = LBA_SIZE,
        .BPB_SecPerClus = 1,
        .BPB_RsvdSecCnt = 32,
        .BPB_NumFATs = 2,
        .BPB_RootEntCnt = 0,
        .BPB_TotSec16 = 0,
        .BPB_Media = 0xF8,
        .BPB_FATSz16 = 0,
        .BPB_SecPerTrk = 0,
        .BPB_NumHeads = 0,
        .BPB_HiddSec = ESP_STARTING_LBA - 1, // Hidden sectors before the ESP
        .BPB_TotSec32 = B2LBA(ESP_SIZE),     // Total sectors in the partition
        .BPB_FATSz32 = 0,                    // Size of each FAT in sectors
        .BPB_ExtFlags = 0,
        .BPB_FSVer = 0,
        .BPB_RootClus = 2,
        .BPB_FSInfo = 1,
        .BPB_BkBootSec = 6,
        .BPB_Reserved = {0x00},
        .BS_DrvNum = 0x80,
        .BS_Reserved1 = 0x00,
        .BS_BootSig = 0x29,
        .BS_VolID = {rand() % 256, rand() % 256, rand() % 256, rand() % 256},
        .BS_VolLab = {"ESP        "},
        .BS_FilSysType = {"FAT32   "},
        .BS_Code = {0x00},
        .BS_Sig = 0xAA55 // Boot sector signature
    };

    FSINFO fsinfo = {
        .FSI_LeadSig = 0x41615252,
        .FSI_Reserved1 = {0x00},
        .FSI_StrucSig = 0x61417272,
        .FSI_Free_Count = 0xFFFFFFFF,
        .FSI_Nxt_Free = 5, // Hint for the next free cluster
        .FSI_Reserved2 = {0x00},
        .FSI_TrailSig = 0xAA550000};

    bpb.BPB_FATSz32 = get_BPB_FATSz32(&bpb);

    fseek(file, ESP_STARTING_LBA * LBA_SIZE, SEEK_SET);
    if (fwrite(&bpb, sizeof(BPB), 1, file) != 1)
    {
        return false;
    }

    if (fwrite(&fsinfo, sizeof(FSINFO), 1, file) != 1)
    {
        return false;
    }


    // Write bpb and fsinfo to the backup boot sector
    fseek(file, (ESP_STARTING_LBA + bpb.BPB_BkBootSec) * LBA_SIZE, SEEK_SET);
    if (fwrite(&bpb, sizeof(BPB), 1, file) != 1)
    {
        return false;
    }
    if (fwrite(&fsinfo, sizeof(FSINFO), 1, file) != 1)
    {
        return false;
    }

    uint32_t cluster; // one cluster is 4 bytes, cluster = 0xFFFFFFFF for single cluster, cluster = num for chaining clusters
    // src: https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system
    for (uint8_t i = 0; i < bpb.BPB_NumFATs; i++)
    {
        fseek(file, (ESP_STARTING_LBA + bpb.BPB_RsvdSecCnt + i * bpb.BPB_FATSz32) * LBA_SIZE, SEEK_SET);

        cluster = 0xFFFFFF00 | bpb.BPB_Media;
        fwrite(&cluster, sizeof(uint32_t), 1, file); // cluster 0

        // Cluster 1; End of Chain (EOC) marker
        cluster = 0xFFFFFFFF;
        fwrite(&cluster, sizeof(uint32_t), 1, file);

        // Cluster 2; Root dir '/' cluster start, if end of file/dir data then write EOC marker
        fwrite(&cluster, sizeof(uint32_t), 1, file);

        // Cluster 3; '/EFI' dir cluster
        fwrite(&cluster, sizeof(uint32_t), 1, file);

        // Cluster 4; '/EFI/BOOT' dir cluster
        fwrite(&cluster, sizeof(uint32_t), 1, file);
    }

    data_region = ESP_STARTING_LBA + bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * bpb.BPB_FATSz32);
    fseek(file, data_region * LBA_SIZE, SEEK_SET);

    DIR_ENTRY efi_dir = {
        .DIR_Name = "EFI        ",
        .DIR_Attr = ATTR_DIRECTORY,
        .DIR_NTRes = 0,
        .DIR_CrtTimeTenth = 0,
        .DIR_CrtTime = 0,
        .DIR_CrtDate = 0,
        .DIR_LstAccDate = 0,
        .DIR_FstClusHI = 0,
        .DIR_WrtTime = 0,
        .DIR_WrtDate = 0,
        .DIR_FstClusLO = 3,         // Cluster 3 for '/EFI'
        .DIR_FileSize = 0           // File size (0 for directories)
    };

    uint16_t in_time, in_date;
    get_current_time(&in_time, &in_date);
    efi_dir.DIR_CrtTime = in_time;
    efi_dir.DIR_CrtDate = in_date;
    efi_dir.DIR_WrtTime = in_time;
    efi_dir.DIR_WrtDate = in_date;

    // Write EFI directory into the root directory
    if (fwrite(&efi_dir, sizeof(DIR_ENTRY), 1, file) != 1)
    {
        return false;
    }

    fseek(file, (data_region + 1) * LBA_SIZE, SEEK_SET); // Move to next cluster
    memcpy(efi_dir.DIR_Name, ".          ", 11);
    if (fwrite(&efi_dir, sizeof(DIR_ENTRY), 1, file) != 1)
    {
        return false;
    }

    memcpy(efi_dir.DIR_Name, "..         ", 11); // Parent directory
    efi_dir.DIR_FstClusLO = 0;                   // Root directory does not have cluster value
    if (fwrite(&efi_dir, sizeof(DIR_ENTRY), 1, file) != 1)
    {
        return false;
    }

    // Write the '/EFI/BOOT' directory entry
    memcpy(efi_dir.DIR_Name, "BOOT       ", 11);
    efi_dir.DIR_FstClusLO = 4; // Cluster 4 for '/EFI/BOOT'
    if(fwrite(&efi_dir, sizeof(DIR_ENTRY), 1, file) != 1)
    {
        return false;
    }

    fseek(file, (data_region + 2) * LBA_SIZE, SEEK_SET); // Move to next cluster
    memcpy(efi_dir.DIR_Name, ".          ", 11);
    if (fwrite(&efi_dir, sizeof(DIR_ENTRY), 1, file) != 1)
    {
        return false;
    }

    memcpy(efi_dir.DIR_Name, "..         ", 11); // Parent directory
    efi_dir.DIR_FstClusLO = 3;
    if (fwrite(&efi_dir, sizeof(DIR_ENTRY), 1, file) != 1)
    {
        return false;
    }

    return true;
}
