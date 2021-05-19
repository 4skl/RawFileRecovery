#include "parse_filter.h"

//File System

char * EFI_START_1 = "EFI PART";
//NTFS
char * NTFS_BOOT_SECTOR_END = "\x55\xAA";
int NTFS_BOOT_SECTOR_END_SIZE = 512;

//Files
char * PDF_START = "%PDF";
char * PNG_START = "\x89PNG\x0D\x0A\x1A\x0A";
char * PNG_END = "IEND\xAE\x42\x60\x82";

//Todo precise more (multiple types,...)
char * DOCX_START = "\x50\x4B\x03\x04\x14\x00\x06\x00\x08\x00\x00\x00\x21\x00";
char * DOCX_END = "\x50\x4B\x05\x06";


bool matchStr(char * in, void * args, size_t size){
    char * val = (char *) args;
    for(size_t i = 0;i<size;i++) if(in[i] != val[i]) return false;
    return true;
}

SFC init_matchText(char * text){ // Test match
    return SFC{.input_size=strlen(text), .displace_to_pos=0, .fun=matchStr, .args=(void *) text};
}

SFC init_DOCXStart(){
    SFC sfcs = init_matchText(DOCX_START);
    sfcs.displace_to_pos = 0;
    return sfcs;
}

SFC init_DOCXEnd(){
    SFC sfce = init_matchText(DOCX_END);
    sfce.displace_to_pos = strlen(DOCX_END)+20;
    return sfce;
}

bool matchEndAligned(char * in, void * args, size_t size){
    PointerInfo * info = (PointerInfo *) args;
    if(info->start_pos%info->alignment != 0) return false;
    char * val = info->val;
    for(size_t i = 0;i<size;i++) if(in[info->alignment-i-1] != val[size-i-1]) return false;
    return true;
}

SFC init_NTFSBootSector(){
    PointerInfo pi{.val=NTFS_BOOT_SECTOR_END, .start_pos=0, .alignment=NTFS_BOOT_SECTOR_END_SIZE};
    return SFC{.input_size=NTFS_BOOT_SECTOR_END_SIZE, .displace_to_pos=-NTFS_BOOT_SECTOR_END_SIZE, .fun=matchEndAligned, .args=(void *) &pi};
}

/*
SFC init_MasterFileTable(){

}

SFC init_MasterFileTableCopy(){

}*/