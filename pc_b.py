import os
import socket
import subprocess

MAP = "bin/"
if not os.path.exists(MAP + "tcp_b.exe"):
    MAP = "bin/Debug/"

server = socket.socket()
server.bind(("0.0.0.0", 6000))
server.listen(1)
print("pc_b.py wacht op laptop A (poort 6000)...")

b = None

while True:
    verbinding, _ = server.accept()
    opdracht = verbinding.recv(1000).decode().split()
    antwoord = "ok"

    if opdracht[0] == "start":
        b = subprocess.Popen([os.path.abspath(MAP + opdracht[1])] + opdracht[2:],
                             stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    elif opdracht[0] == "stop" and b:
        b.kill()
        b.wait()
        b = None
    elif opdracht[0] == "wis" and os.path.exists("b_log.csv"):
        os.remove("b_log.csv")
    elif opdracht[0] == "log":
        antwoord = ""
        if os.path.exists("b_log.csv"):
            antwoord = open("b_log.csv").read()

    print(" ".join(opdracht))
    verbinding.sendall(antwoord.encode())
    verbinding.close()