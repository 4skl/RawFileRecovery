#include "search.h"

//?v include theses too needed ? 
#include <windows.h>
#include <stdio.h>
#include <vector>

struct sign_function_container {
    size_t input_size;
    sign_function fun;
    char * extension;
};

struct sign_address {
    int sign_id;
    LARGE_INTEGER addr;
};

bool matchStr(char * in, const char * val, size_t size){
    for(size_t i = 0;i<size;i++) if(in[i] != val[i]) return false;
    return true;
}

bool matchHello(char * in){ // Test match
    const char * text = "Hello";
    return matchStr(in, text, 5);
}

bool matchTest1(char * in){
    const char * text = "EFI PART";
    return matchStr(in, text, 8);
}

bool matchPDF(char * in){
    const char * text = "%PDF";
    return matchStr(in, text, 4);
}

bool matchPNGStart(char * in){
    const char * text = "\x89PNG\x0D\x0A\x1A\x0A";
    return matchStr(in, text, 8);
}

bool matchPNGEnd(char * in){
    const char * text = "IEND\xAE\x42\x60\x82";
    return matchStr(in, text, 8);
}

void searchAndWrite(std::vector<SFC> starts, std::vector<SFC> ends){
    printf("searchAndWrite\n");
    HANDLE file = CreateFileW(filePath, FILE_GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL); //File share write ?

    if(file == INVALID_HANDLE_VALUE)
    {
        fwprintf(stderr, L"Error on method CreateFileW(%ls), error code : %ld\n",filePath,GetLastError());
        return;
    }
    LARGE_INTEGER endPos;
    if(!GetFileSizeEx(file,&endPos)){
        fprintf(stderr, "Error on method GetFileSizeEx(), error code : %ld\n",GetLastError());
        printf("Used size : -1 (read till end)\n");
        endPos.QuadPart = -1;
    }else{
        printf("Size : %lld\n", endPos.QuadPart);
    }

    LARGE_INTEGER startPos;
    startPos.QuadPart = 0;

    std::vector<sign_addr> addresses_out;
    std::vector<SFC> sign_functions(starts);
    sign_functions.insert( sign_functions.end(), ends.begin(), ends.end() );

    SFC sfc{.input_size=4, .fun=matchPDF};
    functions_check.push_back(sfc);
    SFC sfc2{.input_size=8, .fun=matchTest1};
    functions_check.push_back(sfc2);
    SFC sfc3{.input_size=8, .fun=matchPNGStart};
    functions_check.push_back(sfc3);
    SFC sfc4{.input_size=8, .fun=matchPNGEnd};
    functions_check.push_back(sfc4);


    bool success = searchSignatureAddresses(addresses_out, functions_check, startPos, endPos, file, 8192);
    printf("Success %d\n", success);
    for(size_t i = 0;i<addresses_out.size();i++){
        printf("@%lld : sign %d\n", addresses_out[i].addr, addresses_out[i].sign_id);
    }
    printf("End test\n");
    CloseHandle(file);
}

void testSearch(const wchar_t * filePath){
    printf("Test Search\n");
    HANDLE file = CreateFileW(filePath, FILE_GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL); //File share write ?

    if(file == INVALID_HANDLE_VALUE)
    {
        fwprintf(stderr, L"Error on method CreateFileW(%ls), error code : %ld\n",filePath,GetLastError());
        return;
    }
    LARGE_INTEGER endPos;
    if(!GetFileSizeEx(file,&endPos)){
        fprintf(stderr, "Error on method GetFileSizeEx(), error code : %ld\n",GetLastError());
        printf("Used size : -1 (read till end)\n");
        endPos.QuadPart = 1<<28;//-1;
    }else{
        printf("Size : %lld\n", endPos.QuadPart);
    }

    LARGE_INTEGER startPos;
    startPos.QuadPart = 0;

    std::vector<sign_addr> addresses_out;
    std::vector<SFC> functions_check;

    SFC sfc{.input_size=4, .fun=matchPDF};
    functions_check.push_back(sfc);
    SFC sfc2{.input_size=8, .fun=matchTest1};
    functions_check.push_back(sfc2);
    SFC sfc3{.input_size=8, .fun=matchPNGStart};
    functions_check.push_back(sfc3);
    SFC sfc4{.input_size=8, .fun=matchPNGEnd};
    functions_check.push_back(sfc4);


    bool success = searchSignatureAddresses(addresses_out, functions_check, startPos, endPos, file, 8192);
    printf("Success %d\n", success);
    for(size_t i = 0;i<addresses_out.size();i++){
        printf("@%lld : sign %d\n", addresses_out[i].addr, addresses_out[i].sign_id);
    }
    printf("End test\n");
    CloseHandle(file);
}

bool searchSignatureAddresses(std::vector<sign_addr> &addresses, std::vector<SFC> &sign_functions, LARGE_INTEGER startPos, LARGE_INTEGER endPos, HANDLE file, size_t buffer_size){
    buffer_size += buffer_size%2; //buffer size should be even
    printf("Search start\n");
    size_t max_in_size = 0;
    for(size_t i = 0;i<sign_functions.size();i++){
        size_t sign_size = sign_functions[i].input_size;
        if(sign_size>max_in_size){
            max_in_size = sign_size;
        }
    }
    if(max_in_size*2>buffer_size){//? +1 ?
        buffer_size = max_in_size*2;
    }
    char * buf = (char *) malloc(sizeof(char)*buffer_size);

    DWORD nRead;
    if (SetFilePointerEx(file, startPos, NULL , FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
        fprintf(stderr, "Error on method SetFilePointer(), error code : %ld\n",GetLastError());
        return false;
    }
    printf("buffer size : %d\n",buffer_size);
    if (!ReadFile(file, buf, buffer_size, &nRead, NULL)) {//load first full buffer
        fprintf(stderr, "Error on method ReadFile(), error code : %ld\n",GetLastError());
        return false;
    }
    printf("Iteration start\n");
    unsigned long long r_c = 0;
    LARGE_INTEGER i{.QuadPart = 0};
    do{
        size_t to_read = buffer_size/2; //rolling buffer
        if(to_read>nRead) to_read = nRead;
        //Test signature functions till start reach half buffer
        for(size_t j = 0;j<to_read;j++){
            //printf("%x", *(buf+j));
            for(int sign_id = 0;sign_id<sign_functions.size();sign_id++){
                SFC fun_c = sign_functions[sign_id];
                if(fun_c.fun(buf+j)){
                    LARGE_INTEGER t_addr = i;
                    t_addr.QuadPart += j;
                    sign_addr sa{.sign_id=sign_id, .addr=t_addr};
                    addresses.push_back(sa);
                    printf("Found %d @ %lld\n", sign_id, t_addr.QuadPart);//! error on pos +4096??
                }
            }
        }
        //printf("\n");

        //Move second half to the first half
        memcpy(buf,buf+nRead, nRead);

        //read second half of buffer : now buffer correspond to the file starting at startPos
        i.QuadPart+=to_read;
        if (SetFilePointerEx(file, LARGE_INTEGER{.QuadPart=i.QuadPart+to_read}, NULL , FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
            fprintf(stderr, "Error on method SetFilePointer(), error code : %ld\n",GetLastError());
            return false;
        }

        if (!ReadFile(file, buf+to_read, buffer_size-to_read, &nRead, NULL)) { //Check last readed ?
		    fprintf(stderr, "Error on method ReadFile(), error code : %ld\n",GetLastError());
            return false;
	    }
    }while((endPos.QuadPart == -1 || i.QuadPart<endPos.QuadPart) && nRead);

    free(buf);
    return true;
}