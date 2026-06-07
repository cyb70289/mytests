"""Tests for rl_ttt.agent.QLearningAgent \u2014 Q-table, \u03b5-greedy action
selection, Q-update math, and save/load round-trip.
"""

from __future__ import annotations

import tempfile
from collections import Counter
from pathlib import Path

import numpy as np
import pytest

from rl_ttt.game import (
    ME, OPP, EMPTY,
    initial_board, apply_move,
)
from rl_ttt.agent import QLearningAgent


# --- q_values: lazy init -----------------------------------------------------

def test_q_values_lazy_init_zeros_with_nan_at_illegal():
    agent = QLearningAgent()
    #   X . O         cells 0 and 2 are illegal (occupied)
    #   . X .         cell 4 illegal
    #   . . .
    b = (ME, EMPTY, OPP, EMPTY, ME, EMPTY, EMPTY, EMPTY, EMPTY)
    q = agent.q_values(b)
    assert q.shape == (9,)
    assert q.dtype == np.float64
    assert np.isnan(q[0])    # occupied
    assert q[1] == 0.0       # empty
    assert np.isnan(q[2])    # occupied
    assert q[3] == 0.0
    assert np.isnan(q[4])    # occupied
    assert q[5] == 0.0
    assert q[6] == 0.0
    assert q[7] == 0.0
    assert q[8] == 0.0


def test_q_values_same_state_returns_same_array_identity():
    """Returning an alias (not a copy) lets the Q-update mutate in place."""
    agent = QLearningAgent()
    b = initial_board()
    q1 = agent.q_values(b)
    q2 = agent.q_values(b)
    assert q1 is q2  # identity, not equality


def test_q_values_initial_q_table_is_empty():
    agent = QLearningAgent()
    assert agent.num_states == 0


def test_q_values_increments_num_states_on_new_state():
    agent = QLearningAgent()
    agent.q_values(initial_board())
    assert agent.num_states == 1
    # Same state again: still 1.
    agent.q_values(initial_board())
    assert agent.num_states == 1
    # Different state: 2.
    agent.q_values(apply_move(initial_board(), 4))
    assert agent.num_states == 2


# --- select_action: epsilon = 0 (pure greedy) --------------------------------

def test_select_action_epsilon_zero_returns_argmax():
    agent = QLearningAgent(epsilon=0.0, rng=np.random.default_rng(0))
    b = initial_board()
    # Manually set the Q-values so cell 4 is the unique max.
    q = agent.q_values(b)
    q[:] = 0.0
    q[4] = 1.0
    for _ in range(50):
        a = agent.select_action(b)
        assert a == 4


def test_select_action_epsilon_zero_breaks_ties_randomly():
    """With all legal Q-values equal (e.g., freshly-initialized 0.0), every
    legal action should be selected with roughly uniform probability."""
    agent = QLearningAgent(epsilon=0.0, rng=np.random.default_rng(0))
    b = initial_board()  # all 9 cells legal, all Q-values 0
    counts = Counter(agent.select_action(b) for _ in range(9000))
    # Each of the 9 cells should appear roughly 1000 times. Allow generous
    # tolerance (\u00b150%) to keep the test robust to RNG variance.
    for a in range(9):
        assert 500 <= counts[a] <= 1500, f"action {a}: {counts[a]}/9000 \u2014 not uniform"


def test_select_action_never_picks_illegal_action():
    agent = QLearningAgent(epsilon=0.5, rng=np.random.default_rng(0))
    #   X . O         legal cells: 1, 3, 5, 6, 7, 8
    #   . X .
    #   . . .
    b = (ME, EMPTY, OPP, EMPTY, ME, EMPTY, EMPTY, EMPTY, EMPTY)
    legal = {1, 3, 5, 6, 7, 8}
    for _ in range(2000):
        a = agent.select_action(b)
        assert a in legal


# --- select_action: epsilon = 1.0 (pure random) ------------------------------

def test_select_action_epsilon_one_is_uniform_over_legal():
    agent = QLearningAgent(epsilon=1.0, rng=np.random.default_rng(0))
    b = initial_board()
    counts = Counter(agent.select_action(b) for _ in range(9000))
    for a in range(9):
        assert 500 <= counts[a] <= 1500, f"action {a}: {counts[a]}/9000 \u2014 not uniform"


def test_select_action_epsilon_one_ignores_q_values():
    """Even when one action has a very high Q-value, \u03b5=1 picks uniformly."""
    agent = QLearningAgent(epsilon=1.0, rng=np.random.default_rng(0))
    b = initial_board()
    q = agent.q_values(b)
    q[4] = 100.0  # huge bonus for cell 4
    counts = Counter(agent.select_action(b) for _ in range(9000))
    # cell 4 should still be picked ~1/9 of the time, not always.
    assert 500 <= counts[4] <= 1500


# --- update: terminal --------------------------------------------------------

def test_update_terminal_uses_no_bootstrap_win():
    """Hand-computed: \u03b1=0.5, Q[s][a]=0, r=+1, terminal \u2192 Q'[s][a] = 0.5."""
    agent = QLearningAgent(alpha=0.5, gamma=1.0, rng=np.random.default_rng(0))
    s = initial_board()
    a = 4
    agent.q_values(s)  # initialize
    agent.update(s, a, reward=+1.0, s_next=None)
    assert agent.q_values(s)[a] == pytest.approx(0.5)


