# Implementation Plan

> Executed inline in the same session. Each stage ends with verification + a
> git commit covering both code and docs. Tasks are TDD: write the test, watch
> it fail, write minimal code, watch it pass, refactor, commit.

**Goal:** Build a tabular Q-learning agent that learns to play tic-tac-toe
optimally, with every component transparent and inspectable.

**Architecture:** Pure-Python package (no PyTorch). State = 9-tuple side-to-move
canonicalized. Q-table = `dict[tuple, np.ndarray(9)]`. Two training phases:
50k vs random, then 100k self-play warm-started from phase 1a.

**Tech Stack:** Python 3.10+, numpy, matplotlib, tqdm, pytest. Local `.venv`.

---

## Stage 0 \u2014 Project skeleton

Create directory structure, `requirements.txt`, `.gitignore`, `README.md`,
`docs/DESIGN.md`, `docs/PLAN.md`, package and tests `__init__.py` files.
Initialize git, create `.venv`, install dependencies. Commit.

**Files touched:** see project layout in `README.md`.

**Verify:** `python -c "import numpy, matplotlib, pytest, tqdm"` succeeds in
the venv; `pytest --collect-only tests/` returns 0 tests collected (skeleton
is wired up). Commit: `chore: scaffold project skeleton and design docs`.

---

## Stage 1 \u2014 Game logic (`rl_ttt/game.py`)

The environment. Pure functions over an immutable 9-tuple board state.

**Public API** (this is the contract the rest of the code depends on):

```python
EMPTY: int = 0
ME:    int = +1    # side-to-move's pieces
OPP:   int = -1    # opponent's pieces

Board = tuple[int, int, int, int, int, int, int, int, int]  # length 9

def initial_board() -> Board: ...
def legal_moves(board: Board) -> list[int]: ...      # indices of EMPTY cells
def apply_move(board: Board, action: int) -> Board:  # places +1 at action
    ...
def flip_perspective(board: Board) -> Board:         # negate every cell
    ...
def winner(board: Board) -> int:
    # Returns +1 if ME just won (three +1 in a row),
    #         -1 if OPP just won (three -1 in a row),
    #          0 if no winner.
    ...
def is_terminal(board: Board) -> tuple[bool, int]:
    # (done, reward_for_side_to_move_at_THIS_state)
    # If a winner exists \u2192 (True, winner sign).
    # If no winner and no legal moves \u2192 (True, 0) [draw].
    # Otherwise \u2192 (False, 0).
    ...
def render(board: Board) -> str:
    # Returns a 3-line ASCII rendering of the board (for CLI/play and debug).
    ...
```

**Key invariant:** `apply_move(b, a)` places `+1` at `a` (the agent always
sees itself as `+1`). After the move, the caller is expected to either:

- check for terminal / win (`winner(b_new)` should report `+1` if the
  move just won), then
- if game continues, call `flip_perspective(b_new)` so that the next
  side-to-move sees their own pieces as `+1`.

**Test cases (each its own `def test_...`):**

- `test_initial_board_is_all_empty`
- `test_legal_moves_on_empty_board_is_all_nine_cells`
- `test_legal_moves_excludes_occupied_cells`
- `test_apply_move_places_me_marker`
- `test_apply_move_rejects_illegal_action` (raises `ValueError`)
- `test_flip_perspective_swaps_me_and_opp_keeps_empty`
- `test_flip_perspective_is_involutive` (applying twice returns original)
- `test_winner_detects_each_of_8_winning_lines_for_me` (parametrized over 8 lines)
- `test_winner_detects_each_of_8_winning_lines_for_opp` (parametrized)
- `test_winner_returns_zero_on_empty_board`
- `test_winner_returns_zero_on_partial_non_winning_board`
- `test_is_terminal_false_on_empty_board`
- `test_is_terminal_true_with_winner_returns_correct_sign`
- `test_is_terminal_true_on_full_board_no_winner_returns_zero` (draw)
- `test_render_produces_three_lines_with_correct_glyphs`

**TDD cycle:** for each test \u2014 write test, `pytest tests/test_game.py::<name> -v`
must FAIL, implement minimal code, run again must PASS, then move on.

**Verify (end of stage):** `pytest tests/test_game.py -v` \u2192 all green.

**Commit:** `feat(game): board representation, legal moves, winner detection,
perspective flip`. Include `tests/test_game.py` and `rl_ttt/game.py`.

---

## Stage 2 \u2014 Opponents (`rl_ttt/opponents.py`)

Two opponents the agent will face during training and evaluation:

