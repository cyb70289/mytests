"""rl-ttt: Tabular Q-learning for tic-tac-toe.

This package implements a tabular Q-learning agent that learns to play
tic-tac-toe optimally through two training phases:

  Phase 1a: vs random opponent (validates Q-learning mechanics)
  Phase 1b: self-play with shared Q-table (reaches optimal play)

See docs/DESIGN.md for design decisions and docs/PLAN.md for the staged
implementation plan.
"""
