"""Combine phase-1a and phase-1b runs into a single dataset + plot.

Reads:
    artifacts/full_run_1a/metrics.csv
    artifacts/full_run_1b/metrics.csv

Writes:
    artifacts/full_run/metrics.csv         (combined, with `phase` column)
    artifacts/full_run/learning_curves.png  (3-panel plot, phase boundary at x=50k)
    artifacts/full_run/qtable.pkl          (copy of post-1b agent)
"""
from __future__ import annotations

import csv
import shutil
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

ROOT = Path(__file__).resolve().parent.parent
A_CSV = ROOT / "artifacts/full_run_1a/metrics.csv"
B_CSV = ROOT / "artifacts/full_run_1b/metrics.csv"
OUT_DIR = ROOT / "artifacts/full_run"
OUT_CSV = OUT_DIR / "metrics.csv"
OUT_PNG = OUT_DIR / "learning_curves.png"
OUT_PKL = OUT_DIR / "qtable.pkl"


def load(path: Path) -> list[dict]:
    with open(path) as f:
        return list(csv.DictReader(f))


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    rows_a = load(A_CSV)
    rows_b = load(B_CSV)
    a_last_ep = int(rows_a[-1]["episode"])
    b_offset = a_last_ep  # 1b episodes are numbered 1..100000; we'll add offset

    for r in rows_a:
        r["phase"] = "1a"
    for r in rows_b:
        r["phase"] = "1b"
        r["episode"] = str(int(r["episode"]) + b_offset)

    combined = rows_a + rows_b
    fieldnames = list(combined[0].keys())
    # Put phase first.
    fieldnames = ["phase"] + [k for k in fieldnames if k != "phase"]
    with open(OUT_CSV, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fieldnames)
        w.writeheader()
        w.writerows(combined)
    print(f"wrote {OUT_CSV}  ({len(combined)} rows)")

    shutil.copy(ROOT / "artifacts/full_run_1b/qtable.pkl", OUT_PKL)
    print(f"wrote {OUT_PKL}")

    # --- plot ---------------------------------------------------------------
    eps_a = np.array([int(r["episode"]) for r in rows_a])
    eps_b = np.array([int(r["episode"]) for r in rows_b])
    def col(rows, key):
        return np.array([float(r[key]) for r in rows])

    fig, axes = plt.subplots(1, 3, figsize=(15, 4.5))

    # Panel 1: vs random (stacked area: win / draw / loss)
    ax = axes[0]
    ax.fill_between(eps_a, 0, col(rows_a, "win_vs_random"),
                    color="#2ca02c", alpha=0.7, label="win")
    ax.fill_between(eps_a, col(rows_a, "win_vs_random"),
                    col(rows_a, "win_vs_random") + col(rows_a, "draw_vs_random"),
                    color="#1f77b4", alpha=0.7, label="draw")
    ax.fill_between(eps_a, col(rows_a, "win_vs_random") + col(rows_a, "draw_vs_random"),
                    1.0, color="#d62728", alpha=0.7, label="loss")
    ax.fill_between(eps_b, 0, col(rows_b, "win_vs_random"),
                    color="#2ca02c", alpha=0.7)
    ax.fill_between(eps_b, col(rows_b, "win_vs_random"),
                    col(rows_b, "win_vs_random") + col(rows_b, "draw_vs_random"),
                    color="#1f77b4", alpha=0.7)
    ax.fill_between(eps_b, col(rows_b, "win_vs_random") + col(rows_b, "draw_vs_random"),
                    1.0, color="#d62728", alpha=0.7)
    ax.axvline(a_last_ep, color="k", linestyle="--", alpha=0.5)
    ax.text(a_last_ep + 1500, 0.95, "phase 1b\n(self-play)", fontsize=9)
    ax.set_title("Outcome distribution vs random")
    ax.set_xlabel("episode")
    ax.set_ylabel("fraction")
    ax.set_ylim(0, 1)
    ax.set_xlim(0, eps_b[-1])
    ax.legend(loc="lower right", fontsize=9)

    # Panel 2: vs minimax (draw-rate only; win and loss are tiny in good runs)
    ax = axes[1]
    ax.plot(eps_a, col(rows_a, "draw_vs_minimax"), color="#1f77b4",
            label="draw vs minimax")
    ax.plot(eps_b, col(rows_b, "draw_vs_minimax"), color="#1f77b4")
    ax.plot(eps_a, col(rows_a, "loss_vs_minimax"), color="#d62728",
            label="loss vs minimax")
    ax.plot(eps_b, col(rows_b, "loss_vs_minimax"), color="#d62728")
    ax.axvline(a_last_ep, color="k", linestyle="--", alpha=0.5)
    ax.set_title("Performance vs perfect (minimax)")
    ax.set_xlabel("episode")
    ax.set_ylabel("fraction")
    ax.set_ylim(0, 1.05)
    ax.set_xlim(0, eps_b[-1])
    ax.legend(loc="lower left", fontsize=9)

    # Panel 3: Q-table size growth
    ax = axes[2]
    ax.plot(eps_a, col(rows_a, "qtable_size"), color="#1f77b4",
            label="phase 1a")
    ax.plot(eps_b, col(rows_b, "qtable_size"), color="#ff7f0e",
            label="phase 1b")
    ax.axvline(a_last_ep, color="k", linestyle="--", alpha=0.5)
    ax.set_title("Q-table size (distinct states visited)")
    ax.set_xlabel("episode")
    ax.set_ylabel("# states")
    ax.set_xlim(0, eps_b[-1])
    ax.legend(loc="lower right", fontsize=9)

    fig.suptitle("rl-ttt full training: 50k phase-1a (vs random) + 100k phase-1b (self-play)",
                 fontsize=12)
    fig.tight_layout()
    fig.savefig(OUT_PNG, dpi=110)
    print(f"wrote {OUT_PNG}")


if __name__ == "__main__":
    main()
