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
#include <sys/socket.h> //sockets duh
#include <arpa/inet.h> // contains lots of stuff- has def for IPPROTO_TCP and such
#include <netdb.h>
#include <ifaddrs.h>
#include <netinet/ip.h> // needed for ip_icmp?
#include <netinet/ip_icmp.h> //https://www.cymru.com/Documents/ip_icmp.h

unsigned short in_cksum(unsigned short* addr, int len)
{
    int nleft = len;
    int sum = 0;
    unsigned short* w = addr;
    unsigned short answer = 0;

    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1) {
        *(unsigned char*)(&answer) = *(unsigned char*)w;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    answer = ~sum;
    return (answer);
}

void swap(char* x, char* y) {
    char t = *x; *x = *y; *y = t;
}

// Function to reverse `buffer[i…j]`
char* reverse(char* buffer, int i, int j)
{
    while (i < j) {
        swap(&buffer[i++], &buffer[j--]);
    }

    return buffer;
}

char* itoa(int value, char* buffer, int base)
{
    // invalid input
    if (base < 2 || base > 32) {
        return buffer;
    }

    // consider the absolute value of the number
    int n = abs(value);

    int i = 0;
    while (n)
    {
        int r = n % base;

        if (r >= 10) {
            buffer[i++] = 65 + (r - 10);
        }
        else {
            buffer[i++] = 48 + r;
        }

        n = n / base;
    }

    // if the number is 0
    if (i == 0) {
        buffer[i++] = '0';
    }
    // If the base is 10 and the value is negative, the resulting string
// is preceded with a minus sign (-)
// With any other base, value is always considered unsigned
    if (value < 0 && base == 10) {
        buffer[i++] = '-';
    }

    buffer[i] = '\0'; // null terminate string

    // reverse the string and return it
    return reverse(buffer, 0, i - 1);
}

