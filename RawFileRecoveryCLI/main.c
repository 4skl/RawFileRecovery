#include <windows.h>
#include <stdio.h>
#include <getopt.h>
#include <vector>
#include <string>
#include <iostream>
#include "search.h"

const size_t SECTOR_MIN_SIZE = 512;

int main(int argc, char ** argv)
{
    //Parse args
    std::string filePath; //https://support.microsoft.com/fr-fr/help/100027/info-direct-drive-access-under-win32
                                    //https://docs.microsoft.com/en-us/windows/win32/fileio/naming-a-file#win32-file-namespaces
    std::string outputPath("not0_out.txt");

	int byte_searched = 0; //int ?
    int byte_replaced = -1; //if < 0 invalid

    int min_chunck_size = 1;
    int chunck_size = 256;

    int buffer_size = 8192;

    
    printf("Start option parsing\n");
    bool arg_error = false;

    int option = 0;
    int option_index = 0;
    while ((option = getopt(argc, argv, "P:D:V:C:b:c:s:mn:e:o:")) != -1){
        int argi = optind-1;
        switch (option){
            case 'P':
                if(filePath.empty()) filePath.append(optarg);
                break;

            case 'D':
            if(filePath.empty()){
                    filePath.append("\\\\.\\PhysicalDrive"); //Disk ??
                    filePath.append(optarg); 
            }
                break;

            case 'V':
            if(filePath.empty()){
                filePath.append("\\\\.\\");
                filePath.append(optarg); //[0]
                filePath.append(":");
            }
                break;

            case 'C':
            if(filePath.empty()){
                filePath.append("\\\\.\\CdRom");
                filePath.append(optarg);
            }
                break;
            
            case 'b':
                if(argv[argi][2] == 's'){
                    byte_searched = std::stoi(argv[argi+1]);
                }else if(argv[argi][2] == 'r'){
                    byte_replaced = std::stoi(argv[argi+1]);
                }else{
                    int v = std::stoi(argv[argi+1]);
                    if(v>0){
                        buffer_size = v;
                    }else{
                        printf("-b (buffer size) should be greater than 0\n");
                        arg_error = true;
                    }
                }
                break;
            
            case 'c':
                if(argv[argi][2] == 's'){
                    if(argv[argi][3] == 'r'){
                        int v = std::stoi(argv[argi+1]);
                        if(v>0){
                            min_chunck_size = v;
                        }else{
                            printf("-csr should be greater than 0\n");
                            arg_error = true;
                        }
                    }else if(argv[argi][3] == '\0'){
                        int v = std::stoi(argv[argi+1]);
                        if(v>0){
                            chunck_size = v;
                        }else{
                            printf("-cs should be greater than 0\n");
                            arg_error = true;
                        }
                    }else{
                        arg_error = true;
                    }
                }
                break;
            
            case 'f':
                    //TODO read filter file and parse it
                break;

            case 's':
                if(argv[argi][2] == 'f'){
                    //todo
                }
                break;

            case 'o':
                outputPath.append(optarg);
                break;

            case 'h':
                arg_error = true;
                break;
            default:
                arg_error = true;
                break;
        }
    }

    if(filePath.empty()){
        arg_error = true;
    }

    if(arg_error){
        const char * extraOptions = "-bs <byte removed (default : 0)> | Activated by default\n"
                                    "-br <byte replaced> | Replace each bytes removed by this one\n"
                                    "-csr <min chunk size remove> | Remove area if more than chunck size bytes following\n"
                                    "-cs <chunks size (default : 256)> | Min size for the datas gathered\n"
                                    
                                    "-f <filter file path> | Search by using the filter contained in the file\n"
                                    "-s[f|x] <text|file path> | Store data (of length chunck size or till end if reached) if ASCII (UTF-8?) or bytes of the file (f arg should be added) are in the input file\n"
                                    "-e[f|x] <text|file path> | Stop data storing if ASCII text (or bytes of the hexadecimal text value if x) or bytes of the file are found in the input file\n"
                                    "-m | Can find a file in a file (Continue to search a start even if a file is already recording)\n"
                                    "-n | Name each output file by the index of it's start (create multiples files)\n"
                                    "-b <reading buffer size (default : 8192)> | Size of the buffer collecting data from the device/file\n";
        
        printf("Usage : %s <[-P <file path>] [-D <disk number>] [-V <volume letter>] [-C <cdrom number>]> [-o <output file (default : not0_out.txt)>]\nExtra options :\n%s", argv[0], extraOptions);
        return 1;
    }

    //Check args (print what will be done)
    std::cout << "Args parsed " << arg_error << std::endl;

    //Main code : search file
    size_t filePathSZ = filePath.size();
    wchar_t * filePathW = (wchar_t *) malloc(sizeof(wchar_t)*filePathSZ);
    mbstowcs(filePathW, filePath.c_str(), filePathSZ);

    wprintf(L"Input file : %ls\n", filePathW);

    testSearch(filePathW);

    free(filePathW);
    return 0;
}