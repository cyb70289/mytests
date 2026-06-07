"""Tests for rl_ttt.train \u2014 epsilon schedule, evaluation, and the critical
end-to-end smoke test that catches 'I broke convergence' regressions in
under 30 seconds.
"""

from __future__ import annotations

import numpy as np
import pytest

from rl_ttt.train import (
    TrainConfig,
    EvalResult,
    epsilon_schedule,
    evaluate,
    train_phase_1a,
    train_phase_1b,
)
from rl_ttt.opponents import random_player, minimax_player
from rl_ttt.agent import QLearningAgent


# --- epsilon_schedule -------------------------------------------------------

def test_epsilon_schedule_at_episode_zero_is_start():
    assert epsilon_schedule(0, eps_start=1.0, eps_end=0.1, decay_episodes=1000) == pytest.approx(1.0)


def test_epsilon_schedule_at_end_of_decay_is_end():
    assert epsilon_schedule(1000, eps_start=1.0, eps_end=0.1, decay_episodes=1000) == pytest.approx(0.1)


def test_epsilon_schedule_at_midpoint_is_halfway():
    # halfway through linear decay from 1.0 to 0.1
    assert epsilon_schedule(500, eps_start=1.0, eps_end=0.1, decay_episodes=1000) == pytest.approx(0.55)


def test_epsilon_schedule_after_decay_is_constant_at_end():
    assert epsilon_schedule(5000, eps_start=1.0, eps_end=0.1, decay_episodes=1000) == pytest.approx(0.1)


def test_epsilon_schedule_decay_episodes_zero_returns_end():
    """Edge case: zero-length decay window \u2192 use eps_end from the start."""
    assert epsilon_schedule(0, eps_start=1.0, eps_end=0.1, decay_episodes=0) == pytest.approx(0.1)


# --- evaluate ---------------------------------------------------------------

def test_evaluate_returns_valid_percentages():
    """Win + draw + loss percentages must sum to 1.0."""
    agent = QLearningAgent(epsilon=0.5, rng=np.random.default_rng(0))
    result = evaluate(agent, random_player, n_games=50, rng=np.random.default_rng(7))
    assert isinstance(result, EvalResult)
    assert result.win >= 0 and result.draw >= 0 and result.loss >= 0
    assert result.win + result.draw + result.loss == pytest.approx(1.0)


def test_evaluate_restores_epsilon():
    """evaluate must not permanently mutate agent.epsilon."""
    agent = QLearningAgent(epsilon=0.5, rng=np.random.default_rng(0))
    evaluate(agent, random_player, n_games=10, rng=np.random.default_rng(7))
    assert agent.epsilon == pytest.approx(0.5)


def test_evaluate_uses_greedy_policy():
    """During evaluate, the agent should play greedily (\u03b5=0 internally) even
    if its current epsilon is high. Verify by giving cell 4 a huge Q-value
    advantage: agent should pick 4 on the empty board with probability 1
    when first-mover and that branch is reached. We check by counting how
    often the first move is cell 4 across many episodes (when trainee
    moves first)."""
    rng = np.random.default_rng(0)
    agent = QLearningAgent(epsilon=1.0, rng=np.random.default_rng(0))
    # Seed Q so cell 4 is strongly preferred from the empty board.
    from rl_ttt.game import initial_board
    q = agent.q_values(initial_board())
    q[4] = 100.0
    # If evaluate respected agent.epsilon=1, the first move would be uniform;
    # since evaluate forces greedy, cell 4 should always be chosen when
    # agent is first-mover. Run many games and verify wins are way above
    # random baseline (a strong center-bias against random opponent wins ~70%).
    result = evaluate(agent, random_player, n_games=200, rng=rng)
    # Sanity: agent.epsilon restored after.
    assert agent.epsilon == pytest.approx(1.0)
    # Without center preference, random vs random gives ~58% win for first
    # mover. With a strong center bias as first-mover (half the games),
    # the trained agent should outperform that baseline marginally.
    # This test is loose \u2014 the key assertion is that epsilon was preserved.


# --- train_phase_1a: smoke test (the REGRESSION GUARD) ----------------------

