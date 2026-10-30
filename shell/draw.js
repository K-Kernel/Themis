// Drawing helpers for the Themis shell. Plain SVG built in the browser, no libraries.
// app.js calls drawMap, drawFront, renderDetail and fmt; nothing here talks to the server.

const SVG_NS = 'http://www.w3.org/2000/svg';

function svgEl(parent, name, attrs = {}) {
  const e = document.createElementNS(SVG_NS, name);
  for (const [k, v] of Object.entries(attrs)) e.setAttribute(k, v);
  parent.appendChild(e);
  return e;
}

// 22187.2 -> "22,187", 0.4613 -> "0.46", 12 -> "12"
function fmt(v) {
  if (typeof v !== 'number') return String(v);
  return Math.abs(v) >= 100 ? Math.round(v).toLocaleString('en-GB') : String(+v.toFixed(2));
}

function hasPosition(p) {
  return typeof p.lat === 'number' && typeof p.lon === 'number';
}

// Equirectangular projection measured in degrees: x = lon + 180, y = 90 - lat.
function project(p) {
  return [p.lon + 180, 90 - p.lat];
}

let landPath = null; // built once from LAND (world.js), reused on every redraw

// The map: land, a line from every entity to the pick, the front members and the
// entities. The view frames everything that has a position, at a 2:1 ratio.
function drawMap(svg, schema, pick) {
  svg.replaceChildren();
  const placed = [...schema.entities, ...schema.front].filter(hasPosition).map(project);
  let [minX, minY, maxX, maxY] = [0, 0, 360, 180];
  if (placed.length) {
    minX = Math.min(...placed.map((p) => p[0]));
    maxX = Math.max(...placed.map((p) => p[0]));
    minY = Math.min(...placed.map((p) => p[1]));
    maxY = Math.max(...placed.map((p) => p[1]));
  }
  let w = Math.max(maxX - minX + 32, 40); // room for the labels drawn right of each marker
  let h = Math.max(maxY - minY + 20, 20);
  if (w < 2 * h) w = 2 * h;
  else h = w / 2;
  const cx = (minX + maxX) / 2 + 5;
  const cy = (minY + maxY) / 2;
  svg.setAttribute('viewBox', `${cx - w / 2} ${cy - h / 2} ${w} ${h}`);
  const r = w / 160; // marker size in degrees, so markers keep their size on screen

  if (landPath === null) {
    landPath = LAND.map((ring) =>
      'M' + ring.map(([lon, lat]) => `${(lon + 180).toFixed(1)} ${(90 - lat).toFixed(1)}`).join('L') + 'Z',
    ).join('');
  }
  svgEl(svgEl(svg, 'g', { class: 'land' }), 'path', { d: landPath });

  const target = schema.front.find((f) => f.candidate === pick.candidate);
  if (target && hasPosition(target)) {
    const [tx, ty] = project(target);
    for (const e of schema.entities.filter(hasPosition)) {
      const [ex, ey] = project(e);
      svgEl(svg, 'line', { x1: ex, y1: ey, x2: tx, y2: ty, class: 'trip' });
    }
  }
  for (const f of schema.front.filter(hasPosition)) {
    const [x, y] = project(f);
    const isPick = f.candidate === pick.candidate;
    const dot = svgEl(svg, 'circle', { cx: x, cy: y, r: isPick ? 1.6 * r : r, class: isPick ? 'pick' : 'candidate' });
    svgEl(dot, 'title').textContent = f.label;
  }
  for (const e of schema.entities.filter(hasPosition)) {
    const [x, y] = project(e);
    svgEl(svg, 'rect', { x: x - r, y: y - r, width: 2 * r, height: 2 * r, class: 'entity' });
    svgEl(svg, 'text', { x: x + 1.5 * r, y: y + r, 'font-size': 3 * r, class: 'label' }).textContent = e.label;
  }
  if (target && hasPosition(target)) {
    const [x, y] = project(target);
    svgEl(svg, 'text', { x: x + 2 * r, y: y - 1.5 * r, 'font-size': 4 * r, 'font-weight': 600, class: 'label' })
      .textContent = target.label;
  }
}

