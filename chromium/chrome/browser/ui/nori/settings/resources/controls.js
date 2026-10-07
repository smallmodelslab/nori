// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

// A row for each key in the file. Rows only ever produce TOML literals; the
// browser decides whether they are valid.

function element(tag, properties = {}, ...children) {
  const node = document.createElement(tag);
  Object.assign(node, properties);
  node.append(...children);
  return node;
}

// The text of a TOML string literal, or the literal itself if it is not one.
function stringOf(literal) {
  if (literal.startsWith('\'')) {
    return literal.slice(1, -1);
  }
  try {
    const value = JSON.parse(literal);
    return typeof value === 'string' ? value : literal;
  } catch {
    return literal;
  }
}

const quote = text => JSON.stringify(text);

const SLIDER_STEPS = 1000;

// Rounds to `digits` significant digits, without float noise.
function roundTo(value, digits) {
  return value === 0 ? 0 : Number(value.toPrecision(digits));
}

// Maps a slider position in [0, SLIDER_STEPS] to a value and back. Ranges
// that span orders of magnitude, like a spring's stiffness, are logarithmic
// so that the low end isn't crammed into a few pixels.
function scaleFor({min, max, type}) {
  const logarithmic = min > 0 && max / min >= 100;
  const toValue = logarithmic ?
      t => min * (max / min) ** (t / SLIDER_STEPS) :
      t => min + (max - min) * t / SLIDER_STEPS;
  const toPosition = logarithmic ?
      v => SLIDER_STEPS * Math.log(v / min) / Math.log(max / min) :
      v => SLIDER_STEPS * (v - min) / (max - min);
  const round = type === 'integer' ? Math.round :
      logarithmic                  ? v => roundTo(v, 2) :
                                     v => roundTo(v, 3);
  return {
    toValue: t => Math.min(max, Math.max(min, round(toValue(t)))),
    toPosition: v => Math.min(SLIDER_STEPS, Math.max(0, toPosition(v))),
  };
}

function rangeControl(setting, onLiteral) {
  const scale = scaleFor(setting);
  const input = element(
      'input', {type: 'range', min: 0, max: SLIDER_STEPS, step: 'any'});
  const output = element('output');
  // The slider's own value is its position, so screen readers get the
  // setting's value instead.
  const show = literal => {
    output.value = literal;
    input.ariaValueText = literal;
  };
  input.addEventListener('input', () => {
    const literal = String(scale.toValue(input.valueAsNumber));
    show(literal);
    onLiteral(literal);
  });
  return {
    element: element('div', {className: 'control'}, input, output),
    input,
    setLiteral(literal) {
      const value = Number(literal);
      if (Number.isFinite(value)) {
        input.value = scale.toPosition(value);
      }
      show(literal);
    },
  };
}

function choiceControl({options}, onLiteral) {
  const select = element(
      'select', {},
      ...options.map(option => element('option', {value: option}, option)));
  select.addEventListener('change', () => onLiteral(quote(select.value)));
  return {
    element: element('div', {className: 'control'}, select),
    input: select,
    setLiteral(literal) {
      select.value = stringOf(literal);
    },
  };
}

// Key names shortcut.cc accepts, by KeyboardEvent.code. Shortcuts name
// physical keys, as codes do, so they read the same on any layout.
const KEY_NAMES = {
  Enter: 'enter',
  Tab: 'tab',
  Space: 'space',
  Escape: 'escape',
  Backspace: 'backspace',
  Delete: 'delete',
  ArrowLeft: 'left',
  ArrowRight: 'right',
  ArrowUp: 'up',
  ArrowDown: 'down',
  Home: 'home',
  End: 'end',
  PageUp: 'pageup',
  PageDown: 'pagedown',
  Comma: 'comma',
  Period: 'period',
  Slash: 'slash',
  Semicolon: 'semicolon',
  Quote: 'quote',
  Minus: 'minus',
  Equal: 'equal',
  BracketLeft: 'bracketleft',
  BracketRight: 'bracketright',
  Backslash: 'backslash',
  Backquote: 'backquote',
};

function keyName(code) {
  if (/^Key[A-Z]$/.test(code)) {
    return code.slice(3).toLowerCase();
  }
  if (/^Digit\d$/.test(code)) {
    return code.slice(5);
  }
  if (/^F\d{1,2}$/.test(code)) {
    return code.toLowerCase();
  }
  return KEY_NAMES[code] ?? null;
}

function shortcutOf(event) {
  const key = keyName(event.code);
  if (!key) {
    return null;
  }
  const modifiers = [
    event.ctrlKey && 'ctrl',
    event.altKey && 'alt',
    event.shiftKey && 'shift',
    event.metaKey && 'cmd',
  ].filter(Boolean);
  return [...modifiers, key].join('+');
}

// Click, then press the keys. Escape on its own cancels.
function shortcutControl(_setting, onLiteral) {
  const button = element('button', {className: 'shortcut', type: 'button'});
  let shortcut = '';
  const stopRecording = () => {
    button.classList.remove('recording');
    button.textContent = shortcut;
  };
  button.addEventListener('click', () => {
    button.classList.add('recording');
    button.textContent = 'Press keys…';
  });
  button.addEventListener('blur', stopRecording);
  button.addEventListener('keydown', event => {
    if (!button.classList.contains('recording')) {
      return;
    }
    event.preventDefault();
    event.stopPropagation();
    const pressed = shortcutOf(event);
    if (pressed === 'escape') {
      stopRecording();
    } else if (pressed) {
      shortcut = pressed;
      stopRecording();
      onLiteral(quote(pressed));
    }
  });
  return {
    element: element('div', {className: 'control'}, button),
    input: button,
    setLiteral(literal) {
      shortcut = stringOf(literal);
      if (!button.classList.contains('recording')) {
        button.textContent = shortcut;
      }
    },
  };
}

const CONTROLS = {
  number: rangeControl,
  integer: rangeControl,
  choice: choiceControl,
  shortcut: shortcutControl,
};

// A row for the key at `node.path`, which calls `onLiteral(path, literal)`
// when its control changes and `onSelect(line)` when its label is clicked.
// Call update() with each new version of the node.
export function createRow(node, {onLiteral, onSelect}) {
  const {path, setting} = node;
  const description = element('span');
  const label = element(
      'label', {}, description, ' ', element('span', {className: 'path'}, path));
  const row = element('div', {className: 'row'}, label);
  const control =
      setting && CONTROLS[setting.type](setting, l => onLiteral(path, l));
  if (control) {
    control.input.ariaLabel = path;
    row.append(control.element);
  } else {
    row.classList.add('unknown');
  }

  let line = node.line;
  label.addEventListener('click', () => onSelect(line));

  const update = next => {
    line = next.line;
    row.dataset.line = line;
    row.style.top = `calc(${line - 1} * var(--line))`;
    row.classList.toggle('invalid', Boolean(next.error));
    description.textContent =
        next.error ?? setting?.description ?? 'Not a nori setting';
    label.title = [setting?.description, next.error].filter(Boolean).join('\n');
    // Leaves a control that is in use alone, so that the browser echoing an
    // older value back can't move a slider out from under the pointer.
    if (control && !row.contains(document.activeElement)) {
      control.setLiteral(next.literal);
    }
  };
  update(node);
  return {element: row, update};
}