```python
def random_player(board: Board, rng: np.random.Generator) -> int:
    # Uniform random over legal_moves(board).
    ...

def minimax_player(board: Board) -> int:
    # Returns the optimal action for the side-to-move (always +1 from the
    # current perspective). Uses negamax with alpha-beta pruning and
    # memoization on the canonical board tuple. Ties broken by lowest
    # action index for determinism (this opponent is a fixed reference).
    ...
```

**Implementation notes:**

- Use `functools.cache` (lru_cache) keyed on the board tuple for memoization.
- Negamax: `value = max over legal a of -negamax(flip(apply(board, a)))`.
- Base case: `is_terminal(board)` \u2192 return `reward` from side-to-move's view.
- Return both `value` and `action`; the public `minimax_player` returns action.

**Test cases:**

- `test_random_player_returns_only_legal_actions` (1000 boards \u00d7 1 call each)
- `test_random_player_with_fixed_seed_is_deterministic`
- `test_minimax_finds_immediate_winning_move` (hand-set boards: 2-in-a-row + empty third)
- `test_minimax_blocks_immediate_opponent_win` (hand-set boards)
- `test_minimax_vs_minimax_always_draws` (play 50 games; assert 0 wins, 0 losses, 50 draws)
- `test_minimax_vs_random_never_loses_as_first_mover` (play 200 games seeded; assert 0 losses)
- `test_minimax_vs_random_never_loses_as_second_mover` (play 200 games seeded; assert 0 losses)

**Verify:** `pytest tests/test_minimax.py -v` \u2192 all green.

**Commit:** `feat(opponents): random and minimax-optimal players with full test
coverage`.

---

## Stage 3 \u2014 Q-learning agent (`rl_ttt/agent.py`)

```python
class QLearningAgent:
    def __init__(
        self,
        alpha: float = 0.1,
        gamma: float = 1.0,
        epsilon: float = 0.1,
        rng: np.random.Generator | None = None,
    ): ...

    def q_values(self, board: Board) -> np.ndarray:
        # Lazy-init: if board not seen, create np.full(9, 0.0) with NaN at
        # cells where board[i] != EMPTY. Returns the (possibly newly-created)
        # ndarray (alias, not copy, so callers can update in-place via Q-update).
        ...

    def select_action(self, board: Board) -> int:
        # \u03b5-greedy: with prob epsilon, uniform random from legal_moves(board);
        # otherwise argmax over legal Q-values with uniform random tie-break.
        ...

    def update(
        self,
        s: Board,
        a: int,
        r: float,
        s_next: Board | None,   # None on terminal
    ) -> None:
        # s_next is None \u2192 terminal: Q[s][a] += \u03b1 (r - Q[s][a])
        # otherwise non-terminal: Q[s][a] += \u03b1 (r + \u03b3 nanmax(Q[s_next]) - Q[s][a])
        ...

    def save(self, path: str | Path) -> None: ...

    @classmethod
    def load(cls, path: str | Path, **kwargs) -> "QLearningAgent": ...

    @property
    def num_states(self) -> int: ...
```

**Test cases:**

- `test_q_values_lazy_init_zeros_with_nan_at_illegal`
- `test_q_values_same_state_returns_same_array` (identity, not copy)
- `test_select_action_with_epsilon_zero_returns_argmax`
- `test_select_action_with_epsilon_zero_breaks_ties_randomly`
  (e.g., on the empty board with Q all zero, repeated calls produce a
  roughly uniform distribution over all 9 actions; check that all 9
  appear in 1000 calls)
