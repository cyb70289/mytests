# Q-Learning Demo Web UI — Design Spec

## Purpose

A standalone browser page that visually demonstrates how tabular TD(0) Q-learning
propagates terminal rewards backward through a Q-table across multiple episodes.
The user watches pre-scripted tic-tac-toe games and sees the Bellman update
formula computed with real numbers at each move step.

## Scope

- 4 pre-scripted episodes on one scrollable page
- Fresh Q-table (all zeros) computed in-browser at page load
- No interactive play mode; no dependency on the trained Q-table
- Cross-link with the existing play-vs-agent page

## Files

| File | Purpose |
|------|---------|
| `web/ql-demo.html` | Page structure, episode containers, legend, cross-link |
| `web/ql-demo.css` | Layout (horizontal move cards, responsive wrap), typography, colors |
| `web/ql-demo.js` | Q-table, TD(0) update logic, episode runner, DOM rendering |
| `web/index.html` | Add cross-link to `ql-demo.html` |
| `web/ql-demo.html` | Add cross-link to `index.html` |

Vanilla JS. No framework. Same conventions as existing `web/app.js` and `web/style.css`.

## Episode Design

Four episodes in two chapters. Each episode is a hard-coded array of moves
(cell indices). The demo replays them sequentially against a persistent
in-memory Q-table.

```
Chapter 1 — X Wins:
  Episode 1 moves: [4, 0, 2, 8, 6]
    X center → O corner(0) → X corner(2) → O corner(8) → X corner(6)
    X wins with diagonal 2-4-6

  Episode 2 moves: [4, 0, 6, 3, 2]
    X center → O corner(0) → X corner(6) → O edge(3) → X corner(2)
    X wins with diagonal 2-4-6 (same winning line, different path)

  Both share opening [4, 0] → Q-values from ep 1's terminal reward
  propagate through that shared state in ep 2.

Chapter 2 — X Loses:
  Episode 3 moves: [4, 0, 3, 1, 8, 2]
    X center → O corner(0) → X edge(3) → O edge(1) → X corner(8) → O corner(2)
    O wins with top row 0-1-2

  Episode 4 moves: [4, 0, 1, 3, 5, 6]
    X center → O corner(0) → X edge(1) → O edge(3) → X edge(5) → O corner(6)
    O wins with left column 0-3-6

  Both share opening [4, 0] → negative terminal reward propagates back
  through shared state in ep 4.
```

Episodes 1 & 2 share the opening moves `[4, 0]` (X center, O corner), so the
Q-value from episode 1's terminal reward propagates back through that shared
state in episode 2. Same pattern for episodes 3 & 4.

## Q-Table and TD(0) Update

- **Storage:** JavaScript `Map<string, Float64Array(9)>` keyed by raw board string
  (e.g., `"X_O______"`). No side-to-move canonicalization.
- **Initialization:** Lazy. First access to a state creates a 9-element array
  of zeros, with `null` at occupied cells.
- **α = 0.1**, **γ = 1.0**
- **Update rule:**
  - Non-terminal: `Q[s][a] += 0.1 * (0 + 1.0 * max(Q[s_next]) - Q[s][a])`
  - Terminal win:  `Q[s][a] += 0.1 * (+1 - Q[s][a])`
  - Terminal loss: `Q[s][a] += 0.1 * (-1 - Q[s][a])`

Updates are computed at page load for all episodes and the per-move results
(Q_before, Q_after, formula terms) are stored for rendering.

## Layout

One long scroll:

```
[Page header + legend]
[Chapter label: "X Wins"]
  [Episode 1: horizontal row of move cards]
  [Bridge annotation: why episode 1 looked boring, what to watch in episode 2]
  [Episode 2: horizontal row of move cards]
[Chapter label: "X Loses"]
  [Episode 3: horizontal row of move cards]
  [Bridge annotation]
  [Episode 4: horizontal row of move cards]
```

### Move Card Structure

Each move card contains:
1. **Board grid** — 3×3 with X/O pieces and Q-values overlaid on empty cells
   (3 decimal places, monospace)
2. **Move label** — "X plays cell 4" / "O plays cell 0"
3. **Q-update formula** — symbolic line + numeric substitution line:
   ```
   Q[s][4] += α · (r + γ · max Q[s'] − Q[s][4])
   Q[s][4] += 0.1 · (0 + 1.0 · 0.09 − 0.00) = 0.009
   ```
4. **Term move card (win/loss)** — larger, distinct styling, displays `r = +1`
   (green) or `r = -1` (red), simplified terminal formula

### Color Coding

- Positive Q-update: green highlight
- Negative Q-update: red highlight
- Zero update: gray, subdued
- X pieces: #2c5fa8
- O pieces: #c0463a
- Terminal win: green background card
- Terminal loss: red background card

### Responsive

On viewports < 600px wide, the horizontal move card row wraps into a vertical
stack. CSS `flex-wrap: wrap`.

## Cross-Linking

- `web/index.html` footer: add "Explore how Q-learning works → ql-demo.html"
- `web/ql-demo.html` footer: add "Play against the trained agent → index.html"

## Edge Cases

- **First move of episode 1:** `max(Q[s_next])` is always 0 since Q-table is
  empty. Update equals 0. Shown in gray.
- **Draw outcome:** Not included (pedagogically boring — terminal r=0).
- **Board state collisions:** Handled naturally by `Map.get()`.
- **No `qtable.js` dependency:** The demo runs standalone; `qtable.js` is not
  loaded.

## Explicitly Out of Scope

- Interactive play mode (deferred)
- Draw episodes
- Side-to-move canonicalization
- ε-greedy exploration
- Parameter adjustment UI (α, γ frozen at 0.1, 1.0)
- Symmetry canonicalization