char* ping(char* pc_ip) {
    int numOfConnections = 0;
    char** available_ips = (char**)malloc(10);
    for (int i = 0; i < 1; i++) {
        available_ips[i] = (char*)malloc(20);
    }
    char* local_ip;
    struct ip ip;
    struct icmp icmp;
    int sd;
    const int on = 1;
    struct sockaddr_in sin;
    u_char* packet;
    int count = 1;
    packet = (u_char*)malloc(sizeof(struct ip) + sizeof(struct icmp));

    ip.ip_hl = 0x5;
    ip.ip_v = 0x4;
    ip.ip_tos = 0x0;
    ip.ip_len = sizeof(struct ip) + sizeof(struct icmp);
    ip.ip_id = htons(12830);
    ip.ip_off = 0x0;
    ip.ip_ttl = 64;
    ip.ip_p = IPPROTO_ICMP;
    ip.ip_sum = 0x0;
    ip.ip_src.s_addr = inet_addr(pc_ip);
    //ip.ip_dst.s_addr = inet_addr("172.17.0.1");
    ip.ip_sum = in_cksum((unsigned short*)&ip, 20);
    //memcpy(packet, &ip, sizeof(ip));

    icmp.icmp_type = ICMP_ECHO;
    icmp.icmp_code = 2;
    icmp.icmp_id = 1000;
    icmp.icmp_seq = 0;
    icmp.icmp_cksum = 0;
    icmp.icmp_cksum = in_cksum((unsigned short*)&icmp, sizeof(struct icmp));
    //memcpy(packet + sizeof(struct ip), &icmp, sizeof(struct icmp));

    while (count < 256) {

        char buffer[4];
        itoa(count, buffer, 10);
        char start_ip[] = "192.168.0.";
        buffer[strlen(buffer)] = '\0';
        strcat(start_ip, buffer);
        //printf("ip - %s\n", start_ip);
        ip.ip_dst.s_addr = inet_addr(start_ip);
        memcpy(packet, &ip, sizeof(ip));
        memcpy(packet + sizeof(struct ip), &icmp, sizeof(struct icmp));


        /*	http://linux.die.net/man/7/socket
        #
        #	sockfd = socket(int socket_family, int socket_type, int protocol);
        #
        */
        if ((sd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
            perror("raw socket");
            exit(1);
        }


        /*	http://pubs.opengroup.org/onlinepubs/009695399/functions/setsockopt.html
        #
        #	int setsockopt(int socket, int level, int option_name,
        #   		const void *option_value, socklen_t option_len);
        #
        */
        if (setsockopt(sd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
            perror("setsockopt");
            exit(1);
        }


        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = ip.ip_dst.s_addr;

        if (sendto(sd, packet, sizeof(struct icmp) + sizeof(struct ip), 0, (struct sockaddr*)&sin, sizeof(struct sockaddr)) < 0) {
            perror("sendto");
            exit(1);
        }

        unsigned char buf[sizeof(struct ip) + sizeof(struct icmphdr)];
        int bytes, len = sizeof(sin);

        bzero(buf, sizeof(buf));
        bytes = recvfrom(sd, buf, sizeof(buf), 0, (struct sockaddr*)&sin, &len);
        if (bytes > 0) {
            //printf("Bytes received: %d\n", bytes); //display(buf, bytes);
        }
        else {
            perror("recvfrom");
        }

        struct ip* iprecv = (struct ip*)(buf);
        char buf1[20];
        puts("\nIP HEADER");
        printf("\tIP version: %d\n", iprecv->ip_v);
        printf("\tProtocol: %d\n", iprecv->ip_p);
        printf("\tIdentification: 0x%X\n", ntohs(iprecv->ip_id));
        printf("\tHeader len: %i\n", iprecv->ip_hl * 4);
        printf("\tChecksum: 0x%X\n", ntohs(iprecv->ip_sum));
        printf("\tTTL: %d\n", iprecv->ip_ttl);
        printf("\tSource IP: %s\n", inet_ntoa(iprecv->ip_src));
        printf("\tDestination IP: %s\n", inet_ntoa(iprecv->ip_dst));

        struct icmphdr* icmprecv = (struct icmphdr*)(buf + iprecv->ip_hl * 4);
        puts("\nICMP HEADER");
        printf("\tType: %i\n", icmprecv->type);
        printf("\tCode: %d\n", icmprecv->code);
        printf("\tChecksum: 0x%X\n", ntohs(icmprecv->checksum));
        printf("\tIdentifier: %i\n", icmprecv->un.echo.id);
        strcpy(buf1, inet_ntoa(iprecv->ip_src));
        char buf2[20];
        strcpy(buf2, inet_ntoa(iprecv->ip_dst));
        if (strcmp(buf1, buf2) != 0) {
            local_ip = (char*)malloc(sizeof(inet_ntoa(iprecv->ip_src)));
            strcpy(local_ip, inet_ntoa(iprecv->ip_src));
            strcpy(available_ips[numOfConnections], local_ip);
            numOfConnections++;
            // printf("\tSource IP: %s\n", inet_ntoa(iprecv->ip_src));
            // printf("\tDestination IP: %s\n", inet_ntoa(iprecv->ip_dst));
            while (1) {
                close(sd);
                printf("Available connections: \n");
                for (int i = 0; i < numOfConnections; i++) {
                    printf("%d - %s", i + 1, available_ips[i]);
                }
                int choice;
                scanf("%d", &choice);
                if (choice > 0 && choice <= numOfConnections) {
                    return available_ips[numOfConnections - 1];
                }
            }
        }
        close(sd);
        count++;
    }
}


char* dev = "enx";
static char gateway[20];
static char* skip_ip = { "0.0.0.0" };
static int num_of_ip_adresses = 0;


// char* hex_to_dec(char buf[]){
//   int point = 0;
//   memset(gateway, 0, sizeof(gateway));
//   int next_flag = 0;
//   char dec_buf[3];
//   for(int i = 0; i < strlen(buf)/2; i++){
//       int j = strlen(buf)-1-i;
//       char bf = buf[i];                           
//       buf[i] = buf[j];       
//       buf[j] = bf;                                
//   }
//   for (int i = 0; i < strlen(buf)-1; i = i + 2){
//     int j = i + 1;
//     char bf = buf[i];
//     buf[i] = buf[j];
//     buf[j] = bf;
//   }
//   int sum = 0;
//   for (int i = 0; i < strlen(buf); i++){
//     //int j = i + 1;
//     if (next_flag){
//       sum = 0;
//     }
//     next_flag = 0;
//     int hex_in_dec = 0;
//     hex_in_dec = ((int)buf[i]>=65 && (int)buf[i]<=70) ? (int)buf[i] + 10 - 'A' : (int)buf[i] - '0';
//     if ((i+1)%2==0){
//       sum = sum*16+hex_in_dec;
//       next_flag = 1;
//       //
//       if (sum <10){
//         char symbol = sum + '0'; 
//         dec_buf[0] = symbol;
//       }
//       else{
//         int count = 0;
//         do {
//           int digit = sum % 10;
//           dec_buf[count++] = (digit > 9) ? digit - 10 +'A' : digit + '0';
//         } while ((sum /= 10) != 0);
//         dec_buf[count] = '\0';
//         int i;
//         for (i = 0; i < count / 2; ++i) {
//           char symbol = dec_buf[i];
//           dec_buf[i] = dec_buf[count - i - 1];
//           dec_buf[count - i - 1] = symbol;
//         }
//       }
//       point++;
//       strcat(gateway, dec_buf);
//       if (point !=4){
//         strcat(gateway, ".");
//       }
//       memset(dec_buf, 0, sizeof(dec_buf));

//       //
//     }
//     else{
//       sum = hex_in_dec;
//     }
//   }
//   return gateway;
// }

// char* extract_gateway(){
//   FILE* f;
//   int ip_choice;
//   pid_t pid;
//   int file_size = 0;
//   int word_size = 0;
//   int x = 1;
//   int flag = 0;
//   int skip = 0;
//   char* str = (char*)malloc(15);
//   char** gateway_ip = NULL;
//   int status;
//   int tab_flag = 0;
//   char interface_buf[3];
//   char gateway_buf[8];
//   char** available_ips = (char**)malloc(1);
//   for (int i = 0; i < 1; i++){
//     available_ips[i] = (char*)malloc(20);
//   }
//   if (!(f = fopen("/proc/net/route", "r"))){
//     printf("we");
//   }
//   while(!feof(f)){
//     int space_counter = 0;
//     if (!flag){
//       while(1){
//         char ch = fgetc(f);
//         if (feof(f)){
//           break;
//         }
//         if(ch == ' '){
//           space_counter++;
//         }
//         else if (ch == '\n'){
//           continue;
//         }
//         else{
//           if ((ch == 'e') && space_counter>10){
//             ch = fgetc(f);
//             if (ch == 'n'){
//               ch = fgetc(f);
//               if (ch == 'x'){
//                 fseek(f, -3, SEEK_CUR);
//                 flag = 1;
//                 break;
//               }
//             }
//           }
//           else{
//             space_counter = 0;
//           }
//         }
//       }
//     }
//     memset(interface_buf, 0, sizeof(interface_buf)+1);
//     fread(&interface_buf, sizeof(char), 3, f);
//     //printf("buf - %s", interface_buf);
//     if (strcmp(interface_buf, dev) != 0){
//       break;
//     }
//     else{
//       flag = 0;
//       while(tab_flag != 2 && !feof(f)){
//         char ch2 = fgetc(f);
//         if (ch2 != '\t'){
//           fseek(f, 1, SEEK_CUR);
//         }else{
//           tab_flag++;
//         }
//       }
//       tab_flag = 0;
//       fread(&gateway_buf, sizeof(char), 8, f);
//       //printf("buf - %s\n", gateway_buf);
//       char* gateway_ip = hex_to_dec(gateway_buf);
//       if (strcmp(gateway_ip, skip_ip) == 0){
//         continue;
//       }
//       else{
//         num_of_ip_adresses++;
//         available_ips = (char**)realloc(available_ips, num_of_ip_adresses+1);
//         available_ips[num_of_ip_adresses] = (char*)malloc(20);
//         strcpy(available_ips[num_of_ip_adresses-1], gateway_ip);
//       }
//     }
//   }
//   fclose(f);
//   if (num_of_ip_adresses != 0){
//     while(1){
//       printf("Choose one of available adresses:\n");
//       for (int i = 0; i < num_of_ip_adresses; i++){
//         printf("%d - %s\n", i+1, available_ips[i]);
//       }
//       scanf("%d", &ip_choice);
//       if (ip_choice > 0 && ip_choice <= num_of_ip_adresses){
//         break;
//       }
//       printf("There is no such choice. Try again\n");
//     }
//   }
//   else{
//     printf("There are no available devices\n");
//     exit(1);
//   }

//   return available_ips[ip_choice-1];
// }

char* mount_folder(char* ip) {
    int status;
    int result;
    char name[30];
    char full_pass[100];
    char* folder_name = (char*)malloc(50);
    char* mount_point = (char*)malloc(50);
    char* user_name = (char*)malloc(50);
    char* user_password = (char*)malloc(50);
    printf("Enter user name: ");
    scanf("%s", user_name);
    printf("Enter user password: ");
    scanf("%s", user_password);

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
      "gleb",
      ip,
      folder_name,
      user_name,
      user_password,
      NULL
    };
    pid = fork();
    if (pid == 0) {
        execv("/bin/sh", args);
    }
    wait(&status);
    return mount_point;
}

void unmount_folder(char* path) {
    int status;
    int result;
    pid_t pid;
    char* args[] = {
      "sh",
      "./folderumounter.sh",
      path,
      NULL
    };
    pid = fork();
    if (pid == 0) {
        execv("/bin/sh", args);
    }
    wait(&status);
}

int checkIfMounted() {
    int flag = 1;
    char error1[] = { "mount error(2): No such file or directory\n" };
    char error2[] = { "mount error(13): Permission denied\n" };
    char str[100];
    FILE* f;
    if (!(f = fopen("./errors", "r"))) {
        return 1;
    }
    fgets(str, 98, f);
    fclose(f);
    if (strcmp(str, error1) == 0) {
        printf("%s\n", "Wrong folder name. Try again");
        flag = 0;
    }
    else if (strcmp(str, error2) == 0) {
        printf("%s\n", "Wrong credentials. Try again");
        flag = 0;
    }
    else if (strcmp(str, "q") == 0) {
        exit(1);
    }
    return flag;
}

void chooseComand(char* current_dir, char* path) {
    char file_name[50];
    int fl = 0;
    while (1) {
        char* command = (char*)malloc(200);
        if (fl) {
            printf("%s>", current_dir);
            fflush(stdin);
        }
        fl = 1;
        gets(command);
        if (strncmp(command, "copy", 4) == 0) {
            char* str2 = (char*)malloc(200);
            char copy_path[50];
            printf("Type file path to copy: ");
            scanf(" %s", copy_path);
            strcpy(str2, current_dir);
            strcat(str2, "/");
            strcat(str2, extractFileName(copy_path));
            if (command[5] == '-') {
                if (command[6] == 's') {
                    checkFileType(copy_path, str2);
                }
                else if (command[6] == 'r') {
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
        else if (strcmp(command, "ls") == 0) {
            showFilesInDirectory(current_dir);
        }
        else if (strncmp(command, "cd", 2) == 0) {
            char offset[100];
            strcpy(offset, getOffsetInChangeDir(command));
            strcpy(current_dir, changeDirectory(current_dir, offset, path));
        }
        else if (strcmp(command, "clear") == 0) {
            system("clear");
        }
        else if (strcmp(command, "unmount") == 0) {
            unmount_folder(path);
            return 0;
        }
    }
}

static char ipl[20];

void localIp() {
    char ap[100];
    char value[20];
    struct ifaddrs* addresses;
    if (getifaddrs(&addresses) == -1) {
        printf("getifaddrs call failed\n");
        return -1;
    }

    struct ifaddrs* address = addresses;
    while (address) {
        int family = address->ifa_addr->sa_family;
        if (family == AF_INET) {
            const int family_size = family = sizeof(struct sockaddr_in);
            getnameinfo(address->ifa_addr, family_size, ap, sizeof(ap), 0, 0, NI_NUMERICHOST);
            if (strncmp(ap, "127", 3) != 0) {
                strcpy(ipl, ap);
                break;
            }
            //
        }
        address = address->ifa_next;
    }
    freeifaddrs(addresses);
    //return value;
}


int main() {
    //char* ipl = (char*)malloc(20);
    //printf("fun - %s", localIp());
    //strcpy(ipl, localIp());
    //ping(ip);
    localIp();
    //printf("ipl %s", ipl);
    char* current_dir = (char*)malloc(200);
    char mount_ip[20];
    char path[50];
    strcpy(mount_ip, ping(ipl));
    // strcpy(path, mount_folder(mount_ip));
    // strcpy(current_dir, path);
    while (1) {
        strcpy(path, mount_folder(mount_ip));
        strcpy(current_dir, path);
        if (checkIfMounted()) {
            break;
        }
        //printf("%s\n", "Wrong folder name. Try again");
    }
    chooseComand(current_dir, path);
}