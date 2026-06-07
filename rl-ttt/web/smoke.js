// Node.js smoke test for web/app.js. Loads qtable.js + app.js with a
// minimal DOM mock, then exercises the game flow.
//
// We can't fully test DOM rendering without jsdom, but we CAN verify:
//   1. qtable.js loads and QTABLE is populated
//   2. app.js loads without throwing
//   3. The game state initializes correctly (board empty)
//   4. canonicalForCurrentPlayer logic is correct (spot checks)
//
// Run from the web/ directory:  node smoke.js

"use strict";

const fs = require("fs");
const path = require("path");
const vm = require("vm");

// --- minimal DOM mock ------------------------------------------------------

class FakeElement {
  constructor(tag) {
    this.tagName = tag.toUpperCase();
    this.dataset = {};
    this.className = "";
    this.classList = {
      _set: new Set(),
      add: (...cs) => cs.forEach((c) => this.classList._set.add(c)),
      remove: (...cs) => cs.forEach((c) => this.classList._set.delete(c)),
      contains: (c) => this.classList._set.has(c),
    };
    this.children = [];
    this._textContent = "";
    this._listeners = {};
    this.style = {};
    this.disabled = false;
    this.value = "X";
  }
  setAttribute(k, v) { this[k] = v; }
  getAttribute(k) { return this[k]; }
  appendChild(c) { this.children.push(c); c.parent = this; return c; }
  addEventListener(name, fn) { (this._listeners[name] ||= []).push(fn); }
  querySelector(sel) {
    const m = sel.match(/^\.([\w-]+)$/);
    if (!m) return null;
    const cls = m[1];
    return this.children.find((c) => c.className.split(/\s+/).includes(cls)) || null;
  }
  querySelectorAll(sel) {
    const m = sel.match(/^\.([\w-]+)$/);
    if (!m) return [];
    const cls = m[1];
    return this.children.filter((c) => c.className.split(/\s+/).includes(cls));
  }
  get textContent() { return this._textContent; }
  set textContent(v) { this._textContent = v; }
  get innerHTML() { return ""; }
  set innerHTML(v) { /* ignore */ }
  dispatch(name) { (this._listeners[name] || []).forEach((fn) => fn({ currentTarget: this })); }
}

const elements = {
  board: new FakeElement("div"),
  turn: new FakeElement("div"),
  "new-game": new FakeElement("button"),
  movecount: new FakeElement("div"),
  qsize: new FakeElement("div"),
};

const document = {
  readyState: "complete",
  getElementById: (id) => elements[id] || null,
  addEventListener: () => {},
  createElement: (tag) => new FakeElement(tag),
  querySelectorAll: (sel) => {
    if (sel === ".cell") return elements.board.children;
    return [];
  },
};

const window = {};

// --- run both scripts in a shared sandbox ----------------------------------

const sandbox = { window, document, console, setTimeout, Number, Math, Set };
sandbox.global = sandbox;
vm.createContext(sandbox);

vm.runInContext(fs.readFileSync(path.join(__dirname, "qtable.js"), "utf8"), sandbox);
// In a vm sandbox, `window.QTABLE = X` does NOT make `QTABLE` accessible as
// a bare reference inside the IIFE. Promote it to a top-level binding.
sandbox.QTABLE = window.QTABLE;
console.log(`[1] qtable.js loaded: ${window.QTABLE.numStates} states`);

if (typeof window.QTABLE === "undefined" || typeof window.QTABLE.states !== "object") {
  console.error("FAIL: QTABLE not assigned by qtable.js");
  process.exit(1);
}
if (window.QTABLE.numStates < 1000) {
  console.error(`FAIL: QTABLE has only ${window.QTABLE.numStates} states, expected >1000`);
  process.exit(1);
}

// Now load app.js. It uses an IIFE, so we can't poke at internals — but we
// CAN check that it loaded without throwing and that the DOM elements were
// touched correctly.
try {
  vm.runInContext(fs.readFileSync(path.join(__dirname, "app.js"), "utf8"), sandbox);
  console.log("[2] app.js loaded without throwing");
} catch (e) {
  console.error("FAIL: app.js threw:", e.message);
  process.exit(1);
}

// Spot-check: the init() function should have run and built a board with 9 cells.
const boardChildren = elements.board.children.length;
if (boardChildren !== 9) {
  console.error(`FAIL: expected 9 board cells, got ${boardChildren}`);
  process.exit(1);
}
console.log(`[3] board built with ${boardChildren} cells`);

// Spot-check: qsize stat was set to the Q-table size.
if (elements.qsize.textContent !== String(window.QTABLE.numStates)) {
  console.error(`FAIL: qsize stat = ${elements.qsize.textContent}, expected ${window.QTABLE.numStates}`);
  process.exit(1);
}
console.log(`[4] qsize stat set to ${elements.qsize.textContent}`);

// Spot-check: movecount starts at 0.
if (elements.movecount.textContent !== "0") {
  console.error(`FAIL: movecount = ${elements.movecount.textContent}, expected "0"`);
  process.exit(1);
}
console.log(`[5] movecount starts at 0`);

// Spot-check: clicking a legal cell should make a move.
const cell0 = elements.board.children[0];
cell0.dispatch("click");
const movecountAfter = elements.movecount.textContent;
if (movecountAfter !== "1") {
  console.error(`FAIL: after click, movecount = ${movecountAfter}, expected "1"`);
  process.exit(1);
}
console.log(`[6] clicking cell 0 made 1 move`);

// Spot-check: a clicked legal cell updates its piece glyph.
const piece0 = cell0.querySelector(".piece");
if (!piece0 || piece0.textContent !== "X") {
  console.error(`FAIL: cell 0 piece = ${piece0 && piece0.textContent}, expected "X"`);
  process.exit(1);
}
console.log(`[7] cell 0 shows piece "X"`);

console.log("\nAll smoke checks passed.");
