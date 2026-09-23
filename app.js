import { Chessground } from './vendor/chessground.js';
import { Chess } from './vendor/chess.js';

const $ = id => document.getElementById(id);
const START_FEN = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1';
const MAX_THREADS = Math.min(navigator.hardwareConcurrency || 4, 32); // matches PTHREAD_POOL_SIZE

// ---------------------------------------------------------------------------------------------
// Engine

let worker = null;
let engineReady = false;
// Every "go" produces exactly one "bestmove", in order; stale results are ignored by tag.
const goQueue = [];
let searchTag = 0;
let onBestMove = null;

function uci(cmd) { worker.postMessage(cmd); }

function startEngine() {
    if (!window.crossOriginIsolated || typeof SharedArrayBuffer === 'undefined') {
        setStatus('This page needs cross-origin isolation for multithreading. Reload the page once; ' +
            'if it still fails, your browser does not support SharedArrayBuffer.', true);
        return;
    }
    worker = new Worker('engine-worker.js');
    worker.onmessage = e => handleLine(e.data);
    worker.onerror = e => setStatus('Engine failed to start: ' + e.message, true);
    uci('uci');
    applyOptions();
    uci('isready');
}

function applyOptions() {
    uci('setoption name Threads value ' + $('threads').value);
    uci('setoption name Hash value ' + $('hash').value);
    uci('setoption name UseNNUE value ' + $('nnue').value);
    $('threads-info').textContent = `${evalName()}, ${$('threads').value} thread${$('threads').value === '1' ? '' : 's'}, ${$('hash').value} MB`;
}

function evalName() {
    return $('nnue').value === 'true' ? 'NNUE' : 'Classical';
}

function handleLine(line) {
    if (line.startsWith('wombat error')) return setStatus(line, true);
    if (line === 'readyok' && !engineReady) {
        engineReady = true;
        update();
        return;
    }
    if (line.startsWith('info ') && line.includes(' pv ')) return showInfo(line);
    if (line.startsWith('bestmove')) {
        const tag = goQueue.shift();
        if (tag === searchTag && onBestMove) {
            const cb = onBestMove;
            onBestMove = null;
            cb(line.split(' ')[1]);
        }
    }
}

function search(goArgs, callback) {
    stopSearch();
    searchTag++;
    goQueue.push(searchTag);
    onBestMove = callback;
    thinkingFen = game.fen();
    clearInfo();
    uci(`position fen ${startFen} moves ${history.map(m => m.lan).join(' ')}`.trim());
    uci('go ' + goArgs);
}

function stopSearch() {
    if (goQueue.length && goQueue[goQueue.length - 1] === searchTag) uci('stop');
    searchTag++;
    onBestMove = null;
}

// ---------------------------------------------------------------------------------------------
// Engine info display

let thinkingFen = null;

function clearInfo() {
    for (const id of ['st-depth', 'st-score', 'st-nps', 'st-nodes']) $(id).textContent = '–';
    $('pv').textContent = '';
}

function showInfo(line) {
    if (thinkingFen !== game.fen()) return;
    const tok = line.split(' ');
    const get = key => tok[tok.indexOf(key) + 1];
    const whiteToMove = game.turn() === 'w';

    let scoreText, whiteCp;
    const si = tok.indexOf('score');
    if (tok[si + 1] === 'mate') {
        const m = +tok[si + 2];
        const whiteMate = (m > 0) === whiteToMove;
        scoreText = (whiteMate ? '+' : '-') + 'M' + Math.abs(m);
        whiteCp = whiteMate ? 10000 : -10000;
    } else {
        const cp = +tok[si + 2];
        whiteCp = whiteToMove ? cp : -cp;
        scoreText = (whiteCp > 0 ? '+' : '') + (whiteCp / 100).toFixed(2);
    }

    $('st-depth').textContent = get('depth') + '/' + get('seldepth');
    $('st-score').textContent = scoreText;
    $('st-nps').textContent = formatCount(+get('nps')) + 'n/s';
    $('st-nodes').textContent = formatCount(+get('nodes'));
    $('pv').textContent = pvToSan(tok.slice(tok.indexOf('pv') + 1));
    setEvalBar(whiteCp, scoreText);
}

