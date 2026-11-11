import statistics
import sys

def tel(a_bestand, b_bestand):
    a_log = open(a_bestand).read().splitlines()
    b_log = open(b_bestand).read().splitlines()

    verstuurd = [regel.split(",")[0] for regel in a_log]

    uitgevoerd = b_log

    verloren = [nummer for nummer in verstuurd if nummer not in uitgevoerd]

    dubbel = [nummer for nummer in set(uitgevoerd) if uitgevoerd.count(nummer) > 1]

    mislukt = [regel.split(",")[0] for regel in a_log if ",mislukt," in regel]

    niet_gemeld = [nummer for nummer in mislukt if nummer in uitgevoerd]

    tijden = sorted(float(regel.split(",")[2]) for regel in a_log if ",gelukt," in regel)

    p95 = tijden[int(len(tijden) * 0.95)] if tijden else 0

    tijdstippen = [float(regel.split(",")[3]) for regel in a_log if ",gelukt," in regel]
    gaten = [na - voor for voor, na in zip(tijdstippen, tijdstippen[1:])]
    hersteltijd = max(gaten) if gaten else 0

    return {
        "verstuurd": len(verstuurd),
        "verloren": len(verloren),
        "dubbel": len(dubbel),
        "mislukt": len(mislukt),
        "niet_gemeld": len(niet_gemeld),
        "mediaan_ms": round(statistics.median(tijden), 2) if tijden else 0,
        "p95_ms": round(p95, 2),
        "max_ms": round(max(tijden), 2) if tijden else 0,
        "hersteltijd_ms": round(hersteltijd, 2),
        "verloren_nummers": verloren,
        "dubbel_nummers": dubbel,
    }

def mediaan_in_resultaten():
    regels = open("resultaten/varianten.csv").read().splitlines()
    kop = regels[0].split(",")
    if "gem_ms" in kop:
        plek = kop.index("gem_ms")
    else:
        plek = kop.index("mediaan_ms")
    kop[plek] = "mediaan_ms"

    nieuw = [",".join(kop)]
    for regel in regels[1:]:
        velden = regel.split(",")
        variant, scenario, run = velden[0], velden[1], velden[2]
        log = f"resultaten/logs/{variant}_{scenario}_run{run}"
        uitslag = tel(log + "_a.csv", log + "_b.csv")
        velden[plek] = str(uitslag["mediaan_ms"])
        nieuw.append(",".join(velden))

    open("resultaten/varianten.csv", "w").write("\n".join(nieuw) + "\n")
    print("Mediaan toegevoegd aan resultaten/varianten.csv")


if __name__ == "__main__" and len(sys.argv) > 1 and sys.argv[1] == "resultaten":
    mediaan_in_resultaten()
elif __name__ == "__main__":
    uitslag = tel("a_log.csv", "b_log.csv")
    print("Verstuurd: ", uitslag["verstuurd"])
    print("Verloren:  ", uitslag["verloren"], uitslag["verloren_nummers"])
    print("Dubbel:    ", uitslag["dubbel"], uitslag["dubbel_nummers"])
    print("Mislukt volgens A:", uitslag["mislukt"])
    print("Mislukt, maar wel uitgevoerd:", uitslag["niet_gemeld"])
    print("Mediaan vertraging:   ", uitslag["mediaan_ms"], "ms")
    print("Langzaamste 5%:       ", uitslag["p95_ms"], "ms")
    print("Langste vertraging:   ", uitslag["max_ms"], "ms")
    print("Hersteltijd:          ", uitslag["hersteltijd_ms"], "ms")