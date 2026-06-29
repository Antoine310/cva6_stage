#!/usr/bin/env python3

import os
import time
import subprocess
from pathlib import Path

HIST = Path.home() / ".fpga_history"
LOG = "fpga_build.log"

# -----------------------
# Historique
# -----------------------

durations = []

if HIST.exists():
    with open(HIST) as f:
        durations = [
            int(x.strip())
            for x in f
            if x.strip().isdigit()
        ]

avg = (
    sum(durations) / len(durations)
    if durations else
    3600  # 1h par défaut pour le premier run
)

# -----------------------
# Lancement build
# -----------------------

log_file = open(LOG, "w")

start = time.time()

proc = subprocess.Popen(
    ["make", "fpga"],
    stdout=log_file,
    stderr=subprocess.STDOUT,
)

try:

    while proc.poll() is None:

        elapsed = int(time.time() - start)

        remaining = max(int(avg - elapsed), 0)

        bar_len = 40
        time_percent = min(int(elapsed / avg * 100), 99)
        filled = int(bar_len * time_percent / 100)

        os.system("clear")

        print("=================================")
        print(" FPGA BUILD MONITOR")
        print("=================================\n")

        print(
            "[" +
            "#" * filled +
            "-" * (bar_len - filled) +
            f"] {time_percent}%\n"
        )

        print(
            f"Temps écoulé : "
            f"{elapsed//60:02d}m{elapsed%60:02d}s"
        )

        if elapsed <= avg:
            print(
                f"Temps restant estimé : "
                f"{remaining//60:02d}m{remaining%60:02d}s"
            )
        else:
            print(
                "Temps restant estimé : "
                "dépassement de la moyenne"
            )

        print()

        print(
            f"Durée moyenne historique : "
            f"{int(avg)//60:02d}m{int(avg)%60:02d}s"
        )

        print(
            f"Nombre de synthèses : "
            f"{len(durations)}"
        )

        print("\nEtat : RUNNING")

        time.sleep(2)

finally:

    proc.wait()
    log_file.close()

# -----------------------
# Fin du build
# -----------------------

duration = int(time.time() - start)

with open(HIST, "a") as f:
    f.write(f"{duration}\n")

durations.append(duration)

new_avg = sum(durations) / len(durations)

os.system("clear")

print("=================================")
print(" BUILD COMPLETE")
print("=================================\n")

print(
    f"Synthèse terminée en "
    f"{duration//60}m{duration%60:02d}s"
)

print(
    f"Nouvelle moyenne : "
    f"{int(new_avg)//60}m{int(new_avg)%60:02d}s"
)

delta = duration - avg

print(f"Ecart : {delta:+.0f} s")

print("\nHistorique mis à jour")
print(f"Log sauvegardé : {LOG}")