function formatCount(n) {
    if (n >= 1e9) return (n / 1e9).toFixed(1) + 'G';
    if (n >= 1e6) return (n / 1e6).toFixed(1) + 'M';
    if (n >= 1e3) return (n / 1e3).toFixed(0) + 'k';
    return String(n);
}

function pvToSan(moves) {
    const g = new Chess(game.fen());
    const out = [];
    for (const m of moves) {
        const moveNo = g.moveNumber();
        let san;
        try { san = g.move(m).san; } catch { break; }
        if (g.turn() === 'b') out.push(moveNo + '. ' + san);
        else out.push((out.length ? '' : moveNo + '… ') + san);
    }
    return out.join(' ');
}

function setEvalBar(whiteCp, label) {
    // Logistic mapping, like lichess
    const white = 50 + 50 * (2 / (1 + Math.exp(-0.004 * whiteCp)) - 1);
    $('evalbar-fill').style.height = white + '%';
    $('evalbar-label').textContent = label.replace(/^\+/, '').replace(/\.(\d)\d$/, '.$1');
}

// ---------------------------------------------------------------------------------------------
// Game state

let game = new Chess();
let startFen = START_FEN;
let history = []; // chess.js move objects
let humanColor = 'white';
let orientation = 'white';

const board = Chessground($('board'), {
    fen: START_FEN,
    animation: { duration: 180 },
    movable: { free: false, showDests: true, events: { after: onUserMove } },
    premovable: { enabled: false },
    draggable: { showGhost: true },
    highlight: { lastMove: true, check: true },
});

function mode() { return $('mode').value; }

function dests() {
    const d = new Map();
    for (const m of game.moves({ verbose: true })) {
        if (!d.has(m.from)) d.set(m.from, []);
        d.get(m.from).push(m.to);
    }
    return d;
}

function humanToMove() {
    if (game.isGameOver()) return false;
    if (mode() === 'analyze') return true;
    return (game.turn() === 'w') === (humanColor === 'white');
}

function update() {
    const turnColor = game.turn() === 'w' ? 'white' : 'black';
    const last = history[history.length - 1];
    board.set({
        fen: game.fen(),
        turnColor,
        orientation,
        check: game.inCheck(),
        lastMove: last ? [last.from, last.to] : undefined,
        movable: {
            color: humanToMove() ? turnColor : undefined,
            dests: humanToMove() ? dests() : new Map(),
        },
    });
    $('evalbar').classList.toggle('flipped', orientation === 'black');
    $('fen').value = game.fen();
    renderMoves();
    renderPlayers();

    if (!engineReady) return;

    if (game.isGameOver()) {
        stopSearch();
        setStatus(gameOverText());
    } else if (mode() === 'analyze') {
        setStatus('Analysing… move for either side.');
        search('infinite', null);
    } else if (!humanToMove()) {
        setStatus('Wombat is thinking…');
        search('movetime ' + $('movetime').value, uciMove => {
            playMove({ from: uciMove.slice(0, 2), to: uciMove.slice(2, 4), promotion: uciMove[4] });
        });
    } else {
        setStatus('Your move.');
    }
}

function gameOverText() {
    if (game.isCheckmate()) return `Checkmate — ${game.turn() === 'w' ? 'Black' : 'White'} wins.`;
    if (game.isStalemate()) return 'Draw by stalemate.';
    if (game.isThreefoldRepetition()) return 'Draw by threefold repetition.';
    if (game.isInsufficientMaterial()) return 'Draw by insufficient material.';
    return 'Draw by the fifty-move rule.';
}

function playMove(m) {
    const move = game.move(m);
    history.push(move);
    update();
}

function onUserMove(from, to) {
    const piece = game.get(from);
    const promo = piece && piece.type === 'p' && (to[1] === '8' || to[1] === '1');
    if (promo) askPromotion(piece.color, p => playMove({ from, to, promotion: p }));
    else playMove({ from, to });
}

