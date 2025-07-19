#include <esp.h>
#include <string.h>
#include <const.h>
#include <ctype.h>
#include <utils.h>
#include <inttypes.h>

bool add_path(char* path, FILE* img, FILE* in_file)
{
    if(path[0] != '/')
    {
        fprintf(stderr, "Path must start with '/'\n");
        return false;
    }

    for(size_t i = 0; i < strlen(path); i++)
    {
        path[i] = toupper(path[i]);
    }

    char* start = path + 1;
    char* end = start;
    FILE_TYPE type = TYPE_DIR;
    uint32_t cluster = 2; //root starts at cluster 2 in FAT32 

    while(type == TYPE_DIR)
    {
        while(*end != '/' && *end != '\0') end++;

        if(*end == '/')
        {
            type = TYPE_DIR;
            *end = '\0'; 
        }else
        {
            type = TYPE_FILE;
        }

        char* dot_pos = strchr(start, '.');
        if ((type == TYPE_DIR  && strlen(start) > 11) ||
            (type == TYPE_FILE && strlen(start) > 12) || 
            (dot_pos && ((dot_pos - start > 8) || (end - dot_pos) > 4))) { 

            fprintf(stderr, "Name '%s' is too long for FAT32\n", start);
            if(dot_pos)
            {
                dot_pos[4] = '\0'; // truncate extension if it exists
            }
            else
            {
                size_t diff = strlen(start) - 11;
                *end = '/';

                char* next = end + 1;
                memmove(start + 12, next, strlen(next)); 

                *(path + strlen(path) - diff) = '\0'; //new end with shortened name
                end = start + 11; // reset end to the new end of the name
                *end = '\0'; // terminate the string
            }
        }

        char name[12] = {0};
        memset(name, ' ', sizeof(name));
        if(type == TYPE_DIR || !dot_pos)
        {
            memcpy(name, start, strlen(start) > 11 ? 11 : strlen(start));
        }
        else
        {
            memcpy(name, start, dot_pos - start); //name
            strncpy(&name[8], dot_pos + 1, 3);    //extension
        }

        DIR_ENTRY entry = {0};
        fseek(img, (data_region + cluster - 2) * LBA_SIZE, SEEK_SET);
        bool found = false;
        do {
            fread(&entry, sizeof(DIR_ENTRY), 1, img);
            if( !memcmp(entry.DIR_Name, name, 11))
            {
                cluster = (entry.DIR_FstClusHI << 16) | entry.DIR_FstClusLO;
                found = true;
                break;
            }

        } while(entry.DIR_Name[0] != 0x0);
        

        if(!found)
        {
            if(!add_file(name, img, in_file, type, &cluster))
            {
                fprintf(stderr, "Error adding file %s to cluster %u\n", name, cluster);
                return false;
            }
        }

        *end++ = '/';
        start = end;
    }

    *--end = '\0';
    return true;
}


