#include <fat32.h>

uint32_t get_BPB_FATSz32(const BPB *bpb)
{
    uint32_t RootDirSectors = ((bpb->BPB_RootEntCnt * 32) + (bpb->BPB_BytesPerSec - 1)) / bpb->BPB_BytesPerSec; // this will be 0 for FAT32
    uint32_t TmpVal1 = bpb->BPB_TotSec32 - (bpb->BPB_RsvdSecCnt + RootDirSectors);
    uint32_t TmpVal2 = ((256 * bpb->BPB_SecPerClus) + bpb->BPB_NumFATs) / 2;
    return (TmpVal1 + (TmpVal2 - 1)) / TmpVal2;
}