- `test_select_action_with_epsilon_one_is_uniform_over_legal`
- `test_select_action_never_picks_illegal_action`
- `test_update_terminal_uses_no_bootstrap` (hand-computed: \u03b1=0.5, Q=0, r=1
  \u2192 Q'[s][a] == 0.5)
- `test_update_non_terminal_includes_bootstrap` (hand-computed: \u03b1=0.5,
  \u03b3=1, Q[s][a]=0, r=0, max Q[s']=1 \u2192 Q'[s][a] == 0.5)
- `test_update_does_not_mutate_illegal_nan_cells`
- `test_save_load_round_trip_preserves_q_table`
- `test_num_states_counts_distinct_seen_boards`

**Verify:** `pytest tests/test_agent.py -v` \u2192 all green.

**Commit:** `feat(agent): QLearningAgent with \u03b5-greedy, Q-update, save/load`.

---

## Stage 4 \u2014 Training loop and evaluation (`rl_ttt/train.py`)

Two training entry points (phase 1a and 1b) and one shared evaluation routine.

```python
@dataclass
class EvalResult:
    win: float    # fraction in [0, 1]
    draw: float
    loss: float

@dataclass
class TrainConfig:
    episodes: int
    alpha: float = 0.1
    gamma: float = 1.0
    eps_start: float
    eps_end: float
    eps_decay_episodes: int      # linearly decay over first N episodes
    eval_every: int = 1000
    eval_games_per_opponent: int = 200
    seed: int = 0

def evaluate(
    agent: QLearningAgent,
    opponent: Callable[[Board, np.random.Generator | None], int],
    n_games: int,
    rng: np.random.Generator,
) -> EvalResult: ...

def train_phase_1a(cfg: TrainConfig) -> tuple[QLearningAgent, list[dict]]:
    # Agent vs random_player. Returns (agent, metrics_history) where
    # metrics_history[i] is a dict of one eval cycle.
    ...

def train_phase_1b(
    cfg: TrainConfig,
    warm_start: QLearningAgent | None = None,
) -> tuple[QLearningAgent, list[dict]]:
    # Self-play with shared Q-table. Coin-flip first-mover per episode.
    # If warm_start is provided, use its Q-table as the starting point.
    ...
```

**Episode generator (shared by both phases) \u2014 conceptual sketch:**

```
flip a coin: agent_plays_first \u2208 {True, False}
board = initial_board()                     # always from side-to-move's view
side_to_move = "agent" if agent_plays_first else "opponent"

last_agent_s, last_agent_a = None, None

while not terminal(board):
    if side_to_move == "agent":
        a = agent.select_action(board)
        new_board = apply_move(board, a)
        done, r_immediate = is_terminal(new_board)
        if done:
            # winner=+1 if r_immediate==+1, draw if 0
            agent.update(board, a, r_immediate, None)
            if last_agent_s is not None:
                # opponent's previous move closed the path; no further bootstrap
                pass
            break
        # game continues; flip perspective for next side
        last_agent_s, last_agent_a = board, a
        board = flip_perspective(new_board)
        side_to_move = "opponent"
    else:
        # opponent moves (random for 1a, agent self-play for 1b)
        ...
        new_board = apply_move(board, a_opp)
        done, r_immediate = is_terminal(new_board)
        if done:
            # if r_immediate == +1, opponent just won \u2192 reward to AGENT is -1
            r_for_agent = -r_immediate  # +1 \u2192 -1 (loss); 0 \u2192 0 (draw)
            if last_agent_s is not None:
                agent.update(last_agent_s, last_agent_a, r_for_agent, None)
            break
        # game continues; flip perspective so agent sees its own pieces as +1
        agent_next_board = flip_perspective(new_board)
        if last_agent_s is not None:
            agent.update(last_agent_s, last_agent_a, 0.0, agent_next_board)
        board = agent_next_board
        side_to_move = "agent"
```

For self-play (1b), the "opponent" branch is the same agent calling
`select_action` on the flipped board \u2014 because the Q-table is shared and the
state is side-to-move canonical, both sides naturally use the same policy.

**Test cases:**

- `test_evaluate_against_self_returns_all_draws` (minimax vs minimax via the
  evaluate function should produce draw \u2248 1.0)
- `test_evaluate_random_vs_random_distribution_matches_known_baseline`
  (random-as-first-mover wins ~58%, draws ~13%, loses ~29% with large N
  \u2014 verify within \u00b13% with 5000 games)
- `test_eps_schedule_linear_then_constant` (compute \u03b5 at episode 0, mid-decay,
  end-of-decay, post-decay; assert correct values)
- `test_train_phase_1a_smoke` \u2014 the critical smoke test:
  - `episodes=5000`, `alpha=0.3` (faster), `eps_start=1.0, eps_end=0.1,
    eps_decay_episodes=2500`, `seed=0`
  - assert post-training agent achieves `win + draw >= 0.70` vs random
    over 500 eval games (\u03b5=0)
  - this is the regression test that catches "I broke convergence"
- `test_train_phase_1b_smoke_with_warmstart` \u2014 mini self-play smoke:
  - take phase 1a smoke output, train 5000 self-play episodes,
    assert `draw >= 0.50` vs minimax (won't reach 100% in 5k self-play,
    but should improve markedly)

**Verify:** `pytest tests/test_smoke.py -v` \u2192 all green (these are slower; can
run with `-k smoke` selector).

**Commit:** `feat(train): phase 1a + 1b training, evaluation, smoke test`.

---

## Stage 5 \u2014 Visualization + CLI

### `rl_ttt/viz.py`

```python
def print_q_sanity(agent: QLearningAgent) -> None:
    # Print Q[empty_board] reshaped 3\u00d73 with two decimals; cells are labelled
    # 'C' (center, idx 4), 'corner' (0,2,6,8), 'edge' (1,3,5,7) for clarity.
    ...

def plot_learning_curves(
    metrics_history: list[dict],
    out_path: str | Path,
) -> None:
    # 2-panel matplotlib figure:
    #   top:    win/draw/loss vs random over training episodes
    #   bottom: win/draw/loss vs minimax over training episodes
    # Plus a tiny inset/second figure or third panel for q_table_size over time.
    ...
```

### `rl_ttt/cli.py` (argparse with subcommands)

```
python -m rl_ttt train --phase {1a,1b} --episodes N --seed S \
    --out PATH [--warm-start PATH]

python -m rl_ttt eval --qtable PATH --opponent {random,minimax} \
    --games N --seed S

python -m rl_ttt play --qtable PATH
    # interactive: human types a cell index 0..8; agent responds with greedy
    # action; render after each ply; declare winner.
```

`__main__.py` simply does `from rl_ttt.cli import main; main()`.

**Verify (manual smoke):**

```bash
# Fast smoke: 1000 episodes phase 1a, ensure the CLI runs end-to-end.
python -m rl_ttt train --phase 1a --episodes 1000 --seed 0 \
    --out /tmp/rl_ttt_smoke
ls /tmp/rl_ttt_smoke   # should contain qtable.pkl, metrics.csv, learning_curves.png

# Eval the smoke checkpoint
python -m rl_ttt eval --qtable /tmp/rl_ttt_smoke/qtable.pkl \
    --opponent random --games 200 --seed 7
# Should print win/draw/loss percentages; no crash.

# Interactive play (sanity \u2014 hit '4' then ctrl-C, just want to see render)
echo 4 | python -m rl_ttt play --qtable /tmp/rl_ttt_smoke/qtable.pkl
```

**Commit:** `feat(viz,cli): sanity prints, learning-curve plots, CLI subcommands`.

---

## Stage 6 \u2014 Run full training, verify success criteria

```bash
# Phase 1a (vs random, 50k eps)
python -m rl_ttt train --phase 1a --episodes 50000 --seed 0 \
    --out artifacts/phase1a

# Phase 1b (self-play, 100k eps, warm-started)
python -m rl_ttt train --phase 1b --episodes 100000 --seed 0 \
    --warm-start artifacts/phase1a/qtable.pkl --out artifacts/phase1b

# Final evaluation: 1000 games per opponent
python -m rl_ttt eval --qtable artifacts/phase1b/qtable.pkl \
    --opponent random --games 1000 --seed 42
python -m rl_ttt eval --qtable artifacts/phase1b/qtable.pkl \
    --opponent minimax --games 1000 --seed 42
```

**Verify against success criteria (`docs/DESIGN.md`):**

| Metric | Required |
|---|---|
| Phase 1a vs random: win-or-draw | \u2265 95 % |
| Phase 1a vs minimax: draw | \u2248 80 % plateau |
| Phase 1b vs random: win-or-draw | \u2265 98 % |
| Phase 1b vs minimax: draw | **100 %** |
| Q[empty_board] sanity ordering | center > corners > edges |

If any criterion fails, debug before committing. Use the systematic-debugging
skill.

**Write `docs/RESULTS.md`** with:

- Final per-opponent win/draw/loss percentages for both phases.
- The Q[empty_board] sanity grid (text).
- Q-table size at end of each phase.
- Inline reference to `artifacts/phase{1a,1b}/learning_curves.png`
  (these are gitignored binaries; the doc describes them in text).

**Commit:** `docs(results): record final training outcomes and verification`.
This commit does NOT include any files from `artifacts/` (they remain
gitignored binaries on disk for local inspection).

---

## Stage 7 \u2014 README polish

Update `README.md` to:

- Replace the Stage 0 status banner with a "trained and verified" badge in text.
- Add a "What I learned" section linking RL concepts to specific files/lines.
- Add a "How to interpret the learning curves" guide for someone reading
  this project as study material.

**Commit:** `docs(readme): finalize with results summary and study guide`.

---

## Skill notes for whoever executes (including future me)

- **TDD discipline:** every test gets the red-green-refactor cycle. Watching the
  test fail is non-negotiable; this is what proves the test actually tests
  the thing.
- **Verification discipline:** never say "this works" without showing the
  output of the command that proves it works in the same message.
- **Commit boundaries:** each stage's commit message is suggested above. Stage
  boundaries are checkpoints \u2014 if a stage doesn't pass verification, do not
  proceed to the next stage.
