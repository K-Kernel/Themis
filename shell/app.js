// The page logic. Read /schema once, then ask /solve for a pick every time a
// weight or the method changes, and redraw.
//
// Skeleton: the four functions marked TODO are yours, in order. buildAxes,
// weights and method are written. drawMap, drawFront, renderDetail and fmt come
// from draw.js. Delete this note when the TODOs are done.

let schema = null;  // the /schema answer
let current = null; // the latest /solve answer
let latest = 0;     // number of the newest /solve request sent

// TODO 1. Runs once when the page loads.
//  - const res = await fetch('/schema'); then schema = await res.json();
//  - put schema.title into document.title and into the #title element (textContent)
//  - call buildSliders() and buildAxes()
//  - give both method radio buttons, document.querySelectorAll('input[name=method]'),
//    a 'change' listener that calls solve
//  - finally await solve(), so the page opens with a pick
async function init() {}

// TODO 2. One slider per objective inside #sliders, in objective order:
//    <label><span>NAME</span><input type="range"><output>5</output></label>
//  - build each element with document.createElement; set the input's type, min, max,
//    step and value to 'range', 0, 10, 1 and 5 (Object.assign(input, {...}) does all five)
//  - label.append(name, input, output), then append the label to #sliders
//  - on the input's 'input' event, copy input.value into the output, then call solve()
function buildSliders() {}

// Fills the two axis menus (objective 0 against objective 1 to start) and redraws
// the chart when either changes.
function buildAxes() {
  const n = schema.objectives.length;
  for (const [id, start] of [['x-axis', 0], ['y-axis', Math.min(1, n - 1)]]) {
    const select = document.getElementById(id);
    schema.objectives.forEach((o, k) => select.add(new Option(o.name, k, false, k === start)));
    select.addEventListener('change', show);
  }
}

// The slider values in objective order, and the chosen method.
function weights() {
  return [...document.querySelectorAll('#sliders input')].map((s) => Number(s.value));
}

function method() {
  return document.querySelector('input[name=method]:checked').value;
}

// TODO 3. Ask the server for a pick.
//  - const id = ++latest;  numbers this request
//  - const res = await fetch('/solve', { method: 'POST',
//        headers: { 'Content-Type': 'application/json' },
//        body: JSON.stringify({ weights: weights(), method: method() }) });
//  - const answer = await res.json();
//  - if (id !== latest) return;  a newer request went out while this one was in
//    flight, and without this line a slow old answer could overwrite the new one
//  - if (!res.ok): put answer.error into #status and return; the last pick stays shown
//  - otherwise empty #status, store the answer in current and call show()
async function solve() {}

// TODO 4. Draw the current answer. Return at once while current is null (the axis
// menus can call this before the first answer arrives).
//  - #pick: empty it (replaceChildren()), then add a <strong> holding current.label
//    and one <div> per objective reading "NAME: VALUE", the value as fmt(current.scores[k])
//  - renderDetail(document.getElementById('detail'), current.detail)
//  - drawMap(document.getElementById('map'), schema, current)
//  - drawFront(document.getElementById('front'), schema, xk, yk, current), where xk and
//    yk are Number(...) of the #x-axis and #y-axis values
function show() {}

init();
