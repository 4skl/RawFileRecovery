#include <windows.h>
#include <stdio.h>
#include <vector>

typedef bool (*sign_function)(char *);

typedef struct sign_function_container SFC;

typedef struct sign_address sign_addr;

bool matchHello(char * in);
void searchAndWrite(std::vector<SFC> starts, std::vector<SFC> ends);
void testSearch(const wchar_t * filePath);
bool searchSignatureAddresses(std::vector<sign_addr> &addresses, std::vector<SFC> &sign_functions, LARGE_INTEGER startPos, LARGE_INTEGER endPos, HANDLE file, size_t buffer_size = 8192);