#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024

int main()
{
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in server;
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE];

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("WSAStartup basarisiz\n");
        return 1;
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        printf("Socket olusturulamadi\n");
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR)
    {
        printf("Baglanti basarisiz\n");
        return 1;
    }

    printf("Hava Durumu Sistemine Hosgeldiniz!\n");
    printf("Komutlar:\n");
    printf("LIST - Tum sehirleri listele\n");
    printf("[Sehir Adi] - Belirtilen sehrin hava durumunu goster\n");
    printf("EXIT - Programdan cik\n\n");

    while (1)
    {
        printf("\nKomut girin: ");
        fgets(message, BUFFER_SIZE, stdin);
        message[strcspn(message, "\n")] = 0; // Remove newline

        if (strcmp(message, "EXIT") == 0)
        {
            break;
        }

        // Sunucuya mesaj gönder
        send(sock, message, strlen(message), 0);

        // Yanıtı al
        memset(buffer, 0, BUFFER_SIZE);
        recv(sock, buffer, BUFFER_SIZE, 0);
        printf("\n%s", buffer);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