bool add_file(char* name, FILE* img, FILE* in_file, FILE_TYPE type, uint32_t* parent_cluster)
{
    BPB bpb;
    fseek(img, ESP_STARTING_LBA * LBA_SIZE, SEEK_SET);
    if(fread(&bpb, sizeof(BPB), 1, img) != 1)
    {
        fprintf(stderr, "Error reading BPB\n");
        return false;
    }

    FSINFO fsinfo;
    if(fread(&fsinfo, sizeof(FSINFO), 1, img) != 1)
    {
        fprintf(stderr, "Error reading FSINFO\n");
        return false;
    }

    uint64_t file_size;
    if(type == TYPE_FILE)
    {
        fseek(in_file, 0, SEEK_END);
        file_size = ftell(in_file);
        rewind(in_file);
    }

    uint32_t next_free_cluster = fsinfo.FSI_Nxt_Free;
    uint32_t current_cluster = next_free_cluster;

    for(uint8_t i = 0; i < bpb.BPB_NumFATs; i++)
    {
        uint32_t cluster = fsinfo.FSI_Nxt_Free;
        next_free_cluster = fsinfo.FSI_Nxt_Free;

        fseek(img, (ESP_STARTING_LBA + bpb.BPB_RsvdSecCnt + i * bpb.BPB_FATSz32) * LBA_SIZE + next_free_cluster * sizeof(uint32_t), SEEK_SET);

        if(type == TYPE_FILE && file_size > 0)
        {
            for(size_t lbas = 0; lbas < B2LBA(file_size) - 1; lbas++)
            {
                cluster++;
                next_free_cluster++;
                if(fwrite(&cluster, sizeof(uint32_t), 1, img) != 1)
                {
                    fprintf(stderr, "Error writing cluster %u to FAT\n", cluster);
                    return false; 
                }
            }
        }

        cluster = 0xFFFFFFFF;
        next_free_cluster++;
        if(fwrite(&cluster, sizeof(uint32_t), 1, img) != 1)
        {
            fprintf(stderr, "Error writing EOC marker to FAT\n");
            return false;
        }
    }

    fsinfo.FSI_Nxt_Free = next_free_cluster;
    fseek(img, ESP_STARTING_LBA * LBA_SIZE + sizeof(BPB), SEEK_SET);
    if (fwrite(&fsinfo, sizeof(FSINFO), 1, img) != 1)
    {
        return false;
    }

    fseek(img, (ESP_STARTING_LBA + bpb.BPB_BkBootSec) * LBA_SIZE + sizeof(BPB), SEEK_SET);
    if (fwrite(&fsinfo, sizeof(FSINFO), 1, img) != 1)
    {
        return false;
    }


    fseek(img, (data_region + *parent_cluster - 2) * LBA_SIZE, SEEK_SET);
    DIR_ENTRY entry = {0};
    fread(&entry, sizeof(DIR_ENTRY), 1, img);
    while(entry.DIR_Name[0] != 0x00)
    {
        fread(&entry, sizeof(DIR_ENTRY), 1, img);
    }

    fseek(img, -sizeof(DIR_ENTRY), SEEK_CUR);
    memcpy(entry.DIR_Name, name, 11);
    
    if(type == TYPE_DIR)
    {
        entry.DIR_Attr = ATTR_DIRECTORY;
    }

    uint16_t in_time, in_date;
    get_current_time(&in_time, &in_date);
    entry.DIR_CrtTime = in_time;
    entry.DIR_CrtDate = in_date;
    entry.DIR_WrtTime = in_time;
    entry.DIR_WrtDate = in_date;

    entry.DIR_FstClusHI = (current_cluster >> 16) & 0xFFFF;
    entry.DIR_FstClusLO = current_cluster & 0xFFFF;
    
    entry.DIR_FileSize = type == TYPE_FILE ? file_size : 0;

    if(fwrite(&entry, sizeof(DIR_ENTRY), 1, img) != 1)
    {
        fprintf(stderr, "Error writing directory entry for %s\n", name);
        return false;
    }

    fseek(img, (data_region + current_cluster - 2) * LBA_SIZE, SEEK_SET);
    if(type == TYPE_DIR)
    {
        memcpy(entry.DIR_Name, ".          ", 11);
        if(fwrite(&entry, sizeof(DIR_ENTRY), 1, img) != 1)
        {
            fprintf(stderr, "Error writing '.' entry for directory %s\n", name);
            return false;
        }

        memcpy(entry.DIR_Name, "..         ", 11);
        entry.DIR_FstClusLO = *parent_cluster & 0xFFFF; 
        entry.DIR_FstClusHI = (*parent_cluster >> 16) & 0xFFFF; 
        if(fwrite(&entry, sizeof(DIR_ENTRY), 1, img) != 1)
        {
            fprintf(stderr, "Error writing '.' entry for directory %s\n", name);
            return false;
        }
    }else
    {
        uint8_t* buf = malloc(LBA_SIZE);
        for(size_t i = 0; i < B2LBA(file_size); i++)
        {
            size_t bytes_read = fread(buf, 1, LBA_SIZE, in_file);
            if(fwrite(buf, bytes_read, 1, img) != 1)
            {
                fprintf(stderr, "Error writing file %s to cluster %u\n", name, current_cluster);
                free(buf);
                return false;
            }
        }

        free(buf);
    }


    *parent_cluster = current_cluster; 
    return true;
}

bool add_info_file(FILE* img)
{    
    FILE* fp = fopen("DSKIMG.INF", "wb");
    if(!fp)
    {
        fprintf(stderr, "Error opening DSKIMG.INF for writing\n");
        return false;
    }

    fprintf(fp, "DISK_SIZE=%"PRIu64"\n", DISK_SIZE);
    fclose(fp);

    fp = fopen("DSKIMG.INF", "rb");
    if(!fp)
    {
        fprintf(stderr, "Error opening DSKIMG.INF for reading\n");
        return false;
    }

    char path[25];
    strcpy(path, "/EFI/BOOT/DSKIMG.INF");

    if(!add_path(path, img, fp))
    {
        fprintf(stderr, "Error adding path %s to image\n", path);
        fclose(fp);
        return false;
    }

    fclose(fp);
    return  true;
}
