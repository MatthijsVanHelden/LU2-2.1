#include <winsock2.h>
#include <ws2tcpip.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

int variant = 1;

SOCKET verbind(const char* ip, int poort) {
    SOCKET b = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in adres = {};
    adres.sin_family = AF_INET;
    adres.sin_port = htons(poort);
    inet_pton(AF_INET, ip, &adres.sin_addr);

    if (connect(b, (sockaddr*)&adres, sizeof(adres)) != 0) {
        closesocket(b);
        return INVALID_SOCKET;
    }

    if (variant == 2) {
        DWORD timeout = 1000;
        setsockopt(b, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    }
    return b;
}

int main(int argc, char* argv[]) {
    const char* ip = "127.0.0.1";
    int poort = 5000;
    if (argc > 1) variant = atoi(argv[1]);
    if (argc > 2) ip = argv[2];
    if (argc > 3) poort = atoi(argv[3]);

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    std::ofstream log("a_log.csv");
    SOCKET b = INVALID_SOCKET;
    auto beginTest = std::chrono::steady_clock::now();

    for (int nummer = 1; nummer <= 1000; nummer++) {
        auto start = std::chrono::steady_clock::now();
        bool gelukt = false;

        do {
            if (b == INVALID_SOCKET) {
                b = verbind(ip, poort);
            }

            if (b != INVALID_SOCKET) {
                std::string opdracht = std::to_string(nummer);
                send(b, opdracht.c_str(), (int)opdracht.size(), 0);

                char antwoord[100] = {};
                if (recv(b, antwoord, sizeof(antwoord) - 1, 0) > 0) {
                    gelukt = true;
                }
            }

            if (!gelukt && b != INVALID_SOCKET) {
                closesocket(b);
                b = INVALID_SOCKET;
            }
            if (!gelukt && variant == 2) {
                Sleep(100);
            }
        } while (!gelukt && variant == 2);

        auto eind = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(eind - start).count();
        double tijdstip = std::chrono::duration<double, std::milli>(eind - beginTest).count();

        log << nummer << "," << (gelukt ? "gelukt" : "mislukt") << "," << ms << "," << tijdstip << std::endl;

        Sleep(10);
    }

    std::cout << "Klaar! Alle opdrachten zijn verstuurd." << std::endl;
}