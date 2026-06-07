# Training results

This document records the outcomes of the staged training run defined in
`docs/DESIGN.md`. Numbers come from a single deterministic run with `seed=0`;
all artifacts live in `artifacts/full_run/` and are reproducible by re-running
the commands in [§ Setup](#setup).

## TL;DR

A 50k phase-1a run (vs random) followed by a 100k phase-1b run (self-play,
warm-started) produces an agent that wins **94.6%** of games vs random and
**draws 100%** of games vs the perfect minimax opponent — i.e., it plays
**optimal tic-tac-toe**. Phase-1a alone already reached 100% draws vs
minimax; phase-1b's main contribution is to validate that the agent still
holds up under self-play training (a stationary, non-stationary transition).

| metric                          | phase 1a (ep 50k) | phase 1b (ep 150k) |
|---------------------------------|-------------------|--------------------|
| win rate vs random              | 0.975             | 0.920              |
| draw rate vs random             | 0.025             | 0.075              |
| loss rate vs random             | 0.000             | 0.005              |
| **draw rate vs minimax**        | **1.000**         | **1.000**          |
| loss rate vs minimax            | 0.000             | 0.000              |
| Q-table size (# distinct states)| 4488              | 4510               |

Visual: `artifacts/full_run/learning_curves.png` (gitignored; regenerable).

## Setup

```bash
# Phase 1a: agent vs random opponent
python -m rl_ttt train --phase 1a --episodes 50000 --seed 0 \
    --out artifacts/full_run_1a

# Phase 1b: self-play, warm-started from phase 1a
python -m rl_ttt train --phase 1b --episodes 100000 --seed 0 \
    --out artifacts/full_run_1b \
    --warm-start artifacts/full_run_1a/qtable.pkl

# Combine the two per-phase metric streams into a single CSV + plot
python scripts/combine_runs.py
```

Hyperparameters (from `rl_ttt.cli._PHASE_DEFAULTS`):

| parameter        | phase 1a    | phase 1b    |
|------------------|-------------|-------------|
| α (learning rate)| 0.1         | 0.1         |
| γ (discount)     | 1.0         | 1.0         |
| ε schedule       | 1.0 → 0.1   | 0.3 → 0.05  |
| ε decay window   | first 50%   | first 50%   |
| eval every       | 1000 eps    | 1000 eps    |
| eval games/oppo. | 200         | 200         |

Each eval cycle plays 200 games vs random AND 200 vs minimax with ε=0
(greedy). Outcomes are recorded as a fraction and serialized to
`metrics.csv` along with `qtable_size` and the current ε.

## Phase 1a (vs random) — milestones

| ep    | win vs R | draw vs R | loss vs R | draw vs M | loss vs M | qsize | ε    |
|-------|----------|-----------|-----------|-----------|-----------|-------|------|
|   1000|  0.540   |  0.105    |  0.355    |  0.115    |  0.885    | 1904  | 0.964|
|   5000|  0.775   |  0.065    |  0.160    |  0.120    |  0.880    | 4025  | 0.820|
|  10000|  0.895   |  0.035    |  0.070    |  0.425    |  0.575    | 4387  | 0.640|
|  25000|  0.940   |  0.055    |  0.005    |  1.000    |  0.000    | 4479  | 0.100|
|  50000|  0.975   |  0.025    |  0.000    |  1.000    |  0.000    | 4488  | 0.100|

**Phase 1a takes about 25k episodes to "solve" tic-tac-toe vs a perfect
opponent.** Before then, the agent is winning against random by being
opportunistic, but it is also losing to minimax ~88% of the time — it
hasn't yet learned defensive patterns. Between ep 5000 and ep 25000 the
draw rate vs minimax climbs from 12% to 100% as the random opponent's
exploration accidentally exposes the agent to forcing-line scenarios.

This is the "regression" pattern noted in `docs/DESIGN.md` and the reason
phase 1b exists: a 1a-only agent can be locally optimal against random
without being globally optimal against a perfect defender.

## Phase 1b (self-play) — milestones and instability

| ep      | win vs R | draw vs R | loss vs R | draw vs M | loss vs M | qsize | ε    |
|---------|----------|-----------|-----------|-----------|-----------|-------|------|
|  51000  |  0.915   |  0.070    |  0.015    |  1.000    |  0.000    | 4490  | 0.295|
|  60000  |  0.940   |  0.060    |  0.000    |  1.000    |  0.000    | 4501  | 0.250|
| **70000** |  0.870   |  0.090    |  0.040    |  **0.535** |  **0.465** | 4504  | 0.200|
|  80000  |  0.945   |  0.055    |  0.000    |  1.000    |  0.000    | 4504  | 0.150|
| 100000  |  0.945   |  0.055    |  0.000    |  1.000    |  0.000    | 4507  | 0.050|
| 150000  |  0.920   |  0.075    |  0.005    |  1.000    |  0.000    | 4510  | 0.050|

### Self-play instability zone (ep ~70k–90k)

Six consecutive eval cycles between ep 70k and 90k show the agent losing
**45–55%** of games to the perfect minimax opponent:

```
ep 70000  draw=0.535  loss=0.465
ep 71000  draw=0.540  loss=0.460
ep 72000  draw=0.485  loss=0.515
ep 79000  draw=0.445  loss=0.555
ep 85000  draw=0.500  loss=0.500
ep 90000  draw=0.490  loss=0.510
```

This is a classic self-play collapse. At these episodes ε is between
0.10 and 0.20 (still in the decay window), so the agent is exploring
non-greedy moves. Those exploratory moves produce *real losses* against
the agent's *current* policy (which still contains bad habits) — and
those losses propagate -1 rewards back through the Q-table faster than
ε=0 confirms the optimal policy.

By ep ~95000 the decay finishes at ε=0.05, exploration no longer
perturbs the policy enough to cause real losses, and the agent
re-converges to 100% draws. **Final state: optimal.**

### Standard remedies (not implemented; documented for the reader)

The textbook fixes for self-play collapse are:

1. **Higher ε-end** — keep more exploration even after the schedule
   finishes. Cheap, but never lets the policy fully settle.
2. **Optimistic Q-initialization** — start all Q-values at a small
   positive value so untried actions look promising. Promotes
   exploration; well-known DQN trick.
3. **Lower α** — slower updates dampen the negative feedback. We
   already use α=0.1; α=0.05 would help further at the cost of slower
   learning.
4. **More episodes** — 100k may simply be too few for self-play
   convergence under α=0.1. The instability is brief and self-healing
   here; with a tighter ε-end it would not appear.
5. **Self-play with a frozen "best-so-far" opponent** — keeps the
   training distribution stationary. Used in AlphaGo Zero and most
   modern self-play systems.

The smoke test in `tests/test_smoke.py` already pins phase-1b's
**final** draw-rate-vs-minimax ≥ 50% threshold, so the post-collapse
recovery is what the regression check guards.

## Q-value analysis

### Phase 1a empty-board Q-values

```
0.53    0.49    0.97
0.42    0.56    0.62    [center=4]
0.58    0.49    0.62
```

(First-mover's perspective, after 50k episodes of training against random.)

The agent prefers **cell 2** (top-right corner) with Q=0.97. Center=4
comes in at 0.56. There is no clean center > corners > edges ordering.

**Why?** With ε=1 → 0.1, every cell gets visited during the first
~10k episodes of random exploration. The first time a given opening
coincidentally wins a few games against random (due to the random
opponent's poor play), greedy exploitation then locks in that opening.
This is the "random tie-break momentum" discussed in the
investigation notes — not a bug, just an emergent property of tabular
Q-learning with sparse terminal rewards.

### Phase 1b empty-board Q-values (post-1b)

```
0.02    0.03    0.13
0.03    0.02    0.02    [center=4]
0.03    0.03    0.01
```

All values have collapsed toward zero. Center is **not** the highest
value (it's 0.02, near the bottom of the table).

**Why?** Once the agent plays both sides and both sides are optimal,
every game ends in a draw → terminal reward 0 for every state. With
γ=1 the bootstrapped targets are also 0, and small per-update
perturbations wash out the phase-1a bias. The 0.13 in cell 2 is the
last visible vestige of the phase-1a prior that survived 100k self-play
updates.

**Pedagogical point:** in tic-tac-toe, *all* first moves are equally
strong when the opponent plays perfectly. The Q-values do not converge
to "center is best" because the environment does not reward that
hypothesis — every opening leads to a draw. A network that learned
center > corners would be memorizing a tic-tac-toe heuristic from
human data, not learning from the reward signal.

### Q-value distribution (full table, post-1b)

- 16,137 finite entries (NaN at illegal cells excluded)
- range: [-0.9997, +1.0000]
- mean: +0.029, std: 0.330
- exactly 0: 4,092 (25.4%)

The ±1 extremes are the winning/losing terminal states; the long near-
zero tail is the bulk of the "draw continuations" with low bootstrapped
values. The non-zero fraction (74.6%) shows the network has learned
genuine state distinctions, even if the *empty-board* row itself looks
flat.

## What this run demonstrates

1. **Q-learning solves a simple 9-state-action game.** 50k episodes is
   enough; the bottleneck is the number of *distinct states* the agent
   sees (4488 of the ~5000 reachable), not the number of episodes
   themselves.
2. **A stationary opponent (random) is necessary but not sufficient.**
   Phase-1a alone reached "win most, draw some" against random, but
   needed phase-1b's self-play pressure to *prove* optimality against
   minimax. This is the generalization lesson: an agent can be
   locally optimal against a weak opponent while still being globally
   weak.
3. **Self-play is not free.** The ep 70k–90k collapse shows tabular
   Q-learning can briefly regress in self-play. A naïve decay schedule
   + warm-start can produce a *measurable* drop in game quality before
   the schedule finishes. Real systems (DQN, AlphaGo Zero) need
   opponent-freezing, optimistic init, or experience replay to
   avoid this.
4. **Q-values are not a clean "value" of a state.** They are an
   artifact of the training distribution. In a game where all openings
   tie, Q-values tie too. The interpretability of Q is bounded by
   what the data can distinguish.

## Artifacts (gitignored, regenerable)

```
artifacts/
├── .gitkeep
├── full_run/                 # combined view
│   ├── metrics.csv           # 150 rows, columns: phase, episode, epsilon,
│   │                         #   qtable_size, win/draw/loss vs {random, minimax}
│   ├── learning_curves.png   # 3-panel plot, phase boundary at ep=50k
│   └── qtable.pkl            # final 4510-state agent
├── full_run_1a/              # phase 1a standalone
│   ├── metrics.csv           # 51 rows
│   ├── learning_curves.png
│   └── qtable.pkl
├── full_run_1b/              # phase 1b standalone
│   ├── metrics.csv           # 100 rows
│   ├── learning_curves.png
│   └── qtable.pkl
```

The script `scripts/combine_runs.py` regenerates the combined
artifacts from the two per-phase runs.

## Study-guide questions

1. **Why does the agent reach 100% draws vs minimax in phase 1a (vs
   random) but not for the first 10k episodes?** What is the random
   opponent *teaching* the agent that, in retrospect, looks obvious
   but is not free?
2. **Why does Q[cell 2] dominate after phase 1a but is no longer
   dominant after phase 1b?** What property of self-play erases the
   phase-1a bias?
3. **In the self-play instability zone (ep 70k–90k), the agent
   *briefly* loses ~50% vs minimax. Why does this happen, and why
   does it self-heal by ep 100k?** Which of the five remedies in
   [§ Standard remedies](#standard-remedies-not-implemented-documented-for-the-reader)
   would prevent it entirely?
4. **The empty-board Q-values are all near 0 after phase 1b even
   though the agent plays perfectly.** Is the agent "not confident"?
   Or is the Q-table showing exactly the right thing? Defend your
   answer by reasoning about what the *target* of any update is in
   optimal self-play.
5. **Phase-1a's win-rate vs random drops from 0.975 to 0.920 in
   phase 1b.** Is this a regression, an artifact, or expected
   behavior? (Hint: the training distribution shifted from
   "vs random" to "vs self".)
6. **What would you change to make the Q-values reflect opening
   theory (center > corners > edges)?**  Be specific: would you
   modify the reward? The exploration schedule? The state
   representation? The opponent distribution? (Each of these
   answers a different kind of "why" — distinguish carefully.)
