// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

// Colours TOML one line at a time. It only has to look right: the browser
// parses the file and reports errors itself.

const TOKENS = [
  ['comment', /#.*/y],
  ['string', /"""|'''|"(?:[^"\\]|\\.)*"?|'[^']*'?/y],
  ['bool', /(?:true|false)(?![\w-])/y],
  ['number',
   /[+-]?(?:inf|nan|0x[\da-fA-F_]+|0o[0-7_]+|0b[01_]+|\d[\d_]*(?:\.\d[\d_]*)?(?:[eE][+-]?\d+)?)(?![\w-])/y],
  ['punct', /[=[\]{},.]/y],
  ['bare', /[\w-]+/y],
  ['space', /\s+/y],
  ['text', /./y],
];

const TABLE = /^(\s*)(\[\[?[^\]]*\]?\]?)(.*)$/;

// Splits `line` into [kind, text] pairs, where kind is a TOKENS name or
// "table" or "key".
export function tokenize(line) {
  const table = TABLE.exec(line);
  if (table) {
    return [
      ['space', table[1]],
      ['table', table[2]],
      ...tokenize(table[3]),
    ].filter(([, text]) => text);
  }

  const tokens = [];
  let inValue = false;
  for (let at = 0; at < line.length;) {
    for (const [kind, pattern] of TOKENS) {
      pattern.lastIndex = at;
      const match = pattern.exec(line);
      if (!match) {
        continue;
      }
      const text = match[0];
      inValue ||= text === '=';
      const isKey = !inValue && (kind === 'bare' || kind === 'string');
      tokens.push([isKey ? 'key' : kind, text]);
      at += text.length;
      break;
    }
  }
  return tokens;
}

// One block per line of `text`, so that lines can be marked. Empty blocks
// keep their height through the stylesheet's min-height.
export function renderLines(text) {
  return text.split('\n').map(line => {
    const block = document.createElement('div');
    block.className = 'line';
    for (const [kind, token] of tokenize(line)) {
      if (kind === 'space' || kind === 'text') {
        block.append(token);
        continue;
      }
      const span = document.createElement('span');
      span.className = `tok-${kind}`;
      span.textContent = token;
      block.append(span);
    }
    return block;
  });
}
