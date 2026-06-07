"""Tabular Q-learning agent for tic-tac-toe.

Q is a dict keyed by canonical board states; each entry is a 9-element
numpy array of Q-values (one per cell), with NaN at occupied (illegal)
cells. Lazy initialization: states are added to the dict only when
encountered, with empty cells initialized to 0.0.

\u03b5-greedy action selection with uniform-random tie-breaking. Standard
Q-learning update rule with no-bootstrap on terminal transitions.
"""

from __future__ import annotations

import pickle
from pathlib import Path

import numpy as np

from rl_ttt.game import Board, EMPTY, legal_moves


class QLearningAgent:
    """Tabular Q-learning agent.

    Hyperparameters:
        alpha   \u2014 learning rate (Q-update step size). Mutable; can be changed
                  between episodes (e.g., for decay schedules, though not
                  used by default).
        gamma   \u2014 discount factor. For tic-tac-toe we use 1.0 (undiscounted).
        epsilon \u2014 exploration probability for \u03b5-greedy. Mutable; the
                  training loop decays this externally.
        rng     \u2014 np.random.Generator used for exploration and tie-breaking.
    """

    def __init__(
        self,
        alpha: float = 0.1,
        gamma: float = 1.0,
        epsilon: float = 0.1,
        rng: np.random.Generator | None = None,
    ) -> None:
        self.alpha = float(alpha)
        self.gamma = float(gamma)
        self.epsilon = float(epsilon)
        self.rng = rng if rng is not None else np.random.default_rng()
        self._q_table: dict[Board, np.ndarray] = {}

    # --- Q-table access -----------------------------------------------------

    def q_values(self, board: Board) -> np.ndarray:
        """Return the 9-element Q-value array for `board`.

        If `board` has not been seen before, lazily initialize it: empty
        cells get 0.0, occupied cells get NaN (illegal). The returned array
        is the SAME object stored in the table \u2014 callers can mutate it in
        place (this is how `update` writes back Q-values).
        """
        q = self._q_table.get(board)
        if q is None:
            q = np.full(9, np.nan, dtype=np.float64)
            for i, cell in enumerate(board):
                if cell == EMPTY:
                    q[i] = 0.0
            self._q_table[board] = q
        return q

    @property
    def num_states(self) -> int:
        """Number of distinct states currently in the Q-table."""
        return len(self._q_table)

    # --- action selection ---------------------------------------------------

    def select_action(self, board: Board) -> int:
        """\u03b5-greedy action selection over LEGAL actions.

        With probability `epsilon`, pick a uniformly random legal action.
        Otherwise, pick argmax of Q[board][legal_actions], breaking ties
        uniformly at random.
        """
        legal = legal_moves(board)
        if not legal:
            raise ValueError("select_action called on terminal board")

        if self.rng.random() < self.epsilon:
            # Explore: uniform random over legal actions.
            return legal[int(self.rng.integers(len(legal)))]

        # Exploit: greedy with random tie-break.
        q = self.q_values(board)
        # nanmax safely ignores NaN entries (occupied cells are NaN).
        max_q = np.nanmax(q)
        # `q == max_q` is False for NaN cells, so they're naturally excluded.
        candidates = np.flatnonzero(q == max_q)
        return int(self.rng.choice(candidates))

    # --- Q-update -----------------------------------------------------------

    def update(
        self,
        s: Board,
        a: int,
        reward: float,
        s_next: Board | None,
    ) -> None:
        """Standard tabular Q-learning update.

        Terminal transition (s_next is None):
            Q[s][a] += alpha * (reward - Q[s][a])

        Non-terminal transition:
            Q[s][a] += alpha * (reward + gamma * max(Q[s_next]) - Q[s][a])

        The max over Q[s_next] uses nanmax so illegal cells (NaN) are
        excluded.
        """
        q_s = self.q_values(s)
        if s_next is None:
            target = reward
        else:
            q_next = self.q_values(s_next)
            target = reward + self.gamma * float(np.nanmax(q_next))
        q_s[a] += self.alpha * (target - q_s[a])

    # --- persistence --------------------------------------------------------

    def save(self, path: str | Path) -> None:
        """Pickle the Q-table and hyperparameters to `path`."""
        path = Path(path)
        path.parent.mkdir(parents=True, exist_ok=True)
        payload = {
            "q_table": self._q_table,
            "alpha": self.alpha,
            "gamma": self.gamma,
            "epsilon": self.epsilon,
        }
        with open(path, "wb") as f:
            pickle.dump(payload, f)

    @classmethod
    def load(
        cls,
        path: str | Path,
        rng: np.random.Generator | None = None,
    ) -> "QLearningAgent":
        """Load a previously-saved agent. Returns a NEW instance independent
        of the file (further mutations don't write back automatically)."""
        with open(path, "rb") as f:
            payload = pickle.load(f)
        agent = cls(
            alpha=payload["alpha"],
            gamma=payload["gamma"],
            epsilon=payload["epsilon"],
            rng=rng,
        )
        agent._q_table = payload["q_table"]
        return agent