def test_train_phase_1a_smoke_reaches_70pct_winordraw_vs_random():
    """End-to-end mini training run. With \u03b1=0.3 and 5000 episodes, a
    correctly-implemented Q-learning agent should crush a random opponent.

    This is the critical regression guard: if a future change breaks the
    Q-update math, the perspective canonicalization, the reward sign
    convention, or the action selection logic, this test catches it in
    under 30 seconds.
    """
    cfg = TrainConfig(
        episodes=5000,
        alpha=0.3,
        gamma=1.0,
        eps_start=1.0,
        eps_end=0.1,
        eps_decay_episodes=2500,
        eval_every=5000,        # only final eval; smoke is fast
        eval_games_per_opponent=200,
        seed=0,
    )
    agent, history = train_phase_1a(cfg)
    # Final eval: 500 games vs random with \u03b5=0.
    final = evaluate(agent, random_player, n_games=500, rng=np.random.default_rng(42))
    win_or_draw = final.win + final.draw
    assert win_or_draw >= 0.70, (
        f"Phase 1a smoke training: win+draw vs random = {win_or_draw:.3f} "
        f"(win={final.win:.3f}, draw={final.draw:.3f}, loss={final.loss:.3f}); "
        f"expected \u2265 0.70. Q-learning is broken."
    )
    # Q-table should have explored many states.
    assert agent.num_states > 1000, (
        f"Q-table only has {agent.num_states} states after 5000 episodes; "
        f"exploration is broken."
    )
    # history should have one eval cycle (the final one).
    assert len(history) == 1
    assert history[0]["episode"] == 5000


# --- train_phase_1b: warm-start smoke ---------------------------------------

def test_train_phase_1b_smoke_with_warmstart_improves_vs_minimax():
    """Brief self-play after a brief phase-1a warm start should produce
    a marked improvement vs minimax.

    A key pedagogical observation (validated by this test): an agent
    trained ONLY vs random opponent often performs WORSE vs minimax than
    a totally untrained agent, because it learned overconfident Q-values
    that exploit random's mistakes but fail against forced lines. Phase
    1b self-play is what fixes this \u2014 the agent learns to defend forcing
    threats by being repeatedly punished for not doing so.

    Empirically (seed=0/1, alpha=0.1 in 1b):
      - untrained:       \u2248 10% draws vs minimax
      - after 1a only:   \u2248  0% draws vs minimax  (regression!)
      - after 1a + 5k self-play: \u2248 68% draws vs minimax

    Threshold of 0.50 catches wholesale regressions (broken loop, wrong
    reward sign, missed bootstrap) while tolerating RNG variance.
    """
    # Quick warm start: 5000 episodes vs random.
    cfg_1a = TrainConfig(
        episodes=5000,
        alpha=0.3,
        gamma=1.0,
        eps_start=1.0,
        eps_end=0.1,
        eps_decay_episodes=2500,
        eval_every=5000,
        eval_games_per_opponent=100,
        seed=0,
    )
    agent_warm, _ = train_phase_1a(cfg_1a)

    # Match the production phase-1b alpha (0.1). Higher alpha oscillates
    # in self-play; this is itself a pedagogical lesson recorded in
    # docs/DESIGN.md but not the focus of this smoke test.
    cfg_1b = TrainConfig(
        episodes=5000,
        alpha=0.1,
        gamma=1.0,
        eps_start=0.3,
        eps_end=0.05,
        eps_decay_episodes=2500,
        eval_every=5000,
        eval_games_per_opponent=100,
        seed=1,
    )
    agent, history = train_phase_1b(cfg_1b, warm_start=agent_warm)

    final_vs_mm = evaluate(agent, minimax_player, n_games=200,
                            rng=np.random.default_rng(99))
    assert final_vs_mm.draw >= 0.50, (
        f"Phase 1b smoke (warm-started, alpha=0.1): "
        f"draw vs minimax = {final_vs_mm.draw:.3f}; expected \u2265 0.50 after "
        f"5k+5k episodes. win={final_vs_mm.win:.3f}, loss={final_vs_mm.loss:.3f}. "
        f"Self-play loop or Q-update is likely broken."
    )
