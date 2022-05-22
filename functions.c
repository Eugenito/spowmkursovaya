#include "functions.h"
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>


int getFile(char* fromPath, char* toPath){
  DIR *dir;
  int skipFrom = 0;
  int skipTo = 0;
  struct dirent *entry;
  FILE* inptr;
  FILE* outptr;
  if(!(inptr=fopen(fromPath, "r"))) {
    printf ("Cannot open file %s\n", fromPath);
    return 1;
  }
      
  if(!(outptr=fopen(toPath, "w"))) {
    printf ("Cannot open file %s\n", toPath);
    return 1;
  }
      
  while(!feof(inptr)){
    char ch;
    fread(&ch, sizeof(char), 1, inptr);
    fwrite(&ch, sizeof(char), 1, outptr);
  }
  fclose(inptr);
  fclose(outptr);    
  return skipFrom && skipTo;
}

char* extractFileName(char* path){
  int count = 0;
  char* buf;
  int j = 0;
  for (int i = strlen(path)-1; i > 0; i--){
    if ((int)path[i] != 47){
      count++;
    }
    else{
      break;
    }
  }
  buf = (char*)malloc(strlen(path) - (strlen(path)-count));
  for (int i = strlen(path)-count; i < strlen(path); i++){
    //printf("ch - %c", path[i]);
    buf[j] = path[i];
    j++;
  }
  buf[j] = '\0';
  return buf;
}

static int rootDirFlag = 0;
static char buffer[200];
static char bufferForFiles[200];

void checkFileType(char* path, char* dest){
  int skip = 0;
  DIR *dir;
  struct dirent *entry;
  int flag = 0;
  size_t len1 = strlen(path);
  size_t len2 = strlen(dest);
  if (!(dir = opendir(path))) {
      if(getFile(path, dest)){
        printf("Error while opening file\n");
      }
  }
  else{                                                 
    if (!rootDirFlag){
      if (mkdir(dest, 0777)){
        perror("mkdir failed");
      }
      rootDirFlag = 1;
    }
    char new_path[200];
      while ((entry = readdir(dir)) != NULL) {
        char* name = entry->d_name;
        if (entry->d_type == DT_REG){
          char* name1 = (char*)malloc(sizeof(path)+sizeof(entry->d_name)+1);
          char* name2 = (char*)malloc(sizeof(dest)+sizeof(entry->d_name)+1);
          strcpy(name1, path);
          strcat(name1, "/");
          strcat(name1, entry->d_name);
          strcpy(name2, dest);
          strcat(name2, "/");
          strcat(name2, entry->d_name);
          getFile(name1, name2);
          free(name1);
          free(name2);
        }
        else if (entry->d_type == DT_DIR){
          if (!strcmp(name, ".") || !strcmp(name, "..")){
              continue;
          }
            strcat(buffer, dest);
            strcat(bufferForFiles, path);
            dest[len2] = '/';
            strcpy(dest + len2 + 1, name);

            mkdir(dest, 0777);

            path[len1] = '/';
            strcpy(path + len1 + 1, name);
            checkFileType(path, dest);
            path[len1] = '\0';
            dest[len2] = '\0';
        }
        }
      }
}

int showFilesInDirectory(char* current_path){
  struct dirent *entry;
  DIR* dir;
  int count = 0;
  if (!(dir = opendir(current_path))) {           
    fprintf(stderr, "cannot open directory: %s\n", current_path); 
    return 1;
  }
  else{
    while ((entry = readdir(dir)) != NULL) {
      if (entry->d_type == DT_DIR){
        char* name = entry->d_name;
        if (!strcmp(name, ".") || !strcmp(name, "..")){
            continue;
        }
        if (!strncmp(name, ".Trash", 6)){
          continue;
        }
        count++;
        printf("\x1b[34m%s\x1b[0m\n", entry->d_name);
      }
      else if (entry->d_type == DT_REG){
        count++;
        printf("\x1b[32m%s\x1b[0m\n", entry->d_name);
      }
    }
  }
  return count;
}

char* changeDirectory(char* current_path, char* offset, char* point_path){
  char* tmp = (char*)malloc(sizeof(current_path) + sizeof(offset)+10);
  int flag = 0;
  int count = 0;
  int skip = 0;
  DIR* dir;
  int num;
  int pointLength = strlen(point_path);
  int currentPathLength = strlen(current_path);
  if (strcspn(offset, "..") != strlen(offset) || strcspn(offset, ".") != strlen(offset)){
    skip = 1;
  }
  if (!skip){
    strcpy(tmp, current_path);
    strcat(tmp, "/");
    strcat(tmp, offset);
    if (dir = opendir(tmp) == NULL){
      printf("There is no such directory: %s\n", tmp);
    }
    else{
      strcpy(current_path, tmp);
    }
  }
  if (!strcmp(offset, "..")){
    if (currentPathLength > pointLength){
      for (int i = strlen(current_path); i > 0; i--){
      if ((int)current_path[i] == 47 && !flag){
        flag = 1;
        continue;
      }
      if (flag){
        count++;
        tmp[i] = current_path[i];
      }
    }
    tmp[0] = current_path[0];
    count++;
    tmp[count] = '\0';
    strcpy(current_path, tmp);
    }
    else{
      printf("You can't leave mount point folder\n");
    }
  }
  //closedir(dir);
  return current_path;
}

char* getOffsetInChangeDir(char* str){
  int count = 0;
  char* tmp = (char*)malloc(sizeof(str));
  for (int i = 3; i < strlen(str); i++){
    count++;
    tmp[i-3] = str[i];
  }
  tmp[count] = '\0';
  return tmp;
}
