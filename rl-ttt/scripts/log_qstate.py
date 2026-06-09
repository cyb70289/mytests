"""Log Q-values for target states during training.

Produces a JS file (window.QSTATEDATA = {...}) with per-episode snapshots
of Q-values for multiple canonical board states. Runs phase 1a + phase 1b
training via the train module with a per-episode callback.

Usage:
    python scripts/log_qstate.py --out web/qstate-data.js --seed 0

The output JS file is loaded by web/qstate-animation.html via a plain
<script> tag so the page works from file:// without an HTTP server.
"""

from __future__ import annotations

import argparse
import datetime
import json
import sys
from pathlib import Path

import numpy as np

# Ensure the package root is on the path so `rl_ttt` imports work when run
# directly (e.g. `python scripts/log_qstate.py` from the project root).
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from rl_ttt.game import Board
from rl_ttt.agent import QLearningAgent
from rl_ttt.train import TrainConfig, train_phase_1a, train_phase_1b


# Canonical states where the opponent's piece occupies a cell and it is the
# trainee's turn to move.  From the trainee's (O's) perspective:
#   X's piece = OPP = -1
#   O's piece = ME  = +1 (none placed yet, all other cells are empty)
CASES: list[dict] = [
    {
        "state": (0, 0, 0, 0, -1, 0, 0, 0, 0),
        "label": "X in center, O to move",
        "piece_pos": 4,
    },
    {
        "state": (0, 0, -1, 0, 0, 0, 0, 0, 0),
        "label": "X in upper right, O to move",
        "piece_pos": 2,
    },
]

PHASE_CONFIGS = {
    "1a": {
        "episodes": 50000,
        "alpha": 0.1,
        "gamma": 1.0,
        "eps_start": 1.0,
        "eps_end": 0.1,
    },
    "1b": {
        "episodes": 100000,
        "alpha": 0.1,
        "gamma": 1.0,
        "eps_start": 0.3,
        "eps_end": 0.05,
    },
}


def _q_array_to_json(q: np.ndarray) -> list[float | None]:
    """Convert a 9-element Q-array to a JSON-safe list: NaN -> null."""
    return [None if np.isnan(v) else float(v) for v in q]


class _CaseLogger:
    """Tracks snapshots and dedup state for one target board."""

    def __init__(self, case: dict) -> None:
        self.state: Board = case["state"]
        self.snapshots: list[dict] = []
        self._last_q: list[float | None] | None = None

    def record(self, global_ep: int, agent: QLearningAgent) -> None:
        if not agent.has_state(self.state):
            return
        q_list = _q_array_to_json(agent.q_values(self.state))
        if self._last_q is None or q_list != self._last_q:
            self._last_q = q_list
            self.snapshots.append({"ep": global_ep, "q": q_list})

    def snapshot_count(self, after_ep: int) -> int:
        return sum(1 for s in self.snapshots if s["ep"] <= after_ep)


def run(seed: int, out_path: Path, quiet: bool = False) -> None:
    loggers = [_CaseLogger(c) for c in CASES]

    # Phase 1a
    cfg_1a = TrainConfig(
        episodes=PHASE_CONFIGS["1a"]["episodes"],
        alpha=PHASE_CONFIGS["1a"]["alpha"],
        gamma=PHASE_CONFIGS["1a"]["gamma"],
        eps_start=PHASE_CONFIGS["1a"]["eps_start"],
        eps_end=PHASE_CONFIGS["1a"]["eps_end"],
        eps_decay_episodes=PHASE_CONFIGS["1a"]["episodes"] // 2,
        eval_every=1000,
        eval_games_per_opponent=200,
        seed=seed,
    )

    if not quiet:
        print("=== Phase 1a: training vs random ===", flush=True)

    def cb_1a(ep: int, agent: QLearningAgent) -> None:
        global_ep = ep + 1
        for logger in loggers:
            logger.record(global_ep, agent)

    agent_1a, _history = train_phase_1a(cfg_1a, progress=not quiet, per_episode_callback=cb_1a)

    if not quiet:
        for i, logger in enumerate(loggers):
            print(f"  Case {i} ({CASES[i]['label']}): {logger.snapshot_count(PHASE_CONFIGS['1a']['episodes'])} snapshots", flush=True)

    # Phase 1b
    cfg_1b = TrainConfig(
        episodes=PHASE_CONFIGS["1b"]["episodes"],
        alpha=PHASE_CONFIGS["1b"]["alpha"],
        gamma=PHASE_CONFIGS["1b"]["gamma"],
        eps_start=PHASE_CONFIGS["1b"]["eps_start"],
        eps_end=PHASE_CONFIGS["1b"]["eps_end"],
        eps_decay_episodes=PHASE_CONFIGS["1b"]["episodes"] // 2,
        eval_every=1000,
        eval_games_per_opponent=200,
        seed=seed,
    )

    if not quiet:
        print("=== Phase 1b: self-play ===", flush=True)

    phase_offset = PHASE_CONFIGS["1a"]["episodes"]

    def cb_1b(ep: int, agent: QLearningAgent) -> None:
        global_ep = phase_offset + ep + 1
        for logger in loggers:
            logger.record(global_ep, agent)

    train_phase_1b(
        cfg_1b, warm_start=agent_1a, progress=not quiet, per_episode_callback=cb_1b
    )

    total_episodes = cfg_1a.episodes + cfg_1b.episodes

    if not quiet:
        for i, logger in enumerate(loggers):
            phase_1b_count = logger.snapshot_count(total_episodes) - logger.snapshot_count(phase_offset)
            print(f"  Case {i} ({CASES[i]['label']}): {phase_1b_count} new, {logger.snapshot_count(total_episodes)} total", flush=True)

    payload = {
        "total_episodes": total_episodes,
        "phase_boundary": cfg_1a.episodes,
        "seed": seed,
        "cases": [
            {
                "state": list(c["state"]),
                "label": c["label"],
                "piece_pos": c["piece_pos"],
                "snapshots": logger.snapshots,
            }
            for c, logger in zip(CASES, loggers)
        ],
    }

    out_path.parent.mkdir(parents=True, exist_ok=True)
    inner = json.dumps(payload, separators=(",", ":"))
    ts = datetime.datetime.now().isoformat(timespec="seconds")
    header_lines = [
        f"// Generated by rl_ttt log_qstate on {ts}.",
        f"// Seed: {seed}.  {len(payload['cases'])} cases, "
        f"{sum(len(logger.snapshots) for logger in loggers)} total snapshots.",
    ]
    header = "\n".join(header_lines) + "\n"
    with open(out_path, "w") as f:
        f.write(header + f"window.QSTATEDATA = {inner};\n")
    if not quiet:
        total = sum(len(logger.snapshots) for logger in loggers)
        print(f"\nSaved {total} snapshots ({len(payload['cases'])} cases) to {out_path}")


def main() -> int:
    p = argparse.ArgumentParser(
        description="Log Q-values for target states during full training."
    )
    p.add_argument("--out", type=str, default="web/qstate-data.js",
                   help="Output JS path.")
    p.add_argument("--seed", type=int, default=0)
    p.add_argument("--quiet", action="store_true")
    args = p.parse_args()
    run(seed=args.seed, out_path=Path(args.out), quiet=args.quiet)
    return 0


if __name__ == "__main__":
    sys.exit(main())
