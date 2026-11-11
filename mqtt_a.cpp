#include <mqtt/client.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char* argv[]) {
    int qos = 1;
    std::string broker = "127.0.0.1";
    std::string poort = "1883";
    if (argc > 1) qos = atoi(argv[1]);
    if (argc > 2) broker = argv[2];
    if (argc > 3) poort = argv[3];

    std::ofstream log("a_log.csv");

    try {
        mqtt::client client("tcp://" + broker + ":" + poort, "zoneA");
        mqtt::connect_options opties;
        opties.set_clean_session(false);
        opties.set_automatic_reconnect(1, 2);
        client.connect(opties);

        client.subscribe("klaar", qos);
        auto beginTest = std::chrono::steady_clock::now();

        for (int nummer = 1; nummer <= 1000; nummer++) {
            std::string opdracht = std::to_string(nummer);
            auto start = std::chrono::steady_clock::now();

            bool geplaatst = false;
            while (!geplaatst) {
                try {
                    client.publish(mqtt::make_message("opdrachten", opdracht, qos, false));
                    geplaatst = true;
                } catch (const mqtt::exception&) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }

            bool gelukt = false;
            mqtt::const_message_ptr antwoord;
            while (client.try_consume_message_for(&antwoord, std::chrono::seconds(15))) {
                if (antwoord && antwoord->to_string() == opdracht) {
                    gelukt = true;
                    break;
                }
            }

            auto eind = std::chrono::steady_clock::now();
            double ms = std::chrono::duration<double, std::milli>(eind - start).count();
            double tijdstip = std::chrono::duration<double, std::milli>(eind - beginTest).count();

            log << nummer << "," << (gelukt ? "gelukt" : "mislukt") << "," << ms << "," << tijdstip << std::endl;

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        client.disconnect();
    } catch (const mqtt::exception& fout) {
        std::cout << "MQTT-fout: " << fout.what() << std::endl;
    }

    std::cout << "Klaar! Alle opdrachten zijn verstuurd." << std::endl;
}