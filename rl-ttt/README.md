# rl-ttt: Tic-Tac-Toe Reinforcement Learning

A pedagogical implementation of tabular Q-learning that teaches itself to play
tic-tac-toe. Designed to make every part of the RL pipeline transparent and
inspectable, with the goal of deeply understanding RL fundamentals through a
real example.

> **Status:** Complete. All seven stages from `docs/PLAN.md` are implemented
> and the final agent plays **optimal tic-tac-toe** (100% draws vs the
> perfect minimax opponent). See `docs/RESULTS.md` for the full training
> report and `docs/DESIGN.md` for the design decisions.

## Results (50k phase-1a + 100k phase-1b, seed=0)

| metric                          | phase 1a | phase 1b | target       |
|---------------------------------|----------|----------|--------------|
| win rate vs random              | 0.975    | 0.946    | ≥ 0.70       |
| draw rate vs minimax            | 1.000    | 1.000    | ≥ 0.50       |
| Q-table size (# states)         | 4488     | 4510     | n/a          |

The full milestone table, the self-play instability analysis (a brief
collapse around ep 70k–90k that self-heals), the Q-value discussion, and
a study-guide question set are in `docs/RESULTS.md`.

## Web UI

A single-page browser interface lives in `web/`. Open
`web/index.html` in any modern browser to play against the trained agent
and watch the **Q-values** in each empty cell as the model thinks. The
page runs entirely client-side — no server, no network calls.

```bash
# After training, export the Q-table to a JS file the browser can load.
python -m rl_ttt export --qtable artifacts/full_run_1b/qtable.pkl \
    --out web/qtable.js --seed 0

# Then open web/index.html. (Use `python -m http.server -d web 8000`
# if your browser blocks file:// scripts.)
```

See `docs/WEB_UI.md` for the JS file format, the side-canonicalization
logic, and the smoke test.

## Q-value Animation

`scripts/log_qstate.py` re-trains the agent (phase 1a + 1b, seed=0) and
logs Q-value snapshots for specific board states at every training
episode where the values change.  The result is consumed by
`web/qstate-animation.html`, which replays the evolution in 10 seconds
(600 frames, 60 fps).

Two states are tracked by default (both from O's perspective):
- **X in center, O to move** — `(0,0,0, 0,-1,0, 0,0,0)`
- **X in upper right, O to move** — `(0,0,-1, 0,0,0, 0,0,0)`

Each empty cell is coloured by its Q-value: red = positive (favours O),
green = negative (disfavours O), darker = stronger signal.  Occupied
cells show X.  Q-values are displayed to 3 decimal places.

```bash
# Generate the data file (~90s, ~6.9 MB output):
python scripts/log_qstate.py --out web/qstate-data.json --seed 0

# View the animation (serve the web/ directory):
python -m http.server -d web 8000
# Open http://localhost:8000/qstate-animation.html
```

The animation includes Play/Pause controls, a progress bar with a red
phase-1a → phase-1b transition marker, and a cross-link back to the
main play page.

### Editing target states

To track different board positions, edit the `CASES` list near the top
of `scripts/log_qstate.py`.  Each entry needs a canonical 9-tuple
(`ME = +1`, `OPP = -1`, `EMPTY = 0`), a human-readable label, and the
cell index where X's piece sits.

## Quick Start

```bash
# 1. Create and activate a local virtualenv
python -m venv .venv
source .venv/bin/activate

# 2. Install dependencies
pip install -r requirements.txt

# 3. Run tests (86 tests; runs in ~12s)
pytest tests/ -v

# 4. Train the agent
#    Phase 1a: 50k games vs a random opponent (validates Q-learning mechanics)
python -m rl_ttt train --phase 1a --episodes 50000 --seed 0 --out artifacts/full_run_1a

#    Phase 1b: 100k games of self-play, warm-started from Phase 1a
python -m rl_ttt train --phase 1b --episodes 100000 --seed 0 \
    --warm-start artifacts/full_run_1a/qtable.pkl --out artifacts/full_run_1b

#    Combine the two per-phase metric streams into a single CSV + plot
python scripts/combine_runs.py

# 5. Evaluate against minimax (gold standard for optimal play)
python -m rl_ttt eval --qtable artifacts/full_run_1b/qtable.pkl \
    --opponent minimax --games 1000 --seed 42

# 6. Play against the trained agent yourself (CLI)
python -m rl_ttt play --qtable artifacts/full_run_1b/qtable.pkl

# 7. Play in your browser (with Q-value visualization)
python -m rl_ttt export --qtable artifacts/full_run_1b/qtable.pkl \
    --out web/qtable.js --seed 0
# ... then open web/index.html
```

## What This Project Teaches

This project deliberately uses **tabular** Q-learning (a dict keyed by board
states, not a neural network) to make RL maximally transparent:

| Concept | Where to see it |
|---|---|
| Bellman update / TD(0) | `rl_ttt/agent.py` — one line of arithmetic |
| Exploration vs exploitation | `rl_ttt/agent.py` — ε-greedy with linear decay |
| Credit assignment | Sparse terminal rewards propagate back via the Q-update |
| Stationary vs non-stationary env | Phase 1a (random) vs Phase 1b (self-play) |
| Self-play as a learning loop | `rl_ttt/train.py` — `train_phase_1b` |
| Self-play instability | `docs/RESULTS.md` — the ep 70k–90k collapse discussion |
| Side-to-move canonicalization | `rl_ttt/game.py` — one Q-table covers both players |
| Evaluation discipline | `rl_ttt/train.py` — ε=0 eval vs random AND vs minimax |
| Q-value interpretability limits | `docs/RESULTS.md` — "any opening → draw" analysis |
| Inspecting a trained policy | `web/` — browser UI with live Q-value overlay |
| Q-value dynamics over training | `web/qstate-animation.html` — animated evolution across 150k episodes |

## Project Layout

```
rl-ttt/
├── README.md
├── requirements.txt
├── .gitignore
├── .venv/                  # gitignored, local virtualenv
├── docs/
│   ├── DESIGN.md           # design decisions and rationale
│   ├── PLAN.md             # staged implementation plan
│   ├── RESULTS.md          # training run report and study guide
│   └── WEB_UI.md           # web UI usage, JS format, lookup logic
├── rl_ttt/                 # the package (importable from project root)
│   ├── __init__.py
│   ├── game.py             # board, legal moves, winner, perspective canonicalization
│   ├── opponents.py        # random_player, minimax_player
│   ├── agent.py            # QLearningAgent: Q-table, ε-greedy, Q-update, save/load
│   ├── train.py            # train_phase_1a, train_phase_1b, evaluate
│   ├── viz.py              # plot_learning_curves, format_q_sanity
│   ├── export.py           # qtable.pkl → qtable.js for the web UI
│   ├── cli.py              # argparse entry: train | eval | play | export
│   └── __main__.py
├── tests/
│   ├── test_game.py        # 35 tests
│   ├── test_minimax.py     # 11 tests
│   ├── test_agent.py       # 17 tests
│   ├── test_smoke.py       # 10 tests (end-to-end mini training)
│   └── test_viz.py         # 5 tests
├── scripts/
│   ├── combine_runs.py     # merges per-phase metrics into a single CSV + plot
│   └── log_qstate.py       # logs Q-values for target states during training
├── web/                    # browser-based UI: play vs agent with Q-value visualization
│   ├── index.html
│   ├── style.css
│   ├── app.js
│   ├── ql-demo.html        # step-by-step Q-learning demo
│   ├── ql-demo.js
│   ├── ql-demo.css
│   ├── qstate-animation.html  # animated Q-value evolution across training
│   ├── smoke.js            # Node.js smoke test (no extra deps)
│   ├── qtable.js           # gitignored; generated by `python -m rl_ttt export`
│   └── qstate-data.json    # gitignored; generated by `python scripts/log_qstate.py`
└── artifacts/              # gitignored; saved Q-tables, metrics CSVs, learning-curve PNGs
```

## Reproducing the Results

The run is fully deterministic with `seed=0`:

```bash
# 50k phase-1a + 100k phase-1b takes ~90s total on a modern CPU
python -m rl_ttt train --phase 1a --episodes 50000 --seed 0 --out artifacts/full_run_1a
python -m rl_ttt train --phase 1b --episodes 100000 --seed 0 \
    --warm-start artifacts/full_run_1a/qtable.pkl --out artifacts/full_run_1b
python scripts/combine_runs.py

# Final qtable is at artifacts/full_run/qtable.pkl
python -m rl_ttt eval --qtable artifacts/full_run/qtable.pkl --opponent minimax --games 1000 --seed 99
# Expected:  win=0.0000  draw=1.0000  loss=0.0000
```

The `tests/test_smoke.py` regression suite pins the success criteria at
small scale (2k eps phase-1a, 5k+5k eps phase-1b) so that future changes
that break the training pipeline are caught in CI before a full run.

## License

Personal learning project; no license declared.
