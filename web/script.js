'use strict';

// ── Canvas setup ──────────────────────────────────────────
const canvas  = document.getElementById('whiteboard');
const ctx     = canvas.getContext('2d');
const hint    = document.querySelector('.hint');

function resizeCanvas() {
  // Preserve drawing when resizing
  const imgData = ctx.getImageData(0, 0, canvas.width, canvas.height);
  canvas.width  = canvas.offsetWidth;
  canvas.height = canvas.offsetHeight;
  // Restore dark background
  ctx.fillStyle = '#0a0a12';
  ctx.fillRect(0, 0, canvas.width, canvas.height);
  ctx.putImageData(imgData, 0, 0);
}
window.addEventListener('resize', resizeCanvas);
resizeCanvas();

// ── State ─────────────────────────────────────────────────
let drawing    = false;
let tool       = 'pen';   // 'pen' | 'eraser'
let color      = '#00ffe7';
let lineWidth  = 4;
let lastX      = 0;
let lastY      = 0;

// Undo stack – store canvas snapshots
const undoStack = [];
const MAX_UNDO  = 30;

function saveSnapshot() {
  if (undoStack.length >= MAX_UNDO) undoStack.shift();
  undoStack.push(ctx.getImageData(0, 0, canvas.width, canvas.height));
}

// ── Tool controls ─────────────────────────────────────────
document.getElementById('toolPen').addEventListener('click', () => {
  tool = 'pen';
  document.getElementById('toolPen').classList.add('active');
  document.getElementById('toolEraser').classList.remove('active');
  canvas.style.cursor = 'crosshair';
});

document.getElementById('toolEraser').addEventListener('click', () => {
  tool = 'eraser';
  document.getElementById('toolEraser').classList.add('active');
  document.getElementById('toolPen').classList.remove('active');
  canvas.style.cursor = 'cell';
});

document.getElementById('colorPicker').addEventListener('input', e => {
  color = e.target.value;
});

const sizeSlider  = document.getElementById('sizeSlider');
const sizeDisplay = document.getElementById('sizeDisplay');
sizeSlider.addEventListener('input', e => {
  lineWidth = parseInt(e.target.value, 10);
  sizeDisplay.textContent = lineWidth + 'px';
});

document.getElementById('btnClear').addEventListener('click', () => {
  saveSnapshot();
  ctx.fillStyle = '#0a0a12';
  ctx.fillRect(0, 0, canvas.width, canvas.height);
});

document.getElementById('btnUndo').addEventListener('click', () => {
  if (undoStack.length === 0) return;
  ctx.putImageData(undoStack.pop(), 0, 0);
});

document.getElementById('btnSave').addEventListener('click', () => {
  const link    = document.createElement('a');
  link.download = 'boardnova_whiteboard.png';
  link.href     = canvas.toDataURL('image/png');
  link.click();
});

// ── Drawing ───────────────────────────────────────────────
function getPos(e) {
  const rect = canvas.getBoundingClientRect();
  if (e.touches) {
    return {
      x: e.touches[0].clientX - rect.left,
      y: e.touches[0].clientY - rect.top
    };
  }
  return { x: e.clientX - rect.left, y: e.clientY - rect.top };
}

function startDraw(e) {
  e.preventDefault();
  drawing = true;
  const { x, y } = getPos(e);
  lastX = x; lastY = y;
  saveSnapshot();
  // Hide hint on first draw
  hint.classList.add('hidden');
}

function draw(e) {
  e.preventDefault();
  if (!drawing) return;
  const { x, y } = getPos(e);

  ctx.beginPath();
  ctx.moveTo(lastX, lastY);
  ctx.lineTo(x, y);
  ctx.strokeStyle = (tool === 'eraser') ? '#0a0a12' : color;
  ctx.lineWidth   = (tool === 'eraser') ? lineWidth * 4 : lineWidth;
  ctx.lineCap     = 'round';
  ctx.lineJoin    = 'round';
  ctx.globalAlpha = (tool === 'eraser') ? 1 : 0.92;
  ctx.stroke();

  // Glow for pen
  if (tool === 'pen') {
    ctx.shadowColor = color;
    ctx.shadowBlur  = 8;
    ctx.stroke();
    ctx.shadowBlur  = 0;
  }

  lastX = x; lastY = y;
}

function stopDraw(e) {
  e.preventDefault();
  drawing = false;
  ctx.globalAlpha = 1;
}

// Mouse events
canvas.addEventListener('mousedown',  startDraw);
canvas.addEventListener('mousemove',  draw);
canvas.addEventListener('mouseup',    stopDraw);
canvas.addEventListener('mouseleave', stopDraw);

// Touch events
canvas.addEventListener('touchstart', startDraw, { passive: false });
canvas.addEventListener('touchmove',  draw,      { passive: false });
canvas.addEventListener('touchend',   stopDraw,  { passive: false });
