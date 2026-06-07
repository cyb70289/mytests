"""Tests for rl_ttt.game \u2014 board state, legal moves, winner, terminal,
perspective canonicalization.

State convention:
    Board = tuple of 9 ints, row-major (index 0=top-left, 8=bottom-right).
    Each cell is one of:
        EMPTY = 0
        ME    = +1   (side-to-move's pieces; agent always sees self as +1)
        OPP   = -1   (opponent's pieces)

After ME moves, the resulting board is re-encoded with flip_perspective() so
the OTHER side now sees themselves as +1. This is the side-to-move
canonicalization that lets a single Q-table cover both players.
"""

import pytest

from rl_ttt.game import (
    EMPTY, ME, OPP,
    initial_board,
    legal_moves,
    apply_move,
    flip_perspective,
    winner,
    is_terminal,
    render,
)


# --- initial_board -----------------------------------------------------------

def test_initial_board_is_all_empty():
    b = initial_board()
    assert b == (EMPTY,) * 9
    assert len(b) == 9


# --- legal_moves -------------------------------------------------------------

def test_legal_moves_on_empty_board_is_all_nine_cells():
    assert legal_moves(initial_board()) == [0, 1, 2, 3, 4, 5, 6, 7, 8]


def test_legal_moves_excludes_occupied_cells():
    #   0 X 2
    #   3 4 O      X at 1, O at 5
    #   6 7 8
    b = (EMPTY, ME, EMPTY, EMPTY, EMPTY, OPP, EMPTY, EMPTY, EMPTY)
    assert legal_moves(b) == [0, 2, 3, 4, 6, 7, 8]


def test_legal_moves_on_full_board_is_empty():
    b = (ME, OPP, ME, OPP, ME, OPP, OPP, ME, OPP)
    assert legal_moves(b) == []


# --- apply_move --------------------------------------------------------------

def test_apply_move_places_me_marker():
    b = initial_board()
    b2 = apply_move(b, 4)  # center
    assert b2[4] == ME
    # every other cell unchanged
    for i in range(9):
        if i != 4:
            assert b2[i] == EMPTY
    # original board unchanged (immutability)
    assert b == initial_board()


def test_apply_move_to_each_empty_cell():
    for action in range(9):
        b = apply_move(initial_board(), action)
        assert b[action] == ME
        assert sum(1 for c in b if c == ME) == 1


def test_apply_move_rejects_illegal_action_occupied_cell():
    b = apply_move(initial_board(), 0)  # cell 0 now ME
    with pytest.raises(ValueError):
        apply_move(b, 0)


def test_apply_move_rejects_illegal_action_out_of_range():
    with pytest.raises(ValueError):
        apply_move(initial_board(), 9)
    with pytest.raises(ValueError):
        apply_move(initial_board(), -1)


# --- flip_perspective --------------------------------------------------------

def test_flip_perspective_swaps_me_and_opp_keeps_empty():
    b = (ME, OPP, EMPTY, ME, EMPTY, OPP, EMPTY, ME, OPP)
    flipped = flip_perspective(b)
    assert flipped == (OPP, ME, EMPTY, OPP, EMPTY, ME, EMPTY, OPP, ME)


def test_flip_perspective_is_involutive():
    b = (ME, OPP, EMPTY, ME, EMPTY, OPP, EMPTY, ME, OPP)
    assert flip_perspective(flip_perspective(b)) == b


def test_flip_perspective_on_empty_is_empty():
    assert flip_perspective(initial_board()) == initial_board()


# --- winner ------------------------------------------------------------------
# 8 winning lines: 3 rows, 3 cols, 2 diagonals.
WINNING_LINES = [
    (0, 1, 2), (3, 4, 5), (6, 7, 8),   # rows
    (0, 3, 6), (1, 4, 7), (2, 5, 8),   # cols
    (0, 4, 8), (2, 4, 6),              # diagonals
]


@pytest.mark.parametrize("line", WINNING_LINES)
def test_winner_detects_each_of_8_winning_lines_for_me(line):
    cells = [EMPTY] * 9
    for i in line:
        cells[i] = ME
    assert winner(tuple(cells)) == ME


@pytest.mark.parametrize("line", WINNING_LINES)
def test_winner_detects_each_of_8_winning_lines_for_opp(line):
    cells = [EMPTY] * 9
    for i in line:
        cells[i] = OPP
    assert winner(tuple(cells)) == OPP


def test_winner_returns_zero_on_empty_board():
    assert winner(initial_board()) == 0


def test_winner_returns_zero_on_partial_non_winning_board():
    #   X . O
    #   . X .       X at 0, 4; O at 2 \u2014 nobody has 3 in a row yet.
    #   . . .
    b = (ME, EMPTY, OPP, EMPTY, ME, EMPTY, EMPTY, EMPTY, EMPTY)
    assert winner(b) == 0


def test_winner_returns_zero_on_full_drawn_board():
    #   X O X
    #   X O O
    #   O X X       \u2014 known drawn position.
    b = (ME, OPP, ME, ME, OPP, OPP, OPP, ME, ME)
    assert winner(b) == 0


# --- is_terminal -------------------------------------------------------------

def test_is_terminal_false_on_empty_board():
    done, r = is_terminal(initial_board())
    assert done is False
    assert r == 0


def test_is_terminal_true_with_me_winner_returns_plus_one():
    # ME wins top row
    b = (ME, ME, ME, EMPTY, OPP, OPP, EMPTY, EMPTY, EMPTY)
    done, r = is_terminal(b)
    assert done is True
    assert r == +1


def test_is_terminal_true_with_opp_winner_returns_minus_one():
    # OPP wins top row
    b = (OPP, OPP, OPP, EMPTY, ME, ME, EMPTY, EMPTY, EMPTY)
    done, r = is_terminal(b)
    assert done is True
    assert r == -1


def test_is_terminal_true_on_full_board_no_winner_returns_zero():
    # Same drawn position as test_winner_returns_zero_on_full_drawn_board.
    b = (ME, OPP, ME, ME, OPP, OPP, OPP, ME, ME)
    done, r = is_terminal(b)
    assert done is True
    assert r == 0


# --- render ------------------------------------------------------------------

def test_render_produces_three_lines_with_correct_glyphs():
    #   X . O
    #   . X .
    #   O . .
    b = (ME, EMPTY, OPP, EMPTY, ME, EMPTY, OPP, EMPTY, EMPTY)
    out = render(b)
    lines = out.rstrip("\n").split("\n")
    assert len(lines) == 3
    # use 'X' for ME, 'O' for OPP, '.' for EMPTY \u2014 visually unambiguous.
    assert "X" in lines[0] and "O" in lines[0]
    assert "X" in lines[1]
    assert "O" in lines[2]
    # count total glyphs across all lines
    full = "".join(lines)
    assert full.count("X") == 2
    assert full.count("O") == 2
    assert full.count(".") == 5
