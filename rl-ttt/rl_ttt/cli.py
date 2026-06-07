"""Command-line interface for rl_ttt.

Subcommands:
    train  --phase {1a,1b} --episodes N --seed S --out DIR [--warm-start PATH]
    eval   --qtable PATH --opponent {random,minimax} --games N --seed S
    play   --qtable PATH
    export --qtable PATH --out PATH [--seed S]

Each train run produces three artifacts in --out:
    qtable.pkl            pickled Q-table + hyperparameters
    metrics.csv           one row per evaluation cycle
    learning_curves.png   3-panel plot (vs random, vs minimax, Q-size growth)

`play` is an interactive human-vs-agent session. The human types a cell
index 0..8; the agent responds with its greedy move. Ctrl-C exits.

`export` writes a JS-loadable copy of the Q-table to feed the browser-based
web UI (see docs/WEB_UI.md).
"""

from __future__ import annotations

import argparse
import csv
import sys
from pathlib import Path

import numpy as np

from rl_ttt.agent import QLearningAgent
from rl_ttt.export import write_qtable_js
from rl_ttt.game import (
    Board,
    EMPTY, ME, OPP,
    initial_board,
    legal_moves,
    apply_move,
    flip_perspective,
    is_terminal,
    render,
)
from rl_ttt.opponents import minimax_player, random_player
from rl_ttt.train import TrainConfig, evaluate, train_phase_1a, train_phase_1b
from rl_ttt.viz import format_q_sanity, plot_learning_curves


# --- phase-specific defaults (mirror docs/DESIGN.md) -----------------------

_PHASE_DEFAULTS = {
    "1a": dict(
        alpha=0.1, gamma=1.0,
        eps_start=1.0, eps_end=0.1,
        eps_decay_episodes_fraction=0.5,
        eval_every=1000, eval_games_per_opponent=200,
    ),
    "1b": dict(
        alpha=0.1, gamma=1.0,
        eps_start=0.3, eps_end=0.05,
        eps_decay_episodes_fraction=0.5,
        eval_every=1000, eval_games_per_opponent=200,
    ),
}


# --- subcommand: train ------------------------------------------------------

def _cmd_train(args: argparse.Namespace) -> int:
    defaults = _PHASE_DEFAULTS[args.phase]
    cfg = TrainConfig(
        episodes=args.episodes,
        alpha=defaults["alpha"],
        gamma=defaults["gamma"],
        eps_start=defaults["eps_start"],
        eps_end=defaults["eps_end"],
        eps_decay_episodes=int(args.episodes * defaults["eps_decay_episodes_fraction"]),
        eval_every=defaults["eval_every"],
        eval_games_per_opponent=defaults["eval_games_per_opponent"],
        seed=args.seed,
    )

    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)

    print(f"=== Training phase {args.phase} ===")
    print(f"  episodes:                  {cfg.episodes}")
    print(f"  alpha:                     {cfg.alpha}")
    print(f"  gamma:                     {cfg.gamma}")
    print(f"  epsilon: {cfg.eps_start} -> {cfg.eps_end}"
          f" over {cfg.eps_decay_episodes} eps, then constant")
    print(f"  eval every {cfg.eval_every} eps, {cfg.eval_games_per_opponent} games / opponent")
    print(f"  seed:                      {cfg.seed}")
    print(f"  output dir:                {out_dir}")
    if args.warm_start:
        print(f"  warm-start:                {args.warm_start}")

    if args.phase == "1a":
        if args.warm_start:
            print("WARNING: --warm-start has no effect for phase 1a; starting fresh.",
                  file=sys.stderr)
        agent, history = train_phase_1a(cfg, progress=True)
    else:
        warm = None
        if args.warm_start:
            warm = QLearningAgent.load(args.warm_start)
        agent, history = train_phase_1b(cfg, warm_start=warm, progress=True)

    # Save artifacts.
    qtable_path = out_dir / "qtable.pkl"
    metrics_path = out_dir / "metrics.csv"
    plot_path = out_dir / "learning_curves.png"

    agent.save(qtable_path)
    _write_metrics_csv(history, metrics_path)
    plot_learning_curves(history, plot_path)

    print()
    print(format_q_sanity(agent))
    print()
    final = history[-1]
    print(f"Final eval cycle (@episode {final['episode']}):")
    print(f"  vs random:   win={final['win_vs_random']:.3f}  "
          f"draw={final['draw_vs_random']:.3f}  loss={final['loss_vs_random']:.3f}")
    print(f"  vs minimax:  win={final['win_vs_minimax']:.3f}  "
          f"draw={final['draw_vs_minimax']:.3f}  loss={final['loss_vs_minimax']:.3f}")
    print()
    print(f"Saved: {qtable_path}")
    print(f"Saved: {metrics_path}")
    print(f"Saved: {plot_path}")
    return 0


def _write_metrics_csv(history: list[dict], path: Path) -> None:
    if not history:
        return
    fieldnames = list(history[0].keys())
    with open(path, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fieldnames)
        w.writeheader()
        for row in history:
            w.writerow(row)


# --- subcommand: eval -------------------------------------------------------

def _cmd_eval(args: argparse.Namespace) -> int:
    agent = QLearningAgent.load(args.qtable)
    opponent = {"random": random_player, "minimax": minimax_player}[args.opponent]
    rng = np.random.default_rng(args.seed)
    result = evaluate(agent, opponent, n_games=args.games, rng=rng)
    print(f"Eval: agent (q-table {args.qtable}) vs {args.opponent}, "
          f"{args.games} games, seed={args.seed}")
    print(f"  win:  {result.win:.4f}")
    print(f"  draw: {result.draw:.4f}")
    print(f"  loss: {result.loss:.4f}")
    return 0


