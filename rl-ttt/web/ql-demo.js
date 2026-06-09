// ql-demo.js — Q-learning demo: TD(0) episode replay and rendering
"use strict";

(function () {

// ---- episode data ---------------------------------------------------------

var CHAPTERS = [
  {
    title: "X Wins",
    episodes: [
      { label: "X center \u2192 O corner \u2192 X corner \u2192 O corner \u2192 X wins", moves: [4, 0, 2, 8, 6] },
    ],
  },
  {
    title: "X Loses",
    episodes: [
      { label: "X center \u2192 O corner \u2192 X edge \u2192 O edge \u2192 X corner \u2192 O wins", moves: [4, 0, 3, 1, 8, 2] },
    ],
  },
];

// ---- game helpers ---------------------------------------------------------

var ALPHA = 0.1;
var GAMMA = 1.0;

var WIN_LINES = [
  [0, 1, 2], [3, 4, 5], [6, 7, 8],
  [0, 3, 6], [1, 4, 7], [2, 5, 8],
  [0, 4, 8], [2, 4, 6],
];

function emptyBoard() {
  return ["", "", "", "", "", "", "", "", ""];
}

function boardKey(b) {
  var s = "";
  for (var i = 0; i < 9; i++) s += b[i] || "_";
  return s;
}

function applyMove(b, cell, player) {
  var nb = b.slice();
  nb[cell] = player;
  return nb;
}

function checkWinner(b) {
  for (var w = 0; w < WIN_LINES.length; w++) {
    var line = WIN_LINES[w];
    var a = b[line[0]], c = b[line[1]], d = b[line[2]];
    if (a && a === c && c === d) return a;
  }
  return null;
}

function isBoardFull(b) {
  for (var i = 0; i < 9; i++) if (!b[i]) return false;
  return true;
}

// ---- Q-table --------------------------------------------------------------

/** @type {Map<string, number[]>} */
var qTable;

function initQTable() {
  qTable = new Map();
}

function getQValues(key) {
  var row = qTable.get(key);
  if (!row) {
    row = new Array(9);
    for (var i = 0; i < 9; i++) row[i] = 0.0;
    qTable.set(key, row);
  }
  return row;
}

function maxQ(key) {
  var row = getQValues(key);
  var best = -Infinity;
  for (var i = 0; i < 9; i++) {
    if (row[i] > best) best = row[i];
  }
  return best === -Infinity ? 0.0 : best;
}

/**
 * Apply TD(0) update and return the computed terms for display.
 *
 * Non-terminal: Q[s][a] += alpha * (r + gamma * maxQ(sNext) - Q[s][a])
 * Terminal:     Q[s][a] += alpha * (r - Q[s][a])
 */
function qUpdate(sKey, a, r, sNextKey) {
  var row = getQValues(sKey);
  var qBefore = row[a];
  var maxQNext = sNextKey !== null ? maxQ(sNextKey) : 0.0;
  var isTerminal = sNextKey === null;

  var target;
  if (isTerminal) {
    target = r;
  } else {
    target = r + GAMMA * maxQNext;
  }

  var delta = ALPHA * (target - qBefore);
  row[a] = qBefore + delta;

  return {
    sKey: sKey,
    a: a,
    qBefore: qBefore,
    r: r,
    maxQNext: maxQNext,
    target: target,
    delta: delta,
    qAfter: row[a],
    isTerminal: isTerminal,
  };
}

// ---- episode replay -------------------------------------------------------

/**
 * Replay all episodes sequentially against a shared Q-table.
 * Returns per-move snapshots for rendering.
 *
 * For X's moves, the Q-update is deferred until O replies (so s_next is known),
 * then stored back on X's snapshot. X terminal moves get qInfo immediately.
 * O moves never get qInfo (O doesn't learn).
 */
function replayAllEpisodes(chapters) {
  initQTable();
  var allChapterResults = [];

  for (var ci = 0; ci < chapters.length; ci++) {
    var chapter = chapters[ci];
    var chapterEpisodes = [];

    for (var ei = 0; ei < chapter.episodes.length; ei++) {
      var ep = chapter.episodes[ei];
      var moves = ep.moves;
      var snapshots = [];
      var board = emptyBoard();

      var pendingIdx = -1;
      var pendingS = null;
      var pendingA = null;

      for (var mi = 0; mi < moves.length; mi++) {
        var cell = moves[mi];
        var player = mi % 2 === 0 ? "X" : "O";
        var boardBefore = boardKey(board);
        var newBoard = applyMove(board, cell, player);
        var boardAfter = boardKey(newBoard);
        var winner = checkWinner(newBoard);
        var isDraw = !winner && isBoardFull(newBoard);
        var isTerminal = winner !== null || isDraw;

        var snapshot = {
          moveNum: mi + 1,
          player: player,
          cell: cell,
          boardBefore: boardBefore,
          boardAfter: boardAfter,
          isTerminal: isTerminal,
          winner: winner,
          isDraw: isDraw,
          qInfo: null,
          qValues: null,  // captured Q-values for boardBefore AFTER this update
        };
        snapshots.push(snapshot);

        if (player === "X") {
          pendingIdx = snapshots.length - 1;
          pendingS = boardBefore;
          pendingA = cell;

          if (isTerminal) {
            var reward = winner === "X" ? 1 : 0;
            snapshot.qInfo = qUpdate(pendingS, pendingA, reward, null);
            snapshot.qValues = getQValues(pendingS).slice();
            pendingIdx = -1;
            pendingS = null;
            pendingA = null;
          }
        } else {
          if (pendingIdx >= 0 && pendingS !== null) {
            var reward = 0;
            var sNext;
            if (isTerminal) {
              reward = winner === "O" ? -1 : 0;
              sNext = null;
            } else {
              reward = 0;
              sNext = boardAfter;
            }
            snapshots[pendingIdx].qInfo = qUpdate(pendingS, pendingA, reward, sNext);
            snapshots[pendingIdx].qValues = getQValues(pendingS).slice();
            pendingIdx = -1;
            pendingS = null;
            pendingA = null;
          }
        }

        board = newBoard;
      }

      chapterEpisodes.push({
        label: ep.label,
        snapshots: snapshots,
      });
    }

    allChapterResults.push({
      title: chapter.title,
      episodes: chapterEpisodes,
    });
  }

  return allChapterResults;
}

// ---- rendering helpers ----------------------------------------------------

function renderMiniBoard(boardKey, lastActionCell, qValues) {
  var html = '<div class="mini-board">';
  for (var i = 0; i < 9; i++) {
    var ch = boardKey[i];
    var piece = (ch === "_" || ch === undefined) ? "" : ch;
    var cls = "mini-cell";
    if (piece === "X") cls += " x";
    else if (piece === "O") cls += " o";
    if (i === lastActionCell) cls += " last-action";

    html += '<div class="' + cls + '">';
    html += piece;
    if (piece === "" && qValues) {
      var v = qValues[i];
      if (v !== null && v !== undefined && isFinite(v)) {
        html += '<span class="mini-qval">' + v.toFixed(3) + '</span>';
      }
    }
    html += '</div>';
  }
  html += '</div>';
  return html;
}

function fmtNum(n) {
  if (typeof n !== "number" || !isFinite(n)) return "\u2014";
  var s = n.toFixed(4);
  s = s.replace(/0+$/, "").replace(/\.$/, "");
  return s;
}

function renderQUpdate(qInfo) {
  if (!qInfo) return "";

  var signClass = "zero";
  if (qInfo.delta > 1e-9) signClass = "pos";
  else if (qInfo.delta < -1e-9) signClass = "neg";

  var formula, subst;

  if (qInfo.isTerminal) {
    formula = "Q[s][" + qInfo.a + "] += \u03b1 \u00b7 (r \u2212 Q[s][" + qInfo.a + "])";
    subst = "Q[" + qInfo.sKey + "][" + qInfo.a + "] += " + fmtNum(ALPHA) +
      " \u00b7 (" + fmtNum(qInfo.r) + " \u2212 " + fmtNum(qInfo.qBefore) + ") = " +
      fmtNum(qInfo.qAfter);
  } else {
    formula = "Q[s][" + qInfo.a + "] += \u03b1 \u00b7 (r + \u03b3 \u00b7 max Q[s\u2032] \u2212 Q[s][" + qInfo.a + "])";
    subst = "Q[" + qInfo.sKey + "][" + qInfo.a + "] += " + fmtNum(ALPHA) +
      " \u00b7 (" + fmtNum(qInfo.r) + " + " + fmtNum(GAMMA) +
      " \u00b7 " + fmtNum(qInfo.maxQNext) + " \u2212 " + fmtNum(qInfo.qBefore) +
      ") = " + fmtNum(qInfo.qAfter);
  }

  return '<div class="q-update ' + signClass + '">' +
    '<div>' + formula + '</div>' +
    '<div>' + subst + '</div>' +
    '</div>';
}

function renderTermOutcome(snap) {
  if (!snap.isTerminal) return "";
  if (snap.isDraw) {
    return '<div class="term-outcome">Draw</div>';
  }
  if (snap.winner === "X") {
    return '<div class="term-outcome win">X Wins! r = +1</div>';
  }
  return '<div class="term-outcome loss">X Loses! r = \u22121</div>';
}

// ---- full render ----------------------------------------------------------

function renderMoveCard(s) {
  var cardClass = "move-card";
  if (s.isTerminal && s.winner === "X") cardClass += " term-win";
  else if (s.isTerminal && s.winner === "O") cardClass += " term-loss";

  var html = '<div class="' + cardClass + '">';

  html += '<div class="move-card-header">';
  html += '<span class="' + (s.player === "X" ? "x-move" : "o-move") + '">';
  html += s.player + " plays cell " + s.cell;
  html += '</span>';
  html += '</div>';

  var qVals = null;
  if (s.player === "X" && s.qValues) {
    qVals = s.qValues;
  }
  html += renderMiniBoard(s.boardBefore, s.cell, qVals);
  html += renderTermOutcome(s);

  if (s.player === "X" && s.qInfo) {
    html += renderQUpdate(s.qInfo);
  }

  html += '</div>';
  return html;
}

function renderSnapRow(snaps) {
  var html = '<div class="episode-row">';
  for (var i = 0; i < snaps.length; i++) {
    if (i > 0) {
      html += '<div class="episode-arrow">\u2192</div>';
    }
    html += renderMoveCard(snaps[i]);
  }
  html += '</div>';
  return html;
}

function renderEpisode(epData) {
  var snaps = epData.snapshots;
  var html = '<div class="episode-label">' + epData.label + '</div>';
  html += renderSnapRow(snaps);
  return html;
}

function renderChapter(chData) {
  var html = '<h2 class="chapter-heading">' + chData.title + '</h2>';
  for (var ei = 0; ei < chData.episodes.length; ei++) {
    html += renderEpisode(chData.episodes[ei]);
  }
  return html;
}

function renderAll(chapters) {
  var container = document.getElementById("episodes");
  var html = "";
  for (var ci = 0; ci < chapters.length; ci++) {
    html += renderChapter(chapters[ci]);
  }
  container.innerHTML = html;
}

// ---- init ----------------------------------------------------------------

function init() {
  var chapterResults = replayAllEpisodes(CHAPTERS);
  renderAll(chapterResults);
}

if (document.readyState === "loading") {
  document.addEventListener("DOMContentLoaded", init);
} else {
  init();
}

})();
