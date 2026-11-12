#include <winsock2.h>
#include <ws2tcpip.h>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

std::string modus;
int procent = 0;
double begin = 0, duur = 0, elke = 0;
int doelpoort = 0;
std::string doelIp = "127.0.0.1";
auto starttijd = std::chrono::steady_clock::now();
std::atomic<long long> bytes(0);
std::atomic<long long> stukjes(0);
std::vector<SOCKET> openSockets;
std::mutex socketSlot;

bool isOnderbreking() {
    if (modus != "onderbreking" && modus != "verbreking") {
        return false;
    }
    double t = std::chrono::duration<double>(std::chrono::steady_clock::now() - starttijd).count();
    if (t < begin) {
        return false;
    }
    if (elke == 0) {
        return t < begin + duur;
    }
    double inRonde = t - begin;
    while (inRonde >= elke) {
        inRonde -= elke;
    }
    return inRonde < duur;
}

void doorsturen(SOCKET van, SOCKET naar) {
    srand((unsigned)time(0) + GetCurrentThreadId());
    char buffer[4096];
    while (true) {
        int n = recv(van, buffer, sizeof(buffer), 0);
        if (n <= 0) {
            break;
        }

        if (modus == "verlies" && rand() % 100 < procent) {
            Sleep(300);
        }
        while (modus == "onderbreking" && isOnderbreking()) {
            Sleep(10);
        }
        if (modus == "verbreking" && isOnderbreking()) {
            break;
        }

        if (send(naar, buffer, n, 0) <= 0) {
            break;
        }
        bytes += n;
        stukjes++;
    }
    shutdown(van, SD_BOTH);
    shutdown(naar, SD_BOTH);
}

void verbreker() {
    while (true) {
        if (isOnderbreking()) {
            std::lock_guard<std::mutex> slot(socketSlot);
            for (SOCKET s : openSockets) {
                shutdown(s, SD_BOTH);
            }
            openSockets.clear();
        }
        Sleep(10);
    }
}

void nieuweVerbinding(SOCKET a) {
    SOCKET b = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in adres = {};
    adres.sin_family = AF_INET;
    adres.sin_port = htons(doelpoort);
    inet_pton(AF_INET, doelIp.c_str(), &adres.sin_addr);

    if (connect(b, (sockaddr*)&adres, sizeof(adres)) != 0) {
        shutdown(a, SD_BOTH);
        return;
    }
    {
        std::lock_guard<std::mutex> slot(socketSlot);
        openSockets.push_back(a);
        openSockets.push_back(b);
    }
    std::thread(doorsturen, a, b).detach();  // A -> B
    std::thread(doorsturen, b, a).detach();  // B -> A
}

void tellerSchrijven() {
    long long startMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch()).count();
    while (true) {
        std::ofstream teller("teller.txt");
        teller << bytes << "," << stukjes << "," << startMs << std::endl;
        teller.close();
        Sleep(100);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cout << "Gebruik: storing.exe <luisterpoort> <doelpoort> geen|verlies|onderbreking ..." << std::endl;
        return 1;
    }
    int luisterpoort = atoi(argv[1]);
    std::string doel = argv[2];
    size_t dubbelepunt = doel.find(':');
    if (dubbelepunt != std::string::npos) {
        doelIp = doel.substr(0, dubbelepunt);
        doel = doel.substr(dubbelepunt + 1);
    }
    doelpoort = atoi(doel.c_str());
    modus = argv[3];
    if (modus == "verlies") {
        procent = atoi(argv[4]);
    }
    if (modus == "onderbreking" || modus == "verbreking") {
        begin = atof(argv[4]);
        duur = atof(argv[5]);
        elke = atof(argv[6]);
    }

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in adres = {};
    adres.sin_family = AF_INET;
    adres.sin_port = htons(luisterpoort);
    adres.sin_addr.s_addr = INADDR_ANY;
    bind(server, (sockaddr*)&adres, sizeof(adres));
    listen(server, 10);
    std::cout << "Storing: poort " << luisterpoort << " -> " << doelIp << ":" << doelpoort << ", modus " << modus << std::endl;

    std::thread(tellerSchrijven).detach();
    if (modus == "verbreking") {
        std::thread(verbreker).detach();
    }

    while (true) {
        SOCKET a = accept(server, NULL, NULL);
        if (modus == "verbreking" && isOnderbreking()) {
            closesocket(a);
            continue;
        }
        nieuweVerbinding(a);
    }
}
