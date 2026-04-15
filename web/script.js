// BoardNova - Workboard Logic
const canvas = document.getElementById('whiteboard');
const ctx = canvas.getContext('2d');
const colorPicker = document.getElementById('colorPicker');
const sizePicker = document.getElementById('sizePicker');
const sizeVal = document.getElementById('sizeVal');
const penTool = document.getElementById('penTool');
const eraserTool = document.getElementById('eraserTool');
const displayUsername = document.getElementById('displayUsername');
const connectionStatus = document.getElementById('connectionStatus');

let drawing = false;
let currentMode = 'pen';
let history = [];
let step = -1;

// --- Initialize ---
window.onload = () => {
    const user = localStorage.getItem('boardnova_user');
    if (!user) {
        window.location.href = 'login.html';
        return;
    }
    displayUsername.innerText = user;

    resizeCanvas();
    saveState();
};

function resizeCanvas() {
    // Make canvas white/clean
    canvas.width = 1200;
    canvas.height = 800;
    ctx.lineCap = 'round';
    ctx.lineJoin = 'round';
    ctx.fillStyle = 'white';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
}

// --- Drawing Logic ---
function startDrawing(e) {
    drawing = true;
    draw(e);
}

function stopDrawing() {
    if (drawing) {
        drawing = false;
        ctx.beginPath();
        saveState();
        sendToServer('draw');
    }
}

function draw(e) {
    if (!drawing) return;

    const rect = canvas.getBoundingClientRect();
    const x = (e.clientX || e.touches[0].clientX) - rect.left;
    const y = (e.clientY || e.touches[0].clientY) - rect.top;

    ctx.lineWidth = sizePicker.value;
    ctx.strokeStyle = currentMode === 'eraser' ? '#FFFFFF' : colorPicker.value;

    ctx.lineTo(x, y);
    ctx.stroke();
    ctx.beginPath();
    ctx.moveTo(x, y);
}

canvas.addEventListener('mousedown', startDrawing);
canvas.addEventListener('mousemove', draw);
canvas.addEventListener('mouseup', stopDrawing);
canvas.addEventListener('touchstart', (e) => { e.preventDefault(); startDrawing(e); });
canvas.addEventListener('touchmove', (e) => { e.preventDefault(); draw(e); });
canvas.addEventListener('touchend', stopDrawing);

// --- Toolbar Controls ---
penTool.onclick = () => {
    currentMode = 'pen';
    penTool.classList.add('active');
    eraserTool.classList.remove('active');
};

eraserTool.onclick = () => {
    currentMode = 'eraser';
    eraserTool.classList.add('active');
    penTool.classList.remove('active');
};

sizePicker.oninput = () => {
    sizeVal.innerText = sizePicker.value;
};

// --- History / Undo ---
function saveState() {
    step++;
    if (step < history.length) history.length = step;
    history.push(canvas.toDataURL());
}

function undo() {
    if (step > 0) {
        step--;
        let canvasPic = new Image();
        canvasPic.src = history[step];
        canvasPic.onload = () => {
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            ctx.drawImage(canvasPic, 0, 0);
        };
        sendToServer('undo');
    }
}

function clearCanvas() {
    ctx.fillStyle = 'white';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    saveState();
    sendToServer('clear');
}

function saveCanvas() {
    const link = document.createElement('a');
    link.download = `boardnova_${Date.now()}.png`;
    link.href = canvas.toDataURL();
    link.click();
    sendToServer('save');
}

// --- Server Sync ---
async function sendToServer(action) {
    const user = localStorage.getItem('boardnova_user');
    try {
        const response = await fetch(`http://localhost:8080/${action}`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ action: action, user: user, timestamp: Date.now() })
        });
        
        if (response.ok) {
            connectionStatus.innerText = "Connected to Server";
            document.querySelector('.status-dot').style.backgroundColor = '#10b981';
            document.querySelector('.status-dot').style.boxShadow = '0 0 8px #10b981';
        }
    } catch (err) {
        connectionStatus.innerText = "Server Offline";
        document.querySelector('.status-dot').style.backgroundColor = '#f87171';
        document.querySelector('.status-dot').style.boxShadow = '0 0 8px #f87171';
    }
}

function logout() {
    localStorage.removeItem('boardnova_user');
    window.location.href = 'login.html';
}

// Expose functions to buttons
document.getElementById('logoutBtn').onclick = logout;
document.getElementById('undoBtn').onclick = undo;
document.getElementById('clearBtn').onclick = clearCanvas;
document.getElementById('saveBtn').onclick = saveCanvas;
