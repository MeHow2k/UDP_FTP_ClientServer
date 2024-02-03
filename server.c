#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <libgen.h>

#define SIZE 2048

char *file_path;  // Zmienna globalna do przechowywania ścieżki do pliku

void send_file(FILE *fp, int sockfd, struct sockaddr_in client_addr, socklen_t addr_len) {
    size_t n;
    char data[SIZE];
    char *filename=basename(file_path);
    // Sending filename size
    size_t filename_size = strlen(filename);
    if (sendto(sockfd, &filename_size, sizeof(size_t), 0, (struct sockaddr *)&client_addr, addr_len) == -1) {
        perror("Error-can't send file name size.");
    }

    // Sending filename
    if (sendto(sockfd, filename, filename_size, 0, (struct sockaddr *)&client_addr, addr_len) == -1) {
        perror("Error-can't send file name.");
    }

	
    printf("File about to send to client: %s\n", file_path);

    // Odczytaj rozmiar pliku
    fseek(fp, 0, SEEK_END);
    size_t file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Wysyłanie rozmiaru pliku
    if (sendto(sockfd, &file_size, sizeof(size_t), 0, (struct sockaddr *)&client_addr, addr_len) == -1) {
        perror("[!]Error sending file size.");
    }
    printf("Size of file: %zu bytes\n", file_size);

    // Wysyłanie pliku porcjami
    while ((n = fread(data, 1, SIZE, fp)) > 0) {
        printf("%zu bytes sent!\n", n);
        if (sendto(sockfd, data, n, 0, (struct sockaddr *)&client_addr, addr_len) == -1) {
            perror("Error- can't send data.");
            break;
        }
    }
}

void *handle_connection(void *arg) {
    int sockfd = *((int *)arg);
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Otwórz plik do wysłania
    FILE *fp = fopen(file_path, "rb");
    if (fp == NULL) {
        perror("Error-can't open file.");
        fprintf(stderr, "Cannot open file: %s\n", file_path);
        close(sockfd);
    }

    // Odbierz informacje o kliencie (potrzebne przy użyciu UDP)
    if (recvfrom(sockfd, NULL, 0, 0, (struct sockaddr *)&client_addr, &addr_len) == -1) {
        perror("Error-can't get client info.");
    }

    send_file(fp, sockfd, client_addr, addr_len);
    printf("File was sent. Shutting down server.\n");

    fclose(fp);
    close(sockfd);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "To use: %s <ip> <filepath>\n", argv[0]);
        exit(1);
    }

    char *ip = argv[1];
    file_path = argv[2];  
    int port = 8888;
    int sockfd;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket error");
        exit(1);
    }
    printf("Server socket is ready!.\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error-can't bind");
        exit(1);
    }
    printf("Server binded.\n");
   
    handle_connection(&sockfd);
    
    close(sockfd);

    return 0;
}
