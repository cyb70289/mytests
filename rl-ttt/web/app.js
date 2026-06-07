// rl-ttt web UI — game logic, Q-value lookup, DOM rendering.
// Vanilla JS, no framework. Loaded after qtable.js, which assigns
// window.QTABLE = { numStates, states: { "<9-csv>": [9 floats or null] } }.

"use strict";

(function () {

  // ---- constants ------------------------------------------------------------

  const X = +1;
  const O = -1;
  const EMPTY = 0;

  const WIN_LINES = [
    [0, 1, 2], [3, 4, 5], [6, 7, 8],
    [0, 3, 6], [1, 4, 7], [2, 5, 8],
    [0, 4, 8], [2, 4, 6],
  ];

  // ---- state ---------------------------------------------------------------

  /** Absolute board: +1 = X, -1 = O, 0 = empty. */
  let board;

  /** "playing" | "gameover". */
  let phase;

  /** null | "X" | "O" | "draw". */
  let result;

  /** { cell, side } of the most recent move (for highlight). */
  let lastMove;

  /** Number of moves played in the current game. */
  let moveCount;

  // ---- pure helpers --------------------------------------------------------

  function sideToMovePiece() {
    const xCount = board.filter((v) => v === X).length;
    const oCount = board.filter((v) => v === O).length;
    return xCount === oCount ? X : O;
  }

  function sideToMoveName() {
    return sideToMovePiece() === X ? "X" : "O";
  }

  function legalMoves(b) {
    const out = [];
    for (let i = 0; i < 9; i++) if (b[i] === EMPTY) out.push(i);
    return out;
  }

  function checkWinner(b) {
    for (const [a, c, d] of WIN_LINES) {
      const s = b[a];
      if (s !== EMPTY && s === b[c] && s === b[d]) return s;
    }
    return 0;
  }

  function isDraw(b) {
    return checkWinner(b) === 0 && legalMoves(b).length === 0;
  }

  // ---- Q-table access ------------------------------------------------------

  /**
   * Return the canonical state (9-array) with the current side-to-move as +1.
   *
   * The Q-table is keyed by side-to-move canonical where the player about
   * to act is +1.  We canonicalize from whoever's turn it is right now, so
   * the displayed Q-values always mean "expected value for the player
   * whose turn it is".
   */
  function canonicalForCurrentPlayer(b) {
    const currentPlayer = sideToMovePiece(); // +1 = X, -1 = O
    return b.map((v) => currentPlayer * v);
  }

  /**
   * Return the 9-element Q-array for the current board (from the
   * perspective of the player whose turn it is), with null at illegal
   * cells.  Looks up the Q-table by canonical key.
   */
  function getCurrentQValues(b) {
    const key = canonicalForCurrentPlayer(b).join(",");
    const row = QTABLE.states[key];
    if (!row) {
      return new Array(9).fill(0);
    }
    return row;
  }

  // ---- DOM build -----------------------------------------------------------

  function buildBoardDOM() {
    const root = document.getElementById("board");
    root.innerHTML = "";
    for (let i = 0; i < 9; i++) {
      const cell = document.createElement("div");
      cell.className = "cell";
      cell.dataset.cell = String(i);
      cell.setAttribute("role", "gridcell");
      cell.setAttribute("aria-label", `cell ${i}`);

      const piece = document.createElement("span");
      piece.className = "piece";
      const qval = document.createElement("span");
      qval.className = "qval";

      cell.appendChild(piece);
      cell.appendChild(qval);
      cell.addEventListener("click", onCellClick);
      root.appendChild(cell);
    }
  }

  // ---- render --------------------------------------------------------------

  function render() {
    const q = getCurrentQValues(board);
    const currentSide = sideToMoveName();

    // -- cells
    const cellEls = document.querySelectorAll(".cell");
    cellEls.forEach((cellEl, i) => {
      const v = board[i];
      const pieceEl = cellEl.querySelector(".piece");
      const qvalEl = cellEl.querySelector(".qval");

      // Reset classes (keep only the structural ones).
      cellEl.className = "cell";

      // Piece.
      if (v === X) {
        pieceEl.textContent = "X";
        cellEl.classList.add("x");
      } else if (v === O) {
        pieceEl.textContent = "O";
        cellEl.classList.add("o");
      } else {
        pieceEl.textContent = "";
      }

      // Q-value (only on empty cells).
      qvalEl.className = "qval " + currentSide.toLowerCase();
      if (v === EMPTY) {
        const qv = q[i];
        if (qv !== null && qv !== undefined && Number.isFinite(qv)) {
          qvalEl.textContent = qv.toFixed(2);
        } else {
          qvalEl.textContent = "";
        }
      } else {
        qvalEl.textContent = "";
      }

      // Last-move highlight.
      if (lastMove && lastMove.cell === i) {
        cellEl.classList.add("last-move");
        cellEl.classList.add(lastMove.side === "X" ? "x" : "o");
      }

      // Clickable iff a game is in progress and the cell is empty.
      const clickable =
        phase === "playing" && v === EMPTY;
      if (clickable) cellEl.classList.add("clickable");
    });

    // -- status
    const turnEl = document.getElementById("turn");
    if (result === "X" || result === "O") {
      turnEl.textContent = result + " wins.";
      turnEl.className = "t" + result.toLowerCase();
    } else if (result === "draw") {
      turnEl.textContent = "Draw.";
      turnEl.className = "";
    } else {
      turnEl.textContent = currentSide + " to play ...";
      turnEl.className = "t" + currentSide.toLowerCase();
    }

    // -- stats
    document.getElementById("movecount").textContent = String(moveCount);
    document.getElementById("qsize").textContent = String(QTABLE.numStates);
  }

  // ---- event handlers ------------------------------------------------------

  function onCellClick(ev) {
    if (phase !== "playing") return;
    const i = Number(ev.currentTarget.dataset.cell);
    if (board[i] !== EMPTY) return;
    applyMove(i);
  }

  function onNewGame() {
    startNewGame();
  }

  // ---- game flow -----------------------------------------------------------

  function startNewGame() {
    board = new Array(9).fill(EMPTY);
    result = null;
    lastMove = null;
    moveCount = 0;
    phase = "playing";
    render();
  }

  function applyMove(i) {
    const p = sideToMovePiece();
    board[i] = p;
    lastMove = { cell: i, side: p === X ? "X" : "O" };
    moveCount += 1;
    // Resolve game end.
    const w = checkWinner(board);
    if (w !== 0) {
      result = w === X ? "X" : "O";
      phase = "gameover";
    } else if (isDraw(board)) {
      result = "draw";
      phase = "gameover";
    }
    render();
  }

  // ---- init ----------------------------------------------------------------

  function init() {
    if (typeof QTABLE === "undefined") {
      document.getElementById("turn").textContent =
        "QTABLE not loaded — make sure qtable.js is in the same directory.";
      return;
    }
    buildBoardDOM();
    document.getElementById("new-game").addEventListener("click", onNewGame);
    startNewGame();
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
  } else {
    init();
  }
})();
