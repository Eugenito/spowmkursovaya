#include "./functions.h"
#include <dirent.h>
#include<stdio.h>	
#include<string.h>	
#include<errno.h>
#include<stdlib.h>
#include<unistd.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <errno.h>
#include <sys/wait.h>

char* dev = "enx";
static char gateway[20];
static char* skip_ip = {"0.0.0.0"};
static int num_of_ip_adresses = 0;


char* hex_to_dec(char buf[]){
  int point = 0;
  memset(gateway, 0, sizeof(gateway));
  int next_flag = 0;
  char dec_buf[3];
  for(int i = 0; i < strlen(buf)/2; i++){
      int j = strlen(buf)-1-i;
      char bf = buf[i];                           
      buf[i] = buf[j];       
      buf[j] = bf;                                
  }
  for (int i = 0; i < strlen(buf)-1; i = i + 2){
    int j = i + 1;
    char bf = buf[i];
    buf[i] = buf[j];
    buf[j] = bf;
  }
  int sum = 0;
  for (int i = 0; i < strlen(buf); i++){
    //int j = i + 1;
    if (next_flag){
      sum = 0;
    }
    next_flag = 0;
    int hex_in_dec = 0;
    hex_in_dec = ((int)buf[i]>=65 && (int)buf[i]<=70) ? (int)buf[i] + 10 - 'A' : (int)buf[i] - '0';
    if ((i+1)%2==0){
      sum = sum*16+hex_in_dec;
      next_flag = 1;
      //
      if (sum <10){
        char symbol = sum + '0'; 
        dec_buf[0] = symbol;
      }
      else{
        int count = 0;
        do {
          int digit = sum % 10;
          dec_buf[count++] = (digit > 9) ? digit - 10 +'A' : digit + '0';
        } while ((sum /= 10) != 0);
        dec_buf[count] = '\0';
        int i;
        for (i = 0; i < count / 2; ++i) {
          char symbol = dec_buf[i];
          dec_buf[i] = dec_buf[count - i - 1];
          dec_buf[count - i - 1] = symbol;
        }
      }
      point++;
      strcat(gateway, dec_buf);
      if (point !=4){
        strcat(gateway, ".");
      }
      memset(dec_buf, 0, sizeof(dec_buf));
   
      //
    }
    else{
      sum = hex_in_dec;
    }
  }
  return gateway;
}

char* extract_gateway(){
  FILE* f;
  int ip_choice;
  pid_t pid;
  int file_size = 0;
  int word_size = 0;
  int x = 1;
  int flag = 0;
  int skip = 0;
  char* str = (char*)malloc(15);
  char** gateway_ip = NULL;
  int status;
  int tab_flag = 0;
  char interface_buf[3];
  char gateway_buf[8];
  char** available_ips = (char**)malloc(1);
  for (int i = 0; i < 1; i++){
    available_ips[i] = (char*)malloc(20);
  }
  if (!(f = fopen("/proc/net/route", "r"))){
    printf("we");
  }
  while(!feof(f)){
    int space_counter = 0;
    if (!flag){
      while(1){
        char ch = fgetc(f);
        if (feof(f)){
          break;
        }
        if(ch == ' '){
          space_counter++;
        }
        else if (ch == '\n'){
          continue;
        }
        else{
          if ((ch == 'e') && space_counter>10){
            ch = fgetc(f);
            if (ch == 'n'){
              ch = fgetc(f);
              if (ch == 'x'){
                fseek(f, -3, SEEK_CUR);
                flag = 1;
                break;
              }
            }
          }
          else{
            space_counter = 0;
          }
        }
      }
    }
    memset(interface_buf, 0, sizeof(interface_buf)+1);
    fread(&interface_buf, sizeof(char), 3, f);
    if (strcmp(interface_buf, dev) != 0){
      break;
    }
    else{
      flag = 0;
      while(tab_flag != 1 && !feof(f)){
        char ch2 = fgetc(f);
        if (ch2 != '\t'){
          fseek(f, 1, SEEK_CUR);
        }else{
          tab_flag++;
        }
      }
      tab_flag = 0;
      fread(&gateway_buf, sizeof(char), 8, f);
      char* gateway_ip = hex_to_dec(gateway_buf);
      if (strcmp(gateway_ip, skip_ip) == 0){
        continue;
      }
      else{
        num_of_ip_adresses++;
        available_ips = (char**)realloc(available_ips, num_of_ip_adresses+1);
        available_ips[num_of_ip_adresses] = (char*)malloc(20);
        strcpy(available_ips[num_of_ip_adresses-1], gateway_ip);
      }
    }
  }
  fclose(f);
  if (num_of_ip_adresses != 0){
    printf("Choose one of available adresses:\n");
    for (int i = 0; i < num_of_ip_adresses; i++){
      printf("%d - %s\n", i+1, available_ips[i]);
    }
    scanf("%d", &ip_choice);
  }
  else{
    printf("There are no available devices\n");
    exit(1);
  }

  return available_ips[ip_choice-1];
}

