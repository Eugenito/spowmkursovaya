#pragma once
int getFile(char* fromPath, char* toPath);
char* getOffsetInChangeDir(char* str);
char* changeDirectory(char* current_path, char* offset, char* point_path);
int showFilesInDirectory(char* current_path);
void checkFileType(char* path, char* dest);
char* extractFileName(char* path);