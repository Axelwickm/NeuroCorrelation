import mountNeuroCorrelation from "@neurocorrelation/web";
import "./style.css";

const app = document.querySelector("#app");
app.innerHTML = `
  <main class="page">
    <section class="panel">
      <h1>NeuroCorrelation - Web</h1>
      <p class="summary">
        A spiking neural network simulation exploring spike-timing-dependent plasticity,
        where correlated neurons strengthen connections and uncorrelated ones lose them.
      </p>
      <section class="viewer" id="viewer">
        <div class="status" id="status">Loading browser build…</div>
        <canvas id="canvas"></canvas>
      </section>
      <div class="help">
        <p class="help-title">Controls</p>
        <ul class="shortcut-list">
          <li><span>Drag</span><span>Move the camera</span></li>
          <li><span>Scroll</span><span>Zoom in or out</span></li>
          <li><span>C</span><span>Switch camera mode</span></li>
          <li><span>M</span><span>Switch render mode</span></li>
          <li><span>Space</span><span>Pause or resume</span></li>
          <li><span>N</span><span>Clear neural activity</span></li>
          <li><span>Click</span><span>Select a neuron or toggle an input</span></li>
        </ul>
      </div>
    </section>
    <p class="footnote">
      If the scene does not react immediately, click inside the canvas first.
    </p>
  </main>
`;

const viewer = document.querySelector("#viewer");
const canvas = document.querySelector("#canvas");
const status = document.querySelector("#status");

const syncCanvasSize = () => {
  const width = viewer.clientWidth;
  const height = viewer.clientHeight;
  if (width === 0 || height === 0) return;

  canvas.style.width = `${width}px`;
  canvas.style.height = `${height}px`;
  if (canvas.width !== width) canvas.width = width;
  if (canvas.height !== height) canvas.height = height;
};

const viewerResizeObserver = new ResizeObserver(() => {
  syncCanvasSize();
});

viewerResizeObserver.observe(viewer);
syncCanvasSize();

try {
  await mountNeuroCorrelation({
    canvas,
    preset: "STANDARD",
    print: (message) => console.log(message),
    printErr: (message) => console.error(message),
  });
  status.remove();
} catch (error) {
  status.textContent =
    "The browser build failed to start. Check the console for details.";
  status.dataset.error = "true";
  console.error(error);
}
