#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

int main(int argc, char* argv[]) {
    int interval = 250;
    const char* ip = "127.0.0.1";
    int poort = 7000;
    if (argc > 1) interval = atoi(argv[1]);
    if (argc > 2) ip = argv[2];
    if (argc > 3) poort = atoi(argv[3]);

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    while (true) {
        SOCKET a = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in adres = {};
        adres.sin_family = AF_INET;
        adres.sin_port = htons(poort);
        inet_pton(AF_INET, ip, &adres.sin_addr);
        if (connect(a, (sockaddr*)&adres, sizeof(adres)) != 0) {
            closesocket(a);
            Sleep(100);
            continue;
        }

        while (send(a, "hb", 2, 0) > 0) {
            Sleep(interval);
        }
        closesocket(a);
    }
}
