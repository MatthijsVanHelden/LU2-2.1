#include <winsock2.h>
#include <chrono>
#include <fstream>
#include <iostream>

long long nu() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

int main(int argc, char* argv[]) {
    int timeout = 750;
    int poort = 7000;
    if (argc > 1) timeout = atoi(argv[1]);
    if (argc > 2) poort = atoi(argv[2]);

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    std::ofstream log("hb_log.csv");

    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in adres = {};
    adres.sin_family = AF_INET;
    adres.sin_port = htons(poort);
    adres.sin_addr.s_addr = INADDR_ANY;
    bind(server, (sockaddr*)&adres, sizeof(adres));
    listen(server, 1);

    while (true) {
        SOCKET b = accept(server, NULL, NULL);
        bool alarm = false;

        while (true) {
            fd_set sockets;
            FD_ZERO(&sockets);
            FD_SET(b, &sockets);
            timeval wachttijd = {timeout / 1000, (timeout % 1000) * 1000};
            int klaar = select(0, &sockets, NULL, NULL, &wachttijd);

            if (klaar == 0) {
                if (!alarm) {
                    log << "alarm," << nu() << std::endl;
                    alarm = true;
                }
                continue;
            }

            char hartslag[100];
            if (recv(b, hartslag, sizeof(hartslag), 0) <= 0) {
                break;
            }
            if (alarm) {
                log << "terug," << nu() << std::endl;
                alarm = false;
            }
        }
        closesocket(b);
    }
}