char* mount_folder(char* ip){
  int status;
  int result;
  char name[30];
  char full_pass[100];
  char* folder_name = (char*)malloc(50);
  char* mount_point = (char*)malloc(50);
  printf("Enter folder name: ");
  scanf("%s", folder_name);
  printf("Enter mount point full pass: ");
  scanf("%s", mount_point);
  strcat(full_pass, "//");
  strcat(full_pass, ip);
  strcat(full_pass, "/share");
  pid_t pid;
  char* args[] = {
    "sh",
    "./foldermounter.sh",
    mount_point,
    "alex",
    ip,
    folder_name,
    NULL
  };
  pid = fork();
  if (pid == 0){
    execv("/bin/sh", args);
  }
  wait(&status);
  return mount_point;
}

void unmount_folder(char* pass){
  int status;
  int result;
  pid_t pid;
  char* args[] = {
    "sh",
    "./folderumounter.sh",
    pass,
    NULL
  };
  pid = fork();
  if (pid == 0){
    execv("/bin/sh", args);
  }
  wait(&status);
}

int checkIfMounted(char* current_path){
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
      }
      else if (entry->d_type == DT_REG){
        count++;
      }
    }
  }
  return count;
}

int main(){
  int fl = 0;
  char* current_dir = (char*)malloc(200);
  char mount_ip[20];
  char path[50];
  char file_name[50];
  strcpy(mount_ip, extract_gateway());
  strcpy(path, mount_folder(mount_ip));
  strcpy(current_dir, path);
  if (!checkIfMounted(current_dir)){
    rmdir(current_dir);
    return 0;
  }

  while(1){
    char* command = (char*)malloc(200);
    if (fl){
      printf("%s>", current_dir);
      fflush(stdin);
    }
    fl = 1;  
    gets(command);
    if (strncmp(command, "copy", 4) == 0){
      char* str2 = (char*)malloc(200);
      char copy_path[50];
      printf("Type file path to copy: ");
      scanf(" %s", copy_path);
      strcpy(str2, current_dir);
      strcat(str2, "/");
      strcat(str2, extractFileName(copy_path));
      if (command[5] == '-'){
        if (command[6] == 's'){
          checkFileType(copy_path, str2);       
        }
        else if(command[6] == 'r'){
          printf("Enter file name: ");
          scanf("%s", file_name);
          strcat(copy_path, "/");
          strcat(copy_path, file_name);
          strcpy(str2, current_dir);
          strcat(str2, "/");
          strcat(str2, file_name);              
          checkFileType(str2, copy_path);       
        }
      }
      memset(str2, 0, strlen(str2));
      fl = 0;
    }
    else if (strcmp(command, "ls") == 0){
      showFilesInDirectory(current_dir);
    }
    else if (strncmp(command, "cd", 2) == 0){
      char offset[100];
      strcpy(offset, getOffsetInChangeDir(command));
      strcpy(current_dir, changeDirectory(current_dir, offset, path));
    }
    else if (strcmp(command, "clear") == 0){
      system("clear");
    }
    else if (strcmp(command, "unmount") == 0){
      unmount_folder(path);
      return 0;
    }
  }
}
