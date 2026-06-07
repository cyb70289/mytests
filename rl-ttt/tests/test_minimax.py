"""Tests for rl_ttt.opponents \u2014 random_player and minimax_player.

The two opponents the agent faces during training (random) and evaluation
(both random and minimax). The minimax player is the gold-standard reference
for 'optimal play' \u2014 if the trained agent draws 100% of games against it,
optimal play has been reached.
"""

from collections import Counter

import numpy as np
import pytest

from rl_ttt.game import (
    ME, OPP, EMPTY,
    initial_board, legal_moves, apply_move, flip_perspective, is_terminal,
)
from rl_ttt.opponents import random_player, minimax_player


# --- helpers -----------------------------------------------------------------

def _play_game(
    first_player,
    second_player,
    rng: np.random.Generator | None = None,
) -> int:
    """Play one game; return outcome from FIRST player's view.

    Returns:
        +1 if first player won
        -1 if first player lost
         0 if drawn

    Each player is a callable that takes (board, rng) where board is from
    the player's own side-to-move perspective (their pieces are +1). The
    callable returns an action in 0..8.

    After each move, we flip perspective so the next caller sees their own
    pieces as +1.
    """
    board = initial_board()
    current_is_first = True
    while True:
        player = first_player if current_is_first else second_player
        action = player(board, rng)
        board = apply_move(board, action)
        done, reward_for_mover = is_terminal(board)
        if done:
            # reward_for_mover is +1 if the player who just moved won, 0 for draw.
            # Translate to FIRST player's perspective:
            if reward_for_mover == 0:
                return 0
            return +1 if current_is_first else -1
        # game continues \u2014 flip for next mover
        board = flip_perspective(board)
        current_is_first = not current_is_first


# --- random_player -----------------------------------------------------------

def test_random_player_returns_only_legal_actions():
    rng = np.random.default_rng(0)
    # Generate a variety of boards: empty, a few moves played, near-full.
    test_boards = [
        initial_board(),
        (ME, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY),
        (ME, OPP, EMPTY, OPP, ME, EMPTY, EMPTY, EMPTY, EMPTY),
        (ME, OPP, ME, OPP, ME, OPP, EMPTY, EMPTY, EMPTY),
    ]
    for b in test_boards:
        legal = set(legal_moves(b))
        for _ in range(100):
            a = random_player(b, rng)
            assert a in legal, f"random_player returned {a} not in legal set {legal} for board {b}"


def test_random_player_with_fixed_seed_is_deterministic():
    rng1 = np.random.default_rng(42)
    rng2 = np.random.default_rng(42)
    b = initial_board()
    seq1 = [random_player(b, rng1) for _ in range(20)]
    seq2 = [random_player(b, rng2) for _ in range(20)]
    assert seq1 == seq2


def test_random_player_covers_all_legal_actions_in_long_run():
    rng = np.random.default_rng(0)
    b = initial_board()  # 9 legal actions
    seen = Counter(random_player(b, rng) for _ in range(5000))
    # All 9 cells should appear at least 200 times in 5000 trials.
    for action in range(9):
        assert seen[action] > 200, f"action {action} appeared only {seen[action]} times"


# --- minimax_player ----------------------------------------------------------

def test_minimax_finds_immediate_winning_move_top_row():
    #   X X .         ME plays cell 2 to win the top row.
    #   . O .
    #   O . .
    b = (ME, ME, EMPTY, EMPTY, OPP, EMPTY, OPP, EMPTY, EMPTY)
    assert minimax_player(b) == 2


def test_minimax_finds_immediate_winning_move_diagonal():
    #   X . O         ME plays cell 8 to complete diagonal 0-4-8.
    #   . X O
    #   . . .
    b = (ME, EMPTY, OPP, EMPTY, ME, OPP, EMPTY, EMPTY, EMPTY)
    assert minimax_player(b) == 8


def test_minimax_blocks_immediate_opponent_win():
    #   O O .         OPP threatens top-row win at cell 2; ME must block at 2.
    #   . X .
    #   . . .
    b = (OPP, OPP, EMPTY, EMPTY, ME, EMPTY, EMPTY, EMPTY, EMPTY)
    assert minimax_player(b) == 2


def test_minimax_blocks_diagonal_threat():
    #   O . X         OPP threatens to play cell 8 next to win 0-4-8.
    #   . O .         ME must block at 8.
    #   . . .
    b = (OPP, EMPTY, ME, EMPTY, OPP, EMPTY, EMPTY, EMPTY, EMPTY)
    assert minimax_player(b) == 8


def test_minimax_vs_minimax_always_draws():
    """Tic-tac-toe with optimal play on both sides is a draw."""
    n_games = 20
    outcomes = Counter()
    for _ in range(n_games):
        outcome = _play_game(minimax_player, minimax_player, rng=None)
        outcomes[outcome] += 1
    assert outcomes[+1] == 0, f"unexpected wins for first minimax: {outcomes}"
    assert outcomes[-1] == 0, f"unexpected wins for second minimax: {outcomes}"
    assert outcomes[0] == n_games


def test_minimax_vs_random_never_loses_as_first_mover():
    """The minimax-optimal player can NEVER lose. As first mover vs random,
    it must win or draw every game."""
    rng = np.random.default_rng(0)
    n_games = 200
    losses = 0
    for _ in range(n_games):
        outcome = _play_game(minimax_player, random_player, rng=rng)
        if outcome == -1:
            losses += 1
    assert losses == 0, f"minimax lost {losses}/{n_games} games as first mover"


def test_minimax_vs_random_never_loses_as_second_mover():
    """As second mover vs random, minimax must still never lose."""
    rng = np.random.default_rng(1)
    n_games = 200
    losses = 0
    for _ in range(n_games):
        # In _play_game, first_player=random, second_player=minimax.
        # outcome is from first (random)'s view. random wins (=+1) means
        # minimax lost.
        outcome = _play_game(random_player, minimax_player, rng=rng)
        if outcome == +1:
            losses += 1
    assert losses == 0, f"minimax lost {losses}/{n_games} games as second mover"


def test_minimax_first_move_chooses_center_or_corner():
    """All optimal first moves on an empty board are center or any corner.
    With deterministic tie-break (lowest index), minimax should pick 0 (corner)
    or 4 (center)."""
    a = minimax_player(initial_board())
    assert a in {0, 4}, f"first move {a} is not center or top-left corner"
