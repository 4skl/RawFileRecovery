#pragma once

#include <windows.h>
#include <stdio.h>
#include <vector>

typedef bool (*sign_function)(char *, void *, size_t);

struct sign_function_container {
    size_t input_size;
    long int displace_to_pos; // -... if before start, .. if after end,... indicate where to take the start&the end
    sign_function fun;
    void * args;
};
typedef struct sign_function_container SFC;

struct _sign_address {
    int sign_id;
    LARGE_INTEGER addr;
};
typedef struct _sign_address sign_address;


//File System
extern char * EFI_START_1;

//Files
extern char * PDF_START;
extern char * PNG_START;
extern char * PNG_END;

//Todo precise more (multiple types,...)
extern char * DOCX_START;
extern char * DOCX_END;

bool matchStr(char * in, void * args, size_t size);
SFC init_matchText(char * text);

SFC init_DOCXStart();
SFC init_DOCXEnd();


//File Systems

struct FileInfo {
    bool deleted;
    unsigned long created_timestam;
    unsigned long modified_timestamp;
    LARGE_INTEGER addr_start;
    size_t size;
};

struct Ftree {
    FileInfo info;
    std::vector<Ftree> subFiles;
};
typedef Ftree FileTree;

struct SystemInfo {
    size_t size;
    unsigned int clusterSize; //https://docs.microsoft.com/en-us/previous-versions/windows/it-pro/windows-server-2003/cc781134(v=ws.10)#limitations-of-cluster-sizes-on-an-ntfs-volume
};

struct FileSystem {
    FileTree files;
    struct SystemInfo info;
};

typedef std::vector<FileSystem> FilesystemList;

//NTFS Detector
struct PointerInfo{
    char * val;
    size_t start_pos;
    size_t alignment;
};

bool matchEndAligned(char * in, void * args, size_t size);


SFC init_NTFSBootSector();
SFC init_MasterFileTable();
SFC init_MasterFileTableCopy();
//file_pos getFile(FileTree file); //if file and not folder
std::vector<SFC> ntfsDetector(); // add input infos on the disk, selection of only some os