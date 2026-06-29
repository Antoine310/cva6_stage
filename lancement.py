#!/usr/bin/env python3

import os
import time
import subprocess
from pathlib import Path

HIST = Path.home() / ".fpga_history"
LOG = "fpga_build.log"

STAGES = [
    ("Launched synth_1", 15, "Synthèse"),
    ("Starting open_checkpoint Task", 40, "Open Checkpoint"),
    ("Reading XDEF placement", 50, "Placement"),
    ("Reading XDEF routing", 70, "Routage"),
    ("DRC finished", 85, "DRC"),
    ("Running write_bitstream", 92, "Bitstream"),
    ("write_bitstream completed successfully", 100, "Terminé"),
]

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
    3600
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

current_stage = "Initialisation"
current_percent = 0

try:

    while proc.poll() is None:

        elapsed = int(time.time() - start)

        if os.path.exists(LOG):

            with open(LOG, "r", errors="ignore") as f:
                txt = f.read()

            for keyword, pct, stage in STAGES:
                if keyword in txt:
                    current_percent = pct
                    current_stage = stage

        remaining = max(int(avg - elapsed), 0)

        bar_len = 40
        filled = int(bar_len * current_percent / 100)

        os.system("clear")

        print("=================================")
        print(" FPGA BUILD MONITOR")
        print("=================================\n")

        print(f"Etape : {current_stage}\n")

        print(
            "[" +
            "#" * filled +
            "-" * (bar_len - filled) +
            f"] {current_percent}%\n"
        )

        print(
            f"Temps écoulé : "
            f"{elapsed//60:02d}m{elapsed%60:02d}s"
        )

        print(
            f"Temps restant estimé : "
            f"{remaining//60:02d}m{remaining%60:02d}s"
        )

        print()

        print(
            f"Durée moyenne : "
            f"{int(avg)//60:02d}m{int(avg)%60:02d}s"
        )

        print(
            f"Synthèses précédentes : "
            f"{len(durations)}"
        )

        print("\nEtat : RUNNING")

        time.sleep(2)

finally:

    proc.wait()
    log_file.close()

duration = int(time.time() - start)

with open(HIST, "a") as f:
    f.write(f"{duration}\n")

durations.append(duration)

new_avg = sum(durations) / len(durations)

os.system("clear")

print("=================================")
print(" BUILD COMPLETE")
print("=================================\n")

print(f"Durée réelle : {duration//60}m{duration%60:02d}s")
print(f"Moyenne historique : {int(new_avg)//60}m{int(new_avg)%60:02d}s")

delta = duration - avg

print(f"Ecart : {delta:+.0f} s")

print("\nHistorique mis à jour")
print(f"Log sauvegardé : {LOG}")