#include "search.h"

struct _file_pos {
    int st_sign_id;
    char * filename = NULL;
    char * filename_extension = NULL;
    LARGE_INTEGER addr_start;
    LARGE_INTEGER addr_end;
};

bool searchSignatureAddresses(std::vector<sign_address> &addresses, std::vector<SFC> &sign_functions, LARGE_INTEGER startPos, LARGE_INTEGER endPos, HANDLE file, size_t buffer_size){
    buffer_size += buffer_size%2; //buffer size should be even
    printf("Search start\n");
    size_t max_in_size = 0;
    for(SFC &sfc : sign_functions){
        size_t sign_size = sfc.input_size;
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
    LARGE_INTEGER i{.QuadPart = 0};
    do{
        size_t to_read = buffer_size/2; //rolling buffer
        if(to_read>nRead) to_read = nRead;
        //Test signature functions till start reach half buffer
        for(size_t j = 0;j<to_read;j++){
            //printf("%x", *(buf+j));
            for(int sign_id = 0;sign_id<sign_functions.size();sign_id++){
                SFC fun_c = sign_functions[sign_id];
                if(fun_c.fun(buf+j, fun_c.args, fun_c.input_size)){
                    LARGE_INTEGER t_addr = i;
                    t_addr.QuadPart += j;
                    sign_address sa{.sign_id=sign_id, .addr=t_addr};
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

bool copyToFile(file_pos fp, char * output_path, HANDLE file, DWORD buffer_size){
    //todo things with the handle to write
    size_t file_size = fp.addr_end.QuadPart-fp.addr_start.QuadPart;
    HANDLE file_out = CreateFile(output_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_WRITE_THROUGH, NULL);
    if(file_out == INVALID_HANDLE_VALUE)
    {
        fwprintf(stderr, L"Error on method CreateFile(%s), error code : %ld\n",output_path,GetLastError());
        return false;
    }
    DWORD nRead, nWrite;
    char buf[buffer_size];
    LARGE_INTEGER i;
    i.QuadPart = 0;
    while(i.QuadPart<file_size){
        DWORD rSize = buffer_size;
        if(i.QuadPart+rSize > file_size){
            rSize = file_size-i.QuadPart;
        }
        LARGE_INTEGER in_start{.QuadPart=fp.addr_start.QuadPart+i.QuadPart};
        in_start.QuadPart-=in_start.QuadPart%512;
        
        if (SetFilePointerEx(file, in_start, NULL , FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
            fprintf(stderr, "Error on method SetFilePointer(), error code : %ld\n",GetLastError());
            CloseHandle(file_out);
            return false;
        }
        if (!ReadFile(file, buf, buffer_size, &nRead, NULL)) {//! error 87 there 
            fprintf(stderr, "Error on method ReadFile(), error code : %ld\n",GetLastError());
            CloseHandle(file_out);
            return false;
        }

        if (SetFilePointerEx(file_out, i, NULL , FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
            fprintf(stderr, "Error on method SetFilePointer(), error code : %ld\n",GetLastError());
            CloseHandle(file_out);
            return false;
        }

        if(!WriteFile(file_out,buf+in_start.QuadPart%512,rSize,&nWrite,NULL)){
            fprintf(stderr, "Error on method WriteFile(%s), error code : %ld\n",file_out,GetLastError());
            CloseHandle(file_out);
            return false;
        }
        i.QuadPart += nWrite;
        //? if(nRead == 0) break;
    }
    CloseHandle(file_out);
    return true; 
}

//addresses are sorted
size_t matchFileAddr(std::vector<file_pos> &file_positions_out, std::vector<sign_address> &addresses, std::vector<SFC> &functions_check){
    size_t end_start = functions_check.size()/2;
    for(size_t i = 0;i<addresses.size()-1;i++) {
        if(addresses[i].sign_id<end_start){ //start address
            sign_address start_addr = addresses[i];
            start_addr.addr.QuadPart+=functions_check[start_addr.sign_id].displace_to_pos;
            //for(size_t k = i; k < addresses.size();k++){
                if(addresses[i+1].sign_id-end_start == start_addr.sign_id){ //If addresses is start -> end
                    sign_address end_addr = addresses[i+1];
                    LARGE_INTEGER end_addr_v = end_addr.addr;
                    end_addr_v.QuadPart+=functions_check[end_addr.sign_id].displace_to_pos;
                    file_pos fp;
                    fp.st_sign_id=start_addr.sign_id;
                    fp.addr_start=start_addr.addr;
                    fp.addr_end=end_addr_v;

                    file_positions_out.push_back(fp);
                    //break;
                }
            //}
        }
    }
    return file_positions_out.size();
}

size_t matchFileAddrM(std::vector<file_pos> &file_positions_out, std::vector<sign_address> &addresses, std::vector<SFC> &functions_check){
    size_t end_start = functions_check.size()/2;
    for(size_t i = 0;i<addresses.size()-1;i++) {
        if(addresses[i].sign_id<end_start){ //start address
            sign_address start_addr = addresses[i];
            start_addr.addr.QuadPart+=functions_check[start_addr.sign_id].displace_to_pos;
            for(size_t k = i; k < addresses.size();k++){
                if(addresses[k].sign_id-end_start == start_addr.sign_id){ //If addresses is start -> end
                    sign_address end_addr = addresses[k];
                    LARGE_INTEGER end_addr_v = end_addr.addr;
                    end_addr_v.QuadPart+=functions_check[end_addr.sign_id].displace_to_pos;
                    file_pos fp;
                    fp.st_sign_id=start_addr.sign_id;
                    fp.addr_start=start_addr.addr;
                    fp.addr_end=end_addr_v;

                    file_positions_out.push_back(fp);
                    break;
                }
            }
        }
    }
    return file_positions_out.size();
}

//match the first start address with the first end found, then move to the position of the end
size_t matchFileAddrL(std::vector<file_pos> &file_positions_out, std::vector<sign_address> &addresses, std::vector<SFC> &functions_check){
    size_t end_start = functions_check.size()/2;
    for(size_t i = 0;i<addresses.size()-1;i++) {
        if(addresses[i].sign_id<end_start){ //start address
            sign_address start_addr = addresses[i];
            start_addr.addr.QuadPart+=functions_check[start_addr.sign_id].displace_to_pos;
            for(size_t k = i; k < addresses.size();k++){
                if(addresses[k].sign_id-end_start == start_addr.sign_id){ //If addresses is start -> end
                    sign_address end_addr = addresses[k];
                    LARGE_INTEGER end_addr_v = end_addr.addr;
                    end_addr_v.QuadPart+=functions_check[end_addr.sign_id].displace_to_pos;
                    file_pos fp;
                    fp.st_sign_id=start_addr.sign_id;
                    fp.addr_start=start_addr.addr;
                    fp.addr_end=end_addr_v;

                    file_positions_out.push_back(fp);
                    i+=k;
                    break;
                }
            }
        }
    }
    return file_positions_out.size();
}


//output_file == NULL => don't write file search only
void searchSE(const wchar_t * filePath, const char * output_folder, char ** extensions, std::vector<SFC> starts, std::vector<SFC> ends, size_t max_size=1<<26){ //verify starts[i] -> ends[i]
    printf("Search SE\n");
    HANDLE file = CreateFileW(filePath, FILE_GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL); //File share write ?

    if(file == INVALID_HANDLE_VALUE)
    {
        fwprintf(stderr, L"Error on method CreateFileW(%ls), error code : %ld\n",filePath,GetLastError());
        return;
    }
    LARGE_INTEGER endPos;
    if(!GetFileSizeEx(file,&endPos)){
        fprintf(stderr, "Error on method GetFileSizeEx(), error code : %ld\n",GetLastError());
        endPos.QuadPart = -1; //todo make it as an arg
        printf("Used size : %lld (read till end)\n", endPos.QuadPart);
    }else{
        printf("Size : %lld\n", endPos.QuadPart);
    }

    LARGE_INTEGER startPos;
    startPos.QuadPart = 0LL;

    std::vector<SFC> sign_functions(starts);
    sign_functions.insert( sign_functions.end(), ends.begin(), ends.end() );
    LARGE_INTEGER actualSt = startPos;
    bool success = false;
    do{

        std::vector<sign_address> addresses_out;
        LARGE_INTEGER actualEnd{.QuadPart=actualSt.QuadPart+max_size};
        if(actualEnd.QuadPart>endPos.QuadPart) actualEnd = endPos;
        success = searchSignatureAddresses(addresses_out, sign_functions, actualSt, actualEnd, file, 1<<14); //Buffer 16Ko for perfs increase size, but bugs can occur
        printf("Success %d\n", success);
        /*for(size_t i = 0;i<addresses_out.size();i++){
            file_pos fp{.};
            printf("@%lld : sign %d\n", addresses_out[i].addr, addresses_out[i].sign_id);
        }*/
        std::vector<file_pos> file_positions_out;
        size_t filesFound = matchFileAddrL(file_positions_out,addresses_out,sign_functions);
        printf("%lld files found\n", filesFound);
        if(output_folder != NULL && filesFound){
            size_t i = 0;
            for(const auto& file_p: file_positions_out){
                size_t file_size = file_p.addr_end.QuadPart-file_p.addr_start.QuadPart;
                if(file_size<max_size){
                    char output_file[strlen(output_folder)+50];//!
                    sprintf(output_file,"%s/at%lld_found%lld.%s", output_folder, file_p.addr_end.QuadPart, i, extensions[file_p.st_sign_id]);
                    if(!copyToFile(file_p,output_file,file)){
                        fprintf(stderr, "Error on copying file found %s\n",output_file);
                    }
                }
                i++;
            }
        }
        actualSt.QuadPart += max_size;
    }while((endPos.QuadPart == -1 || actualSt.QuadPart<endPos.QuadPart) && success);
    
    CloseHandle(file);

}

void searchCommonFiles(const wchar_t * filePath, const char * output_folder){
    std::vector<SFC> starts;
    std::vector<SFC> ends;
    starts.push_back(init_DOCXStart());
    ends.push_back(init_DOCXEnd());

    char ** extensions = (char **) malloc(sizeof(char *)*1);
    extensions[0] = "docx";

    searchSE(filePath, output_folder, extensions, starts, ends);
}


FilesystemList searchFileSystems(const wchar_t * filePath, size_t diskSize = 0LL){
    size_t max_size = 1LL<<14;
    FilesystemList fsl;
    printf("Search Filesystem\n");
    HANDLE file = CreateFileW(filePath, FILE_GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL); //File share write ?

    if(file == INVALID_HANDLE_VALUE)
    {
        fwprintf(stderr, L"Error on method CreateFileW(%ls), error code : %ld\n",filePath,GetLastError());
        return fsl;
    }
    LARGE_INTEGER endPos;
    if(!GetFileSizeEx(file,&endPos)){
        fprintf(stderr, "Error on method GetFileSizeEx(), error code : %ld\n",GetLastError());
        endPos.QuadPart = -1; //todo make it as an arg
        printf("Used size : %lld (read till end)\n", endPos.QuadPart);
    }else{
        printf("Size : %lld\n", endPos.QuadPart);
    }

    LARGE_INTEGER startPos;
    startPos.QuadPart = 0LL;

    std::vector<SFC> sign_functions;
    sign_functions.push_back(init_NTFSBootSector());
    LARGE_INTEGER actualSt = startPos;
    bool success = false;
    do{

        std::vector<sign_address> addresses_out;
        LARGE_INTEGER actualEnd{.QuadPart=actualSt.QuadPart+max_size};
        if(actualEnd.QuadPart>endPos.QuadPart) actualEnd = endPos;
        success = searchSignatureAddresses(addresses_out, sign_functions, actualSt, actualEnd, file, 1<<14); //Buffer 16Ko for perfs increase size, but bugs can occur
        printf("Success %d\n", success);
        /*for(size_t i = 0;i<addresses_out.size();i++){
            file_pos fp{.};
            printf("@%lld : sign %d\n", addresses_out[i].addr, addresses_out[i].sign_id);
        }*/

        actualSt.QuadPart += max_size;
    }while((endPos.QuadPart == -1 || actualSt.QuadPart<endPos.QuadPart) && success);
    
    CloseHandle(file);
    return fsl;
}

//test
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
        endPos.QuadPart = 1<<28;//-1; //!
    }else{
        printf("Size : %lld\n", endPos.QuadPart);
    }

    LARGE_INTEGER startPos;
    startPos.QuadPart = 0;

    std::vector<sign_address> addresses_out;
    std::vector<SFC> functions_check;

    SFC sfc = init_matchText(PDF_START);
    functions_check.push_back(sfc);
    SFC sfc2 = init_matchText(PNG_START);
    functions_check.push_back(sfc2);
    SFC sfc3 = init_matchText(PNG_END);
    functions_check.push_back(sfc3);


    bool success = searchSignatureAddresses(addresses_out, functions_check, startPos, endPos, file, 8192);
    printf("Success %d\n", success);
    for(size_t i = 0;i<addresses_out.size();i++){
        printf("@%lld : sign %d\n", addresses_out[i].addr, addresses_out[i].sign_id);
    }
    printf("End test\n");
    CloseHandle(file);
}