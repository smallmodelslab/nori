// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

// chrome://nori-settings. The textarea holds the file; the browser parses
// it and sends back a document (see settings/document_value.h) whose node
// lines place the controls beside the text.

import {addWebUiListener, sendWithPromise} from 'chrome://resources/js/cr.js';

import {createRow} from './controls.js';
import {renderLines} from './highlight.js';

const SAVE_DELAY_MS = 400;

const $ = id => document.getElementById(id);
const text = $('text');
const highlight = $('highlight');
const controls = $('controls');
const banner = $('banner');
const status = $('status');

const rows = new Map();
let saveTimer = 0;
let statusTimer = 0;

function showStatus(message) {
  status.textContent = message;
  clearTimeout(statusTimer);
  statusTimer = setTimeout(() => status.textContent = '', 2000);
}

function lineCount(value) {
  return value.split('\n').length;
}

// The line with the caret, or 0 when the editor is not focused.
function caretLine() {
  return document.activeElement === text ?
      lineCount(text.value.slice(0, text.selectionStart)) :
      0;
}

// Marks `line` in both columns.
function markCurrent(line) {
  for (const [index, block] of [...highlight.children].entries()) {
    block.classList.toggle('current', index + 1 === line);
  }
  for (const row of rows.values()) {
    row.element.classList.toggle(
        'current', row.element.dataset.line === String(line));
  }
}

let invalidLines = new Set();

function renderText() {
  highlight.replaceChildren(...renderLines(text.value));
  for (const line of invalidLines) {
    highlight.children[line - 1]?.classList.add('invalid');
  }
  markCurrent(caretLine());
}

// Replaces the textarea's text, keeping the caret where it was.
function setText(value) {
  if (text.value === value) {
    return;
  }
  const {selectionStart, selectionEnd} = text;
  text.value = value;
  text.setSelectionRange(selectionStart, selectionEnd);
}

// Puts the caret just after the `=` on `line`.
function selectLine(line) {
  const lines = text.value.split('\n');
  const start = lines.slice(0, line - 1).reduce((n, l) => n + l.length + 1, 0);
  const caret = start + (lines[line - 1] ?? '').indexOf('=') + 1;
  text.focus();
  text.setSelectionRange(caret, caret);
  markCurrent(line);
}

// Runs `task` with the latest arguments, one call at a time, dropping the
// ones a slider sends while the browser is still busy with the last.
function latest(task) {
  let pending = null;
  let running = false;
  return async (...args) => {
    pending = args;
    if (running) {
      return;
    }
    running = true;
    while (pending) {
      const next = pending;
      pending = null;
      await task(...next);
    }
    running = false;
  };
}

const setValue = latest(async (path, literal) => {
  clearTimeout(saveTimer);
  saveTimer = 0;
  const edited = await sendWithPromise('setValue', text.value, path, literal);
  if (edited === null) {
    showStatus(`Couldn't set ${path} to ${literal}`);
    return;
  }
  setText(edited);
  renderText();
});

function renderControls(nodes) {
  const seen = new Set();
  for (const node of nodes) {
    seen.add(node.path);
    let row = rows.get(node.path);
    if (!row) {
      row = createRow(node, {onLiteral: setValue, onSelect: selectLine});
      rows.set(node.path, row);
      controls.append(row.element);
    }
    row.update(node);
  }
  for (const [path, row] of rows) {
    if (!seen.has(path)) {
      row.element.remove();
      rows.delete(path);
    }
  }
}

function renderDocument(doc) {
  $('file').textContent = doc.file;
  // Typing that has not been saved yet wins; saving it brings a newer
  // document.
  if (!saveTimer) {
    setText(doc.text);
  }

  const error = doc.syntaxError;
  banner.hidden = !error;
  banner.textContent =
      error ? `Line ${error.line}, column ${error.column}: ${error.message}` :
              '';
  invalidLines = new Set([
    ...doc.nodes.filter(node => node.error).map(node => node.line),
    ...(error ? [error.line] : []),
  ]);

  renderControls(doc.nodes);
  renderText();
}

text.addEventListener('input', () => {
  renderText();
  clearTimeout(saveTimer);
  saveTimer = setTimeout(() => {
    saveTimer = 0;
    chrome.send('setText', [text.value]);
  }, SAVE_DELAY_MS);
});

// Keeps Tab in the editor, as two spaces.
text.addEventListener('keydown', event => {
  if (event.key === 'Tab' && !event.shiftKey && !event.ctrlKey &&
      !event.metaKey && !event.altKey) {
    event.preventDefault();
    document.execCommand('insertText', false, '  ');
  }
});

document.addEventListener('selectionchange', () => markCurrent(caretLine()));
text.addEventListener('blur', () => markCurrent(0));

addWebUiListener('document-changed', renderDocument);
sendWithPromise('getDocument').then(renderDocument);
