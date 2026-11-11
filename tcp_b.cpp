#include <winsock2.h>
#include <fstream>
#include <iostream>
#include <set>
#include <string>

int main(int argc, char* argv[]) {
    int variant = 1;
    int poort = 5000;
    if (argc > 1) variant = atoi(argv[1]);
    if (argc > 2) poort = atoi(argv[2]);

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    std::set<std::string> uitgevoerd;
    std::ifstream oudeLog("b_log.csv");
    std::string regel;
    while (std::getline(oudeLog, regel)) {
        uitgevoerd.insert(regel);
    }
    oudeLog.close();

    std::ofstream log("b_log.csv", std::ios::app);

    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in adres = {};
    adres.sin_family = AF_INET;
    adres.sin_port = htons(poort);
    adres.sin_addr.s_addr = INADDR_ANY;
    bind(server, (sockaddr*)&adres, sizeof(adres));
    listen(server, 1);
    std::cout << "B (V" << variant << ") wacht op A op poort " << poort << std::endl;

    while (true) {
        SOCKET a = accept(server, NULL, NULL);

        while (true) {
            char opdracht[100] = {};
            if (recv(a, opdracht, sizeof(opdracht) - 1, 0) <= 0) {
                break;
            }

            bool alGedaan = uitgevoerd.count(opdracht) > 0;
            if (variant == 1 || !alGedaan) {
                log << opdracht << std::endl;
                uitgevoerd.insert(opdracht);
            }

            send(a, "klaar", 5, 0);
        }
        closesocket(a);
    }
}