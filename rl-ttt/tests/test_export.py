"""Tests for rl_ttt.export.

The exporter converts a QLearningAgent's Q-table to a JS-loadable file.
The output is also valid JSON, so tests can roundtrip-check the inner
object directly.
"""
from __future__ import annotations

import json
import re
from pathlib import Path

import numpy as np
import pytest

from rl_ttt.agent import QLearningAgent
from rl_ttt.export import qtable_to_js_dict, write_qtable_js
from rl_ttt.game import EMPTY, ME, OPP, initial_board, apply_move


# --- helpers ----------------------------------------------------------------

def _extract_json_object(text: str) -> dict:
    """Pull the JSON object literal out of a 'window.QTABLE = {...};' file.

    Strips the JS wrapper and any header comments, then json.loads the
    remaining object. The exporter is contractually required to keep the
    inner object valid JSON.
    """
    # Strip leading comment lines (// ...) before the assignment.
    m = re.search(r"=\s*(\{.*\});?\s*$", text, flags=re.DOTALL)
    assert m is not None, f"could not find JSON object in:\n{text[:500]}"
    return json.loads(m.group(1))


# --- qtable_to_js_dict ------------------------------------------------------

def test_empty_agent_exports_zero_states():
    """A never-touched agent has no Q-table entries."""
    a = QLearningAgent()
    d = qtable_to_js_dict(a)
    assert d == {"numStates": 0, "states": {}}


def test_single_state_roundtrips():
    """A Q-update on the empty board produces one state with 9 entries."""
    a = QLearningAgent(alpha=0.1, gamma=1.0, epsilon=0.0)
    # Force the empty board into the Q-table by calling q_values (lazy init).
    a.q_values(initial_board())
    d = qtable_to_js_dict(a)
    assert d["numStates"] == 1
    assert d["numStates"] == len(d["states"])
    key = "0,0,0,0,0,0,0,0,0"
    assert key in d["states"]
    assert d["states"][key] == [0.0] * 9


def test_illegal_cells_become_null():
    """Cells marked NaN in the Q-array (occupied cells) become None/null."""
    a = QLearningAgent(alpha=0.1, gamma=1.0, epsilon=0.0)
    b = initial_board()
    b = apply_move(b, 0)  # cell 0 now has ME=+1
    a.q_values(b)
    d = qtable_to_js_dict(a)
    key = "1,0,0,0,0,0,0,0,0"
    q_arr = d["states"][key]
    assert q_arr[0] is None, "occupied cell 0 should be null"
    for i in (1, 2, 3, 4, 5, 6, 7, 8):
        assert q_arr[i] == 0.0, f"empty cell {i} should be 0.0"


def test_numerical_values_are_floats_not_numpy_types():
    """JSON cannot serialize numpy.float64 — the exporter must coerce."""
    a = QLearningAgent(alpha=0.5, gamma=1.0, epsilon=0.0)
    a.update(initial_board(), 4, 1.0, None)  # Q[empty][4] = 0.5
    d = qtable_to_js_dict(a)
    key = "0,0,0,0,0,0,0,0,0"
    val = d["states"][key][4]
    assert isinstance(val, float)
    assert val == pytest.approx(0.5)
    # Whole dict must be json.dumps-able without a TypeError.
    json.dumps(d)


# --- write_qtable_js --------------------------------------------------------

def test_write_creates_file_and_assigns_window_qtable(tmp_path: Path):
    a = QLearningAgent()
    a.q_values(initial_board())
    out = tmp_path / "qtable.js"
    write_qtable_js(a, out)
    text = out.read_text()
    assert "window.QTABLE" in text
    assert text.rstrip().endswith(";")
    # Inner object is parseable JSON.
    d = _extract_json_object(text)
    assert d["numStates"] == 1


def test_write_includes_provenance_header(tmp_path: Path):
    a = QLearningAgent()
    a.q_values(initial_board())
    out = tmp_path / "qtable.js"
    write_qtable_js(a, out, source_path="some/qtable.pkl", seed=42)
    text = out.read_text()
    assert "some/qtable.pkl" in text, "source path must be recorded in header"
    assert "42" in text, "seed value must be recorded in header"
    # Header is a leading // comment block, before the window.QTABLE assignment.
    assert text.lstrip().startswith("//"), "file should start with comment lines"
    assert "numStates" not in text.split("\n")[0], "first line is header, not payload"


def test_write_creates_parent_dir(tmp_path: Path):
    a = QLearningAgent()
    a.q_values(initial_board())
    out = tmp_path / "nested/dir/qtable.js"
    write_qtable_js(a, out)
    assert out.exists()


# --- roundtrip property: exported dict can re-construct the Q-values -------

def test_exported_dict_matches_original_qtable():
    """The exported dict, re-read in Python, must equal the agent's Q-table
    up to NaN→None and float coercion."""
    rng = np.random.default_rng(0)
    a = QLearningAgent(alpha=0.3, gamma=1.0, epsilon=0.0, rng=rng)
    # Seed with a few updates so the table has interesting values.
    a.update(initial_board(), 4, +1.0, None)        # Q[empty][4] = 0.3
    a.update(initial_board(), 0, -1.0, None)        # Q[empty][0] = -0.3
    a.update(initial_board(), 0, +1.0, None)        # Q[empty][0] = 0.0
    b = apply_move(initial_board(), 0)              # cell 0 occupied
    a.update(b, 4, +1.0, None)                       # Q[b][4] = 0.3, Q[b][0]=NaN

    d = qtable_to_js_dict(a)
    # Re-build an equivalent agent-side view from the dict.
    for key, q_list in d["states"].items():
        board = tuple(int(x) for x in key.split(","))
        original = a.q_values(board)
        for i, exported in enumerate(q_list):
            original_val = original[i]
            if np.isnan(original_val):
                assert exported is None
            else:
                assert exported == pytest.approx(float(original_val))
