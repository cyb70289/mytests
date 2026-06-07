"""Tests for rl_ttt.viz \u2014 sanity print formatting and learning-curve plot
file creation. The visualization layer is mostly glue; we test format
guarantees and file artifacts, not pixel layout.
"""

from __future__ import annotations

import tempfile
from pathlib import Path

import numpy as np

from rl_ttt.agent import QLearningAgent
from rl_ttt.game import initial_board
from rl_ttt.viz import format_q_sanity, plot_learning_curves


# --- format_q_sanity --------------------------------------------------------

def test_format_q_sanity_contains_three_rows_with_nine_values():
    """The sanity grid shows Q[empty_board] reshaped 3x3 from side-to-move's
    view. Output must be a multi-line string with 9 numeric values."""
    agent = QLearningAgent(rng=np.random.default_rng(0))
    # Seed Q values for the empty board so the output isn't all zeros.
    q = agent.q_values(initial_board())
    q[:] = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9]
    out = format_q_sanity(agent)
    # Should contain each value (rounded to 2 decimals) somewhere.
    for v in [0.10, 0.20, 0.30, 0.40, 0.50, 0.60, 0.70, 0.80, 0.90]:
        assert f"{v:.2f}" in out, f"value {v:.2f} not in:\n{out}"


def test_format_q_sanity_includes_empty_board_marker_in_header():
    """A header line should make it obvious we're showing the empty-board
    Q-values from side-to-move's perspective."""
    agent = QLearningAgent(rng=np.random.default_rng(0))
    out = format_q_sanity(agent)
    lower = out.lower()
    assert "empty" in lower or "initial" in lower or "start" in lower


def test_format_q_sanity_handles_unseen_empty_board():
    """If the agent has never seen the empty board, format_q_sanity must
    not crash \u2014 it should lazily initialize and print zeros."""
    agent = QLearningAgent(rng=np.random.default_rng(0))
    # Don't touch the empty board; agent.num_states == 0 going in.
    assert agent.num_states == 0
    out = format_q_sanity(agent)
    # 0.00 should appear (9 cells, all empty \u2192 all zeros).
    assert "0.00" in out


# --- plot_learning_curves ---------------------------------------------------

def _fake_history(n_cycles: int) -> list[dict]:
    """Build a synthetic metrics history matching the schema train.py emits."""
    history = []
    for i in range(n_cycles):
        win_r = min(1.0, 0.5 + 0.1 * i)
        draw_r = max(0.0, 0.3 - 0.05 * i)
        loss_r = max(0.0, 1.0 - win_r - draw_r)
        history.append({
            "episode": (i + 1) * 1000,
            "epsilon": max(0.05, 1.0 - 0.2 * i),
            "qtable_size": 500 + 50 * i,
            "win_vs_random":   win_r,
            "draw_vs_random":  draw_r,
            "loss_vs_random":  loss_r,
            "win_vs_minimax":  0.0,
            "draw_vs_minimax": min(1.0, 0.1 * i),
            "loss_vs_minimax": max(0.0, 1.0 - 0.1 * i),
        })
    return history


def test_plot_learning_curves_creates_png_file():
    history = _fake_history(10)
    with tempfile.TemporaryDirectory() as td:
        out_path = Path(td) / "curves.png"
        plot_learning_curves(history, out_path)
        assert out_path.exists()
        # PNG file should be > 1 KB (a non-empty figure).
        assert out_path.stat().st_size > 1024


def test_plot_learning_curves_creates_parent_directory():
    history = _fake_history(5)
    with tempfile.TemporaryDirectory() as td:
        out_path = Path(td) / "nested" / "subdir" / "curves.png"
        plot_learning_curves(history, out_path)
        assert out_path.exists()