def test_update_terminal_uses_no_bootstrap_loss():
    """\u03b1=0.5, Q[s][a]=0, r=-1, terminal \u2192 Q'[s][a] = -0.5."""
    agent = QLearningAgent(alpha=0.5, gamma=1.0, rng=np.random.default_rng(0))
    s = initial_board()
    a = 4
    agent.q_values(s)
    agent.update(s, a, reward=-1.0, s_next=None)
    assert agent.q_values(s)[a] == pytest.approx(-0.5)


def test_update_terminal_uses_no_bootstrap_draw():
    """r=0 terminal \u2192 Q'[s][a] unchanged from 0."""
    agent = QLearningAgent(alpha=0.5, gamma=1.0, rng=np.random.default_rng(0))
    s = initial_board()
    a = 4
    agent.q_values(s)
    agent.update(s, a, reward=0.0, s_next=None)
    assert agent.q_values(s)[a] == pytest.approx(0.0)


# --- update: non-terminal (with bootstrap) -----------------------------------

def test_update_non_terminal_includes_bootstrap():
    """\u03b1=0.5, \u03b3=1, Q[s][a]=0, r=0, max Q[s']=1.0 \u2192 Q'[s][a] = 0.5."""
    agent = QLearningAgent(alpha=0.5, gamma=1.0, rng=np.random.default_rng(0))
    s = initial_board()
    a = 4
    s_next = apply_move(s, 4)
    agent.q_values(s)
    q_next = agent.q_values(s_next)
    # Set one cell to 1.0 so nanmax(q_next) = 1.0.
    # cell 4 is now illegal (NaN), set cell 0 to 1.0.
    q_next[0] = 1.0
    agent.update(s, a, reward=0.0, s_next=s_next)
    assert agent.q_values(s)[a] == pytest.approx(0.5)


def test_update_non_terminal_with_gamma_half():
    """\u03b1=0.5, \u03b3=0.5, Q[s][a]=0, r=0, max Q[s']=1 \u2192 Q'[s][a] = 0.25."""
    agent = QLearningAgent(alpha=0.5, gamma=0.5, rng=np.random.default_rng(0))
    s = initial_board()
    a = 4
    s_next = apply_move(s, 4)
    agent.q_values(s)
    agent.q_values(s_next)[0] = 1.0
    agent.update(s, a, reward=0.0, s_next=s_next)
    assert agent.q_values(s)[a] == pytest.approx(0.25)


def test_update_does_not_mutate_nan_illegal_cells():
    """Q-update should only touch Q[s][a], leaving NaN cells untouched."""
    agent = QLearningAgent(alpha=0.5, gamma=1.0, rng=np.random.default_rng(0))
    s = (ME, EMPTY, OPP, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY)
    a = 4  # cell 4 is legal
    q_before = agent.q_values(s).copy()
    agent.update(s, a, reward=+1.0, s_next=None)
    q_after = agent.q_values(s)
    # Cells 0 and 2 are illegal \u2014 must remain NaN.
    assert np.isnan(q_after[0])
    assert np.isnan(q_after[2])
    # Other unchanged cells stay at 0.
    for i in [1, 3, 5, 6, 7, 8]:
        assert q_after[i] == q_before[i]
    # Cell 4 updated.
    assert q_after[4] == pytest.approx(0.5)


# --- save / load round-trip --------------------------------------------------

def test_save_load_round_trip_preserves_q_table_and_hyperparams():
    agent = QLearningAgent(alpha=0.07, gamma=0.93, epsilon=0.42, rng=np.random.default_rng(0))
    # Seed a few states with non-trivial values.
    s1 = initial_board()
    s2 = apply_move(s1, 4)
    agent.q_values(s1)[0] = 0.123
    agent.q_values(s2)[1] = -0.456
    with tempfile.TemporaryDirectory() as td:
        p = Path(td) / "qtable.pkl"
        agent.save(p)
        loaded = QLearningAgent.load(p)
    assert loaded.alpha == pytest.approx(0.07)
    assert loaded.gamma == pytest.approx(0.93)
    assert loaded.epsilon == pytest.approx(0.42)
    assert loaded.num_states == 2
    assert loaded.q_values(s1)[0] == pytest.approx(0.123)
    assert loaded.q_values(s2)[1] == pytest.approx(-0.456)
    # NaN mask preserved.
    assert np.isnan(loaded.q_values(s2)[4])  # cell 4 occupied by ME


def test_load_returns_new_independent_instance():
    """Modifying the loaded agent must not affect the saved file or original."""
    agent_a = QLearningAgent(rng=np.random.default_rng(0))
    s = initial_board()
    agent_a.q_values(s)[0] = 1.0
    with tempfile.TemporaryDirectory() as td:
        p = Path(td) / "qtable.pkl"
        agent_a.save(p)
        loaded = QLearningAgent.load(p)
    loaded.q_values(s)[0] = 99.0
    assert agent_a.q_values(s)[0] == 1.0  # original unchanged
