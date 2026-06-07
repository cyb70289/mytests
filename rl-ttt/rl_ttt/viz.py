"""Visualization helpers: sanity print of Q-values for the empty board,
and learning-curve plots.

These are pedagogical aids \u2014 the goal is to make the agent's progress and
its learned values directly visible to a human reader.
"""

from __future__ import annotations

from pathlib import Path

import matplotlib

# Use a non-interactive backend so plot generation works in headless
# environments (CI, ssh, etc.) without DISPLAY.
matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

from rl_ttt.agent import QLearningAgent
from rl_ttt.game import initial_board


def format_q_sanity(agent: QLearningAgent) -> str:
    """Return a multi-line string showing Q[empty_board] reshaped 3x3.

    Side-to-move convention: the empty board is the same from either player's
    perspective, and the first mover's expected value at each cell is what
    we want to inspect. We expect after training:
        center (cell 4) > corners (0, 2, 6, 8) > edges (1, 3, 5, 7)
    """
    q = agent.q_values(initial_board())   # lazy-inits to all zeros if unseen
    rows = ["Empty-board Q-values (first-mover's perspective):"]
    for r in range(3):
        cells = [f"{q[r * 3 + c]:6.2f}" for c in range(3)]
        line = "  " + "  ".join(cells)
        if r == 1:
            line += "    [center=4]"
        rows.append(line)
    rows.append(f"States seen: {agent.num_states}")
    return "\n".join(rows)


def plot_learning_curves(
    history: list[dict],
    out_path: str | Path,
) -> None:
    """Plot win/draw/loss vs random and vs minimax over training, plus the
    Q-table-size growth curve. Saves a single PNG with 3 stacked panels.
    """
    out_path = Path(out_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)

    episodes = [h["episode"] for h in history]

    fig, axes = plt.subplots(3, 1, figsize=(9, 10), sharex=True)

    # Panel 1: vs random
    ax = axes[0]
    ax.plot(episodes, [h["win_vs_random"]  for h in history], label="win",  color="tab:green")
    ax.plot(episodes, [h["draw_vs_random"] for h in history], label="draw", color="tab:gray")
    ax.plot(episodes, [h["loss_vs_random"] for h in history], label="loss", color="tab:red")
    ax.set_ylim(-0.05, 1.05)
    ax.set_ylabel("fraction of games")
    ax.set_title("Evaluation vs random opponent (\u03b5=0)")
    ax.legend(loc="center right")
    ax.grid(alpha=0.3)

    # Panel 2: vs minimax
    ax = axes[1]
    ax.plot(episodes, [h["win_vs_minimax"]  for h in history], label="win",  color="tab:green")
    ax.plot(episodes, [h["draw_vs_minimax"] for h in history], label="draw", color="tab:gray")
    ax.plot(episodes, [h["loss_vs_minimax"] for h in history], label="loss", color="tab:red")
    ax.set_ylim(-0.05, 1.05)
    ax.set_ylabel("fraction of games")
    ax.set_title("Evaluation vs minimax-optimal opponent (\u03b5=0)")
    ax.legend(loc="center right")
    ax.grid(alpha=0.3)

    # Panel 3: Q-table growth
    ax = axes[2]
    ax.plot(episodes, [h["qtable_size"] for h in history], color="tab:blue")
    ax.set_ylabel("# states in Q-table")
    ax.set_xlabel("training episodes")
    ax.set_title("Q-table coverage over training")
    ax.grid(alpha=0.3)

    fig.tight_layout()
    fig.savefig(out_path, dpi=110)
    plt.close(fig)
