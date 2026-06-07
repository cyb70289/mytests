"""Training and evaluation loops for the tabular Q-learning tic-tac-toe agent.

Two training entry points:

  train_phase_1a(cfg)           \u2014 trainee vs random opponent (stationary env)
  train_phase_1b(cfg, warm_start) \u2014 self-play with shared Q-table

Both delegate to a single _run_episode() generator that records the
trainee's experience tuples (s, a, r, s_next) and updates the Q-table.
For each episode, the trainee is randomly assigned to be first- or
second-mover (coin flip per docs/DESIGN.md).

Evaluation is shared: 200 games per opponent (random and minimax) every
1000 training episodes, with \u03b5 forced to 0 (greedy).

Reward sign convention: rewards are always from the TRAINEE's perspective.
A win for the opponent on their reply produces reward -1 for the trainee.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Callable

import numpy as np

from rl_ttt.agent import QLearningAgent
from rl_ttt.game import (
    Board,
    apply_move,
    flip_perspective,
    initial_board,
    is_terminal,
)
from rl_ttt.opponents import minimax_player, random_player


# --- public types -----------------------------------------------------------

Player = Callable[[Board, np.random.Generator | None], int]


@dataclass
class EvalResult:
    """Outcome distribution over `n_games`. Percentages sum to 1.0."""
    win: float    # fraction of games trainee won
    draw: float
    loss: float


@dataclass
class TrainConfig:
    """Training hyperparameters. Mirrors docs/DESIGN.md."""
    episodes: int
    alpha: float
    gamma: float
    eps_start: float
    eps_end: float
    eps_decay_episodes: int
    eval_every: int
    eval_games_per_opponent: int
    seed: int


# --- pure helpers -----------------------------------------------------------

def epsilon_schedule(
    episode: int,
    eps_start: float,
    eps_end: float,
    decay_episodes: int,
) -> float:
    """Linear decay from `eps_start` at episode 0 to `eps_end` at episode
    `decay_episodes`, then constant `eps_end` thereafter.

    Edge case: decay_episodes == 0 \u2192 always return eps_end.
    """
    if decay_episodes <= 0 or episode >= decay_episodes:
        return eps_end
    frac = episode / decay_episodes
    return eps_start + (eps_end - eps_start) * frac


# --- core episode loop ------------------------------------------------------

def _run_episode(
    agent: QLearningAgent,
    opponent: Player,
    rng: np.random.Generator,
    train: bool,
) -> int:
    """Run one full game. Returns outcome from trainee's view: +1 win,
    0 draw, -1 loss.

    If `train` is True, the trainee's Q-table is updated after each of its
    transitions becomes complete. The opponent's actions are never recorded
    (Design A in docs/PLAN.md: clean abstraction, opponent looks like part
    of the environment).

    First-mover is a fair coin flip per episode.
    """
    trainee_to_move = bool(rng.integers(2))  # True \u2192 trainee is first
    board: Board = initial_board()

    # Buffer the trainee's most recent (s, a) until s_next is observed.
    last_s: Board | None = None
    last_a: int | None = None

    while True:
        if trainee_to_move:
            a = agent.select_action(board)
            new_board = apply_move(board, a)
            done, reward = is_terminal(new_board)
            if done:
                # Trainee's move ended the game (win=+1 or draw-by-filling=0).
                if train:
                    agent.update(board, a, float(reward), None)
                return int(reward)
            # Continue: defer the Q-update until we see the trainee's next state.
            last_s, last_a = board, a
            # Flip perspective so the opponent sees their own pieces as +1.
            board = flip_perspective(new_board)
            trainee_to_move = False
        else:
            a_opp = opponent(board, rng)
            new_board = apply_move(board, a_opp)
            done, reward_for_opp = is_terminal(new_board)
            if done:
                # Opponent's move ended the game; flip sign for trainee's view.
                trainee_reward = -int(reward_for_opp)  # +1 \u2192 -1 (loss); 0 \u2192 0
                if train and last_s is not None:
                    agent.update(last_s, last_a, float(trainee_reward), None)
                return trainee_reward
            # Continue: trainee's next state is the post-opponent board flipped
            # back to trainee's perspective.
            trainee_next_board = flip_perspective(new_board)
            if train and last_s is not None:
                # Non-terminal: bootstrap on trainee's next state.
                agent.update(last_s, last_a, 0.0, trainee_next_board)
            board = trainee_next_board
            trainee_to_move = True


# --- evaluation -------------------------------------------------------------

def evaluate(
    agent: QLearningAgent,
    opponent: Player,
    n_games: int,
    rng: np.random.Generator,
) -> EvalResult:
    """Play `n_games` with the trainee greedy (\u03b5=0) and record outcomes.

    The agent's `epsilon` attribute is saved and restored, so this is safe
    to call during training without disturbing the exploration schedule.
    """
    original_eps = agent.epsilon
    agent.epsilon = 0.0
    try:
        win = draw = loss = 0
        for _ in range(n_games):
            r = _run_episode(agent, opponent, rng, train=False)
            if r > 0:
                win += 1
            elif r < 0:
                loss += 1
            else:
                draw += 1
    finally:
        agent.epsilon = original_eps
    n = float(n_games)
    return EvalResult(win=win / n, draw=draw / n, loss=loss / n)


def _eval_cycle(
    agent: QLearningAgent,
    episode: int,
    n_games: int,
    rng: np.random.Generator,
) -> dict:
    """Run one eval cycle: snapshot metrics vs random AND vs minimax.

    Returns a flat dict suitable for CSV serialization.
    """
    vs_random = evaluate(agent, random_player, n_games, rng)
    vs_minimax = evaluate(agent, minimax_player, n_games, rng)
    return {
        "episode": episode,
        "epsilon": agent.epsilon,
        "qtable_size": agent.num_states,
        "win_vs_random":   vs_random.win,
        "draw_vs_random":  vs_random.draw,
        "loss_vs_random":  vs_random.loss,
        "win_vs_minimax":  vs_minimax.win,
        "draw_vs_minimax": vs_minimax.draw,
        "loss_vs_minimax": vs_minimax.loss,
    }


# --- training loops ---------------------------------------------------------

def train_phase_1a(
    cfg: TrainConfig,
    progress: bool = False,
) -> tuple[QLearningAgent, list[dict]]:
    """Train the agent against a random opponent (stationary environment).

    Returns (agent, history) where history is a list of per-eval-cycle dicts.
    """
    rng = np.random.default_rng(cfg.seed)
    eval_rng = np.random.default_rng(cfg.seed + 10_000)
    agent = QLearningAgent(
        alpha=cfg.alpha,
        gamma=cfg.gamma,
        epsilon=cfg.eps_start,
        rng=rng,
    )

    history = _training_loop(
        agent=agent,
        opponent=random_player,
        cfg=cfg,
        rng=rng,
        eval_rng=eval_rng,
        progress=progress,
        progress_desc="phase 1a (vs random)",
    )
    return agent, history


def train_phase_1b(
    cfg: TrainConfig,
    warm_start: QLearningAgent | None = None,
    progress: bool = False,
) -> tuple[QLearningAgent, list[dict]]:
    """Self-play training with a shared Q-table. Optionally warm-started.

    Returns (agent, history).
    """
    rng = np.random.default_rng(cfg.seed)
    eval_rng = np.random.default_rng(cfg.seed + 10_000)
    if warm_start is not None:
        agent = warm_start
        agent.alpha = cfg.alpha
        agent.gamma = cfg.gamma
        agent.epsilon = cfg.eps_start
        agent.rng = rng
    else:
        agent = QLearningAgent(
            alpha=cfg.alpha,
            gamma=cfg.gamma,
            epsilon=cfg.eps_start,
            rng=rng,
        )

    # Self-play opponent: the SAME agent picking actions on a flipped board.
    # Because the Q-table is shared and state is side-to-move canonical, this
    # naturally implements symmetric \u03b5-greedy self-play.
    def self_play_opponent(board: Board, _rng: np.random.Generator | None) -> int:
        return agent.select_action(board)

    history = _training_loop(
        agent=agent,
        opponent=self_play_opponent,
        cfg=cfg,
        rng=rng,
        eval_rng=eval_rng,
        progress=progress,
        progress_desc="phase 1b (self-play)",
    )
    return agent, history


def _training_loop(
    agent: QLearningAgent,
    opponent: Player,
    cfg: TrainConfig,
    rng: np.random.Generator,
    eval_rng: np.random.Generator,
    progress: bool,
    progress_desc: str,
) -> list[dict]:
    """Shared training loop: episodes + epsilon decay + periodic eval."""
    history: list[dict] = []

    iterator = range(cfg.episodes)
    if progress:
        try:
            from tqdm import tqdm
            iterator = tqdm(iterator, desc=progress_desc, unit="ep")
        except ImportError:
            pass

    for ep in iterator:
        agent.epsilon = epsilon_schedule(
            ep, cfg.eps_start, cfg.eps_end, cfg.eps_decay_episodes
        )
        _run_episode(agent, opponent, rng, train=True)

        if (ep + 1) % cfg.eval_every == 0:
            history.append(
                _eval_cycle(agent, ep + 1, cfg.eval_games_per_opponent, eval_rng)
            )

    # Ensure a final eval cycle exists (in case episodes % eval_every != 0).
    if not history or history[-1]["episode"] != cfg.episodes:
        history.append(
            _eval_cycle(agent, cfg.episodes, cfg.eval_games_per_opponent, eval_rng)
        )

    return history
