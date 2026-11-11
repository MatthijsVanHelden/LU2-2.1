#include <mqtt/client.h>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <thread>

int main(int argc, char* argv[]) {
    int qos = 1;
    std::string broker = "127.0.0.1";
    std::string poort = "1883";
    if (argc > 1) qos = atoi(argv[1]);
    if (argc > 2) broker = argv[2];
    if (argc > 3) poort = argv[3];

    std::set<std::string> uitgevoerd;
    std::ifstream oudeLog("b_log.csv");
    std::string regel;
    while (std::getline(oudeLog, regel)) {
        uitgevoerd.insert(regel);
    }
    oudeLog.close();
    std::ofstream log("b_log.csv", std::ios::app);

    try {
        mqtt::client client("tcp://" + broker + ":" + poort, "zoneB");
        mqtt::connect_options opties;
        opties.set_clean_session(false);
        opties.set_automatic_reconnect(1, 2);
        client.connect(opties);

        client.subscribe("opdrachten", qos);
        std::cout << "B (QoS " << qos << ") wacht op opdrachten..." << std::endl;

        while (true) {
            auto bericht = client.consume_message();
            if (!bericht) {
                continue;
            }
            std::string opdracht = bericht->to_string();

            bool alGedaan = uitgevoerd.count(opdracht) > 0;
            if (qos == 2 || !alGedaan) {
                log << opdracht << std::endl;
                uitgevoerd.insert(opdracht);
            }

            bool geplaatst = false;
            while (!geplaatst) {
                try {
                    client.publish(mqtt::make_message("klaar", opdracht, qos, false));
                    geplaatst = true;
                } catch (const mqtt::exception&) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
        }
    } catch (const mqtt::exception& fout) {
        std::cout << "MQTT-fout: " << fout.what() << std::endl;
    }
}