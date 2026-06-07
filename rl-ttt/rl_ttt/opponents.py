"""Fixed opponents used during training (random) and evaluation
(both random and minimax).

A player is a callable with signature::

    player(board: Board, rng: np.random.Generator | None) -> int

returning an action in 0..8. The board is from the player's own
side-to-move perspective (their pieces are +1). The `rng` parameter is
accepted by all players for uniformity even when not used (minimax is
deterministic).

Performance: minimax uses memoization (lru_cache) keyed on the board tuple,
so once a sub-tree has been searched it is cached for all subsequent
calls in the process. Full search is < 1 ms after warmup.
"""

from __future__ import annotations

from functools import lru_cache

import numpy as np

from rl_ttt.game import (
    Board,
    apply_move,
    flip_perspective,
    is_terminal,
    legal_moves,
)


# --- random_player -----------------------------------------------------------

def random_player(board: Board, rng: np.random.Generator | None) -> int:
    """Pick a uniformly random LEGAL action.

    `rng` is required (raises ValueError if None) to keep behavior fully
    reproducible \u2014 there is no implicit global RNG fallback.
    """
    if rng is None:
        raise ValueError("random_player requires an explicit np.random.Generator")
    legal = legal_moves(board)
    if not legal:
        raise ValueError("random_player called on terminal board with no legal moves")
    idx = int(rng.integers(len(legal)))
    return legal[idx]


# --- minimax_player ----------------------------------------------------------

def minimax_player(board: Board, rng: np.random.Generator | None = None) -> int:
    """Return the optimal action for side-to-move.

    Uses negamax search with memoization on the canonical board tuple.
    Ties are broken by lowest action index (deterministic). The `rng`
    parameter is accepted for player-interface uniformity but ignored.
    """
    legal = legal_moves(board)
    if not legal:
        raise ValueError("minimax_player called on terminal board with no legal moves")
    _, best_action = _negamax(board)
    if best_action is None:
        # Shouldn't happen on a non-terminal board, but guard anyway.
        raise RuntimeError(f"minimax returned no action for non-terminal board {board}")
    return best_action


@lru_cache(maxsize=None)
def _negamax(board: Board) -> tuple[int, int | None]:
    """Return (value, best_action) for side-to-move on `board`.

    value is in {-1, 0, +1}: expected terminal reward under perfect play
    from side-to-move's perspective.

    Memoized on the canonical board tuple: the value of a state from
    side-to-move's view depends only on the state, not on the path that
    led there, so caching is sound.
    """
    done, reward = is_terminal(board)
    if done:
        return reward, None
    best_value = -2  # strictly less than any reachable value (-1, 0, +1)
    best_action: int | None = None
    for a in legal_moves(board):  # ascending order \u2192 deterministic tie-break
        # After our move, flip perspective so the recursive call sees
        # the opponent's pieces as +1 (and ours as -1).
        child = flip_perspective(apply_move(board, a))
        child_value, _ = _negamax(child)
        # negamax: child_value is from opponent's view; negate for ours.
        value = -child_value
        if value > best_value:
            best_value = value
            best_action = a
    return best_value, best_action