// The front as a scatter plot of objective xk against objective yk, with the pick marked.
function drawFront(svg, schema, xk, yk, pick) {
  svg.replaceChildren();
  const W = 440, H = 260, m = { l: 64, r: 16, t: 14, b: 44 };
  svg.setAttribute('viewBox', `0 0 ${W} ${H}`);
  const xs = schema.front.map((f) => f.scores[xk]);
  const ys = schema.front.map((f) => f.scores[yk]);
  const [x0, x1] = [Math.min(...xs), Math.max(...xs)];
  const [y0, y1] = [Math.min(...ys), Math.max(...ys)];
  const sx = (v) => m.l + (x1 === x0 ? 0.5 : (v - x0) / (x1 - x0)) * (W - m.l - m.r);
  const sy = (v) => H - m.b - (y1 === y0 ? 0.5 : (v - y0) / (y1 - y0)) * (H - m.t - m.b);

  svgEl(svg, 'line', { x1: m.l, y1: H - m.b, x2: W - m.r, y2: H - m.b, class: 'axis' });
  svgEl(svg, 'line', { x1: m.l, y1: m.t, x2: m.l, y2: H - m.b, class: 'axis' });
  svgEl(svg, 'text', { x: m.l, y: H - m.b + 16, class: 'tick' }).textContent = fmt(x0);
  svgEl(svg, 'text', { x: W - m.r, y: H - m.b + 16, 'text-anchor': 'end', class: 'tick' }).textContent = fmt(x1);
  svgEl(svg, 'text', { x: m.l - 6, y: H - m.b, 'text-anchor': 'end', class: 'tick' }).textContent = fmt(y0);
  svgEl(svg, 'text', { x: m.l - 6, y: m.t + 8, 'text-anchor': 'end', class: 'tick' }).textContent = fmt(y1);
  const better = (k) => (schema.objectives[k].direction === 'min' ? 'lower is better' : 'higher is better');
  svgEl(svg, 'text', { x: (m.l + W - m.r) / 2, y: H - 8, 'text-anchor': 'middle', class: 'axis-title' })
    .textContent = `${schema.objectives[xk].name} (${better(xk)})`;
  svgEl(svg, 'text', {
    x: 14, y: (m.t + H - m.b) / 2, 'text-anchor': 'middle', class: 'axis-title',
    transform: `rotate(-90 14 ${(m.t + H - m.b) / 2})`,
  }).textContent = `${schema.objectives[yk].name} (${better(yk)})`;

  for (const f of schema.front) {
    const isPick = f.candidate === pick.candidate;
    const dot = svgEl(svg, 'circle', {
      cx: sx(f.scores[xk]), cy: sy(f.scores[yk]), r: isPick ? 7 : 4.5, class: isPick ? 'pick' : 'candidate',
    });
    svgEl(dot, 'title').textContent =
      f.label + ': ' + f.scores.map((v, k) => `${schema.objectives[k].name} ${fmt(v)}`).join(', ');
  }
  const target = schema.front.find((f) => f.candidate === pick.candidate);
  if (target) {
    svgEl(svg, 'text', { x: sx(target.scores[xk]) + 10, y: sy(target.scores[yk]) - 8, 'font-weight': 600, class: 'label' })
      .textContent = target.label;
  }
}

// Any detail object. A key holding a list of objects becomes a table, one column per
// key of the first object; lists inside a cell are joined with " -> ". Anything else
// is shown as key: value.
function renderDetail(box, detail) {
  box.replaceChildren();
  if (detail === null || detail === undefined) return;
  for (const [key, value] of Object.entries(detail)) {
    if (Array.isArray(value) && value.length > 0 && typeof value[0] === 'object') {
      const title = document.createElement('h3');
      title.textContent = key;
      const table = document.createElement('table');
      // text columns first, then numbers; keys arrive in alphabetical order
      const columns = Object.keys(value[0]).sort(
        (a, b) => (typeof value[0][a] === 'number') - (typeof value[0][b] === 'number'));
      const head = table.insertRow();
      for (const c of columns) head.appendChild(document.createElement('th')).textContent = c;
      for (const item of value) {
        const row = table.insertRow();
        for (const c of columns) {
          const v = item[c];
          row.insertCell().textContent = Array.isArray(v) ? v.join(' -> ') : fmt(v);
        }
      }
      box.append(title, table);
    } else {
      const line = document.createElement('p');
      line.textContent = `${key}: ${JSON.stringify(value)}`;
      box.appendChild(line);
    }
  }
}
