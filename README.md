# RawFileRecovery
Recover deleted files on corrupted disk using complex filters

The idea is to read as much datas as possible on the disk while applying filters in a first phase (if the filter return true, the address is saved for later usage).
If there's a disk crash while reading data, trying to get chuncks at random addresses can be useful to map unavailable sectors.
After the application of the filters, theses result (associating filters and addresses) are parsed to retrieve chunks or end and start addresses to read it and either write it as a file on the output folder or to parse and try to recover the filesystem.

:warning: Work in progress, only raw file reading SE is poorly working

## Roadmap
File get SE :heavy_check_mark:
NTFS File system :repeat:
Regex get SE :x:
FAT File system :x:
Others file system :x:
GUI :x:
