#pragma once

#include <windows.h>
#include <stdio.h>
#include <vector>

#include "parse_filter.h"

typedef struct _file_pos file_pos;

bool copyToFile(file_pos fp, char * output_path, HANDLE file, DWORD buffer_size=8192);
size_t matchFileAddr(std::vector<file_pos> &file_positions_out, std::vector<sign_address> &addresses, std::vector<SFC> &functions_check);
size_t matchFileAddrM(std::vector<file_pos> &file_positions_out, std::vector<sign_address> &addresses, std::vector<SFC> &functions_check);
void searchSE(const char * output_folder, std::vector<SFC> starts, std::vector<SFC> ends, size_t max_size=1<<30);
void searchCommonFiles(const wchar_t * filePath, const char * output_folder);
void testSearch(const wchar_t * filePath);
bool searchSignatureAddresses(std::vector<sign_address> &addresses, std::vector<SFC> &sign_functions, LARGE_INTEGER startPos, LARGE_INTEGER endPos, HANDLE file, size_t buffer_size = 8192);