# --- subcommand: play -------------------------------------------------------

_GLYPH_HUMAN = {EMPTY: ".", +1: "H", -1: "A"}  # H=human, A=agent


def _render_human_view(board_absolute: tuple[int, ...]) -> str:
    """Render a board where +1 = HUMAN's pieces, -1 = AGENT's pieces.

    Different from rl_ttt.game.render() which uses the side-to-move convention.
    """
    rows = []
    for r in range(3):
        cells = [_GLYPH_HUMAN[board_absolute[r * 3 + c]] for c in range(3)]
        rows.append(" ".join(cells))
    return "\n".join(rows) + "\n"


def _cmd_play(args: argparse.Namespace) -> int:
    agent = QLearningAgent.load(args.qtable)
    agent.epsilon = 0.0  # play greedily

    print("Interactive tic-tac-toe vs the trained agent.")
    print("Cells are indexed 0..8 (row-major):")
    print("  0 1 2")
    print("  3 4 5")
    print("  6 7 8")
    print("You are H (human); agent is A. Empty cells shown as '.'.")
    print("Ctrl-C to exit.")
    print()

    # Coin-flip who goes first.
    rng = np.random.default_rng()
    human_first = bool(rng.integers(2))
    print(f"{'You' if human_first else 'Agent'} go first.")
    print()

    # We track an "absolute" board where +1 = human, -1 = agent.
    # When the agent acts, we feed it a side-to-move-canonical view where
    # +1 = agent's pieces.
    absolute: tuple[int, ...] = (EMPTY,) * 9
    human_to_move = human_first

    while True:
        print(_render_human_view(absolute))
        # Check terminal in absolute frame: human wins if +1 has 3 in a row.
        from rl_ttt.game import winner
        w = winner(absolute)  # +1 = human win, -1 = agent win, 0 = none
        legal = [i for i, v in enumerate(absolute) if v == EMPTY]
        if w == +1:
            print("You win!")
            return 0
        if w == -1:
            print("Agent wins!")
            return 0
        if not legal:
            print("Draw.")
            return 0

        if human_to_move:
            try:
                raw = input("Your move (0-8): ").strip()
            except (EOFError, KeyboardInterrupt):
                print("\nbye")
                return 0
            try:
                a = int(raw)
            except ValueError:
                print(f"  not a number: {raw!r}")
                continue
            if a not in legal:
                print(f"  illegal move (legal: {legal})")
                continue
            absolute = tuple(v if i != a else +1 for i, v in enumerate(absolute))
            human_to_move = False
        else:
            # Build agent's side-to-move canonical view: agent's pieces = +1.
            # In our absolute frame, agent's pieces are -1, so negate.
            agent_view: Board = tuple(-v for v in absolute)  # type: ignore[assignment]
            a = agent.select_action(agent_view)
            absolute = tuple(v if i != a else -1 for i, v in enumerate(absolute))
            print(f"Agent plays cell {a}.")
            human_to_move = True


# --- argument parsing -------------------------------------------------------

def _build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="python -m rl_ttt",
        description="Train and play a tabular Q-learning tic-tac-toe agent.",
    )
    sub = p.add_subparsers(dest="cmd", required=True)

    # train
    pt = sub.add_parser("train", help="Train the agent.")
    pt.add_argument("--phase", choices=["1a", "1b"], required=True,
                    help="1a = vs random; 1b = self-play (warm-start optional).")
    pt.add_argument("--episodes", type=int, required=True,
                    help="Number of training episodes.")
    pt.add_argument("--seed", type=int, default=0,
                    help="RNG seed for reproducibility.")
    pt.add_argument("--out", type=str, required=True,
                    help="Output directory for qtable.pkl, metrics.csv, learning_curves.png.")
    pt.add_argument("--warm-start", type=str, default=None,
                    help="Path to a qtable.pkl to warm-start from (phase 1b only).")
    pt.set_defaults(func=_cmd_train)

    # eval
    pe = sub.add_parser("eval", help="Evaluate a saved agent against a fixed opponent.")
    pe.add_argument("--qtable", type=str, required=True, help="Path to qtable.pkl.")
    pe.add_argument("--opponent", choices=["random", "minimax"], required=True)
    pe.add_argument("--games", type=int, default=1000)
    pe.add_argument("--seed", type=int, default=42)
    pe.set_defaults(func=_cmd_eval)

    # play
    pp = sub.add_parser("play", help="Play interactively against the trained agent.")
    pp.add_argument("--qtable", type=str, required=True, help="Path to qtable.pkl.")
    pp.set_defaults(func=_cmd_play)

    # export
    pex = sub.add_parser(
        "export",
        help="Export a qtable.pkl to a JS-loadable file for the web UI.",
    )
    pex.add_argument("--qtable", type=str, required=True,
                     help="Path to a saved qtable.pkl.")
    pex.add_argument("--out", type=str, required=True,
                     help="Destination .js path (e.g. web/qtable.js).")
    pex.add_argument("--seed", type=int, default=None,
                     help="Optional seed to record in the header comment.")
    pex.set_defaults(func=_cmd_export)

    return p


# --- subcommand: export -----------------------------------------------------

def _cmd_export(args: argparse.Namespace) -> int:
    agent = QLearningAgent.load(args.qtable)
    write_qtable_js(
        agent,
        args.out,
        source_path=args.qtable,
        seed=args.seed,
    )
    print(f"Exported {agent.num_states} states to {args.out}")
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = _build_parser()
    args = parser.parse_args(argv)
    return args.func(args)
