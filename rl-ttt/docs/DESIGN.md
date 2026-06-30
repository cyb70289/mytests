# Design Decisions

This document records the design decisions for the rl-ttt project and the
reasoning behind each one. It is the output of a structured "grilling" session
where every major decision was identified, evaluated against alternatives, and
explicitly chosen.

## Goal

**Pedagogical:** Build a working RL system whose every component is
transparent and inspectable, in order to deeply understand Q-learning
fundamentals. Tic-tac-toe is the substrate because:

- The full state space (~5,478 reachable states) fits trivially in memory.
- The game is solved \u2014 we can compare against a known-optimal player.
- Episodes are short (\u2264 9 moves), so credit assignment is observable.
- A minimax opponent for the gold-standard evaluation is ~40 lines of code.

## Algorithm

**Tabular Q-learning.** Off-policy TD(0) control with a dict-backed Q-table.

## Training Curriculum

Two sequential phases inside tabular Q-learning:

| Phase | Opponent | Episodes | Why |
|---|---|---|---|
| **1a** | Random | 50,000 | Stationary environment \u2192 guaranteed convergence \u2192 validates the Q-learning mechanics in a setting where we can prove it should work. |
| **1b** | Self-play (shared Q-table, warm-started from 1a) | 100,000 | Non-stationary opponent (it improves as the agent improves). Required to reach **optimal play** (100% draws vs minimax). |

## State, Action, Q-table

| Choice | Value |
|---|---|
| State encoding | 9-tuple of `{-1, 0, +1}`, **side-to-move perspective** (`+1` = my pieces, `-1` = opponent's pieces) |
| Action space | `int` in `0..8` (cell index, row-major) |
| Q-table | `dict[tuple, np.ndarray(shape=(9,), dtype=float64)]`, `NaN` at illegal cells, lazy-init to zeros |
| First-mover | Random coin-flip per episode (training **and** eval) |
| Tie-breaking | Uniform random among Q-value ties |

**Why side-to-move canonicalization?** A single shared Q-table works for both
X-to-move and O-to-move states. `Q(s, a)` always means "expected return from
the *current side-to-move*'s perspective." Self-play becomes natural: each
ply, flip perspective.

**Why no symmetry exploitation (D\u2084 rotations/reflections)?** Considered and
deliberately rejected. With ~5,478 states (\u2248 400 KB Q-table), training is
already fast (~minutes). Symmetry canonicalization muddies the connection
between "state the agent saw" and "state in the Q-table," weakening sanity
checks. Pedagogical clarity > 8\u00d7 data efficiency.

## Reward and Discount

| Concept | Value |
|---|---|
| Win (my move ended the game in my favor) | `+1` |
| Loss (opponent's reply ended the game in their favor) | `-1` |
| Draw (any terminal that isn't a win/loss) | `0` |
| Non-terminal | `0` |
| Discount \u03b3 | `1.0` (undiscounted) |

**Why sparse terminal rewards?** It is the canonical formulation. It forces
the agent to learn credit assignment (the central RL skill) rather than
having a domain heuristic baked in via reward shaping. Q-values become
directly interpretable as "expected terminal outcome from (s, a) under the
current policy."

**Why \u03b3 = 1?** Episodes are at most 9 steps. Discounting changes nothing
observable; undiscounted returns are easier to reason about.

## Q-update

```
non-terminal: Q[s][a] += \u03b1 * (r + \u03b3 * nanmax(Q[s']) - Q[s][a])
terminal:     Q[s][a] += \u03b1 * (r - Q[s][a])
```

`s'` is the canonical state on the agent's **next** turn (after the
opponent's reply). `r` is the reward observed across the two-ply transition
(my move + opponent's reply).

## Exploration

\u03b5-greedy with linear decay:

| Phase | \u03b5_start | \u03b5_end | Decay window | After decay |
|---|---|---|---|---|
| 1a | 1.0 | 0.1 | first 25,000 episodes | constant 0.1 |
| 1b | 0.3 | 0.05 | first 50,000 episodes | constant 0.05 |
| Eval | 0 | 0 | n/a | always greedy |

Both sides explore in self-play (symmetric, unbiased data). Eval is always
greedy so the metric reflects the *learned* policy, not behavior-policy noise.

## Hyperparameters

| Symbol | Value |
|---|---|
| \u03b1 (learning rate) | 0.1, fixed |
| \u03b3 (discount) | 1.0 |
| Seed | passed via `--seed` CLI flag (reproducibility) |

## Evaluation Protocol

Every 1,000 training episodes:

1. Snapshot the Q-table.
2. Play 200 games (\u03b5 = 0) vs **random** \u2192 record win/draw/loss %.
3. Play 200 games (\u03b5 = 0) vs **minimax-optimal** \u2192 record win/draw/loss %.
4. Print Q[empty_board] reshaped 3\u00d73 as a sanity grid.
5. Print the Q-table size (should plateau near ~5,478).

Final evaluation: 1,000 games per opponent.

## Success Criteria

| Metric | Phase 1a target | Phase 1b target |
|---|---|---|
| vs random: win-or-draw % | \u2265 95 % | \u2265 98 % |
| vs minimax: draw % | \u2248 80 % (plateau) | **100 %** (proves optimal play reached) |
| Q[empty_board] ordering | center > corners > edges | center > corners > edges |
| All pytest tests | pass | pass |

## Explicitly Out of Scope (deferred or not planned)

- **DQN** \u2014 deferred to a separate next-step project.
- **Symmetry exploitation** \u2014 not planned.
- **Multi-seed variance reporting** \u2014 deferred.
- **\u03b1 decay schedules** (e.g., Robbins-Monro 1/N(s,a)) \u2014 deferred.
- **Reward shaping** \u2014 intentionally excluded as anti-pedagogical.
- **GPU acceleration** \u2014 unnecessary; CPU training takes minutes.
