// Copyright 2026 The Nori Authors. GPL-3.0; see LICENSE.
if (navigator.platform.includes('Mac')) {
  document.querySelector('#palette-key').textContent = '⌘ B';
  document.querySelector('#close-key').textContent = '⌘ W';
}
document.querySelector('#palette').addEventListener('click', () => {
  chrome.send('openCommandPalette');
});
