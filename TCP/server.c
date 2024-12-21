#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define MAX_CITIES 5

// Hava durumu verisi için yapı
typedef struct
{
    char city[50];
    float temperature;
    int humidity;
    char condition[20];
} WeatherData;

// Global weather data
WeatherData weatherDB[MAX_CITIES] = {
    {"Istanbul", 18.5, 65, "Parcali Bulutlu"},
    {"Ankara", 15.2, 45, "Gunesli"},
    {"Izmir", 22.1, 70, "Acik"},
    {"Antalya", 25.4, 60, "Gunesli"},
    {"Bursa", 17.8, 75, "Yagmurlu"}};

CRITICAL_SECTION clientLock;

// Hava durumu verilerini güncelle (simülasyon)
void updateWeather()
{
    time_t t;
    srand((unsigned)time(&t));

    for (int i = 0; i < MAX_CITIES; i++)
    {
        // Sıcaklığı ±2 derece değiştir
        weatherDB[i].temperature += ((rand() % 40) - 20) / 10.0;
        // Nemi ±5 değiştir
        weatherDB[i].humidity += (rand() % 10) - 5;
        if (weatherDB[i].humidity > 100)
            weatherDB[i].humidity = 100;
        if (weatherDB[i].humidity < 0)
            weatherDB[i].humidity = 0;
    }
}

// İstemci işleme thread fonksiyonu
unsigned __stdcall handleClient(void *data)
{
    SOCKET clientSocket = *(SOCKET *)data;
    free(data);
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);
        memset(response, 0, BUFFER_SIZE);

        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0)
        {
            printf("Istemci baglantisi kesildi veya hata olustu.\n");
            break;
        }

        buffer[bytesReceived] = '\0';
        printf("Istemciden gelen: %s\n", buffer);

        if (strcmp(buffer, "LIST") == 0)
        {
            strcpy(response, "Mevcut Sehirler:\n");
            for (int i = 0; i < MAX_CITIES; i++)
            {
                strcat(response, weatherDB[i].city);
                strcat(response, "\n");
            }
        }
        else
        {
            int found = 0;
            for (int i = 0; i < MAX_CITIES; i++)
            {
                if (_stricmp(buffer, weatherDB[i].city) == 0)
                {
                    snprintf(response, BUFFER_SIZE,
                             "%s Hava Durumu:\nSicaklik: %.1f C\nNem: %d%%\nDurum: %s\n",
                             weatherDB[i].city,
                             weatherDB[i].temperature,
                             weatherDB[i].humidity,
                             weatherDB[i].condition);
                    found = 1;
                    break;
                }
            }
            if (!found)
            {
                strcpy(response, "Sehir bulunamadi!\n");
            }
        }

        printf("Gonderilen yanit: %s\n", response);
        int bytesSent = send(clientSocket, response, strlen(response), 0);
        if (bytesSent <= 0)
        {
            printf("Yanit gonderilemedi!\n");
            break;
        }
    }

    closesocket(clientSocket);
    return 0;
}

int main()
{
    WSADATA wsa;
    SOCKET server_fd;
    struct sockaddr_in address;

    InitializeCriticalSection(&clientLock);

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("WSAStartup basarisiz\n");
        return 1;
    }

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        printf("Socket olusturulamadi\n");
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR)
    {
        printf("Bind basarisiz\n");
        return 1;
    }

    if (listen(server_fd, MAX_CLIENTS) == SOCKET_ERROR)
    {
        printf("Listen basarisiz\n");
        return 1;
    }

    printf("Hava Durumu Sunucusu baslatildi. Port: %d\n", PORT);

    while (1)
    {
        SOCKET *new_socket = (SOCKET *)malloc(sizeof(SOCKET));
        struct sockaddr_in client_addr;
        int addrlen = sizeof(client_addr);

        *new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
        if (*new_socket == INVALID_SOCKET)
        {
            free(new_socket);
            continue;
        }

        printf("Yeni baglanti: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Her 5 bağlantıda bir hava durumunu güncelle
        static int connectionCount = 0;
        if (++connectionCount % 5 == 0)
        {
            updateWeather();
        }

        HANDLE threadHandle = (HANDLE)_beginthreadex(NULL, 0, handleClient, (void *)new_socket, 0, NULL);
        if (threadHandle)
        {
            CloseHandle(threadHandle);
        }
    }

    DeleteCriticalSection(&clientLock);
    closesocket(server_fd);
    WSACleanup();
    return 0;
}
