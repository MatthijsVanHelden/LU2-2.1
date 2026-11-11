import os
import shutil
import socket
import subprocess
import sys
import time

from analyse import tel

HERHALINGEN = 1 if "snel" in sys.argv else 5
MOSQUITTO_SUB = "C:/Program Files/mosquitto/mosquitto_sub.exe"

MAP = "bin/"
if not os.path.exists(MAP + "tcp_a.exe"):
    MAP = "bin/Debug/"

VARIANTEN = {
    "V1": ["tcp", "1"],
    "V2": ["tcp", "2"],
    "V3": ["mqtt", "1"],
    "V4": ["mqtt", "2"],
}

SCENARIOS = {
    "S0": ["geen"],
    "S1": ["verlies", "5"],
    "S2": ["verlies", "20"],
    "S3": ["verbreking", "3", "2", "5"],
    "S4": ["verbreking", "3", "10", "0"],
    "S5": ["geen"],
}

def start(programma, *argumenten):
    return subprocess.Popen([os.path.abspath(MAP + programma)] + list(argumenten),
                            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

def stop(proces):
    proces.kill()
    proces.wait()

IP_B = None
for argument in sys.argv[1:]:
    if argument.count(".") == 3:
        IP_B = argument

IP_A = "127.0.0.1"
if IP_B:
    udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp.connect((IP_B, 6000))
    IP_A = udp.getsockname()[0]
    udp.close()
    print("Laptop A:", IP_A, " Laptop B:", IP_B)

def naar_pc_b(opdracht):
    verbinding = socket.create_connection((IP_B, 6000))
    verbinding.sendall(opdracht.encode())
    antwoord = b""
    while True:
        stukje = verbinding.recv(4096)
        if not stukje:
            break
        antwoord += stukje
    verbinding.close()
    return antwoord.decode()

def start_b(programma, *argumenten):
    if IP_B:
        naar_pc_b("start " + programma + " " + " ".join(argumenten))
        return None
    return start(programma, *argumenten)

def stop_b(b):
    if IP_B:
        naar_pc_b("stop")
    else:
        stop(b)

def weggooien(*bestanden):
    for bestand in bestanden:
        if os.path.exists(bestand):
            os.remove(bestand)

def lees_teller():
    for poging in range(50):
        delen = open("teller.txt").read().strip().split(",")
        if len(delen) == 3:
            return int(delen[0]), int(delen[1]), int(delen[2])
        time.sleep(0.01)

def variant_test(variant, scenario, run):
    soort, getal = VARIANTEN[variant]
    weggooien("a_log.csv", "b_log.csv", "teller.txt")
    if IP_B:
        naar_pc_b("wis")

    if soort == "tcp":
        b_argumenten = [getal, "5000"]
        doel = "5000"
        if IP_B:
            doel = IP_B + ":5000"
        storing = start("storing.exe", "5001", doel, *SCENARIOS[scenario])
        b = start_b("tcp_b.exe", *b_argumenten)
        time.sleep(0.5)
        a = start("tcp_a.exe", getal, "127.0.0.1", "5001")
    else:
        subprocess.run([MOSQUITTO_SUB, "-i", "zoneA", "-t", "klaar", "-E"])
        subprocess.run([MOSQUITTO_SUB, "-i", "zoneB", "-t", "opdrachten", "-E"])
        b_argumenten = [getal, IP_A, "1884"]
        storing = start("storing.exe", "1884", "1883", *SCENARIOS[scenario])
        b = start_b("mqtt_b.exe", *b_argumenten)
        time.sleep(1)
        a = start("mqtt_a.exe", getal, "127.0.0.1", "1884")

    if scenario == "S5":
        time.sleep(4)
        stop_b(b)
        time.sleep(1)
        b = start_b(soort + "_b.exe", *b_argumenten)

    a.wait()
    time.sleep(1)
    bytes_, stukjes, _ = lees_teller()
    stop_b(b)
    stop(storing)

    if IP_B:
        open("b_log.csv", "w").write(naar_pc_b("log"))

    os.makedirs("resultaten/logs", exist_ok=True)
    shutil.copy("a_log.csv", f"resultaten/logs/{variant}_{scenario}_run{run}_a.csv")
    shutil.copy("b_log.csv", f"resultaten/logs/{variant}_{scenario}_run{run}_b.csv")

    uitslag = tel("a_log.csv", "b_log.csv")
    if scenario in ["S3", "S4"] and uitslag["hersteltijd_ms"] < 1000:
        print("LET OP: deze run heeft de storing gemist, draai hem opnieuw!")
    return [uitslag["verstuurd"], uitslag["verloren"], uitslag["dubbel"], uitslag["mislukt"], uitslag["niet_gemeld"],
            uitslag["mediaan_ms"], uitslag["p95_ms"], uitslag["max_ms"], uitslag["hersteltijd_ms"],
            round(bytes_ / 1000, 1), round(stukjes / 1000, 2)]

def heartbeat_test(interval, timeout):
    weggooien("hb_log.csv", "teller.txt")
    a = start("hb_a.exe", str(timeout), "7000")
    storing = start("storing.exe", "7001", "7000", "verlies", "20")
    time.sleep(0.5)
    b = start_b("hb_b.exe", str(interval), IP_A, "7001")
    time.sleep(20)
    stop_b(b)
    stop(storing)
    stop(a)
    valse_alarmen = open("hb_log.csv").read().count("alarm")

    weggooien("hb_log.csv", "teller.txt")
    a = start("hb_a.exe", str(timeout), "7000")
    storing = start("storing.exe", "7001", "7000", "onderbreking", "5", "10", "0")
    time.sleep(0.5)
    b = start_b("hb_b.exe", str(interval), IP_A, "7001")
    time.sleep(12)
    _, _, start_ms = lees_teller()
    stop_b(b)
    stop(storing)
    stop(a)

    uitval_ms = start_ms + 5000
    alarmen = [int(regel.split(",")[1]) for regel in open("hb_log.csv").read().splitlines() if regel.startswith("alarm")]
    na_uitval = [t for t in alarmen if t >= uitval_ms]
    detectie_ms = na_uitval[0] - uitval_ms if na_uitval else ""
    return [valse_alarmen, detectie_ms]

os.makedirs("resultaten", exist_ok=True)
alles = "varianten" not in sys.argv and "heartbeat" not in sys.argv

if alles or "varianten" in sys.argv:
    with open("resultaten/varianten.csv", "w") as uitvoer:
        uitvoer.write("variant,scenario,run,verstuurd,verloren,dubbel,mislukt,niet_gemeld,mediaan_ms,p95_ms,max_ms,"
                     "hersteltijd_ms,bytes_per_opdracht,stukjes_per_opdracht\n")
        for variant in VARIANTEN:
            for scenario in SCENARIOS:
                for run in range(1, HERHALINGEN + 1):
                    uitkomst = variant_test(variant, scenario, run)
                    regel = ",".join(str(x) for x in [variant, scenario, run] + uitkomst)
                    uitvoer.write(regel + "\n")
                    uitvoer.flush()
                    print(regel)

if alles or "heartbeat" in sys.argv:
    with open("resultaten/heartbeat.csv", "w") as uitvoer:
        uitvoer.write("interval_ms,timeout_ms,run,valse_alarmen,detectie_ms\n")
        for interval in [100, 250, 500, 1000]:
            for keer in [2, 3]:
                timeout = interval * keer
                for run in range(1, HERHALINGEN + 1):
                    uitkomst = heartbeat_test(interval, timeout)
                    regel = ",".join(str(x) for x in [interval, timeout, run] + uitkomst)
                    uitvoer.write(regel + "\n")
                    uitvoer.flush()
                    print(regel)

weggooien("a_log.csv", "b_log.csv", "hb_log.csv", "teller.txt")
print("Klaar!")