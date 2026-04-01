# NeuroCorrelation
NeuroCorrelation is a spiking neural network simulation project that explores the concept of Spike Timing Dependent Plasticity (STDP). It is written in C++ and utilizes OpenGL for rendering. Runs native or on web.

## [Try live in browser]( https://axelwickman.com/articles/neurocorrelation?utm_source=gh_neurocorrelation )

![Simulation .gif](https://user-images.githubusercontent.com/1360495/95690086-7adbbe00-0c15-11eb-8d08-820fe2f50eb0.gif)

## About STDP
Spike Timing Dependent Plasticity (STDP) is a biological process that adjusts the strength of connections between neurons in the brain, known as synapses, based on the timing of neuronal spikes. STDP is based on the principle that the synaptic strength increases (potentiation) if a presynaptic neuron's spike precedes a postsynaptic neuron's spike, and decreases (depression) if the order is reversed. This timing-dependent change in synaptic efficacy is crucial for various neural processes, including learning and memory. STDP helps in forming and reinforcing pathways in the neural network that are essential for associative learning.

This simulation shows how correlated input, over time form connection, while uncorrelated neurons lose their connection - Neurons that fire together, wire together.

## Getting Started

### Prerequisites
- Docker
- Git
- Node.js and npm (for the web build)

### Installation
1. Clone the repository and initialize submodules:
```bash
  git clone https://github.com/Axelwickm/NeuroCorrelation.git
  cd NeuroCorrelation
  git submodule update --init --recursive
  ```
2. Build the Docker container: `docker build -t neurocorrelation . `
3. Allow local Docker to display GUI (for Unix-like systems): `xhost +local:docker`
4. Run the Docker container: `docker run -it --rm --name neurocorrelation -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix --device /dev/dri:/dev/dri --gpus all --ipc=host neurocorrelation`

## Web Build

The browser build lives under [`web/`](web) and uses Dockerized Emscripten to compile the existing C++ app to WebAssembly, then serves it through a small Vite example app.

### One-time setup

Install the web workspace dependencies:

```bash
cd web
npm install
```

### Build only the wasm package

This compiles the C++ sources to `web/package/dist/`:

```bash
cd web
npm run build:wasm
```

Output files:

- `web/package/dist/neurocorrelation.mjs`
- `web/package/dist/neurocorrelation.wasm`
- `web/package/dist/neurocorrelation.data`

### Run the browser example locally

This rebuilds the wasm bundle first, then starts the Vite dev server for the example app:

```bash
cd web
npm run dev
```

Open the URL printed by Vite, typically `http://localhost:5173`.

### Production build for the example app

This builds both the wasm package and the example site:

```bash
cd web
npm run build
```

The generated example site is written to `web/example/dist/`.

## Controls

- **Right Drag**: Navigate
- **Right Click + Ctrl**: Hide interface.
- **Left Click**: Toggle selection of a neuron.
- **Space**: Pause/Resume the simulation.
- **'.' (Period)**: Speed up time.
- **',' (Comma)**: Slow down time.
- **C**: Switch to the next camera mode.
- **M**: Switch to the next rendering mode.
- **N**: Clear neural activities.
- **Esc**: Exit the program.

### Camera Modes
- **Mouse Look**: Navigate the camera using the mouse.
- **WASD Keys**: Move the camera (applicable in Mouse Look mode).

- **Orbit and Orbit Momentum Modes**:
  - **Left Click + Drag**: Pan the camera around the scene.
  - **Scroll Wheel**: Move the camera closer or further away.

### Changing the simulation

The size of the network, and it's inputs, are defined in the `main.cpp` file.


## Resources
- [Swedish original essay](NeuroCorrelation_swedish_original.pdf)
- [English translated essay](NeuroCorrelation_english.pdf)

## License
[MIT](https://choosealicense.com/licenses/mit/)
