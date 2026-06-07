"""Tic-tac-toe game logic: pure functions over an immutable 9-tuple board.

State convention (side-to-move canonical):
    Board = tuple of 9 ints in row-major order:
        index 0 1 2
              3 4 5
              6 7 8
    Each cell is one of:
        EMPTY = 0
        ME    = +1   (side-to-move's pieces; agent always sees self as +1)
        OPP   = -1   (opponent's pieces)

After ME makes a move and the game continues, the caller flips the
perspective with flip_perspective() so the next side-to-move sees their own
pieces as +1. This is the side-to-move canonicalization that lets a single
Q-table cover both players.

Public API:
    initial_board() -> Board
    legal_moves(board) -> list[int]
    apply_move(board, action) -> Board                # places +1 at action
    flip_perspective(board) -> Board                  # negates every cell
    winner(board) -> int                              # +1, -1, or 0
    is_terminal(board) -> (done: bool, reward: int)   # reward from side-to-move's view
    render(board) -> str                              # 3-line ASCII for CLI/debug
"""

from __future__ import annotations

# --- constants ---------------------------------------------------------------

EMPTY: int = 0
ME:    int = +1
OPP:   int = -1

Board = tuple[int, int, int, int, int, int, int, int, int]

# All 8 winning lines (3 rows, 3 columns, 2 diagonals).
_WINNING_LINES: tuple[tuple[int, int, int], ...] = (
    (0, 1, 2), (3, 4, 5), (6, 7, 8),
    (0, 3, 6), (1, 4, 7), (2, 5, 8),
    (0, 4, 8), (2, 4, 6),
)

# Rendering glyphs.
_GLYPH: dict[int, str] = {EMPTY: ".", ME: "X", OPP: "O"}


# --- public API --------------------------------------------------------------

def initial_board() -> Board:
    """Return the empty starting board."""
    return (EMPTY,) * 9


def legal_moves(board: Board) -> list[int]:
    """Return cell indices of all EMPTY cells, in ascending order."""
    return [i for i, v in enumerate(board) if v == EMPTY]


def apply_move(board: Board, action: int) -> Board:
    """Return a NEW board with ME placed at `action`.

    Raises ValueError if `action` is out of range [0, 9) or if the target
    cell is already occupied.
    """
    if not 0 <= action < 9:
        raise ValueError(f"action {action} out of range [0, 9)")
    if board[action] != EMPTY:
        raise ValueError(f"action {action} targets occupied cell (value={board[action]})")
    new_cells = list(board)
    new_cells[action] = ME
    return tuple(new_cells)  # type: ignore[return-value]


def flip_perspective(board: Board) -> Board:
    """Return a NEW board with ME and OPP swapped (cell-wise negation).

    EMPTY cells are unchanged. Applying twice returns the original board.
    """
    return tuple(-c for c in board)  # type: ignore[return-value]


def winner(board: Board) -> int:
    """Return +1 if ME has 3 in a row, -1 if OPP does, else 0.

    Note: tic-tac-toe positions can have AT MOST one winner reachable from
    legal play, so we don't need to consider 'both winning' edge cases.
    """
    for a, b, c in _WINNING_LINES:
        line_sum = board[a] + board[b] + board[c]
        if line_sum == 3 * ME:
            return ME
        if line_sum == 3 * OPP:
            return OPP
    return 0


def is_terminal(board: Board) -> tuple[bool, int]:
    """Return (done, reward) where reward is from the side-to-move's view.

    done = True if the game has ended (someone won OR no legal moves left).
    reward = +1 if ME just won,
             -1 if OPP just won,
              0 if draw or game still in progress.
    """
    w = winner(board)
    if w != 0:
        return True, w
    if not legal_moves(board):
        return True, 0
    return False, 0


def render(board: Board) -> str:
    """Return a 3-line ASCII rendering of the board.

    Uses 'X' for ME, 'O' for OPP, '.' for EMPTY. Cells in a row are
    space-separated for readability.
    """
    rows = []
    for row in range(3):
        cells = [_GLYPH[board[row * 3 + col]] for col in range(3)]
        rows.append(" ".join(cells))
    return "\n".join(rows) + "\n"
