#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SIZE 2048
char *destination_path;

void receive_file(int sockfd, struct sockaddr_in server_addr, socklen_t addr_len) {
    char buffer[SIZE];
    int n;

    // Odbierz nazwę pliku
    size_t filename_size;
    n = recvfrom(sockfd, &filename_size, sizeof(size_t), 0, (struct sockaddr*)&server_addr, &addr_len);
    if (n <= 0) {
        perror("Error-can't get file name size.");
        exit(1);
    }

    char filename[filename_size + 1];
    n = recvfrom(sockfd, filename, filename_size, 0, (struct sockaddr *)&server_addr, &addr_len);
    filename[n] = '\0';  // Add the end-of-string character
    printf("Downloading file: %s from server...\n", filename);

    char path[100];
    strcpy(path,destination_path);
    strcat(path,filename);

    printf("Saving file in: %s\n", path);
	
    FILE *fp;
    fp = fopen(path, "wb");
    if (fp == NULL) {
        perror("Error-can't open file");
        exit(1);
    }

    // Odbierz rozmiar pliku
    size_t file_size;
    n = recvfrom(sockfd, &file_size, sizeof(size_t), 0, (struct sockaddr *)&server_addr, &addr_len);
    printf("File size: %zu bytes.\n", file_size);
    if (n <= 0) {
        perror("Error-can't get file size.");
        exit(1);
    }

    // zapisywanie pliku porcjami
    while (file_size > 0) {
        n = recvfrom(sockfd, buffer, SIZE, 0, (struct sockaddr *)&server_addr, &addr_len);
        if (n <= 0) {
            break;
        }
        fwrite(buffer, 1, n, fp);
        file_size -= n;
    }

    fclose(fp);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "To use: %s <ip> <destination>\n", argv[0]);
        exit(1);
    }

    char *ip = argv[1];
    destination_path = argv[2];
    int port = 8888;
    int sockfd;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket Error");
        exit(1);
    }
    printf("Client socket is ready!\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    // Wysyłanie informacji o kliencie do serwera (potrzebne przy użyciu UDP)
    if (sendto(sockfd, NULL, 0, 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error- can't send client info.");
        exit(1);
    }

    receive_file(sockfd, server_addr, sizeof(server_addr));

    printf("File has been downloaded successfully.\n");
    printf("Closing program.\n");
    close(sockfd);

    return 0;
}
