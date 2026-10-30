// The page logic: read /schema once, then ask /solve for a pick every time a
// weight or the method changes, and redraw.

let schema = null;  // the /schema answer
let current = null; // the latest /solve answer
let latest = 0;     // number of the newest /solve request sent

async function init() {
  const res = await fetch('/schema');
  schema = await res.json();
  document.title = schema.title;
  document.getElementById('title').textContent = schema.title;
  buildSliders();
  buildAxes();
  for (const input of document.querySelectorAll('input[name=method]')) {
    input.addEventListener('change', solve);
  }
  await solve();
}

function buildSliders() {
  const box = document.getElementById('sliders');
  for (const objective of schema.objectives) {
    const label = document.createElement('label');
    const name = document.createElement('span');
    name.textContent = objective.name;
    const input = document.createElement('input');
    Object.assign(input, { type: 'range', min: 0, max: 10, step: 1, value: 5 });
    const shown = document.createElement('output');
    shown.textContent = input.value;
    input.addEventListener('input', () => {
      shown.textContent = input.value;
      solve();
    });
    label.append(name, input, shown);
    box.appendChild(label);
  }
}

function buildAxes() {
  const n = schema.objectives.length;
  for (const [id, start] of [['x-axis', 0], ['y-axis', Math.min(1, n - 1)]]) {
    const select = document.getElementById(id);
    schema.objectives.forEach((o, k) => select.add(new Option(o.name, k, false, k === start)));
    select.addEventListener('change', show);
  }
}

function weights() {
  return [...document.querySelectorAll('#sliders input')].map((s) => Number(s.value));
}

function method() {
  return document.querySelector('input[name=method]:checked').value;
}

async function solve() {
  const id = ++latest;
  const res = await fetch('/solve', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ weights: weights(), method: method() }),
  });
  const answer = await res.json();
  if (id !== latest) return; // a newer request went out while this one was in flight
  const status = document.getElementById('status');
  if (!res.ok) {
    status.textContent = answer.error;
    return;
  }
  status.textContent = '';
  current = answer;
  show();
}

function show() {
  if (current === null) return;
  const pick = document.getElementById('pick');
  pick.replaceChildren();
  const name = document.createElement('strong');
  name.textContent = current.label;
  pick.appendChild(name);
  schema.objectives.forEach((o, k) => {
    const line = document.createElement('div');
    line.textContent = `${o.name}: ${fmt(current.scores[k])}`;
    pick.appendChild(line);
  });
  renderDetail(document.getElementById('detail'), current.detail);
  drawMap(document.getElementById('map'), schema, current);
  const xk = Number(document.getElementById('x-axis').value);
  const yk = Number(document.getElementById('y-axis').value);
  drawFront(document.getElementById('front'), schema, xk, yk, current);
}

init();