function askPromotion(color, done) {
    const box = $('promotion');
    const glyphs = color === 'w' ? { q: '♕', r: '♖', b: '♗', n: '♘' } : { q: '♛', r: '♜', b: '♝', n: '♞' };
    box.replaceChildren(...Object.entries(glyphs).map(([p, g]) => {
        const b = document.createElement('button');
        b.textContent = g;
        b.style.fontSize = 'min(8vw, 52px)';
        b.onclick = () => { box.hidden = true; done(p); };
        return b;
    }));
    box.hidden = false;
}

function renderMoves() {
    const ol = $('moves');
    ol.replaceChildren();
    const firstBlack = new Chess(startFen).turn() === 'b';
    const plies = history.map(m => m.san);
    if (firstBlack) plies.unshift('…');
    ol.start = new Chess(startFen).moveNumber();
    for (let i = 0; i < plies.length; i += 2) {
        const li = document.createElement('li');
        for (const san of plies.slice(i, i + 2)) {
            const s = document.createElement('span');
            s.textContent = san;
            li.append(s);
        }
        ol.append(li);
    }
    ol.scrollTop = ol.scrollHeight;
}

function renderPlayers() {
    const engineName = `Wombat (${evalName()}, ${$('threads').value}T, ${(+$('movetime').value / 1000)} s/move)`;
    const names = mode() === 'analyze'
        ? { white: 'White', black: 'Black' }
        : { [humanColor]: 'You', [humanColor === 'white' ? 'black' : 'white']: engineName };
    const top = orientation === 'white' ? 'black' : 'white';
    $('player-top').textContent = names[top];
    $('player-bottom').textContent = names[orientation];
}

function setStatus(text, error = false) {
    $('status').textContent = text;
    $('status').classList.toggle('error', error);
}

function newGame(color, fen = START_FEN) {
    stopSearch();
    game = new Chess(fen);
    startFen = fen;
    history = [];
    humanColor = color;
    orientation = color;
    $('promotion').hidden = true;
    clearInfo();
    setEvalBar(0, '0.0');
    if (engineReady) uci('ucinewgame');
    update();
}

function undo() {
    stopSearch();
    // In play mode take back to the last position where it is the human's turn
    do {
        if (!history.length) break;
        game.undo();
        history.pop();
    } while (mode() === 'play' && (game.turn() === 'w') !== (humanColor === 'white'));
    update();
}

// ---------------------------------------------------------------------------------------------
// Controls

for (let t = 1; t <= MAX_THREADS; t++) $('threads').add(new Option(String(t), String(t)));
$('threads').value = String(Math.min(MAX_THREADS, 4));

$('new-white').onclick = () => { $('mode').value = 'play'; newGame('white'); };
$('new-black').onclick = () => { $('mode').value = 'play'; newGame('black'); };
$('undo').onclick = undo;
$('flip').onclick = () => {
    orientation = orientation === 'white' ? 'black' : 'white';
    board.set({ orientation });
    $('evalbar').classList.toggle('flipped', orientation === 'black');
    renderPlayers();
};
$('mode').onchange = () => { stopSearch(); update(); };
$('movetime').onchange = renderPlayers;
for (const id of ['threads', 'hash', 'nnue']) $(id).onchange = () => {
    stopSearch();
    applyOptions();
    if (id === 'nnue') uci('ucinewgame'); // don't reuse TT scores from the other evaluation
    renderPlayers();
    update();
};
$('fen-form').onsubmit = e => {
    e.preventDefault();
    const fen = $('fen').value.trim();
    try {
        new Chess(fen);
    } catch (err) {
        return setStatus('Invalid FEN: ' + err.message, true);
    }
    const g = new Chess(fen);
    newGame(g.turn() === 'w' ? 'white' : 'black', fen);
};
$('copy-pgn').onclick = async () => {
    const g = new Chess(startFen);
    if (startFen !== START_FEN) {
        g.setHeader('SetUp', '1');
        g.setHeader('FEN', startFen);
    }
    for (const m of history) g.move(m.san);
    await navigator.clipboard.writeText(g.pgn());
    $('copy-pgn').textContent = 'Copied';
    setTimeout(() => ($('copy-pgn').textContent = 'Copy PGN'), 1200);
};

renderPlayers();
update();
startEngine();
