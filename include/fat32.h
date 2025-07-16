#pragma once
#include <stdint.h>

typedef struct
{
    uint8_t BS_jmpBoot[3];
    uint8_t BS_OEMName[8];
    uint16_t BPB_BytesPerSec;
    uint8_t BPB_SecPerClus;
    uint16_t BPB_RsvdSecCnt;
    uint8_t BPB_NumFATs;
    uint16_t BPB_RootEntCnt;
    uint16_t BPB_TotSec16;
    uint8_t BPB_Media;
    uint16_t BPB_FATSz16;
    uint16_t BPB_SecPerTrk;
    uint16_t BPB_NumHeads;
    uint32_t BPB_HiddSec;
    uint32_t BPB_TotSec32;
    uint32_t BPB_FATSz32;
    uint16_t BPB_ExtFlags;
    uint16_t BPB_FSVer;
    uint32_t BPB_RootClus;
    uint16_t BPB_FSInfo;
    uint16_t BPB_BkBootSec;
    uint8_t BPB_Reserved[12];
    uint8_t BS_DrvNum;
    uint8_t BS_Reserved1;
    uint8_t BS_BootSig;
    uint8_t BS_VolID[4];
    uint8_t BS_VolLab[11];
    uint8_t BS_FilSysType[8];

    uint8_t BS_Code[420];
    uint16_t BS_Sig; // 0x55AA
} __attribute__((packed)) BPB;

typedef struct
{
    uint32_t FSI_LeadSig;
    uint8_t FSI_Reserved1[480];
    uint32_t FSI_StrucSig;
    uint32_t FSI_Free_Count;
    uint32_t FSI_Nxt_Free;
    uint8_t FSI_Reserved2[12];
    uint32_t FSI_TrailSig;
} __attribute__((packed)) FSINFO;

typedef enum
{
    ATTR_READ_ONLY = 0x01,
    ATTR_HIDDEN = 0x02,
    ATTR_SYSTEM = 0x04,
    ATTR_VOLUME_ID = 0x08,
    ATTR_DIRECTORY = 0x10,
    ATTR_ARCHIVE = 0x20,
    ATTR_LONG_NAME = ATTR_READ_ONLY | ATTR_HIDDEN |
                     ATTR_SYSTEM | ATTR_VOLUME_ID,
} FAT32_ATTR;

typedef struct
{
    uint8_t DIR_Name[11];
    uint8_t DIR_Attr;
    uint8_t DIR_NTRes;
    uint8_t DIR_CrtTimeTenth;
    uint16_t DIR_CrtTime;
    uint16_t DIR_CrtDate;
    uint16_t DIR_LstAccDate;
    uint16_t DIR_FstClusHI;
    uint16_t DIR_WrtTime;
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
} __attribute__((packed)) DIR_ENTRY;

uint32_t get_BPB_FATSz32(const BPB *bpb)
{
    uint32_t RootDirSectors = ((bpb->BPB_RootEntCnt * 32) + (bpb->BPB_BytesPerSec - 1)) / bpb->BPB_BytesPerSec; // this will be 0 for FAT32
    uint32_t TmpVal1 = bpb->BPB_TotSec32 - (bpb->BPB_RsvdSecCnt + RootDirSectors);
    uint32_t TmpVal2 = ((256 * bpb->BPB_SecPerClus) + bpb->BPB_NumFATs) / 2;
    return (TmpVal1 + (TmpVal2 - 1)) / TmpVal2;
}
